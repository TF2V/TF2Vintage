#ifndef LUA_VECTOR_H
#define LUA_VECTOR_H

#if defined _WIN32
#pragma once
#endif

int lua_openvec3( lua_State *L );

Vector lua_tovec3byvalue( lua_State *L, int narg );
Vector *lua_tovec3( lua_State *L, int idx );
void lua_pushvec3( lua_State *L, Vector const *v );
Vector *luaL_checkvec3( lua_State *L, int narg );

#endif
