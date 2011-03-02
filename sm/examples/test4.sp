/**
 * vim: set ts=4 :
 * =============================================================================
 * cURL Gmail SMTP Example
 * 
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


#pragma semicolon 1
#include <sourcemod>
#include <cURL>

/* =============================================================================
                    YOU MUST KNOW HOW TO USE THIS PLUGIN
   ============================================================================= */
#define I_KNOW_HOW_TO_USE_THIS_PLUGIN	0

/*
Command Usage:
curl_smtp_send admin@gmail.com 123456 admin@hotmail.com

Reference:
create "curl_smtp_message.txt" for cURL read data

*/


#define TEST_FOLDER				"data/curl_test"


#if I_KNOW_HOW_TO_USE_THIS_PLUGIN
public Plugin:myinfo = 
{
	name = "cURL smtp gamil test",
	author = "Raydan",
	description = "cURL smtp gamil test",
	version = "1.0",
	url = "http://www.ZombieX2.net/"
};

new CURL_Default_opt[][2] = {
	{_:CURLOPT_NOSIGNAL,1},
	{_:CURLOPT_NOPROGRESS,1},
	{_:CURLOPT_TIMEOUT,30},
	{_:CURLOPT_VERBOSE,0}
};

#define CURL_DEFAULT_OPT(%1) curl_easy_setopt_int_array(%1, CURL_Default_opt, sizeof(CURL_Default_opt))


new Handle:curl_smtp = INVALID_HANDLE;

new bool:sending = false;
new String:curl_test_path[512];

public OnPluginStart()
{	
	BuildPath(Path_SM, curl_test_path, sizeof(curl_test_path), TEST_FOLDER);
	RegConsoleCmd("curl_smtp_send", curl_smtp_send);
}

public Action:curl_smtp_send(client, args)
{
	if(sending)
	{
		PrintTestDebug("Sending Email, Please wait...");
		return Plugin_Handled;
	}
	
	if(GetCmdArgs() != 3)
	{
		PrintTestDebug("curl_smtp_send \"gmail_email\" \"gmail_password\" \"receiver_email\"");
		return Plugin_Handled;
	}
	
	new String:buffer[256];
	new String:email_from[128];
	new String:email_password[128];
	new String:receiver_email[128];
	
	GetCmdArg(1, email_from, sizeof(email_from));
	GetCmdArg(2, email_password, sizeof(email_password));
	GetCmdArg(3, buffer, sizeof(buffer));
	Format(receiver_email, sizeof(receiver_email),"<%s>",buffer);
	
	if(!CreateMessageFile(email_from))
	{
		PrintTestDebug("Unable Create Message File");
		return Plugin_Handled;
	}
	
	curl_smtp = curl_easy_init();
	if(curl_smtp == INVALID_HANDLE)
	{
		PrintTestDebug("curl_easy_init Fail");
		return Plugin_Handled;
	}
	
	sending = true;
	PrintTestDebug("Sending Email...");
	
	CURL_DEFAULT_OPT(curl_smtp);
	new Handle:file = CreateTestFile("curl_smtp_message.txt","rb");
	new Handle:slist = curl_slist();
	curl_slist_append(slist, receiver_email);

	Format(buffer, sizeof(buffer),"<%s>",email_from);
	curl_easy_setopt_string(curl_smtp, CURLOPT_MAIL_FROM, buffer);
	curl_easy_setopt_string(curl_smtp, CURLOPT_USERNAME,email_from);
	curl_easy_setopt_string(curl_smtp, CURLOPT_PASSWORD,email_password);
	curl_easy_setopt_int(curl_smtp, CURLOPT_USE_SSL, CURLUSESSL_ALL);
	curl_easy_setopt_handle(curl_smtp, CURLOPT_MAIL_RCPT, slist);
	curl_easy_setopt_handle(curl_smtp, CURLOPT_READDATA, file);
	curl_easy_setopt_int(curl_smtp, CURLOPT_SSL_VERIFYPEER, 0); // if set to 1, need ca-bundle.crt
	curl_easy_setopt_int(curl_smtp, CURLOPT_SSL_VERIFYHOST, 2);
	curl_easy_setopt_string(curl_smtp, CURLOPT_URL,"smtp://smtp.gmail.com");
	CloseHandle(file);
	CloseHandle(slist);
	
	curl_easy_perform_thread(curl_smtp, onComplete);
	return Plugin_Handled;
}

public onComplete(Handle:hndl, CURLcode: code, any:data)
{
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_easy_strerror(code, error_buffer, sizeof(error_buffer));
		PrintTestDebug("Send Email FAIL - %s", error_buffer);
		CloseHandle(curl_smtp);
		curl_smtp = INVALID_HANDLE;
		sending = false;
		return;
	}
	
	CloseHandle(curl_smtp);
	curl_smtp = INVALID_HANDLE;
	
	PrintTestDebug("Send Email Success, Check your email now");
	sending = false;
}

public bool:CreateMessageFile(const String:email_from[])
{
	new String:file_path[512];
	Format(file_path, sizeof(file_path),"%s/%s",curl_test_path, "curl_smtp_message.txt");
	new Handle:file = OpenFile(file_path,"w");
	if(file == INVALID_HANDLE)
		return false;
	
	WriteFileLine(file,"MIME-Version: 1.0");
	WriteFileLine(file,"Content-type: text/html; charset=utf-8");
	WriteFileLine(file,"From: <%s>",email_from);
	WriteFileLine(file,"Subject: %s\n","Sourcemod cURL SMTP Test");
	WriteFileString(file,"The Email Content Line #1<br/>Line#2<br/>Url: <a href=\"http://www.google.com\">http://www.google.com</a>", false);
	
	CloseHandle(file);
	return true;
}

public Handle:CreateTestFile(const String:path[], const String:mode[])
{
	new String:file_path[512];
	Format(file_path, sizeof(file_path),"%s/%s",curl_test_path, path);
	return curl_OpenFile(file_path, mode);
}

stock PrintTestDebug(const String:format[], any:...)
{
	decl String:buffer[256];
	VFormat(buffer, sizeof(buffer), format, 2);
	PrintToServer("[CURL SMTP Test] %s", buffer);
}
#else
It make me unable to compile
#endif

