#pragma once
#include <map>
#include <set>
#include <string>

struct MissingElements
{
	std::map<std::string, int> identifiers;
	std::set<int> ids;
};
