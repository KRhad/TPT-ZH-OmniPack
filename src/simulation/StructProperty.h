#include <utility>

#ifndef STRUCTPROPERTY_H_
#define STRUCTPROPERTY_H_

#include <string>
#include <cstdint>

struct StructProperty
{
	enum PropertyType
	{
		TransitionType,
		ParticleType,
		Colour,
		Integer,
		UInteger,
		Float,
		BString,
		String,
		UChar,
		Removed
	};
	std::string Name;
	PropertyType Type = Integer;
	intptr_t Offset = 0;

	StructProperty(std::string name, PropertyType type, intptr_t offset):
		Name(std::move(name)),
		Type(type),
		Offset(offset)
	{

	}

	StructProperty():
		Name("")
	{

	}
};

union PropertyValue {
	int Integer;
	unsigned int UInteger;
	float Float;
};

struct StructPropertyAlias
{
	std::string from, to;
};

#endif
