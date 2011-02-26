#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CURLTHREAD_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CURLTHREAD_H_

#include "extension.h"
#include "curlmanager.h"

struct cURLHandle;

class cURLThread :
	public IThread
{
public:
	cURLThread(cURLHandle *_handle);
	cURLHandle *GetHandle();

public:
	void RunThread(IThreadHandle *pHandle);
	void OnTerminate(IThreadHandle *pHandle, bool cancel);

private:
	cURLHandle *handle;
};


#endif
