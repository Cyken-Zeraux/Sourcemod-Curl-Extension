
#include "curlmanager.h"

cURLManager g_cURLManager;

static size_t curl_write_function(void *ptr, size_t bytes, size_t nmemb, void *stream)
{
	FILE *file = (FILE *)stream;
	if(file->_file == 3 || file->_file == 4)
	{
		return fwrite(ptr, bytes, nmemb, file); 
	}
	return bytes * nmemb;
}

void cURLManager::SDK_OnLoad()
{
	curlhandle_list_mutex = threader->MakeMutex();
	shutdown = false;
}

void cURLManager::SDK_OnUnload()
{
	if(g_cURLThread_List.size() > 0)
	{
		int gggg = 0;
	}



}

void cURLManager::MakecURLThread(cURLThread *thread)
{
	if(shutdown)
	{
		delete thread;
		return;
	}
	curlhandle_list_mutex->Lock();
	g_cURLThread_List.push_back(thread);
	curlhandle_list_mutex->Unlock();

	threader->MakeThread(thread);
}

void cURLManager::RemovecURLThread(cURLThread *thread)
{
	curlhandle_list_mutex->Lock();
	g_cURLThread_List.remove(thread);
	curlhandle_list_mutex->Unlock();
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
		case CURLOPT_USERPWD:
		case CURLOPT_KEYPASSWD:
		case CURLOPT_POSTFIELDS:
		case CURLOPT_REFERER:
		case CURLOPT_FTPPORT:
		case CURLOPT_USERAGENT:
		case CURLOPT_COOKIE:
		case CURLOPT_ENCODING:
		case CURLOPT_CUSTOMREQUEST:
		case CURLOPT_WRITEINFO:
		case CURLOPT_INTERFACE:
		case CURLOPT_KRBLEVEL:
		case CURLOPT_SSL_CIPHER_LIST:
		case CURLOPT_SSLCERTTYPE:
		case CURLOPT_SSLKEYTYPE:
		case CURLOPT_SSLENGINE:
		case CURLOPT_FTP_ACCOUNT:
		case CURLOPT_COOKIELIST:
		case CURLOPT_FTP_ALTERNATIVE_TO_USER:
		case CURLOPT_SSH_HOST_PUBLIC_KEY_MD5:
		case CURLOPT_USERNAME:
		case CURLOPT_PASSWORD:
		case CURLOPT_PROXYUSERNAME:
		case CURLOPT_PROXYPASSWORD:
		case CURLOPT_NOPROXY:
		case CURLOPT_SOCKS5_GSSAPI_SERVICE:
		case CURLOPT_MAIL_FROM:
		case CURLOPT_RTSP_SESSION_ID:
		case CURLOPT_RTSP_STREAM_URI:
		case CURLOPT_RTSP_TRANSPORT:
		case CURLOPT_TLSAUTH_USERNAME:
		case CURLOPT_TLSAUTH_PASSWORD:
		case CURLOPT_TLSAUTH_TYPE:
			supported = true;
			break;
		case CURLOPT_COOKIEFILE:
		case CURLOPT_COOKIEJAR:
		case CURLOPT_RANDOM_FILE:
		case CURLOPT_EGDSOCKET:
		case CURLOPT_SSLCERT:
		case CURLOPT_SSLKEY:
		case CURLOPT_CAINFO:
		case CURLOPT_CAPATH:
		case CURLOPT_NETRC_FILE:
		case CURLOPT_SSH_PUBLIC_KEYFILE:
		case CURLOPT_SSH_PRIVATE_KEYFILE:
		case CURLOPT_CRLFILE:
		case CURLOPT_ISSUERCERT:
		case CURLOPT_SSH_KNOWNHOSTS:
		{
			char realpath[PLATFORM_MAX_PATH];
			g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", value);
			value = realpath;
			supported = true;
			break;
		}
	}

	assert((supported != false));
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
		case CURLOPT_UPLOAD:
		case CURLOPT_INFILESIZE:
		case CURLOPT_LOW_SPEED_LIMIT:
		case CURLOPT_LOW_SPEED_TIME:
		case CURLOPT_RESUME_FROM:
		case CURLOPT_CRLF:
		case CURLOPT_SSLVERSION:
		case CURLOPT_TIMECONDITION:
		case CURLOPT_TIMEVALUE:
		case CURLOPT_HEADER:
		case CURLOPT_NOBODY:
		case CURLOPT_FAILONERROR:
		case CURLOPT_POST:
		case CURLOPT_DIRLISTONLY:
		case CURLOPT_APPEND:
		case CURLOPT_NETRC:
		case CURLOPT_FOLLOWLOCATION:
		case CURLOPT_TRANSFERTEXT:
		case CURLOPT_PUT:
		case CURLOPT_AUTOREFERER:
		case CURLOPT_PROXYPORT:
		case CURLOPT_POSTFIELDSIZE:
		case CURLOPT_MAXREDIRS:
		case CURLOPT_FILETIME:
		case CURLOPT_MAXCONNECTS:
		case CURLOPT_CLOSEPOLICY:
		case CURLOPT_FRESH_CONNECT:
		case CURLOPT_FORBID_REUSE:
		case CURLOPT_CONNECTTIMEOUT:
		case CURLOPT_HTTPGET:
		case CURLOPT_HTTP_VERSION:
		case CURLOPT_FTP_USE_EPSV:
		case CURLOPT_SSLENGINE_DEFAULT:
		case CURLOPT_DNS_USE_GLOBAL_CACHE:
		case CURLOPT_DNS_CACHE_TIMEOUT:
		case CURLOPT_COOKIESESSION:
		case CURLOPT_BUFFERSIZE:
		case CURLOPT_NOSIGNAL:
		case CURLOPT_UNRESTRICTED_AUTH:
		case CURLOPT_FTP_USE_EPRT:
		case CURLOPT_HTTPAUTH:
		case CURLOPT_FTP_CREATE_MISSING_DIRS:
		case CURLOPT_PROXYAUTH:
		case CURLOPT_FTP_RESPONSE_TIMEOUT:
		case CURLOPT_IPRESOLVE:
		case CURLOPT_MAXFILESIZE:
		case CURLOPT_USE_SSL:
		case CURLOPT_TCP_NODELAY:
		case CURLOPT_FTPSSLAUTH:
		case CURLOPT_IGNORE_CONTENT_LENGTH:
		case CURLOPT_FTP_SKIP_PASV_IP:
		case CURLOPT_FTP_FILEMETHOD:
		case CURLOPT_LOCALPORT:
		case CURLOPT_LOCALPORTRANGE:
		case CURLOPT_CONNECT_ONLY:
		case CURLOPT_SSL_SESSIONID_CACHE:
		case CURLOPT_SSH_AUTH_TYPES:
		case CURLOPT_FTP_SSL_CCC:
		case CURLOPT_TIMEOUT_MS:
		case CURLOPT_CONNECTTIMEOUT_MS:
		case CURLOPT_HTTP_TRANSFER_DECODING:
		case CURLOPT_HTTP_CONTENT_DECODING:
		case CURLOPT_NEW_FILE_PERMS:
		case CURLOPT_NEW_DIRECTORY_PERMS:
		case CURLOPT_POSTREDIR:
		case CURLOPT_PROXY_TRANSFER_MODE:
		case CURLOPT_ADDRESS_SCOPE:
		case CURLOPT_CERTINFO:
		case CURLOPT_TFTP_BLKSIZE:
		case CURLOPT_PROTOCOLS:
		case CURLOPT_REDIR_PROTOCOLS:
		case CURLOPT_FTP_USE_PRET:
		case CURLOPT_RTSP_REQUEST:
		case CURLOPT_RTSP_CLIENT_CSEQ:
		case CURLOPT_RTSP_SERVER_CSEQ:
		case CURLOPT_WILDCARDMATCH:
			supported = true;
			break;
	}

	assert((supported != false));
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
		case CURLOPT_INFILESIZE_LARGE:
		case CURLOPT_RESUME_FROM_LARGE:
		case CURLOPT_MAXFILESIZE_LARGE:
		case CURLOPT_POSTFIELDSIZE_LARGE:
		case CURLOPT_MAX_SEND_SPEED_LARGE:
		case CURLOPT_MAX_RECV_SPEED_LARGE:
			supported = true;
			break;
	}

	assert((supported != false));
	if(!supported)
		return false;

	cURLOpt_int64 *int64opt = new cURLOpt_int64();
	int64opt->opt = opt;
	int64opt->value = (curl_off_t)value;

	handle->opt_int64_list.push_back(int64opt);
	return true;
}


