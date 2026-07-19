#include "Component.h"
#include "Style.h"
#include "Window.h"

// TODO: Put component in ui namespace and remove this
using namespace ui;

Component::Component(Point position, Point size) :
	parent(nullptr),
	position(position),
	size(size),
	isMouseInside(false),
	visible(true),
	enabled(true),
	color(Style::Border),
	toDelete(false),
	toAdd(false)
{

}

bool Component::IsFocused()
{
	if (parent)
		return parent->IsFocused(this);
	return false;
}

bool Component::IsClicked()
{
	if (parent)
		return parent->IsClicked(this);
	return false;
}

bool Component::IsMouseDown()
{
	if (parent)
		return parent->IsMouseDown();
	return false;
}

void Component::SetVisible(bool visible)
{
	this->visible = visible;
	toAdd = false;
	if (!visible && parent)
		parent->DefocusComponent(this);
}
