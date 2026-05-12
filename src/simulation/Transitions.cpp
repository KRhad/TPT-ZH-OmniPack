
#include "common/tpt-minmax.h"
#include <cmath>
#include "Simulation.h"
#include "common/tpt-rand.h"

bool Simulation::TransferHeat(int i, int t, int surround[8])
{
	int x = (int)(parts[i].x+0.5f), y = (int)(parts[i].y+0.5f);
	int j, r, rt, s;
	float gel_scale = 1.0f, ctemph, ctempl, swappage;

	if (t == PT_GEL)
		gel_scale = parts[i].tmp*2.55f;

	//some heat convection for liquids
	if ((elements[t].Properties&TYPE_LIQUID) && (t!=PT_GEL || gel_scale > RNG::Ref().between(1, 255)))
	{
		float convGravX, convGravY;
		GetGravityField(x, y, -2.0f, -2.0f, convGravX, convGravY);
		auto offsetX = std::clamp(int(std::round(convGravX + x)), x-1, x+1);
		auto offsetY = std::clamp(int(std::round(convGravY + y)), y-1, y+1);
		//some heat convection for liquids
		if (offsetX != x || offsetY != y)
		{
			r = pmap[offsetY][offsetX];
			if (r && parts[i].type == TYP(r))
			{
				if (parts[i].temp > parts[ID(r)].temp)
				{
					swappage = parts[i].temp;
					parts[i].temp = parts[ID(r)].temp;
					parts[ID(r)].temp = swappage;
				}
			}
		}
	}

	// Heat transfer code
	if (!IsHeatInsulator(parts[i]) && elements[t].HeatConduct*gel_scale != 0 && RNG::Ref().chance(elements[t].HeatConduct*gel_scale, 250))
	{
		// Heat transfer with air
		if (aheat_enable && !(elements[t].Properties&PROP_NOAMBHEAT))
		{
			float dtemp = air->hv[y/CELL][x/CELL] - parts[i].temp; // Temperature difference
			float hc = HeatCapacityOf(parts[i]);
			float alpha = std::min(0.04f, 0.4f * hc); // alpha / heat_capacity must be < 1

			// Here we completely ignore that there are CELL^2 "air pixels" in a cell, and the heat capacity of air
			parts[i].temp = restrict_flt(parts[i].temp + alpha*dtemp / hc, MIN_TEMP, MAX_TEMP);
			air->hv[y/CELL][x/CELL] = restrict_flt(air->hv[y/CELL][x/CELL] - alpha*dtemp, MIN_TEMP, MAX_TEMP);
		}

		// Heat transfer with other elements
		float hc_total = 0.0f; // Total heat capacity of elements involved
		float c_heat = 0.0f; // Total heat distributed between elements
		int surround_hconduct[8]; // IDs of elements which exchange heat

		for (j=0; j<8; j++)
		{
			surround_hconduct[j] = i;
			r = surround[j];
			if (!r)
				continue;
			rt = TYP(r);

			// Check if we can conduct heat
			if (!rt || IsHeatInsulator(parts[ID(r)])
				|| (t == PT_FILT && (rt == PT_BRAY || rt == PT_BIZR || rt == PT_BIZRG))
				|| (rt == PT_FILT && (t == PT_BRAY || t == PT_PHOT || t == PT_BIZR || t == PT_BIZRG))
				|| (t == PT_ELEC && rt == PT_DEUT)
				|| (t == PT_DEUT && rt == PT_ELEC)
				|| (t == PT_HSWC && rt == PT_FILT && parts[i].tmp == 1)
				|| (t == PT_FILT && rt == PT_HSWC && parts[ID(r)].tmp == 1))
				continue;

			surround_hconduct[j] = ID(r);
			float hc = HeatCapacityOf(parts[ID(r)]);
			c_heat += parts[ID(r)].temp * hc;
			hc_total += hc;
		}

		// Add the current particle
		float hc = HeatCapacityOf(parts[i]);
		c_heat += parts[i].temp * hc;
		hc_total += hc;

		// Equilibrium temperature
		float pt = restrict_flt(c_heat / hc_total, MIN_TEMP, MAX_TEMP);

		parts[i].temp = pt;
		for (int j = 0; j < 8; j++)
		{
			parts[surround_hconduct[j]].temp = pt;
		}

		ctemph = ctempl = pt;
		// change boiling point with pressure
		if (((elements[t].Properties&TYPE_LIQUID) && IsElementOrNone(elements[t].HighTemperatureTransitionElement) && (elements[elements[t].HighTemperatureTransitionElement].Properties&TYPE_GAS)) || t==PT_LNTG || t==PT_SLTW)
			ctemph -= 2.0f*air->pv[y/CELL][x/CELL];
		else if (((elements[t].Properties&TYPE_GAS) && IsElementOrNone(elements[t].LowTemperatureTransitionElement) && (elements[elements[t].LowTemperatureTransitionElement].Properties&TYPE_LIQUID)) || t==PT_WTRV)
			ctempl -= 2.0f*air->pv[y/CELL][x/CELL];
		s = 1;

		if (!(elements[t].Properties&PROP_INDESTRUCTIBLE))
		{
			//A fix for ice with ctype = 0
			if ((t==PT_ICEI || t==PT_SNOW) && (!IsElement(parts[i].ctype) || parts[i].ctype==PT_ICEI || parts[i].ctype==PT_SNOW))
				parts[i].ctype = PT_WATR;
			if (elements[t].HighTemperatureTransitionElement > -1 && ctemph >= elements[t].HighTemperatureTransitionThreshold)
			{
				// particle type change due to high temperature
				if (elements[t].HighTemperatureTransitionElement != PT_NUM)
				{
					t = elements[t].HighTemperatureTransitionElement;
				}
				else if (t==PT_ICEI || t==PT_SNOW)
				{
					if (parts[i].ctype>0&&parts[i].ctype<PT_NUM&&parts[i].ctype!=t)
					{
						if ((elements[parts[i].ctype].LowTemperatureTransitionElement==PT_ICEI || elements[parts[i].ctype].LowTemperatureTransitionElement==PT_SNOW))
						{
							if (pt < elements[parts[i].ctype].LowTemperatureTransitionThreshold)
								s = 0;
						}
						else if (pt < 273.15f)
							s = 0;
						if (s)
						{
							t = parts[i].ctype;
							parts[i].ctype = PT_NONE;
							parts[i].life = 0;
						}
					}
					else
						s = 0;
				}
				else if (t==PT_SLTW)
				{
					t = RNG::Ref().chance(1, 4) ? PT_SALT : PT_WTRV;
				}
				else if (t == PT_BRMT)
				{
					if (parts[i].ctype == PT_TUNG)
					{
						if (ctemph < elements[PT_TUNG].HighTemperatureTransitionThreshold)
							s = 0;
						else
						{
							t = PT_LAVA;
							parts[i].type = PT_TUNG;
						}
					}
					else if (ctemph >= elements[t].HighTemperatureTransitionElement)
						t = PT_LAVA;
					else
						s = 0;
				}
				else if (t == PT_CRMC)
				{
					float pres = std::max((air->pv[y/CELL][x/CELL]+air->pv[(y-2)/CELL][x/CELL]+air->pv[(y+2)/CELL][x/CELL]+air->pv[y/CELL][(x-2)/CELL]+air->pv[y/CELL][(x+2)/CELL])*2.0f, 0.0f);
					if (ctemph < pres+elements[PT_CRMC].HighTemperatureTransitionThreshold)
						s = 0;
					else
						t = PT_LAVA;
				}
				else if (t == PT_RIME)
				{
					if (parts[i].tmp > 5)
					{
						t = PT_ACID;
						parts[i].life = 25 + 5 * parts[i].tmp;
						parts[i].tmp = 0;
					}
					else
					{
						t = PT_WATR;
					}
				}
				else
					s = 0;
			}
			else if (elements[t].LowTemperatureTransitionElement>-1 && ctempl<elements[t].LowTemperatureTransitionThreshold)
			{
				// particle type change due to low temperature
				if (elements[t].LowTemperatureTransitionElement!=PT_NUM)
				{
					t = elements[t].LowTemperatureTransitionElement;
				}
				else if (t == PT_WTRV)
				{
					t = (pt < 273.0f) ? PT_RIME : PT_DSTW;
				}
				else if (t == PT_LAVA)
				{
					if (parts[i].ctype > 0 && parts[i].ctype < PT_NUM && parts[i].ctype != PT_LAVA && elements[parts[i].ctype].Enabled)
					{
						if (parts[i].ctype == PT_THRM && pt>=elements[PT_BMTL].HighTemperatureTransitionThreshold)
							s = 0;
						else if ((parts[i].ctype == PT_VIBR || parts[i].ctype == PT_BVBR) && pt>=273.15f)
							s = 0;
						else if (parts[i].ctype == PT_TUNG)
						{

							// TUNG does its own melting in its update function, so HighTemperatureTransition is not LAVA so it won't be handled by the code for HighTemperatureTransition==PT_LAVA below
							// However, the threshold is stored in HighTemperature to allow it to be changed from Lua
							if (pt >= elements[PT_TUNG].HighTemperatureTransitionThreshold)
								s = 0;
						}
						else if (parts[i].ctype == PT_CRMC)
						{
							float pres = std::max((air->pv[y/CELL][x/CELL]+air->pv[(y-2)/CELL][x/CELL]+air->pv[(y+2)/CELL][x/CELL]+air->pv[y/CELL][(x-2)/CELL]+air->pv[y/CELL][(x+2)/CELL])*2.0f, 0.0f);
							if (ctemph >= pres+elements[PT_CRMC].HighTemperatureTransitionThreshold)
								s = 0;
						}
						else if (elements[parts[i].ctype].HighTemperatureTransitionElement == PT_LAVA || parts[i].ctype == PT_HEAC)
						{
							if (pt>=elements[parts[i].ctype].HighTemperatureTransitionThreshold)
								s = 0;
						}
						else if (pt >= 973.0f)
							s = 0; // freezing point for lava with any other (not listed in element properties as turning into lava) ctype
						if (s)
						{
							t = parts[i].ctype;
							parts[i].ctype = PT_NONE;
							if (t == PT_THRM)
							{
								parts[i].tmp = 0;
								t = PT_BMTL;
							}
							if (t == PT_PLUT)
							{
								parts[i].tmp = 0;
								t = PT_LAVA;
							}
						}
					}
					else if (pt < 973.0f)
						t = PT_STNE;
					else
						s = 0;
				}
				else
					s = 0;
			}
			else
				s = 0;

			if (s)
			{ // particle type change occurred
				if (t == PT_ICEI || t == PT_LAVA || t == PT_SNOW)
					parts[i].ctype = parts[i].type;
				if (!(t == PT_ICEI && parts[i].ctype == PT_FRZW) && t != PT_ACID)
					parts[i].life = 0;
				if (t == PT_FIRE)
				{
					//hackish, if tmp isn't 0 the FIRE might turn into DSTW later
					//idealy transitions should use part_create(i) but some elements rely on properties staying constant
					//and I don't feel like checking each one right now
					parts[i].tmp = 0;
				}
				if ((elements[t].Properties & TYPE_GAS) && !(elements[parts[i].type].Properties & TYPE_GAS))
					air->pv[y/CELL][x/CELL] += 0.50f;
				if (t == PT_NONE)
				{
					part_kill(i);
					return true;
				}
				else
					part_change_type(i, x, y, t);
				if (t == PT_FIRE || t == PT_PLSM || t == PT_HFLM)
					parts[i].life = RNG::Ref().between(120, 169);
				if (t == PT_LAVA)
				{
					if (parts[i].ctype == PT_BRMT)		parts[i].ctype = PT_BMTL;
					else if (parts[i].ctype == PT_SAND)	parts[i].ctype = PT_GLAS;
					else if (parts[i].ctype == PT_BGLA)	parts[i].ctype = PT_GLAS;
					else if (parts[i].ctype == PT_PQRT)	parts[i].ctype = PT_QRTZ;
					else if (parts[i].ctype == PT_LITH && parts[i].tmp2 > 3) parts[i].ctype = PT_GLAS;
					parts[i].life = RNG::Ref().between(240, 359);
				}
			}
		}
		else
			s = 0;

		pt = parts[i].temp = restrict_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
		if (t==PT_LAVA)
		{
			parts[i].life = (int)restrict_flt((parts[i].temp-700)/7, 0.0f, 400.0f);
			if (parts[i].ctype==PT_THRM&&parts[i].tmp>0)
			{
				parts[i].tmp--;
				parts[i].temp = 3500;
			}
			if (parts[i].ctype==PT_PLUT&&parts[i].tmp>0)
			{
				parts[i].tmp--;
				parts[i].temp = MAX_TEMP;
			}
		}
		return s == 1;
	}
	else
	{
		if (!(air->blockairh[y/CELL][x/CELL]&0x8))
			air->blockairh[y/CELL][x/CELL]++;

		parts[i].temp = restrict_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
		return false;
	}
}

