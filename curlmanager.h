#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CURLMANAGER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CURLMANAGER_H_

#include "extension.h"
#include "curlthread.h"

class cURLManager
{
public:
	void SDK_OnLoad();
	void SDK_OnUnload();

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
	bool waiting;
	IMutex *curlhandle_list_mutex;
	IEventSignal *shutdown_event;
	SourceHook::List<cURLThread *> g_cURLThread_List;
};

extern cURLManager g_cURLManager;

#endif

