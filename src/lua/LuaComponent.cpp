#ifdef LUACONSOLE

#include "LuaComponent.h"

#include "luascriptinterface.h"

#include "interface/Component.h"
#include "interface/Window.h"
#include "lua/LuaWindow.h"

int LuaComponentCallback::CheckAndAssignArg1(lua_State *l)
{
	if (lua_type(l, 1) != LUA_TNIL)
	{
		luaL_checktype(l, 1, LUA_TFUNCTION);
	}
	LuaSmartRef::Assign(l, 1);
	return 0;
}

LuaComponent::LuaComponent(lua_State * l):
	component(nullptr),
	owner_ref(LUA_REFNIL)
{
	this->l = l; // I don't get how this doesn't cause crashes later on
}

LuaComponent::~LuaComponent()
{
	if (parent)
		parent->ClearRef(this);

	if (!component->GetParent())
		delete component;
}

int LuaComponent::position(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		luaL_checktype(l, 1, LUA_TNUMBER);
		luaL_checktype(l, 2, LUA_TNUMBER);
		component->SetPosition(Point(lua_tointeger(l, 1), lua_tointeger(l, 2)));
		return 0;
	}
	else
	{
		Point pos = component->GetPosition();
		lua_pushinteger(l, pos.X);
		lua_pushinteger(l, pos.Y);
		return 2;
	}
}

int LuaComponent::size(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		luaL_checktype(l, 1, LUA_TNUMBER);
		luaL_checktype(l, 2, LUA_TNUMBER);
		component->SetSize(Point(lua_tointeger(l, 1), lua_tointeger(l, 2)));
		return 0;
	}
	else
	{
		Point size = component->GetSize();
		lua_pushinteger(l, size.X);
		lua_pushinteger(l, size.Y);
		return 2;
	}
}

int LuaComponent::visible(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		luaL_checktype(l, 1, LUA_TBOOLEAN);
		component->SetVisible(lua_toboolean(l, 1));
		return 0;
	}
	else
	{
		lua_pushboolean(l, component->IsVisible());
		return 1;
	}
}

#endif
