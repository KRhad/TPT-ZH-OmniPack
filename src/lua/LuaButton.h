#pragma once

#include "LuaLuna.h"
#include "LuaComponent.h"

class Button;

class LuaButton: public LuaComponent
{
	Button * button;
	LuaComponentCallback actionFunction;
	void triggerAction();
	int action(lua_State * l);
	int text(lua_State * l);
	int enabled(lua_State * l);
public:
	static const char className[];
	static Luna<LuaButton>::RegType methods[];

	LuaButton(lua_State * l);
};
