/*
 *  VS_MaterialManager.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MATERIAL_MANAGER_H
#define VS_MATERIAL_MANAGER_H

#include "VS_Cache.h"
#include "VS_MaterialInternal.h"


class vsMaterialManager : public vsCache<vsMaterialInternal>
{
public:
	vsMaterialManager();
	virtual ~vsMaterialManager();
	
	vsMaterialInternal *	LoadMaterial( const vsString &name );
};


#endif // VS_MATERIAL_MANAGER_H

