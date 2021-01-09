/*--------------------------------------------------------------------
This file is part of the Arduino WiFiEsp library.

The Arduino WiFiEsp library is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The Arduino WiFiEsp library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with The Arduino WiFiEsp library.  If not, see
<http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include<stdarg.h>
#include<stdio.h>

#include "utility/EspSpiDrv.h"
#include "utility/debug.h"


// Array of data to cache the information related to the networks discovered
char 	EspSpiDrv::_networkSsid[][WL_SSID_MAX_LENGTH] = {{"1"},{"2"},{"3"},{"4"},{"5"}};
int32_t EspSpiDrv::_networkRssi[WL_NETWORKS_LIST_MAXNUM] = { 0 };
uint8_t EspSpiDrv::_networkEncr[WL_NETWORKS_LIST_MAXNUM] = { 0 };

// Cached values of retrieved data
char EspSpiDrv::_ssid[] = {0};
uint8_t EspSpiDrv::_bssid[] = {0};
uint8_t EspSpiDrv::_mac[] = {0};
uint8_t EspSpiDrv::_localIp[] = {0};
char EspSpiDrv::fwVersion[] = {0};

uint8_t EspSpiDrv::_connId=0;

uint16_t EspSpiDrv::_remotePort  =0;
uint8_t EspSpiDrv::_remoteIp[] = {0};


void EspSpiDrv::wifiDriverInit(Stream *espSerial)
{
	LOGDEBUG(F("> wifiDriverInit"));

	bool initOK = false;
	
	if (esp32_spi_init_hard(WIFI_CS, WIFI_RST, WIFI_RDY, 1)) {
		initOK=true;
	}

	if (!initOK)
	{
		LOGERROR(F("Cannot initialize ESP module"));
		delay(5000);
		return;
	}

	/* reset is done by esp32_spi_init */
	//reset();

	// check firmware version
	getFwVersion();

	// prints a warning message if the firmware is not 1.X or 2.X
	if ((fwVersion[0] != '1' and fwVersion[0] != '2') or
		fwVersion[1] != '.')
	{
		LOGWARN1(F("Warning: Unsupported firmware"), fwVersion);
		delay(4000);
	}
	else
	{
		LOGINFO1(F("Initilization successful -"), fwVersion);
	}
}


void EspSpiDrv::reset()
{
	LOGDEBUG(F("> reset"));

	// TODO
	//esp32_spi_reset();
}



bool EspSpiDrv::wifiConnect(const char* ssid, const char* passphrase)
{
	LOGDEBUG(F("> wifiConnect"));

	// TODO
	// Escape character syntax is needed if "SSID" or "password" contains
	// any special characters (',', '"' and '/')

	// connect to access point, use CUR mode to avoid connection at boot
	int ret = esp32_spi_connect_AP((uint8_t*)ssid, (uint8_t*)passphrase, 20);

	if (ret==0)
	{
		LOGINFO1(F("Connected to"), ssid);
		return true;
	}

	LOGWARN1(F("Failed connecting to"), ssid);

	// clean additional messages logged after the FAIL tag
	delay(1000);

	return false;
}


bool EspSpiDrv::wifiStartAP(const char* ssid, const char* pwd, uint8_t channel, uint8_t enc, uint8_t espMode)
{
	LOGDEBUG(F("> wifiStartAP"));

	int ret = esp32_spi_wifi_set_ap_passphrase((uint8_t*)ssid, (uint8_t*)pwd, channel);

	if (ret!=0)
	{
		LOGWARN1(F("Failed to start AP"), ssid);
		return false;
	}

	LOGINFO1(F("Access point started"), ssid);
	return true;
}


int8_t EspSpiDrv::disconnect()
{
	LOGDEBUG(F("> disconnect"));

	if(esp32_spi_disconnect_from_AP()>=0)
		return WL_DISCONNECTED;

	// wait and clear any additional message
	delay(2000);

	return WL_DISCONNECTED;
}

