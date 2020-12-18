#include "platform.h"

#include <SDL_events.h>
#ifdef WIN32
#include <codecvt>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include "Window.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Scripting.h"

#if !defined(WIN32)
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#endif

// #define DEVTEST

int runShutdownCommand()
{
#ifdef WIN32 // windows
	return system("shutdown -s -t 0");
#else // osx / linux	
	return system("shutdown -h now");
#endif
}

int runRestartCommand()
{
#ifdef WIN32 // windows	
	return system("shutdown -r -t 0");
#else // osx / linux	
	return system("shutdown -r now");
#endif
}

int runSystemCommand(const std::string& cmd_utf8, const std::string& name, Window* window)
{
	if (window != NULL)
		window->renderSplashScreen();

#ifdef WIN32
	// on Windows we use _wsystem to support non-ASCII paths
	// which requires converting from utf8 to a wstring
	//typedef std::codecvt_utf8<wchar_t> convert_type;
	//std::wstring_convert<convert_type, wchar_t> converter;
	//std::wstring wchar_str = converter.from_bytes(cmd_utf8);
	std::string command = cmd_utf8;

#define BUFFER_SIZE 8192

	TCHAR szEnvPath[BUFFER_SIZE];
	DWORD dwLen = ExpandEnvironmentStringsA(command.c_str(), szEnvPath, BUFFER_SIZE);
	if (dwLen > 0 && dwLen < BUFFER_SIZE)
		command = std::string(szEnvPath);

	std::string exe;
	std::string args;

	Utils::FileSystem::splitCommand(command, &exe, &args);
	exe = Utils::FileSystem::getPreferredPath(exe);

	SHELLEXECUTEINFO lpExecInfo;
	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	lpExecInfo.lpFile = exe.c_str();
	lpExecInfo.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS;
	lpExecInfo.hwnd = NULL;
	lpExecInfo.lpVerb = "open"; // to open  program

	lpExecInfo.lpDirectory = NULL;
	lpExecInfo.nShow = SW_SHOW;  // show command prompt with normal window size 
	lpExecInfo.hInstApp = (HINSTANCE)SE_ERR_DDEFAIL;   //WINSHELLAPI BOOL WINAPI result;
	lpExecInfo.lpParameters = args.c_str(); //  file name as an argument	

	// Don't set directory for relative paths
	if (!Utils::String::startsWith(exe, ".") && !Utils::String::startsWith(exe, "/") && !Utils::String::startsWith(exe, "\\"))
		lpExecInfo.lpDirectory = Utils::FileSystem::getAbsolutePath(Utils::FileSystem::getParent(exe)).c_str();

	ShellExecuteEx(&lpExecInfo);

	if (lpExecInfo.hProcess != NULL)
	{
		if (window == NULL)
			WaitForSingleObject(lpExecInfo.hProcess, INFINITE);
		else
		{
			while (WaitForSingleObject(lpExecInfo.hProcess, 50) == 0x00000102L)
			{
				bool polled = false;

				SDL_Event event;
				while (SDL_PollEvent(&event))
					polled = true;

				if (window != NULL && polled)
					window->renderSplashScreen();
			}
		}

		CloseHandle(lpExecInfo.hProcess);
		return 0;
	}

	return 1;
#elif _ENABLEEMUELEC
	return system((cmd_utf8 + " 2> /storage/.config/emuelec/logs/es_launch_stderr.log > /storage/.config/emuelec/logs/es_launch_stdout.log").c_str()); // emuelec
#else
	return system((cmd_utf8 + " 2> /userdata/system/logs/es_launch_stderr.log | head -300 > /userdata/system/logs/es_launch_stdout.log").c_str()); // batocera
#endif
}

QuitMode quitMode = QuitMode::QUIT;

int quitES(QuitMode mode)
{
	quitMode = mode;

	switch (quitMode)
	{
		case QuitMode::QUIT:
			Scripting::fireEvent("quit");
			break;

		case QuitMode::REBOOT:
		case QuitMode::FAST_REBOOT:
			Scripting::fireEvent("quit", "reboot");
			Scripting::fireEvent("reboot");
			break;

		case QuitMode::SHUTDOWN:
		case QuitMode::FAST_SHUTDOWN:
			Scripting::fireEvent("quit", "shutdown");
			Scripting::fireEvent("shutdown");
			break;
	}


	SDL_Event* quit = new SDL_Event();
	quit->type = SDL_QUIT;
	SDL_PushEvent(quit);
	return 0;
}

void touch(const std::string& filename)
{
#ifndef WIN32
	int fd = open(filename.c_str(), O_CREAT | O_WRONLY, 0644);
	if (fd >= 0)
		close(fd);

	// system(("touch " + filename).c_str());
#endif	
}

void processQuitMode()
{

	switch (quitMode)
	{
	case QuitMode::RESTART:
		LOG(LogInfo) << "Restarting EmulationStation";
		touch("/tmp/restart.please");
		break;
	case QuitMode::REBOOT:
	case QuitMode::FAST_REBOOT:
		LOG(LogInfo) << "Rebooting system";
		touch("/tmp/reboot.please");
		runRestartCommand();
		break;
	case QuitMode::SHUTDOWN:
	case QuitMode::FAST_SHUTDOWN:
		LOG(LogInfo) << "Shutting system down";
		touch("/tmp/shutdown.please");
		runShutdownCommand();
		break;
	}
}

