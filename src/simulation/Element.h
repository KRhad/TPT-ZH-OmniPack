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

#ifndef ELEMENT_H
#define ELEMENT_H

#include "graphics/ARGBColour.h"
#include "simulation/Particle.h"
#include <string>

class Simulation;

#define UPDATE_FUNC_ARGS Simulation *sim, int i, int x, int y, int surround_space, int nt
#define UPDATE_FUNC_SUBCALL_ARGS sim, i, x, y, surround_space, nt
#define GRAPHICS_FUNC_ARGS Simulation *sim, particle *cpart, int nx, int ny, int *pixel_mode, int* cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb
#define GRAPHICS_FUNC_SUBCALL_ARGS sim, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb
#define ELEMENT_CREATE_FUNC_ARGS Simulation *sim, int i, int x, int y, int t, int v
#define ELEMENT_CREATE_ALLOWED_FUNC_ARGS Simulation *sim, int i, int x, int y, int t
#define ELEMENT_CHANGETYPE_FUNC_ARGS Simulation *sim, int i, int x, int y, int from, int to
#define ELEMENT_INIT_FUNC_ARGS Simulation *sim, Element *elem, int t
#define CTYPEDRAW_FUNC_ARGS Simulation *sim, int i, int t, int v
#define CTYPEDRAW_FUNC_SUBCALL_ARGS sim, i, t, v


class Element
{
public:
	std::string Identifier;
	std::string Name;
	ARGBColour Colour;
	int MenuVisible;
	int MenuSection;
	int Enabled;

	float Advection;
	float AirDrag;
	float AirLoss;
	float Loss;
	float Collision;
	float Gravity;
	float NewtonianGravity;
	float Diffusion;
	float HotAir;
	int Falldown;

	int Flammable;
	int Explosive;
	int Meltable;
	int Hardness;
	// Photon wavelengths are ANDed with this value when a photon hits an element, meaning that only wavelengths present in both this value and the original photon will remain in the reflected photon
	unsigned int PhotonReflectWavelengths;

	/* Weight Help
	 * 1   = Gas   ||
	 * 2   = Light || Liquids  0-49
	 * 98  = Heavy || Powder  50-99
	 * 100 = Solid ||
	 * -1 is Neutrons and Photons
	 */
	int Weight;

	unsigned char HeatConduct;
	// Latent value is in TPT imaginary units - 750/226*enthalpy value of the material
	unsigned int Latent;
	std::string Description;

	unsigned int Properties;
	unsigned int CarriesTypeIn;

	float LowPressureTransitionThreshold;
	int LowPressureTransitionElement;
	float HighPressureTransitionThreshold;
	int HighPressureTransitionElement;
	float LowTemperatureTransitionThreshold;
	int LowTemperatureTransitionElement;
	float HighTemperatureTransitionThreshold;
	int HighTemperatureTransitionElement;

	int (*Update) (UPDATE_FUNC_ARGS) = nullptr;
	int (*Graphics) (GRAPHICS_FUNC_ARGS) = nullptr;

	// Func_Create can be used to set initial properties that are not constant (e.g. a random life value)
	// It cannot be used to block creation, to do that use Func_Create_Allowed
	// Particle type should not be changed in this function
	void (*Func_Create)(ELEMENT_CREATE_FUNC_ARGS) = nullptr;

	// Func_Create_Allowed is used to check whether a particle can be created, by both Simulation::part_create and Simulation::part_change_type
	// Arguments are the same as Simulation::part_create or Simulation::part_change_type
	// This function should not modify the particle
	// Before calling this, coordinates and particle type are checked, but not eval_move()
	bool (*Func_Create_Allowed)(ELEMENT_CREATE_ALLOWED_FUNC_ARGS) = nullptr;

	// Func_ChangeType is called by Simulation::part_create, Simulation::part_change_type, and Simulation::part_kill
	// It should be used for things such as setting STKM legs and allocating/freeing a fighters[] slot
	// For part_create and part_change_type, it is called at the end of the function, after the pmap and all the properties and element counts are set
	// For part_kill, it is called at the start of the function, before modifying particle properties or removing it from the pmap
	void (*Func_ChangeType)(ELEMENT_CHANGETYPE_FUNC_ARGS) = nullptr;

	// Ctype draw is called by Simulation::part_create, and handles putting the selected element into this element's ctype
	// Default ctype draw functions are provided in ElementFunctions.cpp, this will ignore selected elements with PROP_NOCTYPEDRAW set
	bool (*CtypeDraw)(CTYPEDRAW_FUNC_ARGS) = nullptr;

	particle DefaultProperties;

	void (*Init) (ELEMENT_INIT_FUNC_ARGS) = nullptr;
	Element();

	/** Returns a list of properties, their type, and offset within the Element structure, for use in Lua **/
	static std::vector<StructProperty> const &GetProperties();

private:
	static std::vector<StructProperty> properties;
};

//defined in ElementFunctions.cpp
int update_legacy_all(UPDATE_FUNC_ARGS);
int graphics_DEFAULT(GRAPHICS_FUNC_ARGS);
int update_POWERED(UPDATE_FUNC_ARGS);

bool basicCtypeDraw(CTYPEDRAW_FUNC_ARGS);
bool ctypeDrawVInTmp(CTYPEDRAW_FUNC_ARGS);
bool ctypeDrawVInCtype(CTYPEDRAW_FUNC_ARGS);

#endif
