#include "UpdateActivity.h"
#include "client/http/Request.h"
#include "prefs/GlobalPrefs.h"
#include "common/Localization.h"
#include "common/Sha256.h"
#include "common/platform/Platform.h"
#include "tasks/Task.h"
#include "tasks/TaskWindow.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/interface/Engine.h"
#include "Config.h"
#include <bzlib.h>
#include <memory>
#include <utility>

class UpdateDownloadTask : public Task
{
public:
	UpdateDownloadTask(UpdateInfo updateInfo, UpdateActivity * a) : a(a), updateInfo(std::move(updateInfo)) {}
private:
	UpdateActivity * a;
	UpdateInfo updateInfo;
	void notifyDoneMain() override {
		a->NotifyDone(this);
	}
	void notifyErrorMain() override
	{
		a->NotifyError(this);
	}
	bool doWork() override
	{
		auto &prefs = GlobalPrefs::Ref();

		auto niceNotifyError = [this](String error) {
			notifyError("Downloaded update is corrupted\n" + error);
			return false;
		};

		auto request = std::make_unique<http::Request>(updateInfo.file);
		request->Start();
		notifyStatus(Localization::Ref().Tr("update.downloading"));
		notifyProgress(-1);
		while(!request->CheckDone())
		{
			int64_t total, done;
			std::tie(total, done) = request->CheckProgress();
			if (total == -1)
			{
				notifyProgress(-1);
			}
			else
			{
				notifyProgress(total ? done * 100 / total : 0);
			}
			Platform::Millisleep(1);
		}

		int status;
		ByteString data;
		try
		{
			std::tie(status, data) = request->Finish();
		}
		catch (const http::RequestError &ex)
		{
			return niceNotifyError("Could not download update: " + String::Build("Server responded with Status ", ByteString(ex.what()).FromAscii()));
		}
		if (status!=200)
		{
			return niceNotifyError("Could not download update: " + String::Build("Server responded with Status ", status));
		}
		if (!data.size())
		{
			return niceNotifyError("Server did not return any data");
		}
		if (updateInfo.size > 0 && int64_t(data.size()) != updateInfo.size)
		{
			return niceNotifyError(String::Build("Package size mismatch: expected ", updateInfo.size, ", got ", data.size()));
		}
		if (updateInfo.sha256.size())
		{
			notifyStatus(Localization::Ref().Tr("update.verifying"));
			notifyProgress(-1);
			auto actualHash = Sha256Hex(std::span<const char>(data.data(), data.size()));
			if (actualHash != updateInfo.sha256)
			{
				return niceNotifyError(String::Build("SHA-256 mismatch: expected ", updateInfo.sha256.FromAscii(), ", got ", actualHash.FromAscii()));
			}
		}

		if (updateInfo.packageType == UpdateInfo::packageAndroidApk)
		{
			if (!Platform::CanInstallUpdatePackage())
				return niceNotifyError("This platform cannot install an Android package");
			if (data.size() < 4 || data[0] != 'P' || data[1] != 'K' || data[2] != 3 || data[3] != 4)
				return niceNotifyError("Invalid APK/ZIP header");
			auto packagePath = ByteString::Build(Platform::DefaultDdir(), PATH_SEP_CHAR, "TPT-ZH-OmniPack-update.apk");
			if (!Platform::WriteFile(std::span<const char>(data.data(), data.size()), packagePath))
				return niceNotifyError("Could not write the verified APK to app storage");
			notifyStatus(Localization::Ref().Tr("update.android_install"));
			notifyProgress(-1);
			if (!Platform::InstallUpdatePackage(packagePath))
			{
				Platform::RemoveFile(packagePath);
				return niceNotifyError("Could not start the Android system installer");
			}
			return true;
		}

		notifyStatus(Localization::Ref().Tr("update.unpacking"));
		notifyProgress(-1);

		unsigned int uncompressedLength;

		if(data.size()<16)
		{
			return niceNotifyError(String::Build("Unsufficient data, got ", data.size(), " bytes"));
		}
		if (data[0]!=0x42 || data[1]!=0x75 || data[2]!=0x54 || data[3]!=0x54)
		{
			return niceNotifyError("Invalid update format");
		}

		uncompressedLength  = (unsigned char)data[4];
		uncompressedLength |= ((unsigned char)data[5])<<8;
		uncompressedLength |= ((unsigned char)data[6])<<16;
		uncompressedLength |= ((unsigned char)data[7])<<24;

		std::vector<char> res(uncompressedLength);

		int dstate;
		dstate = BZ2_bzBuffToBuffDecompress(res.data(), (unsigned *)&uncompressedLength, &data[8], data.size()-8, 0, 0);
		if (dstate)
		{
			return niceNotifyError(String::Build("Unable to decompress update: ", dstate));
		}

		notifyStatus(Localization::Ref().Tr("update.applying"));
		notifyProgress(-1);

		prefs.Set("version.update", true);
		if (!Platform::UpdateStart(res))
		{
			prefs.Set("version.update", false);
			Platform::UpdateCleanup();
			notifyError("Update failed - try downloading a new version.");
			return false;
		}

		return true;
	}
};

UpdateActivity::UpdateActivity(UpdateInfo info) :
	exitGameAfterUpdate(info.packageType == UpdateInfo::packageLegacyExecutable && Platform::CanUpdate())
{
	updateDownloadTask = new UpdateDownloadTask(std::move(info), this);
	updateWindow = new TaskWindow(Localization::Ref().Tr("update.window_title"), updateDownloadTask, true);
}

void UpdateActivity::NotifyDone(Task * sender)
{
	if(sender->GetSuccess())
	{
		Exit();
	}
}

void UpdateActivity::Exit()
{
	updateWindow->Exit();
	if (exitGameAfterUpdate)
		ui::Engine::Ref().Exit();
	delete this;
}

void UpdateActivity::NotifyError(Task * sender)
{
	StringBuilder sb;
	if constexpr (USE_UPDATESERVER)
	{
		sb << Localization::Ref().Tr("update.autoupdate_failed_manual");
	}
	else
	{
		sb << Localization::Ref().Tr("update.autoupdate_failed_visit");
	}
	sb << Localization::Ref().Tr("update.autoupdate_failed_error_prefix") << sender->GetError();
	new ConfirmPrompt(Localization::Ref().Tr("update.autoupdate_failed_title"), sb.Build(), { [this] {
		Platform::OpenURI(PROJECT_URL);
		Exit();
	}, [this] { Exit(); } });
}


UpdateActivity::~UpdateActivity() {
}
