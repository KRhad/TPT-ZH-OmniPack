#include "ProgressBar.h"
#include "Style.h"
#include "common/Format.h"
#include "graphics/VideoBuffer.h"

// TODO: Put progressbar in ui namespace and remove this
using namespace ui;

ProgressBar::ProgressBar(Point position, Point size):
	Component(position, size)
{

}

int ProgressBar::GetProgress()
{
	return progress;
}

void ProgressBar::SetProgress(int progress)
{
	if (progress < 0)
		this->progress = 0;
	else if (progress > 100)
		this->progress = 100;
	else
		this->progress = progress;
}

void ProgressBar::OnDraw(gfx::VideoBuffer *vid)
{
	// clear area
	vid->FillRect(position.X, position.Y, size.X, size.Y, 0, 0, 0, 255);

	pixel borderColor = Style::Border;
	// border
	if (enabled)
		vid->DrawRect(position.X, position.Y, size.X, size.Y, borderColor);
	else
		vid->DrawRect(position.X, position.Y, size.X, size.Y, COLMULT(borderColor, Style::DisabledMultiplier));

	int width = static_cast<int>((size.X - 2) * progress / 100.0f);
	vid->FillRect(position.X+1, position.Y+1, width, size.Y-2, color);

	std::string progressStr;
	if (status.length())
	{
		progressStr = status;
	}
	else
	{
		progressStr = Format::NumberToString<int>(progress);
		progressStr += "%";
	}
	width = gfx::VideoBuffer::TextSize(progressStr).X;
	if (progress < 53)
		vid->DrawString(position.X + size.X/2 - width/2, position.Y + (size.Y-8)/2, progressStr, 192, 192, 192, 255);
	else
		vid->DrawString(position.X + size.X/2 - width/2, position.Y + (size.Y-8)/2, progressStr, 63, 63, 63, 255);
}
