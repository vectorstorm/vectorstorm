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

// #define INSTANCED_MODEL_USES_LOCAL_BUFFER

class vsModel;
class vsModelInstanceGroup;
class vsSerialiserRead;

struct vsModelInstance
{
private:
	vsMatrix4x4 matrix;
	vsColor color;
	vsModelInstanceGroup *group;
	int index;       // our ID within the instance array.
	int matrixIndex; // our ID within the matrix array.
	bool visible;
	friend class vsModelInstanceGroup;

public:
	vsModelInstance() {}
	~vsModelInstance();

	void SetVisible( bool visible );
	void SetMatrix( const vsMatrix4x4& mat );
	void SetMatrix( const vsMatrix4x4& mat, const vsColor &color );

	vsModel * GetModel();
	vsModelInstanceGroup * GetInstanceGroup() { return group; }
	const vsVector4D& GetPosition() const { return matrix.w; }
	const vsMatrix4x4& GetMatrix() const { return matrix; }
};

class vsModelInstanceGroup : public vsEntity
{
	vsModel *m_model;
	vsArray<vsMatrix4x4> m_matrix;
	vsArray<vsColor> m_color;
	vsArray<int> m_matrixInstanceId;
	vsArray<vsModelInstance*> m_instance;
public:

	vsModelInstanceGroup( vsModel *model ):
		m_model(model)
	{
	}

	void TakeInstancesFromGroup( vsModelInstanceGroup *otherGroup );

	vsModelInstance * MakeInstance();		// create an instance of me.
	void AddInstance( vsModelInstance *instance );
	void RemoveInstance( vsModelInstance *instance );
	void UpdateInstance( vsModelInstance *instance, bool show = true ); // must be called to change the matrix on this instance
	vsModel * GetModel() { return m_model; }

	virtual void Draw( vsRenderQueue *queue );
};

class vsModel : public vsEntity
{
	typedef vsEntity Parent;

	struct InstanceData
	{
		vsArray<vsMatrix4x4> matrix;
		vsArray<vsColor> color;
		vsArray<int> matrixInstanceId;
		vsArray<vsModelInstance*> instance;
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
		vsRenderBuffer matrixBuffer;
		vsRenderBuffer colorBuffer;
		bool bufferIsDirty;
#endif // INSTANCED_MODEL_USES_LOCAL_BUFFER

#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
		InstanceData() :
			matrixBuffer(vsRenderBuffer::Type_Dynamic),
			colorBuffer(vsRenderBuffer::Type_Dynamic),
			bufferIsDirty(false)
		{
		}
#else
		InstanceData() {}
#endif

		virtual void	Draw( vsRenderQueue *queue );
	};

	vsMaterial *	m_material;

	vsBox3D				m_boundingBox;
	float				m_boundingRadius;

	static vsModel* LoadModel_Internal( vsSerialiserRead& r );
	static vsFragment* LoadFragment_Internal( vsSerialiserRead& r );
protected:

	vsDisplayList	*m_displayList;				// old-style rendering

	vsArrayStore<vsFragment>	m_fragment;	// new-style rendering

	vsModelInstanceGroup *m_instanceGroup;

	vsTransform3D m_transform;

	void LoadFrom( vsRecord *record );

public:

	static vsModel *	Load( const vsString &filename );
	static vsModel *	LoadBinary( const vsString &filename );

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
	void			AddFragment( vsFragment *fragment );
	void			ClearFragments();
	vsFragment *	GetFragment(int i) { return m_fragment[i]; }
	size_t			GetFragmentCount() { return m_fragment.ItemCount(); }

	virtual void	Draw( vsRenderQueue *list );
	void	DrawInstanced( vsRenderQueue *list, const vsMatrix4x4* matrices, const vsColor* colors, int instanceCount );
};

#endif // VS_MODEL_H

