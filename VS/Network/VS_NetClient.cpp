/*
 *  VS_NetClient.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 26/04/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_NetClient.h"

#if defined(_WIN32)
#include <WinSock.h>
#else
#include <netdb.h>
#endif

vsNetClient::vsNetClient(uint32 addr, uint16 port)
{
	m_publicIP = addr;
	m_publicPort = port;
}

vsNetClient::vsNetClient(const vsString &address, uint16 port)
{
	hostent *h = gethostbyname( address.c_str() );
    if (h == NULL) 
	{
#if !defined(_WIN32)	// todo:  Network errors under WinSock!
		herror("gethostbyname");
#endif
		vsAssert( h != NULL, vsFormatString("Gethostbyname error:  See console output for details" ) );
	}
	
	m_publicIP = ((struct in_addr *)h->h_addr)->s_addr;
	m_publicPort = port;
}