bool isFastShutdown()
{
	return quitMode == QuitMode::FAST_REBOOT || quitMode == QuitMode::FAST_SHUTDOWN;
}

std::string queryIPAdress()
{
#ifdef DEVTEST
	return "127.0.0.1";
#endif

	std::string result;

#if WIN32
	// Init WinSock
	WSADATA wsa_Data;
	int wsa_ReturnCode = WSAStartup(0x101, &wsa_Data);
	if (wsa_ReturnCode != 0)
		return "";

	char* szLocalIP = nullptr;

	// Get the local hostname
	char szHostName[255];
	if (gethostname(szHostName, 255) == 0)
	{
		struct hostent *host_entry;
		host_entry = gethostbyname(szHostName);
		if (host_entry != nullptr)
			szLocalIP = inet_ntoa(*(struct in_addr *)*host_entry->h_addr_list);
	}

	WSACleanup();

	if (szLocalIP == nullptr)
		return "";

	return std::string(szLocalIP); // "127.0.0.1"
#else
	struct ifaddrs *ifAddrStruct = NULL;
	struct ifaddrs *ifa = NULL;
	void *tmpAddrPtr = NULL;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
	{
		if (!ifa->ifa_addr)
			continue;
		
		// check it is IP4 is a valid IP4 Address
		if (ifa->ifa_addr->sa_family == AF_INET)
		{ 			
			tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			
			if (std::string(ifa->ifa_name).find("eth") != std::string::npos || std::string(ifa->ifa_name).find("wlan") != std::string::npos) 
			{
				result = std::string(addressBuffer);
				break;
			}
		}
	}
	// Seeking for ipv6 if no IPV4
	if (result.empty()) 
	{
		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
		{
			if (!ifa->ifa_addr)
				continue;
			
			// check it is IP6 is a valid IP6 Address
			if (ifa->ifa_addr->sa_family == AF_INET6) 
			{ 				
				tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
				char addressBuffer[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
				printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);

				if (std::string(ifa->ifa_name).find("eth") != std::string::npos || std::string(ifa->ifa_name).find("wlan") != std::string::npos)
				{
					result = std::string(addressBuffer);
					break;
				}
			}
		}
	}

	if (ifAddrStruct != NULL)
		freeifaddrs(ifAddrStruct);
#endif

	return result;

}

BatteryInformation queryBatteryInformation()
{
	BatteryInformation ret; 
	ret.hasBattery = false;
	ret.isCharging = false;
	ret.level = 0;

#ifdef DEVTEST
	ret.hasBattery = true;
	ret.isCharging = true;
	ret.level = 33;

	time_t     clockNow = time(0);
	struct tm  clockTstruct = *localtime(&clockNow);
	ret.level = clockTstruct.tm_min;

	return ret;
#endif

#ifdef WIN32
	SYSTEM_POWER_STATUS systemPowerStatus;
	if (GetSystemPowerStatus(&systemPowerStatus))
	{
		if ((systemPowerStatus.BatteryFlag & 128) == 128)
			ret.hasBattery = false;
		else
		{
			ret.hasBattery = true;
			ret.isCharging = (systemPowerStatus.BatteryFlag & 8) == 8;
			ret.level = systemPowerStatus.BatteryLifePercent;
		}
	}
	else
		ret.hasBattery = false;

	return ret;

#endif

	static std::string batteryStatusPath;
	static std::string batteryCapacityPath;

	// Find battery path - only at the first call
	if (batteryStatusPath.empty())
	{
		std::string batteryRootPath;

		auto files = Utils::FileSystem::getDirContent("/sys/class/power_supply");
		for (auto file : files)
		{
			if (Utils::String::toLower(file).find("/bat") != std::string::npos)
			{
				batteryRootPath = file;
				break;
			}
		}

		if (batteryRootPath.empty())
			batteryStatusPath = ".";
		else
		{
			batteryStatusPath = batteryRootPath + "/status";
			batteryCapacityPath = batteryRootPath + "/capacity";
		}
	}

	if (batteryStatusPath.length() <= 1)
	{
		ret.hasBattery = false;
		ret.isCharging = false;
		ret.level = 0;
	}
	else
	{
		ret.hasBattery = true;
		ret.isCharging = (Utils::String::replace(Utils::FileSystem::readAllText(batteryStatusPath), "\n", "") != "Discharging");
		ret.level = atoi(Utils::FileSystem::readAllText(batteryCapacityPath).c_str());
	}

	return ret;
}
#ifdef _ENABLEEMUELEC

/* < emuelec */
std::string getShOutput(const std::string& mStr)
{
    std::string result, file;
    FILE* pipe{popen(mStr.c_str(), "r")};
    char buffer[256];

    while(fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        file = buffer;
        result += file.substr(0, file.size() - 1);
    }

    pclose(pipe);
    return result;
}
/* emuelec >*/
#endif
