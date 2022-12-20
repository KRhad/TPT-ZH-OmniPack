#include <cmath>
#include "Simulation.h"
#include "WallNumbers.h"
#include "common/tpt-math.h"
#include "common/tpt-rand.h"
#include "simulation/elements/FILT.h"
#include "simulation/elements/PRTI.h"

bool Simulation::IsWallBlocking(int x, int y, int type)
{
	if (bmap[y/CELL][x/CELL])
	{
		int wall = bmap[y/CELL][x/CELL];
		if (wall == WL_ALLOWGAS && !(elements[type].Properties&TYPE_GAS))
			return true;
		else if (wall == WL_ALLOWENERGY && !(elements[type].Properties&TYPE_ENERGY))
			return true;
		else if (wall == WL_ALLOWLIQUID && !(elements[type].Properties&TYPE_LIQUID))
			return true;
		else if (wall == WL_ALLOWPOWDER && !(elements[type].Properties&TYPE_PART))
			return true;
		else if (wall == WL_ALLOWAIR || wall == WL_WALL || wall == WL_WALLELEC)
			return true;
		else if (wall == WL_EWALL && !emap[y/CELL][x/CELL])
			return true;
		else if (wall == WL_DETECT && (elements[type].Properties & TYPE_SOLID))
			return -1;
	}
	return false;
}

// create photons when PHOT moves through GLOW
void Simulation::CreateGainPhoton(int pp)
{
	int lr = 2*RNG::Ref().between(0, 1) - 1; // -1 or 1
	float xx = parts[pp].x - lr * 0.3 * parts[pp].vy;
	float yy = parts[pp].y + lr * 0.3 * parts[pp].vx;

	int nx = (int)(xx + 0.5f);
	int ny = (int)(yy + 0.5f);

	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return;

	if (TYP(pmap[ny][nx]) != PT_GLOW)
		return;

	int i = part_create(-1, nx, ny, PT_PHOT);
	if (i < 0)
		return;

	parts[i].life = 680;
	parts[i].x = xx;
	parts[i].y = yy;
	parts[i].vx = parts[pp].vx;
	parts[i].vy = parts[pp].vy;
	parts[i].temp = parts[ID(pmap[ny][nx])].temp;

	int temp_bin = (int)((parts[i].temp-273.0f)*0.25f);
	temp_bin = std::max(0, std::min(25, temp_bin));
	parts[i].ctype = 0x1F << temp_bin;
}

// create photons when NEUT moves through GLAS
void Simulation::CreateCherenkovPhoton(int pp)
{
	int nx = (int)(parts[pp].x + 0.5f);
	int ny = (int)(parts[pp].y + 0.5f);
	if (TYP(pmap[ny][nx]) != PT_GLAS && TYP(pmap[ny][nx]) != PT_BGLA)
		return;

	if (hypotf(parts[pp].vx, parts[pp].vy) < 1.44f)
		return;

	int i = part_create(-1, nx, ny, PT_PHOT);
	if (i < 0)
		return;

	parts[i].ctype = 0x00000F80;
	parts[i].life = 680;
	parts[i].x = parts[pp].x;
	parts[i].y = parts[pp].y;
	parts[i].temp = parts[ID(pmap[ny][nx])].temp;
	parts[i].tmp3 = parts[i].tmp4 = 0;

	int lr = RNG::Ref().between(1, 2);
	if (lr)
	{
		parts[i].vx = parts[pp].vx - 2.5f*parts[pp].vy;
		parts[i].vy = parts[pp].vy + 2.5f*parts[pp].vx;
	}
	else
	{
		parts[i].vx = parts[pp].vx + 2.5f*parts[pp].vy;
		parts[i].vy = parts[pp].vy - 2.5f*parts[pp].vx;
	}

	/* photons have speed of light. no discussion. */
	float r = 1.269f / hypotf(parts[i].vx, parts[i].vy);
	parts[i].vx *= r;
	parts[i].vy *= r;
}

// ************************* //
// Photon movement functions //
// ************************* //

// create sparks from PHOT when hitting PSCN and NSCN
void Simulation::PhotoelectricEffect(int nx, int ny)
{
	unsigned r = pmap[ny][nx];

	if (TYP(r) == PT_PSCN)
	{
		if (TYP(pmap[ny][nx-1]) == PT_NSCN || TYP(pmap[ny][nx+1]) == PT_NSCN ||
		    TYP(pmap[ny-1][nx]) == PT_NSCN || TYP(pmap[ny+1][nx]) == PT_NSCN)
			spark_conductive_attempt(ID(r), nx, ny);
	}
}

