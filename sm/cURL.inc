
#if defined _cURL_included
  #endinput
#endif
#define _cURL_included

#include <cUrl_header>


/*
Unsupport curl options:
ERRORBUFFER              // use curl_get_error_buffer
WRITEFUNCTION            // used
READFUNCTION             // unuse
WRITEINFO                // ???
PROGRESSFUNCTION         // unuse
PROGRESSDATA             // same
HEADERFUNCTION           // unuse
DEBUGFUNCTION            // unuse
DEBUGDATA                // same
SHARE                    // unsupport
PRIVATE                  // unsupport
SSL_CTX_FUNC             // unuse
SSL_CTX_DATA             // same
IOCTLFUNCTION            // unuse
IOCTLDATA                // same
CONV_FROM_NETWORK_FUNC   // unuse
CONV_TO_NETWORK_FUNC     // unuse
CONV_FROM_UTF8_FUNC      // unuse
SOCKOPTFUNCTION          // unuse
SOCKOPTDATA              // unuse
OPENSOCKETFUNCTION       // unuse
OPENSOCKETDATA           // unuse
COPYPOSTFIELDS           // no support
SEEKFUNCTION             // unuse
SEEKDATA                 // unuse
SOCKS5_GSSAPI_SERVICE    // no support
SOCKS5_GSSAPI_NEC        // no support
SSH_KEYFUNCTION          // use default
SSH_KEYDATA              // no support
INTERLEAVEFUNCTION       // no support
CHUNK_BGN_FUNC           // no support
CHUNK_END_FUNC           // no support
FNMATCH_FUNC             // no support
CHUNK_DATA               // no support
FNMATCH_DATA             // no support
*/

/*
These cURL options will use BuildPath, file://c:/cookies.txt
CURLOPT_COOKIEFILE
CURLOPT_COOKIEJAR
CURLOPT_RANDOM_FILE
CURLOPT_EGDSOCKET
CURLOPT_SSLKEY
CURLOPT_CAPATH
CURLOPT_NETRC_FILE
CURLOPT_SSH_PUBLIC_KEYFILE
CURLOPT_SSH_PRIVATE_KEYFILE
CURLOPT_CRLFILE
CURLOPT_ISSUERCERT
CURLOPT_SSH_KNOWNHOSTS
*/

/*
Unsupport curl info
CURLINFO_SLIST

*/


funcenum CURL_OnComplete
{
	public(Handle:hndl, CURLcode: code),
	public(Handle:hndl, CURLcode: code , any:data),
};

enum SendRecv_Act {
	SendRecv_Act_NOTHING = 0,

	SendRecv_Act_GOTO_SEND,
	SendRecv_Act_GOTO_RECV,
	SendRecv_Act_GOTO_WAIT,
	SendRecv_Act_GOTO_END,

	SendRecv_Act_LAST,
};

funcenum CURL_OnSend
{
	SendRecv_Act:public(Handle:hndl, CURLcode: code, const last_sent_dataSize),
	SendRecv_Act:public(Handle:hndl, CURLcode: code, const last_sent_dataSize, any:data),
}

funcenum CURL_OnReceive
{
	SendRecv_Act:public(Handle:hndl, CURLcode: code, const String:receiveData[], const dataSize),
	SendRecv_Act:public(Handle:hndl, CURLcode: code, const String:receiveData[], const dataSize, any:data),
}


native Handle:curl_easy_init();


native bool:curl_easy_setopt_string(Handle:hndl, CURLoption:opt, const any:buffter[]);
native bool:curl_easy_setopt_int(Handle:hndl, CURLoption:opt, value);
native bool:curl_easy_setopt_int_array(Handle:hndl, array[][2], array_size);
native bool:curl_easy_setopt_int64(Handle:hndl, CURLoption:opt, const any:buffter[]);
native bool:curl_easy_setopt_handle(Handle:hndl, CURLoption:opt, Handle:other_hndl);


native CURLcode:curl_load_opt(Handle:hndl);


native CURLcode:curl_easy_perform(Handle:hndl);
native curl_easy_perform_thread(Handle:hndl, CURL_OnComplete:perform_callback=CURL_Default_Perform_CallBack, any:data=0);

native curl_easy_send_recv(Handle:hndl, CURL_OnSend:send_callback, CURL_OnReceive:receive_callback, CURL_OnComplete:complete_callback, SendRecv_Act:act, timeout, recv_buffer_Size = 512, any:data=0);
native bool:curl_send_recv_Signal(Handle:hndl, SendRecv_Act:act);
native bool:curl_send_recv_IsWaiting(Handle:hndl);
native curl_set_send_buffer(Handle:hndl, const any:buffer[], size=0);


native curl_get_error_buffer(Handle:hndl, String:buffer[], maxlen);


native CURLcode:curl_easy_getinfo_string(Handle:hndl, CURLINFO:info, String:buffer[], maxlen);
native CURLcode:curl_easy_getinfo_int(Handle:hndl, CURLINFO:info, &any:data);


native bool:curl_easy_escape(Handle:hndl, const String:url[], String:buffer[], maxlen);
// return output length
native curl_easy_unescape(Handle:hndl, const String:url[], String:buffer[], maxlen);


native curl_easy_strerror(CURLcode: code, String:buffer[], maxlen);
native curl_version(String:buffer[], maxlen);
native curl_protocols(String:buffer[], maxlen);
native curl_features();


/*
curl_OpenFile
curl_httppost
curl_slist
should not shared to another curl handle, & these handle will free when curl handle close
*/

// Can't use sourcemod OpenFile..., for CURLOPT_WRITEDATA, CURLOPT_HEADERDATA, CURLOPT_READDATA, CURLOPT_STDERR, CURLOPT_INTERLEAVEDATA
native Handle:curl_OpenFile(const String:file[], const String:mode[]);


// struct curl_httppost, for CURLOPT_HTTPPOST
native Handle:curl_httppost();
native bool:curl_formadd(Handle:handl, any:...);


// struct curl_slist, for CURLOPT_QUOTE, CURLOPT_HTTPHEADER, CURLOPT_POSTQUOTE, CURLOPT_TELNETOPTIONS, CURLOPT_PREQUOTE, CURLOPT_HTTP200ALIASES, CURLOPT_MAIL_RCPT, CURLOPT_RESOLVE
native Handle:curl_slist();
native curl_slist_append(Handle:hndl, const String:buffer[]);


public CURL_Default_Perform_CallBack(Handle:hndl, CURLcode: code) { }


/**
 * Do not edit below this line!
 */
public Extension:__ext_curl = 
{
	name = "curl",
	file = "curl.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
