#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CURLTHREAD_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CURLTHREAD_H_

#include "extension.h"
#include "curlmanager.h"
#include <curl/curl.h>

struct cURLHandle;
enum cURLThread_Func;
enum cURLThread_Type;


class cURLThread :
	public IThread
{
public:
	cURLThread(cURLHandle *_handle, cURLThread_Type _type);
	~cURLThread();
	cURLHandle *GetHandle();
	cURLThread_Type GetRunType();
	IEventSignal *GetEventSignal();
	char *GetBuffer();
	void SetRecvBufferSize(unsigned int size);

public:
	void RunThread(IThreadHandle *pHandle);
	void OnTerminate(IThreadHandle *pHandle, bool cancel);

public:
	void RunThread_Perform();
	CURLcode RunThread_Send_Recv();

private:
	cURLHandle *handle;
	cURLThread_Type type;
	IEventSignal *event;
	char *recv_buffer;
	unsigned int recv_buffer_size;

public:
	size_t last_iolen;
	cell_t result;
};


#endif
