/*
 *  VS_Octree.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 19/11/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Octree.h"

#include "VS/Graphics/VS_Camera.h"
#include "VS/Graphics/VS_Model.h"

#include <list>

struct vsOctreeModelInfo
{
	vsModel *	m_model;
	int			m_nodeId;
	
	vsOctreeModelInfo() :
		m_model(NULL),
		m_nodeId(-1)
	{
	}
	
	~vsOctreeModelInfo()
	{
	}
};

struct vsOctreeNode
{
	vsBox3D		m_bounds;
	int			m_nodeId;
	int			m_parentNodeId;
	
	std::list<vsOctreeModelInfo*>	m_infoList;
	
	int			m_childId[8];
	
	~vsOctreeNode()
	{
		while ( m_infoList.begin() != m_infoList.end() )
		{
			vsOctreeModelInfo *info = *m_infoList.begin();
			
			m_infoList.remove(info);
			
			vsDelete(info);
		}
	}
};

vsOctree::vsOctree( const vsBox3D &area, int levels )
{
	int nodeCount = 1;
	int nodesThisLevel = 1;
	for ( int i = 0; i < levels; i++ )
	{
		nodesThisLevel *= 8;
		nodeCount += nodesThisLevel;
	}
	
	m_nodeCount = nodeCount;
	m_node = new vsOctreeNode[m_nodeCount];
	
	m_nextNodeToInit = 0;
	
	InitNode( area, levels, -1 );
}

vsOctree::~vsOctree()
{
	vsDeleteArray( m_node );
}

void
vsOctree::InitNode( const vsBox3D &bounds, int levelsToRecurse, int parentId )
{
	vsAssert( m_nextNodeToInit < m_nodeCount, "Octree setup error" );
	vsOctreeNode *node = &m_node[ m_nextNodeToInit ];
	
	node->m_bounds = bounds;
	node->m_nodeId = m_nextNodeToInit;
	node->m_parentNodeId = parentId;
	
	m_nextNodeToInit++;
	
	if ( levelsToRecurse == 0 )
	{
		for ( int i = 0; i < 8; i++ )
		{
			node->m_childId[i] = -1;
		}
	}
	else
	{
		// THIS IS LOGIC IS WRONG FOR MOST CASES!  FIX ME BEFORE COMMITTING BACK TO TRUNK!
		// (Check the implementation of the mmoQuadTreeStore class for a correct impl)
		vsBox3D childBounds = bounds * 0.5f;	// each level's nodes are half the size of the parent nodes!
		
		float halfWidth = childBounds.Width() * 0.5f;
		float halfHeight = childBounds.Height() * 0.5f;
		float halfDepth = childBounds.Depth() * 0.5f;
		
		
		vsVector3D offset[8];
		
		for ( int i = 0; i < 8; i++ )
		{
			if ( i & 0x1 )
			{
				offset[i].x += halfWidth;
			}
			else
			{
				offset[i].x -= halfWidth;
			}

			if ( i & 0x2 )
			{
				offset[i].y += halfHeight;
			}
			else
			{
				offset[i].y -= halfHeight;
			}

			if ( i & 0x4 )
			{
				offset[i].z += halfDepth;
			}
			else
			{
				offset[i].z -= halfDepth;
			}
		}

		for ( int i = 0; i < 8; i++ )
		{
			node->m_childId[i] = m_nextNodeToInit;
			InitNode( childBounds + offset[i], levelsToRecurse-1, node->m_nodeId );
		}
	}
}



void
vsOctree::Draw( const vsCamera3D *camera, vsRenderQueue *queue )
{
	m_nodesDrawn = 0;
	DrawNode( 0, camera, queue );
}

void
vsOctree::DrawNode( int nodeId, const vsCamera3D *camera, vsRenderQueue *queue )
{
	vsAssert( nodeId >= 0 && nodeId < m_nodeCount, "Octree draw error" );
	vsOctreeNode *node = &m_node[ nodeId ];
	
	if ( camera->IsBox3DVisible( node->m_bounds ) )
	{
		// draw all our contents.
		for( std::list<vsOctreeModelInfo*>::iterator i = node->m_infoList.begin(); i != node->m_infoList.end(); i++ )
		{
			vsOctreeModelInfo *info = *i;
			
			info->m_model->Draw(queue);
		}
		
		
		m_nodesDrawn++;
		
		for ( int i = 0; i < 8; i++ )
		{
			if ( node->m_childId[i] > 0 )
			{
				DrawNode( node->m_childId[i], camera, queue );
			}
		}
	}
}

void
vsOctree::InsertInfoAtNode( vsOctreeModelInfo *info, int nodeId )
{
	vsBox3D modelBounds = info->m_model->GetBoundingBox() + info->m_model->GetPosition();
	vsAssert(nodeId >= 0 && nodeId < m_nodeCount, "Illegal octree node insertion");
	vsOctreeNode *node = &m_node[nodeId];
	
	for ( int i = 0; i < 8; i++ )
	{
		if ( node->m_childId[i] > 0 )
		{
			vsOctreeNode *childNode = &m_node[ node->m_childId[i] ];
			
			if ( childNode->m_bounds.EncompassesBox( modelBounds ) )
			{
				InsertInfoAtNode( info, node->m_childId[i] );
				return;
			}
		}
	}
	
	info->m_nodeId = nodeId;
	node->m_infoList.push_back(info);
}

vsOctreeModelInfo *
vsOctree::AddModel( vsModel *model )
{
	vsOctreeModelInfo *info = new vsOctreeModelInfo;
	info->m_model = model;
	
	InsertInfoAtNode( info, 0 );
	
	return info;
}

void
vsOctree::UpdateModel( vsOctreeModelInfo *info )
{
	vsAssert( info->m_nodeId >= 0 && info->m_nodeId < m_nodeCount, "Illegal nodeId set on octree info object??" );
	vsOctreeNode *node = &m_node[info->m_nodeId];
	vsBox3D modelBounds = info->m_model->GetBoundingBox() + info->m_model->GetPosition();
	node->m_infoList.remove(info);

	if ( node->m_bounds.EncompassesBox( modelBounds ) )
	{
		InsertInfoAtNode( info, info->m_nodeId );
	}
	else
	{
		InsertInfoAtNode( info, 0 );
	}
}

void
vsOctree::RemoveModel( vsOctreeModelInfo *info )
{
	vsAssert( info->m_nodeId >= 0 && info->m_nodeId < m_nodeCount, "Illegal nodeId set on octree info object??" );
	vsOctreeNode *node = &m_node[info->m_nodeId];
	
	node->m_infoList.remove(info);
	
	vsDelete(info);
}

