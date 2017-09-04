/*
 *  VS_MeshMaker.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 5/04/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MESHMAKER_H
#define VS_MESHMAKER_H

#include "VS/Graphics/VS_Color.h"
#include "VS/Graphics/VS_Material.h"
#include "VS/Math/VS_Box.h"
#include "VS/Math/VS_Vector.h"
#include "VS/Utils/VS_PointOctree.h"

class vsMesh;

//struct vsMeshMakerCell;
struct vsMeshMakerTriangle;
class vsMeshMakerTriangleEdge;

struct vsMeshMakerTriangleVertex: public vsPointOctreeElement
{
	vsVector3D		m_position;
	vsVector3D		m_normal;
	vsColor			m_color;
	vsVector2D		m_texel;
	int				m_mergeCount;

	vsVector3D		m_totalNormal;

	vsMeshMakerTriangleVertex *	m_fakeNormalMergedWith;

	vsMeshMakerTriangle *		m_firstTriangle;

public:

	enum
	{
		Flag_Position	= BIT(0),
		Flag_Normal		= BIT(1),
		Flag_Color		= BIT(2),
		Flag_Texel		= BIT(3)
	};

	int				m_flags;

	int				m_index;						// used by vsMeshMaker internally

	vsMeshMakerTriangleVertex();

	// returns true if the merge happened, false if we converted 'other' into a fake merged vertex.
	bool				AttemptMergeWith( vsMeshMakerTriangleVertex *other, const vsVector3D &faceNormalOther );
	float				GetMergePriorityWith( const vsMeshMakerTriangleVertex &other, const vsVector3D &faceNormalOther );

	void					AddTriangle( vsMeshMakerTriangle *triangle );
	vsMeshMakerTriangle *	GetFirstTriangle() { return m_firstTriangle; }

	void					CalculateNormal();

	void				SetPosition( const vsVector3D &position );
	void				SetNormal( const vsVector3D &normal );
	void				SetColor( const vsColor &color );
	void				SetTexel( const vsVector2D &texel );

	const vsVector3D &	GetPosition() const { return m_position; }
	const vsVector3D &	GetNormal() const;
	const vsColor &		GetColor() const { return m_color; }
	const vsVector2D &	GetTexel() const { return m_texel; }

	inline bool				operator==(const vsMeshMakerTriangleVertex &other) const;
};

struct vsMeshMakerTriangle
{
	vsMaterial *				m_material;
	vsMeshMakerTriangleVertex	m_vertex[3];

	vsVector3D					m_faceNormal;		// calculated internally;  no need for caller to provide this

	bool						IsDegenerate() const;
};

#define MAKER_CELLS (32)

class vsMeshMaker
{
	struct InternalData;

	int					m_triangleCount;

	vsPointOctree<vsMeshMakerTriangleVertex> *m_octree;
	vsBox3D				m_cellBounds;

	vsMeshMakerTriangleVertex *m_vertex;
	int				m_vertexCount;

	bool			m_buildingNormals;
	bool			m_attemptMerge;

	InternalData		*m_internalData;

	void			BakeTriangleEdge( vsMeshMakerTriangle *triangle, int vertA, int vertB );
	int				BakeTriangleVertex( vsMeshMakerTriangleVertex &vertex, const vsVector3D &faceNormal );
	int				BakeTriangleMaterial( vsMaterial *material );
	void			BuildTriangleStripsForMaterial( int matId );

public:

	enum
	{
		Flag_BuildNormals = BIT(0),
		Flag_NoMerge = BIT(1)
	};
					vsMeshMaker( int flags = 0 );
					~vsMeshMaker();

	void			Clear();
	void			AddTriangle( const vsMeshMakerTriangle &triangle );
	vsMesh *		Bake();
};

#endif // VS_MESHMAKER_H

