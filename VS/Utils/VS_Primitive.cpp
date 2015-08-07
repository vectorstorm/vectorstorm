/*
 *  VS_Primitive.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 3/01/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Primitive.h"

#include "VS_Angle.h"
#include "VS_EulerAngles.h"
#include "VS_Matrix.h"
#include "VS_Mesh.h"


vsSphere::vsSphere( float radius, int slices, const vsString &materialName )
{
	vsAssert(slices > 1, "Need more than one slice!");

	int pitchSlices = slices / 2;

	int vertCount = slices * pitchSlices;
	int triangleCount = vertCount * 2;

	vsMaterial myMaterial(materialName);

	vsMesh *mesh = new vsMesh(vertCount,1);
	mesh->SetTriangleListTriangleCount(0, triangleCount);
	mesh->SetTriangleListMaterial(0, &myMaterial);

	float headingPerSlice = -TWOPI * (1.f / (slices-1));
	float pitchPerSlice = PI * (1.f / (pitchSlices-1));

	float iniHeading = 0.f;
	float iniPitch = PI * -0.5f;

	vsEulerAngles ang;
	vsMatrix3x3 matrix;

	float xIncrement = (1.f / (slices-1));
	float yIncrement = (1.f / (pitchSlices-1));

	for ( int x = 0; x < slices; x++ )
	{
		float heading = iniHeading + (x * headingPerSlice);

		for ( int y = 0; y < pitchSlices; y++ )
		{
			float pitch = iniPitch + (y * pitchPerSlice);

			ang.yaw = heading;
			ang.pitch = pitch;

			matrix.Set( vsQuaternion(ang) );

			mesh->SetVertex( (y*slices)+x, matrix.z * radius );
			mesh->SetNormal( (y*slices)+x, matrix.z );
			mesh->SetTexel( (y*slices)+x, vsVector2D(x * xIncrement, y * yIncrement) );
		}
	}

	for ( int x = 0; x < slices-1; x++ )
	{
		for ( int y = 0; y < pitchSlices-1; y++ )
		{
			int ty = y+1;
			int rx = x+1;

			int tl = (ty*slices)+x;
			int tr = (ty*slices)+rx;
			int bl = (y*slices)+x;
			int br = (y*slices)+rx;

			mesh->AddTriangleToList(0, tr, br, tl);
			mesh->AddTriangleToList(0, br, bl, tl);
		}

		/*
		i = 0;
		for ( int y = 0; y < pitchSlices; y++ )
		{
			indices[i++] = (y*slices)+x+1;
			indices[i++] = (y*slices)+x;
		}
		m_strip[x].SetArray(indices,pitchSlices*2);
		constructor->TriangleStripBuffer(&m_strip[x]);
		*/
		//		constructor->TriangleStrip(indices,slices*2);
	}

	mesh->WriteFragments( this );
	vsDelete(mesh);


	//	vsDelete( constructor );
}

vsGem::vsGem( float radius, int resolution, const vsString &materialName )
{
	vsMaterial myMaterial(materialName);

	int vertCount = 2 + resolution;
	int triangleCount = (resolution-1) * 2;

	vsMesh *mesh = new vsMesh(vertCount,1);
	mesh->SetTriangleListTriangleCount(0, triangleCount);
	mesh->SetTriangleListMaterial(0, &myMaterial);

	int upVert = resolution;
	int downVert = resolution+1;

	float headingPerSlice = -TWOPI * (1.f / (resolution-1));

	vsMatrix3x3 matrix;
	for ( int i = 0; i < resolution; i++ )
	{
		matrix.Set( vsQuaternion(vsVector3D::YAxis, i * headingPerSlice) );

		mesh->SetVertex( i, matrix.z * radius );
		mesh->SetNormal( i, matrix.z );
	}
	mesh->SetVertex( upVert, vsVector3D::YAxis * radius );
	mesh->SetNormal( upVert, vsVector3D::YAxis );

	mesh->SetVertex( downVert, -vsVector3D::YAxis * radius );
	mesh->SetNormal( downVert, -vsVector3D::YAxis );

	for ( int i = 0; i < resolution-1; i++ )
	{
		int left = i;
		int right = i+1;

		mesh->AddTriangleToList(0, left, right, upVert);
		mesh->AddTriangleToList(0, right, left, downVert);
	}

	mesh->WriteFragments(this);
	vsDelete(mesh);
}


