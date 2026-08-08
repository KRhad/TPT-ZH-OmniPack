#include "StartupRequest.h"
#include "client/Client.h"
#include "Config.h"

namespace
{
	bool IsNewerVersion(int major, int minor, int build)
	{
		if (major != int(APP_VERSION.displayVersion[0]))
			return major > int(APP_VERSION.displayVersion[0]);
		if (minor != int(APP_VERSION.displayVersion[1]))
			return minor > int(APP_VERSION.displayVersion[1]);
		return build > int(APP_VERSION.build);
	}

	ByteString ResolveUpdateUrl(ByteString base, ByteString file)
	{
		if (file.BeginsWith("https://"))
			return file;
		if (!file.BeginsWith("/"))
			file = "/" + file;
		return base + file;
	}
}

namespace http
{
	// TODO: update Client::messageOfTheDay
	StartupRequest::StartupRequest(bool newAlternate) :
		Request(ByteString::Build(newAlternate ? UPDATESERVER : SERVER, "/Startup.json")),
		alternate(newAlternate)
	{
		auto user = Client::Ref().GetAuthUser();
		if (user)
		{
			if (alternate)
			{
				// Cursed
				AuthHeaders(user->Username, "");
			}
			else
			{
				AuthHeaders(ByteString::Build(user->UserID), user->SessionID);
			}
		}
	}

	StartupInfo StartupRequest::Finish()
	{
		auto [ status, data ] = Request::Finish();
		ParseResponse(data, status, responseJson);
		StartupInfo startupInfo;
		try
		{
			Json::Value document;
			std::istringstream ss(data);
			ss >> document;
			startupInfo.sessionGood = document["Session"].asBool();
			startupInfo.messageOfTheDay = ByteString(document["MessageOfTheDay"].asString()).FromUtf8();
			for (auto &notification : document["Notifications"])
			{
				startupInfo.notifications.push_back({
					ByteString(notification["Text"].asString()).FromUtf8(),
					notification["Link"].asString()
				});
			}
			if constexpr (!IGNORE_UPDATES)
			{
				auto &versions = document["Updates"];
				auto parseUpdate = [this, &versions, &startupInfo](ByteString key, UpdateInfo::Channel channel) {
					if (!versions.isMember(key))
					{
						return;
					}
					auto &info = versions[key];
					if (info.isNull())
					{
						return;
					}
					auto getOr = [&info](ByteString key, int defaultValue) -> int {
						if (!info.isMember(key))
						{
							return defaultValue;
						}
						return info[key].asInt();
					};
					auto build = getOr(key == "Snapshot" ? "Snapshot" : "Build", 0);
					auto major = getOr("Major", 0);
					auto minor = getOr("Minor", 0);
					if (key == "Snapshot" ? size_t(build) <= APP_VERSION.build : !IsNewerVersion(major, minor, build))
					{
						return;
					}

					const Json::Value *asset = &info;
					if (info.isMember("Platforms"))
					{
						auto &platforms = info["Platforms"];
						if (!platforms.isObject() || !platforms.isMember(IDENT_PLATFORM))
							return;
						asset = &platforms[IDENT_PLATFORM];
					}
					if (!asset->isObject() || !asset->isMember("File"))
						throw RequestError("Update manifest has no file for this platform");

					auto packageType = UpdateInfo::packageLegacyExecutable;
					if (asset->isMember("Format"))
					{
						auto format = ByteString((*asset)["Format"].asString());
						if (format == "android-apk")
							packageType = UpdateInfo::packageAndroidApk;
						else if (format != "butt-executable")
							throw RequestError("Update manifest has an unsupported package format");
					}
					auto sha256 = asset->isMember("Sha256") ? ByteString((*asset)["Sha256"].asString()).ToUpper() : ByteString{};
					if (info.isMember("Platforms") && (sha256.size() != 64 || sha256.find_first_not_of("0123456789ABCDEF") != ByteString::npos))
						throw RequestError("Update manifest has an invalid SHA-256");
					auto size = asset->isMember("Size") ? (*asset)["Size"].asInt64() : int64_t(-1);
					if (info.isMember("Platforms") && size <= 0)
						throw RequestError("Update manifest has an invalid package size");

					auto base = ByteString(alternate ? UPDATESERVER : SERVER);
					startupInfo.updateInfo = UpdateInfo{
						channel,
						ResolveUpdateUrl(base, ByteString((*asset)["File"].asString())),
						sha256,
						ByteString(info["Changelog"].asString()).FromUtf8(),
						packageType,
						size,
						major,
						minor,
						build,
					};
				};
				if constexpr (SNAPSHOT || MOD)
				{
					parseUpdate("Snapshot", UpdateInfo::channelSnapshot);
				}
				else
				{
					parseUpdate("Stable", UpdateInfo::channelStable);
					if (!startupInfo.updateInfo.has_value())
					{
						parseUpdate("Beta", UpdateInfo::channelBeta);
					}
				}
			}
		}
		catch (const std::exception &ex)
		{
			throw RequestError("Could not read response: " + ByteString(ex.what()));
		}
		return startupInfo;
	}
}
