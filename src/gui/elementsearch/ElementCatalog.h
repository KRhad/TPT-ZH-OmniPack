#pragma once

#include <span>
#include <string_view>

struct ElementCatalogRecord
{
	std::string_view identifier;
	std::string_view displayCode;
	std::string_view englishName;
	std::string_view chineseName;
	std::string_view sourceMod;
	std::string_view sourceId;
	int stableId;
	std::string_view menuCategory;
	std::string_view elementState;
	std::string_view defaultEnabled;
	std::string_view duplicateOf;
	std::string_view saveCompatibility;
	std::string_view implementationStatus;
	std::string_view testStatus;
	std::string_view sourceCommit;
	std::string_view englishDescription;
	std::string_view chineseDescription;
	std::string_view license;
	std::string_view notes;
};

std::span<ElementCatalogRecord const> GetElementCatalog();
ElementCatalogRecord const *FindElementCatalogByIdentifier(std::string_view identifier);
ElementCatalogRecord const *FindElementCatalogByStableId(int stableId);
