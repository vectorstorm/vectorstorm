/*
 *  VS_SocketTCP.cpp
 *  Vectorstorm
 *
 *  Created by Trevor Powell on 5/06/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_SocketTCP.h"
#include "VS_Store.h"

#include <stdio.h>
#include <fcntl.h>


#if defined(_WIN32)
#include <WinSock2.h>
typedef int socklen_t;	// yay, standards
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#endif
#include <sys/types.h>

#include <assert.h>

#ifdef _WIN32
#define USE_SELECT
#else
using std::max;
#define USE_POLL
#endif

#define vsLog printf

vsSocketTCP::vsSocketTCP( int maxConnectionCount ):
	m_privateIP(0),
	m_privatePort(0),
	m_listenSocket(-1),
	m_listening(false),
	m_listener(NULL),
	m_connection(NULL),
	m_connectionCount(maxConnectionCount)
{
	m_connection = new vsTCPConnection[ maxConnectionCount ];
#if defined(USE_POLL)
	m_pollfds = new pollfd[ m_connectionCount+1 ];
#endif // USE_POLL

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
	if ( m_listenSocket > -1 )
	{
#ifdef _WIN32
		closesocket( m_listenSocket );
#else
		close( m_listenSocket );
#endif
	}

	for ( int i = 0; i < m_connectionCount; i++ )
	{
		if ( m_connection[i].m_socket != -1 )
		{
#ifdef _WIN32
			closesocket( m_connection[i].m_socket );
#else
			close( m_connection[i].m_socket );
#endif
		}
		delete m_connection[i].m_receiveBuffer;
		delete m_connection[i].m_sendBuffer;
	}
#ifdef USE_POLL
	delete [] m_pollfds;
#endif

	delete [] m_connection;
}

bool
vsSocketTCP::Listen( uint16_t port, vsSocketTCP::Type t )
{
	if ( m_listenSocket == -1 )
	{
		//vsLog("Opening socket");

		m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if ( m_listenSocket == -1 )
		{
			perror("socket");
			vsAssert( m_listenSocket != -1, vsFormatString("Socket error:  See console output for details" ) );
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

	int err = bind( m_listenSocket, (sockaddr *)&addr, sizeof(addr) );
#ifndef _WIN32
	fcntl(m_listenSocket, F_SETFL, O_NONBLOCK);
#endif // _WIN32

	if ( err == -1 )
	{
		perror("bind");
		return false;
	}

	err = listen( m_listenSocket, m_connectionCount );
	m_listening = true;
	if ( err == -1 )
	{
		perror("listen");
		vsAssert( err != -1, vsFormatString("Listen error:  See console output for details" ) );
	}

	socklen_t addrSize = sizeof(addr);
	err = getsockname(m_listenSocket, (sockaddr*)&addr, &addrSize);
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
	return true;
}

void
vsSocketTCP::Send( vsTCPConnection *to, vsStore *packet )
{
	DoSend(to,packet);
}

void
vsSocketTCP::DoSend( vsTCPConnection *to, vsStore *packet )
{
	// check if we're feeling backed up.  If so, just queue ourselves for
	// later transmission.
	int pendingBytesToSend = to->m_sendBuffer->BytesLeftForReading();
	if ( pendingBytesToSend )
	{
		to->m_sendBuffer->Append(packet);
	}
	else
	{
		// nothing backed up, so send immediately!
		int bytesToSend = packet->BytesLeftForReading();
#if defined(_WIN32)
			// Microsoft things that 'send()' sends a char array, rather than a
			// blob of memory addressed by a void pointer.  That's adorable.
			size_t nb = send( to->m_socket, (char*)packet->GetReadHead(), bytesToSend, 0 );
#else
			size_t nb = send( to->m_socket, packet->GetReadHead(), bytesToSend, 0 );
#endif
			if ( nb < bytesToSend )
			{
				packet->SeekReadHeadTo(nb);
				to->m_sendBuffer->WriteBuffer(packet->GetReadHead(), packet->BytesLeftForReading());
			}
	}
}

#if defined(USE_POLL)
void
vsSocketTCP::DoPoll(float maxSleepDuration)
{
	struct sockaddr_storage their_addr;
	struct timeval tv;
	socklen_t addr_size;
	int pollCount = 1;
	addr_size = sizeof their_addr;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	m_pollfds[0].fd = m_listenSocket;
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

	if ( poll(m_pollfds, pollCount, 1000*maxSleepDuration) > 0 )
	{
		if ( m_pollfds[0].revents & POLLERR || m_pollfds[0].revents & POLLHUP || m_pollfds[0].revents & POLLNVAL )
		{
			vsLog("Socket error!");
		}
		if ( m_pollfds[0].revents & POLLIN )		// connection to our main socket?
		{
			for ( int i = 0; i < m_connectionCount; i++ )
			{
				if ( m_connection[i].m_socket == -1 )
				{
					//vsLog("Accepted connection");
					m_connection[i].m_socket = accept( m_listenSocket, (struct sockaddr *)&their_addr, &addr_size );
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
						ssize_t nb = recv( socket, buffer, (30 * 1024)-1, 0 );
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
					size_t bytesToSend = connection->m_sendBuffer->BytesLeftForReading();
					if ( bytesToSend > 0 )
					{
						ssize_t nb = send( socket, connection->m_sendBuffer->GetReadHead(), bytesToSend, 0 );

						if ( nb == (ssize_t)bytesToSend )
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
}
#elif defined(USE_SELECT)
void
vsSocketTCP::DoSelect(float maxSleepDuration)
{
	fd_set rsocks, wsocks;
	socktype_t highsock = m_listenSocket;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;

	addr_size = sizeof their_addr;

	FD_ZERO(&rsocks);
	FD_ZERO(&wsocks);
	if ( m_listenSocket != -1 )
	{
		FD_SET(m_listenSocket, &rsocks);
	}
	for ( int i = 0; i < m_connectionCount; i++ )
	{
		if ( m_connection[i].m_socket != -1 )
		{
			FD_SET(m_connection[i].m_socket, &rsocks);
			size_t bytesToSend = m_connection[i].m_sendBuffer->BytesLeftForReading();
			if ( bytesToSend > 0 )
			{
				FD_SET(m_connection[i].m_socket, &wsocks);
			}

			highsock = vsMax(highsock, m_connection[i].m_socket);
		}
	}
	struct timeval timeout;
	timeout.tv_sec = (int)maxSleepDuration;
	timeout.tv_usec = (int)((1000000 * maxSleepDuration) - (1000000 * timeout.tv_sec));
	int readSocks = select(highsock+1, &rsocks, &wsocks, NULL, &timeout);
	if ( readSocks == -1 )
	{
		// error!
		extern int errno;
		printf("Error: %d", errno);
	}
	if ( readSocks > 0 )
	{
		if ( m_listenSocket != -1 && FD_ISSET(m_listenSocket, &rsocks) )
		{
			for ( int i = 0; i < m_connectionCount; i++ )
			{
				if ( m_connection[i].m_socket == -1 )
				{
					//vsLog("Accepted connection");
					m_connection[i].m_socket = accept( m_listenSocket, (struct sockaddr *)&their_addr, &addr_size );
					if ( m_connection[i].m_socket == -1 )
					{
						perror("accept");
						vsAssert( m_connection[i].m_socket != -1, vsFormatString("Accept error:  See console output for details" ) );
					}
#ifndef _WIN32
					fcntl(m_connection[i].m_socket, F_SETFL, O_NONBLOCK);
#endif
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

		for ( int i = 0; i < m_connectionCount; i++ )
		{
			if ( m_connection[i].m_socket == -1 )
				continue;	// skip this one.
			if ( FD_ISSET(m_connection[i].m_socket, &rsocks) )
			{
				vsTCPConnection* connection = &m_connection[i];
				//int connectionID = i;
				socktype_t socket = connection->m_socket;
				char buffer[30 * 1024];
				// received data on this socket!
				int nb = 0;
				do
				{
					size_t nb = recv( socket, buffer, (30 * 1024)-1, 0 );
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
					else
					{
						// a receive of '0' indicates that this client disconnected.
						m_listener->ConnectionClosed( connection );
						connection->m_socket = -1;
					}
				}
				while( nb > 0 );
			}
			if ( m_connection[i].m_socket != -1 && FD_ISSET(m_connection[i].m_socket, &wsocks) )
			{
				vsTCPConnection* connection = &m_connection[i];
				int connectionID = i;
				socktype_t socket = connection->m_socket;

				// something to send?
				size_t bytesToSend = connection->m_sendBuffer->BytesLeftForReading();
				if ( bytesToSend > 0 )
				{
					size_t nb = send( socket, connection->m_sendBuffer->GetReadHead(), bytesToSend, 0 );

					if ( nb == (size_t)bytesToSend )
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
		}
	}

}
#endif

void
vsSocketTCP::Poll(float maxSleepDuration)
{
#if defined(USE_POLL)
	DoPoll(maxSleepDuration);
#elif defined(USE_SELECT)
	DoSelect(maxSleepDuration);
#endif

	// handle closing any connections which are ready to close
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
					delete m_connection[i].m_receiveBuffer;
					m_connection[i].m_receiveBuffer = NULL;
					delete m_connection[i].m_sendBuffer;
					m_connection[i].m_sendBuffer = NULL;
				}
			}
		}
	}
}

vsTCPConnection*
vsSocketTCP::Connect(const std::string& hostname, uint16_t port)
{
	sockaddr_in addr;
	socklen_t addr_len;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	struct hostent* hp = gethostbyname(hostname.c_str());
	if ( hp == NULL ||  hp->h_addr_list[0] == NULL )
	{
		return NULL;
	}
	addr.sin_addr = *( struct in_addr*)hp->h_addr_list[0];
	memset(addr.sin_zero, '\0', sizeof addr.sin_zero);
	addr_len = sizeof(addr);
	socktype_t sock = -1;
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if ( sock == -1 )
		{
			perror("socket");
			vsAssert( sock != -1, vsFormatString("Socket error:  See console output for details" ) );
		}
	}
	int err = connect( sock, (struct sockaddr *)&addr, addr_len );
	if ( err == 0 )
	{
#ifndef _WIN32
		fcntl(sock, F_SETFL, O_NONBLOCK);
#endif // _WIN32
		m_connection[0].m_receiveBuffer = new vsStore(20 * 1024);
		m_connection[0].m_sendBuffer = new vsStore(600 * 1024);
		m_connection[0].m_closing = false;
		m_connection[0].m_sigHup = false;
		m_connection[0].m_socket = sock;

		if ( m_listener )
		{
			m_listener->NewConnection( &m_connection[0] );
		}
		return &m_connection[0];
	}
	return NULL;
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
	connection->m_closing = true;
}

