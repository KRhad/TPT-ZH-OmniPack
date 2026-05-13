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

#ifndef SAVE_H
#define SAVE_H

#ifdef DEBUG
#include <iostream>
#endif
#include <set>
#include <string>
#include <vector>
#include "BSON.h"
#include "common/Matrix.h"
#include "game/SaveInfo.h"
#include "game/Sign.h"
#include "graphics/ARGBColour.h"
#include "json/json.h"
#include "simulation/Element.h"
#include "simulation/ElementNumbers.h"
#include "simulation/Particle.h"

struct SaveException: public std::exception
{
	std::string message;
protected:
	SaveException(std::string message):
		message(message)
	{
	}

public:
	const char * what() const throw() override
	{
		return message.c_str();
	}
	~SaveException() throw() {}
};

struct ParseException: public SaveException
{
public:
	ParseException(std::string message):
		SaveException(message)
	{
#ifdef DEBUG
		std::cout << "Error parsing save: " << message << std::endl;
#endif
	}
};

struct BuildException: public SaveException
{
public:
	BuildException(std::string message):
		SaveException(message)
	{
#ifdef DEBUG
		std::cout << "Error building save: " << message << std::endl;
#endif
	}
};

class StkmData
{
public:
	bool rocketBoots1 = false;
	bool rocketBoots2 = false;
	bool fan1 = false;
	bool fan2 = false;
	std::vector<unsigned int> rocketBootsFigh = std::vector<unsigned int>();
	std::vector<unsigned int> fanFigh = std::vector<unsigned int>();

	StkmData() = default;

	StkmData(const StkmData & stkmData):
		rocketBoots1(stkmData.rocketBoots1),
		rocketBoots2(stkmData.rocketBoots2),
		fan1(stkmData.fan1),
		fan2(stkmData.fan2),
		rocketBootsFigh(stkmData.rocketBootsFigh),
		fanFigh(stkmData.fanFigh)
	{

	}

	bool hasData()
	{
		return rocketBoots1 || rocketBoots2 || fan1 || fan2
		        || rocketBootsFigh.size() || fanFigh.size();
	}
};

class Simulation;
class Save
{
public:
	// bool which tells whether all of the maps and data in this save have been set
	// is set either when parsing a save from data, or when creating a new save in the Simulation class
	bool expanded;

	bool fromNewerVersion = false;

	unsigned int blockWidth;
	unsigned int blockHeight;
	unsigned int particlesCount;

	// Uncompressed save data
	particle * particles;
	unsigned char ** blockMap;
	float ** fanVelX;
	float ** fanVelY;
	float ** pressure;
	bool hasPressure;
	float ** velocityX;
	float ** velocityY;
	float ** ambientHeat;
	bool hasAmbientHeat;

	// other misc things from save data (mostly jacob1's mod)
	std::string luaCode;
	// log messages to show after loading save
	std::vector<std::string> logMessages;
	std::vector<std::string> adminLogMessages;
	int createdVersion;
	int modCreatedVersion;
	int androidCreatedVersion;
	std::string leftSelectedIdentifier;
	std::string rightSelectedIdentifier;

	// loaded save info
	SaveInfo saveInfo;
	bool saveInfoPresent;

	// Simulation Options
	bool legacyEnable;
	bool gravityEnable;
	bool aheatEnable;
	bool waterEEnabled;
	bool paused;
	int gravityMode;
	float customGravityX;
	float customGravityY;
	int airMode;
	float ambientAirTemp;
	bool ambientAirTempPresent = false;
	float edgePressure;
	bool edgePressurePresent = false;
	float edgeVelocityX;
	float edgeVelocityY;
	bool edgeVelocityPresent = false;
	float vorticityCoeff;
	int convectionMode;
	bool convectionModePresent = false;
	int edgeMode;
	// jacob1's mod simulation options
	// Since not all saves have these, use a bool to decide whether to overwrite options when loading saves
	bool hudEnable;
	bool hudEnablePresent;
	bool msRotation;
	bool msRotationPresent;
	int activeMenu;
	bool activeMenuPresent;
	bool decorationsEnable;
	bool decorationsEnablePresent;
	bool legacyHeatSave;

	std::vector<Sign> signs;
	StkmData stkm;

	typedef std::pair<std::string, int> PaletteItem;
	std::vector<PaletteItem> palette;
	Simulation * sim = nullptr;

	Json::Value authors;

	// Even more jacob1's mod specific things
	unsigned int renderMode;
	bool renderModePresent;
	unsigned int displayMode;
	bool displayModePresent;
	unsigned int colorMode;
	bool colorModePresent;
	typedef std::pair<unsigned char, unsigned char> MOVSdataItem;
	std::vector<MOVSdataItem> MOVSdata;
	typedef std::pair<int, std::vector<ARGBColour> > ANIMdataItem;
	std::vector<ANIMdataItem> ANIMdata;

	int pmapbits;

	Save(const char *saveData, unsigned int saveSize);
	Save(int blockW, int blockH);
	Save(const Save & save);
	~Save();
	void SetSim(Simulation *sim) { this->sim = sim; }

	void ParseSave();
	void BuildSave();

	// converts mod elements from older saves into the new correct id's, since as new elements are added to tpt the id's go up
	// Newer saves use palette instead, this is only for old saves
	int FixType(int type) const;

	Matrix::vector2d Translate(Matrix::vector2d translate);
	void Transform(Matrix::matrix2d transform, Matrix::vector2d translate);
	void Transform(Matrix::matrix2d transform, Matrix::vector2d translate, Matrix::vector2d translateReal, int newWidth, int newHeight);

	Save& operator <<(particle v);
	Save& operator <<(Sign v);

	/** If this save is not expanded, it will parse it
	 ** This could take a while and may throw a BuildException
	 **/
	const unsigned char * const GetSaveData();
	/** If this save is not expanded, it will parse it
	 ** This could take a while and may throw a BuildException
	 **/
	unsigned int GetSaveSize();

	static bool PressureInTmp3(int type);

private:
	unsigned char *saveData;
	unsigned int saveSize;
	Matrix::vector2d translated;

	Save();
	void Dealloc();

	void InitData();
	void SetSize(int newWidth, int newHeight);
	void InitVars();
	
	template <typename T> static T ** Allocate2DArray(int blockWidth, int blockHeight, T defaultVal);
	template <typename T> static void Deallocate2DArray(T ***array, int blockHeight);

	//converts old format and new tpt++ format wall id's into the correct id's for this version
	int ChangeWall(int wt);
	int ChangeWallpp(int wt);

	// functions used by ParseSaveOPS to validate fields and avoid repeated code
	void CheckBsonFieldUser(bson_iterator iter, const char *field, unsigned char **data, unsigned int *fieldLen);
	bool CheckBsonFieldBool(bson_iterator iter, const char *field, bool *flag);
	bool CheckBsonFieldInt(bson_iterator iter, const char *field, int *setting);
	bool CheckBsonFieldFloat(bson_iterator iter, const char *field, float *setting);
	void ParseSaveOPS();
	void ParseSavePSv();

	// used to convert author data between bson and json
	// only supports the minimum amount of conversion we need
	void ConvertJsonToBson(bson *b, Json::Value j, int depth = 0);
	std::set<int> GetNestedSaveIDs(Json::Value j);
	void ConvertBsonToJson(bson_iterator *b, Json::Value *j, int depth = 0);
};

#endif
