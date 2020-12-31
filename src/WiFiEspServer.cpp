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

#include "WiFiEspServer.h"

#include "utility/WiFiEspDrv.h"
#include "utility/debug.h"



WiFiEspServer::WiFiEspServer(uint16_t port)
{
	_port = port;
	_lastSock = NO_SOCKET_AVAIL;
}

void WiFiEspServer::begin()
{
	LOGDEBUG(F("Starting server"));

#ifdef WIFI_ESP_AT
	/* The ESP Module only allows socket 1 to be used for the server */
	_sock = 1; // If this is already in use, the startServer attempt will fail
#else
	_sock = WiFiEspClass::getFreeSocket();
	if (_sock == SOCK_NOT_AVAIL)
	  {
	    LOGERROR(F("No socket available for server"));
	    return;
	  }
#endif
	WiFiEspClass::allocateSocket(_sock);

	_started = WIFIDRV::startServer(_port, _sock);

	if (_started)
	{
		LOGINFO1(F("Server started on port"), _port);
	}
	else
	{
		LOGERROR(F("Server failed to start"));
	}
}

WiFiEspClient WiFiEspServer::available(byte* status)
{
#ifdef WIFI_ESP_AT
	// TODO the original method seems to handle automatic server restart

	int bytes = WIFIDRV::availData(0);
	if (bytes>0)
	{
		LOGINFO1(F("New client"), WIFIDRV::_connId);
		WiFiEspClass::allocateSocket(WIFIDRV::_connId);
		WiFiEspClient client(WIFIDRV::_connId);
		return client;
	}
#else
    int sock = NO_SOCKET_AVAIL;

    if (_sock != NO_SOCKET_AVAIL) {
      // check previous received client socket
      if (_lastSock != NO_SOCKET_AVAIL) {
          WiFiEspClient client(_lastSock);

          if (client.connected() && client.available()) {
              sock = _lastSock;
          }
      }

      if (sock == NO_SOCKET_AVAIL) {
          // check for new client socket
          sock = WIFIDRV::availData(_sock);
      }
    }

    if (sock != NO_SOCKET_AVAIL) {
        WiFiEspClient client(sock);

        if (status != NULL) {
            *status = client.status();
        }

        _lastSock = sock;

        return client;
    }
#endif

    return WiFiEspClient(255);
}

uint8_t WiFiEspServer::status()
{
    if (_sock == NO_SOCKET_AVAIL) {
        return CLOSED;
    } else {
        return  WIFIDRV::getServerState(_sock);
    }
}

size_t WiFiEspServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiEspServer::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;

    if (size==0)
    {
        return 0;
    }

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++)
    {
        if (WiFiEspClass::_state[sock] != 0)
        {
        	WiFiEspClient client(sock);
            n += client.write(buffer, size);
        }
    }
    return n;
}