vsFragment *	vsMakeSolidBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
/*	vsVector2D tex[4] =
	{
		vsVector2D( 0.f, 0.f),
		vsVector2D( 1.f, 0.f),
		vsVector2D( 0.f, 1.f),
		vsVector2D( 1.f, 1.f)
	};*/
	int ts[4] =
	{
		0,2,1,3
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
//	list->TexelArray(tex,4);
	list->TriangleStripArray(ts,4);
	list->ClearVertexArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeTexturedBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	vsVector2D tex[4] =
	{
		vsVector2D( 0.f, 0.f),
		vsVector2D( 1.f, 0.f),
		vsVector2D( 0.f, 1.f),
		vsVector2D( 1.f, 1.f)
	};
	int ts[4] =
	{
		0,2,1,3
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
	list->TexelArray(tex,4);
	list->TriangleStripArray(ts,4);
	list->ClearVertexArray();
	list->ClearTexelArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}
vsFragment *	vsMakeTiledTexturedBox2D( const vsBox2D &box, vsMaterial *material, float tileSize, const vsAngle &angle, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	vsVector2D tex[4] =
	{
		angle.ApplyRotationTo(vsVector2D( box.GetMin().x, box.GetMin().y) * (1.f / tileSize)),
		angle.ApplyRotationTo(vsVector2D( box.GetMax().x, box.GetMin().y) * (1.f / tileSize)),
		angle.ApplyRotationTo(vsVector2D( box.GetMin().x, box.GetMax().y) * (1.f / tileSize)),
		angle.ApplyRotationTo(vsVector2D( box.GetMax().x, box.GetMax().y) * (1.f / tileSize))
	};
	int ts[4] =
	{
		0,2,1,3
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
	list->TexelArray(tex,4);
	list->TriangleStripArray(ts,4);
	list->ClearVertexArray();
	list->ClearTexelArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeOutlineBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	int ls[5] =
	{
		0,1,3,2,0
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
	list->LineStripArray(ls,5);
	list->ClearVertexArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}


vsFragment *	vsMakeSolidBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride )
{
	vsRenderBuffer *buffer = new vsRenderBuffer;

	int arraySize = 24;
	vsRenderBuffer::PN *array = new vsRenderBuffer::PN[arraySize];

	vsVector3D normalVector[3] =
	{
		vsVector3D::XAxis,
		vsVector3D::YAxis,
		vsVector3D::ZAxis
	};

	vsVector3D dimVector[3] =
	{
		vsVector3D( box.Width() * 0.5f, 0.f, 0.f ),
		vsVector3D( 0.f, box.Height() * 0.5f, 0.f ),
		vsVector3D( 0.f, 0.f, box.Depth() * 0.5f ),
	};
	int triList[36];
	int tId = 0;

	for ( int dim = 0; dim < 3; dim++ )	// each dimension
	{
		int uDim = dim+1;
		if ( uDim >= 3 )
			uDim = 0;
		int vDim = uDim+1;
		if ( vDim >= 3 )
			vDim = 0;

		vsVector3D main = dimVector[dim];
		vsVector3D u = dimVector[uDim];
		vsVector3D v = dimVector[vDim];

		int vId = dim * 8;
		//		for ( int vert = 0; vert < 4; vert++ )
		{
			array[vId+0].position = box.Middle() + main - u - v;
			array[vId+1].position = box.Middle() + main + u - v;
			array[vId+2].position = box.Middle() + main + u + v;
			array[vId+3].position = box.Middle() + main - u + v;

			array[vId+4].position = box.Middle() - main - u - v;
			array[vId+5].position = box.Middle() - main + u - v;
			array[vId+6].position = box.Middle() - main + u + v;
			array[vId+7].position = box.Middle() - main - u + v;

			for ( int i = 0; i < 4; i++ )
			{
				array[vId+i].normal = normalVector[dim];
				array[vId+i+4].normal = -normalVector[dim];
			}

			triList[tId++] = vId+2;
			triList[tId++] = vId+1;
			triList[tId++] = vId+0;
			triList[tId++] = vId+0;
			triList[tId++] = vId+3;
			triList[tId++] = vId+2;

			triList[tId++] = vId+4;
			triList[tId++] = vId+5;
			triList[tId++] = vId+6;
			triList[tId++] = vId+6;
			triList[tId++] = vId+7;
			triList[tId++] = vId+4;
		}
	}

	buffer->SetArray( array, arraySize );
	vsDeleteArray( array );

	vsDisplayList *list = new vsDisplayList(512);
	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->BindBuffer(buffer);
	list->TriangleListArray(triList,36);
	list->ClearArrays();

	vsFragment *fragment = new vsFragment;
	fragment->AddBuffer(buffer);
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeTexturedBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride )
{
	vsRenderBuffer *buffer = new vsRenderBuffer;

	int arraySize = 24;
	vsRenderBuffer::PNT *array = new vsRenderBuffer::PNT[arraySize];

	vsVector3D normalVector[3] =
	{
		vsVector3D::XAxis,
		vsVector3D::YAxis,
		vsVector3D::ZAxis
	};

	vsVector3D dimVector[3] =
	{
		vsVector3D( box.Width() * 0.5f, 0.f, 0.f ),
		vsVector3D( 0.f, box.Height() * 0.5f, 0.f ),
		vsVector3D( 0.f, 0.f, box.Depth() * 0.5f ),
	};
	int triList[36];
	int tId = 0;

	for ( int dim = 0; dim < 3; dim++ )	// each dimension
	{
		int uDim = dim+1;
		if ( uDim >= 3 )
			uDim = 0;
		int vDim = uDim+1;
		if ( vDim >= 3 )
			vDim = 0;

		vsVector3D main = dimVector[dim];
		vsVector3D u = dimVector[uDim];
		vsVector3D v = dimVector[vDim];

		int vId = dim * 8;
		//		for ( int vert = 0; vert < 4; vert++ )
		{
			array[vId+0].position = box.Middle() + main - u - v;
			array[vId+1].position = box.Middle() + main + u - v;
			array[vId+2].position = box.Middle() + main + u + v;
			array[vId+3].position = box.Middle() + main - u + v;

			array[vId+4].position = box.Middle() - main - u - v;
			array[vId+5].position = box.Middle() - main + u - v;
			array[vId+6].position = box.Middle() - main + u + v;
			array[vId+7].position = box.Middle() - main - u + v;

			array[vId+0].texel = vsVector2D(0.f,0.f);
			array[vId+1].texel = vsVector2D(1.f,0.f);
			array[vId+2].texel = vsVector2D(1.f,1.f);
			array[vId+3].texel = vsVector2D(0.f,1.f);

			array[vId+4].texel = vsVector2D(1.f,0.f);
			array[vId+5].texel = vsVector2D(0.f,0.f);
			array[vId+6].texel = vsVector2D(0.f,1.f);
			array[vId+7].texel = vsVector2D(1.f,1.f);

			for ( int i = 0; i < 4; i++ )
			{
				array[vId+i].normal = normalVector[dim];
				array[vId+i+4].normal = -normalVector[dim];
			}


			triList[tId++] = vId+2;
			triList[tId++] = vId+1;
			triList[tId++] = vId+0;
			triList[tId++] = vId+0;
			triList[tId++] = vId+3;
			triList[tId++] = vId+2;

			triList[tId++] = vId+4;
			triList[tId++] = vId+5;
			triList[tId++] = vId+6;
			triList[tId++] = vId+6;
			triList[tId++] = vId+7;
			triList[tId++] = vId+4;
		}
	}

	buffer->SetArray( array, arraySize );
	vsDeleteArray( array );

	vsDisplayList *list = new vsDisplayList(512);
	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->BindBuffer(buffer);
	list->TriangleListArray(triList,36);
	list->ClearArrays();

	vsFragment *fragment = new vsFragment;
	fragment->AddBuffer(buffer);
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}


vsFragment *	vsMakeOutlineBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride )
{
	vsVector3D va[8];
	float dx = box.Width() * 0.5f;
	float dy = box.Height() * 0.5f;
	float dz = box.Depth() * 0.5f;

	vsVector3D delta[8] =
	{
		vsVector2D(-dx,-dy),
		vsVector2D( dx,-dy),
		vsVector2D( dx, dy),
		vsVector2D(-dx, dy)
	};

	for ( int i = 0; i < 4; i++ )
	{
		va[i] = va[i+4] = box.Middle() + delta[i];
		va[i].z -= dz;
		va[i+4].z += dz;
	}
	int ll[24] =
	{
		0,1,
		1,2,
		2,3,
		3,0,
		4,5,
		5,6,
		6,7,
		7,4,
		0,4,
		1,5,
		2,6,
		3,7
	};

	vsDisplayList *list = new vsDisplayList(512);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,8);
	list->LineListArray(ll,24);
	list->ClearArrays();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeSolidBox2D_AtOffset( const vsBox2D &box, const vsVector3D &offset, const vsString &material, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	for ( int i = 0; i < 4; i++ )
		va[i] += offset;
/*	vsVector2D tex[4] =
	{
		vsVector2D( 0.f, 0.f),
		vsVector2D( 1.f, 0.f),
		vsVector2D( 0.f, 1.f),
		vsVector2D( 1.f, 1.f)
	};*/
	int ts[4] =
	{
		0,2,1,3
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
//	list->TexelArray(tex,4);
	list->TriangleStripArray(ts,4);
	list->ClearVertexArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeSolidBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride )
{
	return vsMakeSolidBox2D_AtOffset( box, vsVector3D::Zero, material, colorOverride );
}

vsFragment *	vsMakeTexturedBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	vsVector2D tex[4] =
	{
		vsVector2D( 0.f, 0.f),
		vsVector2D( 1.f, 0.f),
		vsVector2D( 0.f, 1.f),
		vsVector2D( 1.f, 1.f)
	};
	int ts[4] =
	{
		0,2,1,3
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
	list->TexelArray(tex,4);
	list->TriangleStripArray(ts,4);
	list->ClearVertexArray();
	list->ClearTexelArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeTexturedBox2D_FlipV( const vsBox2D &box, const vsString &material, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	vsVector2D tex[4] =
	{
		vsVector2D( 0.f, 1.f),
		vsVector2D( 1.f, 1.f),
		vsVector2D( 0.f, 0.f),
		vsVector2D( 1.f, 0.f)
	};
	int ts[4] =
	{
		0,2,1,3
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
	list->TexelArray(tex,4);
	list->TriangleStripArray(ts,4);
	list->ClearVertexArray();
	list->ClearTexelArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeTiledTexturedBox2D( const vsBox2D &box, const vsString &material, float tileSize, const vsAngle &angle, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	vsVector2D tex[4] =
	{
		angle.ApplyRotationTo(vsVector2D( box.GetMin().x, box.GetMin().y) * (1.f / tileSize)),
		angle.ApplyRotationTo(vsVector2D( box.GetMax().x, box.GetMin().y) * (1.f / tileSize)),
		angle.ApplyRotationTo(vsVector2D( box.GetMin().x, box.GetMax().y) * (1.f / tileSize)),
		angle.ApplyRotationTo(vsVector2D( box.GetMax().x, box.GetMax().y) * (1.f / tileSize))
	};
	int ts[4] =
	{
		0,2,1,3
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
	list->TexelArray(tex,4);
	list->TriangleStripArray(ts,4);
	list->ClearVertexArray();
	list->ClearTexelArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeOutlineBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride )
{
	vsVector3D va[4] =
	{
		box.GetMin(),
		vsVector2D(box.GetMax().x,box.GetMin().y),
		vsVector2D(box.GetMin().x,box.GetMax().y),
		box.GetMax()
	};
	int ls[5] =
	{
		0,1,3,2,0
	};

	vsDisplayList *list = new vsDisplayList(128);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,4);
	list->LineStripArray(ls,5);
	list->ClearVertexArray();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}


vsFragment *	vsMakeSolidBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride )
{
	vsRenderBuffer *buffer = new vsRenderBuffer;

	int arraySize = 24;
	vsRenderBuffer::PN *array = new vsRenderBuffer::PN[arraySize];

	vsVector3D normalVector[3] =
	{
		vsVector3D::XAxis,
		vsVector3D::YAxis,
		vsVector3D::ZAxis
	};

	vsVector3D dimVector[3] =
	{
		vsVector3D( box.Width() * 0.5f, 0.f, 0.f ),
		vsVector3D( 0.f, box.Height() * 0.5f, 0.f ),
		vsVector3D( 0.f, 0.f, box.Depth() * 0.5f ),
	};
	int triList[36];
	int tId = 0;

	for ( int dim = 0; dim < 3; dim++ )	// each dimension
	{
		int uDim = dim+1;
		if ( uDim >= 3 )
			uDim = 0;
		int vDim = uDim+1;
		if ( vDim >= 3 )
			vDim = 0;

		vsVector3D main = dimVector[dim];
		vsVector3D u = dimVector[uDim];
		vsVector3D v = dimVector[vDim];

		int vId = dim * 8;
		//		for ( int vert = 0; vert < 4; vert++ )
		{
			array[vId+0].position = box.Middle() + main - u - v;
			array[vId+1].position = box.Middle() + main + u - v;
			array[vId+2].position = box.Middle() + main + u + v;
			array[vId+3].position = box.Middle() + main - u + v;

			array[vId+4].position = box.Middle() - main - u - v;
			array[vId+5].position = box.Middle() - main + u - v;
			array[vId+6].position = box.Middle() - main + u + v;
			array[vId+7].position = box.Middle() - main - u + v;

			for ( int i = 0; i < 4; i++ )
			{
				array[vId+i].normal = normalVector[dim];
				array[vId+i+4].normal = -normalVector[dim];
			}

			triList[tId++] = vId+2;
			triList[tId++] = vId+1;
			triList[tId++] = vId+0;
			triList[tId++] = vId+0;
			triList[tId++] = vId+3;
			triList[tId++] = vId+2;

			triList[tId++] = vId+4;
			triList[tId++] = vId+5;
			triList[tId++] = vId+6;
			triList[tId++] = vId+6;
			triList[tId++] = vId+7;
			triList[tId++] = vId+4;
		}
	}

	buffer->SetArray( array, arraySize );
	vsDeleteArray( array );

	vsDisplayList *list = new vsDisplayList(512);
	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->BindBuffer(buffer);
	list->TriangleListArray(triList,36);
	list->ClearArrays();

	vsFragment *fragment = new vsFragment;
	fragment->AddBuffer(buffer);
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}

vsFragment *	vsMakeTexturedBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride )
{
	vsRenderBuffer *buffer = new vsRenderBuffer;

	int arraySize = 24;
	vsRenderBuffer::PNT *array = new vsRenderBuffer::PNT[arraySize];

	vsVector3D normalVector[3] =
	{
		vsVector3D::XAxis,
		vsVector3D::YAxis,
		vsVector3D::ZAxis
	};

	vsVector3D dimVector[3] =
	{
		vsVector3D( box.Width() * 0.5f, 0.f, 0.f ),
		vsVector3D( 0.f, box.Height() * 0.5f, 0.f ),
		vsVector3D( 0.f, 0.f, box.Depth() * 0.5f ),
	};
	int triList[36];
	int tId = 0;

	for ( int dim = 0; dim < 3; dim++ )	// each dimension
	{
		int uDim = dim+1;
		if ( uDim >= 3 )
			uDim = 0;
		int vDim = uDim+1;
		if ( vDim >= 3 )
			vDim = 0;

		vsVector3D main = dimVector[dim];
		vsVector3D u = dimVector[uDim];
		vsVector3D v = dimVector[vDim];

		int vId = dim * 8;
		//		for ( int vert = 0; vert < 4; vert++ )
		{
			array[vId+0].position = box.Middle() + main - u - v;
			array[vId+1].position = box.Middle() + main + u - v;
			array[vId+2].position = box.Middle() + main + u + v;
			array[vId+3].position = box.Middle() + main - u + v;

			array[vId+4].position = box.Middle() - main - u - v;
			array[vId+5].position = box.Middle() - main + u - v;
			array[vId+6].position = box.Middle() - main + u + v;
			array[vId+7].position = box.Middle() - main - u + v;

			array[vId+0].texel = vsVector2D(0.f,0.f);
			array[vId+1].texel = vsVector2D(1.f,0.f);
			array[vId+2].texel = vsVector2D(1.f,1.f);
			array[vId+3].texel = vsVector2D(0.f,1.f);

			array[vId+4].texel = vsVector2D(1.f,0.f);
			array[vId+5].texel = vsVector2D(0.f,0.f);
			array[vId+6].texel = vsVector2D(0.f,1.f);
			array[vId+7].texel = vsVector2D(1.f,1.f);

			for ( int i = 0; i < 4; i++ )
			{
				array[vId+i].normal = normalVector[dim];
				array[vId+i+4].normal = -normalVector[dim];
			}


			triList[tId++] = vId+2;
			triList[tId++] = vId+1;
			triList[tId++] = vId+0;
			triList[tId++] = vId+0;
			triList[tId++] = vId+3;
			triList[tId++] = vId+2;

			triList[tId++] = vId+4;
			triList[tId++] = vId+5;
			triList[tId++] = vId+6;
			triList[tId++] = vId+6;
			triList[tId++] = vId+7;
			triList[tId++] = vId+4;
		}
	}

	buffer->SetArray( array, arraySize );
	vsDeleteArray( array );

	vsDisplayList *list = new vsDisplayList(512);
	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->BindBuffer(buffer);
	list->TriangleListArray(triList,36);
	list->ClearArrays();

	vsFragment *fragment = new vsFragment;
	fragment->AddBuffer(buffer);
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}


vsFragment *	vsMakeOutlineBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride )
{
	vsVector3D va[8];
	float dx = box.Width() * 0.5f;
	float dy = box.Height() * 0.5f;
	float dz = box.Depth() * 0.5f;

	vsVector3D delta[8] =
	{
		vsVector2D(-dx,-dy),
		vsVector2D( dx,-dy),
		vsVector2D( dx, dy),
		vsVector2D(-dx, dy)
	};

	for ( int i = 0; i < 4; i++ )
	{
		va[i] = va[i+4] = box.Middle() + delta[i];
		va[i].z -= dz;
		va[i+4].z += dz;
	}
	int ll[24] =
	{
		0,1,
		1,2,
		2,3,
		3,0,
		4,5,
		5,6,
		6,7,
		7,4,
		0,4,
		1,5,
		2,6,
		3,7
	};

	vsDisplayList *list = new vsDisplayList(512);

	if ( colorOverride )
	{
		list->SetColor( *colorOverride );
	}
	list->VertexArray(va,8);
	list->LineListArray(ll,24);
	list->ClearArrays();

	vsFragment *fragment = new vsFragment;
	fragment->SetDisplayList(list);
	fragment->SetMaterial( material );

	return fragment;
}
