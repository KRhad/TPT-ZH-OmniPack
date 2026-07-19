#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <string>
#include "Component.h"

namespace gfx
{
class VideoBuffer;
}
class ProgressBar : public Component
{
	int progress = 0;
	std::string status;

public:
	ProgressBar(Point position, Point size);

	int GetProgress();
	void SetProgress(int progress);

	std::string GetStatus() { return status; }
	void SetStatus(std::string status) { this->status = status; }
	void OnDraw(gfx::VideoBuffer* vid) override;
};

#endif // PROGRESSBAR_H
