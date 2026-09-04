#include "LoginModel.h"
#include "LoginView.h"
#include "Config.h"
#include "client/Client.h"
#include "client/http/LoginRequest.h"
#include "client/http/LogoutRequest.h"

void LoginModel::Login(ByteString username, ByteString password)
{
	if (username.Contains("@"))
	{
		statusText = String::Build("使用您的 Powder Toy 帐户登录，而不是您的电子邮件。如果您没有 Powder Toy 帐户，您可以在以下位置创建一个帐户： ", SERVER, "/Register.html");
		loginStatus = loginIdle;
		notifyStatusChanged();
		return;
	}
	statusText = "正在登录……";
	loginStatus = loginWorking;
	notifyStatusChanged();
	loginRequest = std::make_unique<http::LoginRequest>(username, password);
	loginRequest->Start();
}

void LoginModel::Logout()
{
	statusText = "正在退出……";
	loginStatus = loginWorking;
	notifyStatusChanged();
	logoutRequest = std::make_unique<http::LogoutRequest>();
	logoutRequest->Start();
}

void LoginModel::AddObserver(LoginView * observer)
{
	observers.push_back(observer);
	notifyStatusChanged();
}

String LoginModel::GetStatusText()
{
	return statusText;
}

void LoginModel::Tick()
{
	if (loginRequest && loginRequest->CheckDone())
	{
		try
		{
			auto info = loginRequest->Finish();
			auto &client = Client::Ref();
			client.SetAuthUser(info.user);
			for (auto &item : info.notifications)
			{
				client.AddServerNotification(item);
			}
			statusText = "已登录";
			loginStatus = loginSucceeded;
		}
		catch (const http::RequestError &ex)
		{
			statusText = ByteString(ex.what()).FromUtf8();
			loginStatus = loginIdle;
		}
		notifyStatusChanged();
		loginRequest.reset();
	}
	if (logoutRequest && logoutRequest->CheckDone())
	{
		try
		{
			logoutRequest->Finish();
			auto &client = Client::Ref();
			client.SetAuthUser(std::nullopt);
			statusText = "已退出";
		}
		catch (const http::RequestError &ex)
		{
			statusText = ByteString(ex.what()).FromUtf8();
		}
		loginStatus = loginIdle;
		notifyStatusChanged();
		logoutRequest.reset();
	}
}

void LoginModel::notifyStatusChanged()
{
	for (size_t i = 0; i < observers.size(); i++)
	{
		observers[i]->NotifyStatusChanged(this);
	}
}

LoginModel::~LoginModel()
{
	// Satisfy std::unique_ptr
}
