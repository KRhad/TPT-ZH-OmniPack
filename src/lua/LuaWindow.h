#pragma once

#include <cstdint>
#include <map>

#include "LuaLuna.h"
#include "LuaComponent.h"
#include "LuaSmartRef.h"

#include "common/Point.h"

namespace ui
{
	class Window;
}

class LuaWindow
{
	LuaComponentCallback onInitializedFunction;
	LuaComponentCallback onExitFunction;
	LuaComponentCallback onTickFunction;
	LuaComponentCallback onDrawFunction;
	LuaComponentCallback onFocusFunction;
	LuaComponentCallback onBlurFunction;
	LuaComponentCallback onTryExitFunction;
	LuaComponentCallback onTryOkayFunction;
	LuaComponentCallback onMouseMoveFunction;
	LuaComponentCallback onMouseDownFunction;
	LuaComponentCallback onMouseUpFunction;
	LuaComponentCallback onMouseWheelFunction;
	LuaComponentCallback onKeyPressFunction;
	LuaComponentCallback onKeyReleaseFunction;

	std::map<LuaComponent *, LuaSmartRef> grabbed_components;

	ui::Window * window;
	lua_State * l;
	int position(lua_State * l);
	int size(lua_State * l);
	int addComponent(lua_State * l);
	int removeComponent(lua_State * l);

	//Set event handlers
	int onInitialized(lua_State * l);
	int onExit(lua_State * l);
	int onTick(lua_State * l);
	int onDraw(lua_State * l);
	int onFocus(lua_State * l);
	int onBlur(lua_State * l);
	int onTryExit(lua_State * l);
	int onTryOkay(lua_State * l);
	int onMouseMove(lua_State * l);
	int onMouseDown(lua_State * l);
	int onMouseUp(lua_State * l);
	int onMouseWheel(lua_State * l);
	int onKeyPress(lua_State * l);
	int onKeyRelease(lua_State * l);

	void triggerOnInitialized();
	void triggerOnExit();
	void triggerOnTick(uint32_t ticks);
	void triggerOnDraw();
	void triggerOnFocus();
	void triggerOnBlur();
	void triggerOnTryExit();
	void triggerOnTryOkay();
	void triggerOnMouseMove(int x, int y, Point difference);
	void triggerOnMouseDown(int x, int y, unsigned char button);
	void triggerOnMouseUp(int x, int y, unsigned char button);
	void triggerOnMouseWheel(int x, int y, int d);
	void triggerOnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt);
	void triggerOnKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt);

public:
	static const char className[];
	static Luna<LuaWindow>::RegType methods[];

	ui::Window * GetWindow() { return window; }
	void ClearRef(LuaComponent *luaComponent);

	LuaWindow(lua_State * l);
	~LuaWindow();
};
