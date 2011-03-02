/**
 * vim: set ts=4 :
 * =============================================================================
 * cURL Source Rcon Example
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
curl_rcon_connect 127.0.0.1 27015 thepassword
curl_rcon_send status
curl_rcon_send cvarlist
curl_rcon_disconnect

Reference:
http://developer.valvesoftware.com/wiki/Source_RCON_Protocol

*/

#define SERVERDATA_EXECCOMMAND			2
#define SERVERDATA_AUTH					3
#define SERVERDATA_RESPONSE_VALUE		0
#define SERVERDATA_AUTH_RESPONSE		2

#define AUTH_PACKET_ID					5

#define RECEIVE_BUFFER_SIZE				1024


#if I_KNOW_HOW_TO_USE_THIS_PLUGIN
public Plugin:myinfo = 
{
	name = "cURL source engine rcon query test",
	author = "Raydan",
	description = "cURL source engine rcon query test",
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


/* BYTE stuff */
#define PACK_LITTLE_ENDIAN_INT(%1,%2,%3)\
	%3[%1+3] = (%2 >> 24);\
	%3[%1+2] = ((%2 & 0xff0000) >> 16);\
	%3[%1+1] = ((%2 & 0xff00) >> 8);\
	%3[%1] = (%2 & 0xff);\
	%1+=4;


#define PACK_STRING(%1,%2,%3)\
	for(new __i=0;__i<strlen(%2);__i++)\
		%3[%1+__i] = %2[__i];\
	%1+=strlen(%2);


#define PACK_BYTE(%1,%2,%3)\
	%3[%1] = %2;\
	%1+=1;


#define GET_LITTLE_ENDIAN_INT(%1,%2,%3)\
	%2 |= %3[%1+3] & 0xFF;\
	%2 <<= 8;\
	%2 |= %3[%1+2] & 0xFF;\
	%2 <<= 8;\
	%2 |= %3[%1+1] & 0xFF;\
	%2 <<= 8;\
	%2 |= %3[%1] & 0xFF;



new bool:connected = false;
new bool:connecting = false;
new bool:waiting_recv = false;
new bool:authenticate = false;

new Handle:curl_rcon = INVALID_HANDLE;

new String:rconpassword[128];
new packet_id = 20;
new juck_packet = 0;
new bool:is_multiple_packet = false;

public OnPluginStart()
{
	connected = false;
	connecting = false;
	RegConsoleCmd("curl_rcon_connect", curl_rcon_connect);
	RegConsoleCmd("curl_rcon_send", curl_rcon_send);
	RegConsoleCmd("curl_rcon_disconnect", curl_rcon_disconnect);
}

public SendRecv_Act:send_callback(Handle:hndl, CURLcode: code, const last_sent_dataSize)
{
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_get_error_buffer(hndl, error_buffer, sizeof(error_buffer));
		PrintTestDebug("Rcon Send Fail: %s",error_buffer);
		return SendRecv_Act_GOTO_END;
	}
	
	if(authenticate)
	{
		return SendRecv_Act_GOTO_RECV;
	} else {
		juck_packet = 0;
		return SendRecv_Act_GOTO_RECV;
	}
}

public bool:CheckAuthenticate(pos, const String:receiveData[])
{
	new authid = 0;
	GET_LITTLE_ENDIAN_INT((pos+4),authid,receiveData)
	new response = 0;
	GET_LITTLE_ENDIAN_INT((pos+8),response,receiveData)
	if(authid != -1 && authid == AUTH_PACKET_ID && response == SERVERDATA_AUTH_RESPONSE)
	{
		authenticate = true;
		PrintTestDebug("Authenticate To Server!!");
		return true;
	} else {
		PrintTestDebug("Unable Authenticate To Server");
		return false;
	}
}

public PrintResponseData(pos, const String:receiveData[], size)
{
	new String:buffer[RECEIVE_BUFFER_SIZE+1];
	for(new i=pos;i<size;i++)
	{
		buffer[i-pos] = receiveData[i];
	}
	PrintTestDebug("Server Response:");
	PrintToServer("-----------------------------");
	PrintToServer("%s",buffer);
	PrintToServer("-----------------------------");
}

