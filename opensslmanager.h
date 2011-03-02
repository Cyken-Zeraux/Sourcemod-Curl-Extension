#ifndef _INCLUDE_SOURCEMOD_EXTENSION_OPENSSLMANAGER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_OPENSSLMANAGER_H_

#include "extension.h"
#include "opensslthread.h"

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

class OpensslManager
{
public:
	void SDK_OnLoad();
	void SDK_OnUnload();

public:
	bool HashFile(Openssl_Hash algorithm, FILE *pFile, unsigned char **output, int *outlength);
	bool HashString(Openssl_Hash algorithm, unsigned char *input, int size, unsigned char *output, int *outlength);

};

extern OpensslManager g_OpensslManager;


#endif

