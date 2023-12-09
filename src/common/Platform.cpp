#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stack>
#include <sys/stat.h>

#ifdef WIN
#include "game/RequestManager.h"
#include <direct.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>
#ifndef __GNUC__
#include <io.h>
#endif
#ifdef _MSC_VER
#undef DeleteFile
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#endif
#else
#include <dirent.h>
#include <unistd.h>
#include <ctime>
#endif

#ifdef MACOSX
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#include <mach-o/dyld.h>
#include <sys/time.h> // gettimeofday
#endif

#ifdef LIN
#include "images.h"
#endif

#ifdef ANDROID
#include <SDL/SDL_screenkeyboard.h>
#include <SDL/SDL_android.h>
//#include <android/log.h>
//__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "The value of 1 + 1 is %d", 1+1);
#endif

#include "Platform.h"
#include "defines.h"
#include "game/Stamps.h"

namespace Platform
{

std::string originalCwd;
std::string sharedCwd;

std::string GetCwd()
{
#ifdef WIN
	std::string cwd;
	auto cwdPtr = std::unique_ptr<wchar_t, decltype(&free)>(_wgetcwd(NULL, 0), free);
	if (cwdPtr)
	{
		cwd = WinNarrow(cwdPtr.get());
	}
	return cwd;
#else
	char cwdTemp[PATH_MAX];
	getcwd(cwdTemp, PATH_MAX);
	return cwdTemp;
#endif
}

bool ChangeDir(std::string toDir)
{
#ifdef WIN
	return _wchdir(WinWiden(toDir).c_str()) == 0;
#else
	return chdir(toDir.c_str()) == 0;
#endif
}

char *ExecutableName()
{
#ifdef ANDROID
	return SDL_ANDROID_GetAppName();
#else
#ifdef WIN
	char *name= (char *)malloc(64);
	DWORD max=64, res;
	while ((res = GetModuleFileName(NULL, name, max)) >= max)
	{
#elif MACOSX
	char *fn=(char*)malloc(64),*name=(char*)malloc(PATH_MAX);
	uint32_t max=64, res;
	if (_NSGetExecutablePath(fn, &max) != 0)
	{
		char *realloced_fn = (char*)realloc(fn, max);
		assert(realloced_fn != NULL);
		fn = realloced_fn;
		_NSGetExecutablePath(fn, &max);
	}
	if (realpath(fn, name) == NULL)
	{
		free(fn);
		free(name);
		return NULL;
	}
	res = 1;
#else
	char fn[64], *name=(char*)malloc(64);
	size_t max=64, res;
	sprintf(fn, "/proc/self/exe");
	memset(name, 0, max);
	while ((res = readlink(fn, name, max)) >= max-1)
	{
#endif
#ifndef MACOSX
		max *= 2;
		char* realloced_name = (char *)realloc(name, max);
		assert(realloced_name != NULL);
		name = realloced_name;
		memset(name, 0, max);
	}
#endif //not MACOSX
	if (res <= 0)
	{
		free(name);
		return NULL;
	}
	return name;
#endif //ANDROID
}

void DoRestart(bool saveTab, bool disableSignals)
{
	if (saveTab)
	{
		sys_pause = true;
		tab_save(tab_num);
	}
#ifdef ANDROID
	SDL_ANDROID_RestartMyself("");
	exit(-1);
#else
	char *exename = ExecutableName();
	if (exename)
	{
#ifdef WIN
		int ret = (int)ShellExecute(NULL, NULL, exename, disableSignals ? "disable-bluescreen" : nullptr, nullptr, SW_SHOWNORMAL);
		if (ret <= 32)
		{
			fprintf(stderr, "cannot restart: ShellExecute(...) failed: code %i\n", ret);
		}
		else
		{
#ifndef NOHTTP
			RequestManager::Ref().Shutdown();
#endif
		}
#elif defined(LIN) || defined(MACOSX)
		execl(exename, "powder", disableSignals ? "disable-bluescreen" : nullptr);
		int ret = errno;
		fprintf(stderr, "cannot restart: execl(...) failed: code %i\n", ret);
#endif
		free(exename);
	}
	else
	{
		fprintf(stderr, "cannot restart: no executable name???\n");
	}
	exit(-1);
#endif
}

void OpenLink(std::string uri)
{
	int ret = 0;
#ifdef ANDROID
	SDL_ANDROID_OpenExternalWebBrowser(uri.c_str());
#elif WIN
	if (int(INT_PTR(ShellExecuteW(NULL, NULL, WinWiden(uri).c_str(), NULL, NULL, SW_SHOWNORMAL))) <= 32)
		ret = 1;
#elif MACOSX
	std::string command = "open \"" + uri + "\"";
	ret = system(command.c_str());
#elif LIN
	std::string command = "xdg-open \"" + uri + "\"";
	ret = system(command.c_str());
#else
	ret = 1;
#endif
	if (ret)
		printf("Cannot open browser\n");
}

void Millisleep(long int t)
{
#ifdef WIN
	Sleep(t);
#else
	struct timespec s;
	s.tv_sec = t/1000;
	s.tv_nsec = (t%1000)*10000000;
	nanosleep(&s, NULL);
#endif
}

unsigned long GetTime()
{
#ifdef WIN
	return GetTickCount();
#elif defined(MACOSX)
	struct timeval s;
	gettimeofday(&s, NULL);
	return (unsigned int)(s.tv_sec * 1000 + s.tv_usec / 1000);
#else
	struct timespec s;
	clock_gettime(CLOCK_MONOTONIC, &s);
	return s.tv_sec * 1000 + s.tv_nsec / 1000000;
#endif
}

void LoadFileInResource(int name, int type, unsigned int& size, const char*& data)
{
#ifdef _MSC_VER
	HMODULE handle = ::GetModuleHandle(NULL);
	HRSRC rc = ::FindResource(handle, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
	HGLOBAL rcData = ::LoadResource(handle, rc);
	size = ::SizeofResource(handle, rc);
	data = static_cast<const char*>(::LockResource(rcData));
#endif
}

bool RegisterExtension()
{
#ifdef WIN
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	wchar_t programsPath[MAX_PATH];
	IShellLinkW* shellLink = NULL;
	IPersistFile* shellLinkPersist = NULL;

	bool returnval;
	LONG rresult;
	HKEY newkey;
	char *currentfilename = Platform::ExecutableName();
	char *iconname = NULL;
	char *opencommand = NULL;
	char *protocolcommand = NULL;
	//char AppDataPath[MAX_PATH];
	char *AppDataPath = NULL;
	iconname = (char*)malloc(strlen(currentfilename)+6);
	sprintf(iconname, "%s,-102", currentfilename);

	//Create Roaming application data folder
	/*if(!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, AppDataPath)))
	{
		returnval = false;
		goto finalise;
	}*/

	AppDataPath = (char*)_getcwd(NULL, 0);

	//Move Game executable into application data folder
	//TODO: Implement

	opencommand = (char*)malloc(strlen(currentfilename)+53+strlen(AppDataPath));
	protocolcommand = (char*)malloc(strlen(currentfilename)+55+strlen(AppDataPath));
	/*if((strlen(AppDataPath)+strlen(APPDATA_SUBDIR "\\Powder Toy"))<MAX_PATH)
	{
		strappend(AppDataPath, APPDATA_SUBDIR);
		_mkdir(AppDataPath);
		strappend(AppDataPath, "\\Powder Toy");
		_mkdir(AppDataPath);
	} else {
		returnval = false;
		goto finalise;
	}*/
	sprintf(opencommand, "\"%s\" open \"%%1\" ddir \"%s\"", currentfilename, AppDataPath);
	sprintf(protocolcommand, "\"%s\" ddir \"%s\" ptsave \"%%1\"", currentfilename, AppDataPath);

	//Create protocol entry
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\ptsave", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"Powder Toy Save", strlen("Powder Toy Save")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, (LPCSTR)"URL Protocol", 0, REG_SZ, (LPBYTE)"", strlen("")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);


	//Set Protocol DefaultIcon
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\ptsave\\DefaultIcon", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)iconname, strlen(iconname)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Set Protocol Launch command
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\ptsave\\shell\\open\\command", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)protocolcommand, strlen(protocolcommand)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Create extension entry
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\.cps", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"PowderToySave", strlen("PowderToySave")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\.stm", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"PowderToySave", strlen("PowderToySave")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Create program entry
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\PowderToySave", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)"Powder Toy Save", strlen("Powder Toy Save")+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Set DefaultIcon
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\PowderToySave\\DefaultIcon", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)iconname, strlen(iconname)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	//Set Launch command
	rresult = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Classes\\PowderToySave\\shell\\open\\command", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, NULL);
	if (rresult != ERROR_SUCCESS) {
		returnval = false;
		goto finalise;
	}
	rresult = RegSetValueEx(newkey, 0, 0, REG_SZ, (LPBYTE)opencommand, strlen(opencommand)+1);
	if (rresult != ERROR_SUCCESS) {
		RegCloseKey(newkey);
		returnval = false;
		goto finalise;
	}
	RegCloseKey(newkey);

	if (SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, programsPath) != S_OK)
		goto finalise;
	if (CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&shellLink) != S_OK)
		goto finalise;
	shellLink->SetPath(Platform::WinWiden(currentfilename).c_str());
	shellLink->SetWorkingDirectory(Platform::WinWiden(AppDataPath).c_str());
	shellLink->SetDescription(L"Jacob1's Mod");
	if (shellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&shellLinkPersist) != S_OK)
		goto finalise;
	shellLinkPersist->Save(Platform::WinWiden(Platform::WinNarrow(programsPath) + "\\Jacob1's Mod.lnk").c_str(), TRUE);

