/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "curlmanager.h"
#include <curl/curl.h>


cURL_SM g_cURL_SM;

SMEXT_LINK(&g_cURL_SM);

extern sp_nativeinfo_t g_cURLNatives[];

HandleType_t g_cURLHandle = 0;
HandleType_t g_cURLFile = 0;
HandleType_t g_WebForm = 0;
HandleType_t g_cURLSlist = 0;

IdentityToken_t *myself_Identity = NULL;

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
  }
  else
  {
    FD_SET(sockfd, &outfd);
  }
 
  /* select() returns the number of signalled sockets or -1 */ 
  res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
  return res;
}


void Testt()
{
	CURL *curl;
	CURLcode res;
	/* Minimalistic http request */ 
	const char *request = "GET / HTTP/1.0\r\nHost: www.google.com\r\n\r\n";
	curl_socket_t sockfd; /* socket */ 
	long sockextr;
	size_t iolen;

	curl = curl_easy_init();

	char errorBuffer[256];
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
	curl_easy_setopt(curl, CURLOPT_URL, "http://www.google.com");
    /* Do not do the transfer - only connect to host */ 
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
	 curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
    res = curl_easy_perform(curl);

	res = curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &sockextr);

	sockfd = sockextr;

	if(!wait_on_socket(sockfd, 0, 60000L))
    {
      printf("Error: timeout.\n");
      return;
    }

	puts("Sending request.");
    /* Send the request. Real applications should check the iolen
     * to see if all the request has been sent */ 
    res = curl_easy_send(curl, request, strlen(request), &iolen);

	if(CURLE_OK != res)
    {
      printf("Error: %s\n", curl_easy_strerror(res));
      return;
    }
    puts("Reading response.");
 
    /* read the response */ 
    for(;;)
    {
      char buf[128];
 
      wait_on_socket(sockfd, 1, 60000L);
      res = curl_easy_recv(curl, buf, 128, &iolen);
	//	res = curl_easy_recv(curl, buf, 1024, &iolen);
 
      if(CURLE_OK != res)
        break;
 
      printf("Received %u bytes.\n", iolen);
    }
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
}

bool cURL_SM::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	CURLcode code;
	code = curl_global_init(CURL_GLOBAL_ALL);

	if(code)
	{
		smutils->Format(error, maxlength, "%s", curl_easy_strerror(code));
		return false;
	}

	myself_Identity = myself->GetIdentity();

	bool valid = true;

	HandleError err_file, err_handle, err_webform, err_slist;
	g_cURLFile = handlesys->CreateType("cURLFile", this, 0, NULL, NULL, myself_Identity, &err_file);
	g_cURLHandle = handlesys->CreateType("cURLHandle", this, 0, NULL, NULL, myself_Identity, &err_handle);
	g_WebForm = handlesys->CreateType("cURLWebForm", this, 0, NULL, NULL, myself_Identity, &err_webform);
	g_cURLSlist = handlesys->CreateType("cURLSlist", this, 0, NULL, NULL, myself_Identity, &err_slist);
	
	if(g_cURLFile == 0)
	{
		handlesys->RemoveType(g_cURLFile, myself_Identity);
		g_cURLFile = 0;
		snprintf(error, maxlength, "Could not create CURL file type (err: %d)", err_file);
		valid = false;
	}

	if(g_cURLHandle == 0)
	{
		handlesys->RemoveType(g_cURLHandle, myself_Identity);
		g_cURLHandle = 0;
		snprintf(error, maxlength, "Could not create CURL handle type (err: %d)", err_handle);
		valid = false;
	}
	
	if(g_WebForm == 0)
	{
		handlesys->RemoveType(g_WebForm, myself_Identity);
		g_WebForm = 0;
		snprintf(error, maxlength, "Could not create CURL WebForm type (err: %d)", err_webform);
		valid = false;
	}

	if(g_cURLSlist == 0)
	{
		handlesys->RemoveType(g_cURLSlist, myself_Identity);
		g_cURLSlist = 0;
		snprintf(error, maxlength, "Could not create CURL Slist type (err: %d)", err_slist);
		valid = false;
	}

	if(!valid)
		return false;

	sharesys->AddNatives(myself, g_cURLNatives);

	g_cURLManager.SDK_OnLoad();

	curl_version_info_data *vinfo = curl_version_info(CURLVERSION_NOW);
	
	int ff = vinfo->features;
	if(ff & CURL_VERSION_TLSAUTH_SRP)
		int ggg = 1;

	const char * const *proto;

	for(proto=vinfo->protocols; *proto; ++proto) {
          printf("%s ", *proto);
        }
	printf("\n");

	struct feat {
          const char *name;
          int bitmask;
        };

	static const struct feat feats[] = {
          {"AsynchDNS", CURL_VERSION_ASYNCHDNS},
          {"Debug", CURL_VERSION_DEBUG},
          {"TrackMemory", CURL_VERSION_CURLDEBUG},
          {"GSS-Negotiate", CURL_VERSION_GSSNEGOTIATE},
          {"IDN", CURL_VERSION_IDN},
          {"IPv6", CURL_VERSION_IPV6},
          {"Largefile", CURL_VERSION_LARGEFILE},
          {"NTLM", CURL_VERSION_NTLM},
          {"SPNEGO", CURL_VERSION_SPNEGO},
          {"SSL",  CURL_VERSION_SSL},
          {"SSPI",  CURL_VERSION_SSPI},
          {"krb4", CURL_VERSION_KERBEROS4},
          {"libz", CURL_VERSION_LIBZ},
          {"CharConv", CURL_VERSION_CONV},
          {"TLS-SRP", CURL_VERSION_TLSAUTH_SRP}
        };
        printf("Features: ");
        for(int i=0; i<sizeof(feats)/sizeof(feats[0]); i++) {
          if(vinfo->features & feats[i].bitmask)
            printf("%s ", feats[i].name);
        }
	printf("\n");


	//Testt();

	return true;
}

void cURL_SM::SDK_OnUnload()
{
	g_cURLManager.SDK_OnUnload();

	curl_global_cleanup();
}

void cURL_SM::SDK_OnAllLoaded()
{
}

bool cURL_SM::QueryRunning(char *error, size_t maxlength)
{
	return true;
}

void cURL_SM::OnHandleDestroy(HandleType_t type, void *object)
{
	if(type == g_cURLHandle)
	{
		g_cURLManager.RemovecURLHandle((cURLHandle *)object);
	} else if(type == g_cURLFile) {
		FILE *fp = (FILE *)object;
		fclose(fp);
	} else if(type == g_WebForm) {
		WebForm *httpost = (WebForm *)object;
		curl_formfree(httpost->first);
		delete httpost;
	} else if(type == g_cURLSlist) {
		cURL_slist_pack *slist = (cURL_slist_pack *)object;
		curl_slist_free_all(slist->chunk); 
		delete slist;
	}
}
