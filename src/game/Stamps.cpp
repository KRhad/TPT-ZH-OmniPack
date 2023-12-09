#include "Stamps.h"
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <list>
#include <sstream>

#include "defines.h"
#include "Authors.h"
#include "interface.h"
#include "misc.h"
#include "save_legacy.h"

#include "common/Platform.h"
#include "game/Save.h"
#include "interface/Engine.h"
#include "json/json.h"
#include "simulation/Simulation.h"
#include "gui/dialogs/ErrorPrompt.h"

Stamp Stamps::GetStamp(unsigned int i)
{
	if (i < stamps.size())
		return stamps[i];

	return noStamp;
}

int Stamps::GetStampId(std::string name)
{
	for (unsigned int i = 0; i < (unsigned int)stamps.size(); i++)
	{
		if (stamps[i].name == name)
			return i;
	}

	return -1;
}

void Stamps::Init()
{
	WaitForThumbs(true);

	Platform::MakeDirectory("stamps");

	bool succ = InitAsJson();
	if (!succ)
		std::cout << "Couldn't find stamps.json" << std::endl;

	succ = InitAsDef(succ);
	if (!succ)
		std::cout << "Couldn't find stamps.def" << std::endl;

	thumbThreadCanceled = false;
	thumbThreadRunning = true;
	genThumbThread = std::thread([&]() { GenThumbThread(); });
}

bool Stamps::InitAsJson()
{
	int size;
	char *stamp_data = (char*)file_load("stamps" PATH_SEP "stamps.json", &size);
	if (!stamp_data)
		return false;

	std::istringstream datastream(stamp_data);
	Json::Value root;
	datastream >> root;

	Json::Value recent = root["MostRecentlyUsedFirst"];
	for (Json::UInt i = 0; i < recent.size(); i++)
	{
		stamps.push_back(Stamp(recent[i].asString()));
		stampNames.insert(recent[i].asString());
	}

	return true;
}

bool Stamps::InitAsDef(bool appendMode)
{
	FILE *f = fopen("stamps" PATH_SEP "stamps.def", "rb");
	if (!f)
		return false;
	std::vector<Stamp> toAppend;
	char name[11];
	while (true)
	{
		int readsize = fread(name, 1, 10, f);
		if (readsize != 10 || !name[0])
			break;
		if (appendMode)
		{
			if (stampNames.find(name) == stampNames.end())
				toAppend.push_back(Stamp(name));
		}
		else
		{
			stamps.push_back(Stamp(name));
		}
	}
	fclose(f);

	if (appendMode)
	{
		stamps.insert(stamps.begin(), toAppend.begin(), toAppend.end());
	}
	return true;
}

bool Stamps::WriteStampsJson()
{
	Json::Value recent;
	for (const auto & stamp : stamps)
	{
		recent.append(stamp.name);
	}
	Json::Value root;
	root["MostRecentlyUsedFirst"] = recent;

	std::ostringstream datastream;
	datastream << root;

	FILE *f = fopen("stamps" PATH_SEP "stamps.json", "wb");
	if (!f)
		return false;
	fwrite(datastream.str().c_str(), 1, datastream.str().length(), f);
	fclose(f);

	return true;
}

bool Stamps::WriteStampsDef()
{
	FILE *f = fopen("stamps" PATH_SEP "stamps.def", "wb");
	if (!f)
		return false;

	for (const auto & stamp : stamps)
	{
		if (stamp.name.length() == 10)
			fwrite(stamp.name.c_str(), 1, 10, f);
	}
	fclose(f);

	return true;
}


void Stamps::Free()
{
	WaitForThumbs(true);
	for (auto & stamp : stamps)
		if (stamp.thumb)
		{
			free(stamp.thumb);
			stamp.thumb = nullptr;
		}
	stamps.clear();
}

void Stamps::Rescan()
{
	std::list<std::string> stampIDs;
	std::vector<std::string> stampList = Platform::DirectorySearch("stamps", "", { ".stm" });
	for (auto &stamp : stampList)
	{
		if (stamp.length() == 14)
			stampIDs.push_back(stamp.substr(0, 10));
	}
	stampIDs.sort(std::greater<std::string>());

	bool succ = WriteStampsJson();
	succ = WriteStampsDef() || succ;
	if (!succ)
		Engine::Ref().ShowWindow(new ErrorPrompt("Could not write stamps.json and stamps.def"));

	// Re-init everything
	Free();
	Init();
}

bool Stamps::Rename(unsigned int i, std::string newName)
{
	if (i >= stamps.size())
		return false;
	if (!Platform::RenameFile(GetPath(stamps[i].name), GetPath(newName), false))
		return false;
	stamps[i].name = newName;

	ReprocessStamps();

	return true;
}

