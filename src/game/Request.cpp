#include "Request.h"
#ifndef NOHTTP
#include "defines.h"
#include <curl/curl.h>
#include <iostream>
#include "RequestManager.h"
#include "common/Format.h"
#include "common/Platform.h"


#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 56, 0)
# define REQUEST_USE_CURL_MIMEPOST
#endif

#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 61, 0)
# define REQUEST_USE_CURL_TLSV13CL
#endif

void SetupCurlEasyCiphers(CURL *easy)
{
#ifdef SECURE_CIPHERS_ONLY
	curl_version_info_data *version_info = curl_version_info(CURLVERSION_NOW);
	std::string ssl_type = version_info->ssl_version;
	if (ssl_type.find("OpenSSL") != ssl_type.npos)
	{
		curl_easy_setopt(easy, CURLOPT_SSL_CIPHER_LIST, "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:DHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES128-SHA:DHE-RSA-AES128-GCM-SHA256");
#ifdef REQUEST_USE_CURL_TLSV13CL
		curl_easy_setopt(easy, CURLOPT_TLS13_CIPHERS, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256:TLS_AES_128_CCM_8_SHA256:TLS_AES_128_CCM_SHA256");
#endif
	}
	else if (ssl_type.find("Schannel") != ssl_type.npos)
	{
		// TODO: add more cipher algorithms
		curl_easy_setopt(easy, CURLOPT_SSL_CIPHER_LIST, "CALG_ECDH_EPHEM");
	}
#endif
	// TODO: Find out what TLS1.2 is supported on, might need to also allow TLS1.0
	curl_easy_setopt(easy, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 70, 0)
	curl_easy_setopt(easy, CURLOPT_SSL_OPTIONS, CURLSSLOPT_REVOKE_BEST_EFFORT);
#elif defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 44, 0)
	curl_easy_setopt(easy, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);
#elif defined(WIN)
# error "Curl version too low (v7.44+ required)."
#endif
}

Request::Request(std::string uri_):
	uri(uri_),
	rm_total(0),
	rm_done(0),
	rm_finished(false),
	rm_canceled(false),
	rm_started(false),
	added_to_multi(false),
	status(0),
	headers(NULL),
#ifdef REQUEST_USE_CURL_MIMEPOST
	post_fields(NULL)
#else
	post_fields_first(NULL),
	post_fields_last(NULL)
#endif
{
	error_buffer = new char[CURL_ERROR_SIZE];
	easy = curl_easy_init();
	if (!RequestManager::Ref().AddRequest(this))
	{
		status = 604;
		rm_finished = true;
	}
}
#else
Request::Request(std::string uri_) {}
#endif

Request::~Request()
{
#ifndef NOHTTP
	curl_easy_cleanup(easy);
#ifdef REQUEST_USE_CURL_MIMEPOST
	curl_mime_free(post_fields);
#else
	curl_formfree(post_fields_first);
#endif
	curl_slist_free_all(headers);
	delete[] error_buffer;
#endif
}

void Request::Verb(std::string newVerb)
{
#ifndef NOHTTP
	verb = newVerb;
#endif
}

void Request::AddHeader(http::Header header)
{
#ifndef NOHTTP
	headers = curl_slist_append(headers, (header.name + ": " + header.value).c_str());
#endif
}

// add post data to a request
void Request::AddPostData(http::PostData data)
{
	isPost = true;
#ifndef NOHTTP
	if (easy)
	{
		if (std::holds_alternative<http::FormData>(data) && std::get<http::FormData>(data).size())
		{
			auto &formData = std::get<http::FormData>(data);
#ifdef REQUEST_USE_CURL_MIMEPOST
			if (!post_fields)
			{
				post_fields = curl_mime_init(easy);
			}

			for (auto &field : formData)
			{
				curl_mimepart *part = curl_mime_addpart(post_fields);
				curl_mime_data(part, &field.value[0], field.value.size());
				curl_mime_name(part, field.name.c_str());
				if (field.filename.has_value())
				{
					curl_mime_filename(part, field.filename->c_str());
				}
			}
#else
			post_fields_map.insert(formData.begin(), formData.end());
#endif
			use_string_post_field = false;
		}
		else if (std::holds_alternative<http::StringData>(data) && std::get<http::StringData>(data).size())
		{
			auto &stringData = std::get<http::StringData>(data);
			post_field_str = stringData;
			use_string_post_field = true;
		}
	}
#endif
}

// add userID and sessionID headers to the request
void Request::AuthHeaders(std::string ID, std::string session)
{
	if (ID.size())
	{
		if (session.size())
		{
			AddHeader({ "X-Auth-User-Id", ID });
			AddHeader({ "X-Auth-Session-Key", session });
		}
		else
		{
			AddHeader({ "X-Auth-User", ID });
		}
	}
}

#ifndef NOHTTP
size_t Request::HeaderDataHandler(char *ptr, size_t size, size_t count, void *userdata)
{
	Request *req = (Request *)userdata;
	auto actual_size = size * count;
	if (actual_size >= 2 && ptr[actual_size - 2] == '\r' && ptr[actual_size - 1] == '\n')
	{
		if (actual_size > 2 && req->gotStatusLine) // Don't include header list terminator or the status line.
		{
			std::string line = std::string(ptr, ptr + actual_size - 2);
			size_t splitPos = line.find(":");
			if (splitPos != line.npos)
			{
				std::string before = line.substr(0, splitPos);
				std::string after = line.substr(splitPos + 1);

				while (after.size() && (after.front() == ' ' || after.front() == '\t'))
				{
					after = after.substr(1);
				}
				while (after.size() && (after.back() == ' ' || after.back() == '\t'))
				{
					after = after.substr(0, after.size() - 1);
				}
				req->response_headers.push_back({ Format::ToLower(before), after });
			}
			else
			{
				std::cerr << "skipping weird header: " << line << std::endl;
			}
		}
		req->gotStatusLine = true;
		return actual_size;
	}
	return 0;
}

size_t Request::WriteDataHandler(char *ptr, size_t size, size_t count, void *userdata)
{
	Request *req = (Request *)userdata;
	auto actual_size = size * count;
	req->response_body.append(ptr, actual_size);
	return actual_size;
}
#endif

// start the request thread
void Request::Start()
{
#ifndef NOHTTP
	if (CheckStarted() || CheckDone())
	{
		return;
	}

	if (easy)
	{
		if (use_string_post_field)
		{
			curl_easy_setopt(easy, CURLOPT_POSTFIELDS, &post_field_str[0]);
			curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t(post_field_str.size()));
		}
		else
		{
#ifdef REQUEST_USE_CURL_MIMEPOST
			if (post_fields)
			{
				curl_easy_setopt(easy, CURLOPT_MIMEPOST, post_fields);
			}
			else if (isPost)
			{
				curl_easy_setopt(easy, CURLOPT_POST, 1L);
				curl_easy_setopt(easy, CURLOPT_POSTFIELDS, "");
			}
			else
			{
				curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
			}
#else
			if (!post_fields_map.empty())
			{
				for (auto &field : post_fields_map)
				{
					if (field.filename.has_value())
					{
						curl_formadd(&post_fields_first, &post_fields_last,
							CURLFORM_COPYNAME, field.name.c_str(),
							CURLFORM_BUFFER, field.filename->c_str(),
							CURLFORM_BUFFERPTR, &field.value[0],
							CURLFORM_BUFFERLENGTH, field.value.size(),
						CURLFORM_END);
					}
					else
					{
						curl_formadd(&post_fields_first, &post_fields_last,
							CURLFORM_COPYNAME, field.name.c_str(),
							CURLFORM_PTRCONTENTS, &field.value[0],
							CURLFORM_CONTENTLEN, field.value.size(),
						CURLFORM_END);
					}
				}
				curl_easy_setopt(easy, CURLOPT_HTTPPOST, post_fields_first);
			}
			else
			{
				curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
			}
#endif
		}

		if (verb.size())
		{
			curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, verb.c_str());
		}
		curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 85, 0)
