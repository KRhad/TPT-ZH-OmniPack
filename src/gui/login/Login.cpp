#include "Login.h"
#include "interface.h"
#include "misc.h"
#include "game/Request.h"
#include "interface/Button.h"
#include "interface/Icons.h"
#include "interface/Label.h"
#include "interface/Textbox.h"
#include "interface/RichLabel.h"
#include "json/json.h"
#include <cstring>

Login::Login(LoginCallback callback):
	ui::Window(Point(CENTERED, CENTERED), Point(220, 120)),
	callback(callback)
{
#ifndef TOUCHUI
	int buttonHeight = 15;
#else
	int buttonHeight = 25;
#endif
	std::string originalUsername = svf_login ? svf_user : "";

	Label *titleLabel = new Label(Point(5, 3), Point(Label::AUTOSIZE, Label::AUTOSIZE), "服务器登录");
	titleLabel->SetColor(COLRGB(140, 140, 255));
	this->AddComponent(titleLabel);

	usernameTextbox = new Textbox(titleLabel->Below({ 3, 3 }), { size.X - 16, Textbox::AUTOSIZE }, originalUsername);
	usernameTextbox->SetPlaceholder("[用户名]");
	usernameTextbox->SetIcon(IconUsername);
	usernameTextbox->SetCallback([originalUsername, this](){
		UpdateSignInButton(originalUsername);
	});
	this->AddComponent(usernameTextbox);

	passwordTextbox = new Textbox(usernameTextbox->Below({ 0, 4 }), { size.X - 16, Textbox::AUTOSIZE }, "");
	passwordTextbox->SetPlaceholder("[密码]");
	passwordTextbox->SetMasked(true);
	passwordTextbox->SetIcon(IconPassword);
	passwordTextbox->SetCallback([originalUsername, this](){
		UpdateSignInButton(originalUsername);
	});
	this->AddComponent(passwordTextbox);

	signInButton = new Button(Point(0, this->size.Y - buttonHeight), Point(this->size.X / 2 + 1, buttonHeight), "登录");
	UpdateSignInButton(originalUsername);
	signInButton->SetCallback([&](int mb) {
		isLogin ? DoLogin() : DoLogout();
		if (this->callback.loginCallback)
			this->callback.loginCallback();
	});
	this->AddComponent(signInButton);

	closeButton = new Button(Point(this->size.X / 2, this->size.Y - buttonHeight), Point(this->size.X / 2 + 1, buttonHeight), "关闭");
	closeButton->SetCloseButton(true);
	this->AddComponent(closeButton);

	std::string defaultMessage = svf_login ?
		"若要管理头像、密码、用户名或删除账号，请\bt{a:https://powdertoy.co.uk/Profile.html|前往网站}" :
		"没有账号？\bt{a:https://powdertoy.co.uk/Register.html|在此注册}";
	messageLabel = new RichLabel(passwordTextbox->Below({ -3, 3 }), { size.X - 10, Label::AUTOSIZE }, "", true);
	SetMessage(defaultMessage);
	this->AddComponent(messageLabel);

}

void Login::UpdateSignInButton(std::string originalUsername)
{
	if (closeTimer)
		return;
	isLogin = !svf_login || originalUsername != usernameTextbox->GetText();
	signInButton->SetText(isLogin ? "登录" : "退出登录");
	signInButton->SetEnabled(!isLogin || (!usernameTextbox->GetText().empty() && !passwordTextbox->GetText().empty()));
}

void Login::DoLogin()
{
	if (closeTimer)
		return;
	std::string user = usernameTextbox->GetText();
	std::string pass = passwordTextbox->GetText();

	if (user.empty() || pass.empty())
	{
		SetMessage("请输入用户名和密码");
		return;
	}
	if (user.find('@') != user.npos)
	{
		SetMessage("请使用 Powder Toy 账号而非邮箱登录。没有账号可在\bt{a:https://powdertoy.co.uk/Register.html|网站上注册}");
		return;
	}

	int httpStatus;
	std::string data = Request::Simple("https://" SERVER "/Login.json", &httpStatus, {
		{ "name", user },
		{ "pass", pass },
	});

	std::string error;
	ParseServerReturn(data, httpStatus, true, error);
	if (!error.empty())
	{
		SetMessage(error);
		return;
	}

	std::istringstream datastream(data);
	Json::Value root;
	datastream >> root;

	std::string username = root.get("Username", "").asString();
	int userID = root.get("UserID", 0).asInt();
	std::string sessionID = root.get("SessionID", "").asString();
	std::string sessionKey = root.get("SessionKey", "").asString();
	std::string elevation = root.get("Elevation", "").asString();
	if (!username.empty() && userID && !sessionID.empty() && !sessionKey.empty())
	{
		strncpy(svf_user, username.c_str(), 64);
		sprintf(svf_user_id, "%i", userID);
		strncpy(svf_session_id, sessionID.c_str(), 64);
		strncpy(svf_session_key, sessionKey.c_str(), 64);

		if (elevation == "Mod")
		{
			svf_admin = 0;
			svf_mod = 1;
		}
		else if (elevation == "Admin")
		{
			svf_admin = 1;
			svf_mod = 0;
		}
		else
		{
			svf_admin = 0;
			svf_mod = 0;
		}

		svf_login = 1;
		save_presets();

		SetMessage("登录成功");
		autoClose = true;
	}
	else
	{
		SetMessage("无法读取登录响应");
	}
}

void Login::DoLogout()
{
	if (closeTimer)
		return;
	int httpStatus;
	std::string data = Request::SimpleAuth("https://" SERVER "/Logout.json?Key=" + std::string(svf_session_key), &httpStatus, svf_user_id, svf_session_id);

	ClearLoginInfo();
	std::string error;
	ParseServerReturn(data, httpStatus, true, error);
	if (!error.empty())
		SetMessage(error);
	else
		SetMessage("已退出登录");
	autoClose = true;
}

void Login::ClearLoginInfo()
{
	strcpy(svf_user, "");
	strcpy(svf_user_id, "");
	strcpy(svf_session_id, "");
	svf_login = 0;
	svf_own = 0;
	svf_admin = 0;
	svf_mod = 0;
	save_presets();
}

void Login::SetMessage(std::string message)
{
	messageLabel->SetText(message);
	int newHeight = messageLabel->GetPosition().Y + messageLabel->GetSize().Y + signInButton->GetSize().Y + 3;
	this->Resize(position, { size.X, newHeight });
	signInButton->SetPosition({ signInButton->GetPosition().X, newHeight - signInButton->GetSize().Y });
	closeButton->SetPosition({ closeButton->GetPosition().X, newHeight - closeButton->GetSize().Y });
}

void Login::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (sdl_key == SDLK_RETURN)
	{
		DoLogin();
	}
	if (sdl_key == SDLK_TAB)
	{
		bool usernameFocused = IsFocused(usernameTextbox);
		FocusComponent(usernameFocused ? passwordTextbox : usernameTextbox);
	}
}

void Login::OnTick(uint32_t ticks)
{
	if (autoClose)
	{
		autoClose = false;
		closeTimer = 3000;
	}

	if (closeTimer)
	{
		if (closeTimer < ticks)
		{
			this->Close(ui::ExitButton);
			closeTimer = 0;
		}
		else
		{
			closeTimer -= ticks;
		}
	}
}
