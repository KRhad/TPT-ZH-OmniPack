#ifndef ICONS_H
#define ICONS_H

#include "common/Point.h"
#include "graphics/VideoBuffer.h"

enum Icon
{
	IconNone, IconUsername, IconPassword
};

void DrawIcon(Icon icon, gfx::VideoBuffer* vid, Point position);

#endif // ICONS_H
