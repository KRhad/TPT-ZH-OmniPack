#include "common/tpt-minmax.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include "SDLCompat.h" // for SDL_Start/StopTextInput
#include "json/json.h"

#include "defines.h"
#include "EventLoopSDL.h" // for two mouse_get_state that should be removed ...
#include "graphics.h"
#include "hud.h"
#include "interface.h"
#include "IntroText.h"
#include "luaconsole.h"
#include "powder.h"
#include "powdergraphics.h"
#include "PowderToy.h"
#include "misc.h"
#include "save_legacy.h"
#include "update.h"

#include "common/Format.h"
#include "common/Matrix.h"
#include "common/Platform.h"
#include "game/Authors.h"
#include "game/Brush.h"
#include "game/Request.h"
#include "game/Menus.h"
#include "game/Save.h"
#include "game/Sign.h"
#include "game/Stamps.h"
#include "game/ToolTip.h"
#include "graphics/Renderer.h"
#include "graphics/VideoBuffer.h"
#include "interface/Button.h"
#include "interface/Engine.h"
#include "interface/Style.h"
#include "interface/Window.h"
#include "lua/LuaEvents.h"
#include "simulation/Simulation.h"
#include "simulation/SnapshotHistory.h"
#include "simulation/Tool.h"
#include "simulation/ToolNumbers.h"

#include "gui/console/Console.h"
#include "gui/dialogs/ConfirmPrompt.h"
#include "gui/dialogs/InfoPrompt.h"
#include "gui/dialogs/ErrorPrompt.h"
#include "gui/gol/GolWindow.h"
#include "gui/login/Login.h"
#include "gui/options/OptionsUI.h"
#include "gui/profile/ProfileViewer.h"
#include "gui/prop/PropWindow.h"
#include "gui/sign/CreateSign.h"
#include "gui/rendermodes/RenderModesUI.h"
#include "gui/update/UpdateProgress.h"

#include "simulation/elements/LIFE.h"
#include "simulation/elements/STKM.h"

PowderToy::~PowderToy()
{
	CloseEvent ev = CloseEvent();
	HandleEvent(LuaEvents::close, &ev);

	main_end_hack();
	Stamps::Ref().Free();
	delete sim;
	delete clipboardData;
	delete reloadSave;

}

PowderToy::PowderToy():
	ui::Window(Point(0, 0), Point(XRES+BARSIZE, YRES+MENUSIZE)),
	mouse(Point(0, 0)),
	cursor(Point(0, 0)),
	lastMouseDown(0),
	numNotifications(0),
	voteDownload(NULL),
	delayedHttpChecks(true),
	drawState(POINTS),
	isMouseDown(false),
	isStampMouseDown(false),
	toolIndex(0),
	toolStrength(1.0f),
	lastDrawPoint(Point(0, 0)),
	initialDrawPoint(Point(0, 0)),
	ctrlHeld(false),
	shiftHeld(false),
	altHeld(false),
	mouseInZoom(false),
	skipDraw(false),
	placingZoom(false),
	placingZoomTouch(false),
	zoomEnabled(false),
	zoomScopePosition(Point(0, 0)),
	zoomWindowPosition(Point(0, 0)),
	zoomMousePosition(Point(0, 0)),
	zoomScopeSize(32),
	zoomFactor(8),
	reloadSave(NULL),
	state(NONE),
	loadPos(Point(0, 0)),
	loadSize(Point(0, 0)),
	stampData(NULL),
	stampImg(NULL),
	stampOffset(Point(0, 0)),
	waitToDraw(false),
#ifdef TOUCHUI
	stampClickedPos(Point(0, 0)),
	stampClickedOffset(Point(0, 0)),
	initialLoadPos(Point(0, 0)),
	stampQuadrant(0),
	stampMoving(false),
