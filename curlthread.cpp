
#include "curlthread.h"

cURLThread::cURLThread(cURLHandle *_handle):handle(_handle)
{
}

cURLHandle *cURLThread::GetHandle()
{
	return handle;
}

void cURLThread::RunThread(IThreadHandle *pHandle)
{
	handle->running = true;

	CURL *curl = handle->curl;

	g_cURLManager.LoadcURLOption(handle);
	
	if(handle->lasterror != CURLE_OK)
		return;

	if((handle->lasterror = curl_easy_perform(curl)) != CURLE_OK)
		return;
}

static void cUrl_Complete(void *data)
{
	if(data == NULL)
		return;

	cURLThread *thread = (cURLThread*)data;
	cURLHandle *handle = thread->GetHandle();

	handle->callback_Function->PushCell(handle->hndl);
	handle->callback_Function->PushCell(handle->lasterror);
	handle->callback_Function->Execute(NULL);

	delete thread;
}

void cURLThread::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	smutils->AddFrameAction(cUrl_Complete, this);
}

