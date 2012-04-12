/*
 *  VS_PointOctree.h
 *  MMORPG2
 *  
 *  Created by Trevor Powell on 08-10-2011.
 *  Copyright 2011 Trevor Powell.  All rights reserved.
 *
 */	

#ifndef VS_POINTOCTREE_H
#define VS_POINTOCTREE_H



#include "VS/Graphics/VS_Model.h"
#include "VS/Math/VS_Box.h"
#include "VS/Utils/VS_Array.h"

class vsCamera3D;

struct vsPointOctreeElement
{
	vsVector3D position;
};

template<typename T>
class vsPointOctree
{
	struct Node
	{
		vsBox3D		bounds;
		vsVector3D	middle;
		Node*		child[8];
		vsArray<T*> contents;
		bool		leaf;

		Node( const vsBox3D &bounds ):
			bounds(bounds),
			middle(bounds.Middle()),
			contents(),
			leaf(true)
		{
			for ( int i = 0; i < 8; i++ )
			{
				child[i] = NULL;
			}
		}

		~Node()
		{
			for ( int i = 0; i < 8; i++ )
			{
				vsDelete(child[i]);
			}
		}
	};

	Node 		m_node;
	int			m_maxRecursion;
	int			m_maxItemsPerNode;

	void RecursiveAddPoint( int recursionLevel, T* point, Node* node )
	{
		if ( node->leaf )
		{
			if ( recursionLevel >= m_maxRecursion || node->contents.ItemCount() < m_maxItemsPerNode )
			{
				node->contents.AddItem( point );
				return;
			}
			else
			{
				// too many items in this node.  Subdivide!

				Subdivide( node );
			}
		}

		vsAssert( !node->leaf, "PointOctree error" );
		int octant = PickOctant( point->position, node );

		RecursiveAddPoint( recursionLevel + 1, point, node->child[octant] );
	}

	void RecursiveFindPointsWithin( Node *node, vsArray<T*> *result, const vsVector3D &position, float distance )
	{
		if ( node->leaf )
		{
			float sqDistance = distance * distance;
			for (int i = 0; i < node->contents.ItemCount(); ++i)
			{
				if ( ( node->contents[i]->position - position ).SqLength() < sqDistance )
				{
					result->AddItem( node->contents[i] );
				}
			}
		}
		else
		{
			vsAssert( node->contents.ItemCount() == 0, "PointOctree error?" );

			for (size_t i = 0; i < 8; ++i)
			{
				if ( node->child[i]->bounds.IntersectsSphere(position, distance) )
				{
					RecursiveFindPointsWithin( node->child[i], result, position, distance );
				}
			}
		}
	}

	void Subdivide( Node* node )
	{
		node->leaf = false;
		for ( int i = 0; i < 8; i++ )
		{
			vsVector3D corner = node->bounds.Corner(i);
			vsBox3D box;
			box.ExpandToInclude(corner);
			box.ExpandToInclude(node->middle);
			int octant = PickOctant( corner, node );
			node->child[octant] = new Node( box );
		}

		for( int i = 0; i < node->contents.ItemCount(); i++ )
		{
			RecursiveAddPoint( 0, node->contents.GetItem(i), node );
		}
		node->contents.Clear();
	}
	
	int PickOctant( const vsVector3D &position, Node* node )
	{
		vsAssert( !node->leaf, "PointOctree error" );

		vsVector3D delta = node->middle - position;
		int octant = 0;
		if (delta.x < 0.f)
			octant |= 0x1;
		if (delta.y < 0.f)
			octant |= 0x2;
		if (delta.z < 0.f)
			octant |= 0x4;

		return octant;
	}

public:
	vsPointOctree( const vsBox3D &bounds, int maxItemsPerNode ):
		m_node( bounds ),
		m_maxRecursion(10),
		m_maxItemsPerNode( maxItemsPerNode )
	{
	}
	~vsPointOctree()
	{
	}

	void AddPoint( T *point )
	{
		RecursiveAddPoint( 0, point, &m_node );
	}

	void FindPointsWithin( vsArray<T*> *result, const vsVector3D &position, float distance )
	{
		RecursiveFindPointsWithin( &m_node, result, position, distance );
	}
};

#endif /* VS_POINTOCTREE_H */

