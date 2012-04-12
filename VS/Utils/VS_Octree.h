/*
 *  VS_Octree.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 19/11/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_OCTREE_H
#define VS_OCTREE_H

#include "VS/Graphics/VS_Model.h"
#include "VS/Math/VS_Box.h"

class vsCamera3D;
struct vsOctreeNode;
struct vsOctreeModelInfo;

class vsOctree
{
	vsOctreeNode *	m_node;
	int				m_nodeCount;
	
	int				m_nodesDrawn;
	
	int				m_nextNodeToInit;
	
	void			InitNode( const vsBox3D &bounds, int levelsToRecurse, int parentId );
	void			DrawNode( int nodeId, const vsCamera3D *camera, vsRenderQueue *queue );
	
	void			InsertInfoAtNode( vsOctreeModelInfo *info, int nodeId );
	
public:
					vsOctree( const vsBox3D &area, int levels );
					~vsOctree();
	
	vsOctreeModelInfo *	AddModel( vsModel *model );
	void				UpdateModel( vsOctreeModelInfo *info );
	void				RemoveModel( vsOctreeModelInfo *info );
	
	void			Draw( const vsCamera3D *camera, vsRenderQueue *queue );
};


#endif // VS_OCTREE_H

