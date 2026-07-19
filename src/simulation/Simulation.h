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

#ifndef Simulation_h
#define Simulation_h

#include <string>
#include "graphics/ARGBColour.h"
#include "graphics/Pixel.h"
#include "simulation/Air.h"
#include "simulation/Element.h"
#include "simulation/Gravity.h"
#include "simulation/SaveLoadData.h"
#include "simulation/SimulationData.h"
#include "simulation/StructProperty.h"
#include "simulation/elements/PPIP.h"
#include "powder.h"

class Brush;
class CoordStack;
class ElementDataContainer;
class Save;

class Simulation
{
public:
	unsigned int currentTick;

	particle parts[NPART];
	int elementCount[PT_NUM];
	Element elements[PT_NUM];
	Element origElements[PT_NUM];
	std::unique_ptr<ElementDataContainer> elementData[PT_NUM];
	int pfree;
	int parts_lastActiveIndex;
	int debug_currentParticle;
	int debug_mostRecentlyUpdated = -1; // -1 when between full update loops
	bool forceStackingCheck;
	
	Air *air;
	Gravity *grav;

	// settings
	int gravityMode = GRAV_VERTICAL;
	float customGravityX = 0.0f;
	float customGravityY = 0.0f;
	signed char edgeMode = EDGE_VOID;
	signed char saveEdgeMode = EDGE_VOID;
	bool msRotation; //for moving solids
	bool instantActivation; //electronics are instantly activated
	bool includePressure = true;
	int decoSpace = DECOSPACE_SRGB;
	int temperatureScale = 1;

	// misc Simulation variables
	unsigned int lightningRecreate; //timer for when LIGH can be created again
	
	Simulation();
	~Simulation();
	void InitElements();
	void Clear();
	void RecountElements();
	int part_create(int p, int x, int y, int t, int v = -1);
	int part_create_preserve_energy(int i, int x, int y, int t);
	void part_kill(int i);
	void part_delete(int x, int y);
	bool part_change_type(int i, int x, int y, int t, bool ignore_indestructible=false);
	void part_change_type_force(int i, int t);
	void ClearArea(int x, int y, int w, int h);
	void GetGravityField(int x, int y, float particleGrav, float newtonGrav, float & pGravX, float & pGravY);

	void RecalcFreeParticles(bool doLifeDec);
	void UpdateBefore();
	void UpdateParticles(int start, int end);
	void UpdateAfter();
	bool UpdateParticle(int i); // called by UpdateParticles
	void Tick();
	std::string ParticleDebug(int mode, int x, int y);
	
	SaveLoadData LoadSave(int loadX, int loadY, const Save *originalSave, int replace, bool includePressure=true);
	Save * CreateSave(int fullX, int fullY, int fullX2, int fullY2, bool includePressure=true);

	bool flood_water(int x, int y, int i);
	void spark_all(int i, int x, int y);
	bool spark_all_attempt(int i, int x, int y);
	void spark_conductive(int i, int x, int y);
	bool spark_conductive_attempt(int i, int x, int y);

	// Element drawing functions
	int CreateParts(int x, int y, int c, int flags, bool fill, Brush* brush = NULL);
	int CreatePartFlags(int x, int y, int c, int flags);
	void CreateLine(int x1, int y1, int x2, int y2, int c, int flags, Brush* brush = NULL);
	void CreateBox(int x1, int y1, int x2, int y2, int c, int flags);
	int FloodFillPmapCheck(int x, int y, unsigned int type);
	int FloodParts(int x, int y, int fullc, int replace, int flags);

	void CreateWall(int x, int y, int wall);
	void CreateWallLine(int x1, int y1, int x2, int y2, int rx, int ry, int wall);
	void CreateWallBox(int x1, int y1, int x2, int y2, int wall);
	int FloodWalls(int x, int y, int wall, int replace);

	int CreateTool(int x, int y, int brushX, int brushY, int tool, float strength);
	void CreateToolBrush(int x, int y, int tool, float strength, Brush* brush);
	void CreateToolLine(int x1, int y1, int x2, int y2, int tool, float strength, Brush* brush);
	void CreateToolBox(int x1, int y1, int x2, int y2, int tool, float strength);

	int CreateProp(int x, int y, StructProperty prop, PropertyValue propValue);
	void CreatePropBrush(int x, int y, StructProperty prop, PropertyValue propValue, Brush *brush);
	void CreatePropLine(int x1, int y1, int x2, int y2, StructProperty prop, PropertyValue propValue, Brush *brush);
	void CreatePropBox(int x1, int y1, int x2, int y2, StructProperty prop, PropertyValue propValue);
	int FloodProp(int x, int y, StructProperty prop, PropertyValue propValue);

