#if WIN32
#pragma once

#include "ApiSystem.h"

class Win32ApiSystem : public ApiSystem
{
public:
	bool isScriptingSupported(ScriptId script) override;
	std::string getVersion() override;

	std::vector<std::string> getSystemInformations() override;
	std::vector<std::string> getAvailableStorageDevices() override;

	unsigned long getFreeSpaceGB(std::string mountpoint) override;
	std::string getFreeSpaceInfo(const std::string dir);
	std::string getFreeSpaceUserInfo() override;
	std::string getFreeSpaceSystemInfo() override;	
	std::vector<std::string> getVideoModes() override;

	void setReadyFlag(bool ready = true) override;
	bool isReadyFlagSet() override;

	// Themes
	std::vector<BatoceraTheme> getBatoceraThemesList() override;
	std::pair<std::string, int> installBatoceraTheme(std::string thname, const std::function<void(const std::string)>& func) override;
	std::pair<std::string, int> uninstallBatoceraTheme(std::string bezelsystem, const std::function<void(const std::string)>& func = nullptr) override;

	// Bezels
	virtual std::vector<BatoceraBezel> getBatoceraBezelsList();
	virtual std::pair<std::string, int> installBatoceraBezel(std::string bezelsystem, const std::function<void(const std::string)>& func = nullptr);
	virtual std::pair<std::string, int> uninstallBatoceraBezel(std::string bezelsystem, const std::function<void(const std::string)>& func = nullptr);

	// Updates
	std::pair<std::string, int> updateSystem(const std::function<void(const std::string)>& func) override;
	bool canUpdate(std::vector<std::string>& output) override;

	bool ping() override;

	bool launchKodi(Window *window) override;	

	static std::string getEmulatorLauncherPath(const std::string variable);

	std::vector<std::string> getShaderList(const std::string systemName = "") override;

	virtual std::string getSevenZipCommand() override;

protected:
	bool executeScript(const std::string command) override;
	std::pair<std::string, int> executeScript(const std::string command, const std::function<void(const std::string)>& func) override;
	std::vector<std::string> executeEnumerationScript(const std::string command) override;

private:
	void updateEmulatorLauncher(const std::function<void(const std::string)>& func);
};



#endif
