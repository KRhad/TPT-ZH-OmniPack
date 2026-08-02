#pragma once

#include <cstdint>
#include <span>
#include <string_view>

enum class PeriodicContentKind
{
	PeriodicElement,
	Isotope,
	InorganicCompound,
};

enum class PeriodicCompoundGroup
{
	Element,
	Isotope,
	Oxide,
	Hydroxide,
	Acid,
	Base,
	Salt,
	Halide,
	Sulfide,
	Nitride,
	Carbide,
	Hydride,
	Other,
};

struct PeriodicContentLink
{
	std::string_view toolIdentifier;
	PeriodicContentKind contentKind;
	int primaryAtomicNumber;
	std::uint64_t relatedMaskLow;
	std::uint64_t relatedMaskHigh;
	PeriodicCompoundGroup compoundGroup;
	int displayOrder;
	bool periodicVisible;
	std::string_view formula;
	std::string_view chineseName;
	std::string_view englishName;
};

std::span<PeriodicContentLink const> GetPeriodicContentLinks();
bool PeriodicContentRelatesTo(PeriodicContentLink const &link, int atomicNumber);
PeriodicContentLink const *FindPeriodicContentLink(std::string_view identifier);
