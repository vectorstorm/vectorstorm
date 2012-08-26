/*
 *  VS_SocketTCP.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 5/06/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_SocketTCP.h"

#include "VS_NetClient.h"

#include "VS_Store.h"

#include <stdio.h>
#include <fcntl.h>

#if defined(_WIN32)
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#endif
#include <sys/types.h>


vsSocketTCP::vsSocketTCP( int maxConnectionCount ):
m_privateIP(0),
m_privatePort(0),
m_socket(-1),
m_listening(false),
m_inUpdate(false),
m_listener(NULL),
m_connection(NULL),
m_connectionCount(maxConnectionCount)
{
	m_connection = new vsTCPConnection[ maxConnectionCount ];
#ifndef _WIN32
	m_pollfds = new pollfd[ m_connectionCount+1 ];
#endif // _WIN32

	for ( int i = 0; i < m_connectionCount; i++ )
	{
		m_connection[i].m_socket = -1;
		m_connection[i].m_receiveBuffer = m_connection[i].m_sendBuffer = NULL;
		m_connection[i].m_closing = false;
		m_connection[i].m_sigHup = false;
	}
}

vsSocketTCP::~vsSocketTCP()
{
	if ( m_socket > -1 )
	{
#ifdef _WIN32
		closesocket( m_socket );
#else
		close( m_socket );
#endif
	}

	for ( int i = 0; i < m_connectionCount; i++ )
	{
		if ( m_connection[i].m_socket != -1 )
		{
#ifdef _WIN32
			closesocket( m_socket );
#else
			close( m_socket );
#endif
		}
		vsDelete( m_connection[i].m_receiveBuffer );
		vsDelete( m_connection[i].m_sendBuffer );

#ifndef _WIN32
		vsDeleteArray( m_pollfds );
#endif
	}

	vsDeleteArray( m_connection );
}

bool
vsSocketTCP::Listen( uint16_t port, vsSocketTCP::Type t )
{
#ifndef _WIN32
	if ( m_socket == -1 )
	{
		//vsLog("Opening socket");

		m_socket = socket(AF_INET, SOCK_STREAM, 0);
		if ( m_socket == -1 )
		{
			perror("socket");
			vsAssert( m_socket != -1, vsFormatString("Socket error:  See console output for details" ) );
		}
	}

	sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

	switch( t )
	{
		case Type_Global:
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			break;
		case Type_Localhost:
#if defined(_WIN32)
			addr.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
			inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));
#endif
			break;
	}


    memset(addr.sin_zero, '\0', sizeof addr.sin_zero);

	//vsLog("Binding socket to port %d", port);

	int err = bind( m_socket, (sockaddr *)&addr, sizeof(addr) );
#ifndef _WIN32
	fcntl(m_socket, F_SETFL, O_NONBLOCK);
#endif // _WIN32

	if ( err == -1 )
	{
		perror("bind");
		return false;
	}

	err = listen( m_socket, m_connectionCount );
	m_listening = true;
	if ( err == -1 )
	{
		perror("listen");
		vsAssert( err != -1, vsFormatString("Listen error:  See console output for details" ) );
	}

	socklen_t addrSize = sizeof(addr);
	err = getsockname(m_socket, (sockaddr*)&addr, &addrSize);
	if ( err == -1 )
	{
		perror("getsockname");
		vsAssert( err != -1, vsFormatString("Getsockname error:  See console output for details" ) );
	}

	m_privateIP = addr.sin_addr.s_addr;
	m_privatePort = addr.sin_port;

	if ( m_privateIP == 0 )	// should be
	{
		vsLog("Bind succeeded:  Listening on INADDR_ANY:%d", ntohs(m_privatePort));
	}
	else
	{
		char * byte = (char *)&m_privateIP;
		vsLog("Bind succeeded:  Listening on %d.%d.%d.%d:%d", byte[0], byte[1], byte[2], byte[3], ntohs(m_privatePort));
	}
#endif // _WIN32
	return true;
}

void
vsSocketTCP::Send( vsTCPConnection *to, vsStore *packet )
{
	to->m_sendBuffer->Append(packet);
}

void
vsSocketTCP::Update( float timeStep )
{
	m_inUpdate = true;

#ifndef _WIN32

    struct sockaddr_storage their_addr;
	struct timeval tv;
    socklen_t addr_size;
	int pollCount = 1;

	addr_size = sizeof their_addr;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	m_pollfds[0].fd = m_socket;
	m_pollfds[0].events = POLLIN;

	for ( int i = 0; i < m_connectionCount; i++ )
	{
		if ( m_connection[i].m_socket != -1 )
		{
			m_pollfds[pollCount].fd = m_connection[i].m_socket;
			m_pollfds[pollCount].events = POLLIN;

			//if ( m_connection[i].m_sendBuffer->BytesLeftForReading() )
			{
				m_pollfds[pollCount].events |= POLLOUT;
			}
			pollCount++;
		}
	}

	if ( poll(m_pollfds, pollCount, 0) > 0 )
	{
		if ( m_pollfds[0].revents & POLLERR || m_pollfds[0].revents & POLLHUP || m_pollfds[0].revents & POLLNVAL )
		{
			vsLog("Socket error!\n");
		}
		if ( m_pollfds[0].revents & POLLIN )		// connection to our main socket?
		{
			for ( int i = 0; i < m_connectionCount; i++ )
			{
				if ( m_connection[i].m_socket == -1 )
				{
					//vsLog("Accepted connection");
					m_connection[i].m_socket = accept( m_socket, (struct sockaddr *)&their_addr, &addr_size );
					if ( m_connection[i].m_socket == -1 )
					{
						perror("accept");
						vsAssert( m_connection[i].m_socket != -1, vsFormatString("Accept error:  See console output for details" ) );
					}

					fcntl(m_connection[i].m_socket, F_SETFL, O_NONBLOCK);
					m_connection[i].m_receiveBuffer = new vsStore(20 * 1024);
					m_connection[i].m_sendBuffer = new vsStore(600 * 1024);
					m_connection[i].m_closing = false;
					m_connection[i].m_sigHup = false;

					if ( m_listener )
					{
						m_listener->NewConnection( &m_connection[i] );
					}
					break;
				}
			}
		}

		for ( int p = 1; p < pollCount; p++ )
		{
			if ( m_pollfds[p].revents )
			{
				int socket = m_pollfds[p].fd;

				// figure out which connection this is.
				vsTCPConnection *connection = NULL;
				int connectionID = 0;
				for ( int i = 0; i < m_connectionCount; i++ )
				{
					if ( m_connection[i].m_socket == m_pollfds[p].fd )
					{
						connection = &m_connection[i];
						connectionID = i;
						break;
					}
				}

				vsAssert(connection != NULL, "Error:  Couldn't find connection to go with poll results??");

				if ( m_pollfds[p].revents & POLLIN )
				{
					char buffer[30 * 1024];
					// received data on this socket!
					int nb = 0;
					do
					{
						int nb = recv( socket, buffer, (30 * 1024)-1, 0 );
						//buffer[nb] = 0;
						//vsLog("%s",buffer);
						if ( nb > 0 )
						{
							connection->m_receiveBuffer->WriteBuffer( buffer, nb );

							if ( m_listener )
							{
								m_listener->HandleBuffer( connection, connection->m_receiveBuffer );
							}
						}
					}
					while( nb > 0 );

					if ( connection->m_receiveBuffer->BytesLeftForReading() )
					{
						vsLog("vsSocketTCP Socket %d:  bytes left for reading.", socket);
					}
				}
				if ( m_pollfds[p].revents & POLLOUT )
				{
					// something to send?
					int bytesToSend = connection->m_sendBuffer->BytesLeftForReading();
					if ( bytesToSend > 0 )
					{
						int nb = send( socket, connection->m_sendBuffer->GetReadHead(), bytesToSend, 0 );

						if ( nb == bytesToSend )
						{
							connection->m_sendBuffer->Clear();
							//vsLog("Completed send buffer %d, socket %d", connectionID, socket);
						}
						else
						{
							connection->m_sendBuffer->AdvanceReadHead( nb );
							vsLog("Partial send buffer %d, socket %d", connectionID, socket);
						}
					}
				}
				if ( m_pollfds[p].revents & POLLHUP )
				{
					connection->m_sigHup = true;
					connection->m_closing = true;
				}
			}
		}
	}

#endif // _WIN32

	m_inUpdate = false;

	for ( int i = 0; i < m_connectionCount; i++ )
	{
		if ( m_connection[i].m_socket != -1 )
		{
			if ( m_connection[i].m_closing )
			{
				bool closeNow = (m_connection[i].m_sigHup);

				if ( (m_connection[i].m_sendBuffer->BytesLeftForReading() == 0) &&
					(m_connection[i].m_receiveBuffer->BytesLeftForReading() == 0) )
				{
					closeNow = true;
				}

				if ( closeNow )	// either we've received a Hup, or we're marked as closing and we have nothing left to send or receive.
				{
					//vsLog("Closing connection %d", i);
					if ( m_listener )
					{
						m_listener->ConnectionClosed( &m_connection[i] );
					}
					// other end of connection closed.  Kill this connection.
#ifdef _WIN32
					closesocket( m_connection[i].m_socket );
#else
					close( m_connection[i].m_socket );
#endif
					m_connection[i].m_socket = -1;
					vsDelete( m_connection[i].m_receiveBuffer );
					vsDelete( m_connection[i].m_sendBuffer );
				}
			}
		}
	}
}

int
vsSocketTCP::GetConnectionCount()
{
	int count = 0;
	for ( int i = 0; i < m_connectionCount; i++ )
	{
		if ( m_connection[i].m_socket != -1 )
		{
			if ( m_connection[i].m_closing == false )	// don't count it if we're about to close this connection.
			{
				count++;
			}
		}
	}
	return count;
}

void
vsSocketTCP::Close( vsTCPConnection *connection )
{
	//if ( m_inUpdate )
	{
		connection->m_closing = true;
	}
	/*else
	{
		if ( m_listener )
		{
			m_listener->ConnectionClosed( connection );
		}

		// close immediately.
		close( connection->m_socket );
		connection->m_socket = -1;
		vsDelete( connection->m_receiveBuffer );
		vsDelete( connection->m_sendBuffer );
	}*/
}

