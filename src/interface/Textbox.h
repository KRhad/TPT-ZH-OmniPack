#ifndef TEXTBOX_H
#define TEXTBOX_H

#include <functional>
#include "Label.h"

namespace gfx
{
class VideoBuffer;
}
class Textbox : public Label
{
public:
	static const int NOSIZELIMIT = 0; // Component size, not text size
	enum texttype { TEXT, MULTILINE, NUMBER };

	Textbox(Point position, Point size, std::string text, bool multiline = false);

	// Callback doesn't return the text. A reference to the textbox should be copied to get the text or display text
	void SetCallback(std::function<void(void)> callback) { this->callback = callback; }
	void SetDefocusCallback(std::function<void(void)> callback) { this->defocusCallback = callback; }

	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	void OnTextInput(const char *text) override;
	void OnDraw(gfx::VideoBuffer* vid) override;
	void OnFocus() override;
	void OnDefocus() override;

	void SetAutoSize(bool X, bool Y, Point limit);
	void SetCharacterLimit(int characterLimit_) { characterLimit = characterLimit_; }
	void SetAutoCorrect(bool autoCorrect_) { autoCorrect = autoCorrect_; }
	bool IsReadOnly() { return readOnly; }
	void SetReadOnly(bool readOnly) { this->readOnly = readOnly; }
	std::string GetPlaceholder() { return placeholder; }
	void SetPlaceholder(std::string placeholder) { this->placeholder = placeholder; }
	void SetType(texttype type_) { type = type_; }

private:
	Point sizeLimit;
	unsigned int characterLimit;
	bool autoCorrect;
	bool readOnly = false;
	std::string placeholder;
	std::function<void(void)> callback = nullptr;
	std::function<void(void)> defocusCallback = nullptr;
	texttype type;

	bool DeleteHighlight(bool updateDisplayText);
	void InsertText(std::string inserttext);

	bool CharacterValid(char c);
	bool StringValid(const char *str);

protected:
	bool ShowCursor() override { return true; }
};

#endif
