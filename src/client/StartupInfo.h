#pragma once
#include "common/String.h"
#include "ServerNotification.h"
#include <cstdint>
#include <vector>
#include <optional>

struct UpdateInfo
{
	enum PackageType
	{
		packageLegacyExecutable,
		packageAndroidApk,
	};
	enum Channel
	{
		channelStable,
		channelBeta,
		channelSnapshot,
	};
	Channel channel;
	ByteString file;
	ByteString sha256;
	String changeLog;
	PackageType packageType = packageLegacyExecutable;
	int64_t size = -1;
	int major = 0;
	int minor = 0;
	int build = 0;
};

struct StartupInfo
{
	bool sessionGood = false;
	String messageOfTheDay;
	std::vector<ServerNotification> notifications;
	std::optional<UpdateInfo> updateInfo;
};
