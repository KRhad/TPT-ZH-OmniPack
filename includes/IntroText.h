#ifdef TOUCHUI
const char *const introText =
	"\bl\bUThe Powder Toy\bU - 版本 " MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	"\n"
	"Android 版本 " MTOS(MOBILE_MAJOR) "." MTOS(MOBILE_MINOR) "，构建 " MTOS(MOBILE_BUILD) "\n"
	"\n"
	"\bg触摸并拖动右侧菜单可显示各个分类。\n"
	"可滚动的分类菜单可直接拖动浏览。\n"
	"使用音量键调整粒子工具大小。\n"
	"要打开在线存档，请点击左下角的'查找并打开模拟'按钮。\n"
	"使用右下角暂停按钮可暂停或继续游戏。\n\n"
	"\bo右侧按钮依次执行以下操作：\n"
	"\bo   切换擦除工具（长按清屏）\n"
	"\bo   打开控制台（长按显示屏幕键盘）\n"
	"\bo   切换牛顿引力；在装饰菜单中切换装饰层（长按打开模拟设置）\n"
	"\bo   放置或移除缩放窗口\n"
	"\bo   保存图章（长按加载图章）\n\n"
	"\bo放置图章时可以：\n"
	"\bo   将图章拖到任意位置。\n"
	"\bo   轻触图章且不移动即可放置。\n"
	"\bo   轻触图章外任意位置可平移一个像素。\n"
	"\bo   先轻触边缘，再沿圆周拖动即可旋转图章。\n"
	"\n"
	"贡献者：\bgStanislaw K Skowronek（原版 Powder Toy 设计者），\n"
	"\bgSimon Robertshaw, Skresanov Savely, Pilihp64, Catelite, Victoria Hoyle, Nathan Cousins, jacksonmj,\n"
	"\bgFelix Wallin, Lieuwe Mosch, Anthony Boot, Me4502, MaksProg, jacob1, mniip, LBPHacker\n"
	"\n"
#ifndef BETA
	"\bg要使用在线保存等功能，请先注册：\brhttps://powdertoy.co.uk/Register.html\n"
#else
	"\brThis is a BETA, you cannot save things publicly. If you are planning on publishing any saves, use the release version\n"
#endif
	"\n"
	"\bt" MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) "." MTOS(BUILD_NUM) " "
#else
const char *const introText =
	"\bl\bUThe Powder Toy\bU - Version " MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	"\n"
	"\brJ\bla\boc\bgo\btb\bb1\bp'\bws \bbMod version " MTOS(MOD_VERSION) "." MTOS(MOD_MINOR_VERSION) " build " MTOS(MOD_BUILD_VERSION) "\bg   Codebase based on C version 83.0\n"
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
	"\n\boUse 'Z' for a zoom tool. Click to make the drawable zoom window stay around. Use the wheel to change the zoom strength\n"
	"The spacebar can be used to pause and unpause physics.\n"
	"Use 'S' to save parts of the window as 'stamps'.\n"
	"'L' will load the most recent stamp, 'K' shows a library of stamps you saved.\n"
	"The numbers on the keyboard will change the display mode\n"
	"\n"
	"Contributors: \bgStanislaw K Skowronek (Designed the original Powder Toy),\n"
	"\bgSimon Robertshaw, Skresanov Savely, Pilihp64, Catelite, Victoria Hoyle, Nathan Cousins, jacksonmj,\n"
	"\bgFelix Wallin, Lieuwe Mosch, Anthony Boot, Me4502, MaksProg, jacob1, mniip, LBPHacker\n"
	"Thanks to Pilihp64 for the update server for my mod\n"
	"\n"
#ifndef BETA
	"\bg要使用在线保存等功能，请先注册：\brhttps://powdertoy.co.uk/Register.html\n"
#else
	"\brThis is a BETA, you cannot save things publicly. If you are planning on publishing any saves, use the release version\n"
#endif
	"\n"
	"\bt" MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) "." MTOS(BUILD_NUM)
#endif

#ifdef BETA
	" BETA"
#endif
#ifdef X86
	" X86"
#endif
#ifdef X86_SSE
	" X86_SSE"
#endif
#ifdef X86_SSE2
	" X86_SSE2"
#endif
#ifdef X86_SSE3
	" X86_SSE3"
#endif
#ifdef LIN
#ifdef _64BIT
	" LIN64"
#else
	" LIN32"
#endif
#endif
#ifdef WIN
#ifdef _64BIT
	" WIN64"
#else
	" WIN32"
#endif
#endif
#ifdef MACOSX
	" MACOSX"
#endif
#ifdef ANDROID
	" ANDROID"
#endif
#ifdef LUACONSOLE
	" LUACONSOLE"
#endif
#ifdef GRAVFFT
	" GRAVFFT"
#endif
    ;
