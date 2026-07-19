#include "ElementsCommon.h"
#include "GolNumbers.h"

// Interactions which only occur when legacy_enable is on
int update_legacy_all(UPDATE_FUNC_ARGS)
{
	switch (parts[i].type)
	{
	case PT_WTRV:
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_WATR||TYP(r)==PT_DSTW||TYP(r)==PT_SLTW) && RNG::Ref().chance(1, 1000))
					{
						part_change_type(i,x,y,PT_WATR);
						part_change_type(ID(r),x+rx,y+ry,PT_WATR);
					}
					if ((TYP(r)==PT_ICEI || TYP(r)==PT_SNOW) && RNG::Ref().chance(1, 1000))
					{
						part_change_type(i,x,y,PT_WATR);
						if (RNG::Ref().chance(1, 1000))
							part_change_type(ID(r),x+rx,y+ry,PT_WATR);
					}
				}
		if (sim->air->pv[y/CELL][x/CELL] > 4.0f)
			part_change_type(i,x,y,PT_DSTW);
		break;
	case PT_WATR:
	case PT_DSTW:
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_FIRE || TYP(r)==PT_LAVA) && RNG::Ref().chance(1, 10))
					{
						part_change_type(i,x,y,PT_WTRV);
					}
				}
		break;
	case PT_SLTW:
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_FIRE || TYP(r)==PT_LAVA) && RNG::Ref().chance(1, 10))
					{
						if (RNG::Ref().chance(1, 4))
							part_change_type(i,x,y,PT_SALT);
						else
							part_change_type(i,x,y,PT_WTRV);
					}
				}
		break;
	case PT_ICEI:
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_WATR || TYP(r)==PT_DSTW) && RNG::Ref().chance(1, 1000))
					{
						part_change_type(i,x,y,PT_ICEI);
						part_change_type(ID(r),x+rx,y+ry,PT_ICEI);
					}
				}
		break;
	case PT_SNOW:
		for (int rx = -2; rx <= 2; rx++)
			for (int ry = -2; ry <= 2; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_WATR || TYP(r)==PT_DSTW) && RNG::Ref().chance(1, 1000))
					{
						part_change_type(i,x,y,PT_ICEI);
						part_change_type(ID(r),x+rx,y+ry,PT_ICEI);
					}
					if ((TYP(r)==PT_WATR || TYP(r)==PT_DSTW) && RNG::Ref().chance(3, 200))
						part_change_type(i,x,y,PT_WATR);
				}
		break;
	case PT_OIL:
		if (sim->air->pv[y/CELL][x/CELL] < -6.0f)
			part_change_type(i,x,y,PT_GAS);
		break;
	case PT_GAS:
		if (sim->air->pv[y/CELL][x/CELL] > 6.0f)
			part_change_type(i,x,y,PT_OIL);
		break;
	case PT_DESL:
		if (sim->air->pv[y/CELL][x/CELL] > 12.0f)
		{
			part_change_type(i,x,y,PT_FIRE);
			parts[i].life = RNG::Ref().between(120, 169);
		}
	default:
		break;
	}
	return 0;
}

int graphics_DEFAULT(GRAPHICS_FUNC_ARGS)
{
	int t = cpart->type;
	if (sim->elements[t].Properties & PROP_RADIOACTIVE)
		*pixel_mode |= PMODE_GLOW;
	if (sim->elements[t].Properties & TYPE_LIQUID)
	{
		*pixel_mode |= PMODE_BLUR;
	}
	if (sim->elements[t].Properties & TYPE_GAS)
	{
		*pixel_mode &= ~PMODE;
		*pixel_mode |= FIRE_BLEND;
		*firer = *colr/2;
		*fireg = *colg/2;
		*fireb = *colb/2;
		*firea = 125;
		*pixel_mode |= DECO_FIRE;
	}
	return 1;
}