Save * Stamps::Load(unsigned int i, bool reorder)
{
	if (i >= stamps.size())
		return nullptr;

	int size;
	char *data = (char*)file_load(GetPath(stamps[i].name).c_str(), &size);
	if (!data)
		return nullptr;
	Save *save = new Save(data, size);
	free(data);

	if (reorder && i > 0)
	{
		Stamp stamp = stamps.at(i);
		stamps.erase(stamps.begin() + i);
		stamps.push_front(stamp);
		ReprocessStamps();
	}

	return save;
}

Save * Stamps::Load(std::string name, bool reorder)
{
	int stampId = Stamps::GetStampId(name);
	if (stampId != -1)
		return Stamps::Load(stampId, reorder);

	return nullptr;
}

std::string Stamps::Generate(Simulation * sim, int x, int y, int w, int h, bool includePressure)
{
	Json::Value stampInfo;
	stampInfo["type"] = "stamp";
	stampInfo["username"] = svf_user;
	// Blank out stamp name and date because it's useless, and for privacy reasons
	if (authors.size())
	{
		// If top of the authors stack is already a stamp, just replace it with the current one
		if (authors["type"] == "stamp")
			stampInfo["links"] = authors["links"];
		// Otherwise, append full authorship info
		else
			stampInfo["links"].append(authors);
	}

	Save *save = sim->CreateSave(x, y, x + w, y + h, includePressure);
	save->authors = stampInfo;
	try
	{
		save->BuildSave();
	}
	catch (BuildException & e)
	{
		ErrorPrompt *error = new ErrorPrompt("Error building stamp: " + std::string(e.what()));
		Engine::Ref().ShowWindow(error);
		delete save;

		return "";
	}

	std::string ret = Stamps::Add(save);
	delete save;
	return ret;
}

std::string Stamps::Add(Save * save)
{
	std::string name = GenName();
	FILE *f = fopen(GetPath(name).c_str(), "wb");
	if (!f)
		return "";
	fwrite(save->GetSaveData(), save->GetSaveSize(), 1, f);
	fclose(f);

	// Generate thumbnail
	WaitForThumbs(false);
	auto stamp = Stamp(name);
	GenThumb(stamp);
	stamps.push_front(stamp);

	ReprocessStamps();

	return name;
}

void Stamps::Delete(unsigned int i)
{
	if (i < stamps.size())
	{
		stamps[i].dodelete = true;

		WaitForThumbs(false);
		ReprocessStamps();
	}
}

void Stamps::ReprocessStamps()
{
	for (const auto & stamp : stamps)
	{
		if (stamp.dodelete)
		{
			std::string path = GetPath(stamp.name);
			Platform::DeleteFile(path);

			free(stamp.thumb);
		}
	}

	auto it = std::remove_if(stamps.begin(), stamps.end(), [](const Stamp & s) { return s.dodelete; });
	stamps.erase(it, stamps.end());

	WriteStampsJson();
	WriteStampsDef();
}

void Stamps::GenThumbThread()
{
	for (auto & stamp : stamps)
	{
		// Break early if another thread wants us to join
		if (thumbThreadCanceled)
			break;
		GenThumb(stamp);
	}
}

void Stamps::WaitForThumbs(bool killThread)
{
	if (thumbThreadRunning)
	{
		if (killThread)
			thumbThreadCanceled = true;
		genThumbThread.join();
	}
	thumbThreadRunning = false;
}

void Stamps::GenThumb(Stamp & stamp)
{
	if (stamp.thumb)
	{
		free(stamp.thumb);
		stamp.thumb = nullptr;
	}

	int size;
	void *data = file_load(GetPath(stamp.name).c_str(), &size);

	if (data)
	{
		stamp.thumb = prerender_save(data, size, &stamp.thumb_w, &stamp.thumb_h);
		if (stamp.thumb && (stamp.thumb_w > XRES / GRID_S || stamp.thumb_h > YRES / GRID_S))
		{
			int factor_x = (int)ceil((float)stamp.thumb_w / (float)(XRES / GRID_S));
			int factor_y = (int)ceil((float)stamp.thumb_h / (float)(YRES / GRID_S));
			if (factor_y > factor_x)
				factor_x = factor_y;
			pixel *tmp = rescale_img(stamp.thumb, stamp.thumb_w, stamp.thumb_h, &stamp.thumb_w, &stamp.thumb_h, factor_x);
			free(stamp.thumb);
			stamp.thumb = tmp;
		}
	}

	free(data);
}

std::string Stamps::GenName()
{
	time_t t = time(NULL);

	if (lastTime != t)
	{
		lastTime = t;
		lastTimeIndex = 0;
	}
	else
	{
		lastTimeIndex++;
	}

	std::stringstream name;
	name << std::setfill('0') << std::setw(8) << std::hex << lastTime << std::setw(2) << lastTimeIndex;
	return name.str();
}

std::string Stamps::GetPath(std::string name)
{
	std::stringstream filePath;
	filePath << "stamps" << PATH_SEP << name << ".stm";

	return filePath.str();
}
