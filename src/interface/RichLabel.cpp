#include "RichLabel.h"
#include <algorithm>
#include <sstream>
#include "common/Platform.h"

RichLabel::RichLabel(Point position, Point size, std::string text, bool multiline):
	Label(position, size, "", multiline)
{
	SetText(text);
}

void RichLabel::SetText(std::string newText)
{
	std::vector<RichTextRegion> newRegions;
	std::stringstream sb;
	auto it = newText.begin();
	while (it != newText.end())
	{
		auto find = [&newText](auto it, std::string::value_type ch) {
			while (it != newText.end())
			{
				if (*it == ch)
				{
					break;
				}
				++it;
			}
			return it;
		};
		auto beginRegionIt = find(it, '{');
		auto beginDataIt = find(beginRegionIt, ':');
		auto beginTextIt = find(beginDataIt, '|');
		auto endRegionIt = find(beginTextIt, '}');
		if (endRegionIt == newText.end())
		{
			break;
		}
		auto action = std::string(beginRegionIt + 1, beginDataIt);
		auto data = std::string(beginDataIt + 1, beginTextIt);
		auto text = std::string(beginTextIt + 1, endRegionIt);
		sb << std::string(it, beginRegionIt);
		auto good = false;
		if (action == "a" && data.size() && text.size())
		{
			RichTextRegion region;
			region.begin = sb.str().size();
			sb << text;
			region.end = sb.str().size();
			region.action = RichTextRegion::LinkAction{ data };
			newRegions.push_back(region);
			good = true;
		}
		if (!good)
		{
			sb << std::string(beginRegionIt, endRegionIt + 1);
		}
		it = endRegionIt + 1;
	}
	sb << std::string(it, newText.end());
	auto newDisplayText = sb.str();
	Label::SetText(sb.str());
	regions = newRegions;
}

void RichLabel::OnMouseDown(int x, int y, unsigned char button)
{
	Label::OnMouseDown(x, y, button);

	unsigned int cursorPosition = cursor - std::count(text.begin(), text.begin() + cursor, '\r');
	for (auto const &region : regions)
	{
		if (region.begin <= cursorPosition && region.end > cursorPosition)
		{
			if (auto *linkAction = std::get_if<RichTextRegion::LinkAction>(&region.action))
			{
				Platform::OpenLink(linkAction->uri);
				return;
			}
		}
	}
}