	void CreateDeco(int x, int y, int tool, ARGBColour color);
	void CreateDecoBrush(int x, int y, int tool, ARGBColour color, Brush* brush);
	void CreateDecoLine(int x1, int y1, int x2, int y2, int tool, ARGBColour color, Brush* brush);
	void CreateDecoBox(int x1, int y1, int x2, int y2, int tool, ARGBColour color);
	bool ColorCompare(pixel *vid, int x, int y, ARGBColour replace);
	void FloodDeco(pixel *vid, int x, int y, ARGBColour color, ARGBColour replace);

	// movement, functions implemented in Movement.cpp
	unsigned char can_move[PT_NUM][PT_NUM];
	bool IsWallBlocking(int x, int y, int type);
	bool GetNormalInterp(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny);
	void InitCanMove();
	unsigned char EvalMove(int pt, int nx, int ny, unsigned *rr = NULL);
	int DoMove(int i, int x, int y, float nxf, float nyf);
	int Move(int i, int x, int y, float nxf, float nyf);

	// Functions defined here should hopefully be inlined
	// Don't put anything that will change often here, since changes cause a lot of recompiling
	bool IsElement(int t) const
	{
		return (t>0 && t<PT_NUM && elements[t].Enabled);
	}
	bool IsElementOrNone(int t) const
	{
		return (t>=0 && t<PT_NUM && elements[t].Enabled);
	}
	bool InBounds(int x, int y)
	{
		return (x>=0 && y>=0 && x<XRES && y<YRES);
	}
	bool IsHeatInsulator(const particle &p) const
	{
		return elements[p.type].HeatConduct == 0 || (p.type == PT_HSWC && p.life != 10) || ((p.type == PT_PIPE || p.type == PT_PPIP) && (p.tmp & PFLAG_CAN_CONDUCT) == 0);
	}
	float HeatCapacityOf(const particle &p) const
	{
		if ((p.type == PT_PIPE || p.type == PT_PPIP) && IsElement(p.ctype))
			return elements[p.type].HeatCapacity + elements[p.ctype].HeatCapacity;
		else
			return elements[p.type].HeatCapacity;
	}
	std::string ElementResolve(int type, int ctype) const;

	// Most of the time, part_alloc and part_free should not be used directly unless you really know what you're doing. 
	// Use part_create and part_kill instead.
	int part_alloc()
	{
		if (pfree == -1)
			return -1;
		int i = pfree;
		pfree = parts[i].life;
		NUM_PARTS += 1;
		if (i > parts_lastActiveIndex)
			parts_lastActiveIndex = i;
		return i;
	}
	void part_free(int i)
	{
		parts[i].type = 0;
		parts[i].life = pfree;
		pfree = i;
		NUM_PARTS -= 1;
	}
	void pmap_add(int i, int x, int y, int t)
	{
		// NB: all arguments are assumed to be within bounds
		if (elements[t].Properties & TYPE_ENERGY)
			photons[y][x] = PMAP(i, t);
		else if ((!pmap[y][x] || (t!=PT_INVIS && t!= PT_FILT)))// && TYP(pmap[y][x]) != PT_PINV)
			pmap[y][x] = PMAP(i, t);
	}
	void pmap_remove(unsigned int i, int x, int y)
	{
		// NB: all arguments are assumed to be within bounds
		if (pmap[y][x] && ID(pmap[y][x]) == i)
			pmap[y][x] = 0;
#ifndef NOMOD
		else if (TYP(pmap[y][x]) == PT_PINV && parts[ID(pmap[y][x])].tmp2 && (unsigned int)(ID(parts[ID(pmap[y][x])].tmp2)) == i)
			parts[ID(pmap[y][x])].tmp2 = 0;
#endif
		else if (photons[y][x] && ID(photons[y][x]) == i)
			photons[y][x] = 0;
	}

	char GetEdgeMode();
	void SetEdgeMode(char edgeMode);

private:
	// some movement functions are private
	void CreateGainPhoton(int pp);
	void CreateCherenkovPhoton(int pp);
	void PhotoelectricEffect(int nx, int ny);
	unsigned DirectionToMap(float dx, float dy, int t);
	bool IsBlocking(int t, int x, int y);
	bool IsBoundary(int pt, int x, int y);
	bool FindNextBoundary(int pt, int *x, int *y, int dm, int *em, bool reverse);
	bool GetNormal(int pt, int x, int y, float dx, float dy, float *nx, float *ny);

	int TryMove(int i, int x, int y, int nx, int ny);
	
	// Functions in Transitions.cpp
	bool TransferHeat(int i, int t, int surround[8]);
	bool CheckPressureTransitions(int i, int t);

	CoordStack& getCoordStackSingleton();
};

extern Simulation *globalSim; // TODO: remove this

#endif
