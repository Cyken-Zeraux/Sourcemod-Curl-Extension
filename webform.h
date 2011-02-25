#ifndef _INCLUDE_SOURCEMOD_EXTENSION_WEBFORM_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_WEBFORM_H_

#include "extension.h"
#include <curl/curl.h>

enum CURL_WEBFORM {
	CURL_WEBFORM_CONTENTS = 0,
	CURL_WEBFORM_FILE = 1,
};


class WebForm
{
public:
	WebForm();
	~WebForm();

public:
	bool AddString(const char *name, const char *data, CURLformoption opt);

public:
	curl_httppost *GetFormData();

private:
	curl_httppost *first;
	curl_httppost *last;
	CURLFORMcode lastError;
};


#endif

