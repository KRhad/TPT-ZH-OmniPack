 #pragma once

#include <string>
#include <thread>
#include <deque>
#include <ctime>
#include "common/Singleton.h"
#include "graphics/Pixel.h"

class Save;
class Simulation;

struct Stamp
{
	std::string name;

	pixel *thumb = nullptr;
	int thumb_w = 0, thumb_h = 0;

	bool dodelete = false;

	Stamp(std::string name):
		name(name)
	{

	}
};
typedef struct Stamp Stamp;

class Stamps : public Singleton<Stamps>
{
	std::deque<Stamp> stamps;
	Stamp noStamp = Stamp("");

	std::thread genThumbThread;
	bool thumbThreadRunning = false;
	bool thumbThreadCanceled = false;

	time_t lastTime;
	int lastTimeIndex;

	void ReprocessStamps();

	void GenThumb(Stamp &stamp);
	void GenThumbThread();

	std::string GenName();
	std::string GetPath(std::string name);

public:
	unsigned int GetNumStamps() { return stamps.size(); }
	Stamp GetStamp(unsigned int i);
	int GetStampId(std::string name);

	void Init();
	void Free();
	void Rescan();

	Save * Load(unsigned int i, bool reorder);
	Save * Load(std::string name, bool reorder);

	std::string Generate(Simulation * sim, int x, int y, int w, int h, bool includePressure);
	std::string Add(Save * save);
	void Delete(unsigned int i);

	void WaitForThumbs(bool killThread);
};
