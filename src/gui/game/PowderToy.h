#ifndef POWDERTOY_H
#define POWDERTOY_H

#include <functional>
#include <string>
#include "defines.h"
#include "interface/Window.h"
#include "graphics/Pixel.h"

class Button;
class Request;
class ToolTip;
class Save;
class Simulation;
class VideoBuffer;
class PowderToy : public ui::Window
{
public:
	enum StampState { NONE, LOAD, COPY, CUT, SAVE };
	enum DrawState { POINTS, LINE, RECT, FILL };
	enum MouseupReason { mouseUpNormal, mouseUpBlur, mouseUpDrawEnd };

private:
	Point mouse;
	Point cursor;
	int lastMouseDown, heldKey = 0, heldScan = 0;
	bool mouseClickCanceled = false; // allow Lua to cancel mouse clicks in the legacy C code
	int16_t orientation[3];

	// notifications
	int numNotifications;
	Button * AddNotification(std::string message, std::function<void(int)> callback);

	std::string vanillaMotd;
	std::string starcatcherMotd;
	int motdToggle = 0;

	// website stuff
	Request *versionCheck = nullptr;
	Request *sessionCheck = nullptr; // really a tpt++ version check but it does session too and has nice things
	Request *voteDownload = nullptr;
	bool delayedHttpChecks;
	void DelayedHttpInitialization();

	// drawing stuff
	DrawState drawState;
	bool isMouseDown;
	bool isStampMouseDown;
	int toolIndex; // 0 = left mouse, 1 = right mouse
	float toolStrength; // tool (heat / cool) strength can be modified with ctrl or shift
	Point lastDrawPoint; // for normal point to point drawing
	Point initialDrawPoint; // for lines and boxes
	bool ctrlHeld;
	bool shiftHeld;
	bool altHeld;
	bool mouseInZoom; // mouse drawing is canceled when moving in / out of the zoom window, need state
	bool skipDraw; // when mouse moves, don't attempt an extra draw the next tick as normal

	std::string decoTools[3];
	std::string regularTools[3]; // used to store backups of what was selected before entering/exiting deco menu

	// zoom
	bool placingZoom;
	bool placingZoomTouch; // clicked the zoom button, zoom window won't be drawn until user clicks
	bool zoomEnabled;
	Point zoomScopePosition; // position the zoom window is zooming in on
	Point zoomWindowPosition; // position where zoom is drawn on screen (either on the left or the right)
	Point zoomMousePosition; // position where the mouse was when placing the zoom window, needed so that we can resize it without glitching things
	int zoomScopeSize;
	int zoomFactor;
	
	// Simulation object
	Simulation * sim;

	// used when reloading saves / local saves / tabs
	Save * reloadSave;

	// loading stamps
	StampState state;
	Point loadPos;
	Point loadSize;
	Save *stampData;
	pixel *stampImg;
	Point stampOffset;
	bool waitToDraw; // wait a frame to draw stamp after load, because it will be in the wrong spot until another mouse event comes in
	// touch ui stuff for rotating / moving stamps
#ifdef TOUCHUI
	Point stampClickedPos;
	Point stampClickedOffset;
	Point initialLoadPos;
	int stampQuadrant;
	bool stampMoving;
#endif

	// saving stamps
	Point savePos;
	Point saveSize;
	Save *clipboardData;

	bool showBrush = true;

	// bottom bar buttons
	Button *openBrowserButton;
	Button *reloadButton;
	Button *saveButton;
	Button *upvoteButton;
	Button *downvoteButton;
	Button *openTagsButton;
	Button *reportBugButton;
	Button *optionsButton;
	Button *clearSimButton;
	unsigned int loginCheckTicks;
	int loginFinished;
	Button *loginButton;
	Button *renderOptionsButton;
	Button *pauseButton;

	// these buttons only appear when using the touchscreen interface
#ifdef TOUCHUI
	Button *eraseButton;
	Button *openConsoleButton;
	Button *settingsButton;
	Button *zoomButton;
	Button *stampButton;
#endif
	bool ignoreMouseUp;
	bool insideRenderOptions;
	bool deletingRenderOptions;
	bool previousPause;
	bool restorePreviousPause;

public:
	PowderToy();
	~PowderToy();

	void ConfirmUpdate(std::string changelog, std::string file);
	bool MouseClicksIgnored();
	void AdjustCursorSize(int d, bool keyShortcut);
	Point AdjustCoordinates(Point mouse);
	Point SnapCoordinatesWall(Point pos1, Point pos2);
	bool IsMouseInZoom(Point mouse);
	void SetInfoTip(std::string infotip);
	ToolTip *GetQTip(std::string qtip, int y);
	std::string GetMotd();

