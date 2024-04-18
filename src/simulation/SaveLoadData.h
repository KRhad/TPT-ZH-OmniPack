#pragma once
#include <map>
#include <set>
#include <string>
#include <json/json.h>

struct SaveLoadData
{
	std::map<std::string, int> identifiers;
	std::set<int> ids;
	Json::Value authors;

	bool isMissingElements() const
	{
		return identifiers.size() || ids.size();
	}
};
