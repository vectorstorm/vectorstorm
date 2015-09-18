/*
 *  VS_DynamicMaterial.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 07/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_DYNAMICMATERIAL_H
#define VS_DYNAMICMATERIAL_H

#include "VS_Material.h"
#include "VS_MaterialInternal.h"

class vsColor;
class vsShader;
class vsTexture;

class vsDynamicMaterial: public vsMaterial
{
public:
	vsDynamicMaterial();

	void SetTexture( int i, vsTexture *texture, bool linear = true );
	void SetTexture( int i, const vsString &texture, bool linear = true );
	void SetShadowTexture( vsTexture *texture );
	void SetColor( const vsColor& color );
	void SetSpecularColor( const vsColor& specularColor );
	void SetDrawMode( vsDrawMode drawMode );
	void SetCullingType( CullingType cullingType );
	void SetAlphaRef( float alphaRef );
	void SetDepthBiasConstant( float depthBiasConstant );
	void SetDepthBiasFactor( float depthBiasFactor );
	void SetLayer( int layer );
	void SetStencil( StencilOp stencil );
	void SetAlphaTest( bool alphaTest );
	void SetFog( bool fog );
	void SetZRead( bool zRead );
	void SetZWrite( bool zWrite );
	void SetClampU( bool clampU );
	void SetClampV( bool clampV );
	void SetGlow( bool glow );
	void SetPostGlow( bool postGlow );
	void SetHasColor( bool hasColor );
	void SetBlend( bool blend );

	// NOTE:  Shader must be set LAST, after textures and draw mode are selected
	void SetShader( const vsString &vShader, const vsString &fShader );
	void SetShader( vsShader *shader );
	void SetShader(); // use this one to set the default shader.
};

#endif // VS_DYNAMICMATERIAL_H

