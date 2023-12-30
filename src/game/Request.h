#ifndef REQUEST_H
#define REQUEST_H

#include "PostData.h"

#include <map>
#include "curl/system.h"
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>

// minor hacks so I can avoid including curl in the header file. This causes problems with mingw
typedef void CURL;
struct curl_slist;
struct curl_mime_s;
struct curl_httppost;
typedef struct curl_mime_s curl_mime;

class RequestManager;
class Request
{
	std::string uri;
	bool gotStatusLine = false;
	std::vector<http::Header> response_headers;
	std::string response_body;

	CURL *easy;
	char *error_buffer;

	volatile curl_off_t rm_total;
	volatile curl_off_t rm_done;
	volatile bool rm_finished;
	volatile bool rm_canceled;
	volatile bool rm_started;
	std::mutex rm_mutex;

	bool added_to_multi;
	int status;

	std::string verb;
	struct curl_slist *headers;

	bool isPost = false;
	curl_mime *post_fields;
	curl_httppost *post_fields_first, *post_fields_last;
	std::map<std::string, std::string> post_fields_map;
	bool use_string_post_field = false;
	std::string post_field_str;

	std::condition_variable done_cv;

	static size_t HeaderDataHandler(char * ptr, size_t size, size_t count, void * userdata);
	static size_t WriteDataHandler(char * ptr, size_t size, size_t count, void * userdata);

public:
	Request(std::string uri);
	virtual ~Request();

	void Verb(std::string newVerb);
	void AddHeader(http::Header header);
	void AddPostData(http::PostData data);
	void AuthHeaders(std::string ID, std::string session);

	void Start();
	std::string Finish(int *status, std::vector<http::Header> *headers_out = nullptr);
	void Cancel();

	void CheckProgress(int *total, int *done);
	bool CheckDone();
	bool CheckCanceled();
	bool CheckStarted();

	friend class RequestManager;

	static std::string Simple(std::string uri, int *status, http::FormData postData = {});
	static std::string SimpleAuth(std::string uri, int *status, std::string ID, std::string session, http::FormData postData = {});

	static std::string GetStatusCodeDesc(int code);
};

extern const long timeout;
extern std::string proxy;
extern std::string user_agent;

#endif // REQUEST_H
