#include <iomanip>
#include <iostream>
#ifdef WIN
#include <direct.h>
#define getcwd _getcwd
#endif
#if defined(LIN) || defined(MACOSX)
#include <unistd.h> // getcwd
#endif

#include "OptionsUI.h"
#include "graphics.h" // for HeatToColor
#include "common/Format.h"
#include "common/Platform.h"
#include "common/tpt-minmax.h"
#include "game/Brush.h"
#include "game/Menus.h"
#include "graphics/VideoBuffer.h"
#include "interface/Button.h"
#include "interface/Checkbox.h"
#include "interface/Dropdown.h"
#include "interface/Engine.h"
#include "interface/Label.h"
#include "interface/ScrollWindow.h"
#include "interface/Textbox.h"
#include "simulation/Simulation.h"
#include "gui/dialogs/ConfirmPrompt.h"
#include "gui/dialogs/InfoPrompt.h"
#include "gui/gravity/GravityWindow.h"
#include "gui/prop/PropWindow.h" // for ParseFloat

OptionsUI::OptionsUI(Simulation *sim):
	ui::Window(Point(CENTERED, CENTERED), Point(310, 350)),
	sim(sim)
{
#ifndef TOUCHUI
	int checkboxHeight = 13;
	bool useCheckIcon = true;
	int okButtonHeight = 15;
#else
	int checkboxHeight = 20;
	bool useCheckIcon = false;
	int okButtonHeight = 25;
#endif

	Label *headerLabel = new Label(Point(5, 3), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Options");
	headerLabel->SetColor(COLRGB(140, 140, 255));
	this->AddComponent(headerLabel);

	int headerHeight = headerLabel->GetPosition().Y + headerLabel->GetSize().Y;
	scrollArea = new ui::ScrollWindow(Point(0, headerHeight), this->size - Point(0, headerHeight + okButtonHeight));
	scrollArea->SetOnDraw([this](gfx::VideoBuffer *buf) { OnSubwindowDraw(buf); });
	this->AddSubwindow(scrollArea);

	Component *prev;

	prev = heatSimCheckbox = new Checkbox(Point(5, 4), Point(Checkbox::AUTOSIZE, checkboxHeight), "Heat simulation");
	heatSimCheckbox->UseCheckIcon(useCheckIcon);
	heatSimCheckbox->SetCallback([&](bool checked) { this->HeatSimChecked(checked); });
	scrollArea->AddComponent(heatSimCheckbox);

	Label *descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Causes unexpected behavior when disabled");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

	prev = ambientCheckbox = new Checkbox(prev->Below(Point(0, 17)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Ambient heat simulation");
	ambientCheckbox->UseCheckIcon(useCheckIcon);
	ambientCheckbox->SetCallback([&](bool checked) { this->AmbientChecked(checked); });
	scrollArea->AddComponent(ambientCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Heat transfers through empty space");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

	prev = newtonianCheckbox = new Checkbox(prev->Below(Point(0, 17)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Newtonian Gravity");
	newtonianCheckbox->UseCheckIcon(useCheckIcon);
	newtonianCheckbox->SetCallback([&](bool checked) { this->NewtonianChecked(checked); });
	scrollArea->AddComponent(newtonianCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Simulate local gravity fields");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

#ifdef TOUCHUI
	prev = decorationCheckbox = new Checkbox(prev->Below(Point(0, 17)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Decorations");
	decorationCheckbox->UseCheckIcon(useCheckIcon);
	decorationCheckbox->SetCallback([&](bool checked) { this->DecorationsChecked(checked); });
	scrollArea->AddComponent(decorationCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Show deco color on elements");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);
#endif

	prev = waterEqalizationCheckbox = new Checkbox(prev->Below(Point(0, 17)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Water Equalization");
	waterEqalizationCheckbox->UseCheckIcon(useCheckIcon);
	waterEqalizationCheckbox->SetCallback([&](bool checked) { this->WaterEqualizationChecked(checked); });
	scrollArea->AddComponent(waterEqalizationCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Water equalizes in a U-shaped pipe (lags game)");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);


	prev = airSimDropdown = new Dropdown(prev->Below(Point(0, 17)), Point(Dropdown::AUTOSIZE, Dropdown::AUTOSIZE), {"On", "Pressure Off", "Velocity Off", "Off", "No Update"});
	airSimDropdown->SetCallback([&](unsigned int option) { this->AirSimSelected(option); });
	scrollArea->AddComponent(airSimDropdown);

	descLabel = new Label(Point(17, prev->GetPosition().Y), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Air Simulation Mode:");
	scrollArea->AddComponent(descLabel);

	prev = airTempTextbox = new Textbox(prev->Below(Point(0, 4)), Point(0, Textbox::AUTOSIZE), "");
	airTempTextbox->SetCallback([&]() { this->UpdateAirTemp(airTempTextbox->GetText(), false); });
	airTempTextbox->SetDefocusCallback([&]() { this->UpdateAirTemp(airTempTextbox->GetText(), true); });
	scrollArea->AddComponent(airTempTextbox);

	descLabel = new Label(Point(17, prev->GetPosition().Y), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Ambient Air Temperature:");
	scrollArea->AddComponent(descLabel);

	airTempDisplay = new Button(Point(0, airTempTextbox->GetPosition().Y), Point(17, 17), "");
	airTempDisplay->SetEnabled(false);
	scrollArea->AddComponent(airTempDisplay);

	prev = gravityDropdown = new Dropdown(prev->Below(Point(0, 4)), Point(Dropdown::AUTOSIZE, Dropdown::AUTOSIZE), {"Vertical", "Off", "Radial", "Custom"});
	gravityDropdown->SetCallback([&](unsigned int option) { this->GravitySelected(option); });
	scrollArea->AddComponent(gravityDropdown);

	descLabel = new Label(Point(17, prev->GetPosition().Y), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Gravity Simulation Mode:");
	scrollArea->AddComponent(descLabel);

	prev = edgeModeDropdown = new Dropdown(prev->Below(Point(0, 4)), Point(Dropdown::AUTOSIZE, Dropdown::AUTOSIZE), {"Void", "Solid", "Loop"});
	edgeModeDropdown->SetCallback([&](unsigned int option) { this->EdgeModeSelected(option); });
	scrollArea->AddComponent(edgeModeDropdown);

	descLabel = new Label(Point(17, prev->GetPosition().Y), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Edge Mode:");
	scrollArea->AddComponent(descLabel);

	prev = decoSpaceDropdown = new Dropdown(prev->Below(Point(0, 4)), Point(Dropdown::AUTOSIZE, Dropdown::AUTOSIZE), {"sRGB", "Linear", "Gamma 2.2", "Gamma 1.8"});
	decoSpaceDropdown->SetCallback([&](unsigned int option) { this->DecoSpaceSelected(option); });
	scrollArea->AddComponent(decoSpaceDropdown);

	descLabel = new Label(Point(17, prev->GetPosition().Y), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Smudge Tool Color Space:");
	scrollArea->AddComponent(descLabel);

	prev = temperatureScaleDropdown = new Dropdown(prev->Below(Point(0, 4)), Point(Dropdown::AUTOSIZE, Dropdown::AUTOSIZE), {"Kelvin", "Celsius", "Fahrenheit"});
	temperatureScaleDropdown->SetCallback([&](unsigned int option) { this->TemperatureScaleSelected(option); });
	scrollArea->AddComponent(temperatureScaleDropdown);

	descLabel = new Label(Point(17, prev->GetPosition().Y), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Temperature Scale:");
	scrollArea->AddComponent(descLabel);

	// set dropdown widths to width of largest one
	int maxWidth = airSimDropdown->GetSize().X;
	maxWidth = tpt::max(maxWidth, gravityDropdown->GetSize().X);
	maxWidth = tpt::max(maxWidth, edgeModeDropdown->GetSize().X);
	maxWidth = tpt::max(maxWidth, decoSpaceDropdown->GetSize().X);
	maxWidth = tpt::max(maxWidth, temperatureScaleDropdown->GetSize().X);
	maxWidth = tpt::max(maxWidth, gfx::VideoBuffer::TextSize("17538.53F").X + airTempDisplay->GetSize().X + 12); // space for air temp textbox
	int xPos = scrollArea->GetUsableWidth() - 5 - maxWidth;
	airSimDropdown->SetPosition(Point(xPos, airSimDropdown->GetPosition().Y));
	airSimDropdown->SetSize(Point(maxWidth, airSimDropdown->GetSize().Y));
	airTempTextbox->SetPosition(Point(xPos, airTempTextbox->GetPosition().Y));
	airTempTextbox->SetSize(Point(maxWidth - 21, airTempTextbox->GetSize().Y));
	airTempDisplay->SetPosition(Point(xPos + maxWidth - airTempDisplay->GetSize().X, airTempDisplay->GetPosition().Y));
	gravityDropdown->SetPosition(Point(xPos, gravityDropdown->GetPosition().Y));
	gravityDropdown->SetSize(Point(maxWidth, gravityDropdown->GetSize().Y));
	edgeModeDropdown->SetPosition(Point(xPos, edgeModeDropdown->GetPosition().Y));
	edgeModeDropdown->SetSize(Point(maxWidth, edgeModeDropdown->GetSize().Y));
	decoSpaceDropdown->SetPosition(Point(xPos, decoSpaceDropdown->GetPosition().Y));
	decoSpaceDropdown->SetSize(Point(maxWidth, decoSpaceDropdown->GetSize().Y));
	temperatureScaleDropdown->SetPosition(Point(xPos, temperatureScaleDropdown->GetPosition().Y));
	temperatureScaleDropdown->SetSize(Point(maxWidth, temperatureScaleDropdown->GetSize().Y));

#ifndef TOUCHUI
	std::vector<std::string> scaleOptions;
	for (int i = 1; i <= Engine::Ref().GuessBestScale(); i++)
		scaleOptions.push_back(Format::NumberToString<int>(i) + "x");
	prev = scaleDropdown = new Dropdown(Point(heatSimCheckbox->GetPosition().X, prev->Below(Point(0, 14)).Y), Point(23, Dropdown::AUTOSIZE), scaleOptions);
	scaleDropdown->SetCallback([&](unsigned int option) { this->ScaleSelected(option); });
	scrollArea->AddComponent(scaleDropdown);

	descLabel = new Label(prev->Right(Point(3, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Window scale factor for larger screens");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

	prev = resizableCheckbox = new Checkbox(prev->Below(Point(0, 10)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Resizable Window");
	resizableCheckbox->UseCheckIcon(useCheckIcon);
	resizableCheckbox->SetCallback([&](bool checked) { this->ResizableChecked(checked); });
	scrollArea->AddComponent(resizableCheckbox);

	resizableLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Allow resizing window");
	resizableLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(resizableLabel);

	forceIntegerScalingCheckbox = new Checkbox(Point(0, 0), Point(Checkbox::AUTOSIZE, checkboxHeight), "Force Integer Scaling");
	forceIntegerScalingCheckbox->SetPosition(Point(scrollArea->GetUsableWidth() - 5 - forceIntegerScalingCheckbox->GetSize().X, resizableCheckbox->GetPosition().Y));
	forceIntegerScalingCheckbox->UseCheckIcon(useCheckIcon);
	forceIntegerScalingCheckbox->SetCallback([&](bool checked) { this->ForceIntegerScalingChecked(checked); });
	scrollArea->AddComponent(forceIntegerScalingCheckbox);

	forceIntegerScalingLabel = new Label(forceIntegerScalingCheckbox->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Less Blurry");
	forceIntegerScalingLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(forceIntegerScalingLabel);

	filteringDropdown = new Dropdown(Point(0, 0), Point(Dropdown::AUTOSIZE, Dropdown::AUTOSIZE), {"Nearest", "Linear", "Best"});
	filteringDropdown->SetPosition(Point(scrollArea->GetUsableWidth() - 5 - filteringDropdown->GetSize().X, resizableCheckbox->GetPosition().Y));
	filteringDropdown->SetCallback([&](unsigned int option) { this->FilteringSelected(option); });
	scrollArea->AddComponent(filteringDropdown);
	filteringDropdown->SetVisible(false);

	filteringLabel = new Label(Point(0, 0), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Pixel sampling mode:");
	filteringLabel->SetPosition(Point(filteringDropdown->Left(Point(filteringLabel->GetSize().X + 5, 0)).X, filteringDropdown->GetPosition().Y));
	scrollArea->AddComponent(filteringLabel);
	filteringLabel->SetVisible(false);

	prev = fullscreenCheckbox = new Checkbox(prev->Below(Point(0, 17)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Fullscreen");
	fullscreenCheckbox->UseCheckIcon(true);
	fullscreenCheckbox->SetCallback([&](bool checked) { this->FullscreenChecked(checked); });
	scrollArea->AddComponent(fullscreenCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Fill the entire screen");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

	altFullscreenCheckbox = new Checkbox(Point(0, 0), Point(Checkbox::AUTOSIZE, checkboxHeight), "Change Resolution");
	altFullscreenCheckbox->SetPosition(Point(scrollArea->GetUsableWidth() - 5 - altFullscreenCheckbox->GetSize().X, fullscreenCheckbox->GetPosition().Y));
	altFullscreenCheckbox->UseCheckIcon(useCheckIcon);
	altFullscreenCheckbox->SetCallback([&](bool checked) { this->AltFullscreenChecked(checked); });
	scrollArea->AddComponent(altFullscreenCheckbox);

	descLabel = new Label(altFullscreenCheckbox->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Old fullscreen");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);


	prev = fastQuitCheckbox = new Checkbox(prev->Below(Point(0, 24)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Fast Quit");
	fastQuitCheckbox->UseCheckIcon(useCheckIcon);
	fastQuitCheckbox->SetCallback([&](bool checked) { this->FastQuitChecked(checked); });
	scrollArea->AddComponent(fastQuitCheckbox);

	descLabel = new Label(fastQuitCheckbox->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Always exit completely when clicking \"X\"");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);
#endif

	prev = updatesCheckbox = new Checkbox(Point(heatSimCheckbox->GetPosition().X, prev->Below(Point(0, 15)).Y), Point(Checkbox::AUTOSIZE, checkboxHeight), "Update Check");
	updatesCheckbox->UseCheckIcon(useCheckIcon);
	updatesCheckbox->SetCallback([&](bool checked) { this->UpdatesChecked(checked); });
	scrollArea->AddComponent(updatesCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Check for updates at https://starcatcher.us/TPT");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

#ifndef TOUCHUI
	prev = momentumScrollingCheckbox = new Checkbox(prev->Below(Point(0, 15)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Momentum Scrolling");
	momentumScrollingCheckbox->UseCheckIcon(useCheckIcon);
	momentumScrollingCheckbox->SetCallback([&](bool checked) { this->MomentumChecked(checked); });
	scrollArea->AddComponent(momentumScrollingCheckbox);
	
	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Acceleration instead of step scroll");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

	prev = stickyCategoriesCheckbox = new Checkbox(prev->Below(Point(0, 15)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Sticky categories");
	stickyCategoriesCheckbox->UseCheckIcon(useCheckIcon);
	stickyCategoriesCheckbox->SetCallback([&](bool checked) { this->StickyCatsChecked(checked); });
	scrollArea->AddComponent(stickyCategoriesCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Switch between menusections by clicking");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);
#endif

	prev = savePressureCheckbox = new Checkbox(prev->Below(Point(0, 15)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Include Pressure");
	savePressureCheckbox->UseCheckIcon(useCheckIcon);
	savePressureCheckbox->SetCallback([&](bool checked) { this->SavePressureChecked(checked); });
	scrollArea->AddComponent(savePressureCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Include pressure in saves and stamps");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

	prev = circleCheckbox = new Checkbox(prev->Below(Point(0, 15)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Perfect Circle Brush");
	circleCheckbox->UseCheckIcon(useCheckIcon);
	circleCheckbox->SetCallback([&](bool checked) { this->CircleChecked(checked); });
	scrollArea->AddComponent(circleCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Better circle brush, without incorrect points on edges");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

	prev = graveExitsConsole = new Checkbox(prev->Below(Point(0, 15)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Key Under Esc Exits Console");
	graveExitsConsole->UseCheckIcon(useCheckIcon);
	graveExitsConsole->SetCallback([&](bool checked) { this->GraveChecked(checked); });
	scrollArea->AddComponent(graveExitsConsole);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Uncheck this if that key is 0 on your keyboard");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);

#ifndef ANDROID
	prev = incompatibleCheckbox = new Checkbox(prev->Below(Point(0, 15)), Point(Checkbox::AUTOSIZE, checkboxHeight), "Load incompatible saves");
	incompatibleCheckbox->UseCheckIcon(useCheckIcon);
	incompatibleCheckbox->SetCallback([&](bool checked) { this->IncompatibleChecked(checked); });
	scrollArea->AddComponent(incompatibleCheckbox);

	descLabel = new Label(prev->Below(Point(15, 0)), Point(Label::AUTOSIZE, Label::AUTOSIZE), "Saves from the future may load completely garbled");
	descLabel->SetColor(COLRGB(150, 150, 150));
	scrollArea->AddComponent(descLabel);
#endif

#ifndef TOUCHUI
	prev = dataFolderButton = new Button(prev->Below(Point(0, 17)), Point(Button::AUTOSIZE, Button::AUTOSIZE), "Open Data Folder");
	dataFolderButton->SetCallback([&](int mb) { this->DataFolderClicked(); });
	scrollArea->AddComponent(dataFolderButton);

	if (!Platform::sharedCwd.empty())
	{
		migrationButton = new Button(dataFolderButton->GetPosition(), Point(Button::AUTOSIZE, Button::AUTOSIZE), "Migrate to shared data directory");
		migrationButton->SetPosition(Point(size.X - migrationButton->GetSize().X - 17, migrationButton->GetPosition().Y));
		migrationButton->SetCallback([&](int mb) { this->MigrationClicked(); });
		scrollArea->AddComponent(migrationButton);
	}

	scrollArea->SetScrollSize(prev->Below(Point(0, 5)).Y);
#else
	scrollArea->SetScrollSize(descLabel->Below(Point(0, 5)).Y);
#endif

#ifndef TOUCHUI
	Button *okButton = new Button(Point(0, this->size.Y - okButtonHeight), Point(this->size.X+1, okButtonHeight), "OK");
#else
	Button *okButton = new Button(Point(0, this->size.Y - okButtonHeight), Point(this->size.X+1, okButtonHeight), "OK");
#endif
	okButton->SetCloseButton(true);
	this->AddComponent(okButton);

	InitializeOptions();
}

void OptionsUI::InitializeOptions()
{
	heatSimCheckbox->SetChecked(!legacy_enable);
	ambientCheckbox->SetChecked(aheat_enable);
	newtonianCheckbox->SetChecked(sim->grav->IsEnabled());
	waterEqalizationCheckbox->SetChecked(water_equal_test);

	airSimDropdown->SetSelectedOption(sim->air->airMode);
	UpdateAmbientAirTempPreview(sim->air->GetAmbientAirTemp(), true);
	airTempTextbox->SetText(Format::TemperatureToString(sim->air->GetAmbientAirTemp(), sim->temperatureScale));
	gravityDropdown->SetSelectedOption(sim->gravityMode);
	edgeModeDropdown->SetSelectedOption(sim->GetEdgeMode());
	decoSpaceDropdown->SetSelectedOption(sim->decoSpace);
	temperatureScaleDropdown->SetSelectedOption(sim->temperatureScale);

#ifdef TOUCHUI
	decorationCheckbox->SetChecked(decorations_enable);
#else
	scaleDropdown->SetSelectedOption(Engine::Ref().GetScale() - 1);
	resizableCheckbox->SetChecked(Engine::Ref().IsResizable());
	filteringDropdown->SetSelectedOption(Engine::Ref().GetPixelFilteringMode());
	filteringDropdown->SetEnabled(resizableCheckbox->IsChecked());
	fullscreenCheckbox->SetChecked(Engine::Ref().IsFullscreen());
	altFullscreenCheckbox->SetChecked(Engine::Ref().IsAltFullscreen());
	forceIntegerScalingCheckbox->SetChecked(Engine::Ref().IsForceIntegerScaling());
#endif

#ifndef TOUCHUI
	fastQuitCheckbox->SetChecked(Engine::Ref().IsFastQuit());
	momentumScrollingCheckbox->SetChecked(Engine::Ref().IsMomentumScroll());
	stickyCategoriesCheckbox->SetChecked(stickyCategories);
#endif
	updatesCheckbox->SetChecked(doUpdates);
	savePressureCheckbox->SetChecked(sim->includePressure);
	circleCheckbox->SetChecked(perfectCircleBrush);
	graveExitsConsole->SetChecked(::graveExitsConsole);
#ifndef ANDROID
	incompatibleCheckbox->SetChecked(loadIncompatibleSaves);
#endif
}

void OptionsUI::HeatSimChecked(bool checked)
{
	legacy_enable = !legacy_enable;
}

void OptionsUI::AmbientChecked(bool checked)
{
	aheat_enable = checked;
}

void OptionsUI::NewtonianChecked(bool checked)
{
	if (checked)
		sim->grav->StartAsync();
	else
		sim->grav->StopAsync();
}

void OptionsUI::DecorationsChecked(bool checked)
{
	decorations_enable = checked;
}

void OptionsUI::WaterEqualizationChecked(bool checked)
{
	water_equal_test = checked;
}

void OptionsUI::AirSimSelected(unsigned int option)
{
	sim->air->airMode = option;
}

void OptionsUI::UpdateAirTemp(std::string temp, bool isDefocus)
{
	float airTemp;
	bool isValid;
	try
	{
		airTemp = Format::StringToTemperature(temp, sim->temperatureScale);
		isValid = true;
	}
	catch (const std::exception & e)
	{
		isValid = false;
	}

	// While defocusing, correct out of range temperatures and empty textboxes
	if (isDefocus)
	{
		if (temp.empty())
		{
			isValid = true;
			airTemp = R_TEMP + 273.15;
		}
		else if (!isValid)
		{
			isValid = true;
			airTemp = sim->air->GetAmbientAirTempPref();
		}
		else if (airTemp < MIN_TEMP)
			airTemp = MIN_TEMP;
		else if (airTemp > MAX_TEMP)
			airTemp = MAX_TEMP;
		//else
		//	return;

		// Update textbox with the new value
		airTempTextbox->SetText(Format::TemperatureToString(airTemp, sim->temperatureScale));
	}
	// Out of range temperatures are invalid, preview should go away
	else if (airTemp < MIN_TEMP || airTemp > MAX_TEMP)
		isValid = false;

	// If valid, set temp
	if (isValid)
		sim->air->SetAmbientAirTempPref(airTemp);

	UpdateAmbientAirTempPreview(airTemp, isValid);
}

void OptionsUI::UpdateAmbientAirTempPreview(float airTemp, bool isValid)
{
	if (isValid)
	{
		pixel color = HeatToColor(airTemp);
		airTempDisplay->SetBackgroundColor(color);
		airTempDisplay->SetText("");
	}
	else
	{
		airTempDisplay->SetBackgroundColor(0);
		airTempDisplay->SetText("?");
	}
}

void OptionsUI::GravitySelected(unsigned int option)
{
	sim->gravityMode = option;
	if (option == GRAV_CUSTOM)
	{
		Engine::Ref().ShowWindow(new GravityWindow(sim, 0.05f, 40));
	}
}

void OptionsUI::EdgeModeSelected(unsigned int option)
{
	sim->SetEdgeMode(option);
}

void OptionsUI::DecoSpaceSelected(unsigned int option)
{
	sim->decoSpace = option;
}

void OptionsUI::TemperatureScaleSelected(unsigned int option)
{
	sim->temperatureScale = option;
	airTempTextbox->SetText(Format::TemperatureToString(sim->air->GetAmbientAirTemp(), option));
}

void OptionsUI::ScaleSelected(unsigned int option)
{
	Engine::Ref().SetScale(option + 1);
}

void OptionsUI::ResizableChecked(bool checked)
{
	Engine::Ref().SetResizable(checked, true);
	filteringDropdown->SetEnabled(checked);
}

void OptionsUI::FilteringSelected(unsigned int option)
{
	Engine::Ref().SetPixelFilteringMode(option, true);
}

void OptionsUI::FullscreenChecked(bool checked)
{
	Engine::Ref().SetFullscreen(checked);
}

void OptionsUI::AltFullscreenChecked(bool checked)
{
	Engine::Ref().SetAltFullscreen(checked);
}

void OptionsUI::ForceIntegerScalingChecked(bool checked)
{
	Engine::Ref().SetForceIntegerScaling(checked);
}

void OptionsUI::FastQuitChecked(bool checked)
{
	Engine::Ref().SetFastQuit(checked);
}

void OptionsUI::UpdatesChecked(bool checked)
{
	doUpdates = checked;
}

void OptionsUI::SavePressureChecked(bool checked)
{
	sim->includePressure = checked;
}

void OptionsUI::MomentumChecked(bool checked)
{
	Engine::Ref().SetMomentumScroll(checked);
}

void OptionsUI::StickyCatsChecked(bool checked)
{
	stickyCategories = checked;
}

void OptionsUI::CircleChecked(bool checked)
{
	perfectCircleBrush = checked;
}

void OptionsUI::GraveChecked(bool checked)
{
	::graveExitsConsole = checked;
}

void OptionsUI::IncompatibleChecked(bool checked)
{
	loadIncompatibleSaves = checked;
}

void OptionsUI::DataFolderClicked()
{
#ifdef WIN
	const char *openCommand = "explorer ";
#elif MACOSX
	const char *openCommand = "open ";
//#elif LIN
#else
	const char *openCommand = "xdg-open ";
#endif
	char *workingDirectory = new char[FILENAME_MAX + strlen(openCommand)];
	sprintf(workingDirectory, "%s\"%s\"", openCommand, getcwd(NULL, 0));
	int ret = system(workingDirectory);
	if (ret)
		std::cout << "Error, could not open data directory" << std::endl;
	delete[] workingDirectory;
}

void OptionsUI::MigrationClicked()
{
	std::string from = Platform::originalCwd;
	std::string to = Platform::sharedCwd;

	std::string message = "This will migrate all stamps, saves, and scripts from\n\bt" + from + "\bw\nto the shared data directory at\n\bt" + to + "\bw\n\n" +
						  "Files that already exist will not be overwritten.";
	auto prompt = new ConfirmPrompt("Do Migration?", message);
	prompt->SetCallback({ [from, to](bool confirmed) {
		if (confirmed)
		{
			std::string ret = Platform::DoMigration(from, to);
			Engine::Ref().ShowWindow(new InfoPrompt("Migration complete", ret));
		}
	} });
	Engine::Ref().ShowWindow(prompt);
}

void OptionsUI::OnDraw(gfx::VideoBuffer *buf)
{
	buf->DrawLine(0, scrollArea->GetPosition().Y - 1, size.X, scrollArea->GetPosition().Y - 1, 200, 200, 200, 255);

#ifndef TOUCHUI
	if (filteringDropdown->IsSelectingOption())
	{
		resizableLabel->SetText("These options make TPT appear extremely blurry");
		resizableLabel->SetColor(COLRGB(255, 0, 0));
	}
	else
	{
		resizableLabel->SetText("Allow resizing window");
		resizableLabel->SetColor(COLRGB(150, 150, 150));
	}
#endif
}

void OptionsUI::OnSubwindowDraw(gfx::VideoBuffer *buf)
{
	buf->DrawLine(0, temperatureScaleDropdown->Below(Point(0, 5)).Y, scrollArea->GetUsableWidth() - 1, temperatureScaleDropdown->Below(Point(0, 5)).Y, 200, 200, 200, 255);
#ifndef TOUCHUI
	buf->DrawLine(0, altFullscreenCheckbox->Below(Point(0, 17)).Y, scrollArea->GetUsableWidth() - 1, altFullscreenCheckbox->Below(Point(0, 17)).Y, 200, 200, 200, 255);
#endif
}

void OptionsUI::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
#ifndef TOUCHUI
	if ((codeStep == 0 || codeStep == 1) && key == SDLK_UP)
		codeStep++;
	else if ((codeStep == 2 || codeStep == 3) && key == SDLK_DOWN)
		codeStep++;
	else if ((codeStep == 4 || codeStep == 6) && key == SDLK_LEFT)
		codeStep++;
	else if ((codeStep == 5 || codeStep == 7) && key == SDLK_RIGHT)
		codeStep++;
	else
		codeStep = 0;
	if (codeStep == 8)
	{
		filteringDropdown->SetVisible(true);
		filteringLabel->SetVisible(true);

		forceIntegerScalingCheckbox->SetVisible(false);
		forceIntegerScalingLabel->SetVisible(false);
	}
#endif
}
