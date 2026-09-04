/**
 * Powder Toy - simple console
 *
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

#include <cmath>
#include <cstring>
#include <sstream>
#include "interface.h"
#include "powder.h"
#include "legacy_console.h"
#include "common/Format.h"
#include "simulation/Simulation.h"
#include "simulation/WallNumbers.h"
#include "simulation/ElementDataContainer.h"

char console_more=0;
int file_script = 0;

// putting in here since it is only used in this file.
#if defined(WIN) && !defined(strcasecmp)
#ifdef _MSC_VER
#define strcasecmp _stricmp //stricmp is deprecated in visual studio
#else
#define strcasecmp stricmp
#endif
#endif

//takes a a string and compares it to element names, and puts it value into element.
int console_parse_type(const char *txt, int *element, char *err, Simulation *sim)
{
	int i = atoi(txt);
	// alternative names for some elements
	if (!strcasecmp(txt, "C4"))
		i = PT_PLEX;
	else if (!strcasecmp(txt, "C5"))
		i = PT_C5;
	else if (!strcasecmp(txt, "NONE"))
		i = PT_NONE;
	else if (!strcasecmp(txt, "EXPL") && !explUnlocked)
	{
		if (err)
	strcpy(err, "无法识别粒子类型");
		return 0;
	}
	for (int j = 1; j < PT_NUM; j++)
	{
		if (!strcasecmp(txt, sim->elements[j].Name.c_str()) && sim->elements[j].Enabled)
		{
			if (element) *element = j;
			if (err) strcpy(err,"");
			return 1;
		}
	}
	if ((i > 0 && i < PT_NUM && sim->elements[i].Enabled) || !strcasecmp(txt, "NONE") || !strcasecmp(txt, "0"))
	{
		if (element) *element = i;
		if (err) strcpy(err,"");
		return 1;
	}
	if (err) strcpy(err, "无法识别粒子类型");
	return 0;
}
//takes a string and compares it to wall names, and puts it's value into *wall.
int console_parse_wall_type(const char *txt, int *wall)
{
	int i;
	for (i = 0; i < WALLCOUNT; i++) {
		if (strcasecmp(txt,wallTypes[i].name.c_str())==0 && wallTypes[i].drawstyle != -1)
		{
			*wall = i;
			return 1;
		}
	}
	return 0;
}
//takes a string of coords "x,y" and puts the values into x and y.
int console_parse_coords(const char *txt, int *x, int *y, char *err)
{
	int nx = -1, ny = -1;
	if (sscanf(txt,"%d,%d",&nx,&ny)!=2 || nx<0 || nx>=XRES || ny<0 || ny>=YRES)
	{
	if (err) strcpy(err,"坐标无效");
		return 0;
	}
	*x = nx;
	*y = ny;
	return 1;
}

//takes things like 100C or 212F into account
float console_parse_temp(std::string temperature)
{
	try
	{
		float temp = Format::StringToTemperature(temperature, globalSim->temperatureScale);
		return restrict_flt(temp, MIN_TEMP, MAX_TEMP);
	}
	catch (const std::exception &e)
	{
		return -1;
	}
}

//takes a string of either coords or a particle number, and puts the particle number into *which
int console_parse_partref(const char *txt, int *which, char *err)
{
	int i = -1, nx, ny;
	if (err) strcpy(err,"");
	if (strchr(txt,',') && console_parse_coords(txt, &nx, &ny, err))
	{
		i = pmap[ny][nx];
		if (!i)
			i = -1;
		else
			i = ID(i);
	}
	else if (txt)
	{
		char *num = (char*)malloc(strlen(txt)+3);
		i = atoi(txt);
		sprintf(num,"%d",i);
		if (!txt || strcmp(txt,num)!=0)
			i = -1;
		free(num);
	}
	if (i>=0 && i<NPART && parts[i].type)
	{
		*which = i;
		if (err) strcpy(err,"");
		return 1;
	}
	if (err && strcmp(err,"")==0) strcpy(err,"粒子不存在");
	return 0;
}

bool console_parse_hex(char *txt, int *val, char *err)
{
	int base = 10;
	char c;
	if (txt[0] == '#')
	{
		txt++;
		base = 16;
	}
	if (txt[0] == '0' && txt[1] == 'x')
	{
		txt += 2;
		base = 16;
	}
	while ((c = *(txt++)))
	{
		*val *= base;
		if (c >= '0' && c <= '9')
			*val += c - '0';
		else if (base == 16 && c >= 'a' && c <= 'f')
			*val += (c - 'a') + 10;
		else if (base == 16 && c >= 'A' && c <= 'F')
			*val += (c - 'A') + 10;
		else
		{
			if (err)
			{
				if (base == 10)
					strcpy(err, "无效的数字");
				else
	strcpy(err, "十六进制数无效");
			}
			return false;
		}
	}
	return true;
}

int process_command_old(Simulation * sim, pixel *vid_buf, const char *command, std::string *result)
{
	int y,x,nx,ny,i,j,k,m;
	float f;
	int do_next = 1;
	char xcoord[10] = "";
	char ycoord[10] = "";
	char console_error[255] = "";
	char console2[15] = "";
	char console3[15] = "";
	char console4[15] = "";
	char console5[15] = "";
	if (command && strcmp(command, "")!=0 && strncmp(command, " ", 1)!=0)
	{
		sscanf(command,"%14s %14s %14s %14s", console2, console3, console4, console5);//why didn't i know about this function?!
		if (strcmp(console2, "quit")==0)
		{
			return -2;
		}
		else if (strcmp(console2, "file")==0 && console3[0])
		{
			if (file_script) {
				int filesize;
				char *fileread = (char*)file_load(console3, &filesize);
				nx = 0;
				ny = 0;
				if (console4[0] && !console_parse_coords(console4, &nx , &ny, console_error))
				{
					free(fileread);
					return 1;
				}
				if (fileread)
				{
					char pch[501];
					char tokens[31];
					int tokensize;
					j = 0; // line start position in fileread
					m = 0; // token start position in fileread
					memset(pch,0,sizeof(pch));
					for (i=0; i<filesize; i++)
					{
						if (fileread[i] != '\n' && i-m<30)
						{
							pch[i-j] = fileread[i];
							if (fileread[i] != ' ')
								tokens[i-m] = fileread[i];
						}
						if ((fileread[i] == ' ' || fileread[i] == '\n') && i-j<400)
						{
							if (sregexp(tokens, "^x.\\{0,1\\}[0-9]*,y.\\{0,1\\}[0-9]*")==0)
							{
								int starty = 0;
								tokensize = strlen(tokens);
								x = 0;
								y = 0;
								if (tokens[1]!=',')
									sscanf(tokens,"x%d,y%d",&x,&y);
								else
									sscanf(tokens,"x,y%d",&y);
								x += nx;
								y += ny;
								sprintf(xcoord,"%d",x);
								sprintf(ycoord,"%d",y);
								for (unsigned int k = 0; k<strlen(xcoord); k++)//rewrite pch with numbers
								{
									pch[i-j-tokensize+k] = xcoord[k];
									starty = k+1;
								}
								pch[i-j-tokensize+starty] = ',';
								starty++;
								for (unsigned int k = 0; k < strlen(ycoord); k++)
								{
									pch[i-j-tokensize+starty+k] = ycoord[k];
								}
								pch[i-j-tokensize +strlen(xcoord) +1 +strlen(ycoord)] = ' ';
								j = j -tokensize +strlen(xcoord) +1 +strlen(ycoord);
							}
							memset(tokens,0,sizeof(tokens));
							m = i+1;
						}
						if (fileread[i] == '\n')
						{
							
							if (do_next)
							{
								if (strcmp(pch,"else")==0)
									do_next = 0;
								else
									do_next = process_command_old(sim, vid_buf, pch, result);
							}
							else if (strcmp(pch,"endif")==0 || strcmp(pch,"else")==0)
								do_next = 1;
							memset(pch,0,sizeof(pch));
							j = i+1;
						}
					}
					free(fileread);
				}
				else
				{
	sprintf(console_error, "%s 不存在", console3);
				}
			}
			else
			{
	sprintf(console_error, "脚本功能未启用");
			}
			
		}
			else if (strcmp(console2, "load")==0 && console3[0])
			{
				j = atoi(console3);
				if (j)
				{
					if (open_ui(vid_buf, console3, nullptr, 0))
						return -1;
				}
			}
			else if (strcmp(console2, "if")==0 && console3[0])
			{
				if (strcmp(console3, "type")==0)//TODO: add more than just type, and be able to check greater/less than
				{
					if (console_parse_partref(console4, &i, console_error)
					    && console_parse_type(console5, &j, console_error, sim))
					{
						if (parts[i].type==j)
							return 1;
						else
							return 0;
					}
					else
						return 0;
				}
			}
			else if (strcmp(console2, "create")==0 && console3[0] && console4[0])
			{
				if (console_parse_type(console3, &j, console_error, sim)
			        && console_parse_coords(console4, &nx, &ny, console_error))
				{
					if (!j)
	strcpy(console_error, "无法创建 NONE 类型粒子");
					else
					{
						int v = -1;
						if (j&~PMAPMASK)
						{
							v = ID(j);
							j = TYP(j);
						}
						if (sim->part_create(-1, nx, ny, j, v) < 0)
	strcpy(console_error, "无法创建粒子");
					}
				}
			}
			else if (strcmp(console2, "bubble")==0 && console3[0])
			{
				if (console_parse_coords(console3, &nx, &ny, console_error))
				{
					int first, rem1, rem2;

					first = sim->part_create(-1, nx+18, ny, PT_SOAP);
					rem1 = first;

					for (i = 1; i<=30; i++)
					{
						rem2 = sim->part_create(-1, (int)(nx+18*cosf(i/5.0f)), (int)(ny+18*sinf(i/5.0f)), PT_SOAP);

						if (rem1 != -1 && rem2 != -1)
						{
							parts[rem1].ctype = 7;
							parts[rem1].tmp = rem2;
							parts[rem2].tmp2 = rem1;
						}

						rem1 = rem2;
					}

					if (rem1 != -1 && first != -1)
					{
						parts[rem1].ctype = 7;
						parts[rem1].tmp = first;
						parts[first].tmp2 = rem1;
						parts[first].ctype = 7;
					}
				}
			}
			else if ((strcmp(console2, "delete")==0 || strcmp(console2, "kill")==0) && console3[0])
			{
				if (console_parse_partref(console3, &i, console_error))
					sim->part_kill(i);
			}
			else if (strcmp(console2, "reset")==0 && console3[0])
			{
				if (strcmp(console3, "pressure")==0)
				{
					for (nx = 0; nx<XRES/CELL; nx++)
						for (ny = 0; ny<YRES/CELL; ny++)
						{
							sim->air->pv[ny][nx] = 0;
						}
				}
				else if (strcmp(console3, "velocity")==0)
				{
					for (nx = 0; nx<XRES/CELL; nx++)
						for (ny = 0; ny<YRES/CELL; ny++)
						{
							sim->air->vx[ny][nx] = 0;
							sim->air->vy[ny][nx] = 0;
						}
				}
				else if (strcmp(console3, "sparks")==0)
				{
					for (int i = 0; i < NPART; i++)
						if (parts[i].type == PT_SPRK)
						{
							if (parts[i].ctype >= 0 && parts[i].ctype < PT_NUM && sim->elements[parts[i].ctype].Enabled)
							{
								parts[i].type = parts[i].ctype;
								parts[i].life = parts[i].ctype = 0;
							}
							else
								sim->part_kill(i);
						}
					sim->elementData[PT_WIFI]->Simulation_Cleared(sim);
				}
				else if (strcmp(console3, "temp")==0)
				{
					for (i=0; i<NPART; i++)
					{
						if (parts[i].type)
						{
							parts[i].temp = sim->elements[parts[i].type].DefaultProperties.temp;
						}
					}
				}
			}
			else if (strcmp(console2, "set")==0 && console3[0] && console4[0] && console5[0])
			{
				if (strcmp(console3, "life")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].life = j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].life = k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].life = j;
						}
					}
				}
				else if (strcmp(console3, "type")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						if (console_parse_type(console5, &j, console_error, sim))
							for (i=0; i<NPART; i++)
							{
								if (parts[i].type)
									sim->part_change_type_force(i, j);
							}
					}
					else if (console_parse_type(console4, &j, console_error, sim) && j != 0
							 && console_parse_type(console5, &k, console_error, sim))
					{
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								sim->part_change_type_force(i, k);
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error) && j != 0
						    && console_parse_type(console5, &j, console_error, sim))
						{
							sim->part_change_type_force(i, j);
						}
					}
				}
				else if (strcmp(console3, "temp")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						f = console_parse_temp(console5);
						if (f >= 0)
							for (i=0; i<NPART; i++)
							{
								if (parts[i].type)
									parts[i].temp = f;
							}
						else
	strcpy(console_error, "温度无效");
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						f = console_parse_temp(console5);
						if (f >= 0)
							for (i=0; i<NPART; i++)
							{
								if (parts[i].type == j)
									parts[i].temp= f;
							}
						else
	strcpy(console_error, "温度无效");
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							f = console_parse_temp(console5);
							if (f >= 0)
								parts[i].temp = f;
							else
	strcpy(console_error, "温度无效");
						}
					}
				}
				else if (strcmp(console3, "tmp")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].tmp = j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].tmp = k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].tmp = j;
						}
					}
				}
				else if (strcmp(console3, "tmp2")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].tmp2 = j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].tmp2 = k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].tmp2 = j;
						}
					}
				}
				else if (strcmp(console3, "x")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].x = (float)j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].x = (float)k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].x = (float)j;
						}
					}
				}
				else if (strcmp(console3, "y")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].y = (float)j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].y = (float)k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].y = (float)j;
						}
					}
				}
				else if (strcmp(console3, "ctype")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						if (console_parse_type(console5, &j, console_error, sim) || (j = atoi(console5)))
						{
							strcpy(console_error, "");
							for (i=0; i<NPART; i++)
							{
								if (parts[i].type)
									parts[i].ctype = j;
							}
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						if (console_parse_type(console5, &k, console_error, sim) || (k = atoi(console5)))
						{
							strcpy(console_error, "");
							for (i=0; i<NPART; i++)
							{
								if (parts[i].type == j)
									parts[i].ctype = k;
							}
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							if (console_parse_type(console5, &j, console_error, sim) || (j = atoi(console5)))
							{
								strcpy(console_error, "");
								j = atoi(console5);
								parts[i].ctype = j;
							}
						}
					}
				}
				else if (strcmp(console3, "vx")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						f = (float)atof(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].vx = f;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						f = (float)atof(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].vx = f;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							f = (float)atof(console5);
							parts[i].vx = f;
						}
					}
				}
				else if (strcmp(console3, "vy")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						f = (float)atof(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].vy = f;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						f = (float)atof(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].vy = f;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							f = (float)atof(console5);
							parts[i].vy = f;
						}
					}
				}
				else if (strcmp(console3, "dcolor")==0 || strcmp(console3, "dcolour")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						if (console_parse_hex(console5, &j, console_error))
							for (i=0; i<NPART; i++)
							{
								if (parts[i].type)
									parts[i].dcolour = j;
							}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						if (console_parse_hex(console5, &k, console_error))
							for (i=0; i<NPART; i++)
							{
								if (parts[i].type == j)
									parts[i].dcolour = k;
							}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							if (console_parse_hex(console5, &j, console_error))
								parts[i].dcolour = j;
						}
					}
				}
				else if (strcmp(console3, "flags")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].flags = j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].flags = k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].flags = j;
						}
					}
				}
				else if (strcmp(console3, "tmp3")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].tmp3 = j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].tmp3 = k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].tmp3 = j;
						}
					}
				}
				else if (strcmp(console3, "tmp4")==0)
				{
					if (strcmp(console4, "all")==0)
					{
						j = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type)
								parts[i].tmp4 = j;
						}
					}
					else if (console_parse_type(console4, &j, console_error, sim))
					{
						k = atoi(console5);
						for (i=0; i<NPART; i++)
						{
							if (parts[i].type == j)
								parts[i].tmp4 = k;
						}
					}
					else
					{
						if (console_parse_partref(console4, &i, console_error))
						{
							j = atoi(console5);
							parts[i].tmp4 = j;
						}
					}
				}
				else
	strcpy(console_error, "属性无效");
			}
			else
	strcpy(console_error, "命令无效");
	}
	*result = console_error;
	return 1;
}