bool cURLManager::AddcURLOptionHandle(IPluginContext *pContext, cURLHandle *handle, HandleSecurity *sec, CURLoption opt, Handle_t hndl)
{
	if(!handle || handle->running)
		return false;

	void *pointer = NULL;
	int err = SP_ERROR_NONE;
	switch(opt)
	{
		case CURLOPT_WRITEDATA:
		case CURLOPT_HEADERDATA:
		case CURLOPT_READDATA:
		case CURLOPT_STDERR:
		case CURLOPT_INTERLEAVEDATA:
		{
			FILE *pFile = NULL;
			err = handlesys->ReadHandle(hndl, g_cURLFile, sec, (void **)&pFile);
			pointer = pFile;
			break;
		}
		case CURLOPT_HTTPPOST:
		{
			WebForm *webform = NULL;
			err = handlesys->ReadHandle(hndl, g_WebForm, sec, (void **)&webform);
			if(webform != NULL)
				pointer = webform->first;
			break;
		}
		case CURLOPT_HTTPHEADER:
		case CURLOPT_QUOTE:
		case CURLOPT_POSTQUOTE:
		case CURLOPT_TELNETOPTIONS:
		case CURLOPT_PREQUOTE:
		case CURLOPT_HTTP200ALIASES:
		case CURLOPT_MAIL_RCPT:
		case CURLOPT_RESOLVE:
		{
			cURL_slist_pack *data = NULL;
			err = handlesys->ReadHandle(hndl, g_cURLSlist, sec, (void **)&data);
			if(data != NULL)
				pointer = data->chunk;
			break;
		}
	}

	if(err != SP_ERROR_NONE)
	{
		pContext->ThrowNativeErrorEx(err, NULL);
		return false;
	}

	assert((pointer != NULL));
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
	curl_easy_setopt(handle->curl, CURLOPT_WRITEFUNCTION, curl_write_function);

	//curl_easy_setopt(handle->curl, CURLOPT_READFUNCTION, curl_read_function);

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

CURLFORMcode cURLManager::cURLFormAdd(IPluginContext *pContext, const cell_t *params, WebForm *handle)
{
	if(handle == NULL)
		return CURL_FORMADD_INCOMPLETE;

	unsigned int numparams = (unsigned)params[0];
	unsigned int startparam = 2;	
	if(numparams <= 1)
		return CURL_FORMADD_INCOMPLETE;

	CURLformoption form_opts[11] = {CURLFORM_NOTHING};

	char *form_data[10];
	memset(form_data, NULL, sizeof(form_data));

	cell_t *addr;
	int count = 0;
	int err;
	int value;
	for(unsigned int i=startparam;i<=numparams;i++)
	{
		if((err=pContext->LocalToPhysAddr(params[i], &addr)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeErrorEx(err, NULL);
			return CURL_FORMADD_INCOMPLETE;
		}
		CURLformoption form_code = (CURLformoption)*addr;
		switch(form_code)
		{
			case CURLFORM_COPYNAME:
			case CURLFORM_COPYCONTENTS:
			case CURLFORM_FILECONTENT:
			case CURLFORM_FILE:
			case CURLFORM_CONTENTTYPE:
			case CURLFORM_FILENAME:				
				if((err=pContext->LocalToString(params[i+1], &form_data[count])) != SP_ERROR_NONE)
				{
					pContext->ThrowNativeErrorEx(err, NULL);
					return CURL_FORMADD_INCOMPLETE;
				}
				if(form_code == CURLFORM_FILE || form_code == CURLFORM_FILECONTENT) // absolute path
				{
					char realpath[PLATFORM_MAX_PATH];
					g_pSM->BuildPath(Path_Game, realpath, sizeof(realpath), "%s", form_data[count]);
				
					form_data[count] = realpath;
				}
				form_opts[count] = form_code;
				count++;
				i++;
				break;
			case CURLFORM_NAMELENGTH:
			case CURLFORM_CONTENTSLENGTH:				
				if((err=pContext->LocalToPhysAddr(params[i+1], &addr)) != SP_ERROR_NONE)
				{
					pContext->ThrowNativeErrorEx(err, NULL);
					return CURL_FORMADD_INCOMPLETE;
				}
				form_opts[count] = form_code;
				value = *addr;
				form_data[count] = (char *)value;
				count++;
				i++;
				break;
			case CURLFORM_CONTENTHEADER:
			{
				if((err=pContext->LocalToPhysAddr(params[i+1], &addr)) != SP_ERROR_NONE)
				{
					pContext->ThrowNativeErrorEx(err, NULL);
					return CURL_FORMADD_INCOMPLETE;
				}				
				cURL_slist_pack *slist;
				HandleError hndl_err;
				HandleSecurity sec(pContext->GetIdentity(), myself_Identity);
				if((hndl_err = handlesys->ReadHandle(*addr, g_cURLSlist, &sec, (void **)&slist)) != HandleError_None)
				{
					pContext->ThrowNativeError("Invalid curl_slist Handle %x (error %d)", params[1], hndl_err);
					return CURL_FORMADD_INCOMPLETE;
				}
				form_opts[count] = form_code;
				form_data[count] = (char *)slist->chunk;
				count++;
				i++;
				break;
			}
			case CURLFORM_END:
				form_opts[count] = CURLFORM_END;
				break;
		}
	}
	CURLFORMcode ret = curl_formadd(&handle->first, &handle->last,
		form_opts[0],
		form_data[0],
		form_opts[1],
		form_data[1],
		form_opts[2],
		form_data[2],
		form_opts[3],
		form_data[3],
		form_opts[4],
		form_data[4],
		form_opts[5],
		form_data[5],
		form_opts[6],
		form_data[6],
		form_opts[7],
		form_data[7],
		form_opts[8],
		form_data[8],
		form_opts[9],
		form_data[9],
		form_opts[10]
		);
	return ret;
}
