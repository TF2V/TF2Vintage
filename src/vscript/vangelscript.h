#ifndef VANGELSCRIPT_H
#define VANGELSCRIPT_H
#pragma once

extern IScriptVM *CreateAngelScriptVM( void );

extern void DestroyAngelScriptVM( IScriptVM *pVM );

#endif