#include "sqpcheader.h"
#include "squirrel.h"
#include <memory>
#include "sqvm.h"
#include "sqstring.h"
#include "sqjit.h"
#include "sqfuncproto.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/InstSimplifyPass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;
using namespace orc;


class SQJitContext
{
	HSQUIRRELVM _vm;
	sqvector<std::pair<SQObject,std::unique_ptr<Function>>> _jitfuncs;

	LLVMContext _context;
	std::unique_ptr<Module> _module;

	class SQJit
	{
		llvm::orc::ExecutionSession ES;
		const llvm::DataLayout DL;
		std::unique_ptr<llvm::TargetMachine> TM;
		std::map<llvm::orc::VModuleKey, std::shared_ptr<llvm::orc::SymbolResolver>> Resolvers;

		llvm::orc::LegacyRTDyldObjectLinkingLayer ObjectLayer;
		llvm::orc::LegacyIRCompileLayer<decltype(ObjectLayer), llvm::orc::SimpleCompiler> CompileLayer;

		using OptimizeFunction = std::function<std::unique_ptr<llvm::Module>(std::unique_ptr<llvm::Module>)>;
		llvm::orc::LegacyIRTransformLayer<decltype(CompileLayer), OptimizeFunction> OptimizeLayer;

		std::unique_ptr<llvm::orc::JITCompileCallbackManager> CompileCallbackManager;
		llvm::orc::LegacyCompileOnDemandLayer<decltype(OptimizeLayer)> CODLayer;

	public:
		SQJit()
			: TM(EngineBuilder().selectTarget()), DL(TM->createDataLayout()),
			ObjectLayer(ES,
						 [this](VModuleKey K) {
							 return LegacyRTDyldObjectLinkingLayer::Resources{
								 std::make_shared<SectionMemoryManager>(),
								 Resolvers[K]
							 };
						 }),
			CompileLayer(ObjectLayer, SimpleCompiler(*TM)),
			OptimizeLayer(CompileLayer,
						[this](std::unique_ptr<Module> M) {
							return OptimizeModule(std::move(M));
						}),
			CompileCallbackManager(cantFail(createLocalCompileCallbackManager(TM->getTargetTriple(), ES, 0))),
			CODLayer(ES, OptimizeLayer,
					  [&](VModuleKey K) {
						  return Resolvers[K];
					  },
					  [&](VModuleKey K, std::shared_ptr<SymbolResolver> R) {
						  Resolvers[K] = std::move(R);
					  },
					  [](Function &F) {
						  return std::set<Function *>({&F});
					  },
					  *CompileCallbackManager,
					  createLocalIndirectStubsManagerBuilder(TM->getTargetTriple()))
		{
			sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
		}

		VModuleKey AddModule(std::unique_ptr<Module> M)
		{
			// Create a new VModuleKey.
			auto K = ES.allocateVModule();

			// Build a resolver and associate it with the new key.
			Resolvers[K] = createLegacyLookupResolver(
				ES,
				[this](const std::string &Name) -> JITSymbol {
					if (auto Sym = CompileLayer.findSymbol(Name, false))
						return Sym;
					else if (auto Err = Sym.takeError())
						return std::move(Err);
					if (auto SymAddr = RTDyldMemoryManager::getSymbolAddressInProcess(Name))
						return JITSymbol(SymAddr, JITSymbolFlags::Exported);
					return nullptr;
				},
				[](llvm::Error Err) { 
					cantFail(std::move(Err), "lookupFlags failed"); 
				});

			// Add the module to the JIT with the new key.
			cantFail(CODLayer.addModule(K, std::move(M)));
			return K;
		}

		void RemoveModule(VModuleKey K) {
			cantFail(CODLayer.removeModule(K));
		}

		JITSymbol FindSymbol(const std::string Name)
		{
			std::string MangledName;
			raw_string_ostream MangledNameStream(MangledName);
			Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
			return CODLayer.findSymbol(MangledNameStream.str(), true);
		}

	private:
		static std::unique_ptr<Module>
		OptimizeModule(std::unique_ptr<Module> M)
		{
			auto FuncPass = llvm::make_unique<legacy::FunctionPassManager>(M);

			FuncPass->add(createInstructionCombiningPass());
			FuncPass->add(createInstSimplifyLegacyPass());
			FuncPass->add(createReassociatePass());
			FuncPass->add(createGVNPass());
			FuncPass->add(createCFGSimplificationPass());
			FuncPass->doInitialization();

			for ( auto &F : *M )
				FuncPass->run( F );

			return M;
		}
	};
	friend SQJit;
	std::unique_ptr<SQJit> _jit;

	StructType *_sqobjectType;

public:
	SQJitContext(HSQUIRRELVM v)
		: _vm(v), _module(make_unique<Module>("squirt", _context)), _jit(make_unique<SQJit>()) {}

	void Init(void);
	Function *BuildFunc(const SQObject &name,SQFunctionProto *func);

	SQJITFUNC GetFuncPtr(Function*);
};

void SQJitContext::Init( void )
{
	InitializeNativeTarget();
	InitializeAllAsmParsers();
	InitializeAllAsmPrinters();

	_sqobjectType = StructType::get(Type::getIntNPtrTy(_context, sizeof(uintptr_t)), Type::getInt32Ty(_context));
}

SQJITFUNC SQJitContext::GetFuncPtr(Function *f)
{
	JITSymbol sym = _jit->FindSymbol(f->getName());
	if ( sym.takeError() )
		return NULL;
	return (SQJITFUNC)*sym.getAddress();
}

Function *SQJitContext::BuildFunc(const SQObject &name, SQFunctionProto *func)
{
	static int FuncCounter = 0;

	for ( auto &kv : _jitfuncs )
	{
		if (sq_isstring(kv.first) && sq_isstring(name))
		{
			if(!scstrcmp(_stringval(name), _stringval(kv.first)))
				return kv.second.get();
		}
	}

	char nameBuf[256];
	if (sq_isstring(name))
	{
		SQChar buf[256];
		scsprintf(buf, sizeof(buf), "%s", _stringval(name));

	#ifdef SQUNICODE
		int numConverted;
		wcstombs_s(&numConverted, nameBuf, buf, sizeof(nameBuf));
		Assert(numConverted == sizeof(nameBuf));
	#else
		strcpy_s(nameBuf, buf);
	#endif
	}
	else
	{
		sprintf_s(nameBuf, "_unnamed_func_%x", ++FuncCounter);
	}

	FunctionType *ft = FunctionType::get(Type::getInt1Ty(_context), {StructType::get(_context)}, false);
	Function *f = Function::Create(ft, GlobalValue::ExternalLinkage, nameBuf, _module.get());
	
	if (verifyFunction(*f))
	{
		return NULL;
	}
	else
	{
		_jit->AddModule(std::move(_module));
		_module = llvm::make_unique<Module>("squirt", _context);
		std::unique_ptr<Function> jitfunc = make_unique<Function>(*f);
		_jitfuncs.push_back(std::make_pair(name, std::move(jitfunc)));
		return f;
	}
}


SQJITFUNC sq_jitcompile(HSQUIRRELVM v,const SQObject &name,SQFunctionProto *func)
{
	SQJitContext context(v);
	context.Init();
	Function *f = context.BuildFunc(name,func);
	return context.GetFuncPtr(f);
}


SQBool sq_getjitenabled(HSQUIRRELVM v)
{
	return v->_jitenabled;
}

void sq_setjitenabled(HSQUIRRELVM v,SQBool enable)
{
	v->_jitenabled = enable;
}
