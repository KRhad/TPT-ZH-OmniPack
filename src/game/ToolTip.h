#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <string>
#include <vector>
#include "common/Point.h"
#undef min
#undef max

enum { TOOLTIP, ELEMENTTIP, INFOTIP, QTIP, INTROTIP, LUATIP };

class ToolTip
{
	std::string tip;
	Point location;
	int alpha, ID;

public:
	ToolTip(std::string tip_, Point location_, int ID_, int alpha_);

	void UpdateToolTip(std::string toolTip, Point location_, int alpha_);
	void AddToScreen();
	bool DrawToolTip();
	int GetID() { return ID; }
	int GetAlpha() { return alpha; }
	void SetTip(std::string tooltip) { tip = tooltip; }
};

extern std::vector<ToolTip*> toolTips;
void UpdateToolTip(std::string toolTip, Point location, int ID, int alpha);
void DrawToolTips();
int GetToolTipAlpha(int ID);

#endif
