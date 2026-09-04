#pragma once

#include "LuaCompat.h"

class LuaSmartRef
{
	int ref;
	lua_State *rootl;
	bool inited = false;

public:
	LuaSmartRef();
	~LuaSmartRef();
	void Init(lua_State *l);
	void Clear();
	void Assign(lua_State *l, int index); // Copies the value before getting reference, stack unchanged.
	int Push(lua_State *l); // Always pushes exactly one value, possibly nil.

	inline operator int() const
	{
		return ref;
	}

	inline operator lua_Integer() const
	{
		return (lua_Integer)ref;
	}

	inline operator bool() const
	{
		return ref != LUA_REFNIL;
	}
};
