/*
 *  VS_Socket.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/04/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Socket.h"

#include "VS_NetClient.h"

#include "VS_Store.h"

#include <stdio.h>
#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#endif
#include <sys/types.h>

vsSocket::vsSocket(int port):
	m_privateIP(0),
	m_privatePort(0),
	//m_publicIP(0),
	//m_publicPort(0),
	m_listener(nullptr)
{
	UNUSED(port);
#ifndef _WIN32
	vsLog("Opening socket");

	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if ( m_socket == -1 )
	{
		perror("socket");
		vsAssert( m_socket != -1, vsFormatString("Socket error:  See console output for details" ) );
	}

	sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(addr.sin_zero, '\0', sizeof addr.sin_zero);

	vsLog("Binding socket to port %d", port);

	int err = bind( m_socket, (sockaddr *)&addr, sizeof(addr) );
	if ( err == -1 )
	{
		perror("bind");
		vsAssert( m_socket != -1, vsFormatString("Bind error:  See console output for details" ) );
	}

	socklen_t addrSize = sizeof(addr);
	err = getsockname(m_socket, (sockaddr*)&addr, &addrSize);
	if ( err == -1 )
	{
		perror("getsockname");
		vsAssert( m_socket != -1, vsFormatString("Getsockname error:  See console output for details" ) );
	}

	m_privateIP = ntohl(addr.sin_addr.s_addr);
	m_privatePort = ntohs(addr.sin_port);

	if ( m_privateIP == 0 )	// should be
	{
		vsLog("Bind succeeded:  Listening on INADDR_ANY:%d", m_privatePort);
	}
	else
	{
		char * byte = (char *)&m_privateIP;
		vsLog("Bind succeeded:  Listening on %d.%d.%d.%d:%d", byte[0], byte[1], byte[2], byte[3], m_privatePort);
	}
#endif //_WIN32
}

vsSocket::~vsSocket()
{
#ifndef _WIN32
	if ( m_socket > -1 )
		close( m_socket );
#endif // _WIN32
}

void
vsSocket::SetListener( vsSocketListener *l )
{
	m_listener = l;
}

void
vsSocket::Poll()
{
#ifndef _WIN32

	pollfd p;
	p.fd = m_socket;
	p.events = POLLERR | POLLHUP | POLLIN;
	p.revents = 0;

	int ret = poll( &p, 1, 0 );

	if ( ret > 0 )
	{
		if ( p.revents & POLLIN )
		{
			char buffer[1024];
			sockaddr_in from;
			socklen_t fromLen = sizeof(from);
			int flags = 0;

			ssize_t bytes = recvfrom(m_socket, buffer, 1024, flags, (sockaddr *)&from, &fromLen);

			vsAssert(bytes > 0, "Zero or negative byte packet received?");
			vsStore packet(buffer, bytes);

			// now pass around the packet to be interpreted.

			if ( m_listener )
			{
				vsNetClient c(ntohl(from.sin_addr.s_addr), ntohs(from.sin_port));

				m_listener->HandlePacket(&c, &packet);
			}
		}
	}
#endif //_WIN32
}

void
vsSocket::SendTo( vsNetClient *to, vsStore *packet )
{
	packet->Rewind();

	char *buffer = packet->GetReadHead();
	int len = packet->Length();
	int flags = 0;

	sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(to->GetPort());
    destAddr.sin_addr.s_addr = to->GetIP();		// don't convert this to network byte order;  it's already stored that way!
    memset(destAddr.sin_zero, '\0', sizeof destAddr.sin_zero);

	int n = sendto(m_socket, buffer, len, flags, (sockaddr *)&destAddr, sizeof(destAddr));

	if ( n != len )
	{
		if ( n == -1 )
		{
			perror("sendto");
		}
		else
		{
			vsLog("Error:  Expected to write %d bytes, actually wrote %d bytes!", len, n);
		}
		vsAssert( n == len, "Sendto error:  Check console for details." );
	}
}


