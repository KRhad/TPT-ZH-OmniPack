#ifdef LUACONSOLE

#include "luascriptinterface.h"

#include "LuaLabel.h"

#include "graphics/VideoBuffer.h"
#include "interface/Label.h"

const char LuaLabel::className[] = "Label";

#define method(class, name) {#name, &class::name}
Luna<LuaLabel>::RegType LuaLabel::methods[] = {
	method(LuaLabel, text),
	method(LuaLabel, position),
	method(LuaLabel, size),
	method(LuaLabel, visible),
	{0, 0}
};

LuaLabel::LuaLabel(lua_State * l) :
	LuaComponent(l)
{
	this->l = l;
	int posX = luaL_optinteger(l, 1, 0);
	int posY = luaL_optinteger(l, 2, 0);
	int sizeX = luaL_optinteger(l, 3, 10);
	int sizeY = luaL_optinteger(l, 4, 10);
	std::string text = luaL_optstring(l, 5, "");
	Point textSize = gfx::VideoBuffer::TextSize(text);
	textSize += Point(5, 0);
	posX += (sizeX - textSize.X) / 2;
	posY += (sizeY - textSize.Y) / 2;

	label = new Label(Point(posX, posY), textSize, text, true, true);
	label->SetSelfManaged();
	component = label;
}

int LuaLabel::text(lua_State * l)
{
	int args = lua_gettop(l);
	if(args)
	{
		label->SetText(luaL_checkstring(l, 1));
		return 0;
	}
	else
	{
		lua_pushstring(l, label->GetText().c_str());
		return 1;
	}
}

#endif
