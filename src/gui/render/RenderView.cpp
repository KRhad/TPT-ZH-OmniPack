#include "RenderView.h"
#include "simulation/ElementGraphics.h"
#include "simulation/SimulationData.h"
#include "simulation/Simulation.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"
#include "graphics/VideoBuffer.h"
#include "RenderController.h"
#include "RenderModel.h"
#include "gui/interface/Checkbox.h"
#include "gui/interface/Button.h"
#include "gui/game/GameController.h"
#include "gui/game/GameView.h"

class ModeCheckbox : public ui::Checkbox
{
public:
	using ui::Checkbox::Checkbox;
	uint32_t mode;
};

RenderView::RenderView():
	ui::Window(ui::Point(0, 0), ui::Point(XRES, WINDOWH)),
	ren(nullptr),
	toolTip(""),
	isToolTipFadingIn(false)
{
	auto addPresetButton = [this](int index, Icon icon, ui::Point offset, String tooltip) {
		auto *presetButton = new ui::Button(ui::Point(XRES, YRES) + offset, ui::Point(30, 13), "", tooltip);
		presetButton->SetIcon(icon);
		presetButton->SetActionCallback({ [this, index] { c->LoadRenderPreset(index); } });
		AddComponent(presetButton);
	};
	addPresetButton( 1, IconVelocity  , ui::Point( -37,  6), "速度显示模式预设");
	addPresetButton( 2, IconPressure  , ui::Point( -37, 24), "压力显示模式预设");
	addPresetButton( 3, IconPersistant, ui::Point( -76,  6), "残影显示模式预设");
	addPresetButton( 4, IconFire      , ui::Point( -76, 24), "火焰显示模式预设");
	addPresetButton( 5, IconBlob      , ui::Point(-115,  6), "团状显示模式预设");
	addPresetButton( 6, IconHeat      , ui::Point(-115, 24), "温度显示模式预设");
	addPresetButton( 7, IconBlur      , ui::Point(-154,  6), "炫彩显示模式预设");
	addPresetButton( 8, IconBasic     , ui::Point(-154, 24), "无特效显示模式预设");
	addPresetButton( 9, IconGradient  , ui::Point(-193,  6), "热梯度显示模式预设");
	addPresetButton( 0, IconAltAir    , ui::Point(-193, 24), "替代速度显示模式预设");
	addPresetButton(10, IconLife      , ui::Point(-232,  6), "寿命显示模式预设");

	auto addRenderModeCheckbox = [this](unsigned int mode, Icon icon, ui::Point offset, String tooltip) {
		auto *renderModeCheckbox = new ModeCheckbox(ui::Point(0, YRES) + offset, ui::Point(30, 16), "", tooltip);
		renderModes.push_back(renderModeCheckbox);
		renderModeCheckbox->mode = mode;
		renderModeCheckbox->SetIcon(icon);
		renderModeCheckbox->SetActionCallback({ [this] {
			auto renderMode = CalculateRenderMode();
			c->SetRenderMode(renderMode);
		} });
		AddComponent(renderModeCheckbox);
	};
	addRenderModeCheckbox(RENDER_EFFE, IconEffect, ui::Point( 1,  4), "为某些元素添加特殊耀斑效果");
	addRenderModeCheckbox(RENDER_FIRE, IconFire  , ui::Point( 1, 22), "气体火焰效果");
	addRenderModeCheckbox(RENDER_GLOW, IconGlow  , ui::Point(33,  4), "某些元素的发光效果");
	addRenderModeCheckbox(RENDER_BLUR, IconBlur  , ui::Point(33, 22), "液体的模糊效果");
	addRenderModeCheckbox(RENDER_BLOB, IconBlob  , ui::Point(65,  4), "将所有内容绘制成团状");
	addRenderModeCheckbox(RENDER_BASC, IconBasic , ui::Point(65, 22), "基础渲染；关闭后大多数内容将不可见");
	addRenderModeCheckbox(RENDER_SPRK, IconEffect, ui::Point(97,  4), "火花的发光效果");

	auto addDisplayModeCheckbox = [this](unsigned int mode, Icon icon, ui::Point offset, String tooltip) {
		auto *displayModeCheckbox = new ModeCheckbox(ui::Point(0, YRES) + offset, ui::Point(30, 16), "", tooltip);
		displayModes.push_back(displayModeCheckbox);
		displayModeCheckbox->mode = mode;
		displayModeCheckbox->SetIcon(icon);
		displayModeCheckbox->SetActionCallback({ [this, displayModeCheckbox] {
			auto displayMode = c->GetDisplayMode();
			// Air display modes are mutually exclusive
			if (displayModeCheckbox->mode & DISPLAY_AIR)
			{
				displayMode &= ~DISPLAY_AIR;
			}
			if (displayModeCheckbox->GetChecked())
			{
				displayMode |= displayModeCheckbox->mode;
			}
			else
			{
				displayMode &= ~displayModeCheckbox->mode;
			}
			c->SetDisplayMode(displayMode);
		} });
		AddComponent(displayModeCheckbox);
	};
	line1 = 130;
	addDisplayModeCheckbox(DISPLAY_AIRC, IconAltAir    , ui::Point(135,  4), "将压力显示为红色和蓝色，将速度显示为白色");
	addDisplayModeCheckbox(DISPLAY_AIRP, IconPressure  , ui::Point(135, 22), "显示压力，红色为正，蓝色为负");
	addDisplayModeCheckbox(DISPLAY_AIRV, IconVelocity  , ui::Point(167,  4), "显示速度和正压力：上/下添加蓝色，右/左添加红色，静止压力添加绿色");
	addDisplayModeCheckbox(DISPLAY_AIRH, IconHeat      , ui::Point(167, 22), "像热量显示器一样显示空气温度");
	addDisplayModeCheckbox(DISPLAY_AIRW, IconVort      , ui::Point(199,  4), "显示涡量，红色为顺时针，蓝色为逆时针");
	line2 = 232;
	addDisplayModeCheckbox(DISPLAY_WARP, IconWarp      , ui::Point(237, 22), "重力透镜，牛顿重力使光线弯曲");
	addDisplayModeCheckbox(DISPLAY_EFFE, IconEffect    , ui::Point(237,  4), "启用移动固体、火柴人的武器和高级画面效果");
	addDisplayModeCheckbox(DISPLAY_PERS, IconPersistant, ui::Point(269,  4), "元素路径在屏幕上保留一段时间");
	line3 = 302;

	auto addColourModeCheckbox = [this](unsigned int mode, Icon icon, ui::Point offset, String tooltip) {
		auto *colourModeCheckbox = new ModeCheckbox(ui::Point(0, YRES) + offset, ui::Point(30, 16), "", tooltip);
		colourModes.push_back(colourModeCheckbox);
		colourModeCheckbox->mode = mode;
		colourModeCheckbox->SetIcon(icon);
		colourModeCheckbox->SetActionCallback({ [this, colourModeCheckbox] {
			auto colorMode = c->GetColorMode();
			// exception: looks like an independent set of settings but behaves more like an index
			if (colourModeCheckbox->GetChecked())
			{
				colorMode = colourModeCheckbox->mode;
			}
			else
			{
				colorMode = 0;
			}
			c->SetColorMode(colorMode);
		} });
		AddComponent(colourModeCheckbox);
	};
	addColourModeCheckbox(COLOUR_HEAT, IconHeat    , ui::Point(307,  4), "显示元素的温度，深蓝色最冷，粉色最热");
	addColourModeCheckbox(COLOUR_LIFE, IconLife    , ui::Point(307, 22), "以灰度渐变方式显示元素的生命值");
	addColourModeCheckbox(COLOUR_GRAD, IconGradient, ui::Point(339, 22), "稍微改变元素的颜色以显示通过它们扩散的热量");
	addColourModeCheckbox(COLOUR_BASC, IconBasic   , ui::Point(339,  4), "任何东西都没有特殊效果，覆盖所有其他选项和装饰");
	line4 = 372;
}