#endif
	savePos(Point(0, 0)),
	saveSize(Point(0, 0)),
	clipboardData(NULL),
	loginCheckTicks(0),
	loginFinished(0),
	ignoreMouseUp(false),
	insideRenderOptions(false),
	deletingRenderOptions(false),
	previousPause(false),
	restorePreviousPause(false)
{
	ignoreQuits = true;
	hasBorder = false;

	sim = new Simulation();
	globalSim = sim;

	load_presets();

	InitMenusections();
	FillMenus();
	regularTools[0] = "DEFAULT_PT_DUST";
	regularTools[1] = "DEFAULT_PT_NONE";
	regularTools[2] = "DEFAULT_PT_NONE";
	decoTools[0] = "DEFAULT_DECOR_SET";
	decoTools[1] = "DEFAULT_DECOR_CLR";
	decoTools[2] = "DEFAULT_PT_NONE";
	activeTools[0] = GetToolFromIdentifier(regularTools[0]);
	activeTools[1] = GetToolFromIdentifier(regularTools[1]);
	activeTools[2] = GetToolFromIdentifier(regularTools[2]);

	// start placing the bottom row of buttons, starting from the left
#ifdef TOUCHUI
	const int ySize = 20;
	const int xOffset = 0;
	const int tooltipAlpha = 255;
	const int minWidth = 38;
#else
	const int ySize = 15;
	const int xOffset = 1;
	const int tooltipAlpha = -2;
	const int minWidth = 0;
#endif

#ifdef TOUCHUI
	openBrowserButton = new Button(Point(xOffset, YRES+MENUSIZE-20), Point(minWidth, ySize), "\x81");
#else
	openBrowserButton = new Button(Point(xOffset, YRES+MENUSIZE-16), Point(18-xOffset, ySize), "\x81");
#endif
	openBrowserButton->SetCallback([&](int mb) { this->OpenBrowserBtn(mb); });
#ifdef TOUCHUI
	openBrowserButton->SetState(Button::HOLD);
#endif
	openBrowserButton->SetTooltip(new ToolTip("查找并打开模拟", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(openBrowserButton);

	reloadButton = new Button(openBrowserButton->Right(Point(1, 0)), Point(minWidth > 17 ? minWidth : 17, ySize), "\x91");
	reloadButton->SetCallback([&](int mb) { this->ReloadSaveBtn(mb); });
	reloadButton->SetEnabled(false);
#ifdef TOUCHUI
	reloadButton->SetState(Button::HOLD);
#endif
	reloadButton->SetTooltip(new ToolTip("重新加载模拟 \bg(Ctrl+R)", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(reloadButton);

	saveButton = new Button(reloadButton->Right(Point(1, 0)), Point(minWidth > 151 ? minWidth : 151, ySize), "\x82 [未命名模拟]");
	saveButton->SetAlign(Button::LEFT);
	saveButton->SetCallback([&](int mb) { this->DoSaveBtn(mb); });
#ifdef TOUCHUI
	saveButton->SetState(Button::HOLD);
#endif
	saveButton->SetTooltip(new ToolTip("上传新模拟", Point(minWidth > 16 ? minWidth : 16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(saveButton);

#ifdef TOUCHUI
	upvoteButton = new Button(saveButton->Right(Point(1, 0)), Point(minWidth > 40 ? minWidth: 40, ySize), "\xCB");
#else
	upvoteButton = new Button(saveButton->Right(Point(1, 0)), Point(minWidth > 40 ? minWidth: 40, ySize), "\xCB 投票");
#endif
	upvoteButton->SetColor(COLRGB(0, 187, 18));
	upvoteButton->SetCallback([&](int mb) { this->DoVoteBtn(true); });
	upvoteButton->SetTooltip(new ToolTip("赞这个存档", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(upvoteButton);

	downvoteButton = new Button(upvoteButton->Right(Point(0, 0)), Point(minWidth > 16 ? minWidth : 16, ySize), "\xCA");
	downvoteButton->SetColor(COLRGB(187, 40, 0));
	downvoteButton->SetCallback([&](int mb) { this->DoVoteBtn(false); });
	downvoteButton->SetTooltip(new ToolTip("不喜欢此存档", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(downvoteButton);


	// We now start placing buttons from the right side, because tags button is in the middle and uses whatever space is leftover
	Point size = Point(minWidth > 15 ? minWidth : 15, ySize);
	pauseButton = new Button(Point(XRES+BARSIZE-size.X-xOffset, openBrowserButton->GetPosition().Y), size, "\x90");
	pauseButton->SetCallback([&](int mb) { this->TogglePauseBtn(); });
	pauseButton->SetTooltip(new ToolTip("暂停模拟 \bg(空格)", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(pauseButton);

	size = Point(minWidth > 17 ? minWidth : 17, ySize);
	renderOptionsButton = new Button(pauseButton->Left(Point(size.X+1, 0)), size, "\xD8");
	renderOptionsButton->SetCallback([&](int mb) { this->RenderOptionsBtn(); });
	renderOptionsButton->SetTooltip(new ToolTip("渲染设置", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(renderOptionsButton);

	size = Point(minWidth > 95 ? minWidth : 95, ySize);
	loginButton = new Button(renderOptionsButton->Left(Point(size.X+1, 0)), size, "\x84 [登录]");
	loginButton->SetAlign(Button::LEFT);
	loginButton->SetCallback([&](int mb) { this->LoginBtn(); });
	loginButton->SetTooltip(new ToolTip("登录模拟服务器", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(loginButton);

	size = Point(minWidth > 17 ? minWidth : 17, ySize);
	clearSimButton = new Button(loginButton->Left(Point(size.X+1, 0)), size, "\x92");
	clearSimButton->SetCallback([](int mb) { NewSim(); });
	clearSimButton->SetTooltip(new ToolTip("清除所有粒子和墙体", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(clearSimButton);

	size = Point(minWidth > 15 ? minWidth : 15, ySize);
	optionsButton = new Button(clearSimButton->Left(Point(size.X+1, 0)), size, "\xCF");
	optionsButton->SetCallback([&](int mb) { this->OpenOptionsBtn(); });
	optionsButton->SetTooltip(new ToolTip("模拟设置", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(optionsButton);

	size = Point(minWidth > 15 ? minWidth : 15, ySize);
	reportBugButton = new Button(optionsButton->Left(Point(size.X+1, 0)), size, "\xE7");
	reportBugButton->SetCallback([&](int mb) { this->ReportBugBtn(); });
	reportBugButton->SetTooltip(new ToolTip("向 jacob1 报告问题和反馈", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(reportBugButton);

	Point tagsPos = downvoteButton->Right(Point(1, 0));
#ifdef TOUCHUI
	openTagsButton = new Button(tagsPos, Point((reportBugButton->Left(Point(1, 0))-tagsPos).X, ySize), "\x83");
#else
	openTagsButton = new Button(tagsPos, Point((reportBugButton->Left(Point(1, 0))-tagsPos).X, ySize), "\x83 [未设置标签]");
	openTagsButton->SetAlign(Button::LEFT);
#endif
	openTagsButton->SetCallback([&](int mb) { this->OpenTagsBtn(); });
	openTagsButton->SetTooltip(new ToolTip("添加模拟标签", Point(16, YRES-24), TOOLTIP, tooltipAlpha));
	AddComponent(openTagsButton);

#ifdef TOUCHUI
	eraseButton = new Button(Point(XRES+1, 0), Point(BARSIZE-1, 25), "\xE8");
	eraseButton->SetState(Button::HOLD);
	eraseButton->SetCallback([&](int mb) { this->ToggleEraseBtn(mb == 3); });
	eraseButton->SetTooltip(GetQTip("切换到擦除工具（长按清空模拟）", eraseButton->GetPosition().Y+10));
	AddComponent(eraseButton);

	openConsoleButton = new Button(eraseButton->Below(Point(0, 1)), Point(BARSIZE-1, 25), "\xE9");
	openConsoleButton->SetState(Button::HOLD);
	openConsoleButton->SetCallback([&](int mb) { this->OpenConsoleBtn(mb == 3); });

	openConsoleButton->SetTooltip(GetQTip("打开控制台（长按显示屏幕键盘）", openConsoleButton->GetPosition().Y+10));
	AddComponent(openConsoleButton);

	settingsButton = new Button(openConsoleButton->Below(Point(0, 1)), Point(BARSIZE-1, 25), "\xEB");
	settingsButton->SetState(Button::HOLD);
	settingsButton->SetCallback([&](int mb) { this->ToggleSettingBtn(mb == 3); });
	settingsButton->SetTooltip(GetQTip("切换装饰层（长按打开设置）", settingsButton->GetPosition().Y+10));
	AddComponent(settingsButton);

	zoomButton = new Button(settingsButton->Below(Point(0, 1)), Point(BARSIZE-1, 25), "\xEC");
	zoomButton->SetState(Button::HOLD);
	zoomButton->SetCallback([&](int mb) { this->StartZoomBtn(mb == 3); });
	zoomButton->SetTooltip(GetQTip("开始放置缩放窗口", zoomButton->GetPosition().Y+10));
	AddComponent(zoomButton);

	stampButton = new Button(zoomButton->Below(Point(0, 1)), Point(BARSIZE-1, 25), "\xEA");
	stampButton->SetState(Button::HOLD);
	stampButton->SetCallback([&](int mb) { this->SaveStampBtn(mb == 3); });
	stampButton->SetTooltip(GetQTip("保存图章（长按加载图章）", stampButton->GetPosition().Y+10));
	AddComponent(stampButton);
#endif
}

void PowderToy::DelayedHttpInitialization()
{
	if (doUpdates)
	{
		versionCheck = new Request(UPDATESCHEME UPDATESERVER "/Startup.json");
		if (svf_login)
			versionCheck->AuthHeaders(svf_user, ""); //username instead of session
		versionCheck->Start();
	}

	if (svf_login)
	{
		sessionCheck = new Request(SCHEME SERVER "/Startup.json");
		sessionCheck->AuthHeaders(svf_user_id, svf_session_id);
		sessionCheck->Start();
	}
}

void PowderToy::OpenBrowserBtn(unsigned char b)
{
	if (voteDownload)
	{
		voteDownload->Cancel();
		voteDownload = NULL;
		svf_myvote = 0;
		SetInfoTip("错误：上一次投票可能未成功提交");
	}
#ifdef TOUCHUI
	if (ctrlHeld || b != 1)
#else
	if (ctrlHeld)
#endif
		catalogue_ui(vid_buf);
	else
		search_ui(vid_buf);
}

void PowderToy::ReloadSaveBtn(unsigned char b)
{
	if (b == 1 || !strncmp(svf_id, "", 8))
		ReloadSave();
	else
		open_ui(vid_buf, svf_id, NULL, 0);
}

void PowderToy::DoSaveBtn(unsigned char b)
{
	Save *save = sim->CreateSave(0, 0, XRES, YRES);
	bool canReupload = true;
	if (!strcmp(svf_id, "404") || !strcmp(svf_id, "2157797"))
		canReupload = false;
	// Local save
#ifdef TOUCHUI
	if (!svf_login || ctrlHeld || !canReupload || b != 1)
#else
	if (!svf_login || ctrlHeld || !canReupload)
#endif
	{
		Json::Value localSaveInfo;
		localSaveInfo["type"] = "localsave";
		localSaveInfo["username"] = svf_user;
		localSaveInfo["title"] = svf_fileopen ? svf_filename : "unknown";
		localSaveInfo["date"] = (Json::Value::UInt64)time(NULL);
		SaveAuthorInfo(&localSaveInfo);
		save->authors = localSaveInfo;

		bool isQuickSave = mouse.X <= saveButton->GetPosition().X+18 && svf_fileopen;
		// local quick save
		try
		{
			if (!isQuickSave)
			{
				int ret = save_filename_ui(vid_buf, save);
				if (!ret)
				{
					authors = save->authors;
#ifndef TOUCHUI
					SetInfoTip("已创建本地存档。按住 Ctrl 并点击打开按钮可进入本地存档浏览器");
#else
					SetInfoTip("已创建本地存档。长按打开按钮可进入本地存档浏览器");
#endif
				}
			}
			else if (DoLocalSave(svf_filename, save, true))
				SetInfoTip("写入本地存档失败");
			else
				SetInfoTip("更新成功");
		}
		catch (BuildException & e)
		{
			clear_save_info();
			Engine::Ref().ShowWindow(new ErrorPrompt("创建存档失败：" + std::string(e.what())));
		}
	}
	// Online save
	else
	{
		bool isQuickSave = true;
		// not a quicksave
		if (!svf_open || !svf_own || mouse.X > saveButton->GetPosition().X+18)
		{
			// if user canceled the save, then do nothing
			if (!save_name_ui(vid_buf))
			{
				delete save;
				return;
			}
			isQuickSave = false;
		}

		Json::Value serverSaveInfo;
		serverSaveInfo["type"] = "save";
		serverSaveInfo["username"] = svf_user;
		serverSaveInfo["id"] = svf_id[0] ? atoi(svf_id) : -1;
		serverSaveInfo["title"] = svf_name;
		serverSaveInfo["description"] = svf_description;
		serverSaveInfo["published"] = svf_publish;
		serverSaveInfo["date"] = (Json::Value::UInt64)time(NULL);
		SaveAuthorInfo(&serverSaveInfo);
		save->authors = serverSaveInfo;

		try
		{
			bool success = !execute_save(vid_buf, save);
			if (success)
			{
				if (isQuickSave)
					SetInfoTip("保存成功");
				else
					copytext_ui(vid_buf, "存档 ID", "保存成功！", svf_id);
				save->authors["id"] = Format::StringToNumber<int>(svf_id);
				authors = save->authors;
			}
			else
				SetInfoTip("保存失败");
		}
		catch (BuildException & e)
		{
			clear_save_info();
			Engine::Ref().ShowWindow(new ErrorPrompt("创建存档失败：" + std::string(e.what())));
		}
	}
	delete save;
}

void PowderToy::DoVoteBtn(bool up)
{
	if (voteDownload != NULL)
	{
		SetInfoTip("错误：无法投票");
		return;
	}
	bool isReset = (up && svf_myvote == 1) || (!up && svf_myvote == -1);
	voteDownload = new Request(SCHEME SERVER "/Vote.api");
	voteDownload->AuthHeaders(svf_user_id, svf_session_id);
	voteDownload->AddPostData(http::FormData{
		{ "ID", svf_id },
		{ "Action", isReset ? "Reset" : (up ? "Up" : "Down") },
		{ "Key", svf_session_key }
	});
	voteDownload->Start();
	svf_myvote = isReset ? 0 : (up ? 1 : -1); // will be reset later upon error
}

void PowderToy::OpenTagsBtn()
{
	tag_list_ui(vid_buf);
}

void PowderToy::ReportBugBtn()
{
	report_ui(vid_buf, NULL, true);
}

void PowderToy::OpenOptionsBtn()
{
	OptionsUI *optionsUI = new OptionsUI(sim);
	Engine::Ref().ShowWindow(optionsUI);
	save_presets();
}

void PowderToy::LoginBtn()
{
	if (svf_login && mouse.X <= loginButton->GetPosition().X+18)
	{
		ProfileViewer *temp = new ProfileViewer(svf_user);
		Engine::Ref().ShowWindow(temp);
	}
	else
	{
		Login *temp = new Login({ [&]() {
			if (svf_login)
			{
				if (sessionCheck)
				{
					sessionCheck->Cancel();
					sessionCheck = NULL;
				}
				loginFinished = 1;
			}
		} });
		Engine::Ref().ShowWindow(temp);
	}
}

void PowderToy::RenderOptionsBtn()
{
	RenderModesUI *renderModes = new RenderModesUI();
	this->AddSubwindow(renderModes);
	renderModes->HasBorder(true);
	previousPause = sys_pause;
	SetPause(1);
	insideRenderOptions = true;
	deletingRenderOptions = false;
	restorePreviousPause = true;
}

void PowderToy::TogglePauseBtn()
{
	TogglePause();
}

void PowderToy::SetPauseBtn(bool pause)
{
	SetPause(pause);
}

void PowderToy::TogglePause()
{
	if (sys_pause && sim->debug_currentParticle)
	{
#ifdef LUACONSOLE
		std::stringstream logmessage;
		logmessage << "Updated particles from #" << sim->debug_currentParticle << " to end due to unpause";
		luacon_log(logmessage.str());
#endif
		sim->UpdateParticles(sim->debug_currentParticle, NPART);
		sim->UpdateAfter();
		sim->debug_currentParticle = 0;
	}
	sys_pause = !sys_pause;
	restorePreviousPause = false;
}

void PowderToy::SetPause(bool pause)
{
	if (pause != sys_pause)
		TogglePause();
}

void PowderToy::OpenConsole()
{
	Console *console = new Console();
	Engine::Ref().ShowWindow(console);
}

// functions called by touch interface buttons are here
#ifdef TOUCHUI
void PowderToy::ToggleEraseBtn(bool alt)
{
	if (alt)
	{
		NewSim();
		SetInfoTip("已清空模拟");
	}
	else
	{
		Tool *erase = GetToolFromIdentifier("DEFAULT_PT_NONE");
		if (activeTools[0] == erase)
		{
			activeTools[0] = activeTools[1];
			activeTools[1] = erase;
			SetInfoTip("已取消擦除工具");
		}
		else
		{
			activeTools[1] = activeTools[0];
			activeTools[0] = erase;
			SetInfoTip("已选择擦除工具");
		}
	}
}

void PowderToy::OpenConsoleBtn(bool alt)
{
	if (alt)
		Platform::ShowOnScreenKeyboard("", false);
	else
	{
		Console *console = new Console();
		Engine::Ref().ShowWindow(console);
	}
}

void PowderToy::ToggleSettingBtn(bool alt)
{
	if (alt)
		OpenOptionsBtn();
	else
	{
		decorations_enable = !decorations_enable;
		if (decorations_enable)
			SetInfoTip("装饰层已开启");
		else
			SetInfoTip("装饰层已关闭");
	}
}

void PowderToy::StartZoomBtn(bool alt)
{
	if (ZoomWindowShown() || placingZoomTouch)
		HideZoomWindow();
	else
	{
		placingZoomTouch = true;
		UpdateZoomCoordinates(mouse);
	}
}

void PowderToy::SaveStampBtn(bool alt)
{
	if (alt)
	{
		ResetStampState();

		int reorder = 1;
		int stampID = stamp_ui(vid_buf, &reorder);
		if (stampID >= 0)
			stampData = Stamps::Ref().Load(stampID, reorder);
		else
			stampData = NULL;

		if (stampData)
		{
			stampImg = prerender_save((char*)stampData->GetSaveData(), stampData->GetSaveSize(), &loadSize.X, &loadSize.Y);
			if (stampImg)
			{
				state = LOAD;
				loadPos.X = CELL*((XRES-loadSize.X+CELL)/2/CELL);
				loadPos.Y = CELL*((YRES-loadSize.Y+CELL)/2/CELL);
				stampClickedPos = Point(XRES, YRES)/2;
				ignoreMouseUp = true;
				waitToDraw = true;
			}
			else
			{
				free(stampData);
				stampData = NULL;
			}
		}
	}
	else
	{
		if (state == NONE)
		{
			state = SAVE;
			isStampMouseDown = false;
			ignoreMouseUp = true;
		}
		else
			ResetStampState();
	}
}

#endif

// misc main gui functions
void PowderToy::ConfirmUpdate(std::string changelog, std::string file)
{
#ifdef ANDROID
	std::string title = "\bw是否更新 TPT？";
#else
	std::string title = "\bwDo you want to update Jacob1's Mod?";
#endif

	auto prompt = new ConfirmPrompt(title, changelog, "\bt更新");
	prompt->SetCallback({ [file](bool confirmed) {
		if (confirmed)
		{
#if defined(ANDROID) || defined(MACOSX)
			Platform::OpenLink(file);
#else
			UpdateProgress * update = new UpdateProgress(file, svf_user, [](char *data, int len)
			{
				if (!do_update(data, len))
					Engine::Ref().Shutdown();
				else
				{
					ErrorPrompt *error = new ErrorPrompt("更新失败 - 尝试下载新版本。");
					Engine::Ref().ShowWindow(error);
				}
			});
			Engine::Ref().ShowWindow(update);
#endif
		}
	} });
	Engine::Ref().ShowWindow(prompt);
}

void PowderToy::UpdateDrawMode()
{
	if (ctrlHeld && shiftHeld)
	{
		int tool = ((ToolTool*)activeTools[toolIndex])->GetID();
		if (tool == -1 || tool == TOOL_PROP)
			drawState = FILL;
		else
			drawState = POINTS;
	}
	else if (ctrlHeld)
		drawState = RECT;
	else if (shiftHeld)
		drawState = LINE;
	else
		drawState = POINTS;
}

void PowderToy::UpdateToolStrength()
{
	if (shiftHeld)
		toolStrength = 10.0f;
	else if (ctrlHeld)
		toolStrength = .1f;
	else
		toolStrength = 1.0f;
}

Point PowderToy::LineSnapCoords(Point point1, Point point2)
{
	Point diff = point2 - point1;
	if (abs(diff.X / 2) > abs(diff.Y)) // vertical
		return point1 + Point(diff.X, 0);
	else if(abs(diff.X) < abs(diff.Y / 2)) // horizontal
		return point1 + Point(0, diff.Y);
	else if(diff.X * diff.Y > 0) // NW-SE
		return point1 + Point((diff.X + diff.Y)/2, (diff.X + diff.Y)/2);
	else // SW-NE
		return point1 + Point((diff.X - diff.Y)/2, (diff.Y - diff.X)/2);
}

Point PowderToy::RectSnapCoords(Point point1, Point point2)
{
	Point diff = point2 - point1;
	if (diff.X * diff.Y > 0) // NW-SE
		return point1 + Point((diff.X + diff.Y)/2, (diff.X + diff.Y)/2);
	else // SW-NE
		return point1 + Point((diff.X - diff.Y)/2, (diff.Y - diff.X)/2);
}

void PowderToy::SwapToDecoToolset()
{
	regularTools[0] = activeTools[0]->GetIdentifier();
	regularTools[1] = activeTools[1]->GetIdentifier();
	regularTools[2] = activeTools[2]->GetIdentifier();
	activeTools[0] = GetToolFromIdentifier("DEFAULT_DECOR_SET");
	activeTools[1] = GetToolFromIdentifier(decoTools[1]);
	activeTools[2] = GetToolFromIdentifier(decoTools[2]);
	if (activeTools[1] == nullptr)
		activeTools[1] = GetToolFromIdentifier("DEFAULT_DECOR_CLR");
	if (activeTools[2] == nullptr)
		activeTools[2] = GetToolFromIdentifier("DEFAULT_PT_NONE");
}

void PowderToy::SwapToRegularToolset()
{
	decoTools[0] = activeTools[0]->GetIdentifier();
	decoTools[1] = activeTools[1]->GetIdentifier();
	decoTools[2] = activeTools[2]->GetIdentifier();
	activeTools[0] = GetToolFromIdentifier(regularTools[0]);
	activeTools[1] = GetToolFromIdentifier(regularTools[1]);
	activeTools[2] = GetToolFromIdentifier(regularTools[2]);

	if (activeTools[0] == nullptr)
		activeTools[0] = GetToolFromIdentifier("DEFAULT_PT_DUST");
	if (activeTools[1] == nullptr)
		activeTools[1] = GetToolFromIdentifier("DEFAULT_PT_NONE");
	if (activeTools[2] == nullptr)
		activeTools[2] = GetToolFromIdentifier("DEFAULT_PT_NONE");
}

bool PowderToy::MouseClicksIgnored()
{
	return PlacingZoomWindow() || state != NONE;
}

void PowderToy::AdjustCursorSize(int d, bool keyShortcut)
{
	if (PlacingZoomWindow())
	{
		if (keyShortcut && !altHeld)
			d = d * static_cast<int>(std::ceil(zoomScopeSize / 5.0f + 0.5f));
		zoomScopeSize = std::max(2, std::min(zoomScopeSize+d, 60));
		zoomFactor = 256/zoomScopeSize;
		UpdateZoomCoordinates(zoomMousePosition);
		return;
	}

	if (altHeld && !ctrlHeld && !shiftHeld)
		currentBrush->ChangeRadius(Point(d > 0 ? 1 : -1, d > 0 ? 1 : -1));
	else if (shiftHeld && !ctrlHeld)
		currentBrush->ChangeRadius(Point(d, 0));
	else if (ctrlHeld && !shiftHeld)
		currentBrush->ChangeRadius(Point(0, d));
	else
	{
		if (keyShortcut && !altHeld)
			d = d * static_cast<int>(std::ceil(currentBrush->GetRadius().X / 5.0f + 0.5f));
		currentBrush->ChangeRadius(Point(d, d));
	}
}

Point PowderToy::AdjustCoordinates(Point mouse)
{
	//adjust coords into the simulation area
	mouse.Clamp(Point(0, 0), Point(XRES-1, YRES-1));

	//Change mouse coords to take zoom window into account
	if (ZoomWindowShown())
	{
		Point zoomWindowPosition2 = zoomWindowPosition + Point(zoomFactor * zoomScopeSize, zoomFactor * zoomScopeSize);
		if (mouse.IsInside(zoomWindowPosition, zoomWindowPosition2))
		{
			mouse.X = ((mouse.X - zoomWindowPosition.X) / zoomFactor) + zoomScopePosition.X;
			mouse.Y = ((mouse.Y - zoomWindowPosition.Y) / zoomFactor) + zoomScopePosition.Y;
		}
	}
	return mouse;
}

Point PowderToy::SnapCoordinatesWall(Point pos1, Point pos2)
{
	if (activeTools[toolIndex]->GetType() == WALL_TOOL)
	{
		pos1 = pos1/4*4;
		if (drawState == PowderToy::RECT)
		{
			if (pos1.X >= pos2.X)
				pos1.X += CELL-1;

			if (pos1.Y >= pos2.Y)
				pos1.Y += CELL-1;
		}
	}
	return pos1;
}

bool PowderToy::IsMouseInZoom(Point mouse)
{
	//adjust coords into the simulation area
	mouse.Clamp(Point(0, 0), Point(XRES-1, YRES-1));

	return mouse != AdjustCoordinates(mouse);
}

void PowderToy::SetInfoTip(std::string infotip)
{
	UpdateToolTip(infotip, Point(XCNTR-gfx::VideoBuffer::TextSize(infotip).X/2, YCNTR-10), INFOTIP, 1000);
}

ToolTip * PowderToy::GetQTip(std::string qtip, int y)
{
	return new ToolTip(qtip, Point(XRES-5-gfx::VideoBuffer::TextSize(qtip).X, y), QTIP, -2);
}

void PowderToy::UpdateZoomCoordinates(Point mouse)
{
	zoomMousePosition = mouse;
	zoomScopePosition = mouse-Point(zoomScopeSize/2, zoomScopeSize/2);
	zoomScopePosition.Clamp(Point(0, 0), Point(XRES-zoomScopeSize, YRES-zoomScopeSize));

	if (mouse.X < XRES/2)
		zoomWindowPosition = Point(XRES-zoomScopeSize*zoomFactor, 0);
	else
		zoomWindowPosition = Point(0, 0);
}

void PowderToy::ReloadSave()
{
	if (!reloadSave)
		return;
	SnapshotHistory::TakeSnapshot(sim);
	try
	{
		auto saveLoadData = sim->LoadSave(0, 0, reloadSave, 1);
		authors = saveLoadData.authors;
		if (!authors.size())
			DefaultSaveInfo();
	}
	catch (ParseException & e)
	{
		Engine::Ref().ShowWindow(new InfoPrompt("重新加载存档失败", e.what()));
	}
}

void PowderToy::SetReloadPoint(Save * reloadSave_)
{
	if (reloadSave)
	{
		delete reloadSave;
		reloadSave = NULL;
	}
	if (reloadSave_)
		reloadSave = new Save(*reloadSave_);
}

void PowderToy::UpdateStampCoordinates(Point cursor, Point offset)
{
	loadPos.X = cursor.X;
	loadPos.Y = cursor.Y;
	loadPos -= offset;
	// Stamps can now be pasted out of bounds
	// Instead of clamping stamp borders in-bounds, clamp them to sim area instead (cursor is already guaranteed to be in the sim area, though)
	loadPos.Clamp(Point(0, 0), Point(XRES - 1, YRES - 1));
	//loadPos.Clamp(loadSize/2, Point(XRES, YRES)-loadSize/2);
}

void PowderToy::ResetStampState()
{
	if (state == LOAD)
	{
		delete stampData;
		stampData = NULL;
		free(stampImg);
		stampImg = NULL;
#ifdef TOUCHUI
		stampMoving = false;
#endif
	}
	state = NONE;
	isStampMouseDown = false;
	isMouseDown = false; // do this here also because we always want to cancel mouse drawing when going into a new stamp state
	stampOffset = Point(0, 0);
}

void PowderToy::TranslateSave(Point point)
{
	try
	{
		if (stampData)
		{
			Matrix::vector2d translate = Matrix::v2d_new(point.X, point.Y);
			Matrix::vector2d translated = stampData->Translate(translate);
			stampOffset += Point(translated.x, translated.y);
	
			free(stampImg);
			stampImg = prerender_save((char*)stampData->GetSaveData(), stampData->GetSaveSize(), &loadSize.X, &loadSize.Y);
		}
	}
	catch (BuildException & e)
	{
		ResetStampState();
		SetInfoTip("转换图章版本时发生异常：" + std::string(e.what()));
	}
}

void PowderToy::TransformSave(int a, int b, int c, int d)
{
	try
	{
		if (stampData)
		{
			Matrix::matrix2d transform = Matrix::m2d_new(a, b, c, d);
			Matrix::vector2d translate = Matrix::v2d_zero;
			stampData->Transform(transform, translate);
	
			free(stampImg);
			stampImg = prerender_save((char*)stampData->GetSaveData(), stampData->GetSaveSize(), &loadSize.X, &loadSize.Y);
		}
	}
	catch (BuildException & e)
	{
		ResetStampState();
		SetInfoTip("变换图章时发生异常：" + std::string(e.what()));
	}
}

void PowderToy::HideZoomWindow()
{
	placingZoom = false;
	placingZoomTouch = false;
	zoomEnabled = false;
}

Button * PowderToy::AddNotification(std::string message, std::function<void(int)> callback)
{
	int messageSize = gfx::VideoBuffer::TextSize(message).X;
	Button *notificationButton = new Button(Point(XRES - 26 - messageSize - 5, YRES - 22 - 20 * numNotifications),
	        Point(messageSize + 5, 15), message);
	notificationButton->SetColor(COLRGB(255, 216, 32));
	Button *discardButton = new Button(Point(XRES - 24, YRES - 22 - 20 * numNotifications), Point(15, 15), "\xAA");
	discardButton->SetColor(COLRGB(255, 216, 32));

	notificationButton->SetCallback([this, notificationButton, discardButton, callback](int mb) {
		this->RemoveComponent(notificationButton);
		this->RemoveComponent(discardButton);
		callback(mb);
	});
	discardButton->SetCallback([this, notificationButton, discardButton](int mb) {
		this->RemoveComponent(notificationButton);
		this->RemoveComponent(discardButton);
	});

	AddComponent(notificationButton);
	AddComponent(discardButton);
	numNotifications++;
	return notificationButton;
}

std::string PowderToy::GetMotd()
{
	if (starcatcherMotd.empty() && vanillaMotd.empty())
		return "链接：\bt{a:https://powdertoy.co.uk|Powder Toy 主页}\bg，\bt{a:https://powdertoy.co.uk/Discussions/Categories/Index.html|论坛}\bg，\bt{a:https://github.com/The-Powder-Toy/The-Powder-Toy|TPT 官方 GitHub}\bg，\bt{a:https://github.com/jacob1/The-Powder-Toy/tree/c++|Jacob1 模组 GitHub}";

	if (starcatcherMotd.empty())
		return vanillaMotd;
	else if (vanillaMotd.empty())
		return starcatcherMotd;

	return motdToggle++ % 2 == 0 ? vanillaMotd : starcatcherMotd;
}

void PowderToy::LoadRenderPreset(int preset)
{
	if (Renderer::Ref().LoadRenderPreset(preset))
	{
		std::string tooltip = Renderer::Ref().GetRenderPresetToolTip(preset);
		UpdateToolTip(tooltip, Point(XCNTR-gfx::VideoBuffer::TextSize(tooltip).X/2, YCNTR-10), INFOTIP, 255);
		save_presets();
	}
}

// Engine events
void PowderToy::OnTick(uint32_t ticks)
{
	int mouseX, mouseY;
#ifdef LUACONSOLE

	/*std::stringstream orStr;
	orStr << orientation[0] << ", " << orientation[1] << ", " << orientation[2] << std::endl;
	luacon_log(mystrdup(orStr.str().c_str()));*/
#endif
	//sdl_key = heldKey; // ui_edit_process in deco editor uses this global so we have to set it ):
	int mouseDown = mouse_get_state(&mouseX, &mouseY);
	main_loop_temp(mouseClickCanceled ? 0 : mouseDown, lastMouseDown, heldKey, heldScan, mouseX, mouseY, shiftHeld, ctrlHeld, altHeld);
	lastMouseDown = mouseDown;
	mouseClickCanceled = false;
	heldKey = 0;
	heldScan = 0;

	if (!loginFinished)
		loginCheckTicks = (loginCheckTicks+1)%51;
	waitToDraw = false;

	if (skipDraw)
		skipDraw = false;
	else if (isMouseDown)
	{
		if (drawState == POINTS)
		{
			activeTools[toolIndex]->DrawLine(sim, currentBrush, lastDrawPoint, cursor, true, toolStrength);
			lastDrawPoint = cursor;
		}
		else if (drawState == LINE)
		{
			Point drawPoint2 = cursor;
			if (altHeld)
				drawPoint2 = LineSnapCoords(initialDrawPoint, cursor);
			if (((ToolTool*)activeTools[toolIndex])->GetID() == TOOL_WIND)
			{
				activeTools[toolIndex]->DrawLine(sim, currentBrush, initialDrawPoint, drawPoint2, false, toolStrength);
			}
			activeTools[toolIndex]->Drag(sim, currentBrush, initialDrawPoint, cursor);
		}
		else if (drawState == FILL)
		{
			activeTools[toolIndex]->FloodFill(sim, currentBrush, cursor);
		}
	}

	if (versionCheck && versionCheck->CheckDone())
	{
		int status = 200;
		std::string result = versionCheck->Finish(&status);
		if (status == 200 && !ParseServerReturn(result, status, true))
		{
			std::istringstream datastream(result);
			Json::Value root;

			try
			{
				datastream >> root;

				starcatcherMotd = root["MessageOfTheDay"].asString();

				Json::Value updates = root["Updates"];
				Json::Value stable = updates["Stable"];
				int major = stable["Major"].asInt();
				int minor = stable["Minor"].asInt();
				int buildnum = stable["Build"].asInt();
				std::string changelog = stable["Changelog"].asString();
#ifdef ANDROID
				std::string file = stable["File"].asString();
				if (buildnum > MOBILE_BUILD)
				{
					std::stringstream changelogStream;
	changelogStream << "\bb当前版本：" << MOBILE_MAJOR << "." << MOBILE_MINOR << "（" << MOBILE_BUILD << "）\n新版本：" << major << "." << minor << "（" << buildnum << "）\n\n\bw更新日志：\n";
#else
				std::string file = UPDATESCHEME UPDATESERVER + stable["File"].asString();
				if (buildnum > MOD_BUILD_VERSION)
				{
					std::stringstream changelogStream;
	changelogStream << "\bb当前版本：" << MOD_VERSION << "." << MOD_MINOR_VERSION << "（" << MOD_BUILD_VERSION << "）\n新版本：" << major << "." << minor << "（" << buildnum << "）\n\n\bw更新日志：\n";
#endif
					changelogStream << changelog;
					std::string changelogText = changelogStream.str();

					AddNotification("有新版本可用，点击查看！", [this, changelogText, file](int mb) {
						if (mb == 1)
							this->ConfirmUpdate(changelogText, file);
					});
				}


				Json::Value notifications = root["Notifications"];
				for (int i = 0; i < (int)notifications.size(); i++)
				{
					std::string message = notifications[i]["Text"].asString();
					std::string link = notifications[i]["Link"].asString();

					AddNotification(message, [link](int mb) {
						if (mb == 1)
							Platform::OpenLink(link);
					});
				}
			}
			catch (std::exception &e)
			{
				SetInfoTip("错误：更新服务器返回了无效数据");
				UpdateToolTip("", Point(16, 20), INTROTIP, 0);
			}
		}
		versionCheck = NULL;
	}
	if (sessionCheck && sessionCheck->CheckDone())
	{
		int status = 200;
		std::string result = sessionCheck->Finish(&status);
		// ignore timeout errors or others, since the user didn't actually click anything
		if (status != 200 || ParseServerReturn(result, status, true))
		{
			// key icon changes to red
			loginFinished = -1;
		}
		else
		{
			std::istringstream datastream(result);
			Json::Value root;

			try
			{
				datastream >> root;

				loginFinished = 1;
				if (!root["Session"].asInt())
				{
					if (svf_login)
						loginFinished = -1;
					// TODO: better login system, why do we reset all these
					strcpy(svf_user, "");
					strcpy(svf_user_id, "");
					strcpy(svf_session_id, "");
					svf_login = 0;
					svf_own = 0;
					svf_admin = 0;
					svf_mod = 0;
				}

				vanillaMotd = root["MessageOfTheDay"].asString();

				Json::Value notifications = root["Notifications"];
				for (int i = 0; i < (int)notifications.size(); i++)
				{
					std::string message = notifications[i]["Text"].asString();
					std::string link = notifications[i]["Link"].asString();

					AddNotification(message, [link](int mb) {
						if (mb == 1)
							Platform::OpenLink(link);
					});
				}
			}
			catch (std::exception &e)
			{
				// this shouldn't happen because the server hopefully won't return bad data ...
				loginFinished = -1;
			}
		}
		sessionCheck = NULL;
	}
	// delay these a frame, due to the way startup initializatin is ordered
	// http hasn't been initialized when the PowderToy object is created
	if (delayedHttpChecks)
	{
		DelayedHttpInitialization();
		delayedHttpChecks = false;
	}
	if (voteDownload && voteDownload->CheckDone())
	{
		int status;
		std::string result = voteDownload->Finish(&status);
		if (ParseServerReturn(result, status, false))
			svf_myvote = 0;
		else if (svf_myvote == 0)
			SetInfoTip("已清除投票");
		else
			SetInfoTip("投票成功");
		voteDownload = NULL;
	}

	if (openConsole)
	{
		OpenConsole();
		openConsole = false;
	}
	if (openSign)
	{
		// if currently moving a sign, stop doing so
		if (MSIGN != -1)
			MSIGN = -1;
		else
		{
			Point cursor = AdjustCoordinates(Point(mouseX, mouseY));
			int signID = InsideSign(sim, cursor.X, cursor.Y, true);
			if (signID == -1 && signs.size() >= MAXSIGNS)
				SetInfoTip("已达到标牌数量上限");
			else
				Engine::Ref().ShowWindow(new CreateSign(signID, cursor));
		}
		openSign = false;
	}
	if (openProp)
	{
		Engine::Ref().ShowWindow(new PropWindow());
		openProp = false;
	}
	if (showLargeScreenDialog)
	{
		int scale = Engine::Ref().GetScale();
		std::stringstream message;
		message << "检测到屏幕尺寸足够大，将切换到 " << scale << " 倍界面：";
		message << screenWidth << "x" << screenHeight << "（检测到），需要 " << VIDXRES * scale << "x" << VIDYRES * scale << "（所需）";
		message << "\n 要撤消此操作，请点击'取消'。您可以随时在设置中更改此设置。";

		auto prompt = new ConfirmPrompt("检测到大屏幕", message.str());
		prompt->SetCallback({ [](bool confirmed) {
			if (!confirmed)
				Engine::Ref().SetScale(1);
		} });
		Engine::Ref().ShowWindow(prompt);

		showLargeScreenDialog = false;
	}

	// a ton of stuff with the buttons on the bottom row has to be updated
	// later, this will only be done when an event happens
	reloadButton->SetEnabled(reloadSave != NULL ? true : false);
#ifdef TOUCHUI
	openBrowserButton->SetState(ctrlHeld ? Button::INVERTED : Button::HOLD);
	saveButton->SetState((svf_login && ctrlHeld) ? Button::INVERTED : Button::HOLD);
#else
	openBrowserButton->SetState(ctrlHeld ? Button::INVERTED : Button::NORMAL);
	saveButton->SetState((svf_login && ctrlHeld) ? Button::INVERTED : Button::NORMAL);
#endif
	std::string saveButtonText = "\x82 ";
	std::string saveButtonTip;
	bool canReupload = true;
	if (!strcmp(svf_id, "404") || !strcmp(svf_id, "2157797"))
		canReupload = false;
	if (!svf_login || ctrlHeld || !canReupload)
	{
		// button text
		if (svf_fileopen)
			saveButtonText += svf_filename;
		else if (svf_open)
			saveButtonText += svf_name;
		else
			saveButtonText += "[保存到本机]";

		// button tooltip
		if (svf_fileopen && mouse.X <= saveButton->GetPosition().X+18)
			saveButtonTip = "覆盖当前打开的本地模拟存档。";
		else
		{
			if (!svf_login)
				saveButtonTip = "将模拟保存到本地磁盘；登录后可保存到服务器。";
			else
				saveButtonTip = "将模拟保存到本机";
		}
	}
	else
	{
		// button text
		if (svf_open)
			saveButtonText += svf_name;
		else
			saveButtonText += "[未命名模拟]";

		// button tooltip
		if (svf_open && svf_own)
		{
			if (mouse.X <= saveButton->GetPosition().X+18)
				saveButtonTip = "重新上传当前模拟";
			else
				saveButtonTip = "修改模拟属性";
		}
		else
			saveButtonTip = "上传新模拟";
	}
	saveButton->SetText(saveButtonText);
	saveButton->SetTooltipText(saveButtonTip);
	saveButton->SetEnabled(canReupload);

	bool votesAllowed = svf_login && svf_open && svf_own == 0;
	upvoteButton->SetEnabled(votesAllowed && voteDownload == NULL);
	downvoteButton->SetEnabled(votesAllowed && voteDownload == NULL);
	int alphaLevel = votesAllowed ? ui::Style::HighlightAlphaHover : ui::Style::HighlightAlpha;
	upvoteButton->SetBackgroundColor(svf_myvote == 1 ? COLMODALPHA(upvoteButton->GetColor(), alphaLevel) : 0);
	downvoteButton->SetBackgroundColor(svf_myvote == -1 ? COLMODALPHA(downvoteButton->GetColor(), alphaLevel) : 0);
	upvoteButton->SetTooltipText("赞这个存档");
	downvoteButton->SetTooltipText("踩这个存档");

#ifndef TOUCHUI
	if (svf_tags[0])
		openTagsButton->SetText("\x83 " + std::string(svf_tags));
	else
		openTagsButton->SetText("\x83 [未设置标签]");
	openTagsButton->SetEnabled(svf_open);
	if (svf_own)
		openTagsButton->SetTooltipText("添加或移除模拟标签");
	else
		openTagsButton->SetTooltipText("添加模拟标签");
#endif

	// set login button text, key turns green or red depending on whether session check succeeded
	std::string loginButtonText;
	std::string loginButtonTip;
	if (svf_login)
	{
		if (loginFinished == 1)
		{
			loginButtonText = "\x0F\x01\xFF\x01\x84\x0E " + std::string(svf_user);
			if (mouse.X <= loginButton->GetPosition().X+18)
				loginButtonTip = "查看并编辑个人资料";
			else if (svf_mod && mouse.X >= loginButton->Right(Point(-15, 0)).X)
				loginButtonTip = "你是版主";
			else if (svf_admin && mouse.X >= loginButton->Right(Point(-15, 0)).X)
		loginButtonTip = "天佑所为";
			else
				loginButtonTip = "使用其他名称登录模拟服务器";
		}
		else if (loginFinished == -1)
		{
			loginButtonText = "\x0F\xFF\x01\x01\x84\x0E " + std::string(svf_user);
			loginButtonTip = "无法验证登录状态";
		}
		else
		{
			loginButtonText = "\x84 " + std::string(svf_user);
			loginButtonTip = "正在等待登录服务器...";
		}
	}
	else
	{
		if (loginFinished == -1)
			loginButtonText = "\x0F\xFF\x01\x01\x84\x0E [登录]";
		else
			loginButtonText = "\x84 [登录]";
		loginButtonTip = "登录模拟服务器";
	}
	loginButton->SetText(loginButtonText);
	loginButton->SetTooltipText(loginButtonTip);

	pauseButton->SetState(sys_pause ? Button::INVERTED : Button::NORMAL);
	if (sys_pause)
		pauseButton->SetTooltipText("继续模拟 \bg(空格)");
	else
		pauseButton->SetTooltipText("暂停模拟 \bg(空格)");

	if (placingZoomTouch)
		UpdateToolTip("\x0F\xEF\xEF\020轻触任意位置放置缩放窗口（音量键调整大小，点击缩放按钮取消）", Point(16, YRES-24), TOOLTIP, 255);
#ifdef TOUCHUI
	if (state == SAVE || state == COPY)
		UpdateToolTip("\x0F\xEF\xEF\020拖动选取要复制的矩形区域（点击保存按钮取消）", Point(16, YRES-24), TOOLTIP, 255);
	else if (state == CUT)
		UpdateToolTip("\x0F\xEF\xEF\020拖动选取要复制并剪切的矩形区域（点击保存按钮取消）", Point(16, YRES-24), TOOLTIP, 255);
	else if (state == LOAD)
		UpdateToolTip("\x0F\xEF\xEF\020拖动图章可移动，轻触图章可放置；在图章外轻触或拖动可平移和旋转。", Point(16, YRES-24), TOOLTIP, 255);
#else
	if (state == SAVE || state == COPY)
		UpdateToolTip("\x0F\xEF\xEF\020拖动选取要复制的矩形区域（右键取消）", Point(16, YRES-24), TOOLTIP, 255);
	else if (state == CUT)
		UpdateToolTip("\x0F\xEF\xEF\020拖动选取要复制并剪切的矩形区域（右键取消）", Point(16, YRES-24), TOOLTIP, 255);
#endif
	if (insideRenderOptions && this->Subwindows.size() == 0)
	{
		insideRenderOptions = false;
		deletingRenderOptions = false;
		if (restorePreviousPause)
		{
			sys_pause = previousPause;
			restorePreviousPause = false;
		}
		save_presets();
	}
	VideoBufferHack();
}

void PowderToy::OnDraw(gfx::VideoBuffer *buf)
{
#ifdef LUACONSOLE
	luacon_step(mouse.X, mouse.Y);
	ConfirmRunEmbeddedLuaCode();
#endif
	if (insideRenderOptions)
		return;
	ARGBColour dotColor = 0;
	if (svf_fileopen && svf_login && ctrlHeld)
		dotColor = COLPACK(0x000000);
	else if ((!svf_login && svf_fileopen) || (svf_open && svf_own && !ctrlHeld))
		dotColor = COLPACK(0xFFFFFF);
	if (dotColor)
	{
		for (int i = 1; i <= saveButton->GetSize().Y-1; i+= 2)
			buf->DrawPixel(saveButton->GetPosition().X+18, saveButton->GetPosition().Y+i, COLR(dotColor), COLG(dotColor), COLB(dotColor), 255);
	}

	if (svf_login)
	{
		for (int i = 1; i <= saveButton->GetSize().Y-1; i+= 2)
			buf->DrawPixel(loginButton->GetPosition().X+18, loginButton->GetPosition().Y+i, 255, 255, 255, 255);

		// login check hasn't finished, key icon is dynamic
		if (loginFinished == 0)
			buf->FillRect(loginButton->GetPosition().X+2+loginCheckTicks/3, loginButton->GetPosition().Y+1, 16-loginCheckTicks/3, 13, 0, 0, 0, 255);

		if (svf_admin)
		{
			Point iconPos = loginButton->Right(Point(-12, (saveButton->GetSize().Y-12)/2+2));
			buf->DrawString(iconPos.X, iconPos.Y, "\xC9", 232, 127, 35, 255);
			buf->DrawString(iconPos.X, iconPos.Y, "\xC7", 255, 255, 255, 255);
			buf->DrawString(iconPos.X, iconPos.Y, "\xC8", 255, 255, 255, 255);
		}
		else if (svf_mod)
		{
			Point iconPos = loginButton->Right(Point(-12, (saveButton->GetSize().Y-12)/2+2));
			buf->DrawString(iconPos.X, iconPos.Y, "\xC9", 35, 127, 232, 255);
			buf->DrawString(iconPos.X, iconPos.Y, "\xC7", 255, 255, 255, 255);
		}
		// amd logo
		/*else if (true)
		{
			Point iconPos = loginButton->Right(Point(-12, (saveButton->GetSize().Y-10)/2));
			buf->DrawString(iconPos.X, iconPos.Y, "\x97", 0, 230, 153, 255);
		}*/
	}
	Renderer::Ref().RecordingTick();

	reset_clip_rect();
	buf->ResetClipRect();
}

bool PowderToy::BeforeMouseMove(int x, int y, Point difference)
{
	MouseMoveEvent ev = MouseMoveEvent(x, y, difference.X, difference.Y);
	return HandleEvent(LuaEvents::mousemove, &ev);
}

void PowderToy::OnMouseMove(int x, int y, Point difference)
{
	mouse = Point(x, y);
	cursor = AdjustCoordinates(mouse);
	bool tmpMouseInZoom = IsMouseInZoom(mouse);
	if (placingZoom)
		UpdateZoomCoordinates(mouse);
	if (state == LOAD)
	{
#ifdef TOUCHUI
		if (stampMoving)
			UpdateStampCoordinates(cursor, stampClickedOffset);
#else
		UpdateStampCoordinates(cursor);
#endif
	}
	else if (state == SAVE || state == COPY || state == CUT)
	{
		if (isStampMouseDown)
		{
			saveSize.X = cursor.X + 1 - savePos.X;
			saveSize.Y = cursor.Y + 1 - savePos.Y;
			if (savePos.X + saveSize.X < 0)
				saveSize.X = 0;
			else if (savePos.X + saveSize.X > XRES)
				saveSize.X = XRES - savePos.X;
			if (savePos.Y + saveSize.Y < 0)
				saveSize.Y = 0;
			else if (savePos.Y + saveSize.Y > YRES)
				saveSize.Y = YRES - savePos.Y;
		}
	}
	else if (isMouseDown)
	{
		if (mouseInZoom == tmpMouseInZoom)
		{
			if (drawState == POINTS)
			{
				activeTools[toolIndex]->DrawLine(sim, currentBrush, lastDrawPoint, cursor, true, toolStrength);
				lastDrawPoint = cursor;
				skipDraw = true;
			}
			else if (drawState == FILL)
			{
				activeTools[toolIndex]->FloodFill(sim, currentBrush, cursor);
				skipDraw = true;
			}
		}
		else if (drawState == POINTS || drawState == FILL)
		{
			isMouseDown = false;
			drawState = POINTS;
			// special lua mouseevent for moving in / out of zoom window
			MouseUpEvent ev = MouseUpEvent(x, y, 0, mouseUpDrawEnd);
			HandleEvent(LuaEvents::mouseup, &ev);
		}
		mouseInZoom = tmpMouseInZoom;
	}

	// moving sign, update coordinates here
	if (MSIGN >= 0 && MSIGN < (int)signs.size())
	{
		signs[MSIGN].SetPos(cursor);
	}
}

bool PowderToy::BeforeMouseDown(int x, int y, unsigned char button)
{
	MouseDownEvent ev = MouseDownEvent(x, y, button);
	bool ret = HandleEvent(LuaEvents::mousedown, &ev);
	if (!ret)
		mouseClickCanceled = true;
	return ret;
}

void PowderToy::OnMouseDown(int x, int y, unsigned char button)
{
	mouse = Point(x, y);
	cursor = AdjustCoordinates(mouse);
	mouseInZoom = IsMouseInZoom(mouse);
	deletingRenderOptions = false;
	if (deco_disablestuff)
	{

	}
	else if (placingZoomTouch)
	{
		if (x < XRES && y < YRES)
		{
			placingZoomTouch = false;
			placingZoom = true;
			UpdateZoomCoordinates(mouse);
		}
	}
	else if (placingZoom)
	{

	}
	else if (state == LOAD)
	{
		isStampMouseDown = true;
#ifdef TOUCHUI
		stampClickedPos = cursor;
		initialLoadPos = loadPos;
		UpdateStampCoordinates(cursor);
		stampClickedOffset = loadPos-initialLoadPos;
		loadPos -= stampClickedOffset;
		if (cursor.IsInside(GetStampPos(), GetStampPos() + GetStampSize()))
			stampMoving = true;
		// calculate which side of the stamp this touch is on
		else
		{
			int xOffset = (loadSize.X - loadSize.Y)/2;
			Point diff = cursor - GetStampCenterPos() / CELL * CELL;
			if (std::abs(diff.X) - xOffset > std::abs(diff.Y))
				stampQuadrant = (diff.X > 0) ? 3 : 1; // right : left
			else
				stampQuadrant = (diff.Y > 0) ? 2 : 0; // down : up
		}
#endif
	}
	else if (state == SAVE || state == COPY || state == CUT)
	{
		// right click cancel
		if (button == 3)
		{
			ResetStampState();
		}
		// placing initial coordinate
		else if (!isStampMouseDown)
		{
			savePos = cursor;
			saveSize = Point(1, 1);
			isStampMouseDown = true;
		}
	}
	else if (InsideSign(sim, cursor.X, cursor.Y, ctrlHeld) != -1 || MSIGN != -1)
	{
		// do nothing
	}
	else if (insideRenderOptions)
	{
		deletingRenderOptions = true;
	}
	else if (sim->InBounds(mouse.X, mouse.Y))
	{
		toolIndex = (button == 1 || button == 2) ? 0 : 1;
		UpdateDrawMode();
		// this was in old drawing code, still needed?
		//if (activeTools[0]->GetType() == DECO_TOOL && button == 4)
		//	activeTools[1] = GetToolFromIdentifier("DEFAULT_DECOR_CLR");
		if (button == 2 || (altHeld && !shiftHeld && !ctrlHeld))
		{
			Tool *tool = activeTools[toolIndex]->Sample(sim, cursor, shiftHeld);
			if (tool)
				activeTools[toolIndex] = tool;
			return;
		}

		isMouseDown = true;
		if (drawState == LINE || drawState == RECT)
		{
			initialDrawPoint = cursor;
			// WIND lines are constantly being drawn as mouse is down, so we want to take a snapshot at the start
			if (((ToolTool*)activeTools[toolIndex])->GetID() == TOOL_WIND)
				SnapshotHistory::TakeSnapshot(sim);
		}
		else if (drawState == POINTS)
		{
			SnapshotHistory::TakeSnapshot(sim);
			lastDrawPoint = cursor;
			activeTools[toolIndex]->DrawPoint(sim, currentBrush, cursor, toolStrength);
		}
		else if (drawState == FILL)
		{
			SnapshotHistory::TakeSnapshot(sim);
			activeTools[toolIndex]->FloodFill(sim, currentBrush, cursor);
		}
	}
}

bool PowderToy::BeforeMouseUp(int x, int y, unsigned char button)
{
	// lua mouse event, cancel mouse action if the function returns false
	MouseUpEvent ev = MouseUpEvent(x, y, button, mouseUpNormal);
	return HandleEvent(LuaEvents::mouseup, &ev);
}

void PowderToy::OnMouseUp(int x, int y, unsigned char button)
{
	mouse = Point(x, y);
	cursor = AdjustCoordinates(mouse);

	if (placingZoom)
	{
		placingZoom = false;
		zoomEnabled = true;
	}
	else if (ignoreMouseUp)
	{
		// ignore mouse up when some touch ui buttons on the right side are pressed
		ignoreMouseUp = false;
	}
	else if (state == LOAD)
	{
		UpdateDrawMode(); // LOAD branch always returns early, so run this here
		if (button == 3 || y >= YRES+MENUSIZE-16)
		{
			ResetStampState();
			return;
		}
		// never had a mouse down event while in LOAD state, return
		if (!isStampMouseDown)
			return;
		isStampMouseDown = false;
#ifdef TOUCHUI
		if (loadPos != initialLoadPos)
		{
			stampMoving = false;
			return;
		}
		else if (cursor.IsInside(Point(0, 0), Point(XRES, YRES)) && !cursor.IsInside(GetStampPos(), GetStampPos() + GetStampSize()))
		{
			// figure out which side this touch started and ended on (arbitrary direction numbers)
			// imagine 4 quadrants coming out of the stamp, with the edges diagonally starting directly from each corner
			// if you tap the screen in one of these corners, it shifts the stamp one pixel in that direction
			// if you move between quadrants you can rotate in that direction
			// or flip horizontally / vertically by dragging accross the stamp without touching the side quadrants
			int quadrant, xOffset = (loadSize.X - loadSize.Y) / 2;
			Point diff = cursor - GetStampCenterPos() / CELL * CELL;
			if (std::abs(diff.X)-xOffset > std::abs(diff.Y))
				quadrant = (diff.X > 0) ? 3 : 1; // right : left
			else
				quadrant = (diff.Y > 0) ? 2 : 0; // down : up

			// shift (arrow keys)
			if (quadrant == stampQuadrant)
				TranslateSave(Point((quadrant-2)%2, (quadrant-1)%2));
			// rotate 90 degrees
			else if (quadrant%2 != stampQuadrant%2)
				TransformSave(0, (quadrant-stampQuadrant+1)%4 == 0 ? -1 : 1, (quadrant-stampQuadrant+1)%4 == 0 ? 1 : -1, 0);
			// flip 180
			else
				TransformSave((quadrant%2)*-2+1, 0, 0, (quadrant%2)*2-1);
			return;
		}
		else if (!stampMoving)
			return;
		stampMoving = false;
#endif
		SnapshotHistory::TakeSnapshot(sim);

		try
		{
			Point realLoadPos = GetStampPos();
			auto saveLoadData = sim->LoadSave(realLoadPos.X, realLoadPos.Y, stampData, 0, !shiftHeld);
#ifdef LUACONSOLE
			if (saveLoadData.isMissingElements())
				luacon_log("粘贴内容缺少自定义元素");
#endif
			MergeStampAuthorInfo(saveLoadData.authors);
		}
		catch (ParseException & e)
		{
			Engine::Ref().ShowWindow(new InfoPrompt("加载存档失败", e.what()));
		}

		ResetStampState();
		return;
	}
	else if (state == SAVE || state == COPY || state == CUT)
	{
		UpdateDrawMode(); // SAVE/COPY/CUT branch always returns early, so run this here
		// already placed initial coordinate. If they haven't ... no idea what happened here
		// mouse could be 4 if strange stuff with zoom window happened so do nothing and reset state in that case too
		if (button != 1)
		{
			ResetStampState();
			return;
		}
		if (!isStampMouseDown)
			return;

		// make sure size isn't negative
		if (saveSize.X < 0)
		{
			savePos.X = savePos.X + saveSize.X - 1;
			saveSize.X = abs(saveSize.X) + 2;
		}
		if (saveSize.Y < 0)
		{
			savePos.Y = savePos.Y + saveSize.Y - 1;
			saveSize.Y = abs(saveSize.Y) + 2;
		}
		if (saveSize.X > 0 && saveSize.Y > 0)
		{
			switch (state)
			{
			case COPY:
			{
				Json::Value clipboardInfo;
				clipboardInfo["type"] = "clipboard";
				clipboardInfo["username"] = svf_user;
				clipboardInfo["date"] = (Json::Value::UInt64)time(NULL);
				SaveAuthorInfo(&clipboardInfo);

				delete clipboardData;
				clipboardData = NULL;
				clipboardData = sim->CreateSave(savePos.X, savePos.Y, saveSize.X+savePos.X, saveSize.Y+savePos.Y, !shiftHeld);
				clipboardData->authors = clipboardInfo;
				try
				{
					clipboardData->BuildSave();
				}
				catch (BuildException & e)
				{
					delete clipboardData;
					clipboardData = NULL;
					Engine::Ref().ShowWindow(new ErrorPrompt("生成存档失败：" + std::string(e.what())));
				}
				break;
			}
			case CUT:
			{
				Json::Value clipboardInfo;
				clipboardInfo["type"] = "clipboard";
				clipboardInfo["username"] = svf_user;
				clipboardInfo["date"] = (Json::Value::UInt64)time(NULL);
				SaveAuthorInfo(&clipboardInfo);

				delete clipboardData;
				clipboardData = NULL;
				clipboardData = sim->CreateSave(savePos.X, savePos.Y, saveSize.X+savePos.X, saveSize.Y+savePos.Y, !shiftHeld);
				clipboardData->authors = clipboardInfo;
				try
				{
					clipboardData->BuildSave();
				}
				catch (BuildException & e)
				{
					delete clipboardData;
					clipboardData = NULL;
					Engine::Ref().ShowWindow(new ErrorPrompt("生成存档失败：" + std::string(e.what())));
					break;
				}
				SnapshotHistory::TakeSnapshot(sim);
				sim->ClearArea(savePos.X, savePos.Y, saveSize.X, saveSize.Y);
				break;
			}
			case SAVE:
				Stamps::Ref().Generate(sim, savePos.X, savePos.Y, saveSize.X, saveSize.Y, !shiftHeld);
				break;
			default:
				break;
			}
		}
		ResetStampState();
		return;
	}
	else if (MSIGN != -1)
		MSIGN = -1;
	else if (insideRenderOptions)
	{
		if (this->Subwindows.size() && deletingRenderOptions)
			this->Subwindows[0]->Close(ui::MouseOutside);
	}
	else if (isMouseDown)
	{
		if (drawState == POINTS)
		{
			activeTools[toolIndex]->DrawLine(sim, currentBrush, lastDrawPoint, cursor, true, toolStrength);
			activeTools[toolIndex]->Click(sim, currentBrush, cursor);
		}
		else if (drawState == LINE)
		{
			if (altHeld)
				cursor = LineSnapCoords(initialDrawPoint, cursor);
			// WIND lines are constantly being drawn as mouse is down, so we don't need to take a snapshot on mouseup
			if (((ToolTool*)activeTools[toolIndex])->GetID() != TOOL_WIND)
				SnapshotHistory::TakeSnapshot(sim);
			activeTools[toolIndex]->DrawLine(sim, currentBrush, initialDrawPoint, cursor, false, 1.0f);
		}
		else if (drawState == RECT)
		{
			if (altHeld)
				cursor = RectSnapCoords(initialDrawPoint, cursor);
			SnapshotHistory::TakeSnapshot(sim);
			activeTools[toolIndex]->DrawRect(sim, currentBrush, initialDrawPoint, cursor, toolStrength);
		}
		else if (drawState == FILL)
		{
			activeTools[toolIndex]->FloodFill(sim, currentBrush, cursor);
		}
		isMouseDown = false;
	}
	else
	{
		// ctrl+click moves a sign
		if (ctrlHeld)
		{
			int signID = InsideSign(sim, cursor.X, cursor.Y, true);
			if (signID != -1)
				MSIGN = signID;
		}
		// link signs are clicked from here
		else
		{
			toolIndex = (button == 1 || button == 2) ? 0 : 1;
			bool signTool = ((ToolTool*)activeTools[toolIndex])->GetID() == TOOL_SIGN;
			int signID = InsideSign(sim, cursor.X, cursor.Y, false);
			if (signID != -1)
			{
				// this is a hack so we can edit clickable signs when sign tool is selected (normal signs are handled in activeTool->Click())
				if (signTool)
					openSign = true;
				else
				{
					switch (signs[signID].GetType())
					{
					case Sign::Spark:
					{
						Point realPos = signs[signID].GetRealPos();
						if (pmap[realPos.Y][realPos.X])
							sim->spark_all_attempt(ID(pmap[realPos.Y][realPos.X]), realPos.X, realPos.Y);
						break;
					}
					case Sign::SaveLink:
						open_ui(vid_buf, (char*)signs[signID].GetLinkText().c_str(), 0, 0);
						break;
					case Sign::ThreadLink:
						Platform::OpenLink(SCHEME "powdertoy.co.uk/Discussions/Thread/View.html?Thread=" + signs[signID].GetLinkText());
						break;
					case Sign::SearchLink:
						strncpy(search_expr, signs[signID].GetLinkText().c_str(), 255);
						search_own = 0;
						search_ui(vid_buf);
						break;
					default:
						break;
					}
				}
			}
		}
	}
	// update the drawing mode for the next line
	// since ctrl/shift state may have changed since we started drawing
	UpdateDrawMode();
	deletingRenderOptions = false;
}

bool PowderToy::BeforeMouseWheel(int x, int y, int d)
{
	MouseWheelEvent ev = MouseWheelEvent(x, y, d);
	return HandleEvent(LuaEvents::mousewheel, &ev);
}

void PowderToy::OnMouseWheel(int x, int y, int d)
{
	AdjustCursorSize(d > 0 ? 1 : -1, false);
}

bool PowderToy::BeforeKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (!repeat)
	{
		heldKey = key;
		heldScan = scan;
	}

	if (ctrl && !ctrlHeld)
	{
		ctrlHeld = true;
		openBrowserButton->SetTooltipText("从本机打开模拟 \bg(Ctrl+O)");
		UpdateToolStrength();
	}
	if (shift && !shiftHeld)
	{
		shiftHeld = true;
		UpdateToolStrength();
	}
	if (alt && !altHeld)
		altHeld = true;

	// do nothing when deco textboxes are selected
	if (deco_disablestuff)
		return true;

	KeyEvent ev = KeyEvent(key, scan, repeat, shift, ctrl, alt);
	if (!HandleEvent(LuaEvents::keypress, &ev))
	{
		heldKey = 0;
		heldScan = 0;
		return false;
	}
	return true;
}

void PowderToy::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	UpdateToolTip(introText, Point(16, 20), INTROTIP, 255);
	if (deco_disablestuff)
		return;

	// loading a stamp, special handling here
	// if stamp was transformed, key presses get ignored
	if (state == LOAD)
	{
		switch (key)
		{
		case SDLK_LEFT:
			TranslateSave(Point(-1, 0));
			break;
		case SDLK_RIGHT:
			TranslateSave(Point(1, 0));
			break;
		case SDLK_UP:
			TranslateSave(Point(0, -1));
			break;
		case SDLK_DOWN:
			TranslateSave(Point(0, 1));
			break;
		}
		if (scan == SDL_SCANCODE_R && !repeat)
		{
			// vertical invert
			if (ctrlHeld && shiftHeld)
				TransformSave(1, 0, 0, -1);
			// horizontal invert
			else if (shiftHeld)
				TransformSave(-1, 0, 0, 1);
			// rotate counterclockwise 90 degrees
			else
				TransformSave(0, 1, -1, 0);
		}
	}

	if (repeat)
		return;

	// handle normal keypresses
	switch (scan)
	{
	case SDL_SCANCODE_Q:
		if (ctrl && Engine::Ref().IsGlobalQuit())
			break;
	case SDL_SCANCODE_ESCAPE:
	{
		if (this->Subwindows.size() && insideRenderOptions)
		{
			deletingRenderOptions = false;
			this->Subwindows[0]->Close(ui::Escape);
			break;
		}

		auto prompt = new ConfirmPrompt("你即将退出", "您确定要退出游戏吗？", "退出");
		prompt->SetCallback({ [this](bool confirmed) {
			if (confirmed)
			{
				this->ignoreQuits = false;
				this->Close(ui::Escape);
			}
		} });
		Engine::Ref().ShowWindow(prompt);
		break;
	}
	case SDL_SCANCODE_F5:
		if (state != LOAD)
			ReloadSave();
		break;
	case SDL_SCANCODE_INSERT:
		REPLACE_MODE = !REPLACE_MODE;
		SPECIFIC_DELETE = false;
		break;
	case SDL_SCANCODE_DELETE:
		SPECIFIC_DELETE = !SPECIFIC_DELETE;
		REPLACE_MODE = false;
		break;
	case SDL_SCANCODE_GRAVE:
		OpenConsole();
		break;
	case SDL_SCANCODE_EQUALS:
		if (ctrl)
		{
			for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
				if (parts[i].type == PT_SPRK)
				{
					if (parts[i].ctype >= 0 && parts[i].ctype < PT_NUM && globalSim->elements[parts[i].ctype].Enabled)
					{
						parts[i].type = parts[i].ctype;
						parts[i].life = parts[i].ctype = 0;
					}
					else
						sim->part_kill(i);
				}
				else if (parts[i].type == PT_WIRE)
				{
					parts[i].ctype = parts[i].tmp = 0;
				}
			sim->elementData[PT_WIFI]->Simulation_Cleared(globalSim);
		}
		else
		{
			sim->air->ClearPresVel();
			for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
				if (Save::PressureInTmp3(parts[i].type))
				{
					parts[i].tmp3 = 0;
				}
		}
		break;
	case SDL_SCANCODE_TAB:
		if (!ctrl)
			currentBrush->SetShape((currentBrush->GetShape()+1) % NUM_DEFAULTBRUSHES);
		break;
	case SDL_SCANCODE_W:
		if (sim->elementCount[PT_STKM2] <= 0 || ctrl)
		{
			++sim->gravityMode; // cycle gravity mode

			std::string toolTip;
			switch (sim->gravityMode)
			{
			default:
				sim->gravityMode = GRAV_VERTICAL;
			case GRAV_VERTICAL:
				toolTip = "重力：垂直";
				break;
			case GRAV_OFF:
				toolTip = "重力：关闭";
				break;
			case GRAV_RADIAL:
				toolTip = "重力：径向";
				break;
			case GRAV_CUSTOM:
				toolTip = "重力：自定义";
				break;
			}
			UpdateToolTip(toolTip, Point(XCNTR - gfx::VideoBuffer::TextSize(toolTip.c_str()).X / 2, YCNTR - 10), INFOTIP, 255);
		}
		break;
	case SDL_SCANCODE_E:
		if (ctrlHeld)
		{
			sim->SetEdgeMode(++sim->edgeMode);

			std::string toolTip;
			switch (sim->edgeMode)
			{
			default:
				sim->edgeMode = EDGE_VOID;
			case EDGE_VOID:
				toolTip = "边缘模式：虚空";
				break;
			case EDGE_SOLID:
				toolTip = "边缘模式：实心";
				break;
			case EDGE_LOOP:
				toolTip = "边缘模式：循环";
				break;
			}
			UpdateToolTip(toolTip, Point(XCNTR - gfx::VideoBuffer::TextSize(toolTip.c_str()).X / 2, YCNTR - 10), INFOTIP, 255);
		}
		else
		{
			element_search_ui(vid_buf, &activeTools[0], &activeTools[1], &activeTools[2]);
		}
		break;
	case SDL_SCANCODE_R:
		if (state != LOAD)
		{
			if (ctrlHeld)
				ReloadSave();
			else if (!shiftHeld)
				static_cast<LIFE_ElementDataContainer&>(*sim->elementData[PT_LIFE]).golGeneration = 0;
		}
		break;
	case SDL_SCANCODE_T:
		show_tabs = !show_tabs;
		break;
	case SDL_SCANCODE_Y:
		if (ctrlHeld)
		{
			SnapshotHistory::HistoryForward(sim);
		}
		else
		{
			++sim->air->airMode;

			std::string toolTip;
			switch (sim->air->airMode)
			{
			default:
				sim->air->airMode = AIR_ON;
			case AIR_ON:
				toolTip = "空气：开启";
				break;
			case AIR_PRESSUREOFF:
				toolTip = "空气：压力关闭";
				break;
			case AIR_VELOCITYOFF:
				toolTip = "空气：速度关闭";
				break;
			case AIR_OFF:
				toolTip = "空气：关闭";
				break;
			case AIR_NOUPDATE:
				toolTip = "空气：停止更新";
				break;
			}
			UpdateToolTip(toolTip, Point(XCNTR - gfx::VideoBuffer::TextSize(toolTip.c_str()).X / 2, YCNTR - 10), INFOTIP, 255);
		}
		break;
	case SDL_SCANCODE_U:
		if (ctrlHeld)
		{
			sim->air->ClearAirH();
		}
		else
		{
			aheat_enable = !aheat_enable;
			if (aheat_enable)
				SetInfoTip("环境热：开");
			else
				SetInfoTip("环境热：关闭");
		}
		break;
	case SDL_SCANCODE_I:
		if (!ctrlHeld)
		{
			for (int nx = 0; nx < XRES/CELL; nx++)
				for (int ny = 0; ny < YRES/CELL; ny++)
				{
					sim->air->pv[ny][nx] = -sim->air->pv[ny][nx];
					sim->air->vx[ny][nx] = -sim->air->vx[ny][nx];
					sim->air->vy[ny][nx] = -sim->air->vy[ny][nx];
				}
		}
		else
		{
			auto prompt = new ConfirmPrompt("安装 Powder Toy", "即将安装 The Powder Toy", "安装");
			prompt->SetCallback({ [](bool wasConfirmed) {
				if (wasConfirmed)
				{
					if (Platform::RegisterExtension())
					{
						InfoPrompt *info = new InfoPrompt("安装成功", "Powder Toy 已安装！");
						Engine::Ref().ShowWindow(info);
					}
					else
					{
						ErrorPrompt *error = new ErrorPrompt("安装失败：可能没有权限，或当前平台不支持安装");
						Engine::Ref().ShowWindow(error);
					}
				}
			} });
			Engine::Ref().ShowWindow(prompt);
		}
		break;
	case SDL_SCANCODE_O:
#ifdef TOUCHUI
		catalogue_ui(vid_buf);
#else
		if  (ctrl)
		{
			catalogue_ui(vid_buf);
		}
		else
		{
			old_menu = !old_menu;
			if (old_menu)
				UpdateToolTip("实验性旧菜单已启用，按 O 关闭",
						Point(XCNTR - gfx::VideoBuffer::TextSize("实验性旧菜单已启用，按 O 关闭").X / 2, YCNTR - 10), INFOTIP, 500);
		}
#endif
		break;
	case SDL_SCANCODE_P:
		if (ctrlHeld)
		{
			openProp = true;
			activeTools[shiftHeld ? 1 : 0] = GetToolFromIdentifier("DEFAULT_UI_PROPERTY");
			break;
		}
	case SDL_SCANCODE_F2:
		if (Renderer::Ref().TakeScreenshot(ctrlHeld, 0).length())
			SetInfoTip("截图已保存");
		else
			SetInfoTip("截图保存失败");
		break;
	case SDL_SCANCODE_LEFTBRACKET:
		AdjustCursorSize(-1, true);
		break;
	case SDL_SCANCODE_RIGHTBRACKET:
		AdjustCursorSize(1, true);
		break;
	case SDL_SCANCODE_A:
		if (ctrlHeld && (svf_mod || svf_admin))
		{
			std::string authorString = authors.toStyledString();
			InfoPrompt *info = new InfoPrompt("保存作者信息", authorString);
			Engine::Ref().ShowWindow(info);
		}
		break;
	case SDL_SCANCODE_S:
	{
		//if stkm2 is out, you must be holding left ctrl, else not be holding ctrl at all
		bool stk2 = sim->elementCount[PT_STKM2] > 0;
		if (stk2 ? Engine::Ref().GetModifiers()&KMOD_LCTRL : !ctrlHeld)
		{
			ResetStampState();
			state = SAVE;
		}
		if ((stk2 && (Engine::Ref().GetModifiers()&KMOD_RCTRL)) || (!stk2 && ctrl))
			tab_save(tab_num);
		break;
	}
	case SDL_SCANCODE_D:
		if (globalSim->elementCount[PT_STKM2] > 0 && !ctrl)
			break;
	case SDL_SCANCODE_F3:
		DEBUG_MODE = !DEBUG_MODE;
		SetCurrentHud();
		break;
	case SDL_SCANCODE_F:
	{
		std::string logmessage = "";
		if (debug_flags & DEBUG_PARTICLE)
		{
			SetPause(1);
			if (alt)
			{
				logmessage = sim->ParticleDebug(0, 0, 0);
			}
			else if (shift)
			{
				logmessage = sim->ParticleDebug(1, cursor.X, cursor.Y);
			}
			else if (ctrl)
			{
				if (!(finding & 0x1))
					finding |= 0x1;
				else
					finding &= ~0x1;
			}
			else
			{
				if  (sim->debug_currentParticle)
					logmessage = sim->ParticleDebug(1, -1, -1);
				else
					framerender = 1;
			}
		}
		else
		{
			if (ctrl)
			{
				if (!(finding & 0x1))
					finding |= 0x1;
				else
					finding &= ~0x1;
			}
			else
			{
				SetPause(1);
				if  (globalSim->debug_currentParticle)
					logmessage = sim->ParticleDebug(1, -1, -1);
				else
					framerender = 1;
			}
		}
#ifdef LUACONSOLE
		if (logmessage.size())
			luacon_log(logmessage);
#endif
		break;
	}
	case SDL_SCANCODE_G:
		if (ctrl)
		{
			drawgrav_enable =! drawgrav_enable;
		}
		else
		{
			if (shift)
				GRID_MODE = (GRID_MODE + 9) % 10;
			else
				GRID_MODE = (GRID_MODE + 1) % 10;
		}
		break;
	case SDL_SCANCODE_H:
		if (!ctrl)
		{
			hud_enable = !hud_enable;
			break;
		}
	case SDL_SCANCODE_F1:
		if (!GetToolTipAlpha(INTROTIP))
			UpdateToolTip(introText, Point(16, 20), INTROTIP, 10235);
		else
			UpdateToolTip(introText, Point(16, 20), INTROTIP, 0);
		break;
	case SDL_SCANCODE_K:
	case SDL_SCANCODE_L:
		ResetStampState();
		// open stamp interface
		if (scan == SDL_SCANCODE_K)
		{
			int reorder = 1;
			int stampID = stamp_ui(vid_buf, &reorder);
			if (stampID >= 0)
				stampData = Stamps::Ref().Load(stampID, reorder);
			else
				stampData = nullptr;
		}
		// else, open most recent stamp
		else
			stampData = Stamps::Ref().Load(0, false);

		// if a stamp was actually loaded
		if (stampData)
		{
			int width, height;
			stampImg = prerender_save((char*)stampData->GetSaveData(), stampData->GetSaveSize(), &width, &height);
			if (stampImg)
			{
				state = LOAD;
				loadSize = Point(width, height);
				waitToDraw = true;
				UpdateStampCoordinates(cursor);
			}
			else
			{
				delete stampData;
				stampData = nullptr;
			}
		}
		break;
	case SDL_SCANCODE_SEMICOLON:
		if (ctrl)
		{
			SPECIFIC_DELETE = !SPECIFIC_DELETE;
			REPLACE_MODE = false;
		}
		else
		{
			REPLACE_MODE = !REPLACE_MODE;
			SPECIFIC_DELETE = false;
		}
		break;
	case SDL_SCANCODE_Z:
		// ctrl + z
		if (ctrlHeld)
		{
			if (shiftHeld)
				SnapshotHistory::HistoryForward(sim);
			else
				SnapshotHistory::HistoryRestore(sim);
		}
		// zoom window
		else
		{
			if (isStampMouseDown)
				break;
			placingZoom = true;
			isMouseDown = false;
			UpdateZoomCoordinates(mouse);
		}
		break;
	case SDL_SCANCODE_X:
		if (ctrlHeld)
		{
			ResetStampState();
			state = CUT;
		}
		break;
	case SDL_SCANCODE_C:
		if (ctrlHeld)
		{
			ResetStampState();
			state = COPY;
		}
		break;
	case SDL_SCANCODE_V:
		if (ctrlHeld && clipboardData)
		{
			ResetStampState();
			stampData = new Save(*clipboardData);
			if (stampData)
			{
				stampImg = prerender_save((char*)stampData->GetSaveData(), stampData->GetSaveSize(), &loadSize.X, &loadSize.Y);
				if (stampImg)
				{
					state = LOAD;
					isStampMouseDown = false;
					UpdateStampCoordinates(cursor);
				}
				else
				{
					delete stampData;
					stampData = NULL;
				}
			}
		}
		break;
	case SDL_SCANCODE_B:
		if (sdl_mod & (KMOD_CTRL|KMOD_GUI))
		{
			decorations_enable = !decorations_enable;
			if (decorations_enable)
				SetInfoTip("装饰层：开");
			else
				SetInfoTip("装饰层：关");
		}
		else if (active_menu == SC_DECO)
		{
			active_menu = last_active_menu;
			SwapToRegularToolset();
		}
		else
		{
			last_active_menu = active_menu;
			decorations_enable = true;
			SetPause(1);
			active_menu = SC_DECO;

			SwapToDecoToolset();
		}
		break;
	case SDL_SCANCODE_N:
		if (ctrlHeld)
		{
			if (num_tabs < 22-GetNumMenus())
			{
				tab_save(tab_num);
				num_tabs++;
				tab_num = num_tabs;
				NewSim();
				tab_save(tab_num);
			}
		}
		else
		{
			if (sim->grav->IsEnabled())
			{
				sim->grav->StopAsync();
				SetInfoTip("牛顿引力：关闭");
			}
			else
			{
				sim->grav->StartAsync();
				SetInfoTip("牛顿引力：开");
			}
		}
		break;
	case SDL_SCANCODE_SPACE:
		TogglePause();
		break;
	case SDL_SCANCODE_1:
		if (shiftHeld && DEBUG_MODE)
		{
			if (ctrlHeld)
				Renderer::Ref().XORColorMode(COLOR_LIFE);
			else
				LoadRenderPreset(CM_LIFE);
		}
		else if (ctrlHeld)
			Renderer::Ref().ToggleDisplayMode(DISPLAY_AIRV);
		else
			LoadRenderPreset(CM_VEL);
		break;
	case SDL_SCANCODE_2:
		if (ctrlHeld)
			Renderer::Ref().ToggleDisplayMode(DISPLAY_AIRP);
		else
			LoadRenderPreset(CM_PRESS);
		break;
	case SDL_SCANCODE_3:
		if (ctrlHeld)
			Renderer::Ref().ToggleDisplayMode(DISPLAY_PERS);
		else
			LoadRenderPreset(CM_PERS);
		break;
	case SDL_SCANCODE_4:
		if (ctrlHeld)
			Renderer::Ref().ToggleRenderMode(FIREMODE);
		else
			LoadRenderPreset(CM_FIRE);
		break;
	case SDL_SCANCODE_5:
		if (ctrlHeld)
			Renderer::Ref().ToggleRenderMode(PMODE_BLOB);
		else
			LoadRenderPreset(CM_BLOB);
		break;
	case SDL_SCANCODE_6:
		if (ctrlHeld)
			Renderer::Ref().XORColorMode(COLOR_HEAT);
		else
		{
			LoadRenderPreset(CM_HEAT);
			if (shiftHeld)
			{
				heatmode = (heatmode == 1) ? 0 : 1;
				if (heatmode)
					SetInfoTip("动态热量模式：开");
				else
					SetInfoTip("动态热量模式：关");
			}
		}
		break;
	case SDL_SCANCODE_7:
		if (ctrlHeld)
			Renderer::Ref().ToggleDisplayMode(DISPLAY_WARP);
		else
			LoadRenderPreset(CM_FANCY);
		break;
	case SDL_SCANCODE_8:
		LoadRenderPreset(CM_NOTHING);
		break;
	case SDL_SCANCODE_9:
		if (ctrlHeld)
			Renderer::Ref().XORColorMode(COLOR_GRAD);
		else
			LoadRenderPreset(CM_GRAD);
		break;
	case SDL_SCANCODE_0:
		if (shiftHeld && DEBUG_MODE)
		{
			if (ctrlHeld)
				Renderer::Ref().ToggleDisplayMode(DISPLAY_AIRW);
			else
				LoadRenderPreset(CM_VORT);
		}
		else if (ctrlHeld)
			Renderer::Ref().ToggleDisplayMode(DISPLAY_AIRC);
		else
			LoadRenderPreset(CM_CRACK);
		break;
	}

	// STKM & STKM2
	if (state != LOAD)
	{
		STKM_ElementDataContainer::StkmKeys pressedKey = STKM_ElementDataContainer::None;
		bool stk2 = false;
		if (key == SDLK_UP || scan == SDL_SCANCODE_W)
		{
			pressedKey = STKM_ElementDataContainer::Up;
			stk2 = scan == SDL_SCANCODE_W;
		}
		else if (key == SDLK_LEFT || scan == SDL_SCANCODE_A)
		{
			pressedKey = STKM_ElementDataContainer::Left;
			stk2 = scan == SDL_SCANCODE_A;
		}
		else if (key == SDLK_DOWN || scan == SDL_SCANCODE_S)
		{
			pressedKey = STKM_ElementDataContainer::Down;
			stk2 = scan == SDL_SCANCODE_S;
		}
		else if (key == SDLK_RIGHT || scan == SDL_SCANCODE_D)
		{
			pressedKey = STKM_ElementDataContainer::Right;
			stk2 = scan == SDL_SCANCODE_D;
		}
		if (stk2 && ctrl)
			return;
		if (pressedKey != STKM_ElementDataContainer::None)
			static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).HandleKeyPress(pressedKey, stk2);
	}

	if (!PlacingZoomWindow() && state == NONE)
	{
		if (key == SDLK_UP && ctrl && tab_num > 1)
		{
			tab_save(tab_num);
			tab_num--;
			tab_load(tab_num);
		}
		else if (key == SDLK_DOWN && ctrl && tab_num < num_tabs)
		{
			tab_save(tab_num);
			tab_num++;
			tab_load(tab_num);
		}
	}
}

bool PowderToy::BeforeKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (!ctrl && ctrlHeld)
	{
		ctrlHeld = false;
		openBrowserButton->SetTooltipText("查找并打开模拟");
		UpdateToolStrength();
	}
	if (!shift && shiftHeld)
	{
		shiftHeld = false;
		UpdateToolStrength();
	}
	if (!alt && altHeld)
		altHeld = false;

	if (deco_disablestuff)
		return true;

	KeyEvent ev = KeyEvent(key, scan, repeat, shift, ctrl, alt);
	return HandleEvent(LuaEvents::keyrelease, &ev);
}

void PowderToy::OnKeyRelease(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	// temporary
	if (key == 0)
	{
		ctrlHeld = shiftHeld = altHeld = 0;
	}

	if (repeat || deco_disablestuff)
		return;

	switch (scan)
	{
	case SDL_SCANCODE_Z:
		if (placingZoom)
			HideZoomWindow();
		break;
	}

	// STKM & STKM2
	if (state != LOAD)
	{
		STKM_ElementDataContainer::StkmKeys pressedKey = STKM_ElementDataContainer::None;
		bool stk2 = false;
		if (key == SDLK_UP || scan == SDL_SCANCODE_W)
		{
			pressedKey = STKM_ElementDataContainer::Up;
			stk2 = scan == SDL_SCANCODE_W;
		}
		else if (key == SDLK_LEFT || scan == SDL_SCANCODE_A)
		{
			pressedKey = STKM_ElementDataContainer::Left;
			stk2 = scan == SDL_SCANCODE_A;
		}
		else if (key == SDLK_DOWN || scan == SDL_SCANCODE_S)
		{
			pressedKey = STKM_ElementDataContainer::Down;
			stk2 = scan == SDL_SCANCODE_S;
		}
		else if (key == SDLK_RIGHT || scan == SDL_SCANCODE_D)
		{
			pressedKey = STKM_ElementDataContainer::Right;
			stk2 = scan == SDL_SCANCODE_D;
		}
		if (pressedKey != STKM_ElementDataContainer::None)
			static_cast<STKM_ElementDataContainer&>(*sim->elementData[PT_STKM]).HandleKeyRelease(pressedKey, stk2);
	}
}

bool PowderToy::BeforeTextInput(const char *text)
{
	TextInputEvent ev = TextInputEvent(text);
	return HandleEvent(LuaEvents::textinput, &ev);
}

void PowderToy::OnDefocus()
{
	ctrlHeld = shiftHeld = altHeld = false;
	openBrowserButton->SetTooltipText("查找并打开模拟");
	lastMouseDown = heldKey = heldScan = 0; // temporary
	ResetStampState();
	UpdateDrawMode();
	UpdateToolStrength();

	BlurEvent ev = BlurEvent();
	HandleEvent(LuaEvents::blur, &ev);
	// Send fake mouseup event to Lua
	MouseUpEvent ev2 = MouseUpEvent(0, 0, 0, mouseUpBlur);
	HandleEvent(LuaEvents::mouseup, &ev2);
}

void PowderToy::OnJoystickMotion(uint8_t joysticknum, uint8_t axis, int16_t value)
{
	orientation[axis] = value;
}


void PowderToy::OnFileDrop(const char *filename)
{
	int len = strlen(filename);
	if (len < 4 || (strcmp(filename + (len - 4), ".cps") && strcmp(filename + (len - 4), ".stm")))
	{
		Engine::Ref().ShowWindow(new ErrorPrompt("拖入的文件不是 TPT 存档，扩展名必须为 .cps 或 .stm"));
		return;
	}

	int fileSize;
	char *fileData = (char*)file_load(filename, &fileSize);
	Save *save = new Save(fileData, fileSize);
	free(fileData);

	try
	{
		sim->LoadSave(0, 0, save, 1);
	}
	catch (ParseException &e)
	{
		Engine::Ref().ShowWindow(new ErrorPrompt("加载存档失败：" + std::string(e.what())));
	}

	delete save;

	UpdateToolTip(introText, Point(16, 20), INTROTIP, 255);
}