# ifdef ENFORCE_HTTPS
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "https");
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https");
# else
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "https,http");
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https,http");
# endif
#else
# ifdef ENFORCE_HTTPS
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
# else
		curl_easy_setopt(easy, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
		curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
# endif
#endif
		SetupCurlEasyCiphers(easy);
		curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 10L);

		curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error_buffer);
		error_buffer[0] = 0;

		curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, timeout);
		curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(easy, CURLOPT_URL, uri.c_str());

		if (proxy.size())
		{
			curl_easy_setopt(easy, CURLOPT_PROXY, proxy.c_str());
		}

		curl_easy_setopt(easy, CURLOPT_PRIVATE, (void *) this);
		curl_easy_setopt(easy, CURLOPT_USERAGENT, user_agent.c_str());
#ifdef DEBUG
		curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
#endif

		curl_easy_setopt(easy, CURLOPT_HEADERDATA, (void *)this);
		curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, Request::HeaderDataHandler);

		curl_easy_setopt(easy, CURLOPT_WRITEDATA, (void *) this);
		curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, Request::WriteDataHandler);
	}

	{
		std::lock_guard<std::mutex> g(rm_mutex);
		rm_started = true;
	}
	RequestManager::Ref().StartRequest(this);
#endif
}


// finish the request (if called before the request is done, this will block)
std::string Request::Finish(int *status_out, std::vector<http::Header> *headers_out)
{
#ifndef NOHTTP
	if (CheckCanceled())
	{
		return ""; // shouldn't happen but just in case
	}

	std::string response_out;
	{
		std::unique_lock<std::mutex> l(rm_mutex);
		done_cv.wait(l, [this]() { return rm_finished; });

		rm_started = false;
		rm_canceled = true;
		if (status_out)
		{
			*status_out = status;
		}
		if (headers_out)
		{
			*headers_out = std::move(response_headers);
		}
		response_out = std::move(response_body);
	}

	RequestManager::Ref().RemoveRequest(this);
	return response_out;
#else
	if (status_out)
		*status_out = 604;
	return "";
#endif
}

