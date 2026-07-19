#pragma once

#include "LuaLuna.h"
#include "LuaComponent.h"

class ProgressBar;

class LuaProgressBar: public LuaComponent
{
	ProgressBar * progressBar;
	int progress(lua_State * l);
	int status(lua_State * l);
public:
	static const char className[];
	static Luna<LuaProgressBar>::RegType methods[];

	LuaProgressBar(lua_State * l);
};
