#ifndef LABEL_H
#define LABEL_H

#include <string>
#include "common/Point.h"
#include "Component.h"

namespace gfx
{
class VideoBuffer;
}
class Label : public Component
{
private:
	void FindWordPosition(unsigned int position, unsigned int *cursorStart, unsigned int *cursorEnd, const char* spaces);
	unsigned int UpdateCursor(unsigned int position);
	bool CheckPlaceCursor(bool updateCursor, unsigned int position, int posX, int posY);
	uint32_t currentTick;
	bool isClicked;

protected:
	std::string text;
	int textWidth, textHeight;
	bool multiline;
	unsigned int cursor, cursorStart;
	int cursorX, cursorY; // these two and cursorPosReset are used by Textboxes to make the up / down arrow keys work
	bool cursorPosReset;
	uint32_t lastClick;
	unsigned int numClicks, clickPosition;
	bool autosizeX, autosizeY;
	bool noCutoff;

	void UpdateDisplayText(bool updateCursor = false, bool firstClick = false);
	void MoveCursor(unsigned int *cursor, int amount);
	virtual bool ShowCursor() { return false; }
	void CopySelection();

public:
	Label(Point position, Point size, std::string text, bool multiline = false, bool noCutoff = false);
	virtual ~Label() { }

	void SetSize(Point size) override;

	void SetText(std::string text_);
	std::string GetText();
	bool IsMultiline() { return multiline; }
	void SelectAll();

	void OnMouseDown(int x, int y, unsigned char button) override;
	void OnMouseUp(int x, int y, unsigned char button) override;
	void OnDefocus() override;
	void OnMouseMoved(int x, int y, Point difference) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	void OnDraw(gfx::VideoBuffer* vid) override;
	void OnTick(uint32_t ticks) override;

	static const int AUTOSIZE = -1;
};

#endif
