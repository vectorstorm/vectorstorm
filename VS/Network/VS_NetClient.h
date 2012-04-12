/*
 *  VS_NetClient.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 26/04/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_NETCLIENT_H
#define VS_NETCLIENT_H

class vsNetClient
{
	uint32		m_publicIP;
	uint32		m_privateIP;
	
	uint16		m_publicPort;
	uint16		m_privatePort;
public:
	
	vsNetClient(uint32 addr, uint16 port);
	vsNetClient(const vsString &address, uint16 port);
	
	uint32		GetIP() { return m_publicIP; }
	uint16		GetPort() { return m_publicPort; }
};

#endif // VS_NETCLIENT_H