	returnval = true;
	finalise:

	if(iconname) free(iconname);
	if(opencommand) free(opencommand);
	if(currentfilename) free(currentfilename);
	if(protocolcommand) free(protocolcommand);

	if (shellLinkPersist)
		shellLinkPersist->Release();
	if (shellLink)
		shellLink->Release();
	CoUninitialize();

	return returnval;
#elif ANDROID
	return false;
#elif LIN
	int success = 1;
	std::string filename = Platform::ExecutableName(), pathname = filename.substr(0, filename.rfind('/'));
	for (size_t i = 0; i < filename.size(); i++)
	{
		if (filename[i] == '\'')
		{
			filename.insert(i, "'\\'");
			i += 3;
		}
	}
	filename.insert(filename.size(), "'");
	filename.insert(0, "'");
	FILE *f;
	const char *mimedata =
"<?xml version=\"1.0\"?>\n"
"	<mime-info xmlns='http://www.freedesktop.org/standards/shared-mime-info'>\n"
"	<mime-type type=\"application/vnd.powdertoy.save\">\n"
"		<comment>Powder Toy save</comment>\n"
"		<glob pattern=\"*.cps\"/>\n"
"		<glob pattern=\"*.stm\"/>\n"
"	</mime-type>\n"
"</mime-info>\n";
	f = fopen("powdertoy-save.xml", "wb");
	if (!f)
		return false;
	fwrite(mimedata, 1, strlen(mimedata), f);
	fclose(f);