	// drawing stuff
	void UpdateDrawMode();
	void UpdateToolStrength();
	DrawState GetDrawState() { return drawState; }
	Point GetMousePos() { return mouse; }
	bool IsMouseDown() { return isMouseDown; }
	float GetToolStrength() { return toolStrength; }
	Point GetInitialDrawPoint() { return initialDrawPoint; }
	int GetToolIndex() { return toolIndex; }
	Point LineSnapCoords(Point point1, Point point2);
	Point RectSnapCoords(Point point1, Point point2);

	void SwapToDecoToolset();
	void SwapToRegularToolset();

	// zoom window stuff
	bool IsZoomEnabled() { return zoomEnabled; }
	void SetZoomEnabled(bool zoomEnabled) { this->zoomEnabled = zoomEnabled; }
	bool PlacingZoomWindow() { return placingZoom; }
	bool ZoomWindowShown() { return placingZoom || zoomEnabled; }
	Point GetZoomScopePosition() { return zoomScopePosition; }
	void SetZoomScopePosition(Point zoomScopePosition) { this->zoomScopePosition = zoomScopePosition; }
	Point GetZoomWindowPosition() { return zoomWindowPosition; }
	void SetZoomWindowPosition(Point zoomWindowPosition) { this->zoomWindowPosition = zoomWindowPosition; }
	int GetZoomScopeSize() { return zoomScopeSize; }
	void SetZoomScopeSize(int zoomScopeSize) { this->zoomScopeSize = zoomScopeSize; }
	int GetZoomWindowFactor() { return zoomFactor; }
	void SetZoomWindowFactor(int zoomFactor) { this->zoomFactor = zoomFactor; }
	void UpdateZoomCoordinates(Point mouse);
	void HideZoomWindow();

	void ReloadSave();
	void SetReloadPoint(Save * reloadSave);

	// stamp stuff (so main() can get needed info)
	void UpdateStampCoordinates(Point cursor, Point offset = Point(0, 0));
	StampState GetStampState() { return state; }
	void ResetStampState();
	Point GetStampPos() { return (loadPos - (loadSize - stampOffset)/2) / CELL * CELL; }
	Point GetStampCenterPos() { return (loadPos + stampOffset/2) / CELL * CELL; }
	Point GetStampSize() { return loadSize; }
	pixel * GetStampImg() { return waitToDraw ? NULL : stampImg; }
	Point GetSavePos() { return savePos; }
	Point GetSaveSize() { return saveSize; }
	bool PlacedInitialStampCoordinate() { return isStampMouseDown; }
	void TranslateSave(Point point);
	void TransformSave(int a, int b, int c, int d);

	void OnTick(uint32_t ticks) override;
	void OnDraw(gfx::VideoBuffer *buf) override;
	void OnMouseMove(int x, int y, Point difference) override;
	void OnMouseDown(int x, int y, unsigned char button) override;
	void OnMouseUp(int x, int y, unsigned char button) override;
	void OnMouseWheel(int x, int y, int d) override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	void OnKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	void OnDefocus() override;
	void OnJoystickMotion(uint8_t joysticknum, uint8_t axis, int16_t value) override;
	void OnFileDrop(const char *filename) override;

	bool BeforeMouseMove(int x, int y, Point difference) override;
	bool BeforeMouseDown(int x, int y, unsigned char button) override;
	bool BeforeMouseUp(int x, int y, unsigned char button) override;
	bool BeforeMouseWheel(int x, int y, int d) override;
	bool BeforeKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	bool BeforeKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	bool BeforeTextInput(const char *text) override;

	void OpenBrowserBtn(unsigned char b);
	void ReloadSaveBtn(unsigned char b);
	void DoSaveBtn(unsigned char b);
	void DoVoteBtn(bool up);
	void OpenTagsBtn();
	void ReportBugBtn();
	void OpenOptionsBtn();
	void LoginBtn();
	void RenderOptionsBtn();
	void TogglePauseBtn();
	void SetPauseBtn(bool pause);

	void TogglePause();
	void SetPause(bool pause);
	void SetBrushEnable(bool showBrush) { this->showBrush = showBrush; }
	bool GetBrushEnable() { return showBrush; }
	void OpenConsole();

	bool IsinsideRenderOptions() { return insideRenderOptions; }
	void LoadRenderPreset(int preset);

#ifdef TOUCHUI
	void ToggleEraseBtn(bool alt);
	void OpenConsoleBtn(bool alt);
	void ToggleSettingBtn(bool alt);
	void StartZoomBtn(bool alt);
	void SaveStampBtn(bool alt);
#endif
};

#endif
