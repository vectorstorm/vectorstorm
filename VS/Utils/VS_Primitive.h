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

	vsSphere( float radius, int resolution, const vsString &materialName );
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


vsFragment *	vsMakeSolidBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTiledTexturedBox2D( const vsBox2D &box, vsMaterial *material, float tileSize, const vsAngle &angle = vsAngle::Zero, vsColor *colorOverride = NULL );
vsFragment *	vsMakeOutlineBox2D( const vsBox2D &box, vsMaterial *material, vsColor *colorOverride = NULL );

vsFragment *	vsMakeSolidBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTiledTexturedBox2D( const vsBox2D &box, const vsString &material, float tileSize, const vsAngle &angle = vsAngle::Zero, vsColor *colorOverride = NULL );
vsFragment *	vsMakeOutlineBox2D( const vsBox2D &box, const vsString &material, vsColor *colorOverride = NULL );


vsFragment *	vsMakeSolidBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeOutlineBox3D( const vsBox3D &box, vsMaterial *material, vsColor *colorOverride = NULL );

vsFragment *	vsMakeSolidBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeTexturedBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride = NULL );
vsFragment *	vsMakeOutlineBox3D( const vsBox3D &box, const vsString &material, vsColor *colorOverride = NULL );

#endif // VS_PRIMITIVE_H

