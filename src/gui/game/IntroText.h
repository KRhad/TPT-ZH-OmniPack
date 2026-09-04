#pragma once
#include "Config.h"
#include "SimulationConfig.h"
#include "common/String.h"

inline ByteString VersionInfo()
{
	ByteStringBuilder sb;
	sb << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1];
	if constexpr (!SNAPSHOT)
	{
		sb << "." << APP_VERSION.build;
	}
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
	ByteStringBuilder sb;
	sb << "\bl\bU" << APPNAME << "\bU - 版本 " << DISPLAY_VERSION[0] << "." << DISPLAY_VERSION[1] << " - https://powdertoy.co.uk、irc.libera.chat #powder、https://tpt.io/discord\n"
	      "\n"
	      "\n"
	      "\bgControl+C/V/X 分别为复制、粘贴、剪切。\n"
	      "\bg要选择一种材质，请将鼠标悬停在右侧的一个图标上，它将显示该组中的一系列元素。\n"
	      "\bg使用鼠标左/右键从菜单中选择您的材料。\n"
	      "通过在绘图区域上拖动鼠标左/右键来绘制自由线条。\n"
	      "Shift+拖动将创建直线粒子。\n"
	      "Ctrl+拖动将产生填充矩形。\n"
	      "Ctrl+Shift+点击将淹没一个封闭区域。\n"
	      "使用鼠标滚轮或 [、] 键调整粒子工具大小。\n"
	      "单击中键或按住 Alt 单击可取样粒子。\n"
	      "Ctrl+Z 可撤销操作。\n"
	      "\n\bo按 Z 使用缩放工具；单击可固定放大窗口，滚轮可调整缩放倍率。\n"
	      "空格键可用于暂停物理。使用“F”前进一帧。\n"
	      "使用“S”将窗口的一部分保存为“图章”。 “L”加载最新的图章，“K”显示您保存的图章库。\n"
	      "使用“P”截图并保存到当前目录。\n"
	      "按 H 显示或隐藏 HUD；按 D 切换 HUD 调试信息。\n"
	      "\n";
	if constexpr (BETA)
	{
		sb << "\br这是一个BETA，你不能公开保存东西，也不能打开旧版本中用它制作的本地保存和图章。\n"
		      "\br如果您计划发布任何保存，请使用发布版本。\n";
	}
	else
	{
		sb << "\bg要使用保存等在线功能，需要注册：\br" << SERVER << "/Register.html\n";
	}
	sb << "\n\bt" << VersionInfo();
	return sb.Build();
}
