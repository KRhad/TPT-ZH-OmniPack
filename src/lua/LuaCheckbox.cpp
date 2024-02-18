#ifdef LUACONSOLE

#include "LuaCheckbox.h"

#include "luascriptinterface.h"
#include "interface/Checkbox.h"

const char LuaCheckbox::className[] = "checkbox";

#define method(class, name) {#name, &class::name}
Luna<LuaCheckbox>::RegType LuaCheckbox::methods[] = {
	method(LuaCheckbox, action),
	method(LuaCheckbox, text),
	method(LuaCheckbox, position),
	method(LuaCheckbox, size),
	method(LuaCheckbox, visible),
	method(LuaCheckbox, checked),
	{0, 0}
};

LuaCheckbox::LuaCheckbox(lua_State * l) :
	LuaComponent(l),
	actionFunction(l)
{
	int posX = luaL_optinteger(l, 1, 0);
	int posY = luaL_optinteger(l, 2, 0);
	int sizeX = luaL_optinteger(l, 3, 10);
	int sizeY = luaL_optinteger(l, 4, 10);
	std::string text = tpt_lua_optString(l, 5, "");

	checkbox = new Checkbox(Point(posX, posY), Point(sizeX, sizeY), text);
	component = checkbox;
	checkbox->SetCallback([this] (bool checked) { triggerAction(); });
}

int LuaCheckbox::checked(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		checkbox->SetChecked(lua_toboolean(l, 1));
		return 0;
	}
	else
	{
		lua_pushboolean(l, checkbox->IsChecked());
		return 1;
	}
}

int LuaCheckbox::action(lua_State * l)
{
	return actionFunction.CheckAndAssignArg1(l);
}

int LuaCheckbox::text(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		checkbox->SetText(tpt_lua_checkString(l, 1));
		return 0;
	}
	else
	{
		tpt_lua_pushString(l, checkbox->GetText());
		return 1;
	}
}

void LuaCheckbox::triggerAction()
{
	if(actionFunction)
	{
		lua_rawgeti(l, LUA_REGISTRYINDEX, actionFunction);
		lua_rawgeti(l, LUA_REGISTRYINDEX, owner_ref);
		lua_pushboolean(l, checkbox->IsChecked());
		if (tpt_lua_pcall(l, 2, 0, 0))
		{
			luacon_log(tpt_lua_toString(l, -1));
		}
	}
}

#endif
