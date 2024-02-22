#ifndef ENGINE_H
#define ENGINE_H

#include <stack>
#include <string>
#include "Window.h"
#include "common/Singleton.h"
#include "common/tpt-stdint.h"
#include "common/Point.h"

class Engine : public Singleton<Engine>
{
public:
	Engine();
	~Engine();

	bool Running() { return top != nullptr; }
	void Shutdown() { isShutdown = true; }
	bool IsShutdown() { return isShutdown; }

	void ShowWindow(ui::Window *window);
	void CloseTop(ui::DeleteReason deleteReason);
	ui::Window * GetTop() { return top; }
	int GetStackSize() { return windows.size(); }
	void RestorePreviousBuffer();
	void ProcessWindowUpdates();

	unsigned int GetScale();
	void SetScale(unsigned int scale);
	int GuessBestScale();

	bool IsResizable();
	void SetResizable(bool resizable, bool recreateWindow);

	int GetPixelFilteringMode();
	void SetPixelFilteringMode(int pixelFilteringMode, bool recreateWindow);

	bool IsFullscreen();
	void SetFullscreen(bool fullscreen);

	bool IsAltFullscreen();
	void SetAltFullscreen(bool altFullscreen);

	bool IsForceIntegerScaling();
	void SetForceIntegerScaling(bool forceIntegerScaling);

	bool IsMomentumScroll() { return momentumScroll; }
	void SetMomentumScroll(bool momentumScroll) { this->momentumScroll = momentumScroll; }

	void StartTextInput();
	void StopTextInput();
	bool DoTextInput() { return textInput; }

	float GetFpsLimit() { return fpsLimit; }
	void SetFpsLimit(float fpsLimit) { this->fpsLimit = fpsLimit; }
	int GetDrawingFrequency() { return drawLimit; }
	void SetDrawingFrequency(int drawLimit) { this->drawLimit = drawLimit; }

	bool IsFastQuit() { return fastQuit; }
	void SetFastQuit(bool fastQuit) { this->fastQuit = fastQuit; }

	void ClipboardPush(std::string text);
	std::string ClipboardPull();
	int GetModifiers();

	bool IsSecondaryEngineLoop() { return secondaryEngineLoop; }
	void SetSecondaryEngineLoop(bool secondaryLoop) { secondaryEngineLoop = secondaryLoop; };
private:
	void ShowWindowDelayed();
	void CloseWindowDelayed();

	std::stack<ui::Window*> windows;
	std::stack<pixel*> buffers;
	ui::Window *top, *nextTop;
	bool isShutdown = false;

	unsigned int scale = 1;
	bool resizable = false;
	int pixelFilteringMode = 0;
	bool fullscreen = false;
	bool altFullscreen = false;
	bool forceIntegerScaling = true;
	bool momentumScroll = true;

	bool textInput = false;

	float fpsLimit = 60.0f;
	int drawLimit = 0;

	bool fastQuit = false;

	bool secondaryEngineLoop = false;
};

#endif
