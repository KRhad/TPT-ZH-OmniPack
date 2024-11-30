#ifndef RENDERER_H
#define RENDERER_H

#include <ctime>
#include <string>
#include "common/Singleton.h"

#define CM_VEL 0
#define CM_PRESS 1
#define CM_PERS 2
#define CM_FIRE 3
#define CM_BLOB 4
#define CM_HEAT 5
#define CM_FANCY 6
#define CM_NOTHING 7
#define CM_GRAD 8
#define CM_CRACK 9
#define CM_LIFE 10
#define CM_COUNT 11

class Save;

struct RenderPreset
{
	unsigned int renderMode;
	unsigned int displayMode;
	unsigned int colorMode;
	std::string tooltip;
};

// This class is mostly unused at the moment, but is used for controlling render / display modes
class Renderer : public Singleton<Renderer>
{
	bool recording = false;
	int screenshotIndex = 1;
	time_t lastScreenshotTime = 0;
	int recordingIndex = 0;
	int recordingFolder = 0;

	unsigned int renderMode;
	unsigned int displayMode;
	unsigned int colorMode;

	RenderPreset renderPresets[11];

	void InitRenderPresets();

public:
	Renderer();

	std::string TakeScreenshot(bool includeUI, int format);
	void RecordingTick();
	int StartRecording();
	void StopRecording();

	bool LoadRenderPreset(int preset);
	std::string GetRenderPresetToolTip(int preset);

	// render modes
	bool HasRenderMode(unsigned int renderMode);
	void ToggleRenderMode(unsigned int renderMode);
	unsigned int GetRenderMode();
	void SetRenderMode(unsigned int renderMode);

	// display modes
	bool HasDisplayMode(unsigned int displayMode);
	void ToggleDisplayMode(unsigned int displayMode);
	unsigned int GetDisplayMode();
	void SetDisplayMode(unsigned int displayMode);

	// color modes
	void SetColorMode(unsigned int color_mode);
	void XORColorMode(unsigned int color_mode);
	unsigned int GetColorMode();

	void LoadSave(Save *save);
	void CreateSave(Save *save);
};

#endif // RENDERER_H
