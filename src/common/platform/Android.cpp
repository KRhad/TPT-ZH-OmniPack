#include "Platform.h"
#include "Android.h"
#include "common/Defer.h"
#include "Config.h"
#include <ctime>
#include <SDL.h>
#include <jni.h>
#include <android/log.h>

namespace Platform
{
void OpenURI(ByteString uri)
{
	if (!CallActivityVoidFunc("openUri", uri))
	{
		fprintf(stderr, "cannot open URI: Android activity request failed\n");
	}
}

long unsigned int GetTime()
{
	struct timespec s;
	clock_gettime(CLOCK_MONOTONIC, &s);
	return s.tv_sec * 1000 + s.tv_nsec / 1000000;
}

ByteString ExecutableNameFirstApprox()
{
	return "/proc/self/exe";
}

bool CanUpdate()
{
	return false;
}

bool CanInstallUpdatePackage()
{
	return true;
}

bool InstallUpdatePackage(ByteString filename)
{
	return CallActivityVoidFunc("installApkUpdate", filename);
}

void SetupCrt()
{
}

std::optional<ByteString> CallActivityStringFunc(const char *funcName)
{
	ByteString result;
	struct CheckFailed : public std::runtime_error
	{
		using runtime_error::runtime_error;
	};
	try
	{
		auto CHECK = [](auto thing, const char *what) {
			if (!thing)
			{
				throw CheckFailed(what);
			}
			return thing;
		};
#define CHECK(a) CHECK(a, #a)
		auto *env              = CHECK((JNIEnv *)SDL_AndroidGetJNIEnv());
		auto activityInst      = CHECK((jobject)SDL_AndroidGetActivity());
		auto activityCls       = CHECK(env->GetObjectClass(activityInst));
		auto getClassLoaderMth = CHECK(env->GetMethodID(activityCls, "getClassLoader", "()Ljava/lang/ClassLoader;"));
		auto classLoaderInst   = CHECK(env->CallObjectMethod(activityInst, getClassLoaderMth));
		auto classLoaderCls    = CHECK(env->FindClass("java/lang/ClassLoader"));
		auto findClassMth      = CHECK(env->GetMethodID(classLoaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"));
		auto strClassName      = CHECK(env->NewStringUTF(ByteString::Build(APPID, ".PowderActivity").c_str()));
		Defer deleteStrClassName([env, strClassName]() { env->DeleteLocalRef(strClassName); });
		auto mPowderActivity   = CHECK((jclass)(env->CallObjectMethod(classLoaderInst, findClassMth, strClassName)));
		auto funcMth           = CHECK(env->GetMethodID(mPowderActivity, funcName, "()Ljava/lang/String;"));
		auto resultRef         = CHECK((jstring)env->CallObjectMethod(activityInst, funcMth));
		Defer deleteStr([env, resultRef]() { env->DeleteLocalRef(resultRef); });
		auto *resultBytes      = CHECK(env->GetStringUTFChars(resultRef, nullptr));
		Defer deleteUtf([env, resultRef, resultBytes]() { env->ReleaseStringUTFChars(resultRef, resultBytes); });
		result = resultBytes;
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "CallActivityStringFunc/%s failed: %s", funcName, ex.what());
		return std::nullopt;
	}
#undef CHECK
	return result;
}

bool CallActivityVoidFunc(const char *funcName)
{
	try
	{
		auto *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
		auto activityInst = (jobject)SDL_AndroidGetActivity();
		if (!env || !activityInst)
		{
			throw std::runtime_error("Android activity unavailable");
		}
		auto activityCls = env->GetObjectClass(activityInst);
		if (!activityCls)
		{
			throw std::runtime_error("Android activity class unavailable");
		}
		Defer deleteActivityClass([env, activityCls]() { env->DeleteLocalRef(activityCls); });
		auto funcMth = env->GetMethodID(activityCls, funcName, "()V");
		if (!funcMth)
		{
			if (env->ExceptionCheck())
			{
				env->ExceptionClear();
			}
			throw std::runtime_error("Android activity method unavailable");
		}
		env->CallVoidMethod(activityInst, funcMth);
		if (env->ExceptionCheck())
		{
			env->ExceptionDescribe();
			env->ExceptionClear();
			throw std::runtime_error("Android activity method threw");
		}
		return true;
	}
	catch (const std::exception &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "CallActivityVoidFunc/%s failed: %s", funcName, ex.what());
		return false;
	}
}

bool CallActivityVoidFunc(const char *funcName, ByteString argument)
{
	try
	{
		auto *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
		auto activityInst = (jobject)SDL_AndroidGetActivity();
		if (!env || !activityInst)
			throw std::runtime_error("Android activity unavailable");
		auto activityCls = env->GetObjectClass(activityInst);
		if (!activityCls)
			throw std::runtime_error("Android activity class unavailable");
		Defer deleteActivityClass([env, activityCls]() { env->DeleteLocalRef(activityCls); });
		auto funcMth = env->GetMethodID(activityCls, funcName, "(Ljava/lang/String;)V");
		if (!funcMth)
		{
			if (env->ExceptionCheck())
				env->ExceptionClear();
			throw std::runtime_error("Android activity string method unavailable");
		}
		auto stringArgument = env->NewStringUTF(argument.c_str());
		if (!stringArgument)
			throw std::runtime_error("Android string allocation failed");
		Defer deleteArgument([env, stringArgument]() { env->DeleteLocalRef(stringArgument); });
		env->CallVoidMethod(activityInst, funcMth, stringArgument);
		if (env->ExceptionCheck())
		{
			env->ExceptionDescribe();
			env->ExceptionClear();
			throw std::runtime_error("Android activity string method threw");
		}
		return true;
	}
	catch (const std::exception &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "CallActivityVoidFunc/%s failed: %s", funcName, ex.what());
		return false;
	}
}

void DoRestart()
{
	if (!CallActivityVoidFunc("restartApplication"))
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "restart request failed; reopen the app to apply the saved language");
	}
}

ByteString DefaultDdir()
{
	auto result = CallActivityStringFunc("getDefaultDdir");
	if (result)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "DefaultDdir succeeded, data dir is %s", result->c_str());
		return *result;
	}
	return "";
}
}
