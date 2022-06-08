#include "angelscript.h"
#include "as_vector.h"

#include "mathlib/vector.h"
#include "mathlib/mathlib.h"

//=============================================================================
//
// Vector
// 
//=============================================================================

static void VectorConstruct( Vector *ptr )
{
	new( ptr ) Vector();
}

static void VectorConstruct( float x, float y, float z, Vector *ptr )
{
	new( ptr ) Vector( x, y, z );
}

static void VectorCopyConstruct( Vector const &other, Vector *ptr )
{
	new( ptr ) Vector( other );
}

static void VectorListConstruct( float *list, Vector *ptr )
{
	new( ptr ) Vector( list[0], list[1], list[2] );
}

static void VectorDestruct( Vector *ptr )
{
	ptr->~Vector();
}

static float *VectorOpIndex( asUINT i, Vector &vec )
{
	if ( i >= 3 )
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException( "Out of range" );

		// Return a null pointer
		return 0;
	}

	return &(vec[i]);
}

static Vector &VectorOpAddAssign( Vector const &other, Vector &vec )
{
	return vec += other;
}

static Vector &VectorOpAddAssignFloat( float value, Vector &vec )
{
	return vec += value;
}

static Vector &VectorOpSubAssign( Vector const &other, Vector &vec )
{
	return vec -= other;
}

static Vector &VectorOpSubAssignFloat( float value, Vector &vec )
{
	return vec -= value;
}

static Vector &VectorOpMulAssign( Vector const &other, Vector &vec )
{
	return vec *= other;
}

static Vector &VectorOpMulAssignFloat( float value, Vector &vec )
{
	return vec *= value;
}

static Vector &VectorOpDivAssign( Vector const &other, Vector &vec )
{
	return vec /= other;
}

static Vector &VectorOpDivAssignFloat( float value, Vector &vec )
{
	return vec /= value;
}

static Vector VectorOpNeg( Vector const &vec )
{
	return -vec;
}

static Vector VectorOpAdd( Vector const &vec, Vector const &other )
{
	return vec + other;
}

static Vector VectorOpSub( Vector const &vec, Vector const &other )
{
	return vec - other;
}

static Vector VectorOpMul( Vector const &vec, Vector const &other )
{
	return vec * other;
}

static Vector VectorOpMulFloat( Vector const &vec, float value )
{
	return vec * value;
}

static Vector VectorOpDiv( Vector const &vec, Vector const &other )
{
	return vec / other;
}

static Vector VectorOpDivFloat( Vector const &vec, float value )
{
	return vec / value;
}

static float VectorLengthSqr( Vector const &vec )
{
	return vec.LengthSqr();
}

static float VectorLength2D( Vector const &vec )
{
	return vec.Length2D();
}

static float VectorLength2DSqr( Vector const &vec )
{
	return vec.Length2DSqr();
}

static bool VectorIsZero( Vector const &vec )
{
	return vec.IsZero();
}

