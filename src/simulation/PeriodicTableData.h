#pragma once

#include <span>
#include <string_view>

enum class PeriodicStandardState
{
	Solid,
	Liquid,
	Gas,
	Unknown,
};

enum class PeriodicMaterialClass
{
	Metal,
	Metalloid,
	Nonmetal,
	Unknown,
};

struct PeriodicElementRecord
{
	int atomicNumber;
	std::string_view symbol;
	std::string_view chineseName;
	std::string_view englishName;
	std::string_view identifier;
	int stableId;
	int period;
	int group;
	int tableRow;
	int tableColumn;
	std::string_view family;
	std::string_view series;
	PeriodicStandardState standardState;
	PeriodicMaterialClass materialClass;
	bool radioactive;
	bool implemented;
};

std::span<PeriodicElementRecord const> GetPeriodicTableData();
PeriodicElementRecord const *FindPeriodicElementByAtomicNumber(int atomicNumber);
PeriodicElementRecord const *FindPeriodicElementByStableId(int stableId);