uint32_t RenderView::CalculateRenderMode()
{
	uint32_t renderMode = 0;
	for (auto &checkbox : renderModes)
	{
		if (checkbox->GetChecked())
			renderMode |= checkbox->mode;
	}

	return renderMode;
}

void RenderView::OnMouseDown(int x, int y, unsigned button)
{
	if(x > XRES || y < YRES)
		c->Exit();
}

void RenderView::OnTryExit(ExitMethod method)
{
	c->Exit();
}

void RenderView::NotifyRendererChanged(RenderModel * sender)
{
	ren = sender->GetRenderer();
	rendererSettings = sender->GetRendererSettings();
}

void RenderView::NotifySimulationChanged(RenderModel * sender)
{
	sim = sender->GetSimulation();
}

void RenderView::NotifyRenderChanged(RenderModel * sender)
{
	for (size_t i = 0; i < renderModes.size(); i++)
	{
		//Compares bitmasks at the moment, this means that "Point" is always on when other options that depend on it are, this might confuse some users, TODO: get the full list and compare that?
		auto renderMode = renderModes[i]->mode;
		renderModes[i]->SetChecked(renderMode == (sender->GetRenderMode() & renderMode));
	}
}

void RenderView::NotifyDisplayChanged(RenderModel * sender)
{
	for (size_t i = 0; i < displayModes.size(); i++)
	{
		auto displayMode = displayModes[i]->mode;
		displayModes[i]->SetChecked(displayMode == (sender->GetDisplayMode() & displayMode));
	}
}

