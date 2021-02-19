#include "NetworkThread.h"
#include "ApiSystem.h"
#include "SystemConf.h"
#include "guis/GuiMsgBox.h"
#include "LocaleES.h"
#include "Log.h"
#include <chrono>
#include <thread>

NetworkThread::NetworkThread(Window* window) : mWindow(window)
{    
	LOG(LogDebug) << "NetworkThread : Starting";

    // creer le thread
    mFirstRun = true;
    mRunning = true;
	mThread = new std::thread(&NetworkThread::run, this);
}

NetworkThread::~NetworkThread() 
{
	LOG(LogDebug) << "NetworkThread : Exit";

	mRunning = false;
	mThread->join();
	delete mThread;
}

void NetworkThread::run() 
{
	while (mRunning) 
	{
		if (mFirstRun) 
		{
			std::this_thread::sleep_for(std::chrono::seconds(15));
			mFirstRun = false;
		}
		else
			std::this_thread::sleep_for(std::chrono::hours(1));		

		if (!SystemConf::getInstance()->getBool("updates.enabled"))
			continue;
		
		LOG(LogDebug) << "NetworkThread : Checking for updates";

		std::vector<std::string> msgtbl;
		if (ApiSystem::getInstance()->canUpdate(msgtbl)) 
		{
			std::string msg = "";
			for (int i = 0; i < msgtbl.size(); i++)
			{
				if (i != 0) msg += "\n";
				msg += msgtbl[i];
			}

			LOG(LogDebug) << "NetworkThread : Update available " << msg.c_str();
			mWindow->displayNotificationMessage(_U("\uF019  ") + _("UPDATE AVAILABLE") + std::string(": ") + msg);
			mRunning = false;
		}
		else
			LOG(LogDebug) << "NetworkThread : No update found";				
	}
}