#ifndef CREATESIGN_H
#define CREATESIGN_H
#include "common/Point.h"
#include "interface/Window.h"
#include "game/Sign.h"

class Label;
class Button;
class Textbox;
class CreateSign : public ui::Window
{
	Label *newSignLabel, *pointerLabel;
	Button *okButton, *leftJuButton, *middleJuButton, *rightJuButton, *noneJuButton, *moveButton, *deleteButton;
	Textbox *signTextbox;

	int signID;
	Sign theSign;
public:
	CreateSign(int signID, Point pos);

	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;

	void SetJustification(Sign::Justification ju);
	void MoveSign();
	void DeleteSign();
	void SaveSign();
};

#endif