public SendRecv_Act:HandleResponse(const String:receiveData[], const dataSize)
{
	if(!is_multiple_packet)
	{
		new packet_size = 0;
		GET_LITTLE_ENDIAN_INT(0,packet_size,receiveData)
		new request_id = 0;
		GET_LITTLE_ENDIAN_INT(4,request_id,receiveData)
		new response_type = 0;
		GET_LITTLE_ENDIAN_INT(8,response_type,receiveData)
		if(dataSize >= packet_size) // small response, like "version"
		{
			PrintResponseData(12,receiveData,dataSize);
			waiting_recv = false;
			return SendRecv_Act_GOTO_WAIT;
		} else {
			is_multiple_packet = true;
			PrintResponseData(12,receiveData,dataSize);
			return SendRecv_Act_GOTO_RECV;
		}
	} else {
		PrintResponseData(0,receiveData,dataSize);
		if(RECEIVE_BUFFER_SIZE > dataSize)
		{
			PrintTestDebug("No More Multiple Packet");
			is_multiple_packet = false;
			waiting_recv = false;
			return SendRecv_Act_GOTO_WAIT;
		}
		return SendRecv_Act_GOTO_RECV;
	}
}

public SendRecv_Act:recv_callback(Handle:hndl, CURLcode: code, const String:receiveData[], const dataSize)
{
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_get_error_buffer(hndl, error_buffer, sizeof(error_buffer));
		PrintTestDebug("Rcon Receive Fail: %s",error_buffer);
		return SendRecv_Act_GOTO_END;
	}
	
	if(authenticate)
	{
		return HandleResponse(receiveData, dataSize);
	} else {
		if(dataSize == 14)
		{
			juck_packet++;
		} else {
			if(!CheckAuthenticate(14,receiveData))
				return SendRecv_Act_GOTO_END;
			return SendRecv_Act_GOTO_WAIT;
		}
		if(juck_packet == 1)
		{
			juck_packet++;
			return SendRecv_Act_GOTO_RECV;
		} else {
			if(!CheckAuthenticate(0,receiveData))
				return SendRecv_Act_GOTO_END;
			return SendRecv_Act_GOTO_WAIT;
		}
	}
}

public senc_recv_complete_callback(Handle:hndl, CURLcode: code)
{
	PrintTestDebug("Disconnect To Source Server");
	
	CloseHandle(curl_rcon);
	curl_rcon = INVALID_HANDLE;
	connected = false;
	connecting = false;
	authenticate = false;
}

public OnSourceServerConnected(Handle:hndl, CURLcode: code, any:data)
{
	connecting = false;
	if(code != CURLE_OK)
	{
		new String:error_buffer[256];
		curl_easy_strerror(code, error_buffer, sizeof(error_buffer));
		PrintToServer("Source Server Connect FAIL - %s", error_buffer);
		CloseHandle(curl_rcon);
		curl_rcon = INVALID_HANDLE;
		connected = false;
		return;
	}
	
	PrintTestDebug("Connected To Source Server, Now Try Authenticate");
	
	waiting_recv = false;
	connected = true;
	authenticate = false;
	
	ConstructPacketToSendBuffer(AUTH_PACKET_ID, SERVERDATA_AUTH, rconpassword);
	juck_packet = 0;
	curl_easy_send_recv(curl_rcon, send_callback, recv_callback, senc_recv_complete_callback, SendRecv_Act_GOTO_SEND, 5000, 5000, RECEIVE_BUFFER_SIZE);
}

public ConnectToSourceServer(const String:ip[], port)
{
	curl_rcon = curl_easy_init();
	if(curl_rcon != INVALID_HANDLE)
	{
		CURL_DEFAULT_OPT(curl_rcon);
		curl_easy_setopt_int(curl_rcon, CURLOPT_CONNECT_ONLY, 1);
		curl_easy_setopt_string(curl_rcon, CURLOPT_URL, ip);
		curl_easy_setopt_int(curl_rcon, CURLOPT_PORT, port),
		curl_easy_perform_thread(curl_rcon, OnSourceServerConnected);
	} else {
		PrintTestDebug("Unable Create cURL Handle");
		connected = false;
		connecting = false;
		authenticate = false;
	}
}

