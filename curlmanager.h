#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CURLMANAGER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CURLMANAGER_H_

#include "extension.h"
#include "curlthread.h"
#include <sh_list.h>
#include <curl/curl.h>

class cURLThread;

enum SendRecv_Act {
	SendRecv_Act_NOTHING = 0,

	SendRecv_Act_GOTO_SEND,
	SendRecv_Act_GOTO_RECV,
	SendRecv_Act_GOTO_WAIT,
	SendRecv_Act_GOTO_END,

	SendRecv_Act_LAST,
};

enum cURLThread_Type {
	cURLThread_Type_NOTHING = 0,

	cURLThread_Type_PERFORM,
	cURLThread_Type_SEND_RECV,

	cURLThread_Type_LAST,
};

enum cURLThread_Func {
	cURLThread_Func_NOTHING = 0,

	cURLThread_Func_COMPLETE,
	cURLThread_Func_SEND,
	cURLThread_Func_RECV,

	cURLThread_Func_LAST,
};

struct cURLOpt_string {
	CURLoption opt;
	char *value;
};

struct cURLOpt_int {
	CURLoption opt;
	int value;
};

struct cURLOpt_int64 {
	CURLoption opt;
	curl_off_t value;
};

struct cURLOpt_pointer {
	CURLoption opt;
	void *value;
	void *handle_obj;
};

struct cURLHandle {
	cURLHandle():curl(NULL),running(false),lasterror(CURLE_OK),opt_loaded(false),
		sockextr(INVALID_SOCKET),send_timeout(60000),recv_timeout(60000),send_buffer(NULL),thread(NULL)
	{
		memset(errorBuffer,0,sizeof(errorBuffer));
		memset(callback_Function, 0, sizeof(callback_Function));
		memset(UserData,0,sizeof(UserData));
	}
	CURL *curl;
	char errorBuffer[CURL_ERROR_SIZE];
	SourceHook::List<cURLOpt_string *> opt_string_list;
	SourceHook::List<cURLOpt_int *> opt_int_list;
	SourceHook::List<cURLOpt_int64 *> opt_int64_list;
	SourceHook::List<cURLOpt_pointer *> opt_pointer_list;
	bool running;
	CURLcode lasterror;
	bool opt_loaded;
	IPluginFunction *callback_Function[cURLThread_Func_LAST];
	Handle_t hndl;
	int UserData[2];
	cURLThread *thread;

	/* use for send & recv */
	long sockextr;
	long send_timeout;
	long recv_timeout;
	unsigned char *send_buffer;
	unsigned int send_buffer_length;
};

struct cURL_slist_pack {
	cURL_slist_pack():chunk(NULL)
	{
	}
	curl_slist *chunk;
};

struct WebForm {
	WebForm():first(NULL), last(NULL)
	{
	}
	curl_httppost *first;
	curl_httppost *last;
};


class cURLManager
{
public:
	void SDK_OnLoad();
	void SDK_OnUnload();
public:
	bool IsShutdown();

public:
	void CreatecURLThread(cURLThread *thread);
	void RemovecURLThread(cURLThread *thread);

public:
	void RemovecURLHandle(cURLHandle *handle);
	bool AddcURLOptionString(cURLHandle *handle, CURLoption opt, char *value);
	bool AddcURLOptionInt(cURLHandle *handle, CURLoption opt, int value);
	bool AddcURLOptionInt64(cURLHandle *handle, CURLoption opt, long long value);
	bool AddcURLOptionHandle(IPluginContext *pContext, cURLHandle *handle, HandleSecurity *sec, CURLoption opt, Handle_t hndl);
	void LoadcURLOption(cURLHandle *handle);

private:
	void FreeOptionPointer(cURLOpt_pointer *pInfo);
		
public:
	CURLFORMcode cURLFormAdd(IPluginContext *pContext, const cell_t *params, WebForm *handle);

private:
	bool shutdown;
	bool waiting;
	IMutex *curlhandle_list_mutex;
	IEventSignal *shutdown_event;
	SourceHook::List<cURLThread *> g_cURLThread_List;
};

extern cURLManager g_cURLManager;


#endif