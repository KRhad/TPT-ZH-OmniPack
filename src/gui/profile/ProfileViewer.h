#ifndef PROFILEVIEWER_H
#define PROFILEVIEWER_H

#include <string>
#include "interface/Window.h"
#include "graphics/Pixel.h"

class Request;
class Label;
class Button;
class Textbox;
namespace ui
{
	class ScrollWindow;
}
class ProfileViewer : public ui::Window
{
	std::string name;
	Request *profileInfoDownload;
	Request *avatarDownload;
	Request *profileSaveDownload;
	pixel *avatar;

	ui::ScrollWindow *scrollArea;

	Label *usernameLabel, *ageLabel, *locationLabel, *websiteLabel, *biographyLabel;
	Label *saveCountLabel, *saveAverageLabel, *highestVoteLabel;
	Button *enableEditingButton, *openProfileButton, *avatarUploadButton;

public:
	ProfileViewer(std::string profileName);
	~ProfileViewer();

	void OnTick(uint32_t ticks) override;
	void OnDrawAfterSubwindows(gfx::VideoBuffer *buf) override;

	// callback functions for all the buttons
	void EnableEditing();
	void SaveProfile();
	void OpenProfile();
	void OpenProfileEdit();
	void UploadAvatar();
	void ResizeArea(int biographyLabelHeight);
};

#endif
