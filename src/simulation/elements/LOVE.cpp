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

#include "simulation/ElementsCommon.h"
#include "simulation/ElementDataContainer.h"

static bool loverule[9][9] =
{
	{0,0,1,1,0,1,1,0,0},
	{0,1,0,0,1,0,0,1,0},
	{1,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,1},
	{0,1,0,0,0,0,0,1,0},
	{0,1,0,0,0,0,0,1,0},
	{0,0,1,0,0,0,1,0,0},
	{0,0,0,1,0,1,0,0,0},
	{0,0,0,0,1,0,0,0,0},
};

class LOVE_ElementDataContainer : public ElementDataContainer
{
	bool love[XRES/9][YRES/9];
public:
	LOVE_ElementDataContainer()
	{
		std::fill_n(&love[0][0], (XRES/9)*(YRES/9), false);
	}

	std::unique_ptr<ElementDataContainer> Clone() override { return std::make_unique<LOVE_ElementDataContainer>(*this); }

	void Simulation_BeforeUpdate(Simulation *sim) override
	{
		if (sim->elementCount[PT_LOVE] <= 0)
			return;
		//set love to true in any grid space with an LOVE in it
		std::fill_n(&love[0][0], (XRES/9)*(YRES/9), false);
		for (int x = 9; x < ((XRES-4)/9)*9; x++)
			for (int y = 9; y < (int)((YRES-4)/9)*9; y++)
				if (pmap[y][x] && parts[ID(pmap[y][x])].type == PT_LOVE)
					love[x/9][y/9] = true;

		//create the correct pattern in any grid space that had LOVE
		for (int x = 9; x < ((XRES-4)/9)*9; x+=9)
			for (int y = 9; y < (int)((YRES-4)/9)*9; y+=9)
				if (love[x/9][y/9])
					for (int nx = 0; nx < 9; nx++)
						for (int ny = 0; ny < 9; ny++)
						{
							int rt = pmap[y+ny][x+nx];
							if (!rt && loverule[ny][nx])
								sim->part_create(-1, x+nx, y+ny, PT_LOVE);
							else if (!rt)
								continue;
							else if (parts[ID(rt)].type == PT_LOVE && !loverule[ny][nx])
								sim->part_kill(ID(rt));
						}
	}
};

int LOVE_update(UPDATE_FUNC_ARGS)
{
	//kill any out of range LOVE
	if (x<9 || y<9 || x>=(int)((XRES-4)/9)*9 || y>=(int)((YRES-4)/9)*9)
		sim->part_kill(i);
	return 0;
}

void LOVE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LOVE";
	elem->Name = "LOVE";
	elem->Colour = COLPACK(0xFF30FF);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 373.0f;
	elem->HeatConduct = 40;
	elem->Latent = 0;
	elem->Description = "爱...";
	elem->DetailedDescription = "描述：爱心形状，同样是类似于彩蛋的东西。\n导热率：40\n初始温度：99.85℃/373K";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &LOVE_update;
	elem->Graphics = NULL;
	elem->Init = &LOVE_init_element;

	sim->elementData[t].reset(new LOVE_ElementDataContainer);
}
