#pragma once

#include "LuaLuna.h"
#include "LuaComponent.h"

class Textbox;

class LuaTextbox: public LuaComponent
{
	LuaComponentCallback onTextChangedFunction;
	Textbox * textbox;
	int text(lua_State * l);
	int readonly(lua_State * l);
	int focus(lua_State *l);
	int onTextChanged(lua_State * l);
	void triggerOnTextChanged();
public:
	static const char className[];
	static Luna<LuaTextbox>::RegType methods[];

	LuaTextbox(lua_State * l);
};
