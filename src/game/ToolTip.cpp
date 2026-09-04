#include "ToolTip.h"
#include "font.h"
#include "graphics.h"
#include "common/tpt-minmax.h"

// functions in tooltip class
ToolTip::ToolTip(std::string tip_, Point location_, int ID_, int alpha_):
	tip(tip_),
	location(location_),
	ID(ID_)
{
	if (alpha_ == -1)
		alpha = 15;
	else
		alpha = alpha_;
}

// update the visibility / text / location of a tooltip
void ToolTip::UpdateToolTip(std::string toolTip, Point location_, int alpha_)
{
	tip = toolTip;
	location = location_;
	//alpha_ == -1 is used for fading in tooltips for some reason (gets reduced by 5 every frame in draw still)
	if (alpha_ <= -1 && alpha < 255)
	{
		alpha += 15;
		if (alpha > 255)
			alpha = 255;
	}
	//else, we want to directly set alpha
	else if (ID != INTROTIP || alpha > 255 || alpha_ == 0)
		alpha = alpha_;
}

// add or replace a tooltip on the screen
void ToolTip::AddToScreen()
{
	// make sure tooltip is in current on screen tooltips list, if not then add it
	ToolTip *foundTip = NULL;
	for (unsigned int i = 0; i < toolTips.size(); i++)
		if (toolTips[i]->GetID() == ID)
		{
			foundTip = toolTips[i];
			break;
		}
	if (!foundTip)
	{
		if (ID != INTROTIP || alpha > 255)
		{
			foundTip = new ToolTip(tip, location, ID, alpha);
			toolTips.push_back(foundTip);
		}
		else
			return;
	}

	foundTip->UpdateToolTip(tip, location, alpha);
}

bool ToolTip::DrawToolTip()
{
	if (alpha > 0)
	{
		if (ID == ELEMENTTIP)
		{
			int visibleAlpha = std::min(alpha, 255);
			fillrect(vid_buf, 4, YRES - 32, XRES + BARSIZE - 8, 29, 0, 0, 0, visibleAlpha * 3 / 4);
			drawrect(vid_buf, 4, YRES - 32, XRES + BARSIZE - 8, 29, 96, 96, 96, visibleAlpha / 2);
			drawtextwrap(vid_buf, location.X, location.Y, XRES + BARSIZE - 16, 26, tip.c_str(), 255, 255, 255, visibleAlpha);
		}
		else if (ID == ELEMENTLONGTIP)
		{
			int visibleAlpha = std::min(alpha, 255);
			int contentWidth = XRES + BARSIZE - 16;
			int measuredHeight = textwrapheight(const_cast<char *>(tip.c_str()), contentWidth);
			int contentHeight = std::min(measuredHeight, (FONT_H + 2) * 7);
			int footerHeight = FONT_H + 7;
			int boxHeight = contentHeight + footerHeight + 9;
			int boxY = YRES - boxHeight - 3;

			fillrect(vid_buf, 4, boxY, XRES + BARSIZE - 8, boxHeight, 0, 0, 0, visibleAlpha * 7 / 8);
			drawrect(vid_buf, 4, boxY, XRES + BARSIZE - 8, boxHeight, 96, 96, 96, visibleAlpha / 2);
			drawtextwrap(vid_buf, 8, boxY + 5, contentWidth, contentHeight, tip.c_str(), 255, 255, 255, visibleAlpha);
			fillrect(vid_buf, 8, boxY + contentHeight + 7, XRES + BARSIZE - 17, 1, 80, 80, 80, visibleAlpha / 2);
			drawtext(vid_buf, 8, boxY + contentHeight + 11, "再次点击已选元素，打开完整说明（上下拖动阅读）", 255, 216, 32, visibleAlpha);
		}
		else if (ID == INFOTIP)
			drawtext_outline(vid_buf, location.X, location.Y, tip.c_str(), 255, 255, 255, std::min(alpha, 255), 0, 0, 0, std::min(alpha, 255));
		else
			drawtext(vid_buf, location.X, location.Y, tip.c_str(), 255, 255, 255, std::min(alpha, 255));
		alpha -= 5;
	}
	return alpha > 0;
}

// other functions related to tooltips
std::vector<ToolTip*> toolTips;
void UpdateToolTip(std::string toolTip, Point location, int ID, int alpha)
{
	for (unsigned int i = 0; i < toolTips.size(); i++)
		if (toolTips[i]->GetID() == ID)
		{
			toolTips[i]->UpdateToolTip(toolTip, location, alpha);
			return;
		}
	if (ID != INTROTIP || alpha > 255)
		toolTips.push_back(new ToolTip(toolTip, location, ID, alpha));
}

void DrawToolTips()
{
	for (int i = toolTips.size()-1; i >= 0; i--)
	{
		if (!toolTips[i]->DrawToolTip())
		{
			delete toolTips[i];
			toolTips.erase(toolTips.begin()+i);
		}
	}
}

int GetToolTipAlpha(int ID)
{
	for (unsigned int i = 0; i < toolTips.size(); i++)
		if (toolTips[i]->GetID() == ID)
			return toolTips[i]->GetAlpha();
	return 0;
}
