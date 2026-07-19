#ifndef RICHLABEL_H
#define RICHLABEL_H

#include "Label.h"
#include <string>
#include <variant>
#include <vector>

class RichLabel : public Label
{
	struct RichTextRegion
	{
		unsigned int begin;
		unsigned int end;
		struct LinkAction
		{
			std::string uri;
		};
		using Action = std::variant<LinkAction>;
		Action action;
	};
	std::vector<RichTextRegion> regions;

public:
	RichLabel(Point position, Point size, std::string text, bool multiline = false);

	void SetText(std::string newText) override;
	void OnMouseDown(int x, int y, unsigned char button) override;
};

#endif // RICHLABEL_H
