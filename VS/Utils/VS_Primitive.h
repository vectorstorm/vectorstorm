/*
 *  VS_Primitive.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 3/01/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_PRIMITIVE_H
#define VS_PRIMITIVE_H

#include "VS/Graphics/VS_Model.h"

class vsMesh;

class vsSphere : public vsModel
{
public:

	vsSphere( float radius, int resolution, const vsString &materialName, const vsColor& color = c_white, const vsVector3D &offset = vsVector3D::Zero );
};

class vsGem : public vsModel
{
public:

	vsGem( float radius, int resolution, const vsString &materialName );
};

/*
class vsCylinder : public vsModel
{
public:

	vsCylinder( float radius, float height, int resolution, const vsString &materialName );
	virtual ~vsCylinder();
};
*/


// vsFragment *	vsMakeSolidBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride = NULL );
// vsFragment *	vsMakeTexturedBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride = NULL );
// vsFragment *	vsMakeTiledTexturedBox2D( const vsBox2D &box, vsMaterial *material, float tileSize, const vsAngle &angle = vsAngle::Zero, vsColor *colorOverride = NULL );
// vsFragment *	vsMakeOutlineBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride = NULL );


// A "Fringed" box is surrounded by boxes of equal-size around all edges, but
// with alpha set to zero.  This can potentially be useful if you're drawing
// really really small boxes.  But in general, you can ignore this function.  It's
// for special case stuff where you're trying to get a little extra anti-aliasing
// on a very very small box.  :)
vsFragment *	vsMakeFringedBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );

vsFragment *	vsMakeSolidBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox2D( const vsBox2D &box, const vsString &material, const vsVector2D& texScale, vsColor *colorOverride = NULL );
// a variant of the above which flips V coordinates.  Useful if we're going to draw this box in a 3D context, where Y is inverted.
vsFragment *	vsMakeTexturedBox2D_FlipV( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTiledTexturedBox2D( const vsBox2D &box, const vsString &material, float tileSize, const vsAngle &angle = vsAngle::Zero, vsColor *colorOverride = NULL );
vsFragment *	vsMakeOutlineBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );


vsFragment *	vsMakeSolidBox2D_AtOffset( const vsBox2D &box, const vsVector3D &offset, const vsString &material, vsColor *colorOverride = NULL );

vsFragment *	vsMakeSolidBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeOutlineBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride = NULL );

vsFragment *	vsMakeSolidBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeOutlineBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride = NULL );

#endif // VS_PRIMITIVE_H

