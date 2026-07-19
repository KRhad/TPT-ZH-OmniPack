#pragma once

#include "LuaLuna.h"
#include "LuaComponent.h"

class Label;

class LuaLabel: public LuaComponent
{
	Label * label;
	int text(lua_State * l);
public:
	static const char className[];
	static Luna<LuaLabel>::RegType methods[];

	LuaLabel(lua_State * l);
};