unsigned Simulation::DirectionToMap(float dx, float dy, int t)
{
	// TODO:
	// Adding extra directions causes some inaccuracies.
	// Not adding them causes problems with some diagonal surfaces (photons absorbed instead of reflected).
	// For now, don't add them.
	// Solution may involve more intelligent setting of initial i0 value in find_next_boundary?
	// or rewriting normal/boundary finding code

	return (dx >= 0) |
		   (((dx + dy) >= 0) << 1) |     /*  567  */
		   ((dy >= 0) << 2) |            /*  4+0  */
		   (((dy - dx) >= 0) << 3) |     /*  321  */
		   ((dx <= 0) << 4) |
		   (((dx + dy) <= 0) << 5) |
		   ((dy <= 0) << 6) |
		   (((dy - dx) <= 0) << 7);
	/*
	return (dx >= -0.001) |
		   (((dx + dy) >= -0.001) << 1) |     //  567
		   ((dy >= -0.001) << 2) |            //  4+0
		   (((dy - dx) >= -0.001) << 3) |     //  321
		   ((dx <= 0.001) << 4) |
		   (((dx + dy) <= 0.001) << 5) |
		   ((dy <= 0.001) << 6) |
		   (((dy - dx) <= 0.001) << 7);
	}*/
}

bool Simulation::IsBlocking(int t, int x, int y)
{
	if (t & REFRACT) {
		if (x<0 || y<0 || x>=XRES || y>=YRES)
			return false;
		if (TYP(pmap[y][x]) == PT_GLAS || TYP(pmap[y][x]) == PT_BGLA)
			return true;
		return false;
	}

	return !EvalMove(t, x, y);
}

bool Simulation::IsBoundary(int pt, int x, int y)
{
	if (!IsBlocking(pt, x, y))
		return false;
	if (IsBlocking(pt, x, y-1) && IsBlocking(pt, x, y+1) && IsBlocking(pt, x-1, y) && IsBlocking(pt, x+1, y))
		return false;
	return true;
}

bool Simulation::FindNextBoundary(int pt, int *x, int *y, int dm, int *em, bool reverse)
{
	static int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
	static int dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	static int de[8] = { 0x83, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xC1 };

	if (*x <= 0 || *x >= XRES-1 || *y <= 0 || *y >= YRES-1)
		return false;

	if (*em != -1)
	{
		dm &= de[*em];
	}

	unsigned int mask = 0;
	for (int i = 0; i < 8; ++i)
	{
		if ((dm & (1U << i)) && IsBlocking(pt, *x + dx[i], *y + dy[i]))
		{
			mask |= (1U << i);
		}
	}

	for (int i = 0; i < 8; ++i)
	{
		int n = (i + (reverse ? 1 : -1)) & 7;
		if (((mask & (1U << i))) && !(mask & (1U << n)))
		{
			*x += dx[i];
			*y += dy[i];
			*em = i;
			return true;
		}
	}

	return false;
}

bool Simulation::GetNormal(int pt, int x, int y, float dx, float dy, float *nx, float *ny)
{
	if (!dx && !dy)
		return false;

	if (!IsBoundary(pt, x, y))
		return false;

	int ldm = DirectionToMap(-dy, dx, pt);
	int rdm = DirectionToMap(dy, -dx, pt);
	int lx = x, rx = x;
	int ly = y, ry = y;
	int lv = 1, rv = 1;
	int lm = -1, rm = -1;

	int j = 0;
	for (int i = 0; i < SURF_RANGE; i++)
	{
		if (lv)
			lv = FindNextBoundary(pt, &lx, &ly, ldm, &lm, true);
		if (rv)
			rv = FindNextBoundary(pt, &rx, &ry, rdm, &rm, false);
		j += lv + rv;
		if (!lv && !rv)
			break;
	}

	if (j < NORMAL_MIN_EST)
		return false;

	if ((lx == rx) && (ly == ry))
		return false;

	float ex = (float)rx - lx;
	float ey = (float)ry - ly;
	float r = 1.0f/hypot(ex, ey);
	*nx =  ey * r;
	*ny = -ex * r;

	return true;
}

bool Simulation::GetNormalInterp(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny)
{
	dx /= NORMAL_FRAC;
	dy /= NORMAL_FRAC;

	int i, x, y;
	for (i = 0; i < NORMAL_INTERP; i++)
	{
		x = (int)(x0 + 0.5f);
		y = (int)(y0 + 0.5f);
		if (x < 0 || y < 0 || x >= XRES || y >= YRES)
		{
			return 0;
		}
		if (IsBoundary(pt, x, y))
			break;
		x0 += dx;
		y0 += dy;
	}
	if (i >= NORMAL_INTERP)
		return false;

	if (pt == PT_PHOT)
		PhotoelectricEffect(x, y);

	return GetNormal(pt, x, y, dx, dy, nx, ny);
}

