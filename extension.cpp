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

	curl_version_info_data *vinfo = curl_version_info(CURLVERSION_NOW);
	
	int ff = vinfo->features;
	if(ff & CURL_VERSION_TLSAUTH_SRP)
		int ggg = 1;

	return true;
}

void cURL_SM::SDK_OnUnload()
{
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
