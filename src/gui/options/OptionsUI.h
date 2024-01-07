#ifndef OPTIONS_H
#define OPTIONS_H
#include <string>
#include "interface/Window.h"

class Button;
class Checkbox;
class Dropdown;
class Label;
class Simulation;
class Textbox;
namespace ui
{
	class ScrollWindow;
}
class OptionsUI : public ui::Window
{
	ui::ScrollWindow *scrollArea;

	Checkbox *heatSimCheckbox, *ambientCheckbox, *newtonianCheckbox, *waterEqalizationCheckbox, *decorationCheckbox;
	Dropdown *airSimDropdown, *gravityDropdown, *edgeModeDropdown, *decoSpaceDropdown, *temperatureScaleDropdown;
	Textbox *airTempTextbox;
	Button *airTempDisplay;

	Dropdown *scaleDropdown;
	Label *resizableLabel, *filteringLabel, *forceIntegerScalingLabel;
	Checkbox *resizableCheckbox, *fullscreenCheckbox, *altFullscreenCheckbox;
	Checkbox *forceIntegerScalingCheckbox;
	Dropdown *filteringDropdown;

	Checkbox *fastQuitCheckbox, *updatesCheckbox, *momentumScrollingCheckbox, *stickyCategoriesCheckbox, *savePressureCheckbox;
	Checkbox *circleCheckbox, *graveExitsConsole, *incompatibleCheckbox;
	Button *dataFolderButton, *migrationButton;

	Simulation * sim;

	int codeStep = 0;

	void InitializeOptions();
	void HeatSimChecked(bool checked);
	void AmbientChecked(bool checked);
	void NewtonianChecked(bool checked);
	void DecorationsChecked(bool checked);
	void WaterEqualizationChecked(bool checked);
	void AirSimSelected(unsigned int option);
	void GravitySelected(unsigned int option);
	unsigned int oldEdgeMode;
	void EdgeModeSelected(unsigned int option);
	void DecoSpaceSelected(unsigned int option);
	void TemperatureScaleSelected(unsigned int option);
	void ScaleSelected(unsigned int option);
	void ResizableChecked(bool checked);
	void FilteringSelected(unsigned int option);
	void FullscreenChecked(bool checked);
	void AltFullscreenChecked(bool checked);
	void ForceIntegerScalingChecked(bool checked);
	void FastQuitChecked(bool checked);
	void UpdatesChecked(bool checked);
	void SavePressureChecked(bool checked);
	void MomentumChecked(bool checked);
	void StickyCatsChecked(bool checked);
	void CircleChecked(bool checked);
	void GraveChecked(bool checked);
	void IncompatibleChecked(bool checked);
	void DataFolderClicked();
	void MigrationClicked();

	void UpdateAirTemp(std::string temp, bool isDefocus);
	void UpdateAmbientAirTempPreview(float airTemp, bool isValid);


	void OnDraw(gfx::VideoBuffer *buf) override;
	void OnSubwindowDraw(gfx::VideoBuffer *buf);
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;

public:
	OptionsUI(Simulation * sim);
};

#endif
