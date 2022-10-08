#include <algorithm>
#include <cstring>
#include <cstddef>
#include "Particle.h"

std::vector<StructProperty> particle::properties = {
	{ "type"   , StructProperty::ParticleType, (intptr_t)(offsetof(particle, type   )) },
	{ "life"   , StructProperty::Integer     , (intptr_t)(offsetof(particle, life   )) },
	{ "ctype"  , StructProperty::ParticleType, (intptr_t)(offsetof(particle, ctype  )) },
	{ "x"      , StructProperty::Float       , (intptr_t)(offsetof(particle, x      )) },
	{ "y"      , StructProperty::Float       , (intptr_t)(offsetof(particle, y      )) },
	{ "vx"     , StructProperty::Float       , (intptr_t)(offsetof(particle, vx     )) },
	{ "vy"     , StructProperty::Float       , (intptr_t)(offsetof(particle, vy     )) },
	{ "temp"   , StructProperty::Float       , (intptr_t)(offsetof(particle, temp   )) },
	{ "flags"  , StructProperty::UInteger    , (intptr_t)(offsetof(particle, flags  )) },
	{ "tmp"    , StructProperty::Integer     , (intptr_t)(offsetof(particle, tmp    )) },
	{ "tmp2"   , StructProperty::Integer     , (intptr_t)(offsetof(particle, tmp2   )) },
	{ "tmp3"   , StructProperty::Integer     , (intptr_t)(offsetof(particle, tmp3   )) },
	{ "tmp4"   , StructProperty::Integer     , (intptr_t)(offsetof(particle, tmp4   )) },
	{ "dcolour", StructProperty::UInteger    , (intptr_t)(offsetof(particle, dcolour)) },
};

std::vector<StructProperty> const &particle::GetProperties()
{
	return properties;
}

std::vector<StructPropertyAlias> const &particle::GetPropertyAliases()
{
	static std::vector<StructPropertyAlias> aliases = {
		{ "pavg0" , "tmp3"    },
		{ "pavg1" , "tmp4"    },
		{ "dcolor", "dcolour" },
	};
	return aliases;
}

StructProperty particle::PropertyByName(const std::string& Name)
{
	auto prop = std::find_if(properties.begin(), properties.end(), [&](StructProperty prop) {
		return prop.Name == Name;
	});
	if (prop == properties.end())
		return properties[0];
	return *prop;
}

int Particle_GetOffset(std::string key, int * format)
{
	int offset = -1;
	for (auto &alias : particle::GetPropertyAliases())
	{
		if (key == alias.from)
		{
			key = alias.to;
		}
	}
	for (auto &prop : particle::GetProperties())
	{
		if (key == prop.Name)
		{
			offset = prop.Offset;
			switch (prop.Type)
			{
			case StructProperty::ParticleType:
				*format = (key == "type") ? 2 : 0; // FormatElement is tightly coupled with "type"
				break;

			case StructProperty::Integer:
			case StructProperty::UInteger:
				*format = 3;
				break;

			case StructProperty::Float:
				*format = 1;
				break;

			default:
				break;
			}
		}
	}
	return offset;
}
