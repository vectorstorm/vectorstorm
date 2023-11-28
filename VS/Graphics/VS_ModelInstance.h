/*
 *  VS_ModelInstance.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 21/03/2016
 *  Copyright 2016 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_MODELINSTANCE_H
#define VS_MODELINSTANCE_H

#include "VS/Math/VS_Matrix.h"
#include "VS/Graphics/VS_Color.h"

class vsModel;
class vsModelInstanceGroup;
class vsModelInstanceLodGroup;

struct vsModelInstance
{
private:
	vsMatrix4x4 matrix;
	vsColor color;
	vsModelInstanceGroup *group;
	vsModelInstanceLodGroup *lodGroup;
	int index;       // our ID within the instance array.
	int matrixIndex; // our ID within the matrix array.
	size_t lodLevel;
	bool visible;
	friend class vsModelInstanceGroup;
	friend class vsModelInstanceLodGroup;

public:
	vsModelInstance();
	~vsModelInstance();

	void SetVisible( bool visible );
	bool IsVisible() const { return visible; }
	void SetMatrix( const vsMatrix4x4& mat, const vsColor &color );
	void SetMatrix( const vsMatrix4x4& mat ); // in general, always use the 'Set Matrix' function above if you can!  It's much more optimal to set both matrix and color at the same time!
	void SetColor( const vsColor &color );
	void SetLodLevel( size_t lodLevel );
	int GetLodCount();
	void UpdateGroup();

	vsModel * GetModel();
	vsModelInstanceGroup * GetInstanceGroup() { return group; }
	const vsVector4D& GetPosition() const { return matrix.w; }
	const vsMatrix4x4& GetMatrix() const { return matrix; }
	const vsColor& GetColor() const { return color; }
};

#endif // VS_MODELINSTANCE_H

