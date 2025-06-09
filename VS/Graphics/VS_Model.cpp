/*
 *  VS_Model.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 9/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Model.h"
#include "VS_ModelInstance.h"
#include "VS_ModelInstanceGroup.h"

#include "VS_DisplayList.h"
#include "VS_RenderQueue.h"
#include "VS_EulerAngles.h"

#include "VS_File.h"
#include "VS_Record.h"
#include "VS_Serialiser.h"
#include "VS_Store.h"


vsModel *
vsModel::Load( const vsString &filename_in )
{
	vsString filename = filename_in;
	// Check extension.
	size_t dot = filename.rfind('.');
	if ( dot != vsString::npos && dot == filename.size() - 4 )
	{
		// three-character extension
		vsString extension = filename.substr(dot+1,-1);
		if ( extension == "vmd" || extension == "vmb" )
			filename.erase(dot,-1);
	}

	vsString binaryFilename = filename + ".vmb";
	vsString textFilename = filename + ".vmd";
	if ( vsFile::Exists(binaryFilename) )
		return LoadBinary( binaryFilename );
	else
		return LoadText( textFilename );
}

vsFragment*
vsModel::LoadFragment_Internal( vsSerialiserRead& r )
{
	vsFragment *result = nullptr;

	vsString tag;
	r.String(tag);
	if ( tag == "Fragment" )
	{
		result = new vsFragment;
		vsString materialName, format;
		r.String(materialName);
		r.String(format);
		result->SetMaterial(materialName);
		int32_t vertexCount;
		r.Int32(vertexCount);
		vsRenderBuffer *vbo = new vsRenderBuffer(vsRenderBuffer::Type_Static);
		vsRenderBuffer *ibo = new vsRenderBuffer(vsRenderBuffer::Type_Static);

		if ( format == "PCNT" )
		{
			vsRenderBuffer::PCNT *buffer = new vsRenderBuffer::PCNT[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				vsColor c;
				r.Color(c);
				buffer[i].color = c;
				vsVector3D n;
				r.Vector3D(n);
				buffer[i].normal = n;
				vsVector2D t;
				r.Vector2D(t);
				buffer[i].texel = t;
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PCN" )
		{
			vsRenderBuffer::PCN *buffer = new vsRenderBuffer::PCN[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				vsColor c;
				r.Color(c);
				buffer[i].color = c;
				vsVector3D n;
				r.Vector3D(n);
				buffer[i].normal = n;
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PNT" )
		{
			vsRenderBuffer::PNT *buffer = new vsRenderBuffer::PNT[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				vsVector3D n;
				r.Vector3D(n);
				buffer[i].normal = n;
				vsVector2D t;
				r.Vector2D(t);
				buffer[i].texel = t;
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PCT" )
		{
			vsRenderBuffer::PCT *buffer = new vsRenderBuffer::PCT[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				vsColor c;
				r.Color(c);
				buffer[i].color = c;
				vsVector2D t;
				r.Vector2D(t);
				buffer[i].texel = t;
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PC" )
		{
			vsRenderBuffer::PC *buffer = new vsRenderBuffer::PC[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				vsColor c;
				r.Color(c);
				buffer[i].color = c;
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PN" )
		{
			vsRenderBuffer::PN *buffer = new vsRenderBuffer::PN[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				vsVector3D n;
				r.Vector3D(n);
				buffer[i].normal = n;
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PT" )
		{
			vsRenderBuffer::PT *buffer = new vsRenderBuffer::PT[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				vsVector2D t;
				r.Vector2D(t);
				buffer[i].texel = t;
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "P" )
		{
			vsRenderBuffer::P *buffer = new vsRenderBuffer::P[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else
		{
			vsAssert(0, vsFormatString("Unsupported vertex format: %s", format) );
		}

		r.String(tag);

		vsAssert( tag == "IndexBuffer", "Not matching up??" );
		int32_t indexCount;
		r.Int32(indexCount);
		uint16_t *indices = new uint16_t[ indexCount ];
		for ( int i = 0; i < indexCount; i++ )
		{
			int32_t ind;
			r.Int32(ind);
			indices[i] = ind;
		}
		ibo->SetArray(indices, indexCount);
		vsDeleteArray(indices);

		result->SetSimple( vbo, ibo, vsFragment::SimpleType_TriangleList );
	}

	return result;
}

vsModel*
vsModel::LoadModel_InternalV1( vsSerialiserRead& r )
{
	vsModel *result = new vsModel;
	r.String(result->m_name);

	vsVector3D trans, scale;
	vsVector4D rot;
	r.Vector3D(trans);
	r.Vector4D(rot);
	r.Vector3D(scale);

	result->SetPosition(trans);
	result->SetScale(scale);
	result->SetOrientation( vsQuaternion( rot.x, rot.y, rot.z, rot.w ) );

	int32_t meshCount;
	r.Int32(meshCount);

	for ( int i = 0; i < meshCount; i++ )
	{
		vsFragment *f = LoadFragment_Internal(r);
		result->AddFragment(f);
	}

	int32_t childCount;
	r.Int32(childCount);

	for ( int32_t i = 0; i < childCount; i++ )
	{
		vsModel *child = LoadModel_Internal(r);
		result->AddChild(child);
	}
	return result;
}

vsModel*
vsModel::LoadModel_InternalV2( vsSerialiserRead& r )
{
	vsModel *result = new vsModel;
	r.String(result->m_name);

	vsVector3D trans, scale;
	vsVector4D rot;
	r.Vector3D(trans);
	r.Vector4D(rot);
	r.Vector3D(scale);

	result->SetPosition(trans);
	result->SetScale(scale);
	result->SetOrientation( vsQuaternion( rot.x, rot.y, rot.z, rot.w ) );

	int32_t lodCount;
	r.Int32(lodCount);
	result->SetLodCount(lodCount);

	for ( int l = 0; l < lodCount; l++ )
	{
		int32_t meshCount;
		r.Int32(meshCount);

		for ( int i = 0; i < meshCount; i++ )
		{
			vsFragment *f = LoadFragment_Internal(r);
			result->AddLodFragment(l, f);
		}
	}

	int32_t childCount;
	r.Int32(childCount);

	for ( int32_t i = 0; i < childCount; i++ )
	{
		vsModel *child = LoadModel_Internal(r);
		result->AddChild(child);
	}
	return result;
}

vsModel*
vsModel::LoadModel_Internal( vsSerialiserRead& r )
{
	vsModel *result = nullptr;
	vsString tag;
	r.String(tag);
	if ( tag == "ModelV1" )
	{
		result = LoadModel_InternalV1(r);
	}
	else if ( tag == "ModelV2" )
	{
		result = LoadModel_InternalV2(r);
	}

	return result;
}

vsModel *
vsModel::LoadBinary( const vsString &filename )
{
	vsModel *result = nullptr;

	vsFile file(filename);
	vsStore store(file.GetLength());
	file.Store(&store);
	vsSerialiserRead r(&store);

	result = LoadModel_Internal(r);


	return result;
}

vsModel *
vsModel::LoadText( const vsString &filename )
{
	vsFile file(filename);
	vsRecord r;

	while( file.Record(&r) )
	{
		if ( r.GetLabel().AsString() == "Model" )
		{
			vsModel *result = new vsModel;
			result->LoadFrom(&r);
			return result;
		}
	}

	return nullptr;
}

void
vsModel::LoadFrom( vsRecord *record )
{
	for ( int i = 0; i < record->GetChildCount(); i++ )
	{
		vsRecord *sr = record->GetChild(i);

		vsString srLabel = sr->GetLabel().AsString();

		if ( srLabel == "name" )
		{
			m_name = sr->GetToken(0).AsString();
			// don't use the name right now.
		}
		else if ( srLabel == "matrix" )
		{
			vsAssert( sr->GetChildCount() == 4, "Wrong number of matrix lines" );
			vsMatrix4x4 mat;
			for ( int i = 0; i < 4; i++ )
			{
				mat[i] = sr->GetChild(i)->Vector4D();
			}
			SetPosition(mat.w);
			vsQuaternion q(mat.z, mat.y);
			SetOrientation(q);
			// SetScale(mat.GetScale());
		}
		else if ( srLabel == "translation" )
		{
			SetPosition( sr->Vector3D() );
		}
		else if ( srLabel == "rotation" )
		{
			SetOrientation( sr->Quaternion() );
		}
		else if ( srLabel == "scale" )
		{
			SetScale( sr->Vector3D() );
		}
		else if ( srLabel == "rotationXYZDegrees" )
		{
			// for convenience, allow the rotation to be set as euler angles,
			// using degrees.
			vsVector3D xyz = sr->Vector3D();
			SetOrientation( vsQuaternion( vsEulerAngles(
							DEGREES(xyz.x),
							DEGREES(xyz.y),
							DEGREES(xyz.z)
							) ) );
		}
		else if ( srLabel == "Model" )
		{
			vsModel *child = new vsModel;
			child->LoadFrom(sr);

			AddChild( child );
		}
		else if ( srLabel == "DisplayList" )
		{
			vsDisplayList *list = vsDisplayList::Load( sr );
			SetDisplayList( list );
		}
		else if ( srLabel == "Fragment" )
		{
			vsFragment *fragment = vsFragment::Load( sr );
			AddFragment( fragment );
		}
	}
}


vsModel::vsModel( vsDisplayList *list ):
	m_material(nullptr),
	m_boundingBox(),
	m_lowBoundingBox(),
	m_boundingRadius(0.f),
	m_lodLevel(0),
	m_instanceGroup(nullptr),
	m_displayList(list)
{
	SetLodCount(1);
	SetLodLevel(0);
}

vsModel::~vsModel()
{
	m_lod.Clear();

	if ( m_displayList )
		vsDelete(m_displayList);
	vsDelete( m_material );
	vsDelete( m_instanceGroup );
}

void
vsModel::SetAsInstanceModel()
{
	if ( !m_instanceGroup )
	{
		m_instanceGroup = new vsModelInstanceGroup(this);
	}
}

void
vsModel::SetDisplayList( vsDisplayList *list )
{
	if ( m_displayList )
		vsDelete( m_displayList );
	m_displayList = list;

	BuildBoundingBox();
}

void
vsModel::AddLodFragment( int lodLevel, vsFragment *fragment )
{
	vsAssert((int)lodLevel < m_lod.ItemCount(), "Tried to add a fragment to a non-existant lod??");
	if ( fragment )
		m_lod[lodLevel]->fragment.AddItem( fragment );
}

void
vsModel::DestroyFragment( vsFragment *fragment )
{
	for ( int i = 0; i < m_lod.ItemCount(); i++ )
		m_lod[i]->fragment.RemoveItem( fragment );
}

void
vsModel::ClearFragments()
{
	m_lod[0]->fragment.Clear();
}

void
vsModel::BuildBoundingBox()
{
	vsBox3D boundingBox;
	if ( m_displayList )
	{
		m_displayList->GetBoundingBox(boundingBox);
	}
	for ( vsArrayStore<vsFragment>::Iterator iter = m_lod[0]->fragment.Begin(); iter != m_lod[0]->fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		vsBox3D fragmentBox;
		if ( fragment->IsSimple() )
		{
			vsRenderBuffer *b = fragment->GetSimpleVBO();
			for (int i = 0; i < b->GetPositionCount(); i++ )
				boundingBox.ExpandToInclude( b->GetPosition(i) );
		}
		else
		{
			if ( fragment->GetDisplayList() )
			{
				fragment->GetDisplayList()->GetBoundingBox( fragmentBox );
				boundingBox.ExpandToInclude( fragmentBox );
			}
		}
	}

	vsEntity *c = m_child;

	while ( c )
	{
		vsModel *childSprite = dynamic_cast<vsModel*>(c);
		if ( childSprite )
		{
			childSprite->BuildBoundingBox();
			vsBox3D childBox = childSprite->GetBoundingBox();

			boundingBox.ExpandToInclude( childBox + childSprite->GetPosition() );
		}
		c = c->GetNext();
	}

	SetBoundingBox( boundingBox );
}

void
vsModel::BuildLowBoundingBox( float threshhold )
{
	vsBox3D boundingBox;
	if ( m_displayList )
	{
		m_displayList->GetBoundingBox(boundingBox);
	}
	for ( vsArrayStore<vsFragment>::Iterator iter = m_lod[0]->fragment.Begin(); iter != m_lod[0]->fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		vsBox3D fragmentBox;
		if ( fragment->IsSimple() )
		{
			vsRenderBuffer *b = fragment->GetSimpleVBO();
			for (int i = 0; i < b->GetPositionCount(); i++ )
			{
				const vsVector3D v = b->GetPosition(i);
				if ( v.y < threshhold )
					boundingBox.ExpandToInclude(v);
			}
		}
		// else if ( fragment->GetDisplayList() )
		// {
		// 	fragment->GetDisplayList()->GetLowBoundingBox( fragmentBox );
		// 	boundingBox.ExpandToInclude( fragmentBox );
		// }
	}
	SetLowBoundingBox( boundingBox );
}

vsDisplayList::Stats
vsModel::CalculateStats()
{
	vsDisplayList::Stats result = {0,0,0};

	if ( m_displayList )
		result = m_displayList->CalculateStats();
	for ( vsArrayStore<vsFragment>::Iterator iter = m_lod[0]->fragment.Begin(); iter != m_lod[0]->fragment.End(); iter++ )
	{
		result += iter->GetDisplayList()->CalculateStats();
	}

	return result;
}

void
vsModel::DrawInstanced( vsRenderQueue *queue, const vsMatrix4x4* matrices, const vsColor* colors, int instanceCount, vsShaderValues *shaderValues, vsShaderOptions *options, int lodLevel )
{
	vsAssert( lodLevel < m_lod.ItemCount(), "Invalid lod level set?");
	for( vsArrayStoreIterator<vsFragment> iter = m_lod[lodLevel]->fragment.Begin(); iter != m_lod[lodLevel]->fragment.End(); iter++ )
	{
		if ( iter->IsVisible() )
			queue->AddFragmentInstanceBatch( *iter, matrices, colors, instanceCount, shaderValues, options );
	}
}

void
vsModel::DrawInstanced( vsRenderQueue *queue, vsRenderBuffer* matrixBuffer, vsRenderBuffer* colorBuffer, vsShaderValues *shaderValues, vsShaderOptions *options, int lodLevel )
{
	vsAssert( lodLevel < m_lod.ItemCount(), "Invalid lod level set?");
	for( vsArrayStoreIterator<vsFragment> iter = m_lod[lodLevel]->fragment.Begin(); iter != m_lod[lodLevel]->fragment.End(); iter++ )
	{
		if ( iter->IsVisible() )
			queue->AddFragmentInstanceBatch( *iter, iter->GetVAO(), matrixBuffer, colorBuffer, shaderValues, options );
	}
}

void
vsModel::DrawInstanced( vsRenderQueue *queue, vsVertexArrayObject* vao, vsRenderBuffer* matrixBuffer, vsRenderBuffer* colorBuffer, vsShaderValues *shaderValues, vsShaderOptions *options, int lodLevel )
{
	vsAssert( lodLevel < m_lod.ItemCount(), "Invalid lod level set?");
	for( vsArrayStoreIterator<vsFragment> iter = m_lod[lodLevel]->fragment.Begin(); iter != m_lod[lodLevel]->fragment.End(); iter++ )
	{
		if ( iter->IsVisible() )
			queue->AddFragmentInstanceBatch( *iter, vao, matrixBuffer, colorBuffer, shaderValues, options );
	}
}

void
vsModel::Draw( vsRenderQueue *queue )
{
	if ( GetVisible() )
	{
		if ( m_instanceGroup )
		{
			m_instanceGroup->Draw( queue );
		}
		else
		{
			bool hasTransform = (m_transform != vsTransform3D::Identity);

			if ( hasTransform )
			{
				queue->PushMatrix( m_transform.GetMatrix() );
			}

			if ( m_displayList )
			{
				// old rendering support
				vsDisplayList *list = queue->GetGenericList();
				if ( m_material )
				{
					list->SetMaterial( m_material );
				}
				list->SetMatrix4x4( queue->GetMatrix() );
				list->Append( *m_displayList );
				list->PopTransform();
			}
			else
			{
				DynamicDraw( queue );
			}

			vsLod *lod = m_lod[m_lodLevel];
			for( int i = 0; i < lod->fragment.ItemCount(); i++ )
			{
				vsFragment *f = lod->fragment[i];
				if ( f->IsVisible() )
					queue->AddFragmentBatch( f );
			}

			DrawChildren(queue);

			if ( hasTransform )
			{
				queue->PopMatrix();
			}
		}
	}
}

vsModelInstance *
vsModel::MakeInstance()
{
	SetAsInstanceModel();
	return m_instanceGroup->MakeInstance();
}

void
vsModel::UpdateInstance( vsModelInstance *inst, bool show )
{
	m_instanceGroup->UpdateInstance( inst, show );
}

void
vsModel::RemoveInstance( vsModelInstance *inst )
{
	m_instanceGroup->RemoveInstance( inst );
}

void
vsModel::SetLodCount(int count)
{
	vsAssert(count > 0, "Zero-LOD vsModels are not supported");
	m_lod.SetArraySize(count);
	m_lodLevel = vsMin(m_lodLevel, count-1);
}

int
vsModel::GetLodFragmentCount( int lodId ) const
{
	return m_lod[lodId]->fragment.ItemCount();
}

bool
vsModel::CollideRay(vsVector3D *result, vsVector3D *resultNormal, float *resultT, const vsVector3D &pos, const vsVector3D &dir) const
{
	vsVector3D localPos = m_transform.ApplyInverseTo(pos);
	vsVector3D localDir = m_transform.GetRotation().Inverse().ApplyTo(dir);

	// now we need to collision test against all our triangles.
	vsArray<vsDisplayList::Triangle> triangles;
	if ( m_displayList )
		m_displayList->GetTriangles(triangles);
	for ( int i = 0; i < GetFragmentCount(); i++ )
	{
		m_lod[0]->fragment[i]->GetTriangles(triangles);
	}

	bool hit = false;
	for ( int i = 0; i < triangles.ItemCount(); i++ )
	{
		float u, v;
		vsDisplayList::Triangle &t = triangles[i];
		float localT;
		if ( vsCollideRayVsTriangle( localPos, localDir,
					t.vertex[0], t.vertex[1], t.vertex[2],
					&localT, &u, &v ) )
		{
			if ( localT < *resultT )
			{
				hit = true;
				*result = pos + dir * (localT);
				*resultT = localT;

				vsVector3D triangleNormal = (t.vertex[2]-t.vertex[0]).Cross( t.vertex[1]-t.vertex[0] );
				triangleNormal.NormaliseSafe();
				if ( triangleNormal.Dot( localDir ) > 0.f ) // reversed!  (is this the right thing to do if we're hitting the back of a triangle?
					triangleNormal *= -1.f;
				*resultNormal = m_transform.GetRotation().ApplyTo( triangleNormal );
			}
		}
	}
	return hit;
}

void
vsModel::GatherVerticesInYInterval( vsArray<vsVector3D>& result, float minY, float maxY )
{
	// [TODO] Also implement for display lists.
	for ( vsArrayStore<vsFragment>::Iterator iter = m_lod[0]->fragment.Begin(); iter != m_lod[0]->fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		vsBox3D fragmentBox;
		if ( fragment->IsSimple() )
		{
			vsRenderBuffer *b = fragment->GetSimpleVBO();
			for (int i = 0; i < b->GetPositionCount(); i++ )
			{
				const vsVector3D v = b->GetPosition(i);
				if ( v.y >= minY && v.y <= maxY )
					result.AddItem(v);
			}
		}
		// [TODO] Also implement for non-simple fragments, maybe?
		//
		// else if ( fragment->GetDisplayList() )
		// {
		// 	fragment->GetDisplayList()->GetLowBoundingBox( fragmentBox );
		// 	boundingBox.ExpandToInclude( fragmentBox );
		// }
	}
}

void
vsModel::SaveOBJ( const vsString& filename )
{
	vsFile f(filename, vsFile::MODE_Write);

	for ( vsArrayStore<vsFragment>::Iterator iter = m_lod[0]->fragment.Begin(); iter != m_lod[0]->fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		vsBox3D fragmentBox;
		if ( fragment->IsSimple() )
		{
			vsString line;
			vsRenderBuffer *b = fragment->GetSimpleVBO();
			for (int i = 0; i < b->GetPositionCount(); i++ )
			{
				const vsVector3D v = b->GetPosition(i);
				const vsColor c = b->GetColor(i);

				line = vsFormatString( "v %f %f %f %f %f %f\n", v.x, v.y, v.z, c.r, c.g, c.b );
				f.WriteBytes( line.c_str(), line.size() );
			}
			for (int i = 0; i < b->GetPositionCount(); i++ )
			{
				const vsVector3D n = b->GetNormal(i);
				line = vsFormatString( "vn %f %f %f\n", n.x, n.y, n.z );
				f.WriteBytes( line.c_str(), line.size() );
			}
			b = fragment->GetSimpleIBO();
			for (int i = 0; i < b->GetIntArraySize(); i+=3 )
			{
				int aa = b->GetIntArray()[i]+1;
				int bb = b->GetIntArray()[i+1]+1;
				int cc = b->GetIntArray()[i+2]+1;

				line = vsFormatString("f %d//%d %d//%d %d//%d\n", aa, aa, bb, bb, cc, cc);
				f.WriteBytes( line.c_str(), line.size() );
			}
		}
	}

}

