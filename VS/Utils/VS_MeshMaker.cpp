/*
 *  VS_MeshMaker.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 5/04/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_MeshMaker.h"

#include "VS_Mesh.h"

#include "VS_Box.h"

#include "VS_DisableDebugNew.h"
#include <vector>
#include <list>
#include "VS_EnableDebugNew.h"


#define MAX_MESH_MAKER_MATERIALS (10)
#define MAX_MESH_MAKER_STRIPS (1000)

static float s_mergeTolerance = 0.4f;
//static float s_splitFactor = 0.707f;


class vsMeshMakerTriangleEdge
{
public:

	vsMeshMakerTriangle *	m_aTriangle;
	vsMeshMakerTriangle *	m_bTriangle;

	vsMeshMakerTriangleVertex *	m_aVertex;
	vsMeshMakerTriangleVertex *	m_bVertex;

	vsMeshMakerTriangleEdge()
	{
		m_aTriangle = m_bTriangle = nullptr;
		m_aVertex = m_bVertex = nullptr;
	}
};

struct vsMeshMaker::InternalData
{
	vsMesh *	m_mesh;

	vsMaterial							*m_material[MAX_MESH_MAKER_MATERIALS];
	std::vector<vsMeshMakerTriangle>		m_materialTriangle[MAX_MESH_MAKER_MATERIALS];
	int									m_materialCount;

	std::vector<vsMeshMakerTriangleEdge>	m_triangleEdge;
};



vsMeshMakerTriangleVertex::vsMeshMakerTriangleVertex()
{
	m_flags = 0L;
	m_mergeCount = 0;
	m_fakeNormalMergedWith = nullptr;

	//m_triangleCount = 0;
	m_firstTriangle = nullptr;
}

void
vsMeshMakerTriangleVertex::AddTriangle( vsMeshMakerTriangle *triangle )
{
	/*vsAssert( m_triangleCount < MAX_TRIANGLES_PER_VERTEX, "Ran out of triangle storage" );

	m_triangle[ m_triangleCount++ ] = triangle;*/

	if ( m_firstTriangle == nullptr )
	{
		m_firstTriangle = triangle;
	}
}

void
vsMeshMakerTriangleVertex::SetPosition( const vsVector3D &pos )
{
	position = pos;
	m_position = pos;
	m_flags |= Flag_Position;
}

void
vsMeshMakerTriangleVertex::SetNormal( const vsVector3D &normal )
{
	m_normal = normal;
	m_flags |= Flag_Normal;
}

const vsVector3D &
vsMeshMakerTriangleVertex::GetNormal() const
{
	if ( m_fakeNormalMergedWith )
		return m_fakeNormalMergedWith->GetNormal();

	return m_normal;
}

void
vsMeshMakerTriangleVertex::SetColor( const vsColor &color )
{
	m_color = color;
	m_flags |= Flag_Color;
}

void
vsMeshMakerTriangleVertex::SetTexel( const vsVector2D &texel )
{
	m_texel = texel;
	m_flags |= Flag_Texel;
}

float
vsMeshMakerTriangleVertex::GetMergePriorityWith( const vsMeshMakerTriangleVertex &other, const vsVector3D &faceNormalOther )
{
	// no merging if we're a fake vertex.
	if ( m_fakeNormalMergedWith != nullptr )
		return -1.f;

	// check my normal against 'faceNormalOther'
	//vsAssert( m_flags & Flag_Normal, "error:  merging when I have no normal set??" );
	vsAssert( GetFirstTriangle(), "Merging a vertex which doesn't have any triangles??" );

	//if ( m_normal.Dot( faceNormalOther ) > s_mergeTolerance )
	vsVector3D faceNormal = GetFirstTriangle()->m_faceNormal;
	float normalProjection = faceNormal.Dot( faceNormalOther );
	bool colorMatches = (other.m_color == m_color);
	if ( colorMatches && normalProjection > s_mergeTolerance )
	{
		// good enough to merge!
		return normalProjection;
	}

	return -1.f;
}