// ************************* //
// Actual movement functions //
// ************************* //

void Simulation::InitCanMove()
{
	// can_move[moving type][type at destination]
	//  0 = No move/Bounce
	//  1 = Swap
	//  2 = Both particles occupy the same space.
	//  3 = Varies, go run some extra checks

	// particles that don't exist shouldn't move...
	for (int destinationType = 0; destinationType < PT_NUM; destinationType++)
		can_move[0][destinationType] = 0;

	//initialize everything else to swapping by default
	for (int movingType = 1; movingType < PT_NUM; movingType++)
		for (int destinationType = 0; destinationType < PT_NUM; destinationType++)
			can_move[movingType][destinationType] = 1;

	//photons go through everything by default
	for (int destinationType = 1; destinationType < PT_NUM; destinationType++)
		can_move[PT_PHOT][destinationType] = 2;

	for (int movingType = 1; movingType < PT_NUM; movingType++)
	{
		for (int destinationType = 1; destinationType < PT_NUM; destinationType++)
		{
			// weight check, also prevents particles of same type displacing each other
			if (elements[movingType].Weight <= elements[destinationType].Weight || destinationType == PT_GEL)
				can_move[movingType][destinationType] = 0;

			//other checks for NEUT and energy particles
			if (movingType == PT_NEUT && elements[destinationType].Properties&PROP_NEUTPASS)
				can_move[movingType][destinationType] = 2;
			if (movingType == PT_NEUT && elements[destinationType].Properties&PROP_NEUTABSORB)
				can_move[movingType][destinationType] = 1;
			if (movingType == PT_NEUT && elements[destinationType].Properties&PROP_NEUTPENETRATE)
				can_move[movingType][destinationType] = 1;
			if (elements[movingType].Properties&PROP_NEUTPENETRATE && destinationType == PT_NEUT)
				can_move[movingType][destinationType] = 0;
			if (elements[movingType].Properties&TYPE_ENERGY && elements[destinationType].Properties&TYPE_ENERGY)
				can_move[movingType][destinationType] = 2;
			if (elements[destinationType].Properties&PROP_INDESTRUCTIBLE)
				can_move[movingType][destinationType] = 0;
		}
	}
	for (int destinationType = 0; destinationType < PT_NUM; destinationType++)
	{
		//set what stickmen can move through
		unsigned char stkm_move = 0;
		if (elements[destinationType].Properties & (TYPE_LIQUID | TYPE_GAS))
			stkm_move = 2;
		if (!destinationType || destinationType == PT_PRTO || destinationType == PT_SPAWN || destinationType == PT_SPAWN2)
			stkm_move = 2;
		can_move[PT_STKM][destinationType] = stkm_move;
		can_move[PT_STKM2][destinationType] = stkm_move;
		can_move[PT_FIGH][destinationType] = stkm_move;

		//spark shouldn't move
		can_move[PT_SPRK][destinationType] = 0;
	}
	for (int movingType = 1; movingType < PT_NUM; movingType++)
	{
		//everything "swaps" with VACU and BHOL to make them eat things
		can_move[movingType][PT_BHOL] = 1;
		can_move[movingType][PT_NBHL] = 1;
		//nothing goes through stickmen
		can_move[movingType][PT_STKM] = 0;
		can_move[movingType][PT_STKM2] = 0;
		can_move[movingType][PT_FIGH] = 0;
		//INVS behavior varies with pressure
		can_move[movingType][PT_INVIS] = 3;
#ifndef NOMOD
		can_move[movingType][PT_PINV] = 3;
#endif
		//stop CNCT from being displaced by other particles
		can_move[movingType][PT_CNCT] = 0;
		//VOID and PVOD behavior varies with powered state and ctype
		can_move[movingType][PT_PVOD] = 3;
		can_move[movingType][PT_VOID] = 3;
		//nothing moves through EMBR (not sure why, but it's killed when it touches anything)
		can_move[movingType][PT_EMBR] = 0;
		can_move[PT_EMBR][movingType] = 0;
		//Energy particles move through VIBR and BVBR, so it can absorb them
		if (elements[movingType].Properties&TYPE_ENERGY)
		{
			can_move[movingType][PT_VIBR] = 1;
			can_move[movingType][PT_BVBR] = 1;
		}

		//SAWD cannot be displaced by other powders
#ifdef NOMOD
		if (elements[movingType].Properties & TYPE_PART)
#else
		if ((elements[movingType].Properties & TYPE_PART) && movingType != PT_RAZR)
#endif
			can_move[movingType][PT_SAWD] = 0;
	}
	//a list of lots of things PHOT can move through
	for (int destinationType = 0; destinationType < PT_NUM; destinationType++)
	{
		if (destinationType == PT_GLAS || destinationType == PT_PHOT || destinationType == PT_FILT || destinationType == PT_H2
		 || destinationType == PT_WATR || destinationType == PT_DSTW || destinationType == PT_SLTW || destinationType == PT_GLOW
		 || destinationType == PT_ISOZ || destinationType == PT_ISZS || destinationType == PT_QRTZ || destinationType == PT_PQRT
		 || destinationType == PT_INVIS|| destinationType == PT_BGLA || destinationType == PT_C5
#ifndef NOMOD
		 || destinationType == PT_PINV
#endif
		 || (elements[destinationType].Properties&PROP_CLONE) || (elements[destinationType].Properties&PROP_BREAKABLECLONE))
			can_move[PT_PHOT][destinationType] = 2;
		if (destinationType != PT_DMND && destinationType != PT_INSL && destinationType != PT_VOID && destinationType != PT_PVOD
			 && destinationType != PT_VIBR && destinationType != PT_BVBR && destinationType != PT_PRTO && destinationType != PT_PRTI
#ifndef NOMOD
			 && destinationType != PT_PPTO && destinationType != PT_PPTI
#endif
			 )
		{
			can_move[PT_PROT][destinationType] = 2;
			can_move[PT_GRVT][destinationType] = 2;
		}
	}

	//other special cases that weren't covered above
	can_move[PT_DEST][PT_DMND] = 0;
	can_move[PT_DEST][PT_CLNE] = 0;
	can_move[PT_DEST][PT_PCLN] = 0;
	can_move[PT_DEST][PT_BCLN] = 0;
	can_move[PT_DEST][PT_PBCN] = 0;
	can_move[PT_DEST][PT_ROCK] = 0;

	can_move[PT_NEUT][PT_INVIS] = 2;
#ifndef NOMOD
	can_move[PT_ELEC][PT_PINV] = 2;
#endif
	can_move[PT_ELEC][PT_LCRY] = 2;
	can_move[PT_ELEC][PT_EXOT] = 2;
	can_move[PT_ELEC][PT_GLOW] = 2;
	can_move[PT_PHOT][PT_LCRY] = 3; //varies according to LCRY life
	can_move[PT_PHOT][PT_GPMP] = 3;

	can_move[PT_PHOT][PT_BIZR] = 2;
	can_move[PT_ELEC][PT_BIZR] = 2;
	can_move[PT_PHOT][PT_BIZRG] = 2;
	can_move[PT_ELEC][PT_BIZRG] = 2;
	can_move[PT_PHOT][PT_BIZRS] = 2;
	can_move[PT_ELEC][PT_BIZRS] = 2;
	can_move[PT_BIZR][PT_FILT] = 2;
	can_move[PT_BIZRG][PT_FILT] = 2;

	can_move[PT_ANAR][PT_WHOL] = 1; //WHOL eats ANAR
	can_move[PT_ANAR][PT_NWHL] = 1;
	can_move[PT_ELEC][PT_DEUT] = 1;
	can_move[PT_SPNG][PT_SPNG] = 3;
	can_move[PT_THDR][PT_THDR] = 2;
	can_move[PT_EMBR][PT_EMBR] = 2;
	can_move[PT_TRON][PT_SWCH] = 3;
	can_move[PT_SOAP][PT_OIL] = 0;
	can_move[PT_OIL][PT_SOAP] = 1;

#ifndef NOMOD
	can_move[PT_RAZR][PT_CNCT] = 1;
	can_move[PT_RAZR][PT_GEL] = 1;
	can_move[PT_MOVS][PT_MOVS] = 2;
#endif
}