void Request::CheckProgress(int *total, int *done)
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	if (total)
	{
		*total = rm_total;
	}
	if (done)
	{
		*done = rm_done;
	}
#endif
}

// returns true if the request has finished
bool Request::CheckDone()
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	return rm_finished;
#else
	return true;
#endif
}

// returns true if the request was canceled
bool Request::CheckCanceled()
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	return rm_canceled;
#else
	return false;
#endif
}

// returns true if the request is running
bool Request::CheckStarted()
{
#ifndef NOHTTP
	std::lock_guard<std::mutex> g(rm_mutex);
	return rm_started;
#else
	return true;
#endif
}

// cancels the request, the request thread will delete the Request* when it finishes (do not use Request in any way after canceling)
void Request::Cancel()
{
#ifndef NOHTTP
	{
		std::lock_guard<std::mutex> g(rm_mutex);
		rm_canceled = true;
	}
	RequestManager::Ref().RemoveRequest(this);
#endif
}

std::string Request::Simple(std::string uri, int *status, http::FormData postData)
{
	return SimpleAuth(uri, status, "", "", postData);
}

std::string Request::SimpleAuth(std::string uri, int *status, std::string ID, std::string session, http::FormData postData)
{
	Request *request = new Request(uri);
	request->AddPostData(postData);
	request->AuthHeaders(ID, session);
	request->Start();
	return request->Finish(status);
}

std::string Request::GetStatusCodeDesc(int ret)
{
	switch (ret)
	{
	case 0:   return "状态码 0（可能是程序错误）";
	case 100: return "继续";
	case 101: return "正在切换协议";
	case 102: return "正在处理";
	case 200: return "成功";
	case 201: return "已创建";
	case 202: return "已接受";
	case 203: return "非权威信息";
	case 204: return "无内容";
	case 205: return "重置内容";
	case 206: return "部分内容";
	case 207: return "多状态";
	case 300: return "多种选择";
	case 301: return "永久移动";
	case 302: return "已找到";
	case 303: return "请查看其他地址";
	case 304: return "未修改";
	case 305: return "使用代理";
	case 306: return "切换代理";
	case 307: return "临时重定向";
	case 400: return "错误请求";
	case 401: return "未授权";
	case 402: return "需要付款";
	case 403: return "禁止访问";
	case 404: return "未找到";
	case 405: return "不允许此方法";
	case 406: return "不可接受";
	case 407: return "需要代理身份验证";
	case 408: return "请求超时";
	case 409: return "冲突";
	case 410: return "资源已移除";
	case 411: return "需要内容长度";
	case 412: return "前置条件失败";
	case 413: return "请求实体过大";
	case 414: return "请求地址过长";
	case 415: return "不支持的媒体类型";
	case 416: return "请求范围无法满足";
	case 417: return "预期条件失败";
	case 418: return "我是茶壶";
	case 422: return "无法处理的实体";
	case 423: return "已锁定";
	case 424: return "依赖项失败";
	case 425: return "无序集合";
	case 426: return "需要升级";
	case 444: return "无响应";
	case 450: return "被 Windows 家长控制阻止";
	case 499: return "客户端已关闭请求";
	case 500: return "服务器内部错误";
	case 501: return "尚未实现";
	case 502: return "网关错误";
	case 503: return "服务不可用";
	case 504: return "网关超时";
	case 505: return "不支持此 HTTP 版本";
	case 506: return "服务器配置协商错误";
	case 507: return "存储空间不足";
	case 509: return "超出带宽限制";
	case 510: return "未扩展";
	case 600: return "客户端内部错误";
	case 601: return "不支持的协议";
	case 602: return "未找到服务器";
	case 603: return "响应格式错误";
	case 604: return "网络不可用";
	case 605: return "请求超时";
	case 606: return "网址格式错误";
	case 607: return "连接被拒绝";
	case 608: return "未找到代理服务器";
	case 609: return "SSL：证书状态无效";
	case 610: return "因程序关闭而取消";
	case 611: return "重定向次数过多";
	case 612: return "SSL：连接错误";
	case 613: return "SSL：未找到加密引擎";
	case 614: return "SSL：无法设置默认加密引擎";
	case 615: return "SSL：本地证书错误";
	case 616: return "SSL：无法使用指定密码套件";
	case 617: return "SSL：无法初始化加密引擎";
	case 618: return "SSL：无法加载 CA 证书文件";
	case 619: return "SSL：无法加载 CRL 文件";
	case 620: return "SSL：颁发者检查失败";
	case 621: return "SSL：固定公钥不匹配";
	default:  return "未知状态码";
	}
}
