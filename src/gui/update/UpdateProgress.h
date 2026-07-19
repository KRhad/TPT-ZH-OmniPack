#ifndef UPDATEPROGRESS_H
#define UPDATEPROGRESS_H

#include <functional>
#include <string>
#include "interface/Window.h"

class Request;
class ProgressBar;
class UpdateProgress : public ui::Window
{
	Request *download;
	bool completed = false;
	ProgressBar * progress;
	std::function<void(char*, int)> callback;

	void ShowError(std::string message);

public:
	UpdateProgress(std::string uri, std::string username, std::function<void(char*, int)> callback);

	void OnTick(uint32_t ticks) override;
};

#endif // UPDATEPROGRESS_H
