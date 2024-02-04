#pragma once
#include <map>
#include <set>
#include <string>

struct MissingElements
{
	std::map<std::string, int> identifiers;
	std::set<int> ids;

	operator bool() const
	{
		return identifiers.size() || ids.size();
	}
};