void RegisterVector3( asIScriptEngine *engine )
{
	int r = 0;

#ifdef AS_CAN_USE_CPP11
	r = engine->RegisterObjectType( "Vector3", sizeof( Vector ), asOBJ_VALUE | asGetTypeTraits<Vector>() ); Assert( r >= 0 );
#else
	r = engine->RegisterObjectType( "Vector3", sizeof( Vector ), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK ); Assert( r >= 0 );
#endif

	// Construction
	r = engine->RegisterObjectBehaviour( "Vector3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR( VectorConstruct, (Vector *), void ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Vector3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTIONPR( VectorConstruct, (float, float, float, Vector *), void ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Vector3", asBEHAVE_CONSTRUCT, "void f(const Vector3 &in)", asFUNCTION( VectorCopyConstruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Vector3", asBEHAVE_DESTRUCT, "void f()", asFUNCTION( VectorDestruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Vector3", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float}", asFUNCTION( VectorListConstruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );

	// Set/Get
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opAssign(const Vector3 &in)", asMETHOD(Vector, operator=), asCALL_THISCALL ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "const float &opIndex(uint) const", asFUNCTION( VectorOpIndex ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "float &opIndex(uint)", asFUNCTION( VectorOpIndex ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );

	// Assign arithmetic
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opAddAssign(const Vector3 &in)", asFUNCTION( VectorOpAddAssign ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opAddAssign(float)", asFUNCTION( VectorOpAddAssignFloat ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opSubAssign(const Vector3 &in)", asFUNCTION( VectorOpSubAssign ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opSubAssign(float)", asFUNCTION( VectorOpSubAssignFloat ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opMulAssign(const Vector3 &in)", asFUNCTION( VectorOpMulAssign ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opMulAssign(float)", asFUNCTION( VectorOpMulAssignFloat ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opDivAssign(const Vector3 &in)", asFUNCTION( VectorOpDivAssign ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 &opDivAssign(float)", asFUNCTION( VectorOpDivAssignFloat ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );

	// Arithmetic
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 opNeg()", asFUNCTION( VectorOpNeg ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 opAdd(const Vector3 &in)", asFUNCTION( VectorOpAdd ), asCALL_CDECL_OBJFIRST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 opSub(const Vector3 &in)", asFUNCTION( VectorOpSub ), asCALL_CDECL_OBJFIRST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 opMul(const Vector3 &in)", asFUNCTION( VectorOpMul ), asCALL_CDECL_OBJFIRST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 opMul(float)", asFUNCTION( VectorOpMulFloat ), asCALL_CDECL_OBJFIRST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 opDiv(const Vector3 &in)", asFUNCTION( VectorOpDiv ), asCALL_CDECL_OBJFIRST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 opDiv(float)", asFUNCTION( VectorOpDivFloat ), asCALL_CDECL_OBJFIRST ); Assert( r >= 0 );

	// Methods
	r = engine->RegisterObjectMethod( "Vector3", "void Negate()", asMETHOD( Vector, Negate ), asCALL_THISCALL ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "float Length() const", asFUNCTIONPR( VectorLength, ( const Vector & ), float ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "float Length2D() const", asFUNCTION( VectorLength2D ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "float LengthSqr() const", asFUNCTION( VectorLengthSqr ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "float Length2DSqr() const", asFUNCTION( VectorLength2DSqr ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "float NormalizeInPlace()", asMETHOD( Vector, NormalizeInPlace ), asCALL_THISCALL ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 Normalized() const", asMETHOD( Vector, Normalized ), asCALL_THISCALL ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "float Dot() const", asMETHOD( Vector, Dot ), asCALL_THISCALL ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Vector3", "Vector3 Cross(const Vector3 &in) const", asMETHOD( Vector, Cross ), asCALL_THISCALL ); Assert( r >= 0 );

	// Properties
	r = engine->RegisterObjectProperty( "Vector3", "float x", asOFFSET( Vector, x ) ); Assert( r >= 0 );
	r = engine->RegisterObjectProperty( "Vector3", "float y", asOFFSET( Vector, y ) ); Assert( r >= 0 );
	r = engine->RegisterObjectProperty( "Vector3", "float z", asOFFSET( Vector, z ) ); Assert( r >= 0 );
}

//=============================================================================
//
// Quaternion
// 
//=============================================================================

static void QuaternionConstruct( Quaternion *ptr )
{
	new( ptr ) Quaternion();
}

static void QuaternionConstruct( float x, float y, float z, float w, Quaternion *ptr )
{
	new( ptr ) Quaternion( x, y, z, w );
}

static void QuaternionCopyConstruct( Quaternion const &other, Quaternion *ptr )
{
	new( ptr ) Quaternion( other );
}

static void QuaternionListConstruct( float *list, Quaternion *ptr )
{
	new( ptr ) Quaternion( list[0], list[1], list[2], list[3] );
}

static void QuaternionDestruct( Quaternion *ptr )
{
	ptr->~Quaternion();
}

static float *QuaternionOpIndex( asUINT i, Quaternion &quat )
{
	if ( i >= 4 )
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException( "Out of range" );

		// Return a null pointer
		return 0;
	}

	return &( quat[i] );
}

void RegisterQuaternion( asIScriptEngine *engine )
{
	int r = 0;

#ifdef AS_CAN_USE_CPP11
	r = engine->RegisterObjectType( "Quaternion", sizeof( Quaternion ), asOBJ_VALUE | asGetTypeTraits<Quaternion>() ); Assert( r >= 0 );
#else
	r = engine->RegisterObjectType( "Quaternion", sizeof( Quaternion ), asOBJ_VALUE | asOBJ_APP_CLASS_CDK ); Assert( r >= 0 );
#endif

	// Construction
	r = engine->RegisterObjectBehaviour( "Quaternion", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR( QuaternionConstruct, (Quaternion *), void ), asCALL_CDECL_OBJLAST );
	r = engine->RegisterObjectBehaviour( "Quaternion", asBEHAVE_DESTRUCT, "void f()", asFUNCTION( QuaternionDestruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Quaternion", asBEHAVE_CONSTRUCT, "void f(const Quaternion &in)", asFUNCTION( QuaternionCopyConstruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Quaternion", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTIONPR( QuaternionConstruct, (float, float, float, float, Quaternion *), void ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Quaternion", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float, float}", asFUNCTION( QuaternionListConstruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );

	// Methods
	r = engine->RegisterObjectMethod( "Quaternion", "float &opIndex(uint)", asFUNCTION( QuaternionOpIndex ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod( "Quaternion", "const float &opIndex(uint) const", asFUNCTION( QuaternionOpIndex ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );

	// Properties
	r = engine->RegisterObjectProperty( "Quaternion", "float x", asOFFSET( Quaternion, x ) ); Assert( r >= 0 );
	r = engine->RegisterObjectProperty( "Quaternion", "float y", asOFFSET( Quaternion, y ) ); Assert( r >= 0 );
	r = engine->RegisterObjectProperty( "Quaternion", "float z", asOFFSET( Quaternion, z ) ); Assert( r >= 0 );
	r = engine->RegisterObjectProperty( "Quaternion", "float w", asOFFSET( Quaternion, w ) ); Assert( r >= 0 );
}

//=============================================================================
//
// matrix3x4_t
// 
//=============================================================================

static void MatrixConstruct( matrix3x4_t *ptr )
{
	new( ptr ) matrix3x4_t();
}

static void MatrixConstruct( Vector const &xAxis, Vector const &yAxis, Vector const &zAxis, Vector const &vOrigin, matrix3x4_t *ptr )
{
	new( ptr ) matrix3x4_t( xAxis, yAxis, zAxis, vOrigin );
}

static void MatrixCopyConstruct( matrix3x4_t const &other, matrix3x4_t *ptr )
{
	new( ptr ) matrix3x4_t( other );
}

static void MatrixListConstruct( float *list, matrix3x4_t *ptr )
{
	new( ptr ) matrix3x4_t( list[0], list[1], list[2], list[3], list[4], list[5], list[6], list[7], list[8], list[9], list[10], list[11] );
}

static void MatrixDestruct( matrix3x4_t *ptr )
{
	ptr->~matrix3x4_t();
}

static float *MatrixOpIndex( asUINT i, asUINT j, matrix3x4_t &mat )
{
	if ( i >= 3 || j >= 4 )
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException( "Out of range" );

		// Return a null pointer
		return 0;
	}

	return &( mat[i][j] );
}

void RegisterMatrix( asIScriptEngine *engine )
{
	int r = 0;

#ifdef AS_CAN_USE_CPP11
	r = engine->RegisterObjectType( "Matrix", sizeof( matrix3x4_t ), asOBJ_VALUE | asGetTypeTraits<matrix3x4_t>() ); Assert( r >= 0 );
#else
	r = engine->RegisterObjectType( "Matrix", sizeof( matrix3x4_t ), asOBJ_VALUE | asOBJ_APP_CLASS_CDK ); Assert( r >= 0 );
#endif

	// Construction
	r = engine->RegisterObjectBehaviour( "Matrix", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR( MatrixConstruct, (matrix3x4_t *), void ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Matrix", asBEHAVE_DESTRUCT, "void f()", asFUNCTION( MatrixDestruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Matrix", asBEHAVE_CONSTRUCT, "void f(const Matrix &in)", asFUNCTION( MatrixCopyConstruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Matrix", asBEHAVE_CONSTRUCT, "void f(const Vector3 &in, const Vector3 &in, const Vector3 &in, const Vector3 &in)", asFUNCTIONPR( MatrixConstruct, (Vector const &, Vector const &, Vector const &, Vector const &, matrix3x4_t *), void ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectBehaviour( "Matrix", asBEHAVE_LIST_CONSTRUCT, "void f(const int &in) {float, float, float, float, float, float, float, float, float, float, float, float}", asFUNCTION( MatrixListConstruct ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );

	// Methods
	r = engine->RegisterObjectMethod( "Matrix", "float &opIndex(uint, uint)", asFUNCTION( MatrixOpIndex ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
	r = engine->RegisterObjectMethod("Matrix", "const float &opIndex(uint, uint) const", asFUNCTION( MatrixOpIndex ), asCALL_CDECL_OBJLAST ); Assert( r >= 0 );
}

//=============================================================================
//=============================================================================

void RegisterValveScriptMath( asIScriptEngine *engine )
{
	RegisterVector3( engine );
	RegisterQuaternion( engine );
	RegisterMatrix( engine );
}
