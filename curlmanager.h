#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CURLMANAGER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CURLMANAGER_H_

#include "extension.h"
#include "curlthread.h"
#include <sh_list.h>
#include <curl/curl.h>

struct cURL_slist_pack {
	cURL_slist_pack():chunk(NULL)
	{
	}
	curl_slist *chunk;
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
};

struct cURLHandle {
	cURLHandle():curl(NULL),running(false),lasterror(CURLE_OK),opt_loaded(false),
		callback_Function(NULL)
	{
		memset(errorBuffer,0,sizeof(errorBuffer));
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
	IPluginFunction *callback_Function;
	Handle_t hndl;
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
	void RemovecURLHandle(cURLHandle *handle);
	bool AddcURLOptionString(cURLHandle *handle, CURLoption opt, char *value);
	bool AddcURLOptionInt(cURLHandle *handle, CURLoption opt, int value);
	bool AddcURLOptionInt64(cURLHandle *handle, CURLoption opt, long long value);
	bool AddcURLOptionHandle(IPluginContext *pContext, cURLHandle *handle, HandleSecurity *sec, CURLoption opt, Handle_t hndl);
	void LoadcURLOption(cURLHandle *handle);

public:
	CURLFORMcode cURLFormAdd(IPluginContext *pContext, const cell_t *params, WebForm *handle);
};

extern cURLManager g_cURLManager;

#endif