//very ugly function that somehow makes powered elements floodfill update and also maintain compatibility if not
int update_POWERED(UPDATE_FUNC_ARGS)
{
#ifndef NOMOD
	if (parts[i].type == PT_PPTI || parts[i].type == PT_PPTO)
	{
		if (parts[i].tmp2 > 0 && parts[i].tmp2 != 10)
			parts[i].tmp2--;
	}
	else if (parts[i].life > 0 && parts[i].life != 10)
		parts[i].life--;
#else
	if (parts[i].life > 0 && parts[i].life != 10)
			parts[i].life--;

#endif
	for (int rx = -2; rx <= 2; rx++)
		for (int ry = -2; ry <= 2; ry++)
			if (rx || ry)
			{
				int r = pmap[y+ry][x+rx];
				if (!r)
					continue;

				int pavg = parts_avg(i, ID(r), PT_INSL);
				bool insulated = pavg == PT_INSL || pavg == PT_RSSS;
#ifdef NOMOD
				if (parts[i].type != PT_SWCH || !insulated)
#else
				if ((parts[i].type != PT_SWCH && parts[i].type != PT_BUTN) || !insulated)
#endif
				{
					if (TYP(r)==parts[i].type && parts[i].type == PT_SWCH)
					{
						if (parts[i].life>=10&&parts[ID(r)].life>0&&parts[ID(r)].life<10)
							parts[i].life = 9;
						else if (parts[i].life==0&&parts[ID(r)].life>=10)
						{
							//Set to other particle's life instead of 10, otherwise spark loops form when SWCH is sparked while turning on
							parts[i].life = parts[ID(r)].life;
						}
					}
					if (TYP(r)==PT_SPRK && parts[ID(r)].life>0 && (parts[ID(r)].life<4 || parts[i].type == PT_SWCH))
					{
#ifndef NOMOD
						//Mod powered elements are always instantly activated
						if ((parts[i].type == PT_PPTI || parts[i].type == PT_PPTO) && ID(pmap[y][x]) == (unsigned)i)
						{
							if (parts[ID(r)].life>2)
							{
								if (parts[ID(r)].ctype==PT_PSCN && parts[i].tmp2 < 10)
								{
									sim->FloodProp(x, y, particle::PropertyByName("tmp2"), { 10 });
									sim->FloodProp(x, y, particle::PropertyByName("flags"), { parts[i].flags | FLAG_SKIPMOVE });
								}
								else if (parts[ID(r)].ctype==PT_NSCN && parts[i].tmp2 >= 10)
								{
									sim->FloodProp(x, y, particle::PropertyByName("tmp2"), { 9 });
									sim->FloodProp(x, y, particle::PropertyByName("flags"), { parts[i].flags | FLAG_SKIPMOVE });
								}
							}
						}
						else if (parts[i].type == PT_ANIM)
						{
							if (parts[ID(r)].life>2)
							{
								if (parts[ID(r)].ctype==PT_PSCN && parts[i].life < 10)
								{
									sim->FloodProp(x, y, particle::PropertyByName("life"), { 10 });
									sim->FloodProp(x, y, particle::PropertyByName("flags"), { parts[i].flags | FLAG_SKIPMOVE });
								}
								else if (parts[ID(r)].ctype==PT_NSCN && (parts[i].life >= 10 || parts[i].tmp != (int)(parts[i].temp-273.15) || parts[i].tmp2 > 1))
								{
									sim->FloodProp(x, y, particle::PropertyByName("life"), { 9 });
									sim->FloodProp(x, y, particle::PropertyByName("flags"), { parts[i].flags | FLAG_SKIPMOVE });
									sim->FloodProp(x, y, particle::PropertyByName("tmp"), { 0 });
									sim->FloodProp(x, y, particle::PropertyByName("tmp2"), { 0 });
								}
								else if (parts[ID(r)].ctype==PT_METL)
								{
									if (parts[i].life == 10)
									{
										sim->FloodProp(x, y, particle::PropertyByName("life"), { 9 });
									}
									else if (parts[i].life == 0)
									{
										sim->FloodProp(x, y, particle::PropertyByName("life"), { 14 });
										return 0;
									}
								}
							}
						}
						//element is instantly activated (mod elements are always instantly activated)
						else if ((sim->instantActivation || parts[i].type == PT_BUTN || parts[i].type == PT_PINV) && ID(pmap[y][x]) == (unsigned)i)
						{
							if (parts[ID(r)].ctype == PT_PSCN && parts[i].life < 10)
							{
								sim->FloodProp(x, y, particle::PropertyByName("life"), { 10 });
								sim->FloodProp(x, y, particle::PropertyByName("flags"), { parts[i].flags | FLAG_SKIPMOVE });
							}
							else if (parts[ID(r)].ctype == PT_NSCN && parts[i].life >= 10)
							{
								sim->FloodProp(x, y, particle::PropertyByName("life"), { 9 });
								sim->FloodProp(x, y, particle::PropertyByName("flags"), { parts[i].flags | FLAG_SKIPMOVE });
							}
							else if ((parts[i].type == PT_SWCH || parts[i].type == PT_BUTN) && parts[ID(r)].ctype != PT_PSCN && parts[ID(r)].ctype != PT_NSCN && !(parts[ID(r)].ctype == PT_INWR && parts[ID(r)].tmp == 1) && parts[i].life == 10)
							{
								sim->spark_conductive(i, x, y);
								return 1;
							}
						}
#else
						if (false) { }
#endif
						else
						{
							if ((parts[i].type == PT_PUMP || parts[i].type == PT_GPMP || parts[i].type == PT_HSWC || parts[i].type == PT_PBCN))
								continue;
							if (parts[i].type != PT_SWCH && parts[ID(r)].ctype == PT_PSCN && parts[i].life < 10)
								parts[i].life = 10;
							else if (parts[i].type != PT_SWCH && parts[ID(r)].ctype == PT_NSCN)
								parts[i].life = 9;
							else if (parts[i].type == PT_SWCH && parts[ID(r)].ctype != PT_PSCN && parts[ID(r)].ctype != PT_NSCN && !(parts[ID(r)].ctype == PT_INWR && parts[ID(r)].tmp == 1) && parts[i].life == 10)
							{
								sim->spark_conductive(i, x, y);
								return 1;
							}
						}
					}
					if (TYP(r)==parts[i].type && parts[i].type != PT_SWCH)
					{
						if (parts[i].life==10&&parts[ID(r)].life>0&&parts[ID(r)].life<10)
							parts[i].life = 9;
						else if (parts[i].life==0&&parts[ID(r)].life==10)
							parts[i].life = 10;
					}
				}
			}
	return 0;
}

bool basicCtypeDraw(CTYPEDRAW_FUNC_ARGS)
{
	if (sim->parts[i].type == t || sim->elements[t].Properties & PROP_NOCTYPEDRAW)
		return false;
	sim->parts[i].ctype = t;
	return true;
}

bool ctypeDrawVInTmp(CTYPEDRAW_FUNC_ARGS)
{
	if (!basicCtypeDraw(CTYPEDRAW_FUNC_SUBCALL_ARGS))
		return false;
	if (t == PT_LIFE)
		sim->parts[i].tmp = v;
	return true;
}

bool ctypeDrawVInCtype(CTYPEDRAW_FUNC_ARGS)
{
	if (!basicCtypeDraw(CTYPEDRAW_FUNC_SUBCALL_ARGS))
		return false;
	if (t == PT_LIFE)
		sim->parts[i].ctype |= PMAPID(v);
	return true;
}
