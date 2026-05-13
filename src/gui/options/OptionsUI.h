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
	Dropdown *airSimDropdown, *convectionModeDropdown, *gravityDropdown, *edgeModeDropdown, *decoSpaceDropdown, *temperatureScaleDropdown;
	Textbox *airTempTextbox, *edgePressureTextbox, *vorticityCoeffTextbox;
	Button *airTempDisplay, *edgePressureDisplay, *edgeVelocityButton, *edgeVelocityDisplay;

	Dropdown *scaleDropdown;
	Label *resizableLabel, *filteringLabel, *forceIntegerScalingLabel;
	Checkbox *resizableCheckbox, *fullscreenCheckbox, *altFullscreenCheckbox;
	Checkbox *forceIntegerScalingCheckbox;
	Dropdown *filteringDropdown;

	Checkbox *fastQuitCheckbox, *globalQuitCheckbox, *updatesCheckbox, *momentumScrollingCheckbox, *stickyCategoriesCheckbox, *savePressureCheckbox;
	Checkbox *circleCheckbox, *graveExitsConsole, *incompatibleCheckbox;
	Button *dataFolderButton, *migrationButton;
	Checkbox *redirectStdCheckbox;

	Simulation * sim;

	int codeStep = 0;

	void InitializeOptions();
	void HeatSimChecked(bool checked);
	void AmbientChecked(bool checked);
	void NewtonianChecked(bool checked);
	void DecorationsChecked(bool checked);
	void WaterEqualizationChecked(bool checked);
	void AirSimSelected(unsigned int option);
	void ConvectionModeSelected(unsigned int option);
	void GravitySelected(unsigned int option);
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
	void GlobalQuitChecked(bool checked);
	void UpdatesChecked(bool checked);
	void SavePressureChecked(bool checked);
	void MomentumChecked(bool checked);
	void StickyCatsChecked(bool checked);
	void CircleChecked(bool checked);
	void GraveChecked(bool checked);
	void IncompatibleChecked(bool checked);
	void RedirectChecked(bool checked);
	void DataFolderClicked();
	void MigrationClicked();

	void UpdateAirTemp(std::string temp, bool isDefocus);
	void UpdateAmbientAirTempPreview(float airTemp, bool isValid);
	void UpdateEdgePressure(std::string edgePres, bool isDefocus);
	void UpdateEdgePressurePreview(float edgePres, bool isValid);
	void EdgeVelocityClicked();
	void UpdateEdgeVelocityPreview(float edgeVelocityX, float edgeVelocityY, bool isValid);
	void UpdateVorticityCoeff(std::string temp, bool isDefocus);
	void VorticityCoeffToTextbox(float vorticity);
	void EdgePressureToTextbox(float pressure);


	void OnDraw(gfx::VideoBuffer *buf) override;
	void OnDrawAfterSubwindows(gfx::VideoBuffer *buf) override;
	void OnSubwindowDraw(gfx::VideoBuffer *buf);
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;

public:
	OptionsUI(Simulation * sim);
};

#endif
