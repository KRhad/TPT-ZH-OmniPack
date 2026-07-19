#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "graphics.h"
#include "powdergraphics.h"
#include "Renderer.h"

#include "common/Format.h"
#include "common/Platform.h"
#include "game/Save.h"
#include "graphics/VideoBuffer.h"

Renderer::Renderer():
	renderMode(RENDER_FIRE | RENDER_SPRK | RENDER_EFFE | RENDER_BASC),
	displayMode(0),
	colorMode(COLOR_DEFAULT)
{
	InitRenderPresets();
}

std::string Renderer::TakeScreenshot(bool includeUI, int format)
{
	int w = includeUI ? XRES+BARSIZE : XRES;
	int h = includeUI ? YRES+MENUSIZE : YRES;
	gfx::VideoBuffer *vid = new gfx::VideoBuffer(w, h);
	vid->CopyBufferFrom(vid_buf, XRES+BARSIZE, YRES+MENUSIZE, w, h);

	std::vector<char> screenshotData;
	time_t screenshotTime = time(nullptr);
	std::string fileExtension = "";
	if (format == 0)
	{
		screenshotData = Format::VideoBufferToPNG(*vid);
		fileExtension = ".png";
	}
	else if (format == 1)
	{
		screenshotData = Format::VideoBufferToBMP(*vid);
		fileExtension = ".bmp";
	}
	else if (format == 2)
	{
		screenshotData = Format::VideoBufferToPPM(*vid);
		fileExtension = ".ppm";
	}
	delete vid;

	// Optional suffix to distinguish screenshots taken at the exact same time
	std::string suffix = "";
	if (screenshotTime == lastScreenshotTime)
	{
		screenshotIndex++;
		suffix = " (" + Format::NumberToString<int>(screenshotIndex) + ")";
	}
	else
	{
		screenshotIndex = 1;
	}
	std::string date = Format::UnixtimeToDate(screenshotTime, "%Y-%m-%d %H.%M.%S");
	std::string filename = "screenshot " + date + suffix + fileExtension;

	try
	{
		std::ofstream screenshot;
		screenshot.open(filename, std::ios::binary);
		if (screenshot.is_open())
		{
			screenshot.write(&screenshotData[0], screenshotData.size());
			screenshot.close();
		}
		else
			return "";
	}
	catch (std::exception& e)
	{
		std::cout << "Error saving screenshot: " << e.what() << std::endl;
		return "";
	}

	lastScreenshotTime = screenshotTime;

	return filename;
}

