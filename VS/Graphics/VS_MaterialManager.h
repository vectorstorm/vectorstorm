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

#include "VS/Utils/VS_Cache.h"
#include "VS/Graphics/VS_MaterialInternal.h"


class vsMaterialManager : public vsCache<vsMaterialInternal>
{
public:
	vsMaterialManager();
	virtual ~vsMaterialManager();

	vsMaterialInternal *	LoadMaterial( const vsString &name );

	void ReloadAll();
};


#endif // VS_MATERIAL_MANAGER_H

