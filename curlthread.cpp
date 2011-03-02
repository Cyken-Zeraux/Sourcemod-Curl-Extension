
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
		pFunc->PushCell(handle->lasterror);
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
		pFunc->PushCell(handle->lasterror);
		pFunc->PushStringEx(thread->GetBuffer(), thread->last_iolen, SM_PARAM_STRING_COPY|SM_PARAM_STRING_BINARY, 0);
		pFunc->PushCell(thread->last_iolen);
		pFunc->PushCell(handle->UserData[1]);
		pFunc->Execute(&result);
		thread->SetSenRecvAction((SendRecv_Act)result);
	}
	thread->EventSignal();
}

void cURLThread::RunThread_Send_Recv()
{
	assert((handle->sockextr != INVALID_SOCKET));

	if(handle->sockextr == INVALID_SOCKET || event == NULL)
	{
		handle->lasterror = CURLE_SEND_ERROR;
		return;
	}

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
	if(!wait_on_socket(handle->sockextr, 0, handle->send_timeout))
	{
		handle->lasterror = CURLE_OPERATION_TIMEDOUT;
		goto sm_send_frame;
	}

	if(handle->send_buffer == NULL)
	{
		handle->lasterror = CURLE_SEND_ERROR;	
		goto sm_send_frame;
	}
	
	handle->lasterror = curl_easy_send(handle->curl, handle->send_buffer, handle->send_buffer_length, &last_iolen);
	delete handle->send_buffer;
	handle->send_buffer = NULL;

	// put res to frame, let frame do action
sm_send_frame:
	smutils->AddFrameAction(curl_send_FramAction, this);
	waiting = true;
	event->Wait();
	waiting = false;
	if(g_cURL_SM.IsShutdown())
		goto act_end;

	goto select_action;


/* Recv Action */
act_recv:
	if(!wait_on_socket(handle->sockextr, 1, handle->recv_timeout))
	{
		handle->lasterror = CURLE_OPERATION_TIMEDOUT;
		goto sm_recv_frame;
	}
	
	memset(recv_buffer, 0, recv_buffer_size);
	handle->lasterror = curl_easy_recv(handle->curl, recv_buffer, recv_buffer_size, &last_iolen);
	
sm_recv_frame:
	smutils->AddFrameAction(curl_recv_FramAction, this);
	goto act_wait;


/* Wait Action */
act_wait:
	if(g_cURL_SM.IsShutdown())
		goto act_end;
	waiting = true;
	event->Wait();
	waiting = false;
	if(g_cURL_SM.IsShutdown())
		goto act_end;
	goto select_action; // select action again



/* End Action */
act_end:
	return;
}


void cURLThread::RunThread(IThreadHandle *pHandle)
{
	if(type == cURLThread_Type_PERFORM)
	{
		RunThread_Perform();
	} else if(type == cURLThread_Type_SEND_RECV) {
		RunThread_Send_Recv();
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
	if(!g_cURL_SM.IsShutdown())
	{
		smutils->AddFrameAction(cUrl_Thread_Finish, this);
		waiting = true;
		event->Wait();
		waiting = false;
	}
	delete this;
}