bool
vsMeshMakerTriangleVertex::AttemptMergeWith( vsMeshMakerTriangleVertex *other, const vsVector3D &faceNormalOther )
{
	const float epsilon = 0.01f;
	const float sqEpsilon = epsilon*epsilon;

	bool closeEnough = ((other->m_position - m_position).SqLength() < sqEpsilon);
	bool colorMatches = (other->m_color == m_color);
	bool texelMatches = ((other->m_texel - m_texel).SqLength() < sqEpsilon);

	//vsVector2D deltaTexel = other->m_texel - m_texel;
	//deltaTexel.x -= vsFloor(deltaTexel.x+0.5f);
	//deltaTexel.y -= vsFloor(deltaTexel.y+0.5f);

	if ( closeEnough && colorMatches ) //&& deltaTexel.SqLength() < sqEpsilon )
	{
		// check my normal against 'faceNormalOther'
		//vsAssert( m_flags & Flag_Normal, "error:  merging when I have no normal set??" );
		vsAssert( GetFirstTriangle(), "Merging a vertex which doesn't have any triangles??" );

		//if ( m_normal.Dot( faceNormalOther ) > s_mergeTolerance )
		vsVector3D faceNormal = GetFirstTriangle()->m_faceNormal;
		if ( faceNormal.Dot( faceNormalOther ) > s_mergeTolerance )
		{
			if ( m_mergeCount == 0 )
			{
				m_totalNormal = GetNormal();	// we keep track of the running total, so we can blend correctly.
			}

			m_mergeCount++;
			m_totalNormal += faceNormalOther;
			vsVector3D newNormal = m_totalNormal;
			newNormal.Normalise();
			SetNormal( newNormal );

			if ( !texelMatches )
			{
				other->SetNormal( newNormal );	// set the normal explicitly,
												// just to force the 'has normal'
												// flag on.

				// make the other vertex fake match me.
				other->m_fakeNormalMergedWith = this;
			}

			return texelMatches;
		}
	}

	vsAssert(false,"Tried to merge things taht can't merge??");
	return false;
}

inline bool
vsMeshMakerTriangleVertex::operator==(const vsMeshMakerTriangleVertex &other) const
{
	if ( m_flags != other.m_flags )
		return false;

	const float epsilon = 0.01f;
	const float sqEpsilon = epsilon*epsilon;

	if ( m_flags & Flag_Position && (other.m_position - m_position).SqLength() > sqEpsilon )
		return false;
	if ( m_flags & Flag_Normal && other.m_normal != m_normal )
		return false;
	if ( m_flags & Flag_Color && other.m_color != m_color )
		return false;
	if ( m_flags & Flag_Texel && other.m_texel != m_texel )
		return false;

	return true;
}

bool
vsMeshMakerTriangle::IsDegenerate() const
{
	// we're degenerate if any two of our vertices have the same position.

	if ( m_vertex[0].GetPosition() == m_vertex[1].GetPosition() ||
		m_vertex[0].GetPosition() == m_vertex[2].GetPosition() ||
		m_vertex[1].GetPosition() == m_vertex[2].GetPosition() )
	{
		return true;
	}
	return false;
}


vsMeshMaker::vsMeshMaker( int flags )
{
	m_octree = nullptr;
	m_buildingNormals = flags & Flag_BuildNormals;
	m_attemptMerge = !(flags & Flag_NoMerge);
	m_triangleCount = 0;
//	m_triangle = new vsMeshMakerTriangle[maxTriangleCount];

	m_internalData = new InternalData;

	for ( int i = 0; i < MAX_MESH_MAKER_MATERIALS; i++ )
	{
		m_internalData->m_materialTriangle[i].clear();
//		m_internalData->m_materialTriangle[i] = new vsMeshMakerTriangle[maxTriangleCount];
//		m_internalData->m_materialTriangleCount[i] = 0;
	}

	m_internalData->m_materialCount = 0;

	//m_cellCount = MAKER_CELLS * MAKER_CELLS * MAKER_CELLS;
	//m_cell = new vsMeshMakerCell[m_cellCount];
	m_vertex = nullptr;
}

