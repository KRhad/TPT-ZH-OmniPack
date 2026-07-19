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

#ifdef CONVERTELEMENTS //prevent this file from automatically being compiled when people add every file they can find to Visual Studio

/* This program reads src/elementdata.c and includes/powder.h 
 * (relative to the current working directory) and puts all the element
 * data into the new format with named properties.
 * 
 * Before using this, remove the #ifdef REALISTIC bit from elementdata.c
 * 
 * Afterwards, you will need to manually copy and edit the update and 
 * graphics functions. 
 */


#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>

#ifdef WIN
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

void outputLicenseHeader(std::ostream& outputStream)
{
	outputStream << 
	"/*" << std::endl <<
	" * This program is free software; you can redistribute it and/or modify" << std::endl <<
	" * it under the terms of the GNU General Public License as published by" << std::endl <<
	" * the Free Software Foundation; either version 3 of the License, or" << std::endl <<
	" * (at your option) any later version." << std::endl <<
	" *" << std::endl <<
	" * This program is distributed in the hope that it will be useful," << std::endl <<
	" * but WITHOUT ANY WARRANTY; without even the implied warranty of" << std::endl <<
	" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" << std::endl <<
	" * GNU General Public License for more details." << std::endl <<
	" *" << std::endl <<
	" * You should have received a copy of the GNU General Public License" << std::endl <<
	" * along with this program.  If not, see <http://www.gnu.org/licenses/>." << std::endl <<
	" */" << std::endl << std::endl;
}

std::string trimWhitespace(std::string inputStr)
{
	std::string whitespaceChars(" \t");
	size_t textStart = inputStr.find_first_not_of(whitespaceChars);
	size_t textEnd = inputStr.find_last_not_of(whitespaceChars);
	if (textStart!=std::string::npos && textEnd!=std::string::npos)
		return inputStr.substr(textStart, textEnd+1-textStart);
	else
		return inputStr;
}

bool readDataArrayRow(std::istream& inputStream, std::vector<std::string>& returnData)
{
	returnData.resize(0);
	bool inComment = false;
	bool startedWithBracket = false;
	while (inputStream.good())
	{
		char c = inputStream.get();
		if (c=='/' && inputStream.peek()=='*')
		{
			inputStream.get();
			inComment = true;
		}
		else if (inComment && c=='*' && inputStream.peek()=='/')
		{
			inputStream.get();
			inComment = false;
		}
		else if (c=='/' && inputStream.peek()=='/')
		{
			std::string tmp;
			std::getline(inputStream, tmp);
		}
		else if (!inComment && c=='}')
		{
			return false;
		}
		else if (!inComment && c!=' ' && c!='\t' && c!='\r' && c!='\n')
		{
			if (c=='{') startedWithBracket = true;
			else if (c!='/') inputStream.putback(c);
			break;
		}
	}
	bool done = false;
	while (inputStream.good() && !done)
	{
		std::string dataItem;
		bool inString = false;
		int bracketLevel = 0;
		char prevC = 0;
		while (inputStream.good())
		{
			char c = inputStream.get();
			if (c=='"' && prevC!='\\')
			{
				inString = !inString;
			}
			if (c=='{' && !inString)
			{
				bracketLevel++;
			}
			if (c=='}' && !inString)
			{
				if (bracketLevel==0)
				{
					done = true;
					break;
				}
				bracketLevel--;
			}
			if (c==',' && !inString)
			{
				if (!startedWithBracket)
				{
					done = true;
				}
				break;
			}
			prevC = c;
			dataItem += c;
		}
		returnData.push_back(trimWhitespace(dataItem));
	}
	if (startedWithBracket)
	{
		while (inputStream.good() && inputStream.peek()!='}')
		{
			if (inputStream.get()==',')
				break;
		}
	}
	return true;
}

class part_type
{
public:
	std::string CodeName;

	std::string Name;
	std::string Colour;
	std::string Advection;
	std::string AirDrag;
	std::string AirLoss;
	std::string Loss;
	std::string Collision;
	std::string Gravity;
	std::string Diffusion;
	std::string PressureAdd_NoAmbHeat;
	std::string Falldown;
	std::string Flammable;
	std::string Explosive;
	std::string Meltable;
	std::string Hardness;
	std::string MenuVisible;
	std::string Enabled;
	std::string Weight;
	std::string MenuSection;
	std::string CreationTemperature;
	std::string HeatConduct;
	std::string Description;
	std::string State;
	std::string Properties;
	std::string Update;
	std::string Graphics;
	
