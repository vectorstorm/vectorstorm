/*
 *  VS_Model.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 9/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MODEL_H
#define VS_MODEL_H

#include "VS/Graphics/VS_Color.h"
#include "VS/Graphics/VS_DisplayList.h"
#include "VS/Graphics/VS_Entity.h"
#include "VS/Graphics/VS_Fragment.h"
#include "VS/Graphics/VS_Material.h"
#include "VS/Math/VS_Box.h"
#include "VS/Math/VS_Transform.h"
#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_ArrayStore.h"

class vsModel;
struct vsModelInstance;
class vsModelInstanceGroup;
class vsSerialiserRead;

struct vsLod
{
	vsArrayStore<vsFragment>	fragment;
};

class vsModel : public vsEntity
{
	typedef vsEntity Parent;
	vsMaterial *	m_material;

	vsBox3D				m_boundingBox;
	float				m_boundingRadius;

	static vsModel* LoadModel_Internal( vsSerialiserRead& r );
	static vsModel* LoadModel_InternalV1( vsSerialiserRead& r );
	static vsModel* LoadModel_InternalV2( vsSerialiserRead& r );
	static vsFragment* LoadFragment_Internal( vsSerialiserRead& r );

	vsArrayStore<vsLod> m_lod; // new-new-style rendering.
	size_t m_lodLevel; // which lod am I rendering right now?  0 == 'm_fragment'.
	vsModelInstanceGroup *m_instanceGroup;
protected:

	vsDisplayList	*m_displayList;				// old-style rendering
	vsTransform3D m_transform;
	void LoadFrom( vsRecord *record );

public:

	static vsModel *	Load( const vsString &filename ); // trim the extension (if any) and try to load either binary or text format.
	static vsModel *	LoadBinary( const vsString &filename );
	static vsModel *	LoadText( const vsString &filename );

	vsModel( vsDisplayList *displayList = NULL );
	virtual			~vsModel();

	void SetAsInstanceModel(); // if set, this model won't be drawn;  only instances will.
	vsModelInstanceGroup *GetInstanceGroup() { return m_instanceGroup; }

	vsModelInstance * MakeInstance();		// create an instance of me.
	void RemoveInstance( vsModelInstance *model );
	void			UpdateInstance( vsModelInstance *, bool show = true ); // must be called to change the matrix on this instance

	void			SetMaterial( const vsString &name ) { vsDelete( m_material ); m_material = new vsMaterial(name); }
	void			SetMaterial( vsMaterial *material ) { vsDelete( m_material ); m_material = material; }
	vsMaterial *	GetMaterial() { return m_material; }

	virtual void		SetPosition( const vsVector3D &pos ) { m_transform.SetTranslation( pos ); }
	const vsVector3D &	GetPosition() const { return m_transform.GetTranslation(); }

	virtual void			SetOrientation( const vsQuaternion &quat ) { m_transform.SetRotation( quat ); }
	const vsQuaternion &	GetOrientation() const { return m_transform.GetRotation(); }

	const vsMatrix4x4 &		GetMatrix() const { return m_transform.GetMatrix(); }

	const vsVector3D &		GetScale() const { return m_transform.GetScale(); }
	void					SetScale( const vsVector3D &s ) { m_transform.SetScale(s); }
	void					SetScale( float s ) { m_transform.SetScale(s); }

	const vsBox3D &			GetBoundingBox() const { return m_boundingBox; }
	void					SetBoundingBox(const vsBox3D &box) { m_boundingBox = box; }
	void					BuildBoundingBox();

	float					GetBoundingRadius() { return m_boundingRadius; }

	void				SetTransform( const vsTransform3D &t ) { m_transform = t; }
	const vsTransform3D&	GetTransform() const { return m_transform; }

	void			SetDisplayList( vsDisplayList *list );
	void			AddFragment( vsFragment *fragment ) { AddLodFragment(0, fragment); }
	void			ClearFragments();
	vsFragment *	GetFragment(int i) { return GetLodFragment(0,i); }
	size_t			GetFragmentCount() { return GetLodFragmentCount(0); }

	size_t			GetLodCount() { return m_lod.ItemCount(); }
	size_t			GetLodFragmentCount( size_t lodId );
	vsFragment *	GetLodFragment(size_t lodId, size_t fragmentId) { return m_lod[lodId]->fragment[fragmentId]; }
	void			SetLodCount(size_t count);
	void			SetLodLevel(size_t level) { m_lodLevel = level; }
	size_t			GetLodLevel() { return m_lodLevel; }
	void			AddLodFragment( size_t lodId, vsFragment *fragment );

	virtual void	Draw( vsRenderQueue *list );
	void	DrawInstanced( vsRenderQueue *list, const vsMatrix4x4* matrices, const vsColor* colors, int instanceCount, vsShaderValues *values );
	void	DrawInstanced( vsRenderQueue *list, vsRenderBuffer* matrixBuffer, vsRenderBuffer* colorBuffer, vsShaderValues *values );
};

#endif // VS_MODEL_H