vsMeshMaker::~vsMeshMaker()
{
	//vsDeleteArray( m_cell );
	vsDelete( m_octree );
	vsDeleteArray( m_vertex );
	for ( int i = 0; i < MAX_MESH_MAKER_MATERIALS; i++ )
	{
		m_internalData->m_materialTriangle[i].clear();
		//vsDeleteArray( m_internalData->m_materialTriangle[i] );
	}
	for ( int i = 0; i < m_internalData->m_materialCount; i++ )
	{
		vsDelete( m_internalData->m_material[i] );
	}
	vsDelete( m_internalData );
}

void
vsMeshMaker::Clear()
{
	vsDeleteArray( m_vertex );
	m_triangleCount = 0;
	for ( int i = 0; i < m_internalData->m_materialCount; i++ )
	{
		vsDelete( m_internalData->m_material[i] );
	}
	m_internalData->m_materialCount = 0;
	for ( int i = 0; i < MAX_MESH_MAKER_MATERIALS; i++ )
	{
		m_internalData->m_materialTriangle[i].clear();
	}
}

void
vsMeshMaker::AddTriangle( const vsMeshMakerTriangle &triangle_in )
{
	// don't bother with degenerate triangles
	if ( triangle_in.IsDegenerate() )
	{
		return;
	}

	vsMeshMakerTriangle triangle = triangle_in;

	vsVector3D ab = triangle.m_vertex[1].GetPosition() - triangle.m_vertex[0].GetPosition();
	vsVector3D ac = triangle.m_vertex[2].GetPosition() - triangle.m_vertex[0].GetPosition();
	triangle.m_faceNormal = ac.Cross(ab);
	if ( triangle.m_faceNormal.SqLength() == 0.f )
	{
		// Also a degenerate triangle (all verts along a single line).  But a less trivially recognised one.
		return;
	}
	triangle.m_faceNormal.Normalise();

	int matId = BakeTriangleMaterial( triangle.m_material );
//	triangle.m_materialIndex = matId;

	m_internalData->m_materialTriangle[matId].push_back(triangle);
	m_triangleCount++;
}

void
vsMeshMaker::BakeTriangleEdge( vsMeshMakerTriangle *triangle, int vertA, int vertB )
{
	std::vector<vsMeshMakerTriangleEdge>::iterator iter;

	for ( iter = m_internalData->m_triangleEdge.begin(); iter != m_internalData->m_triangleEdge.end(); iter++ )
	{
		vsMeshMakerTriangleEdge *edge = &*iter;

		if ( edge->m_bTriangle == nullptr )
		{
			if ( edge->m_aVertex == &m_vertex[vertA] && edge->m_bVertex == &m_vertex[vertB] )
			{
				// found matching pair!
				vsAssert( edge->m_bTriangle == nullptr, "Edge with more than two polygons??" );

				edge->m_bTriangle = triangle;
				return;
			}
			else if ( edge->m_aVertex == &m_vertex[vertB] && edge->m_bVertex == &m_vertex[vertA] )
			{
				// found matching pair!
				vsAssert( edge->m_bTriangle == nullptr, "Edge with more than two polygons??" );

				edge->m_bTriangle = triangle;
				return;
			}
		}
	}

	vsMeshMakerTriangleEdge e;
	e.m_aTriangle = triangle;
	e.m_aVertex = &m_vertex[vertA];
	e.m_bVertex = &m_vertex[vertB];

	m_internalData->m_triangleEdge.push_back(e);
}

