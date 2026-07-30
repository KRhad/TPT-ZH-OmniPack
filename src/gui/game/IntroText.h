#pragma once
#include "Config.h"
#include "SimulationConfig.h"
#include "common/String.h"
#include "common/Localization.h"

inline ByteString VersionInfo()
{
	ByteStringBuilder sb;
	sb << APPNAME << " " << RELEASE_LABEL
	   << " (The Powder Toy " << UPSTREAM_VERSION.displayVersion[0] << "." << UPSTREAM_VERSION.displayVersion[1] << "." << UPSTREAM_VERSION.build << ")";
	sb << " " << IDENT;
	if constexpr (MOD)
	{
		sb << " MOD " << MOD_ID << " UPSTREAM " << UPSTREAM_VERSION.build;
	}
	if constexpr (SNAPSHOT)
	{
		sb << " SNAPSHOT " << APP_VERSION.build;
	}
	if constexpr (LUACONSOLE)
	{
		sb << " LUACONSOLE";
	}
	if constexpr (NOHTTP)
	{
		sb << " NOHTTP";
	}
	else if constexpr (ENFORCE_HTTPS)
	{
		sb << " HTTPS";
	}
	if constexpr (DEBUG)
	{
		sb << " DEBUG";
	}
	return sb.Build();
}

inline ByteString IntroText()
{
	auto tr = [](const char *key) { return Localization::Ref().Tr(key).ToUtf8(); };
	ByteStringBuilder sb;
	sb << tr("intro.title_prefix") << APPNAME << " " << RELEASE_LABEL << tr("intro.title_after_name") << UPSTREAM_VERSION.displayVersion[0] << "." << UPSTREAM_VERSION.displayVersion[1] << tr("intro.title_after_version")
	      << tr("intro.toggle_help")
	      << tr("intro.material.hover")
	      << tr("intro.material.pick")
	      << tr("intro.draw.freeform")
	      << "\n"
	      << tr("intro.tool.size")
	      << tr("intro.tool.sample")
	      << tr("intro.control.copy_paste_cut")
	      << tr("intro.paste.transform")
	      << tr("intro.draw.straight")
	      << tr("intro.draw.rectangles")
	      << "\n"
	      << tr("intro.sim.pause")
	      << tr("intro.tool.undo")
	      << tr("intro.save.stamps")
	      << "\n"
	      << tr("intro.view.modes")
	      << tr("intro.hud.debug")
	      << tr("intro.zoom.tool")
	      << tr("intro.search.highlight")
	      << "\n";
	if constexpr (BETA)
	{
		sb << tr("intro.beta.warning")
		   << tr("intro.beta.publish");
	}
	else
	{
		sb << tr("intro.online.register") << "\br" << SERVER << "/Register.html\n";
	}
	sb << tr("intro.version_prefix") << VersionInfo();
	return sb.Build();
}
