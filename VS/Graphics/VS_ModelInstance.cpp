/*
 *  VS_ModelInstance.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 21/03/2016
 *  Copyright 2016 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_ModelInstance.h"
#include "VS_ModelInstanceGroup.h"
#include "VS_Model.h"

vsModelInstance::~vsModelInstance()
{
	group->RemoveInstance(this);
}

void
vsModelInstance::SetVisible( bool v )
{
	if ( visible != v )
	{
		visible = v;
		group->UpdateInstance( this, visible );
	}
}

void
vsModelInstance::SetMatrix( const vsMatrix4x4& mat )
{
	SetMatrix( mat, c_white );
}

void
vsModelInstance::SetMatrix( const vsMatrix4x4& mat, const vsColor &c )
{
	if ( matrix != mat || color != c)
	{
		matrix = mat;
		color = c;
		if ( visible )
		{
			group->UpdateInstance( this, visible );
		}
	}
}

vsModel *
vsModelInstance::GetModel()
{
	return GetInstanceGroup()->GetModel();
}