void EspSpiDrv::config(IPAddress ip)
{
	LOGDEBUG(F("> config"));

	char buf[16];
	sprintf_P(buf, PSTR("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);

	uint32_t dw = ip;
	int ret = esp32_spi_wifi_set_ip_config(1, (uint8_t*)&dw, NULL, NULL);
	delay(500);

	if (ret==0)
	{
		LOGINFO1(F("IP address set"), buf);
	}
}

void EspSpiDrv::configAP(IPAddress ip)
{
	LOGDEBUG(F("> config"));
	
	config(ip);
}

uint8_t EspSpiDrv::getConnectionStatus()
{
	LOGDEBUG(F("> getConnectionStatus"));

/*
	AT+CIPSTATUS

	Response

		STATUS:<stat>
		+CIPSTATUS:<link ID>,<type>,<remote_IP>,<remote_port>,<local_port>,<tetype>

	Parameters

		<stat>
			2: Got IP
			3: Connected
			4: Disconnected
		<link ID> ID of the connection (0~4), for multi-connect
		<type> string, "TCP" or "UDP"
		<remote_IP> string, remote IP address.
		<remote_port> remote port number
		<local_port> ESP8266 local port number
		<tetype>
			0: ESP8266 runs as client
			1: ESP8266 runs as server
*/

	// 4: client disconnected
	// 5: wifi disconnected
	int s = esp32_spi_status();
	if(s==2 or s==3 or s==4)
		return WL_CONNECTED;
	else if(s==5)
		return WL_DISCONNECTED;

	return WL_IDLE_STATUS;
}

uint8_t EspSpiDrv::getClientState(uint8_t sock)
{
	LOGDEBUG1(F("> getClientState"), sock);

	if (esp32_spi_socket_status(sock) == SOCKET_ESTABLISHED)
	{
		LOGDEBUG(F("Connected"));
		return true;
	}

	LOGDEBUG(F("Not connected"));
	return false;
}

uint8_t* EspSpiDrv::getMacAddress()
{
	LOGDEBUG(F("> getMacAddress"));

	memset(_mac, 0, WL_MAC_ADDR_LENGTH);
	memcpy(_mac, esp32_spi_MAC_address(), WL_MAC_ADDR_LENGTH);

	return _mac;
}


void EspSpiDrv::getIpAddress(IPAddress& ip)
{
	LOGDEBUG(F("> getIpAddress"));

	esp32_spi_net_t* inet = esp32_spi_get_network_data();
	if (inet)
	{
		_localIp[0] = inet->localIp[0];
		_localIp[1] = inet->localIp[1];
		_localIp[2] = inet->localIp[2];
		_localIp[3] = inet->localIp[3];

		ip = _localIp;
	}
}

void EspSpiDrv::getIpAddressAP(IPAddress& ip)
{
	LOGDEBUG(F("> getIpAddressAP"));

	getIpAddress(ip);
}



char* EspSpiDrv::getCurrentSSID()
{
	LOGDEBUG(F("> getCurrentSSID"));

	_ssid[0] = 0;
	strcpy(_ssid, esp32_spi_get_ssid());

	return _ssid;
}

uint8_t* EspSpiDrv::getCurrentBSSID()
{
	LOGDEBUG(F("> getCurrentBSSID"));

	memset(_bssid, 0, WL_MAC_ADDR_LENGTH);
	memcpy(_bssid, esp32_spi_get_bssid(), WL_MAC_ADDR_LENGTH);

	return _bssid;

}

int32_t EspSpiDrv::getCurrentRSSI()
{
	LOGDEBUG(F("> getCurrentRSSI"));

    int ret=esp32_spi_get_rssi();

    return ret;
}


uint8_t EspSpiDrv::getScanNetworks()
{
	uint8_t ssidListNum = 0;

	LOGDEBUG(F("----------------------------------------------"));
	LOGDEBUG(F(">> AT+CWLAP"));

	esp32_spi_aps_list_t* aps_list = esp32_spi_scan_networks();
	if(aps_list == NULL){
		return -1;
	}

	uint32_t count = aps_list->aps_num;
	for(int i=0; i<count; i++)
	{	
		esp32_spi_ap_t* ap = aps_list->aps[i];
		_networkEncr[ssidListNum] = ap->encr;
		
		memset(_networkSsid[ssidListNum], 0, WL_SSID_MAX_LENGTH );
		strcpy(_networkSsid[ssidListNum], (const char*)&ap->ssid);

		_networkRssi[ssidListNum] = ap->rssi;
		
		if(ssidListNum==WL_NETWORKS_LIST_MAXNUM-1)
			break;

		ssidListNum++;
	}
	aps_list->del(aps_list);

	LOGDEBUG1(F("---------------------------------------------- >"), ssidListNum);
	LOGDEBUG();

	return ssidListNum;
}

bool EspSpiDrv::getNetmask(IPAddress& mask) {
	LOGDEBUG(F("> getNetmask"));

	esp32_spi_net_t* inet = esp32_spi_get_network_data();
	if (inet) {
		mask = inet->subnetMask;
		return true;
	}

	return false;
}

bool EspSpiDrv::getGateway(IPAddress& gw)
{
	LOGDEBUG(F("> getGateway"));

	esp32_spi_net_t* inet = esp32_spi_get_network_data();
	if (inet) {
		gw = inet->gatewayIp;
		return true;
	}

	return false;
}

char* EspSpiDrv::getSSIDNetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
		return NULL;

	return _networkSsid[networkItem];
}

uint8_t EspSpiDrv::getEncTypeNetowrks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
		return 0;

    return _networkEncr[networkItem];
}

int32_t EspSpiDrv::getRSSINetoworks(uint8_t networkItem)
{
	if (networkItem >= WL_NETWORKS_LIST_MAXNUM)
		return 0;

    return _networkRssi[networkItem];
}

