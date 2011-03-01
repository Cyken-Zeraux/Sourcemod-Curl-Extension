#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CURLTHREAD_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CURLTHREAD_H_

#include "extension.h"
#include "curlmanager.h"
#include <curl/curl.h>

struct cURLHandle;
enum cURLThread_Func;
enum cURLThread_Type;
enum SendRecv_Act;

class cURLThread :
	public IThread
{
	friend class cURLManager;

public:
	cURLThread(cURLHandle *_handle, cURLThread_Type _type);
	~cURLThread();
	cURLHandle *GetHandle();
	cURLThread_Type GetRunType();
	void EventSignal();
	bool IsWaiting();
	char *GetBuffer();
	void SetRecvBufferSize(unsigned int size);
	void SetSenRecvAction(SendRecv_Act act);

public:
	void RunThread(IThreadHandle *pHandle);
	void OnTerminate(IThreadHandle *pHandle, bool cancel);

public:
	void RunThread_Perform();
	void RunThread_Send_Recv();

private:
	bool waiting;
	cURLHandle *handle;
	cURLThread_Type type;
	IEventSignal *event;
	char *recv_buffer;
	unsigned int recv_buffer_size;
	SendRecv_Act send_recv_act;

public:
	size_t last_iolen;
};


#endif
