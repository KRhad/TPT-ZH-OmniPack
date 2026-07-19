#include "Textbox.h"
#include "common/tpt-minmax.h"
#include <sstream>
#include <cstring>
#include "Style.h"
#include "EventLoopSDL.h"
#include "common/Format.h"
#include "common/Platform.h"
#include "graphics/VideoBuffer.h"
#include "interface/Engine.h"

// TODO: Put progressbar in ui namespace and remove this
using namespace ui;

Textbox::Textbox(Point position, Point size_, std::string text, bool multiline):
	Label(position, size_, text, multiline),
	sizeLimit(Point(NOSIZELIMIT, NOSIZELIMIT)),
	characterLimit(10000),
	autoCorrect(true),
	type(TEXT)
{
	doesTextInput = true;
}

//deletes any highlighted text, returns true if there was something deleted (cancels backspace/delete action)
bool Textbox::DeleteHighlight(bool updateDisplayText)
{
	if (cursor == cursorStart)
		return false;
	if (cursor > cursorStart)
	{
		text.erase(cursorStart, cursor-cursorStart);
		cursor = cursorStart;
	}
	else
	{
		text.erase(cursor, cursorStart-cursor);
		cursorStart = cursor;
	}
	if (updateDisplayText)
	{
		UpdateDisplayText();
		if (callback)
			callback();
	}
	return true;
}

void Textbox::InsertText(std::string inserttext)
{
	inserttext = Format::CleanString(inserttext, true, true, type != MULTILINE);
	if (!inserttext.length())
		return;

	std::string oldText = text;
	int oldCursor = cursor, oldCursorStart = cursorStart;
	size_t len = inserttext.length();

	DeleteHighlight(false);
	//text.insert(cursor, 1, static_cast<char>(key));
	text.insert(cursor, inserttext);

	int oldLines = std::count(text.begin(), text.begin()+cursor+len, '\r');
	UpdateDisplayText();
	// this character doesn't fit, revert changes to string (not really a nice way to do this :|)
	if ((multiline ? (autosizeY ? sizeLimit.Y != NOSIZELIMIT && size.Y > sizeLimit.Y : textHeight > size.Y) : (autosizeX ? sizeLimit.X != NOSIZELIMIT && size.X > sizeLimit.X : textWidth > size.X))
	    || text.length() > characterLimit)
	{
		text = oldText;
		cursor = oldCursor;
		cursorStart = oldCursorStart;
		UpdateDisplayText();
		return;
	}
	int newLines = std::count(text.begin(), text.begin()+cursor+len, '\r'), newLines2 = newLines;
	while ((newLines = std::count(text.begin(), text.begin()+cursor+len+newLines-oldLines, '\r')) != newLines2)
		newLines2 = newLines; //account for any newlines that were inserted (while loop for proper checking if like 1000 newlines were inserted ...)

	//put cursor at end of the new paste, accounting for extra newlines
	cursor += len+newLines-oldLines;
	cursorStart = cursor;
	UpdateDisplayText();

	if (callback)
		callback();
}

bool Textbox::CharacterValid(char character)
{
	switch (type)
	{
	case NUMBER:
		return character >= '0' && character <= '9';
	case MULTILINE:
		if (character == '\n')
			return true;
	case TEXT:
		return character >= ' ' && character <= '~';
	}
	return false;
}

bool Textbox::StringValid(const char *str)
{
	for (char c : std::string(str))
		if (!CharacterValid(c))
			return false;
	return true;
}

