/**
 * vim: set ts=4 :
 * =============================================================================
 * cURL Echo Client Example
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
curl_echo_connect 127.0.0.1 9999
curl_echo_send the message
curl_echo_send exit
curl_echo_send quit

*/



#if I_KNOW_HOW_TO_USE_THIS_PLUGIN
public Plugin:myinfo = 
{
	name = "cURL echo test",
	author = "Raydan",
	description = "cURL echo test",
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


new bool:connected = false;
new bool:connecting = false;
new bool:waiting_recv = false;
new bool:is_exit = false;

new Handle:curl_echo = INVALID_HANDLE;

public OnPluginStart()
{
	connected = false;
	connecting = false;
	RegConsoleCmd("curl_echo_connect", curl_echo_connect);
	RegConsoleCmd("curl_echo_send", curl_echo_send);
}

public SendRecv_Act:send_callback(Handle:hndl, CURLcode: code, const last_sent_dataSize)
{
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_easy_strerror(code, error_buffer, sizeof(error_buffer));
		PrintTestDebug("Echo Send Fail: %s",error_buffer);
		return SendRecv_Act_GOTO_END;
	}
	return (is_exit) ? SendRecv_Act_GOTO_END : SendRecv_Act_GOTO_RECV;
}

public SendRecv_Act:recv_callback(Handle:hndl, CURLcode: code, const String:receiveData[], const dataSize)
{
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_easy_strerror(code, error_buffer, sizeof(error_buffer));
		PrintTestDebug("Echo Receive Fail: %s",error_buffer);
		return SendRecv_Act_GOTO_END;
	}
	
	new String:buffer[dataSize];
	strcopy(buffer,dataSize, receiveData);
	PrintTestDebug("Echo Receive: DataSize - %d",dataSize);
	PrintTestDebug("Echo Receive: Data - %s",buffer);
	waiting_recv = false;
	return SendRecv_Act_GOTO_WAIT;
}

public senc_recv_complete_callback(Handle:hndl, CURLcode: code)
{
	PrintTestDebug("Disconnected To Echo Server");
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_easy_strerror(code, error_buffer, sizeof(error_buffer));
		PrintTestDebug("Error On Disceonnect - %s", error_buffer);
	}
	
	CloseHandle(curl_echo);
	curl_echo = INVALID_HANDLE;
	connected = false;
	connecting = false;
}

public ConnectToEchoServer(const String:ip[], port)
{
	curl_echo = curl_easy_init();
	if(curl_echo != INVALID_HANDLE)
	{
		CURL_DEFAULT_OPT(curl_echo);
		curl_easy_setopt_int(curl_echo, CURLOPT_CONNECT_ONLY, 1);
		curl_easy_setopt_string(curl_echo, CURLOPT_URL, ip);
		curl_easy_setopt_int(curl_echo, CURLOPT_PORT, port),
		curl_easy_perform_thread(curl_echo, OnEchoServerConnected);
	} else {
		PrintTestDebug("Unable Create cURL Handle");
		connected = false;
		connecting = false;
	}
}

public OnEchoServerConnected(Handle:hndl, CURLcode: code, any:data)
{
	connecting = false;
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_easy_strerror(code, error_buffer, sizeof(error_buffer));
		PrintToServer("Connect To Echo Server FAIL - %s", error_buffer);
		CloseHandle(curl_echo);
		curl_echo = INVALID_HANDLE;
		connected = false;
		return;
	}
	
	PrintTestDebug("Connected To Echo Server!!");
	
	waiting_recv = false;
	is_exit = false;
	connected = true;
	curl_easy_send_recv(curl_echo, send_callback, recv_callback, senc_recv_complete_callback, SendRecv_Act_GOTO_WAIT, 60000,60000);
}

public Action:curl_echo_connect(client, args)
{
	if(connected)
	{
		PrintTestDebug("Already Connected To Echo Server");
		return Plugin_Handled;
	}
	
	if(connecting)
	{
		PrintTestDebug("Already Connecting To Echo Server");
		return Plugin_Handled;
	}
	
	if(GetCmdArgs() != 2)
	{
		PrintTestDebug("curl_echo_connect \"server_ip\" \"server_port\"");
		return Plugin_Handled;
	}
	
	new String:ip[128];
	new String:buffer[8];
	GetCmdArg(1, ip, sizeof(ip));
	GetCmdArg(2, buffer, sizeof(buffer));
	new port = StringToInt(buffer);
	
	connected = false;
	connecting = true;
	ConnectToEchoServer(ip, port);
	return Plugin_Handled;
}

public Action:curl_echo_send(client, args)
{
	if(!connected)
	{
		PrintTestDebug("Not Connected To Echo Server");
		return Plugin_Handled;
	}
	
	if(connecting)
	{
		PrintTestDebug("Connecting To Echo Server");
		return Plugin_Handled;
	}
	
	if(waiting_recv)
	{
		PrintTestDebug("Waiting Echo Server Reply");
		return Plugin_Handled;
	}
	
	if(GetCmdArgs() == 0)
	{
		PrintTestDebug("Usage: curl_echo_send <the string you want to send>");
		return Plugin_Handled;
	}
	
	new String:buffer[256];
	GetCmdArgString(buffer, sizeof(buffer));
	
	new String:old_buffer[256];
	strcopy(old_buffer, sizeof(old_buffer),buffer);
	
	is_exit = (StrEqual(buffer,"exit") || StrEqual(buffer,"quit")) ? true : false;

	if(StrCat(buffer, sizeof(buffer), "\n") == 0) // Add \n to the end
	{
		PrintTestDebug("Can't Send, Maximum Length is 256");
		return Plugin_Handled;
	}
	
	waiting_recv = true;
	PrintTestDebug("Echo Send: %s",old_buffer);
	curl_set_send_buffer(curl_echo, buffer);
	curl_send_recv_Signal(curl_echo, SendRecv_Act_GOTO_SEND);
	return Plugin_Handled;
}


stock PrintTestDebug(const String:format[], any:...)
{
	decl String:buffer[256];
	VFormat(buffer, sizeof(buffer), format, 2);
	PrintToServer("[CURL ECHO Test] %s", buffer);
}
#else
It make me unable to compile
#endif
