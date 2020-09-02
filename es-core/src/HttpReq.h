#pragma once
#ifndef ES_CORE_HTTP_REQ_H
#define ES_CORE_HTTP_REQ_H

#include <curl/curl.h>
#include <map>
#include <sstream>
#include <fstream>
#include <stdio.h>

/* Usage:
 * HttpReq myRequest("www.google.com", "/index.html");
 * //for blocking behavior: while(myRequest.status() == HttpReq::REQ_IN_PROGRESS);
 * //for non-blocking behavior: check if(myRequest.status() != HttpReq::REQ_IN_PROGRESS) in some sort of update method
 * 
 * //once one of those completes, the request is ready
 * if(myRequest.status() != REQ_SUCCESS)
 * {
 *    //an error occured
 *    LOG(LogError) << "HTTP request error - " << myRequest.getErrorMessage();
 *    return;
 * }
 *
 * std::string content = myRequest.getContent();
 * //process contents...
*/

class HttpReq
{
public:
	HttpReq(const std::string& url, const std::string outputFilename = "");
	~HttpReq();

	enum Status
	{
		REQ_IN_PROGRESS = 0,
		REQ_IO_ERROR	= 3,				
		REQ_FILESTREAM_ERROR = 4,		

		REQ_SUCCESS = 200,
		REQ_400_BADREQUEST = 400,
		REQ_401_FORBIDDEN = 401,
		REQ_403_BADLOGIN = 403,
		REQ_404_NOTFOUND = 404,
		
		REQ_426_SERVERMAINTENANCE = 423,
		REQ_426_BLACKLISTED = 426,
		REQ_429_TOOMANYREQUESTS = 429,

		REQ_430_TOOMANYSCRAPS = 430,
		REQ_430_TOOMANYFAILURES = 431
	};

	Status status(); //process any received data and return the status afterwards

	std::string getErrorMsg();

	std::string getContent(); // mStatus must be REQ_SUCCESS

	// int saveContent(const std::string filename, bool checkMedia = false);

	static std::string urlEncode(const std::string &s);
	static bool isUrl(const std::string& s);

	int getPercent() { return mPercent; }
	int getPosition() { return mPosition; }

	std::string getUrl() { return mUrl; }
	bool wait();

private:
	void closeStream();

	static size_t write_content(void* buff, size_t size, size_t nmemb, void* req_ptr);
	//static int update_progress(void* req_ptr, double dlTotal, double dlNow, double ulTotal, double ulNow);

	//god dammit libcurl why can't you have some way to check the status of an individual handle
	//why do I have to handle ALL messages at once
	static std::map<CURL*, HttpReq*> s_requests;

	static CURLM* s_multi_handle;

	void onError(const char* msg);

	CURL* mHandle;

	Status mStatus;

	// string steam mode
	std::stringstream mContent;

	// file stream mode
	std::string   mFilePath;
	std::string   mTempStreamPath;	
	FILE*		  mFile;

	std::string mErrorMsg;
	std::string mUrl;

	int mPercent;
	double mPosition;
};

#endif // ES_CORE_HTTP_REQ_H