int
vsMeshMaker::BakeTriangleVertex( vsMeshMakerTriangleVertex &vertex, const vsVector3D &faceNormal )
{
	if ( m_attemptMerge )
	{
		float bestPriority = -1.f;
		vsMeshMakerTriangleVertex *best = nullptr;

		vsVector2D deltaTexel;

		const float testDistance = 1.1f;
		vsArray<vsMeshMakerTriangleVertex*> array;
		m_octree->FindPointsWithin( &array, vertex.GetPosition(), testDistance );

		for (int i = 0; i < array.ItemCount(); i++)
		{
			//vsMeshMakerTriangleVertex *other = &m_vertex[i];
			vsMeshMakerTriangleVertex *other = array[i];
			if ( m_buildingNormals )
			{
				const float epsilon = 0.01f;
				const float sqEpsilon = epsilon*epsilon;

				const bool closeEnough = (other->GetPosition() - vertex.GetPosition()).SqLength() < sqEpsilon;
				//			bool texelMatches = ((other.m_texel - m_texel).SqLength() < sqEpsilon);

				deltaTexel = other->GetTexel() - vertex.GetTexel();
				if ( deltaTexel.x > 1.f )
				{
					deltaTexel.x -= vsFloor(deltaTexel.x);
				}
				if ( deltaTexel.y > 1.f )
				{
					deltaTexel.y -= vsFloor(deltaTexel.y);
				}
				//			const bool texelMatches = deltaTexel.SqLength() < sqEpsilon;
				bool texelMatches = true;

				if ( closeEnough && texelMatches )
				{
					float priority = other->GetMergePriorityWith(vertex, faceNormal);

					if ( priority > bestPriority )
					{
						bestPriority = priority;
						best = other;
					}
				}
			}
			else
			{
				if ( *other == vertex )
				{
					return other->m_index;
				}
			}
		}

		if ( best && bestPriority >= 0.f )
		{
			vsMeshMakerTriangleVertex newVertex = vertex;

			bool didMerge = best->AttemptMergeWith(&newVertex, faceNormal);
			if ( didMerge )
			{
				return best->m_index;
			}
			else
			{
				// did a fake merge.
				newVertex.m_index = m_vertexCount;
				m_vertex[m_vertexCount] = newVertex;
				//cell->m_vertexIndex.push_back(m_vertexCount);
				m_octree->AddPoint( &m_vertex[m_vertexCount] );
				m_vertexCount++;

				return m_vertexCount-1;
			}
		}
	}

	vertex.m_index = m_vertexCount;
	m_vertex[m_vertexCount] = vertex;
	m_octree->AddPoint( &m_vertex[m_vertexCount] );
	//cell->m_vertexIndex.push_back(m_vertexCount);

	if ( m_buildingNormals )
	{
		m_vertex[m_vertexCount].SetNormal( faceNormal );
	}

	m_vertexCount++;

	return m_vertexCount-1;
}

int
vsMeshMaker::BakeTriangleMaterial( vsMaterial *material )
{
	for ( int i = 0; i < m_internalData->m_materialCount; i++ )
	{
		if ( *m_internalData->m_material[i] == *material )
		{
			return i;
		}
	}

	m_internalData->m_material[m_internalData->m_materialCount++] = new vsMaterial(*material);

	return m_internalData->m_materialCount-1;
}

void
vsMeshMaker::BuildTriangleStripsForMaterial( int matId )
{
	int count = m_internalData->m_materialTriangle[matId].size();
	//vsLog("Total triangle count:  %d", count);

	std::vector<vsMeshMakerTriangle>::iterator t = m_internalData->m_materialTriangle[matId].begin();

	m_internalData->m_mesh->SetTriangleListTriangleCount( matId, count );
	m_internalData->m_mesh->SetTriangleListMaterial( matId, m_internalData->m_material[matId] );

	while( t != m_internalData->m_materialTriangle[matId].end() )
	{
		m_internalData->m_mesh->AddTriangleToList( matId, t->m_vertex[0].m_index, t->m_vertex[1].m_index, t->m_vertex[2].m_index );
		t++;
	}
}

