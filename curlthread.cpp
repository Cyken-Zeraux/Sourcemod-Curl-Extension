
#include "curlthread.h"

static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
{
	struct timeval tv;
	fd_set infd, outfd, errfd;
	int res; 
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec= (timeout_ms % 1000) * 1000;
 
	FD_ZERO(&infd);
	FD_ZERO(&outfd);
	FD_ZERO(&errfd);
 
	FD_SET(sockfd, &errfd); /* always check for error */ 
 
	if(for_recv)
	{
		FD_SET(sockfd, &infd);
	} else {
		FD_SET(sockfd, &outfd);
	} 
	/* select() returns the number of signalled sockets or -1 */ 
	res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
	return res;
}


cURLThread::cURLThread(cURLHandle *_handle, cURLThread_Type _type):
handle(_handle),type(_type),event(NULL),last_iolen(0),recv_buffer_size(0),recv_buffer(NULL)
{
	switch(type)
	{
		case cURLThread_Type_Send_Recv:
			event = threader->MakeEventSignal();
			break;
	}

}

cURLThread::~cURLThread()
{
	if(event != NULL)
	{
		event->DestroyThis();
		event = NULL;
	}
	if(recv_buffer != NULL)
	{
		delete recv_buffer;
		recv_buffer = NULL;
	}

	g_cURLManager.RemovecURLThread(this);
}

cURLHandle *cURLThread::GetHandle()
{
	return handle;
}

cURLThread_Type cURLThread::GetRunType()
{
	return type;
}

IEventSignal *cURLThread::GetEventSignal()
{
	return event;
}

char *cURLThread::GetBuffer()
{
	return recv_buffer;
}

void cURLThread::SetRecvBufferSize(unsigned int size)
{
	if(recv_buffer_size != 0)
		return;

	if(size == 0)
		size = 1024;

	recv_buffer_size = size;

	recv_buffer = new char[recv_buffer_size];
}

void cURLThread::RunThread_Perform()
{
	g_cURLManager.LoadcURLOption(handle);

	if(handle->lasterror != CURLE_OK)
		return;

	if((handle->lasterror = curl_easy_perform(handle->curl)) != CURLE_OK)
		return;

	handle->lasterror = curl_easy_getinfo(handle->curl, CURLINFO_LASTSOCKET, &handle->sockextr);
}

static void curl_send_FramAction(void *data)
{
	if(data == NULL)
		return;

	cURLThread *thread = (cURLThread*)data;
	cURLHandle *handle = thread->GetHandle();
	IEventSignal *event = thread->GetEventSignal();

	assert((event != NULL));
	
	IPluginFunction *pFunc = handle->callback_Function[cURLThread_Func_Send];
	if(pFunc != NULL)
	{
		pFunc->PushCell(handle->hndl);
		pFunc->PushCell(handle->UserData[1]);
		pFunc->Execute(NULL);
	}

	event->Signal();
}

static void curl_recv_FramAction(void *data)
{
	if(data == NULL)
		return;

	cURLThread *thread = (cURLThread*)data;
	cURLHandle *handle = thread->GetHandle();
	IEventSignal *event = thread->GetEventSignal();

	assert((event != NULL));
	
	IPluginFunction *pFunc = handle->callback_Function[cURLThread_Func_Recv];
	if(pFunc != NULL)
	{
		pFunc->PushCell(handle->hndl);
		pFunc->PushStringEx(thread->GetBuffer(), thread->last_iolen, SM_PARAM_STRING_COPY|SM_PARAM_STRING_BINARY, 0);
		pFunc->PushCell(thread->last_iolen);
		pFunc->PushCell(handle->UserData[1]);
		pFunc->Execute(NULL);
	}

	event->Signal();
}

CURLcode cURLThread::RunThread_Send_Recv()
{
	assert((event != NULL));
	assert((handle->sockextr != INVALID_SOCKET));

	if(handle->sockextr == INVALID_SOCKET || event == NULL)
		return CURLE_SEND_ERROR;

	if(!wait_on_socket(handle->sockextr, 0, handle->timeout))
		return CURLE_OPERATION_TIMEDOUT;
 
	smutils->AddFrameAction(curl_send_FramAction, this);

	event->Wait();

	size_t iolen;
	if(handle->send_buffer == NULL)	
		return CURLE_OK;
	
	CURLcode res = curl_easy_send(handle->curl, handle->send_buffer, strlen(handle->send_buffer), &iolen);
	handle->send_buffer = NULL;
	if(res != CURLE_OK)
		return res;

	size_t new_size = 0;

	for(;;)
	{
		if(!wait_on_socket(handle->sockextr, 1, handle->timeout))
			return CURLE_OPERATION_TIMEDOUT;
 
		memset(recv_buffer, '\0', recv_buffer_size);
		res = curl_easy_recv(handle->curl, recv_buffer, recv_buffer_size, &last_iolen);

		if(res != CURLE_OK) { // no more data receive
			handle->errorBuffer[0] = '\0';
			break;
		}
		 
		new_size += last_iolen;

		smutils->AddFrameAction(curl_recv_FramAction, this);
		event->Wait();
	}	
	return CURLE_OK;
}


static void cUrl_Send_Recv_Finish(void *data)
{
	if(data == NULL)
		return;

	cURLThread *thread = (cURLThread*)data;
	
	if(thread->GetRunType() == cURLThread_Type_Send_Recv)
	{
		cURLHandle *handle = thread->GetHandle();
		IEventSignal *event = thread->GetEventSignal();
		assert((event != NULL));

		IPluginFunction *pFunc = handle->callback_Function[cURLThread_Func_Send_Recv_Complete];
		if(pFunc != NULL)
		{
			pFunc->PushCell(handle->hndl);
			pFunc->PushCell(handle->lasterror);
			pFunc->PushCell(handle->UserData[0]);
			pFunc->Execute(&thread->result);

			event->Signal();
		}
	}
}

void cURLThread::RunThread(IThreadHandle *pHandle)
{
	handle->running = true;

	if(type == cURLThread_Type_Perform)
	{
		RunThread_Perform();
	} else if(type == cURLThread_Type_Send_Recv) {
		result = 0;
		handle->lasterror = CURLE_OK;
		while(result == 0 && handle->lasterror == CURLE_OK)
		{
			handle->lasterror = RunThread_Send_Recv();

			smutils->AddFrameAction(cUrl_Send_Recv_Finish, this);
			event->Wait();
		}
	}
}


static void cUrl_Thread_Finish(void *data)
{
	if(data == NULL)
		return;

	cURLThread *thread = (cURLThread*)data;
	cURLHandle *handle = thread->GetHandle();
	
	IPluginFunction *pFunc = handle->callback_Function[cURLThread_Func_Complete];
	if(pFunc != NULL)
	{
		pFunc->PushCell(handle->hndl);
		pFunc->PushCell(handle->lasterror);
		pFunc->PushCell(handle->UserData[0]);
		pFunc->Execute(NULL);
	}

	delete thread;
}

void cURLThread::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	handle->running = false;
	smutils->AddFrameAction(cUrl_Thread_Finish, this);
}

