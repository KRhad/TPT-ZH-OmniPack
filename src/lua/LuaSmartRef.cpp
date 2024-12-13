#ifdef LUACONSOLE
#include "LuaSmartRef.h"

LuaSmartRef::LuaSmartRef():
	ref(LUA_REFNIL)
{

}

LuaSmartRef::~LuaSmartRef()
{
	if (inited)
		Clear();
}

void LuaSmartRef::Init(lua_State *l)
{
	tpt_lua_getmainthread(l);
	rootl = lua_tothread(l, -1);
	lua_pop(l, 1);
	inited = true;
}

void LuaSmartRef::Clear()
{
	luaL_unref(rootl, LUA_REGISTRYINDEX, ref);
	ref = LUA_REFNIL;
	inited = false;
}

void LuaSmartRef::Assign(lua_State *l, int index)
{
	if (!inited)
		Init(l);
	if (index < 0)
	{
		index = lua_gettop(l) + index + 1;
	}
	Clear();
	lua_pushvalue(l, index);
	ref = luaL_ref(l, LUA_REGISTRYINDEX);
}

int LuaSmartRef::Push(lua_State *l)
{
	if (!inited)
		Init(l);
	lua_rawgeti(l, LUA_REGISTRYINDEX, ref);
	return lua_type(l, -1);
}

#endif
