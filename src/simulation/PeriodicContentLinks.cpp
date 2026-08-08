#include "PeriodicContentLinks.h"

bool PeriodicContentRelatesTo(PeriodicContentLink const &link, int atomicNumber)
{
	if (atomicNumber < 1 || atomicNumber > 118)
		return false;
	if (atomicNumber <= 64)
		return (link.relatedMaskLow & (UINT64_C(1) << (atomicNumber - 1))) != 0;
	return (link.relatedMaskHigh & (UINT64_C(1) << (atomicNumber - 65))) != 0;
}

PeriodicContentLink const *FindPeriodicContentLink(std::string_view identifier)
{
	for (auto const &link : GetPeriodicContentLinks())
		if (link.toolIdentifier == identifier)
			return &link;
	return nullptr;
}
