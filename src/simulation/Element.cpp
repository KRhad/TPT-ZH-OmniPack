/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstring>
#include "simulation/Element.h"
#include "simulation/Simulation.h"

#include "powder.h"

Element::Element():
	Identifier(""),
	Name(""),
	Colour(COLPACK(0xFF00FF)),
	MenuVisible(0),
	MenuSection(0),
	Enabled(0),
	Advection(0.0f),
	AirDrag(0.0f),
	AirLoss(1.00f),
	Loss(1.00f),
	Collision(0.0f),
	Gravity(0.0f),
	NewtonianGravity(1.0f),
	Diffusion(0.00f),
	HotAir(0.00f),
	Falldown(0),
	Flammable(0),
	Explosive(0),
	Meltable(0),
	Hardness(30),
	PhotonReflectWavelengths(0x3FFFFFFF),
	Weight(50),
	HeatConduct(128),
	Latent(0),
	Description("No Description."),
	Properties(TYPE_SOLID),
	CarriesTypeIn(0),
	LowPressureTransitionThreshold(IPL),
	LowPressureTransitionElement(NT),
	HighPressureTransitionThreshold(IPH),
	HighPressureTransitionElement(NT),
	LowTemperatureTransitionThreshold(ITL),
	LowTemperatureTransitionElement(NT),
	HighTemperatureTransitionThreshold(ITH),
	HighTemperatureTransitionElement(NT)
{
	memset(&DefaultProperties, 0, sizeof(particle));
	DefaultProperties.temp = R_TEMP + 273.15f;
}

std::vector<StructProperty> Element::properties = {
	{ "Name",                      StructProperty::String,          offsetof(Element, Name)                      },
	{ "Colour",                    StructProperty::Colour,          offsetof(Element, Colour)                    },
	{ "Color",                     StructProperty::Colour,          offsetof(Element, Colour)                    },
	{ "MenuVisible",               StructProperty::Integer,         offsetof(Element, MenuVisible)               },
	{ "MenuSection",               StructProperty::Integer,         offsetof(Element, MenuSection)               },
	{ "Enabled",                   StructProperty::Integer,         offsetof(Element, Enabled)                   },
	{ "Advection",                 StructProperty::Float,           offsetof(Element, Advection)                 },
	{ "AirDrag",                   StructProperty::Float,           offsetof(Element, AirDrag)                   },
	{ "AirLoss",                   StructProperty::Float,           offsetof(Element, AirLoss)                   },
	{ "Loss",                      StructProperty::Float,           offsetof(Element, Loss)                      },
	{ "Collision",                 StructProperty::Float,           offsetof(Element, Collision)                 },
	{ "NewtonianGravity",          StructProperty::Float,           offsetof(Element, NewtonianGravity)          },
	{ "Gravity",                   StructProperty::Float,           offsetof(Element, Gravity)                   },
	{ "Diffusion",                 StructProperty::Float,           offsetof(Element, Diffusion)                 },
	{ "HotAir",                    StructProperty::Float,           offsetof(Element, HotAir)                    },
	{ "Falldown",                  StructProperty::Integer,         offsetof(Element, Falldown)                  },
	{ "Flammable",                 StructProperty::Integer,         offsetof(Element, Flammable)                 },
	{ "Explosive",                 StructProperty::Integer,         offsetof(Element, Explosive)                 },
	{ "Meltable",                  StructProperty::Integer,         offsetof(Element, Meltable)                  },
	{ "Hardness",                  StructProperty::Integer,         offsetof(Element, Hardness)                  },
	{ "PhotonReflectWavelengths",  StructProperty::UInteger,        offsetof(Element, PhotonReflectWavelengths)  },
	{ "Weight",                    StructProperty::Integer,         offsetof(Element, Weight)                    },
	{ "Temperature",               StructProperty::Float,           offsetof(Element, DefaultProperties.temp)    },
	{ "HeatConduct",               StructProperty::UChar,           offsetof(Element, HeatConduct)               },
	{ "Description",               StructProperty::String,          offsetof(Element, Description)               },
	{ "State",                     StructProperty::Removed,         0                                            },
	{ "Properties",                StructProperty::UInteger,        offsetof(Element, Properties)                },
	{ "CarriesTypeIn",             StructProperty::UInteger,        offsetof(Element, CarriesTypeIn)             },
	{ "LowPressure",               StructProperty::Float,           offsetof(Element, LowPressureTransitionThreshold)     },
	{ "LowPressureTransition",     StructProperty::TransitionType,  offsetof(Element, LowPressureTransitionElement)       },
	{ "HighPressure",              StructProperty::Float,           offsetof(Element, HighPressureTransitionThreshold)    },
	{ "HighPressureTransition",    StructProperty::TransitionType,  offsetof(Element, HighPressureTransitionElement)      },
	{ "LowTemperature",            StructProperty::Float,           offsetof(Element, LowTemperatureTransitionThreshold)  },
	{ "LowTemperatureTransition",  StructProperty::TransitionType,  offsetof(Element, LowTemperatureTransitionElement)    },
	{ "HighTemperature",           StructProperty::Float,           offsetof(Element, HighTemperatureTransitionThreshold) },
	{ "HighTemperatureTransition", StructProperty::TransitionType,  offsetof(Element, HighTemperatureTransitionElement)   }
};

std::vector<StructProperty> const &Element::GetProperties()
{
	return properties;
}
