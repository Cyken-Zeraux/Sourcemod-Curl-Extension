#ifndef _INCLUDE_SOURCEMOD_EXTENSION_DEFINE_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_DEFINE_H_

#include <sh_list.h>
#include <curl/curl.h>

#ifdef PLATFORM_LINUX
#define INVALID_SOCKET	-1
#endif

class cURLThread;
struct cURLHandle;


enum SendRecv_Act {
	SendRecv_Act_NOTHING = 0,

	SendRecv_Act_GOTO_SEND,
	SendRecv_Act_GOTO_RECV,
	SendRecv_Act_GOTO_WAIT,
	SendRecv_Act_GOTO_END,

	SendRecv_Act_LAST,
};

enum cURLThread_Type {
	cURLThread_Type_NOTHING = 0,

	cURLThread_Type_PERFORM,
	cURLThread_Type_SEND_RECV,

	cURLThread_Type_LAST,
};

enum cURLThread_Func {
	cURLThread_Func_NOTHING = 0,

	cURLThread_Func_COMPLETE,
	cURLThread_Func_SEND,
	cURLThread_Func_RECV,

	cURLThread_Func_LAST,
};

struct cURLOpt_string {
	CURLoption opt;
	char *value;
};

struct cURLOpt_int {
	CURLoption opt;
	int value;
};

struct cURLOpt_int64 {
	CURLoption opt;
	curl_off_t value;
};

struct cURLOpt_pointer {
	CURLoption opt;
	void *value;
};

struct cURLHandle {
	cURLHandle():curl(NULL),running(false),lasterror(CURLE_OK),opt_loaded(false),
		thread(NULL),sockextr(INVALID_SOCKET),send_timeout(60000),recv_timeout(60000),send_buffer(NULL)
	{
	}
	CURL *curl;
	char errorBuffer[CURL_ERROR_SIZE];
	SourceHook::List<cURLOpt_string *> opt_string_list;
	SourceHook::List<cURLOpt_int *> opt_int_list;
	SourceHook::List<cURLOpt_int64 *> opt_int64_list;
	SourceHook::List<cURLOpt_pointer *> opt_pointer_list;
	bool running;
	CURLcode lasterror;
	bool opt_loaded;
	IPluginFunction *callback_Function[cURLThread_Func_LAST];
	Handle_t hndl;
	int UserData[2];
	cURLThread *thread;

	/* use for send & recv */
	long sockextr;
	long send_timeout;
	long recv_timeout;
	unsigned char *send_buffer;
	unsigned int send_buffer_length;
};

class ICloseHelper {
public:
	ICloseHelper():_handle(NULL),_marked_delete(false)
	{
	}
	cURLHandle *_handle;
	bool _marked_delete;
	bool TryDelete();
	virtual void Delete() =0;
};

class cURL_slist_pack : public ICloseHelper {
public:
	cURL_slist_pack():chunk(NULL)
	{
	}
	curl_slist *chunk;
	void Delete();
};

class WebForm : public ICloseHelper {
public:
	WebForm():first(NULL), last(NULL)
	{
	}
	curl_httppost *first;
	curl_httppost *last;
	SourceHook::List<cURL_slist_pack *> slist_record;
	void Delete();
};

class cURL_OpenFile : public ICloseHelper {
public:
	FILE *pFile;
	void Delete();
};


enum OpensslThread_Type {
	OpensslThread_Type_NOTHING = 0,

	OpensslThread_Type_HASH_FILE,

	OpensslThread_Type_LAST,
};

enum Openssl_Hash {
	Openssl_Hash_MD5 = 0,
	Openssl_Hash_MD4,
	Openssl_Hash_MD2,
	Openssl_Hash_SHA,
	Openssl_Hash_SHA1,
	Openssl_Hash_SHA224,
	Openssl_Hash_SHA256,
	Openssl_Hash_SHA384,
	Openssl_Hash_SHA512,
	Openssl_Hash_RIPEMD160,
};

struct Openssl_Hash_pack {
	Openssl_Hash_pack():path(NULL), output(NULL)
	{
	}
	IPluginFunction *hash_callback;
	int UserData;
	char *path;
	Openssl_Hash algorithm;
	bool success;
	char *output;
};

#endif