	std::string LowPressureTransitionThreshold;
	std::string LowPressureTransitionElement;
	std::string HighPressureTransitionThreshold;
	std::string HighPressureTransitionElement;
	std::string LowTemperatureTransitionThreshold;
	std::string LowTemperatureTransitionElement;
	std::string HighTemperatureTransitionThreshold;
	std::string HighTemperatureTransitionElement;

	std::string Latent;
};

int main()
{
	std::ifstream powderhStream, elementdataStream;
	powderhStream.open("includes/powder.h");
	elementdataStream.open("src/elementdata.c");
	if (!powderhStream.is_open())
	{
		std::cerr << "Error: could not open includes/powder.h" << std::endl;
		return 1;
	}
	if (!elementdataStream.is_open())
	{
		std::cerr << "Error: could not open src/elementdata.c" << std::endl;
		return 1;
	}
#ifdef WIN32
	_mkdir("src");
	_mkdir("src\\simulation");
	_mkdir("src\\simulation\\elements");
#else
	mkdir("src", 0755);
	mkdir("src/simulation", 0755);
	mkdir("src/simulation/elements", 0755);
#endif
	
	std::vector<part_type> elementData;

	// Read element identifiers from powder.h
	std::string dataline;
	bool modElements = false;
	while (powderhStream.good())
	{
		//Read a line from the file
		std::getline(powderhStream, dataline);
		std::stringstream datalineStream(dataline);
		
		//Look for lines beginning with #define
		std::string firstWord;
		datalineStream >> firstWord;
		if (firstWord!="#define")
			continue;
		
		//Parse the #define, look for ones starting with PT_
		std::string defineName, defineValue;
		unsigned elementId = 0;
		datalineStream >> defineName >> defineValue;
		if (defineName.compare(0, 3, "PT_"))
			continue;
		if (defineName=="PT_NUM")
			continue;
		if (defineName=="PT_NORMAL_NUM")
		{
			modElements = true;
			continue;
		}
		std::stringstream(defineValue) >> elementId;

		if (elementId >= elementData.size())
			elementData.resize(elementId+1);
		else if (modElements)
		{
			elementId = elementData.size();
			elementData.resize(elementData.size()+1);
		}
		elementData[elementId].CodeName = defineName.substr(3);
	}

	// Read data from elementdata.c
	while (elementdataStream.good())
	{
		std::getline(elementdataStream, dataline);
		if (dataline.find("ptypes[PT_NUM] =") != std::string::npos)
		{
			// ptypes data
			while (elementdataStream.good())
			{
				// Find the { indicating the start of the data
				if (elementdataStream.get()=='{') break;
			}
			std::vector<std::string> dataRow;
			unsigned elementId = 0;
			while (readDataArrayRow(elementdataStream, dataRow))
			{
				if (dataRow.size()<26)
				{
					std::cerr << "Error: element data row " << elementId << " not long enough. Make sure you remove the #ifdef REALISTIC stuff from elementdata.c" << std::endl;
					exit(1);
				}
				if (elementId >= elementData.size())
					elementData.resize(elementId+1);

				elementData[elementId].Name = dataRow[0];
				elementData[elementId].Colour = dataRow[1];
				elementData[elementId].Advection = dataRow[2];
				elementData[elementId].AirDrag = dataRow[3];
				elementData[elementId].AirLoss = dataRow[4];
				elementData[elementId].Loss = dataRow[5];
				elementData[elementId].Collision = dataRow[6];
				elementData[elementId].Gravity = dataRow[7];
				elementData[elementId].Diffusion = dataRow[8];
				elementData[elementId].PressureAdd_NoAmbHeat = dataRow[9];
				elementData[elementId].Falldown = dataRow[10];
				elementData[elementId].Flammable = dataRow[11];
				elementData[elementId].Explosive = dataRow[12];
				elementData[elementId].Meltable = dataRow[13];
				elementData[elementId].Hardness = dataRow[14];
				elementData[elementId].MenuVisible = dataRow[15];
				elementData[elementId].Enabled = dataRow[16];
				elementData[elementId].Weight = dataRow[17];
				elementData[elementId].MenuSection = dataRow[18];
				elementData[elementId].CreationTemperature = dataRow[19];
				elementData[elementId].HeatConduct = dataRow[20];
				elementData[elementId].Description = dataRow[21];
				elementData[elementId].State = dataRow[22];
				elementData[elementId].Properties = dataRow[23];
				elementData[elementId].Update = dataRow[24];
				elementData[elementId].Graphics = dataRow[25];

				elementId++;
			}
		}
		else if (dataline.find("ptransitions[PT_NUM] =") != std::string::npos)
		{
			// ptransitions data
			while (elementdataStream.good())
			{
				// Find the { indicating the start of the data
				if (elementdataStream.get()=='{') break;
			}
			std::vector<std::string> dataRow;
			unsigned elementId = 0;
			while (readDataArrayRow(elementdataStream, dataRow))
			{
				if (dataRow.size()<8)
				{
					std::cerr << "Error: ptransitions data row not long enough" << std::endl;
					exit(1);
				}
				if (elementId >= elementData.size())
					elementData.resize(elementId);

				elementData[elementId].LowPressureTransitionThreshold = dataRow[0];
				elementData[elementId].LowPressureTransitionElement = dataRow[1];
				elementData[elementId].HighPressureTransitionThreshold = dataRow[2];
				elementData[elementId].HighPressureTransitionElement = dataRow[3];
				elementData[elementId].LowTemperatureTransitionThreshold = dataRow[4];
				elementData[elementId].LowTemperatureTransitionElement = dataRow[5];
				elementData[elementId].HighTemperatureTransitionThreshold = dataRow[6];
				elementData[elementId].HighTemperatureTransitionElement = dataRow[7];

				elementId++;
			}
		}
		else if (dataline.find("platent[PT_NUM] =") != std::string::npos)
		{
			// platent data
			while (elementdataStream.good())
			{
				// Find the { indicating the start of the data
				if (elementdataStream.get()=='{') break;
			}
			std::vector<std::string> dataRow;
			unsigned elementId = 0;
			while (readDataArrayRow(elementdataStream, dataRow))
			{
				if (dataRow.size()<1)
				{
					std::cerr << "Error: platent data row not long enough" << std::endl;
					exit(1);
				}
				if (elementId >= elementData.size())
					elementData.resize(elementId);
				elementData[elementId].Latent = dataRow[0];
				elementId++;
			}
		}
	}

	//Finished reading element data, now create new element files
	for (unsigned elementId=0; elementId<elementData.size(); elementId++)
	{
		if (elementData[elementId].CodeName=="")
			continue;

		std::ofstream elementFile;
		std::string filename = "src/simulation/elements/" + elementData[elementId].CodeName + ".cpp";
		elementFile.open(filename.c_str(), std::ios::trunc);
		if (!elementFile.is_open() || !elementFile.good())
		{
			std::cerr << "Could not open " << filename << " for writing, skipping" << std::endl;
			continue;
		}
		outputLicenseHeader(elementFile);

		elementFile << "#include \"simulation/ElementsCommon.h\"" << std::endl << std::endl;

		elementFile << "void " << elementData[elementId].CodeName << "_init_element(ELEMENT_INIT_FUNC_ARGS)" << std::endl;
		elementFile << "{" << std::endl;
		std::string initPropertyPrefix = "\telem->"; // string to put at the start of each line defining a property

		elementFile << initPropertyPrefix << "Identifier = \"DEFAULT_PT_" << elementData[elementId].CodeName << "\";" << std::endl;
		elementFile << initPropertyPrefix << "Name = " << elementData[elementId].Name << ";" << std::endl;
		if (elementData[elementId].Colour.compare(0, 8, "PIXPACK(")==0)
		{
			elementFile << initPropertyPrefix << "Colour = COL" << elementData[elementId].Colour.substr(std::string("PIX").length(), std::string::npos) << ";" << std::endl;
		}
		else
		{
			elementFile << initPropertyPrefix << "Colour = " << elementData[elementId].Colour << ";" << std::endl;
		}
		elementFile << initPropertyPrefix << "MenuVisible = " << elementData[elementId].MenuVisible << ";" << std::endl;
		elementFile << initPropertyPrefix << "MenuSection = " << elementData[elementId].MenuSection << ";" << std::endl;
		elementFile << initPropertyPrefix << "Enabled = " << elementData[elementId].Enabled << ";" << std::endl;
		elementFile << std::endl;
		elementFile << initPropertyPrefix << "Advection = " << elementData[elementId].Advection << ";" << std::endl;
		elementFile << initPropertyPrefix << "AirDrag = " << elementData[elementId].AirDrag << ";" << std::endl;
		elementFile << initPropertyPrefix << "AirLoss = " << elementData[elementId].AirLoss << ";" << std::endl;
		elementFile << initPropertyPrefix << "Loss = " << elementData[elementId].Loss << ";" << std::endl;
		elementFile << initPropertyPrefix << "Collision = " << elementData[elementId].Collision << ";" << std::endl;
		elementFile << initPropertyPrefix << "Gravity = " << elementData[elementId].Gravity << ";" << std::endl;
		elementFile << initPropertyPrefix << "Diffusion = " << elementData[elementId].Diffusion << ";" << std::endl;
		elementFile << initPropertyPrefix << "PressureAdd_NoAmbHeat = " << elementData[elementId].PressureAdd_NoAmbHeat << ";" << std::endl;
		elementFile << initPropertyPrefix << "Falldown = " << elementData[elementId].Falldown << ";" << std::endl;
		elementFile << std::endl;
		elementFile << initPropertyPrefix << "Flammable = " << elementData[elementId].Flammable << ";" << std::endl;
		elementFile << initPropertyPrefix << "Explosive = " << elementData[elementId].Explosive << ";" << std::endl;
		elementFile << initPropertyPrefix << "Meltable = " << elementData[elementId].Meltable << ";" << std::endl;
		elementFile << initPropertyPrefix << "Hardness = " << elementData[elementId].Hardness << ";" << std::endl;
		elementFile << std::endl;
		elementFile << initPropertyPrefix << "Weight = " << elementData[elementId].Weight << ";" << std::endl;
		elementFile << std::endl;
		elementFile << initPropertyPrefix << "DefaultProperties.temp = " << elementData[elementId].CreationTemperature << ";" << std::endl;
		elementFile << initPropertyPrefix << "HeatConduct = " << elementData[elementId].HeatConduct << ";" << std::endl;
		elementFile << initPropertyPrefix << "Latent = " << elementData[elementId].Latent << ";" << std::endl;
		elementFile << initPropertyPrefix << "Description = " << elementData[elementId].Description << ";" << std::endl;
		elementFile << std::endl;
		elementFile << initPropertyPrefix << "State = " << elementData[elementId].State << ";" << std::endl;
		elementFile << initPropertyPrefix << "Properties = " << elementData[elementId].Properties << ";" << std::endl;
		elementFile << std::endl;
		elementFile << initPropertyPrefix << "LowPressureTransitionThreshold = " << elementData[elementId].LowPressureTransitionThreshold << ";" << std::endl;
		elementFile << initPropertyPrefix << "LowPressureTransitionElement = " << elementData[elementId].LowPressureTransitionElement << ";" << std::endl;
		elementFile << initPropertyPrefix << "HighPressureTransitionThreshold = " << elementData[elementId].HighPressureTransitionThreshold << ";" << std::endl;
		elementFile << initPropertyPrefix << "HighPressureTransitionElement = " << elementData[elementId].HighPressureTransitionElement << ";" << std::endl;
		elementFile << initPropertyPrefix << "LowTemperatureTransitionThreshold = " << elementData[elementId].LowTemperatureTransitionThreshold << ";" << std::endl;
		elementFile << initPropertyPrefix << "LowTemperatureTransitionElement = " << elementData[elementId].LowTemperatureTransitionElement << ";" << std::endl;
		elementFile << initPropertyPrefix << "HighTemperatureTransitionThreshold = " << elementData[elementId].HighTemperatureTransitionThreshold << ";" << std::endl;
		elementFile << initPropertyPrefix << "HighTemperatureTransitionElement = " << elementData[elementId].HighTemperatureTransitionElement << ";" << std::endl;
		elementFile << std::endl;
		elementFile << initPropertyPrefix << "Update = " << elementData[elementId].Update << ";" << std::endl;
		elementFile << initPropertyPrefix << "Graphics = " << elementData[elementId].Graphics << ";" << std::endl;

		elementFile << "}" << std::endl << std::endl;
		elementFile.close();
	}

	std::cout << "Finished" << std::endl;

	return 0;
}

#endif