	const char *protocolfiledata_tmp =
"[Desktop Entry]\n"
"Type=Application\n"
"Name=Jacob1's Mod\n"
"Comment=Physics sandbox game\n"
"MimeType=x-scheme-handler/ptsave;\n"
"NoDisplay=true\n"
"Categories=Game;Simulation\n"
"Icon=powdertoy.png\n";
	std::stringstream protocolfiledata;
	protocolfiledata << protocolfiledata_tmp << "Exec=" << filename << " ptsave %u\nPath=" << pathname << "\n";
	f = fopen("powdertoy-tpt-ptsave.desktop", "wb");
	if (!f)
		return false;
	fwrite(protocolfiledata.str().c_str(), 1, strlen(protocolfiledata.str().c_str()), f);
	fclose(f);
	success = system("xdg-desktop-menu install powdertoy-tpt-ptsave.desktop") && success;

	const char *desktopopenfiledata_tmp =
"[Desktop Entry]\n"
"Type=Application\n"
"Name=Jacob1's Mod\n"
"Comment=Physics sandbox game\n"
"MimeType=application/vnd.powdertoy.save;\n"
"NoDisplay=true\n"
"Categories=Game;Simulation\n"
"Icon=powdertoy.png\n";
	std::stringstream desktopopenfiledata;
	desktopopenfiledata << desktopopenfiledata_tmp << "Exec=" << filename << " open %f\nPath=" << pathname << "\n";
	f = fopen("powdertoy-tpt-open.desktop", "wb");
	if (!f)
		return false;
	fwrite(desktopopenfiledata.str().c_str(), 1, strlen(desktopopenfiledata.str().c_str()), f);
	fclose(f);
	success = system("xdg-mime install powdertoy-save.xml") && success;
	success = system("xdg-desktop-menu install powdertoy-tpt-open.desktop") && success;

