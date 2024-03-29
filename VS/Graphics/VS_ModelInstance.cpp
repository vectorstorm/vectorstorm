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

vsModelInstance::vsModelInstance():
	group(nullptr),
	lodLevel(0),
	visible(false)
{
}

vsModelInstance::~vsModelInstance()
{
	if ( group )
		group->RemoveInstance(this);
}

void
vsModelInstance::UpdateGroup()
{
	if ( group )
		group->UpdateInstance( this, visible );
}

void
vsModelInstance::SetVisible( bool v )
{
	if ( visible != v )
	{
		visible = v;
		UpdateGroup();
	}
}

void
vsModelInstance::SetMatrix( const vsMatrix4x4& mat )
{
	SetMatrix( mat, c_white );
}

void
vsModelInstance::SetColor( const vsColor &color )
{
	SetMatrix( matrix, color );
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
			UpdateGroup();
		}
	}
}

void
vsModelInstance::SetLodLevel( size_t lodLevel_in )
{
	if ( lodLevel_in != lodLevel )
	{
		lodLevel = lodLevel_in;
		UpdateGroup();
	}
}

int
vsModelInstance::GetLodCount()
{
	return GetModel()->GetLodCount();
}

vsModel *
vsModelInstance::GetModel()
{
	return GetInstanceGroup()->GetModel();
}