void Textbox::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	Label::OnKeyPress(key, scan, repeat, shift, ctrl, alt);
	if (ctrl)
	{
		if (key == SDLK_LEFT || key == SDLK_RIGHT)
			return;
		switch (scan)
		{
		case SDL_SCANCODE_A:
		case SDL_SCANCODE_C:
			//these are handled by Labels and should be ignored
			return;
		case SDL_SCANCODE_V:
		{
			if (readOnly)
				break;
			std::string clipboard = Engine::Ref().ClipboardPull();
			if (clipboard.length())
				InsertText(clipboard);
			break;
		}
		case SDL_SCANCODE_X:
			if (readOnly)
				break;
			Label::CopySelection();
			DeleteHighlight(true);
			break;
		case SDL_SCANCODE_BACKSPACE:
			if (!readOnly && !DeleteHighlight(true) && cursor > 0)
			{
				size_t stopChar;
				stopChar = text.substr(0, cursor).find_last_not_of(" .,!?\r\n");
				if (stopChar == text.npos)
					stopChar = -1;
				else
					stopChar = text.substr(0, stopChar).find_last_of(" .,!?\r\n");
				text.erase(stopChar+1, cursor-(stopChar+1));
				cursor = cursorStart = stopChar+1;

				UpdateDisplayText();
				if (callback)
					callback();
			}
			break;
		case SDL_SCANCODE_DELETE:
			if (!readOnly && !DeleteHighlight(true) && text.length() && cursor < text.length())
			{
				size_t stopChar;
				stopChar = text.find_first_not_of(" .,!?\n", cursor);
				stopChar = text.find_first_of(" .,!?\n", stopChar);
				text.erase(cursor, stopChar-cursor);

				UpdateDisplayText();
				if (callback)
					callback();
			}
			break;
		}
		return;
	}
	if (shift)
	{
		if (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_UP || key == SDLK_DOWN)
			return;
	}
	switch (scan) //all of these do different things if any text is highlighted
	{
	case SDL_SCANCODE_BACKSPACE:
		if (!readOnly && !DeleteHighlight(true) && cursor > 0)
		{
			// move cursor barkward 1 real character (accounts for formatting that also needs to be deleted)
			MoveCursor(&cursor, -1);
			text.erase(cursor, cursorStart-cursor);
			cursorStart = cursor;

			int oldLines = std::count(text.begin(), text.begin()+cursor, '\r'), oldCursor = cursor;
			UpdateDisplayText();
			int newLines = std::count(text.begin(), text.begin()+cursor, '\r') - (cursor - oldCursor);

			// make sure cursor is correct when deleting the character put us on the previous line or next line (deleting a space)
			if (oldLines != newLines)
			{
				cursor += newLines-oldLines;
				cursorStart = cursor;
			}

			if (callback)
				callback();
		}
		break;
	case SDL_SCANCODE_DELETE:
		if (!readOnly && !DeleteHighlight(true) && text.length() && cursor < text.length())
		{
			// move cursor forward 1 real character (accounts for formatting that also needs to be deleted)
			MoveCursor(&cursor, 1);
			text.erase(cursorStart, cursor-cursorStart);
			cursor = cursorStart;

			int oldLines = std::count(text.begin(), text.begin()+cursor, '\r'), oldCursor = cursor;;
			UpdateDisplayText();
			int newLines = std::count(text.begin(), text.begin()+cursor, '\r') - (cursor - oldCursor);

			// make sure cursor is correct when deleting the character put us on the next line (deleting a space)
			if (oldLines != newLines)
			{
				cursor += newLines-oldLines;
				cursorStart = cursor;
			}

			if (callback)
				callback();
		}
		break;
	}
	switch (key)
	{
	case SDLK_LEFT:
		if (cursor != cursorStart)
		{
			if (cursor < cursorStart)
				cursorStart = cursor;
			else
				cursor = cursorStart;
		}
		else
		{
			MoveCursor(&cursor, -1);
			cursorStart = cursor;
			UpdateDisplayText();
		}
		break;
	case SDLK_RIGHT:
		if (cursor != cursorStart)
		{
			if (cursor > cursorStart)
				cursorStart = cursor;
			else
				cursor = cursorStart;
		}
		else
		{
			MoveCursor(&cursor, 1);
			cursorStart = cursor;
			UpdateDisplayText();
		}
		break;
	case SDLK_UP:
		if (cursor != cursorStart)
		{
			if (cursor < cursorStart)
				cursorStart = cursor;
			else
				cursor = cursorStart;
		}
		else
		{
			if (cursorPosReset)
			{
				UpdateDisplayText();
				cursorPosReset = false;
			}
			cursorY -= 12;
			if (cursorY < 0)
				cursorY = 0;
			int cx = cursorX, cy = cursorY;
			UpdateDisplayText(true, true);
			// make sure these are set to what we want
			cursorX = cx;
			cursorY = cy;
		}
		break;
	case SDLK_DOWN:
		if (cursor != cursorStart)
		{
			if (cursor > cursorStart)
				cursorStart = cursor;
			else
				cursor = cursorStart;
		}
		else
		{
			if (cursorPosReset)
			{
				UpdateDisplayText();
				cursorPosReset = false;
			}
			cursorY += 12;
			if (cursorY > size.Y)
				cursorY = size.Y;
			int cx = cursorX, cy = cursorY;
			UpdateDisplayText(true, true);
			// make sure these are set to what we want
			cursorX = cx;
			cursorY = cy;
		}
		break;
	case SDLK_RETURN:
		if (!readOnly && this->type == MULTILINE)
		{
			InsertText("\n");
		}
		break;
	}
}

void Textbox::OnTextInput(const char *text)
{
	if (!readOnly && StringValid(text))
		InsertText(text);
}

void Textbox::OnDraw(gfx::VideoBuffer* vid)
{
	Label::OnDraw(vid);

	ARGBColour borderColor, defaultColor = Style::Border;
	if (!enabled)
		borderColor = COLMULT(defaultColor, Style::DisabledMultiplier);
	else if (IsClicked())
		borderColor = COLADD(defaultColor, Style::ClickedModifier);
	else if (IsFocused())
		borderColor = defaultColor;
	else
		borderColor = COLMULT(defaultColor, Style::DeselectedMultiplier);

	if (!text.length() && placeholder.length())
	{
		int pos = position.X + 3;
		if (icon && !multiline)
			pos += 13;
		vid->DrawString(pos, position.Y+4, placeholder, COLMODALPHA(color, 170));
	}
	vid->DrawRect(position.X, position.Y, size.X, size.Y, borderColor);
}

void Textbox::OnFocus()
{
#ifdef TOUCHUI
	char buffer[1024];
	memcpy(buffer, text.c_str(), tpt::min(text.length() + 1, (size_t)1024));
	Platform::GetOnScreenKeyboardInput(buffer, 1024, autoCorrect);
	SetText(buffer);
	if (callback)
		callback();
#endif
}

void Textbox::OnDefocus()
{
	if (defocusCallback)
		defocusCallback();
}

void Textbox::SetAutoSize(bool X, bool Y, Point limit)
{
	autosizeX = X;
	autosizeY = Y;
	sizeLimit = limit;
}