/*
   RETURN-value explanation
1 = Swap
0 = No move/Bounce
2 = Both particles occupy the same space.
 */
unsigned char Simulation::EvalMove(int pt, int nx, int ny, unsigned *rr)
{
	unsigned r;
	int result;

	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return 0;

	r = pmap[ny][nx];
	if (r)
		r = (r&~PMAPMASK) | parts[ID(r)].type;
#ifndef NOMOD
	if (TYP(r) == PT_PINV && parts[ID(r)].tmp2)
		r = parts[ID(r)].tmp2;
#endif
	if (rr)
		*rr = r;
	if (pt>=PT_NUM || TYP(r)>=PT_NUM)
		return 0;
	result = can_move[pt][TYP(r)];
	if (result == 3)
	{
		switch TYP(r)
		{
		case PT_LCRY:
			if (pt==PT_PHOT)
				result = (parts[ID(r)].life > 5)? 2 : 0;
			break;
		case PT_GPMP:
			if (pt == PT_PHOT)
				result = (parts[ID(r)].life < 10) ? 2 : 0;
			break;
		case PT_INVIS:
			float pressureResistance;
			if (parts[ID(r)].tmp > 0)
				pressureResistance = (float)parts[ID(r)].tmp;
			else
				pressureResistance = 4.0f;

			if (air->pv[ny/CELL][nx/CELL] < -pressureResistance || air->pv[ny/CELL][nx/CELL] > pressureResistance)
				result = 2;
			else
				result = 0;
			break;
#ifndef NOMOD
		case PT_PINV:
			if (parts[ID(r)].life >= 10)
				result = 2;
			else
				result = 0;
			break;
#endif
		case PT_PVOD:
			if (parts[ID(r)].life == 10)
			{
				if(!parts[ID(r)].ctype || (parts[ID(r)].ctype==pt) != (parts[ID(r)].tmp&1))
					result = 1;
				else
					result = 0;
			}
			else
				result = 0;
			break;
		case PT_VOID:
			if(!parts[ID(r)].ctype || (parts[ID(r)].ctype==pt) != (parts[ID(r)].tmp&1))
				result = 1;
			else
				result = 0;
			break;
		case PT_SWCH:
			if (pt == PT_TRON)
			{
				if (parts[ID(r)].life >= 10)
					return 2;
				else
					return 0;
			}
			break;
		case PT_SPNG:
			if (parts[ID(r)].vx == 0 && parts[ID(r)].vy == 0)
				result = 0;
			else
				result = 2;
			break;
		default:
			if (pt == PT_SPNG)
			{
				int vx = (int)parts[pt].vx;
				int vy = (int)parts[pt].vy;
				parts[ID(r)].x += vx;
				parts[ID(r)].y += vy;
				result = 2;
			}
			// This should never happen
			// If it were to happen, try_move would interpret a 3 as a 1
			result =  1;
			break;
		}
	}
	if (bmap[ny/CELL][nx/CELL])
	{
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWGAS && !(elements[pt].Properties&TYPE_GAS))// && elements[pt].Falldown!=0 && pt!=PT_FIRE && pt!=PT_SMKE)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWENERGY && !(elements[pt].Properties&TYPE_ENERGY))// && elements[pt].Falldown!=0 && pt!=PT_FIRE && pt!=PT_SMKE)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWLIQUID && elements[pt].Falldown!=2)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWPOWDER && elements[pt].Falldown!=1)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWAIR || bmap[ny/CELL][nx/CELL]==WL_WALL || bmap[ny/CELL][nx/CELL]==WL_WALLELEC)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_EWALL && !emap[ny/CELL][nx/CELL])
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_EHOLE && !emap[ny/CELL][nx/CELL] && !(elements[pt].Properties&TYPE_SOLID) && !(elements[TYP(r)].Properties&TYPE_SOLID))
			return 2;
	}
	return result;
}

