#ifndef ANGELSCRIPT_VM_H
#define ANGELSCRIPT_VM_H

#ifdef _WIN32
#pragma once
#endif

extern IScriptVM *CreateAngelScriptVM( void );


extern void DestroyAngelScriptVM( IScriptVM *pVM );


#endif