char* EspSpiDrv::getFwVersion()
{
	LOGDEBUG(F("> getFwVersion"));

	fwVersion[0] = 0;

	esp32_spi_firmware_version(fwVersion);

    return fwVersion;
}



bool EspSpiDrv::ping(const char *host)
{
	LOGDEBUG(F("> ping"));

	int ret = esp32_spi_ping((uint8_t *)host, 1, 100);
	
	if (ret>=0) {
		return true;
	}

	LOGWARN1(F("ping failed"), ret);

	return false;
}



// Start server TCP on port specified
bool EspSpiDrv::startServer(uint16_t port, uint8_t sock)
{
	LOGDEBUG1(F("> startServer"), port);

	int ret = esp32_spi_start_server(port, sock, TCP_MODE);

	return ret==0;
}


bool EspSpiDrv::startClient(const char* host, uint16_t port, uint8_t sock, uint8_t protMode)
{
	LOGDEBUG2(F("> startClient"), host, port);
	
    if (!strcmp(host, "0"))
    {
	    return esp32_spi_start_server(port, sock, protMode);
    }

    if (sock != 0xff)
    {
        uint8_t ip[6];
        if (esp32_spi_get_host_by_name((uint8_t*)host, ip) == 0)
        {
            if (esp32_spi_socket_connect(sock, ip, 0, port, (esp32_socket_mode_enum_t)protMode) != 0)
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

	return true;
}


// Start server TCP on port specified
void EspSpiDrv::stopClient(uint8_t sock)
{
	LOGDEBUG1(F("> stopClient"), sock);

	esp32_spi_socket_close(sock);
}


uint8_t EspSpiDrv::getServerState(uint8_t sock)
{
    return esp32_spi_server_socket_status(sock);
}



////////////////////////////////////////////////////////////////////////////
// TCP/IP functions
////////////////////////////////////////////////////////////////////////////



uint16_t EspSpiDrv::availData(uint8_t connId)
{
    //LOGDEBUG(bufPos);

	uint16_t ret = esp32_spi_socket_available(connId);

	if (ret > 0) {
		uint8_t _remotePortArr[2] = {0};

		esp32_spi_get_remote_data(connId, _remoteIp, _remotePortArr);
		_remotePort = (_remotePortArr[0]<<8)+_remotePortArr[1];
	}

	return ret;
}


bool EspSpiDrv::getData(uint8_t connId, uint8_t *data, bool peek, bool* connClose)
{
	int ret = esp32_spi_socket_read(connId, data, 1);
	if (ret < 1) {
		return false;
	}

	return true;
}

/**
 * Receive the data into a buffer.
 * It reads up to bufSize bytes.
 * @return	received data size for success else -1.
 */
int EspSpiDrv::getDataBuf(uint8_t connId, uint8_t *buf, uint16_t bufSize)
{
	int ret = esp32_spi_socket_read(connId, buf, bufSize);
	if (ret < 1) {
		return -1;
	}

	return bufSize;
}


bool EspSpiDrv::sendData(uint8_t sock, const uint8_t *data, uint16_t len)
{
	LOGDEBUG2(F("> sendData:"), sock, len);

	if (esp32_spi_socket_write(sock, (uint8_t*)data, len) < 1)
	{
		return false;
	}

    return true;
}

// Overrided sendData method for __FlashStringHelper strings
bool EspSpiDrv::sendData(uint8_t sock, const __FlashStringHelper *data, uint16_t len, bool appendCrLf)
{
	LOGDEBUG2(F("> sendData:"), sock, len);

	if (esp32_spi_socket_write(sock, (uint8_t*)data, len) < 1)
	{
		return false;
	}

    return true;
}

bool EspSpiDrv::checkDataSent(uint8_t sock)
{
    const uint16_t TIMEOUT_DATA_SENT = 25;
    uint16_t timeout = 0;
    int8_t _data = 0;

    do {
        _data = esp32_spi_check_data_sent(sock);

        if (_data>0) timeout = 0;
        else{
            ++timeout;
            delay(100);
        }

    }while((_data<1)&&(timeout<TIMEOUT_DATA_SENT));
    return (timeout==TIMEOUT_DATA_SENT)?0:1;
}

bool EspSpiDrv::sendDataUdp(uint8_t sock, const char* host, uint16_t port, const uint8_t *data, uint16_t len)
{
	LOGDEBUG2(F("> sendDataUdp:"), sock, len);
	LOGDEBUG2(F("> sendDataUdp:"), host, port);

	if (!startClient(host, port, sock, UDP_MODE)) {
		return false;
	}
	if (esp32_spi_add_udp_data(sock, (uint8_t*)data, len) < 0) {
		return false;
	}
	if (esp32_spi_send_udp_data(sock) < 0) {
		return false;
	}
	return true;
}



void EspSpiDrv::getRemoteIpAddress(IPAddress& ip)
{
	ip = _remoteIp;
}

uint16_t EspSpiDrv::getRemotePort()
{
	return _remotePort;
}



EspSpiDrv espSpiDrv;