	const char *desktopfiledata_tmp =
"[Desktop Entry]\n"
"Version=1.0\n"
"Encoding=UTF-8\n"
"Name=Jacob1's Mod\n"
"Type=Application\n"
"Comment=Physics sandbox game\n"
"Categories=Game;Simulation\n"
"Icon=powdertoy.png\n";
	std::stringstream desktopfiledata;
	desktopfiledata << desktopfiledata_tmp << "Exec=" << filename << "\nPath=" << pathname << "\n";
	f = fopen("powdertoy-jacobsmod.desktop", "wb");
	if (!f)
		return false;
	fwrite(desktopfiledata.str().c_str(), 1, strlen(desktopfiledata.str().c_str()), f);
	fclose(f);
	success = system("xdg-desktop-menu install powdertoy-jacobsmod.desktop") && success;

	f = fopen("powdertoy-save-32.png", "wb");
	if (!f)
		return false;
	fwrite(icon_doc_32_png, 1, icon_doc_32_png_size, f);
	fclose(f);
	f = fopen("powdertoy-save-16.png", "wb");
	if (!f)
		return false;
	fwrite(icon_doc_16_png, 1, icon_doc_16_png_size, f);
	fclose(f);
	f = fopen("powdertoy.png", "wb");
	if (!f)
		return false;
	fwrite(icon_desktop_48_png, 1, icon_desktop_48_png_size, f);
	fclose(f);
	success = system("xdg-icon-resource install --noupdate --context mimetypes --size 32 powdertoy-save-32.png application-vnd.powdertoy.save") && success;
	success = system("xdg-icon-resource install --noupdate --context mimetypes --size 16 powdertoy-save-16.png application-vnd.powdertoy.save") && success;
	success = system("xdg-icon-resource install --noupdate --novendor --size 48 powdertoy.png") && success;
	success = system("xdg-icon-resource forceupdate") && success;
	success = system("xdg-mime default powdertoy-tpt-open.desktop application/vnd.powdertoy.save") && success;
	success = system("xdg-mime default powdertoy-tpt-ptsave.desktop x-scheme-handler/ptsave") && success;
	unlink("powdertoy.png");
	unlink("powdertoy-save-32.png");
	unlink("powdertoy-save-16.png");
	unlink("powdertoy-save.xml");
	unlink("powdertoy-jacobsmod.desktop");
	unlink("powdertoy-tpt-open.desktop");
	unlink("powdertoy-tpt-ptsave.desktop");
	return !success;
#elif defined MACOSX
	return false;
#endif
}

// brings up an on screen keyboard and sends one key input for every key pressed
// the tiny keyboard designed to do this doesn't work, so this will bring up a blocking keyboard
// key presses are still sent one at a time when it is done (also seems to overflow every 90 characters, which seems to be the max)
bool ShowOnScreenKeyboard(const char *str, bool autoCorrect)
{
#ifdef ANDROID
	// keyboard without text input is a lot nicer
	// but for some reason none of the keys work, and mouse input is never sent through outside of the keyboard
	// unless you try to press a key (inside the keyboard) where for some reason it clicks the thing under that key
	//SDL_ANDROID_ToggleScreenKeyboardWithoutTextInput();

	// blocking fullscreen keyboard
	SDL_ANDROID_ToggleScreenKeyboardTextInput(str, autoCorrect);
	return true;
#endif
	return false;
}