public Action:curl_rcon_connect(client, args)
{
	if(connected)
	{
		PrintTestDebug("Already Connected To Source Server");
		return Plugin_Handled;
	}
	
	if(connecting)
	{
		PrintTestDebug("Already Connecting To Source Server");
		return Plugin_Handled;
	}
	
	if(GetCmdArgs() != 3)
	{
		PrintTestDebug("curl_rcon_connect \"server_ip\" \"server_port\" \"rconpassword\"");
		return Plugin_Handled;
	}
	
	new String:ip[128];
	new String:buffer[8];
	GetCmdArg(1, ip, sizeof(ip));
	GetCmdArg(2, buffer, sizeof(buffer));
	GetCmdArg(3, rconpassword, sizeof(rconpassword));
	new port = StringToInt(buffer);
	
	connected = false;
	connecting = true;
	authenticate = false;
	ConnectToSourceServer(ip, port);
	
	return Plugin_Handled;
}

public Action:curl_rcon_send(client, args)
{
	if(!connected || !authenticate)
	{
		PrintTestDebug("Not Connected To Source Server");
		return Plugin_Handled;
	}
	
	if(connecting)
	{
		PrintTestDebug("Connecting To Source Server");
		return Plugin_Handled;
	}
	
	if(waiting_recv)
	{
		PrintTestDebug("Waiting Source Server Reply");
		return Plugin_Handled;
	}
	
	if(GetCmdArgs() == 0)
	{
		PrintTestDebug("Usage: curl_rcon_send <the command you want to send>");
		return Plugin_Handled;
	}
	
	new String:buffer[256];
	GetCmdArgString(buffer, sizeof(buffer));
	
	waiting_recv = true;
	
	packet_id++;
	if(packet_id > 32767)
		packet_id = 20;
	
	is_multiple_packet = false;
	ConstructPacketToSendBuffer(packet_id,SERVERDATA_EXECCOMMAND,buffer);
	curl_send_recv_Signal(curl_rcon, SendRecv_Act_GOTO_SEND);
	
	return Plugin_Handled;
}

public Action:curl_rcon_disconnect(client, args)
{
	if(!connected)
	{
		PrintTestDebug("Not Connected To Source Server");
		return Plugin_Handled;
	}
	
	if(connecting)
	{
		PrintTestDebug("Connecting To Source Server");
		return Plugin_Handled;
	}
	
	if(waiting_recv) // if receiving data & close connect, server may crash!
	{
		PrintTestDebug("Waiting Source Server Reply");
		return Plugin_Handled;
	}
	
	curl_send_recv_Signal(curl_rcon, SendRecv_Act_GOTO_END);
	return Plugin_Handled;
}


public ConstructPacketToSendBuffer(id, cmd, String:s1[])
{
	new data_length = strlen(s1);
	new packet_length = data_length+12;
	new pos = 0;
	new String:packet_data[256];
	PACK_LITTLE_ENDIAN_INT(pos,packet_length,packet_data)
	PACK_LITTLE_ENDIAN_INT(pos,id,packet_data)
	PACK_LITTLE_ENDIAN_INT(pos,cmd,packet_data)
	PACK_STRING(pos,s1,packet_data)
	PACK_BYTE(pos,0x00,packet_data)
	PACK_BYTE(pos,0x00,packet_data)
	PACK_BYTE(pos,0x00,packet_data)
	PACK_BYTE(pos,0x00,packet_data)
	
	curl_set_send_buffer(curl_rcon, packet_data, pos+4);
}

stock PrintTestDebug(const String:format[], any:...)
{
	decl String:buffer[256];
	VFormat(buffer, sizeof(buffer), format, 2);
	PrintToServer("[CURL RCON Test] %s", buffer);
}

#else
It make me unable to compile
#endif

