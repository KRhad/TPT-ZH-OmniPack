#ifdef LUACONSOLE

#include "LuaTextbox.h"

#include "luascriptinterface.h"

#include "interface/Textbox.h"

const char LuaTextbox::className[] = "Textbox";

#define method(class, name) {#name, &class::name}
Luna<LuaTextbox>::RegType LuaTextbox::methods[] = {
	method(LuaTextbox, text),
	method(LuaTextbox, readonly),
	method(LuaTextbox, onTextChanged),
	method(LuaTextbox, position),
	method(LuaTextbox, size),
	method(LuaTextbox, visible),
	{0, 0}
};

LuaTextbox::LuaTextbox(lua_State * l) :
	LuaComponent(l),
	onTextChangedFunction(l)
{
	this->l = l;
	int posX = luaL_optinteger(l, 1, 0);
	int posY = luaL_optinteger(l, 2, 0);
	int sizeX = luaL_optinteger(l, 3, 10);
	int sizeY = luaL_optinteger(l, 4, 10);
	std::string text = luaL_optstring(l, 5, "");
	std::string placeholder = luaL_optstring(l, 6, "");

	textbox = new Textbox(Point(posX, posY), Point(sizeX, sizeY), text);
	textbox->SetPlaceholder(placeholder);
	textbox->SetCallback([this] { triggerOnTextChanged(); });
	textbox->SetSelfManaged();
	component = textbox;
}

int LuaTextbox::readonly(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		luaL_checktype(l, 1, LUA_TBOOLEAN);
		textbox->SetReadOnly(lua_toboolean(l, 1));
		return 0;
	}
	else
	{
		lua_pushboolean(l, textbox->IsReadOnly());
		return 1;
	}
}

int LuaTextbox::onTextChanged(lua_State * l)
{
	return onTextChangedFunction.CheckAndAssignArg1(l);
}

void LuaTextbox::triggerOnTextChanged()
{
	if (onTextChangedFunction)
	{
		lua_rawgeti(l, LUA_REGISTRYINDEX, onTextChangedFunction);
		lua_rawgeti(l, LUA_REGISTRYINDEX, owner_ref);
		if (lua_pcall(l, 1, 0, 0))
		{
			luacon_log(lua_tostring(l, -1));
		}
	}
}

int LuaTextbox::text(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		textbox->SetText(luaL_checkstring(l, 1));
		return 0;
	}
	else
	{
		lua_pushstring(l, textbox->GetText().c_str());
		return 1;
	}
}

#endif
