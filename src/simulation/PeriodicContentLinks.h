#pragma once

#include <cstdint>
#include <span>
#include <string_view>

enum class PeriodicContentKind
{
	PeriodicElement,
	Isotope,
	InorganicCompound,
	RelatedMaterial,
};

enum class PeriodicCompoundGroup
{
	Element,
	Allotrope,
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
	Organic,
	Polymer,
	Alloy,
	Mineral,
	Ceramic,
	Glass,
	Semiconductor,
	Composite,
	Engineering,
	Other,
};

struct PeriodicContentLink
{
	std::string_view toolIdentifier;
	int stableId;
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
	std::string_view relationBasis;
	std::string_view module;
	std::string_view sourceReference;
	std::string_view confidence;
	std::string_view notes;
};

std::span<PeriodicContentLink const> GetPeriodicContentLinks();
bool PeriodicContentRelatesTo(PeriodicContentLink const &link, int atomicNumber);
PeriodicContentLink const *FindPeriodicContentLink(std::string_view identifier);