int Simulation::TryMove(int i, int x, int y, int nx, int ny)
{
	unsigned r, e;

	if (x==nx && y==ny)
		return 1;
	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return 1;

	e = EvalMove(parts[i].type, nx, ny, &r);

	/* half-silvered mirror */
	if (!e && parts[i].type == PT_PHOT &&
	        ((TYP(r) == PT_BMTL && RNG::Ref().chance(1, 2)) || TYP(pmap[y][x]) == PT_BMTL))
		e = 2;

	if (!e) //if no movement
	{
		if (TYP(r) == PT_WOOD)
		{
			float vel = std::sqrt(std::pow(parts[i].vx, 2) + std::pow(parts[i].vy, 2));
			if (vel > 5)
				part_change_type(ID(r), nx, ny, PT_SAWD);
		}
		if (!(elements[parts[i].type].Properties & TYPE_ENERGY))
			return 0;
		if (!legacy_enable && parts[i].type==PT_PHOT && r)//PHOT heat conduction
		{
			if (TYP(r) == PT_COAL || TYP(r) == PT_BCOL)
				parts[ID(r)].temp = parts[i].temp;

			if (TYP(r) < PT_NUM && elements[TYP(r)].HeatConduct && (TYP(r)!=PT_HSWC||parts[ID(r)].life==10) && TYP(r)!=PT_FILT)
				parts[i].temp = parts[ID(r)].temp = restrict_flt((parts[ID(r)].temp+parts[i].temp)/2, MIN_TEMP, MAX_TEMP);
		}
		else if ((parts[i].type==PT_NEUT || parts[i].type==PT_ELEC) && ((elements[TYP(r)].Properties&PROP_CLONE) || (elements[TYP(r)].Properties&PROP_BREAKABLECLONE))) {
			if (!parts[ID(r)].ctype)
				parts[ID(r)].ctype = parts[i].type;
		}
#ifdef NOMOD
		if (TYP(r)==PT_PRTI && (elements[parts[i].type].Properties & TYPE_ENERGY))
#else
		if ((TYP(r)==PT_PRTI || TYP(r)==PT_PPTI) && (elements[parts[i].type].Properties & TYPE_ENERGY))
#endif
		{
			PortalChannel *channel = static_cast<PRTI_ElementDataContainer&>(*elementData[PT_PRTI]).GetParticleChannel(this, ID(r));
			int slot = PRTI_ElementDataContainer::GetSlot(x-nx,y-ny);
			if (channel->StoreParticle(this, i, slot))
				return -1;
		}
		return 0;
	}

	if (e == 2) //if occupy same space
	{
		switch (parts[i].type)
		{
		case PT_PHOT:
		{
			switch TYP(r)
			{
			case PT_GLOW:
				if (!parts[ID(r)].life && RNG::Ref().chance(1, 30))
				{
					parts[ID(r)].life = 120;
					CreateGainPhoton(i);
				}
				break;
			case PT_FILT:
				parts[i].ctype = interactWavelengths(&parts[ID(r)], parts[i].ctype);
				break;
			case PT_C5:
				if (parts[ID(r)].life > 0 && (parts[ID(r)].ctype & parts[i].ctype & 0xFFFFFFC0))
				{
					float vx = ((parts[ID(r)].tmp << 16) >> 16) / 255.0f;
					float vy = (parts[ID(r)].tmp >> 16) / 255.0f;
					float vn = parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy;
					
					// if the resulting velocity would be 0, that would cause division by 0 inside the else
					// shoot the photon off at a 90 degree angle instead (probably particle order dependent)
					if (parts[i].vx + vx == 0 && parts[i].vy + vy == 0)
					{
						parts[i].vx = vy;
						parts[i].vy = -vx;
					}
					else
					{
						parts[i].ctype = (parts[ID(r)].ctype & parts[i].ctype) >> 6;
						// add momentum of photons to each other
						parts[i].vx += vx;
						parts[i].vy += vy;
						// normalize velocity to original value
						vn /= parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy;
						vn = sqrtf(vn);
						parts[i].vx *= vn;
						parts[i].vy *= vn;
					}
					parts[ID(r)].life = 0;
					parts[ID(r)].ctype = 0;
				}
				else if (!parts[ID(r)].ctype && parts[i].ctype & 0xFFFFFFC0)
				{
					parts[ID(r)].life = 1;
					parts[ID(r)].ctype = parts[i].ctype;
					parts[ID(r)].tmp = (0xFFFF & (int)(parts[i].vx * 255.0f)) | (0xFFFF0000 & (int)(parts[i].vy * 16711680.0f));
					parts[ID(r)].tmp2 = (0xFFFF & (int)((parts[i].x - x) * 255.0f)) | (0xFFFF0000 & (int)((parts[i].y - y) * 16711680.0f));
					part_kill(i);
				}
				break;
			case PT_INVIS:
			{
				float pressureResistance = (parts[ID(r)].tmp > 0) ? (float)parts[ID(r)].tmp : 4.0f;

				if (air->pv[ny/CELL][nx/CELL] >= -pressureResistance && air->pv[ny/CELL][nx/CELL] <= pressureResistance)
				{
					part_change_type(i, x, y, PT_NEUT);
					parts[i].ctype = 0;
				}
				break;
			}
#ifndef NOMOD
			case PT_PINV:
				if (!parts[ID(r)].life)
				{
					part_change_type(i, x, y, PT_ELEC);
					parts[i].ctype = 0;
				}
				break;
#endif
			case PT_BIZR:
			case PT_BIZRG:
			case PT_BIZRS:
				part_change_type(i, x, y, PT_ELEC);
				parts[i].ctype = 0;
				break;
			case PT_H2:
				if (!(parts[i].tmp&0x1))
				{
					part_change_type(i, x, y, PT_PROT);
					parts[i].ctype = 0;
					parts[i].tmp2 = 0x1;
	
					part_create(ID(r), x, y, PT_ELEC);
					return -1;
				}
				break;
			case PT_GPMP:
				if (parts[ID(r)].life == 0)
				{
					part_change_type(i, x, y, PT_GRVT);
					parts[i].tmp = (int)(parts[ID(r)].temp - 273.15f);
				}
				break;
			}
			break;
		}
		case PT_NEUT:
			if (TYP(r) == PT_GLAS || TYP(r) == PT_BGLA)
			{
				if (RNG::Ref().chance(1, 10))
					CreateCherenkovPhoton(i);
			}
			break;
		case PT_ELEC:
			if (TYP(r) == PT_GLOW)
			{
				part_change_type(i, x, y, PT_PHOT);
				parts[i].ctype = 0x3FFFFFFF;
			}
			break;
		case PT_PROT:
			if (TYP(r) == PT_INVIS)
				part_change_type(i, x, y, PT_NEUT);
			break;
		case PT_BIZR:
		case PT_BIZRG:
			if (TYP(r) == PT_FILT)
				parts[i].ctype = interactWavelengths(&parts[ID(r)], parts[i].ctype);
			break;
		}
		return 1;
	}
	//else e=1 , we are trying to swap the particles, return 0 no swap/move, 1 is still overlap/move, because the swap takes place later

	switch (TYP(r))
	{
	case PT_VOID:
	case PT_PVOD:
		// This is where void eats particles
		// Void ctype already checked in eval_move
		part_kill(i);
		return 0;
	case PT_BHOL:
	case PT_NBHL:
		// This is where blackhole eats particles
		part_kill(i);
		if (!legacy_enable)
		{
			parts[ID(r)].temp = restrict_flt(parts[ID(r)].temp+parts[i].temp/2, MIN_TEMP, MAX_TEMP);//3.0f;
		}
		return 0;
	case PT_WHOL:
	case PT_NWHL:
		// Whitehole eats anar
		if (parts[i].type == PT_ANAR)
		{
			if (!legacy_enable)
			{
				parts[ID(r)].temp = restrict_flt(parts[ID(r)].temp- (MAX_TEMP-parts[i].temp)/2, MIN_TEMP, MAX_TEMP);
			}
			part_kill(i);
			return 0;
		}
		break;
	case PT_DEUT:
		if (parts[i].type == PT_ELEC)
		{
			if (parts[ID(r)].life < 6000)
				parts[ID(r)].life += 1;
			parts[ID(r)].temp = 0;
			part_kill(i);
			return 0;
		}
		break;
	case PT_VIBR:
	case PT_BVBR:
		if (elements[parts[i].type].Properties & TYPE_ENERGY)
		{
			parts[ID(r)].tmp += 20;
			part_kill(i);
			return 0;
		}
		break;
		// SOAP slowly floats up inside OIL
		case PT_SOAP:
			if (parts[i].type == PT_OIL)
			{
				if (RNG::Ref().chance(19, 20) || std::abs(parts[i].x - nx) > 3 || std::abs(parts[i].y - ny) > 3)
					return 0;
			}
			break;
	}

	switch (parts[i].type)
	{
	case PT_NEUT:
		if (elements[TYP(r)].Properties & PROP_NEUTABSORB)
		{
			part_kill(i);
			return 0;
		}
		break;
	case PT_CNCT:
		{
			float cnctGravX, cnctGravY; // Calculate offset from gravity
			GetGravityField(x, y, elements[PT_CNCT].Gravity, elements[PT_CNCT].Gravity, cnctGravX, cnctGravY);
			int offsetX = 0, offsetY = 0;
			if (cnctGravX > 0.0f) offsetX++;
			else if (cnctGravX < 0.0f) offsetX--;
			if (cnctGravY > 0.0f) offsetY++;
			else if (cnctGravY < 0.0f) offsetY--;
			if ((offsetX != 0) != (offsetY != 0) && // Is this a different position (avoid diagonals, doesn't work well)
				((nx - x) * offsetX > 0 || (ny - y) * offsetY > 0) && // Is the destination particle below the moving particle
				(TYP(pmap[y+offsetY][x+offsetX]) == PT_CNCT || TYP(pmap[y+offsetY][x+offsetX]) == PT_ROCK)) //check below CNCT for another CNCT or ROCK
				return 0;
		}
		break;
	case PT_GBMB:
		if (parts[i].life > 0)
			return 0;
		break;
	}

	if ((bmap[y/CELL][x/CELL]==WL_EHOLE && !emap[y/CELL][x/CELL]) && !(bmap[ny/CELL][nx/CELL]==WL_EHOLE && !emap[ny/CELL][nx/CELL]))
		return 0;

	e = ID(r); //e is now the particle number at r (pmap[ny][nx])
	if (r)//the swap part, if we make it this far, swap
	{
		if (parts[i].type==PT_NEUT)
		{
			// target material is NEUTPENETRATE, meaning it gets moved around when neutron passes
			unsigned s = pmap[y][x];
			if (s && !(elements[TYP(s)].Properties&PROP_NEUTPENETRATE))
				return 1; // if the element currently underneath neutron isn't NEUTPENETRATE, don't move anything except the neutron
			// if nothing is currently underneath neutron, only move target particle
			if (bmap[y/CELL][x/CELL] == WL_ALLOWENERGY)
				return 1; // do not drag target particle into an energy only wall
			if (s)
			{
				pmap[ny][nx] = (s&~PMAPMASK)|parts[ID(s)].type;
				parts[ID(s)].x = (float)nx;
				parts[ID(s)].y = (float)ny;
			}
			else pmap[ny][nx] = 0;
			parts[e].x = (float)x;
			parts[e].y = (float)y;
			pmap[y][x] = PMAP(e, parts[e].type);
			return 1;
		}

		if (ID(pmap[ny][nx]) == e)
			pmap[ny][nx] = 0;
		parts[e].x += float(x - nx);
		parts[e].y += float(y - ny);
		int rx = int(parts[e].x + 0.5f);
		int ry = int(parts[e].y + 0.5f);
		// This check will never fail unless the pmap array has already been corrupted via another bug
		// In that case, r's position is inaccurate (not actually at nx/ny) and rx/ry may be out of bounds
		if (InBounds(rx, ry))
			pmap[ry][rx] = PMAP(e, parts[e].type);
	}
	return 1;
}