// takes a buffer (which can have existing text), and the buffer size
// The user then types the text, which is placed in the buffer for use
void GetOnScreenKeyboardInput(char * buff, int buffSize, bool autoCorrect)
{
#ifdef ANDROID
	SDL_ANDROID_GetScreenKeyboardTextInput(buff, buffSize, autoCorrect);
#endif
}

bool IsOnScreenKeyboardShown()
{
#ifdef ANDROID
	return SDL_IsScreenKeyboardShown(NULL);
#endif
	return false;
}

void Vibrate(int milliseconds)
{
#ifdef ANDROID
	SDL_ANDROID_Vibrate(milliseconds);
#endif
}

int CheckLoadedPtsave()
{
#ifdef ANDROID
	return SDL_ANDROID_GetPtsaveOpen();
#endif
	return 0;
}

bool Stat(std::string filename)
{
#ifdef WIN
	struct _stat s;
	if (_wstat(WinWiden(filename).c_str(), &s) == 0)
#else
	struct stat s;
	if (stat(filename.c_str(), &s) == 0)
#endif
	{
		return true; // Something exists, be it a file, directory, link, etc.
	}
	else
	{
		return false; // Doesn't exist
	}
}

bool FileExists(std::string filename)
{
#ifdef WIN
	struct _stat s;
	if (_wstat(WinWiden(filename).c_str(), &s) == 0)
#else
	struct stat s;
	if (stat(filename.c_str(), &s) == 0)
#endif
	{
		if(s.st_mode & S_IFREG)
		{
			return true; // Is file
		}
		else
		{
			return false; // Is directory or something else
		}
	}
	else
	{
		return false; // Doesn't exist
	}
}

bool DirectoryExists(std::string directory)
{
#ifdef WIN
	struct _stat s;
	if (_wstat(WinWiden(directory).c_str(), &s) == 0)
#else
	struct stat s;
	if (stat(directory.c_str(), &s) == 0)
#endif
	{
		if(s.st_mode & S_IFDIR)
		{
			return true; // Is directory
		}
		else
		{
			return false; // Is file or something else
		}
	}
	else
	{
		return false; // Doesn't exist
	}
}

bool IsLink(std::string path)
{
#ifdef WIN
	struct _stat s;
	if (_wstat(WinWiden(path).c_str(), &s) == 0)
#else
	struct stat s;
	if (stat(path.c_str(), &s) == 0)
#endif
	{
#ifdef WIN
	if (GetFileAttributesW(WinWiden(path).c_str()) & FILE_ATTRIBUTE_REPARSE_POINT)
#else
		if (s.st_mode & S_IFLNK)
#endif
		{
			return true; // Is path
		}
		else
		{
			return false; // Is file or something else
		}
	}
	else
	{
		return false; // Doesn't exist
	}
}

bool DeleteFile(std::string filename)
{
#ifdef WIN
	return _wremove(WinWiden(filename).c_str()) == 0;
#else
	return std::remove(filename.c_str()) == 0;
#endif
}

bool RenameFile(std::string filename, std::string newFilename, bool replace)
{
#ifdef WIN
	if (replace)
	{
		// TODO: we rely on errno but errors from this are available through GetLastError(); fix
		return MoveFileExW(WinWiden(filename).c_str(), WinWiden(newFilename).c_str(), MOVEFILE_REPLACE_EXISTING);
	}
	return _wrename(WinWiden(filename).c_str(), WinWiden(newFilename).c_str()) == 0;
#else
	// TODO: Make atomic :( Could use renameat2 with RENAME_NOREPLACE on linux and
	// renamex_np with RENAME_EXCL on darwin, but both require filesystem support;
	// I don't think it's worth it for now. -- LBPHacker
	if (!replace && FileExists(newFilename))
	{
		return false;
	}
	return rename(filename.c_str(), newFilename.c_str()) == 0;
#endif
}

bool DeleteDirectory(std::string folder)
{
#ifdef WIN
	return _wrmdir(WinWiden(folder).c_str()) == 0;
#else
	return rmdir(folder.c_str()) == 0;
#endif
}

bool MakeDirectory(std::string dir)
{
#ifdef WIN
	return _wmkdir(WinWiden(dir).c_str()) == 0;
#else
	return mkdir(dir.c_str(), 0755) == 0;
#endif
}

