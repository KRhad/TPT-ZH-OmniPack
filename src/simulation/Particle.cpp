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
	{ "dcolor" , StructProperty::UInteger    , (intptr_t)(offsetof(particle, dcolour)) },
};

std::vector<StructProperty> const &particle::GetProperties()
{
	return properties;
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

int Particle_GetOffset(const char * key, int * format)
{
	int offset;
	if (!strcmp(key, "type"))
	{
		offset = offsetof(particle, type);
		*format = 2;
	}
	else if (!strcmp(key, "life"))
	{
		offset = offsetof(particle, life);
		*format = 0;
	}
	else if (!strcmp(key, "ctype"))
	{
		offset = offsetof(particle, ctype);
		*format = 0;
	}
	else if (!strcmp(key, "temp"))
	{
		offset = offsetof(particle, temp);
		*format = 1;
	}
	else if (!strcmp(key, "tmp"))
	{
		offset = offsetof(particle, tmp);
		*format = 0;
	}
	else if (!strcmp(key, "tmp2"))
	{
		offset = offsetof(particle, tmp2);
		*format = 0;
	}
	else if (!strcmp(key, "vy"))
	{
		offset = offsetof(particle, vy);
		*format = 1;
	}
	else if (!strcmp(key, "vx"))
	{
		offset = offsetof(particle, vx);
		*format = 1;
	}
	else if (!strcmp(key, "x"))
	{
		offset = offsetof(particle, x);
		*format = 1;
	}
	else if (!strcmp(key, "y")) {
		offset = offsetof(particle, y);
		*format = 1;
	}
	else if (!strcmp(key, "dcolour"))
	{
		offset = offsetof(particle, dcolour);
		*format = 3;
	}
	else if (!strcmp(key, "dcolor"))
	{
		offset = offsetof(particle, dcolour);
		*format = 3;
	}
	else if (!strcmp(key, "flags"))
	{
		offset = offsetof(particle, flags);
		*format = 3;
	}
	else if (!strcmp(key, "tmp3"))
	{
		offset = offsetof(particle, tmp3);
		*format = 0;
	}
	else if (!strcmp(key, "tmp4"))
	{
		offset = offsetof(particle, tmp4);
		*format = 0;
	}
	else
	{
		offset = -1;
	}
	return offset;
}