// try to move particle, and if successful call move() to update variables
int Simulation::DoMove(int i, int x, int y, float nxf, float nyf)
{
	// volatile to hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
	volatile float tmpx = nxf, tmpy = nyf;
	int nx = (int)(tmpx+0.5f), ny = (int)(tmpy+0.5f), result;

	if (parts[i].type == PT_NONE)
		return 0;
	if (GetEdgeMode() == 2)
	{
		bool x_ok = (nx >= CELL && nx < XRES-CELL);
		bool y_ok = (ny >= CELL && ny < YRES-CELL);
		if (!x_ok)
			nxf = remainder_p(nxf-CELL+.5f, XRES-CELL*2.0f)+CELL-.5f;
		if (!y_ok)
			nyf = remainder_p(nyf-CELL+.5f, YRES-CELL*2.0f)+CELL-.5f;
		nx = (int)(nxf+0.5f);
		ny = (int)(nyf+0.5f);
	}
	result = TryMove(i, x, y, nx, ny);
	if (result && Move(i,x,y,nxf,nyf))
		return -1;
	return result;
}

//update pmap and parts[i].x,y
int Simulation::Move(int i, int x, int y, float nxf, float nyf)
{
	// volatile to hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
	volatile float tmpx = nxf, tmpy = nyf;
	int nx = (int)(tmpx+0.5f), ny = (int)(tmpy+0.5f), t = parts[i].type;

	parts[i].x = nxf;
	parts[i].y = nyf;
	if (ny!=y || nx!=x)
	{

		if (pmap[y][x] && (int)ID(pmap[y][x]) == i)
			pmap[y][x] = 0;
#ifndef NOMOD
		else if (TYP(pmap[y][x]) == PT_PINV && ID(parts[ID(pmap[y][x])].tmp2) && ID(parts[ID(pmap[y][x])].tmp2) == i)
			parts[ID(pmap[y][x])].tmp2 = 0;
#endif
		else if (photons[y][x] && (int)ID(photons[y][x]) == i)
			photons[y][x] = 0;

		//kill particle if particle is out of bounds
		if (!InBounds(nx, ny))
		{
			part_kill(i);
			return -1;
		}

		if (elements[t].Properties & TYPE_ENERGY)
			photons[ny][nx] = PMAP(i, t);
#ifndef NOMOD
		else if (t && TYP(pmap[ny][nx]) != PT_PINV && (t != PT_MOVS || !TYP(pmap[ny][nx]) || TYP(pmap[ny][nx]) == PT_MOVS))
			pmap[ny][nx] = PMAP(i, t);
		else if (t && TYP(pmap[ny][nx]) == PT_PINV)
			parts[ID(pmap[ny][nx])].tmp2 = PMAP(i, t);
#else
		else if (t)
			pmap[ny][nx] = PMAP(i, t);
#endif
	}
	return 0;
}
