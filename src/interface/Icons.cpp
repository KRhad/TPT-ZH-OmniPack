#include "Icons.h"

void DrawIcon(Icon icon, gfx::VideoBuffer* vid, Point position)
{
	switch (icon)
	{
	case IconUsername:
		vid->DrawChar(position.X, position.Y, 0x8B, COLARGB(32, 64, 128, 255));
		vid->DrawChar(position.X, position.Y, 0x8A, COLARGB(255, 255, 255, 255));
		break;
	case IconPassword:
		vid->DrawChar(position.X - 1, position.Y + 1, 0x8C, COLARGB(160, 144, 32, 255));
		vid->DrawChar(position.X - 1, position.Y + 1, 0x84, COLARGB(255, 255, 255, 255));
		break;
	default:
		break;
	}
}