// Returns a list of all files in a directory matching a search
// search - list of search terms. extensions - list of extensions to also match
std::vector<std::string> DirectorySearch(std::string directory, std::string search, std::vector<std::string> extensions)
{
	// Get full file listing
	// Normalise directory string, ensure / or \ is present
	if (!directory.size() || (directory.back() != '/' && directory.back() != '\\'))
		directory += PATH_SEP;
	std::vector<std::string> directoryList;
#ifdef WIN
	//Windows
	struct _finddata_t currentFile;
	intptr_t findFileHandle;
	std::string fileMatch = directory + "*.*";
	findFileHandle = _findfirst(fileMatch.c_str(), &currentFile);
	if (findFileHandle == -1L)
	{
#ifdef DEBUG
		printf("Unable to open directory: %s\n", directory.c_str());
#endif
		return std::vector<std::string>();
	}
	do
	{
		directoryList.push_back(currentFile.name);
	}
	while (_findnext(findFileHandle, &currentFile) == 0);
	_findclose(findFileHandle);
#else
	struct dirent * directoryEntry;
	DIR *directoryHandle = opendir(directory.c_str());
	if(!directoryHandle)
	{
#ifdef DEBUG
		printf("Unable to open directory: %s\n", directory.c_str());
#endif
		return std::vector<std::string>();
	}
	while ((directoryEntry = readdir(directoryHandle)))
	{
		directoryList.push_back(directoryEntry->d_name);
	}
	closedir(directoryHandle);
#endif

	std::vector<std::string> searchResults;
	for (std::string &filename : directoryList)
	{
		std::string originalFilename = filename;
		bool extensionMatch = !extensions.size();
		for (std::string &extension : extensions)
		{
			size_t filenameLength = filename.length() - extension.length();
			if (filename.find(extension, filenameLength) == filenameLength)
			{
				extensionMatch = true;
				filename = filename.substr(0, filenameLength);
				break;
			}
		}

		std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
		std::transform(search.begin(), search.end(), search.begin(), ::tolower);

		bool searchMatch = !search.size();
		if (search.size() && filename.find(search) != std::string::npos)
			searchMatch = true;

		if (searchMatch && extensionMatch)
			searchResults.push_back(originalFilename);
	}

	//Filter results
	return searchResults;
}

