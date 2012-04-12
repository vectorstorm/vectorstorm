/*
 *  VS_HashTable.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 22/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_HashTable.h"

// The following hash algorithm is adapted from code by Paul Hsieh,
// which is available from http://www.azillionmonkeys.com/qed/hash.html
// There, this algorithm is referred to as "SuperFastHash".
//
// Only very slight modifications have been made here, to match the local
// coding conventions and using our own internal integer types.


#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16 *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)+(uint32_t)(((const uint8_t *)(d))[0]) )
#endif


uint32 vsCalculateHash(const char * data, uint32 len) {
	uint32 hash = len, tmp;
	int rem;
	
    if (len <= 0 || data == NULL) 
	{
		return 0;
	}
	
    rem = len & 3;
    len >>= 2;
	
    /* Main loop */
    for (;len > 0; len--) 
	{
        hash  += get16bits(data);
        tmp    = (get16bits(data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof(uint16);
        hash  += hash >> 11;
    }
	
    /* Handle end cases */
    switch (rem) 
	{
        case 3: hash += get16bits(data);
			hash ^= hash << 16;
			hash ^= data[sizeof (uint16)] << 18;
			hash += hash >> 11;
			break;
        case 2: hash += get16bits (data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
        case 1: hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
    }
	
    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
	
    return hash;
}

