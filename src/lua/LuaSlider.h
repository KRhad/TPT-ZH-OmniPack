#pragma once

#include "LuaLuna.h"
#include "LuaComponent.h"

class Slider;

class LuaSlider: public LuaComponent
{
	Slider * slider;
	LuaComponentCallback onValueChangedFunction;
	void triggerOnValueChanged();
	int onValueChanged(lua_State * l);
	int steps(lua_State * l);
	int value(lua_State * l);
public:
	static const char className[];
	static Luna<LuaSlider>::RegType methods[];

	LuaSlider(lua_State * l);
};