std::string DoMigration(std::string fromDir, std::string toDir)
{
	if (fromDir.at(fromDir.length() - 1) != '/')
		fromDir = fromDir + '/';
	if (toDir.at(toDir.length() - 1) != '/')
		toDir = toDir + '/';

	std::ofstream logFile(fromDir + "/migrationlog.txt", std::ios::out);
	logFile << "Running migration of data from " << fromDir + " to " << toDir << std::endl;

	// Get lists of files to migrate
	auto stamps = DirectorySearch(fromDir + "stamps", "", { ".stm" });
	auto saves = DirectorySearch(fromDir + "Saves", "", { ".cps", ".stm" });
	auto scripts = DirectorySearch(fromDir + "scripts", "", { ".lua", ".txt" });
	auto downloadedScripts = DirectorySearch(fromDir + "scripts/downloaded", "", { ".lua" });
	bool hasScriptinfo = FileExists(toDir + "scripts/downloaded/scriptinfo");
	auto screenshots = DirectorySearch(fromDir, "screenshot", { ".png" });
	bool hasAutorun = FileExists(fromDir + "autorun.lua");
	bool hasScriptManager = FileExists(fromDir + "scriptmanager.lua");
	bool hasPref = FileExists(fromDir + "powder.pref");

	if (stamps.empty() && saves.empty() && scripts.empty() && downloadedScripts.empty() && screenshots.empty() && !hasAutorun && !hasScriptManager && !hasPref)
	{
		logFile << "Nothing to migrate.";
		return "Nothing to migrate. This button is used to migrate data from pre-96.0 TPT installations to the shared directory";
	}

	std::stringstream result;
	std::stack<std::string> dirsToDelete;

	// Migrate a list of files
	auto migrateList = [&](std::vector<std::string> list, std::string directory, std::string niceName) {
		result << std::endl << niceName << ": ";
		if (!list.empty() && !directory.empty())
			MakeDirectory(toDir + directory);
		int migratedCount = 0, failedCount = 0;
		for (auto &item : list)
		{
			std::string from = fromDir + directory + "/" + item;
			std::string to = toDir + directory + "/" + item;
			if (!FileExists(to))
			{
				if (rename(from.c_str(), to.c_str()))
				{
					failedCount++;
					logFile << "failed to move " << from << " to " << to << std::endl;
				}
				else
				{
					migratedCount++;
					logFile << "moved " << from << " to " << to << std::endl;
				}
			}
			else
			{
				logFile << "skipping " << from << "(already exists)" << std::endl;
			}
		}

		dirsToDelete.push(directory);
		result << "\bt" << migratedCount << " migratated\x0E, \br" << failedCount << " failed\x0E";
		int duplicates = list.size() - migratedCount - failedCount;
		if (duplicates)
			result << ", " << list.size() - migratedCount - failedCount << " skipped (duplicate)";
	};

	// Migrate a single file
	auto migrateFile = [&fromDir, &toDir, &result, &logFile](std::string filename) {
		std::string from = fromDir + filename;
		std::string to = toDir + filename;
		if (!FileExists(to))
		{
			if (rename(from.c_str(), to.c_str()))
			{
				logFile << "failed to move " << from << " to " << to << std::endl;
				result << std::endl << "\br" << filename << " migration failed\x0E";
			}
			else
			{
				logFile << "moved " << from << " to " << to << std::endl;
				result << std::endl << filename << " migrated";
			}
		}
		else
		{
			logFile << "skipping " << from << "(already exists)" << std::endl;
			result << std::endl << filename << " skipped (already exists)";
		}

		if (!DeleteFile(fromDir + filename)) {
			logFile << "failed to delete " << filename << std::endl;
		}
	};

	// Do actual migration
	DeleteFile(fromDir + "stamps/stamps.def");
	migrateList(stamps, "stamps", "Stamps");
	migrateList(saves, "Saves", "Saves");
	if (!scripts.empty())
		migrateList(scripts, "scripts", "Scripts");
	if (!hasScriptinfo && !downloadedScripts.empty())
	{
		migrateList(downloadedScripts, "scripts/downloaded", "Downloaded scripts");
		migrateFile("scripts/downloaded/scriptinfo");
	}
	if (!screenshots.empty())
		migrateList(screenshots, "", "Screenshots");
	if (hasAutorun)
		migrateFile("autorun.lua");
	if (hasScriptManager)
		migrateFile("scriptmanager.lua");
	if (hasPref)
		migrateFile("powder.pref");

	// Delete leftover directories
	while (!dirsToDelete.empty())
	{
		std::string toDelete = dirsToDelete.top();
		if (!DeleteDirectory(fromDir + toDelete)) {
			logFile << "failed to delete " << toDelete << std::endl;
		}
		dirsToDelete.pop();
	}

	// chdir into the new directory
	Platform::ChangeDir(toDir);

	if (stamps.size())
		Stamps::Ref().Rescan();

	logFile << std::endl << std::endl << "Migration complete. Results: " << result.str();
	logFile.close();

	return result.str();
}

#ifdef WIN
std::string WinNarrow(const std::wstring& source)
{
	int buffer_size = WideCharToMultiByte(CP_UTF8, 0, source.c_str(), source.size(), nullptr, 0, NULL, NULL);
	if (!buffer_size)
	{
		return "";
	}
	std::string output(buffer_size, 0);
	if (!WideCharToMultiByte(CP_UTF8, 0, source.c_str(), source.size(), &output[0], buffer_size, NULL, NULL))
	{
		return "";
	}
	return output;
}

std::wstring WinWiden(const std::string& source)
{
	int buffer_size = MultiByteToWideChar(CP_UTF8, 0, source.c_str(), source.size(), nullptr, 0);
	if (!buffer_size)
	{
		return L"";
	}
	std::wstring output(buffer_size, 0);
	if (!MultiByteToWideChar(CP_UTF8, 0, source.c_str(), source.size(), &output[0], buffer_size))
	{
		return L"";
	}
	return output;
}
#endif

}