vsMesh *
vsMeshMaker::Bake()
{
	vsDelete( m_octree );
	m_cellBounds.Clear();
	for ( int matId = 0; matId < m_internalData->m_materialCount; matId++ )
	{
		std::vector<vsMeshMakerTriangle> *triangleList = &m_internalData->m_materialTriangle[matId];
		//int triangleCount = m_internalData->m_materialTriangle[matId].size();

		std::vector<vsMeshMakerTriangle>::iterator iter;

		for ( iter = triangleList->begin(); iter != triangleList->end(); iter++ )
		{
			vsMeshMakerTriangle *triangle = &*iter;
			m_cellBounds.ExpandToInclude( triangle->m_vertex[0].GetPosition() );
			m_cellBounds.ExpandToInclude( triangle->m_vertex[1].GetPosition() );
			m_cellBounds.ExpandToInclude( triangle->m_vertex[2].GetPosition() );
		}
	}
	m_cellBounds.Expand( 20.0f );
	m_octree = new vsPointOctree<vsMeshMakerTriangleVertex>( m_cellBounds, 16 );


	// build a list of unique vertices, converting our triangles to refer to indices, instead.

	int maxVertexCount = m_triangleCount * 3;
	m_vertexCount = 0;
	m_vertex = new vsMeshMakerTriangleVertex[maxVertexCount];
	for ( int matId = 0; matId < m_internalData->m_materialCount; matId++ )
	{
		std::vector<vsMeshMakerTriangle> *triangleList = &m_internalData->m_materialTriangle[matId];
		//int triangleCount = m_internalData->m_materialTriangle[matId].size();

		std::vector<vsMeshMakerTriangle>::iterator iter;
		//vsLog("Baking %d triangles.", triangleList->size());

		for ( iter = triangleList->begin(); iter != triangleList->end(); iter++ )
		{
			vsMeshMakerTriangle *triangle = &*iter;

			triangle->m_vertex[0].m_index = BakeTriangleVertex( triangle->m_vertex[0], triangle->m_faceNormal );
			m_vertex[iter->m_vertex[0].m_index].AddTriangle(triangle);
			triangle->m_vertex[1].m_index = BakeTriangleVertex( triangle->m_vertex[1], triangle->m_faceNormal );
			m_vertex[iter->m_vertex[1].m_index].AddTriangle(triangle);
			triangle->m_vertex[2].m_index = BakeTriangleVertex( triangle->m_vertex[2], triangle->m_faceNormal );
			m_vertex[iter->m_vertex[2].m_index].AddTriangle(triangle);
		}
	}

	//vsLog("Ended up with %d vertices.", m_vertexCount);
	m_internalData->m_mesh = new vsMesh(m_vertexCount, m_internalData->m_materialCount);

	for ( int i = 0; i < m_vertexCount; i++ )
	{
		if ( m_vertex[i].m_flags & vsMeshMakerTriangleVertex::Flag_Position )
		{
			m_internalData->m_mesh->SetVertex(i, m_vertex[i].GetPosition());
		}
		if ( m_vertex[i].m_flags & vsMeshMakerTriangleVertex::Flag_Normal )
		{
			m_internalData->m_mesh->SetNormal(i, m_vertex[i].GetNormal());
		}
		if ( m_vertex[i].m_flags & vsMeshMakerTriangleVertex::Flag_Color )
		{
			m_internalData->m_mesh->SetColor(i, m_vertex[i].GetColor());
		}
		if ( m_vertex[i].m_flags & vsMeshMakerTriangleVertex::Flag_Texel )
		{
			m_internalData->m_mesh->SetTexel(i, m_vertex[i].GetTexel());
		}
	}

	// 2 - for each material, build triangle strips using their indices.

	for ( int matId = 0; matId < m_internalData->m_materialCount; matId++ )
	{
		BuildTriangleStripsForMaterial( matId );
	}

	// 3 - using all of the above data, build a vsMesh containing the vertices, materials, and triangle strips.
	m_internalData->m_mesh->Bake();

	vsMesh *result = m_internalData->m_mesh;
	m_internalData->m_mesh = nullptr;

	return result;
}