void RenderView::NotifyColourChanged(RenderModel * sender)
{
	for (size_t i = 0; i < colourModes.size(); i++)
	{
		auto colorMode = colourModes[i]->mode;
		colourModes[i]->SetChecked(colorMode == sender->GetColorMode());
	}
}

void RenderView::OnDraw()
{
	Graphics * g = GetGraphics();
	g->DrawFilledRect(WINDOW.OriginRect(), 0x000000_rgb);
	auto *view = GameController::Ref().GetView();
	view->PauseRendererThread();
	ren->ApplySettings(*rendererSettings);
	view->RenderSimulation(*sim, true);
	view->AfterSimDraw(*sim);
	for (auto y = 0; y < YRES; ++y)
	{
		auto &video = ren->GetVideo();
		std::copy_n(video.data() + video.Size().X * y, video.Size().X, g->Data() + g->Size().X * y);
	}
	g->DrawLine({ 0, YRES }, { XRES-1, YRES }, 0xC8C8C8_rgb);
	g->DrawLine({ line1, YRES }, { line1, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ line2, YRES }, { line2, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ line3, YRES }, { line3, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ line4, YRES }, { line4, WINDOWH }, 0xC8C8C8_rgb);
	g->DrawLine({ XRES, 0 }, { XRES, WINDOWH }, 0xFFFFFF_rgb);
	if(toolTipPresence && toolTip.length())
	{
		g->BlendText({ 6, Size.Y-MENUSIZE-12 }, toolTip, 0xFFFFFF_rgb .WithAlpha(toolTipPresence>51?255:toolTipPresence*5));
	}
}

void RenderView::OnTick()
{
	if (isToolTipFadingIn)
	{
		isToolTipFadingIn = false;
		toolTipPresence.SetTarget(120);
	}
	else
	{
		toolTipPresence.SetTarget(0);
	}
}

void RenderView::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (repeat)
		return;
	if (shift && key == '1')
		c->LoadRenderPreset(10);
	else if(key >= '0' && key <= '9')
	{
		c->LoadRenderPreset(key-'0');
	}
}

void RenderView::ToolTip(ui::Point senderPosition, String toolTip)
{
	this->toolTip = toolTip;
	this->isToolTipFadingIn = true;
}

RenderView::~RenderView() {
}