void Renderer::RecordingTick()
{
	if (!recording)
		return;

	gfx::VideoBuffer *screenshot = new gfx::VideoBuffer(XRES, YRES);
	screenshot->CopyBufferFrom(vid_buf, XRES+BARSIZE, YRES+MENUSIZE, XRES, YRES);

	std::vector<char> screenshotData = Format::VideoBufferToPPM(*screenshot);

	std::stringstream fileName;
	fileName << "recordings" << PATH_SEP << recordingFolder << PATH_SEP << "frame_"
	         << std::setfill('0') << std::setw(6) << (recordingIndex++) << ".ppm";

	try
	{
		std::ofstream screenshot;
		screenshot.open(fileName.str(), std::ios::binary);
		if (screenshot.is_open())
		{
			screenshot.write(&screenshotData[0], screenshotData.size());
			screenshot.close();
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Error saving screenshot: " << e.what() << std::endl;
	}
}

int Renderer::StartRecording()
{
	time_t startTime = time(NULL);
	recordingFolder = startTime;
	std::stringstream recordingDir;
	recordingDir << "recordings" << PATH_SEP << recordingFolder;
	Platform::MakeDirectory("recordings");
	Platform::MakeDirectory(recordingDir.str());
	recording = true;
	recordingIndex = 0;
	return recordingFolder;
}

void Renderer::StopRecording()
{
	recording = false;
	recordingIndex = 0;
	recordingFolder = 0;
}

void Renderer::InitRenderPresets()
{
	for (int i = 0; i < CM_COUNT; i++)
	{
		renderPresets[i].renderMode = RENDER_BASC;
		renderPresets[i].displayMode = 0;
		renderPresets[i].colorMode = COLOR_DEFAULT;
	}

	renderPresets[CM_VEL].renderMode |= RENDER_EFFE;
	renderPresets[CM_VEL].displayMode = DISPLAY_AIRV;
	renderPresets[CM_VEL].tooltip = "Velocity Display";

	renderPresets[CM_PRESS].renderMode |= RENDER_EFFE;
	renderPresets[CM_PRESS].displayMode = DISPLAY_AIRP;
	renderPresets[CM_PRESS].tooltip = "Pressure Display";

	renderPresets[CM_PERS].renderMode |= RENDER_EFFE;
	renderPresets[CM_PERS].displayMode = DISPLAY_PERS;
	renderPresets[CM_PERS].tooltip = "Persistent Display";

	renderPresets[CM_FIRE].renderMode |= RENDER_FIRE | RENDER_SPRK | RENDER_EFFE;
	renderPresets[CM_FIRE].tooltip = "Fire Display";

	renderPresets[CM_BLOB].renderMode |= RENDER_FIRE | RENDER_SPRK | RENDER_EFFE | RENDER_BLOB;
	renderPresets[CM_BLOB].tooltip = "Blob Display";

	renderPresets[CM_HEAT].displayMode = DISPLAY_AIRH;
	renderPresets[CM_HEAT].colorMode = COLOR_HEAT;
	renderPresets[CM_HEAT].tooltip = "Heat Display";

	renderPresets[CM_FANCY].renderMode |= RENDER_FIRE | RENDER_SPRK | RENDER_GLOW | RENDER_BLUR | RENDER_EFFE;
	renderPresets[CM_FANCY].displayMode = DISPLAY_WARP;
	renderPresets[CM_FANCY].tooltip = "Fancy Display";

	renderPresets[CM_NOTHING].tooltip = "Nothing Display";

	renderPresets[CM_GRAD].colorMode = COLOR_GRAD;
	renderPresets[CM_GRAD].tooltip = "Heat Gradient Display";

	renderPresets[CM_LIFE].colorMode = COLOR_LIFE;
	renderPresets[CM_LIFE].tooltip = "Life Gradient Display";

	renderPresets[CM_CRACK].renderMode |= RENDER_EFFE;
	renderPresets[CM_CRACK].displayMode = DISPLAY_AIRC;
	renderPresets[CM_CRACK].tooltip = "Alternate Velocity Display";

	renderPresets[CM_VORT].renderMode |= RENDER_EFFE;
	renderPresets[CM_VORT].displayMode = DISPLAY_AIRW;
	renderPresets[CM_VORT].tooltip = "Vorticity Display";
}

bool Renderer::LoadRenderPreset(int preset)
{
	if (preset < 0 || preset >= CM_COUNT)
		return false;

	renderMode = renderPresets[preset].renderMode;
	displayMode = renderPresets[preset].displayMode;
	colorMode = renderPresets[preset].colorMode;

	if (HasRenderMode(RENDER_FIRE))
	{
		memset(fire_r, 0, sizeof(fire_r));
		memset(fire_g, 0, sizeof(fire_g));
		memset(fire_b, 0, sizeof(fire_b));
	}

	if (HasDisplayMode(DISPLAY_PERS))
	{
		memset(pers_bg, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
	}

	return true;
}

std::string Renderer::GetRenderPresetToolTip(int preset)
{
	if (preset < 0 || preset >= CM_COUNT)
		return "Invalid Render Mode Preset";
	return renderPresets[preset].tooltip;
}

bool Renderer::HasRenderMode(unsigned int renderMode)
{
	return (this->renderMode & renderMode) == renderMode;
}

void Renderer::ToggleRenderMode(unsigned int renderMode)
{
	if (HasRenderMode(renderMode))
		this->renderMode &= ~renderMode;
	else
		this->renderMode |= renderMode;
}

unsigned int Renderer::GetRenderMode()
{
	return renderMode;
}

void Renderer::SetRenderMode(unsigned int renderMode)
{
	this->renderMode = renderMode;
}


bool Renderer::HasDisplayMode(unsigned int displayMode)
{
	return (this->displayMode & displayMode) == displayMode;
}

void Renderer::ToggleDisplayMode(unsigned int displayMode)
{
	if (HasDisplayMode(displayMode))
		this->displayMode &= ~displayMode;
	else
		this->displayMode |= displayMode;

	if (displayMode == DISPLAY_PERS)
		memset(pers_bg, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
}

unsigned int Renderer::GetDisplayMode()
{
	return displayMode;
}

void Renderer::SetDisplayMode(unsigned int displayMode)
{
	this->displayMode = displayMode;
}

void Renderer::SetColorMode(unsigned int color_mode)
{
	colorMode = color_mode;
}

void Renderer::XORColorMode(unsigned int color_mode)
{
	colorMode ^= color_mode;
}

unsigned int Renderer::GetColorMode()
{
	return colorMode;
}

// Called when loading tabs. Used to load some renderer settings
void Renderer::LoadSave(Save *save)
{
	if (!save)
		return;

	if (save->renderModePresent)
		renderMode = save->renderMode;

	if (save->displayModePresent)
		displayMode = save->displayMode;

	if (save->colorModePresent)
		colorMode = save->colorMode;
}

// Called when creating tabs. Used to save some renderer settings
void Renderer::CreateSave(Save *save)
{
	save->decorationsEnable = decorations_enable;
	save->decorationsEnablePresent = true;
	save->hudEnable = hud_enable;
	save->hudEnablePresent = true;
	save->activeMenu = active_menu;
	save->activeMenuPresent = true;

	save->renderMode = renderMode;
	save->renderModePresent = true;

	save->displayMode = displayMode;
	save->displayModePresent = true;

	save->colorMode = colorMode;
	save->colorModePresent = true;
}
