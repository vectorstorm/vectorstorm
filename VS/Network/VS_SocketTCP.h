/*
 *  VS_SocketTCP.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 5/06/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SOCKET_TCP_H
#define VS_SOCKET_TCP_H

class vsStore;
class vsNetClient;

struct vsTCPConnection
{
	int32_t		m_socket;			// -1 if not currently in use
	vsStore	*m_receiveBuffer;	// storage space for reassembling the stream.
	vsStore	*m_sendBuffer;		// storage space for reassembling the stream.

	bool		m_closing;			// when true, we'll try to close this connection once send and receive buffers are empty.
	bool		m_sigHup;			// when true, we've received a hup, and this connection must be closed immediately.
};


class vsTCPListener
{
public:

	vsTCPListener() {}
	virtual ~vsTCPListener() {}

	virtual void	NewConnection( vsTCPConnection *connection ) {}
	virtual void	ConnectionClosed( vsTCPConnection *connection ) {}
	virtual void	HandleBuffer( vsTCPConnection *connection, vsStore *packet ) {}
};


class vsSocketTCP
{
	uint32_t		m_privateIP;		// stored in NETWORK BYTE ORDER
	uint16_t		m_privatePort;

	int32_t		m_socket;
	bool		m_listening;

	bool		m_inUpdate;

	vsTCPListener *		m_listener;

	vsTCPConnection *	m_connection;
	int					m_connectionCount;
#ifndef _WIN32
	struct pollfd *		m_pollfds;
#endif

public:

	enum Type
	{
		Type_Global,		// listen for any connections
		Type_Localhost		// only listen for connections from localhost
	};

	vsSocketTCP( int maxConnectionCount );
	~vsSocketTCP();

	void	SetListener( vsTCPListener * l ) { m_listener = l; }

	bool	Listen( uint16_t port = 0, Type t = Type_Global );	// create a socket.
	bool	IsListening() { return m_listening; }
	void	Update( float timeStep );

	int		GetConnectionCount();

	void	Send( vsTCPConnection *to, vsStore *packet );
	void	Close( vsTCPConnection *to );						// close this connection (will try to finish any queued sending first, though)
};

#endif // VS_SOCKET_TCP_H

