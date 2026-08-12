#include "Dynamic.h"
#include "Clipboard.h"
#include "client/GameSave.h"
#include "prefs/GlobalPrefs.h"
#include "PowderToySDL.h"
#include "ClipboardImpls.h"
#include "common/platform/SDLCompat.h"
#if !TPT_SDL3
# include <SDL_syswm.h>
#endif
#include <iostream>
#include <cstring>

namespace Clipboard
{
#define CLIPBOARD_IMPLS_DECLARE(subsystem, factory) std::unique_ptr<ClipboardImpl> factory();
		CLIPBOARD_IMPLS(CLIPBOARD_IMPLS_DECLARE)
#undef CLIPBOARD_IMPLS_DECLARE

	struct ClipboardImplEntry
	{
		PlatformSubsystem subsystem;
		std::unique_ptr<ClipboardImpl> (*factory)();
	} clipboardImpls[] = {
#define CLIPBOARD_IMPLS_DEFINE(subsystem, factory) { subsystem, factory },
		CLIPBOARD_IMPLS(CLIPBOARD_IMPLS_DEFINE)
#undef CLIPBOARD_IMPLS_DEFINE
		{ PlatformSubsystem::Unknown, nullptr },
	};

	std::unique_ptr<GameSave> clipboardData;
	static std::unique_ptr<ClipboardImpl> clipboard;

	void InvokeClipboardSetClipboardData()
	{
		if (clipboard)
		{
			if (clipboardData)
			{
				clipboard->SetClipboardData(); // this either works or it doesn't, we don't care
			}
			else
			{
				std::cerr << "cannot put save on clipboard: no data to transfer" << std::endl;
			}
		}
	}

	void SerializeClipboard(std::vector<char> &saveData)
	{
		std::tie(std::ignore, saveData) = clipboardData->Serialise();
	}

	void SetClipboardData(std::unique_ptr<GameSave> data)
	{
		clipboardData = std::move(data);
		InvokeClipboardSetClipboardData();
	}

	void InvokeClipboardGetClipboardData()
	{
		if (clipboard)
		{
			auto result = clipboard->GetClipboardData();
			if (std::holds_alternative<ClipboardImpl::GetClipboardDataUnchanged>(result))
			{
				std::cerr << "not getting save from clipboard, data unchanged" << std::endl;
				return;
			}
			if (std::holds_alternative<ClipboardImpl::GetClipboardDataUnknown>(result))
			{
				return;
			}
			clipboardData.reset();
			auto *data = std::get_if<ClipboardImpl::GetClipboardDataChanged>(&result);
			if (!data)
			{
				return;
			}
			try
			{
				clipboardData = std::make_unique<GameSave>(data->data);
			}
			catch (const ParseException &e)
			{
				std::cerr << "got bad save from clipboard: " << e.what() << std::endl;
				return;
			}
			std::cerr << "got save from clipboard" << std::endl;
		}
	}

	const GameSave *GetClipboardData()
	{
		InvokeClipboardGetClipboardData();
		return clipboardData.get();
	}

	static bool enabled = false;
	void Init()
	{
		enabled = GlobalPrefs::Ref().Get("NativeClipboard.Enabled", false);
	}

	bool GetEnabled()
	{
		return enabled;
	}

	void SetEnabled(bool newEnabled)
	{
		enabled = newEnabled;
		RecreateWindow();
	}

	PlatformSubsystem currentSubsystem;

	void RecreateWindow()
	{
		// old window is gone (or doesn't exist), associate clipboard data with the new one
		clipboard.reset();
		currentSubsystem = PlatformSubsystem::Unknown;
#if TPT_SDL3
		if (auto *driver = SDL_GetCurrentVideoDriver())
		{
			if (!std::strcmp(driver, "windows"))
				currentSubsystem = PlatformSubsystem::Windows;
			else if (!std::strcmp(driver, "cocoa"))
				currentSubsystem = PlatformSubsystem::Cocoa;
			else if (!std::strcmp(driver, "x11"))
				currentSubsystem = PlatformSubsystem::X11;
			else if (!std::strcmp(driver, "wayland"))
				currentSubsystem = PlatformSubsystem::Wayland;
		}
#else
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		SDL_GetWindowWMInfo(sdl_window, &info);
		switch (info.subsystem)
		{
		case SDL_SYSWM_WINDOWS: currentSubsystem = PlatformSubsystem::Windows; break;
		case SDL_SYSWM_COCOA: currentSubsystem = PlatformSubsystem::Cocoa; break;
		case SDL_SYSWM_X11: currentSubsystem = PlatformSubsystem::X11; break;
		case SDL_SYSWM_WAYLAND: currentSubsystem = PlatformSubsystem::Wayland; break;
		default: break;
		}
#endif
		if (enabled)
		{
			for (auto *impl = clipboardImpls; impl->factory; ++impl)
			{
				if (impl->subsystem == currentSubsystem)
				{
					clipboard = impl->factory();
					break;
				}
			}
		}
		InvokeClipboardSetClipboardData();
	}

	std::optional<String> Explanation()
	{
		return clipboard ? clipboard->Explanation() : std::nullopt;
	}
}
