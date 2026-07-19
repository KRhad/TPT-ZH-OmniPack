#ifndef LOGIN_H
#define LOGIN_H

#include <functional>
#include <string>
#include "interface/Window.h"

class Label;
class Button;
class Textbox;
class Login : public ui::Window
{
	Textbox *usernameTextbox, *passwordTextbox;
	Label *messageLabel;
	Button *signInButton, *closeButton;

	struct LoginCallback
	{
		std::function<void ()> loginCallback = nullptr;
	};
	LoginCallback callback;

	bool isLogin = true;
	bool autoClose = false;
	uint32_t closeTimer = 0;

	void UpdateSignInButton(std::string originalUsername);
	void DoLogin();
	void DoLogout();
	void ClearLoginInfo();
	void SetMessage(std::string message);
public:
	Login(LoginCallback callback);

	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	void OnTick(uint32_t ticks) override;
};

#endif // LOGIN_H
