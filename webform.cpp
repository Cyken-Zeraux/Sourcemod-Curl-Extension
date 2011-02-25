
#include "webform.h"

WebForm::WebForm() : first(NULL), last(NULL), lastError(CURL_FORMADD_OK)
{
}

WebForm::~WebForm()
{
	curl_formfree(first);
}

bool WebForm::AddString(const char *name, const char *data, CURLformoption opt)
{
	lastError = curl_formadd(&first,
		&last,
		CURLFORM_COPYNAME,
		name,
		opt,
		data,
		CURLFORM_END);

	return (lastError == CURL_FORMADD_OK);
}

curl_httppost *WebForm::GetFormData()
{
	return first;
}

