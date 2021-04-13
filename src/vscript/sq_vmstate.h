#ifndef SQ_VMSTATE_H
#define SQ_VMSTATE_H

#ifdef _WIN32
#pragma once
#endif

void WriteSquirrelState( HSQUIRRELVM pVM, CUtlBuffer *pBuffer );
void ReadSquirrelState( HSQUIRRELVM pVM, CUtlBuffer *pBuffer );

#endif