#ifndef REQUESTMANAGER_H
#define REQUESTMANAGER_H
#ifndef NOHTTP

#include <condition_variable>
#include <ctime>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include "common/Singleton.h"

// minor hack so I can avoid including curl in the header file. This causes problems with mingw
typedef void CURLM;

class Request;
class RequestManager : public Singleton<RequestManager>
{
	std::thread worker_thread;
	std::set<Request *> requests;
	int requests_added_to_multi = 0;

	std::set<Request *> requests_to_add;
	bool requests_to_start = false;
	bool requests_to_remove = false;
	bool initialized = false;
	bool rt_shutting_down = false;
	std::mutex rt_mutex;
	std::condition_variable rt_cv;

	CURLM *multi = nullptr;

	void Start();
	void Worker();
	void MultiAdd(Request *request);
	void MultiRemove(Request *request);
	bool AddRequest(Request *request);
	void StartRequest(Request *request);
	void RemoveRequest(Request *request);

public:
	RequestManager() = default;
	~RequestManager() = default;

	void Initialise(std::string proxy);
	void Shutdown();

	friend class Request;
};

extern const long timeout;
extern std::string proxy;
extern std::string user_agent;

#endif // NOHTTP
#endif // REQUESTMANAGER_H
