
#include "curlmanager.h"
#include "webform.h"

cURLManager g_cURLManager;

static size_t curl_dump_write_to_socket(void *ptr, size_t bytes, size_t nmemb, void *stream)
{
	//return fwrite(ptr, bytes, nmemb, (FILE *)stream); 
	FILE *file = (FILE *)stream;
	if(file->_file == 3 || file->_file == 4)
	{
		return fwrite(ptr, bytes, nmemb, file); 
	}
	return bytes * nmemb;
}

void cURLManager::RemovecURLHandle(cURLHandle *handle)
{
	if(!handle)
		return;
	
	curl_easy_cleanup(handle->curl);

	SourceHook::List<cURLOpt_string *>::iterator iter;
	cURLOpt_string *pInfo;
	for (iter=handle->opt_string_list.begin(); iter!=handle->opt_string_list.end(); iter++)
	{
		pInfo = (*iter);
		delete pInfo->value;
		delete pInfo;
	}
	handle->opt_string_list.clear();

	SourceHook::List<cURLOpt_int *>::iterator iter2;
	cURLOpt_int*pInfo2;
	for (iter2=handle->opt_int_list.begin(); iter2!=handle->opt_int_list.end(); iter2++)
	{
		pInfo2 = (*iter2);
		delete pInfo2;
	}
	handle->opt_int_list.clear();	

	SourceHook::List<cURLOpt_pointer *>::iterator iter3;
	cURLOpt_pointer *pInfo3;
	for(iter3=handle->opt_pointer_list.begin(); iter3!=handle->opt_pointer_list.end(); iter3++)
	{
		pInfo3 = (*iter3);
		delete pInfo3;
	}
	handle->opt_pointer_list.clear();

	SourceHook::List<cURLOpt_int64 *>::iterator iter4;
	cURLOpt_int64 *pInfo4;
	for(iter4=handle->opt_int64_list.begin(); iter4!=handle->opt_int64_list.end(); iter4++)
	{
		pInfo4 = (*iter4);
		delete pInfo4;
	}
	handle->opt_int64_list.clear();

	delete handle;
}

bool cURLManager::AddcURLOptionString(cURLHandle *handle, CURLoption opt, char *value)
{
	if(!handle || handle->running || !value)
		return false;

	bool supported = false;
	switch(opt)
	{
		case CURLOPT_URL:
		case CURLOPT_PROXY:
		case CURLOPT_PROXYUSERPWD:
		case CURLOPT_RANGE:
			supported = true;
			break;
	}

	if(!supported)
		return false;

	cURLOpt_string *stringopt = new cURLOpt_string();
	stringopt->opt = opt;
	stringopt->value = new char[strlen(value)+1];
	memcpy(stringopt->value,value, strlen(value)+1);

	handle->opt_string_list.push_back(stringopt);

	return true;
}

bool cURLManager::AddcURLOptionInt(cURLHandle *handle, CURLoption opt, int value)
{
	if(!handle || handle->running)
		return false;

	bool supported = false;
	switch(opt)
	{
		case CURLOPT_PORT:
		case CURLOPT_NOPROGRESS:
		case CURLOPT_VERBOSE:
		case CURLOPT_PROXYTYPE:
		case CURLOPT_HTTPPROXYTUNNEL:
		case CURLOPT_TIMEOUT:
		case CURLOPT_SSL_VERIFYPEER:
		case CURLOPT_SSL_VERIFYHOST:
			supported = true;
			break;
	}

	if(!supported)
		return false;

	cURLOpt_int *intopt = new cURLOpt_int();
	intopt->opt = opt;
	intopt->value = value;

	handle->opt_int_list.push_back(intopt);

	return true;
}

bool cURLManager::AddcURLOptionInt64(cURLHandle *handle, CURLoption opt, long long value)
{
	if(!handle || handle->running)
		return false;

	bool supported = false;
	switch(opt)
	{
		case CURLOPT_PORT:
			supported = true;
			break;
	}

	if(!supported)
		return false;

	cURLOpt_int64 *int64opt = new cURLOpt_int64();
	int64opt->opt = opt;
	int64opt->value = (curl_off_t)value;

	handle->opt_int64_list.push_back(int64opt);
	return true;
}


bool cURLManager::AddcURLOptionHandle(cURLHandle *handle, HandleSecurity *sec, CURLoption opt, Handle_t hndl)
{
	if(!handle || handle->running)
		return false;

	void *pointer = NULL;
	switch(opt)
	{
		case CURLOPT_WRITEDATA:
		case CURLOPT_HEADERDATA:
		case CURLOPT_READDATA:
		{
			FILE *pFile = NULL;
			handlesys->ReadHandle(hndl, g_cURLFile, sec, (void **)&pFile);
			pointer = pFile;
			break;
		}
		case CURLOPT_HTTPPOST:
		{
			WebForm *webform = NULL;
			handlesys->ReadHandle(hndl, g_WebForm, sec, (void **)&webform);
			if(webform != NULL)
			{
				pointer = webform->GetFormData();
			}
			break;
		}
	}
	
	if(pointer == NULL)
		return false;
	
	cURLOpt_pointer *pointeropt = new cURLOpt_pointer();
	pointeropt->opt = opt;
	pointeropt->value = pointer;

	handle->opt_pointer_list.push_back(pointeropt);
	return true;
}

void cURLManager::LoadcURLOption(cURLHandle *handle)
{
	if(!handle || handle->opt_loaded)
		return;

	handle->opt_loaded = true;
	
	curl_easy_setopt(handle->curl, CURLOPT_ERRORBUFFER, handle->errorBuffer);
	curl_easy_setopt(handle->curl, CURLOPT_WRITEFUNCTION, curl_dump_write_to_socket);

	//curl_easy_setopt(handle->curl, CURLOPT_WRITEDATA, handle);

	SourceHook::List<cURLOpt_string *>::iterator iter;
	cURLOpt_string *pInfo;
	for(iter=handle->opt_string_list.begin(); iter!=handle->opt_string_list.end(); iter++)
	{
		pInfo = (*iter);
		if((handle->lasterror = curl_easy_setopt(handle->curl, pInfo->opt, pInfo->value)) != CURLE_OK)
			return;
	}

	SourceHook::List<cURLOpt_int *>::iterator iter2;
	cURLOpt_int *pInfo2;
	for(iter2=handle->opt_int_list.begin(); iter2!=handle->opt_int_list.end(); iter2++)
	{
		pInfo2 = (*iter2);
		if((handle->lasterror = curl_easy_setopt(handle->curl, pInfo2->opt, pInfo2->value)) != CURLE_OK)
			return;
	}

	SourceHook::List<cURLOpt_pointer *>::iterator iter3;
	cURLOpt_pointer *pInfo3;
	for(iter3=handle->opt_pointer_list.begin(); iter3!=handle->opt_pointer_list.end(); iter3++)
	{
		pInfo3 = (*iter3);
		if((handle->lasterror = curl_easy_setopt(handle->curl, pInfo3->opt, pInfo3->value)) != CURLE_OK)
			return;
	}

	SourceHook::List<cURLOpt_int64 *>::iterator iter4;
	cURLOpt_int64 *pInfo4;
	for(iter4=handle->opt_int64_list.begin(); iter4!=handle->opt_int64_list.end(); iter4++)
	{
		pInfo4 = (*iter4);
		if((handle->lasterror = curl_easy_setopt(handle->curl, pInfo4->opt, (curl_off_t)pInfo4->value)) != CURLE_OK)
			return;
	}
}


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