bool Simulation::CheckPressureTransitions(int i, int t)
{
	int x = (int)(parts[i].x+0.5f), y = (int)(parts[i].y+0.5f);
	float gravtot = std::fabs(grav->gravy[(y/CELL)*(XRES/CELL)+(x/CELL)]) + std::fabs(grav->gravx[(y/CELL)*(XRES/CELL)+(x/CELL)]);

	// particle type change due to high pressure
	if (elements[t].HighPressureTransitionElement > -1 && air->pv[y/CELL][x/CELL] > elements[t].HighPressureTransitionThreshold)
	{
		if (elements[t].HighPressureTransitionElement != PT_NUM)
			t = elements[t].HighPressureTransitionElement;
		else if (t == PT_BMTL)
		{
			if (air->pv[y/CELL][x/CELL] > 2.5f)
				t = PT_BRMT;
			else if (air->pv[y/CELL][x/CELL] > 1.0f && parts[i].tmp == 1)
				t = PT_BRMT;
			else
				return false;
		}
	}
	// particle type change due to low pressure
	else if (elements[t].LowPressureTransitionElement > -1 && air->pv[y/CELL][x/CELL] < elements[t].LowPressureTransitionThreshold && gravtot <= elements[t].HighPressureTransitionThreshold/4.0f)
	{
		if (elements[t].LowPressureTransitionElement != PT_NUM)
			t = elements[t].LowPressureTransitionElement;
		else
			return false;
	}
	// particle type change due to high gravity
	else if (elements[t].HighPressureTransitionElement > -1 && gravtot > elements[t].HighPressureTransitionThreshold/4.0f)
	{
		if (elements[t].HighPressureTransitionElement != PT_NUM)
			t = elements[t].HighPressureTransitionElement;
		else if (t == PT_BMTL)
		{
			if (gravtot > 0.625f)
				t = PT_BRMT;
			else if (gravtot>0.25f && parts[i].tmp==1)
				t = PT_BRMT;
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;

	// particle type change occurred
	parts[i].life = 0;
	if (!t)
		part_kill(i);
	else
		part_change_type(i,x,y,t);

	if (t == PT_FIRE)
		parts[i].life = RNG::Ref().between(120, 169);
	return true;
}
