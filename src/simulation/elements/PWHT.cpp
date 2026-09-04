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

#ifndef NOMOD
#include "simulation/ElementsCommon.h"

int PWHT_flood(UPDATE_FUNC_ARGS)
{
	StructProperty prop;

	if (parts[i].ctype == 1)
		prop = particle::PropertyByName("life");
	else if (parts[i].ctype == 2)
		prop = particle::PropertyByName("ctype");
	else if (parts[i].ctype == 3)
		prop = particle::PropertyByName("type");
	else if (parts[i].ctype == 4)
		prop = particle::PropertyByName("tmp");
	else if (parts[i].ctype == 5)
		prop = particle::PropertyByName("tmp2");
	else if (parts[i].ctype == 6)
		prop = particle::PropertyByName("vy");
	else if (parts[i].ctype == 7)
		prop = particle::PropertyByName("vx");
	else if (parts[i].ctype == 8)
		prop = particle::PropertyByName("x");
	else if (parts[i].ctype == 9)
		prop = particle::PropertyByName("y");
	else if (parts[i].ctype == 10)
		prop = particle::PropertyByName("dcolour");
	else if (parts[i].ctype == 11)
		prop = particle::PropertyByName("flags");
	else if (parts[i].ctype == 12)
		prop = particle::PropertyByName("tmp3");
	else if (parts[i].ctype == 13)
		prop = particle::PropertyByName("tmp4");
	else
	{
		parts[i].ctype = 0;
		return 0;
	}


	if (prop.Type == StructProperty::Float)
	{
		PropertyValue valuef;
		valuef.Float = parts[i].temp;
		return sim->FloodProp(x, y - 1, prop, valuef);
	}
	else if (prop.Type == StructProperty::UInteger)
	{
		PropertyValue valueui;
		valueui.UInteger = parts[i].dcolour;
		return sim->FloodProp(x, y - 1, prop, valueui);
	}
	else
	{
		return sim->FloodProp(x, y - 1, prop, { parts[i].tmp });
	}
}

int PWHT_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life < 10)
		return 0;
	int r = pmap[y-1][x];
	if TYP(r)
	{
		if (!parts[i].ctype)
		{
			PropertyValue valuef;
			valuef.Float = parts[i].temp;
			sim->FloodProp(x, y - 1, particle::PropertyByName("temp"), valuef);
		}
		else
		{
			PWHT_flood(UPDATE_FUNC_SUBCALL_ARGS);
		}
	}
	return 0;
}

int PWHT_graphics(GRAPHICS_FUNC_ARGS)
{
	int lifemod = ((cpart->life>10?10:cpart->life)*19);
	*colb += lifemod;
	return 0;
}

bool PWHT_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return (y > 0 && TYP(pmap[y-1][x]) != PT_PWHT) && (y < YRES-1 && TYP(pmap[y+1][x]) != PT_PWHT);
}

void PWHT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PWHT";
	elem->Name = "PWHT";
	elem->Colour = COLPACK(0x381D7C);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "可控加热器，以填充方式把粒子加热到自身温度。只需使用一个。";
	elem->DetailedDescription = "描述：可控洪泛属性写入器，默认用自身温度一次性修改紧贴其上方、彼此连通的整片粒子；PSCN 开启，NSCN 关闭。它不是逐点传热，因此一个 PWHT 就能瞬间处理大面积连通区域。\n参数：Ctype=0 写入温度；1=Life，2=Ctype，3=Type，4=Tmp，5=Tmp2，6=Vy，7=Vx，8=X，9=Y，10=装饰色，11=Flags，12=Tmp3，13=Tmp4。写入浮点属性时数值取 PWHT 温度；普通整数属性取 PWHT 的 Tmp；装饰色取 PWHT 自身装饰色。\n放置限制：PWHT 正上方和正下方不能紧邻另一个 PWHT。修改 Type、坐标或标志等底层属性可能破坏结构，应先在副本中测试。\n导热率：0\n初始状态：Life=10(开启)；初始温度 22℃/295.15K";

	elem->Properties = TYPE_SOLID|PROP_POWERED;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 10;

	elem->Update = &PWHT_update;
	elem->Graphics = &PWHT_graphics;
	elem->Func_Create_Allowed = &PWHT_create_allowed;
	elem->Init = &PWHT_init_element;
}

#endif
