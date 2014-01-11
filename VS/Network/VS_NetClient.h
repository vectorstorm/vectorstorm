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
	uint32_t		m_privateIP;
	uint16_t		m_privatePort;

	// For eventual later use, if I ever do a serious network game.
	//uint32_t		m_publicIP;
	//uint16_t		m_publicPort;
public:

	vsNetClient(uint32_t addr, uint16_t port);
	vsNetClient(const vsString &address, uint16_t port);

	uint32_t		GetIP() { return m_privateIP; }
	uint16_t		GetPort() { return m_privatePort; }
};

#endif // VS_NETCLIENT_H

