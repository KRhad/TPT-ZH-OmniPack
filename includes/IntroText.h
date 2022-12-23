#ifdef TOUCHUI
const char *const introText =
	"\blThe Powder Toy - Version " MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	"\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\n"
	"Android version " MTOS(MOBILE_MAJOR) "." MTOS(MOBILE_MINOR) " build " MTOS(MOBILE_BUILD) "\n"
	"\n"
	"\bgTouch and drag the menus on the right side to show each menusection.\n"
	"For scrollable menusections, the menus can be dragged around.\n"
	"Use the volume keys to change the tool size for particles.\n"
	"To open online saves, tap the 'Find & open a simulation' button on the very bottom left.\n"
	"To pause and unpause the game, use the pause button on the very bottom right.\n\n"
	"\boThe buttons on the right do the following actions respectively:\n"
	"\bo   Toggle erase tool (hold button down to clear screen)\n"
	"\bo   Open the console (hold button to show the on screen keyboard)\n"
	"\bo   Toggle Newtonian gravity, or decorations while in deco menu (hold to open simulation settings)\n"
	"\bo   Place or remove a zoom window\n"
	"\bo   Save a stamp (hold to load a stamp)\n\n"
	"\boWhen placing a stamp, you may do the following:\n"
	"\bo   Drag the stamp around wherever you want.\n"
	"\bo   Tap the stamp without moving it to place.\n"
	"\bo   Touch any location outside of the stamp to shift it by one pixel.\n"
	"\bo   Rotate the stamp by first tapping around the edges and then dragging in a circle.\n"
	"\n"
	"Contributors: \bgStanislaw K Skowronek (Designed the original Powder Toy),\n"
	"\bgSimon Robertshaw, Skresanov Savely, cracker64, Catelite, Bryan Hoyle, Nathan Cousins, jacksonmj,\n"
	"\bgFelix Wallin, Lieuwe Mosch, Anthony Boot, Me4502, MaksProg, jacob1, mniip, LBPHacker\n"
	"\n"
#ifndef BETA
	"\bgTo use online features such as saving, you need to register at: \brhttps://powdertoy.co.uk/Register.html\n"
#else
	"\brThis is a BETA, you cannot save things publicly. If you are planning on publishing any saves, use the release version\n"
#endif
	"\n"
	"\bt" MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) "." MTOS(BUILD_NUM) " "
#else
const char *const introText =
	"\blThe Powder Toy - Version " MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) " - https://powdertoy.co.uk, irc.libera.chat #powder, https://tpt.io/discord\n"
	"\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\n"
	"\brJ\bla\boc\bgo\btb\bb1\bp'\bws \bbMod version " MTOS(MOD_VERSION) "." MTOS(MOD_MINOR_VERSION) " build " MTOS(MOD_BUILD_VERSION) "\bg   Codebase based on C version 83.0\n"
	"\n"
	"\bgControl+C/V/X are Copy, Paste and cut respectively.\n"
	"\bgTo choose a material, hover over one of the icons on the right, it will show a selection of elements in that group.\n"
	"\bgPick your material from the menu using mouse left/right buttons.\n"
	"Draw freeform lines by dragging your mouse left/right button across the drawing area.\n"
	"Shift+drag will create straight lines of particles.\n"
	"Ctrl+drag will result in filled rectangles.\n"
	"Ctrl+Shift+click will flood-fill a closed area.\n"
	"Use the mouse scroll wheel, or '[' and ']', to change the tool size for particles.\n"
	"Middle click or Alt+Click to \"sample\" the particles.\n"
	"Ctrl+Z will act as Undo.\n"
	"\n\boUse 'Z' for a zoom tool. Click to make the drawable zoom window stay around. Use the wheel to change the zoom strength\n"
	"The spacebar can be used to pause and unpause physics.\n"
	"Use 'S' to save parts of the window as 'stamps'.\n"
	"'L' will load the most recent stamp, 'K' shows a library of stamps you saved.\n"
	"The numbers on the keyboard will change the display mode\n"
	"\n"
	"Contributors: \bgStanislaw K Skowronek (Designed the original Powder Toy),\n"
	"\bgSimon Robertshaw, Skresanov Savely, cracker64, Catelite, Bryan Hoyle, Nathan Cousins, jacksonmj,\n"
	"\bgFelix Wallin, Lieuwe Mosch, Anthony Boot, Me4502, MaksProg, jacob1, mniip, LBPHacker\n"
	"Thanks to cracker64 for the update server for my mod\n"
	"\n"
#ifndef BETA
	"\bgTo use online features such as saving, you need to register at: \brhttps://powdertoy.co.uk/Register.html\n"
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
