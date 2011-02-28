
#include "curlthread.h"
#include <openssl/crypto.h>


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
handle(_handle),type(_type),event(NULL),last_iolen(0),recv_buffer_size(0),recv_buffer(NULL),
waiting(false)
{
	assert((type > cURLThread_Type_NOTHING && type < cURLThread_Type_LAST));
	handle->running = true;
	handle->thread = this;
	event = threader->MakeEventSignal();
	assert((event != NULL));
}

cURLThread::~cURLThread()
{
	waiting = false;

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

void cURLThread::EventSignal()
{
	assert((event != NULL));
	event->Signal();
}

bool cURLThread::IsWaiting()
{
	return waiting;
}

/*void cURLThread::EventWait()
{
	assert((event != NULL));
	waiting = true;
	event->Wait();
	waiting = false;
}*/

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

void cURLThread::SetSenRecvAction(SendRecv_Act act)
{
	assert((act > SendRecv_Act_NOTHING && act < SendRecv_Act_LAST));
	send_recv_act = act;
}

void cURLThread::RunThread_Perform()
{
	g_cURLManager.LoadcURLOption(handle);

	if(handle->lasterror != CURLE_OK)
		return;

	//g_cURLManager.test();
	
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
	
	IPluginFunction *pFunc = handle->callback_Function[cURLThread_Func_SEND];
	assert((pFunc != NULL));
	if(pFunc != NULL)
	{
		cell_t result;
		pFunc->PushCell(handle->hndl);
		pFunc->PushCell(thread->last_iolen);
		pFunc->PushCell(handle->UserData[1]);
		pFunc->Execute(&result);
		thread->SetSenRecvAction((SendRecv_Act)result);
	}

	thread->EventSignal();
}

static void curl_recv_FramAction(void *data)
{
	if(data == NULL)
		return;

	cURLThread *thread = (cURLThread*)data;
	cURLHandle *handle = thread->GetHandle();
	
	IPluginFunction *pFunc = handle->callback_Function[cURLThread_Func_RECV];
	assert((pFunc != NULL));
	if(pFunc != NULL)
	{
		cell_t result;
		pFunc->PushCell(handle->hndl);
		pFunc->PushStringEx(thread->GetBuffer(), thread->last_iolen, SM_PARAM_STRING_COPY|SM_PARAM_STRING_BINARY, 0);
		pFunc->PushCell(thread->last_iolen);
		pFunc->PushCell(handle->UserData[1]);
		pFunc->Execute(&result);
		thread->SetSenRecvAction((SendRecv_Act)result);
	}
	thread->EventSignal();
}

CURLcode cURLThread::RunThread_Send_Recv()
{
	assert((handle->sockextr != INVALID_SOCKET));

	if(handle->sockextr == INVALID_SOCKET || event == NULL)
		return CURLE_SEND_ERROR;

	CURLcode res = CURLE_OK;

/* Select Action */
select_action:
	if(send_recv_act == SendRecv_Act_GOTO_SEND)
		goto act_send;
	else if(send_recv_act == SendRecv_Act_GOTO_RECV)
		goto act_recv;
	else if(send_recv_act == SendRecv_Act_GOTO_WAIT)
		goto act_wait;
	else
		goto act_end;


/* Send Action */
act_send:
	smutils->AddFrameAction(curl_send_FramAction, this);
	waiting = true;
	event->Wait();
	waiting = false;
	if(g_cURLManager.IsShutdown())
		goto act_end;

	if(!wait_on_socket(handle->sockextr, 0, handle->timeout))
		return CURLE_OPERATION_TIMEDOUT;

	if(handle->send_buffer == NULL)  // wtf
		return CURLE_SEND_ERROR;		
	
	// TODO: binary data
	res = curl_easy_send(handle->curl, handle->send_buffer, strlen(handle->send_buffer), &last_iolen);
	handle->send_buffer = NULL;
	if(res != CURLE_OK)
		return res;

	goto select_action;



/* Recv Action */
act_recv:
	if(!wait_on_socket(handle->sockextr, 1, handle->timeout))
		return CURLE_OPERATION_TIMEDOUT;
	
	memset(recv_buffer, 0, recv_buffer_size);
	res = curl_easy_recv(handle->curl, recv_buffer, recv_buffer_size, &last_iolen);
	if(res != CURLE_OK)
		return res;	
	
	smutils->AddFrameAction(curl_recv_FramAction, this);
	goto act_wait;



/* Wait Action */
act_wait:
	waiting = true;
	event->Wait();
	waiting = false;
	if(g_cURLManager.IsShutdown())
		goto act_end;
	goto select_action; // select action again



/* End Action */
act_end:
	return res;
}


void cURLThread::RunThread(IThreadHandle *pHandle)
{
	if(type == cURLThread_Type_PERFORM)
	{
		RunThread_Perform();
	} else if(type == cURLThread_Type_SEND_RECV) {
		handle->lasterror = RunThread_Send_Recv();
	}
}


static void cUrl_Thread_Finish(void *data)
{
	if(data == NULL)
		return;

	cURLThread *thread = (cURLThread*)data;
	cURLHandle *handle = thread->GetHandle();
		
	IPluginFunction *pFunc = handle->callback_Function[cURLThread_Func_COMPLETE];
	assert((pFunc != NULL));
	if(pFunc != NULL)
	{
		pFunc->PushCell(handle->hndl);
		pFunc->PushCell(handle->lasterror);
		pFunc->PushCell(handle->UserData[0]);
		pFunc->Execute(NULL);
	}

	thread->EventSignal();
}

void cURLThread::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	handle->running = false;
	if(g_cURLManager.IsShutdown())
	{
		g_cURLManager.RemovecURLHandle(handle);
	} else {
		smutils->AddFrameAction(cUrl_Thread_Finish, this);
		waiting = true;
		event->Wait();
		waiting = false;
	}
	delete this;
}

