/*
 *  VS_Socket.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/04/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SOCKET_H
#define VS_SOCKET_H

class vsStore;
class vsNetClient;

class vsSocketListener
{
public:

	vsSocketListener() {}
	virtual ~vsSocketListener() {}

	virtual bool	HandlePacket( vsNetClient *from, vsStore *packet ) { UNUSED(from); UNUSED(packet); return false; }
};

class vsSocket
{
	uint32_t		m_privateIP;
	uint32_t		m_publicIP;
	int			m_privatePort;
	int			m_publicPort;

	int			m_socket;

	vsSocketListener *		m_listener;

public:

				vsSocket(int port = 0);		// a port of 0 means that we don't care what port we're using.  This is the preferred usage, as it will allow multiple instances to run on one computer!
	virtual		~vsSocket();

	void		SetListener( vsSocketListener *listener );

	void		SendTo( vsNetClient *to, vsStore *packet );

	void		Poll();		// call once per frame to check for any activity on the socket.
};


#endif // VS_SOCKET_H

