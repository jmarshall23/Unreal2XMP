/*=============================================================================
	D3DMaterialState.cpp: Unreal Direct3D rendering interface implementation.
	Copyright 2001 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Daniel Vogel
=============================================================================*/

#include "D3DDrv.h"

//
// Voodoo 3 hack.
//
extern D3DTEXTUREOP DUMMY_MODULATE2X;

//
// FD3DModifierInfo::FD3DModifierInfo
//
FD3DModifierInfo::FD3DModifierInfo()
{
	ModifyTextureTransforms		= 0;
	ModifyFramebufferBlending	= 0;
	ModifyColor					= 0;
	ModifyOpacity				= 0;
	
	Matrix				= FMatrix::Identity;
	TexCoordSource		= TCS_Stream0;
	TexCoordCount		= TCN_2DCoords;
	TexCoordProjected	= 0;

	FrameBufferBlending	= FB_Overwrite;
	ZWrite				= 1;
	ZTest				= 1;
	AlphaTest			= 0;
	TwoSided			= 0;
	AlphaRef			= 0;

	TFactorColor		= 0;
	AlphaBlend			= 0;

	BestFallbackPoint	= NULL;

	Opacity				= NULL;
	OpacityOverrideTexModifier = 0;
}

//
// FD3DModifierInfo::SetDetailTextureScale - apply a texture scaling transform for a detail texture.
//
void FD3DModifierInfo::SetDetailTextureScale( FLOAT Scale )
{
	ModifyTextureTransforms = 1;
	Matrix *= FMatrix (
					FPlane(Scale,0,0,0),
					FPlane(0,Scale,0,0),
					FPlane(0,0,1,0),
					FPlane(0,0,0,1)
				);
}

//
// FD3DMaterialStateStage::FD3DMaterialStateStage
//
FD3DMaterialStateStage::FD3DMaterialStateStage()
{
	appMemcpy( &TextureTransformMatrix, &FMatrix::Identity, sizeof(D3DMATRIX));

	TextureTransformsEnabled	= 0;
	TexCoordIndex				= D3DTSS_TCI_PASSTHRU;
	TexCoordCount				= D3DTTFF_DISABLE;
	TextureAddressU				= D3DTADDRESS_CLAMP;
	TextureAddressV				= D3DTADDRESS_CLAMP;
	TextureAddressW				= D3DTADDRESS_CLAMP;
    TextureMipLODBias           = -1000.f; //!! will be set to DefaultTexMipBias
	Texture		= NULL;
	
	ResultArg	= D3DTA_CURRENT;
	ColorArg0	= D3DTA_CURRENT;
	ColorArg1	= D3DTA_CURRENT;
	ColorArg2	= D3DTA_CURRENT;
	ColorOp		= D3DTOP_SELECTARG1;
	AlphaArg0	= D3DTA_CURRENT;
	AlphaArg1	= D3DTA_CURRENT;
	AlphaArg2	= D3DTA_CURRENT;
	AlphaOp		= D3DTOP_SELECTARG1;
}

//
// FD3DMaterialState::FD3DMaterialState
//
FD3DMaterialState::FD3DMaterialState()
{
	PixelShader			= PS_None;
	AlphaBlending		= 0;
	AlphaTest			= 0;
	AlphaRef			= 0;
	ZTest				= 1;
	ZWrite				= 1;
	TwoSided			= 0;
	FillMode			= FM_Solid;
	SrcBlend			= D3DBLEND_ONE;
	DestBlend			= D3DBLEND_ZERO;
	TFactorColor		= 0;
	StagesUsed			= 0;
	OverrideFogColor	= 0;
	PixelShader2		= NULL;
	VertexShader2		= NULL;
}

/*----------------------------------------------------------------------------
	SetMaterialShader.
----------------------------------------------------------------------------*/

#define NEWPASS()															\
		CurrentState->MaterialPasses[PassesUsed]->StagesUsed = StagesUsed;					\
		PassesUsed++;														\
		StagesUsed = 0;														\
		CurrentState->MaterialPasses[PassesUsed] = MaterialStatePool.AllocateState(&DefaultPass); \
		CurrentState->MaterialPasses[PassesUsed]->ZWrite = 0;

//
//
//

void FD3DRenderInterface::HandleDetail( UBitmapMaterial* DetailBitmap, INT& PassesUsed, INT& StagesUsed, INT& FreeStagesUsed, FD3DModifierInfo InModifierInfo, UBOOL SinglePassOnly )
{
	INT MaxTextureStages = RenDev->DeviceCaps8.MaxSimultaneousTextures;
	if( StagesUsed >= MaxTextureStages+FreeStagesUsed )
	{
#if 1
		// No room for detail texture in single pass, start a new render pass if possible
		if( (!InModifierInfo.ModifyFramebufferBlending || InModifierInfo.FrameBufferBlending == FB_Overwrite) && !SinglePassOnly )
		{
			NEWPASS();
			FD3DMaterialStateStage& Stage = CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed];
			SetShaderBitmap( Stage, DetailBitmap );
			Stage.ColorArg1 = D3DTA_TEXTURE;
			Stage.AlphaArg1 = D3DTA_DIFFUSE;
			Stage.ColorOp   = D3DTOP_SELECTARG1;
			Stage.AlphaOp   = D3DTOP_SELECTARG1;
            Stage.TextureMipLODBias = RenDev->DetailTexMipBias;
			ApplyTexModifier( Stage, &InModifierInfo );
			StagesUsed++;
		
			// modulate with framebuffer.
			CurrentState->MaterialPasses[PassesUsed]->SrcBlend			= D3DBLEND_DESTCOLOR;
			CurrentState->MaterialPasses[PassesUsed]->DestBlend			= D3DBLEND_SRCCOLOR;
			CurrentState->MaterialPasses[PassesUsed]->AlphaBlending		= 1;
			CurrentState->MaterialPasses[PassesUsed]->ZWrite			= 0;
			CurrentState->MaterialPasses[PassesUsed]->ZTest				= 1;
			CurrentState->MaterialPasses[PassesUsed]->OverrideFogColor	= 1;
			CurrentState->MaterialPasses[PassesUsed]->OverriddenFogColor= FColor( 127, 127, 127, 0 );
			CurrentState->MaterialPasses[PassesUsed]->TwoSided			= InModifierInfo.TwoSided;

            //Direct3DDevice8->SetTextureStageState( Stage, D3DTSS_MIPMAPLODBIAS, *(DWORD*)&LodBias );
		}
#endif
	}
	else 
	{
		// Add detail texture as a single pass
		FD3DMaterialStateStage& Stage = CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed];
		SetShaderBitmap( Stage, DetailBitmap );
		Stage.ColorArg1 = D3DTA_TEXTURE;
		Stage.ColorArg2 = D3DTA_CURRENT;
		Stage.AlphaArg1 = D3DTA_CURRENT;
		Stage.ColorOp   = DUMMY_MODULATE2X;
		Stage.AlphaOp   = D3DTOP_SELECTARG1;
        Stage.TextureMipLODBias = RenDev->DetailTexMipBias;
		ApplyTexModifier( Stage, &InModifierInfo );
		StagesUsed++;
	}
}

//
// Handle lightmap in single pass
//
void FD3DRenderInterface::HandleLightmap_SP( FD3DMaterialStateStage& Stage, FD3DTexture* Lightmap )
{
	Stage.AlphaArg1	= D3DTA_CURRENT;
	Stage.ColorArg1	= D3DTA_TEXTURE;
	Stage.ColorArg2	= D3DTA_CURRENT;
	Stage.AlphaOp	= D3DTOP_SELECTARG1;
	Stage.ColorOp	= (CurrentState->LightingModulate2X && !CurrentState->UseStaticLighting) ? DUMMY_MODULATE2X : D3DTOP_MODULATE;
	Stage.Texture	= Lightmap;
	Stage.TextureAddressU			= D3DTADDRESS_CLAMP; 
	Stage.TextureAddressV			= D3DTADDRESS_CLAMP; 
	Stage.TexCoordIndex				= TCS_Stream1;
	Stage.TextureTransformsEnabled	= 0;
}

//
// Patch in the diffuse lighting into the first texture stage
//
void FD3DRenderInterface::HandleDiffuse_Patch( FD3DMaterialStateStage& Stage, UBOOL Modulate2X )
{
	Stage.ColorArg2	= D3DTA_DIFFUSE;
	Stage.AlphaArg2	= D3DTA_DIFFUSE;
	Stage.ColorOp	= CurrentState->LightingModulate2X ? DUMMY_MODULATE2X : D3DTOP_MODULATE;
	Stage.AlphaOp	= D3DTOP_MODULATE;
}

void FD3DRenderInterface::HandleDiffuse_SP( FD3DMaterialStateStage& Stage )
{
	Stage.ColorArg1	= D3DTA_DIFFUSE;
	Stage.ColorArg2 = CurrentState->LightingModulate2X ? D3DTA_DIFFUSE : Stage.ColorArg2;
	Stage.ColorOp	= CurrentState->LightingModulate2X ? D3DTOP_ADD : D3DTOP_SELECTARG1;
	Stage.AlphaArg1	= D3DTA_DIFFUSE;
	Stage.AlphaOp	= D3DTOP_SELECTARG1;
}

void FD3DRenderInterface::HandleDiffuse_Stage( FD3DMaterialStateStage& Stage, UBOOL Modulate2X )
{
	Stage.ColorArg1	= D3DTA_DIFFUSE;
	Stage.ColorArg2	= D3DTA_CURRENT;
	Stage.ColorOp	= CurrentState->LightingModulate2X ? DUMMY_MODULATE2X : D3DTOP_MODULATE;
	Stage.AlphaArg2	= D3DTA_CURRENT;
	Stage.AlphaOp	= D3DTOP_SELECTARG2;
}

//
// Blend using TFactorColor
//
void FD3DRenderInterface::HandleTFactor_SP( FD3DMaterialStateStage& Stage )
{
	Stage.AlphaArg1	= D3DTA_CURRENT;
	Stage.AlphaArg2 = D3DTA_TFACTOR;
	Stage.ColorArg1	= D3DTA_CURRENT;
	Stage.ColorArg2	= D3DTA_TFACTOR;
	Stage.AlphaOp	= D3DTOP_MODULATE;
	Stage.ColorOp	= CurrentState->LightingModulate2X ? DUMMY_MODULATE2X : D3DTOP_MODULATE;
	Stage.Texture	= NULL;
}

//
// Handle lightmap is it own pass
//
void FD3DRenderInterface::HandleLighting_MP( FD3DMaterialStateStage& Stage, FD3DTexture* Lightmap, UBOOL UseDiffuse )
{
	Stage.AlphaArg1 = D3DTA_DIFFUSE;
	Stage.ColorArg1 = D3DTA_TEXTURE;
	Stage.ColorArg2 = D3DTA_DIFFUSE;
	Stage.AlphaOp	= D3DTOP_SELECTARG1;

	if( !Lightmap )
		Stage.ColorOp = D3DTOP_SELECTARG2;
	else
	if( !UseDiffuse )
		Stage.ColorOp = D3DTOP_SELECTARG1;
	else
		Stage.ColorOp = CurrentState->LightingModulate2X ? DUMMY_MODULATE2X : D3DTOP_MODULATE;

	Stage.Texture	= Lightmap;
	Stage.TextureAddressU = D3DTADDRESS_WRAP; 
	Stage.TextureAddressV = D3DTADDRESS_WRAP; 
	
	Stage.TexCoordIndex = TCS_Stream1;
	Stage.TextureTransformsEnabled = 0;
}

void FD3DRenderInterface::SetShaderBitmap( FD3DMaterialStateStage& Stage, UBitmapMaterial* BitmapMaterial )
{
	if( BitmapMaterial )
	{
		Stage.Texture = CacheTexture( BitmapMaterial->Get(Viewport->CurrentTime,Viewport)->GetRenderInterface() );
		if( Stage.Texture->Direct3DCubeTexture8 )
		{
			// setup cubemap clamping
			Stage.TextureAddressU = RenDev->CubemapTextureAddressing;
			Stage.TextureAddressV = RenDev->CubemapTextureAddressing;
		}
		else
		{
			switch( BitmapMaterial->UClampMode )
			{
			case TC_Wrap:	Stage.TextureAddressU = D3DTADDRESS_WRAP;  break;
			case TC_Clamp:	Stage.TextureAddressU = D3DTADDRESS_CLAMP; break;
			}
			switch( BitmapMaterial->VClampMode )
			{
			case TC_Wrap:	Stage.TextureAddressV = D3DTADDRESS_WRAP;  break;
			case TC_Clamp:	Stage.TextureAddressV = D3DTADDRESS_CLAMP; break;
			}
		}
		Stage.TextureAddressW = RenDev->CubemapTextureAddressing;
	}
	else
		Stage.Texture = NULL;
}

//
// ApplyFinalBlend
//
void FD3DRenderInterface::ApplyFinalBlend( FD3DModifierInfo* InModifierInfo )
{
	guard(FD3DRenderInterface::ApplyFinalBlend);

	FColor	FogColor = FColor(0,0,0,1);

	switch( InModifierInfo->FrameBufferBlending )
	{
	case FB_Overwrite :
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ZERO;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 0;
		break;
	case FB_AlphaBlend:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_SRCALPHA;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_INVSRCALPHA;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		break;
	case FB_AlphaModulate_MightNotFogCorrectly:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_INVSRCALPHA;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
		CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
		break;
	case FB_Modulate:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_DESTCOLOR;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_SRCCOLOR;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
		CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 127, 127, 127, 0 );
		break;
	case FB_Translucent:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ONE;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
		CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
		break;
	case FB_Darken:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ZERO;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_INVSRCCOLOR;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
		CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
		break;
	case FB_Brighten:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_INVSRCCOLOR;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
		CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
		break;
	case FB_Invisible:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ZERO;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ONE;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
		CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
		break;
	case FB_ShadowBlend:
		CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_SRCALPHA;
		CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ONE;
		CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
		CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
	}

	CurrentState->MaterialPasses[0]->ZWrite		= InModifierInfo->ZWrite;
	CurrentState->MaterialPasses[0]->ZTest		= InModifierInfo->ZTest;
	CurrentState->MaterialPasses[0]->TwoSided	= InModifierInfo->TwoSided;
	CurrentState->MaterialPasses[0]->AlphaTest	= InModifierInfo->AlphaTest;
	CurrentState->MaterialPasses[0]->AlphaRef	= InModifierInfo->AlphaRef;

	unguard;
}

//
// ApplyTexModifier
//
void FD3DRenderInterface::ApplyTexModifier( FD3DMaterialStateStage& Stage, FD3DModifierInfo* ModifierInfo )
{
	guard(FD3DRenderInterface::ApplyTexModifier);
	
	if( ModifierInfo->ModifyTextureTransforms )
	{
		// Apply texture coordinate transforms
		switch( ModifierInfo->TexCoordCount )
		{
		case TCN_2DCoords:
			Stage.TexCoordCount = D3DTTFF_COUNT2;
			break;
		case TCN_3DCoords:
			Stage.TexCoordCount = D3DTTFF_COUNT3;
			break;
		case TCN_4DCoords:
			Stage.TexCoordCount = D3DTTFF_COUNT4;
			break;
		default:
			appErrorf(TEXT("Unknown TexCoordCount %d"), ModifierInfo->TexCoordCount );
		}		
		if( ModifierInfo->TexCoordProjected )
			Stage.TexCoordCount |= D3DTTFF_PROJECTED;

		if( RenDev->Is3dfx )
		{
			switch( ModifierInfo->TexCoordSource )
			{		
			case TCS_CameraEnvMapCoords:
			case TCS_WorldEnvMapCoords:
				ModifierInfo->TexCoordSource = TCS_WorldCoords;
				break;
			}

			if( !ModifierInfo->TexCoordProjected )
				Stage.TexCoordCount = D3DTTFF_COUNT2;
		}

		switch( ModifierInfo->TexCoordSource )
		{
		case TCS_NoChange:
			Stage.TexCoordIndex = D3DTSS_TCI_PASSTHRU;
			Stage.TextureTransformsEnabled = 0;
			break;
		case TCS_Stream0:
		case TCS_Stream1:
		case TCS_Stream2:
		case TCS_Stream3:
		case TCS_Stream4:
		case TCS_Stream5:
		case TCS_Stream6:
		case TCS_Stream7:
			{
				Stage.TexCoordIndex = ModifierInfo->TexCoordSource;
				if( ModifierInfo->TexCoordProjected || ModifierInfo->Matrix != FMatrix::Identity )
				{
					Stage.TextureTransformMatrix = *(D3DMATRIX*)&ModifierInfo->Matrix;
					Stage.TextureTransformsEnabled = 1;
				}
				else
					Stage.TextureTransformsEnabled = 0;
			}
			break;
		case TCS_WorldCoords:
			{
				D3DXMATRIX	CameraToTexture,
							InvViewMatrix;
				D3DXMatrixInverse(&InvViewMatrix,NULL,(D3DXMATRIX*) &CurrentState->WorldToCamera);
				if( ModifierInfo->Matrix != FMatrix::Identity )
				{
					D3DXMatrixMultiply(&CameraToTexture,&InvViewMatrix,(D3DXMATRIX*)&ModifierInfo->Matrix);		
					Stage.TextureTransformMatrix = CameraToTexture;		
				}
				else
					Stage.TextureTransformMatrix = InvViewMatrix;		
				Stage.TextureTransformsEnabled = 1;
				Stage.TexCoordIndex = D3DTSS_TCI_CAMERASPACEPOSITION;				
			}
			break;
		case TCS_CameraCoords:
			{
				if( ModifierInfo->Matrix != FMatrix::Identity )
				{
					Stage.TextureTransformMatrix = *(D3DMATRIX*)&ModifierInfo->Matrix;		
					Stage.TextureTransformsEnabled = 1;
				}
				else
					Stage.TextureTransformsEnabled = 0;
				Stage.TexCoordIndex = D3DTSS_TCI_CAMERASPACEPOSITION;				
			}
			break;
		case TCS_WorldEnvMapCoords:
			{
				D3DXMATRIX WorldToTexture;
				if( ModifierInfo->Matrix != FMatrix::Identity )
				{
					D3DXMatrixMultiply(&WorldToTexture, (D3DXMATRIX*)&CurrentState->WorldToCamera, (D3DXMATRIX*)&ModifierInfo->Matrix);
					D3DXMatrixTranspose(&WorldToTexture, &WorldToTexture);
				}
				else
					D3DXMatrixTranspose(&WorldToTexture, (D3DXMATRIX*)&CurrentState->WorldToCamera);
				Stage.TextureTransformMatrix = WorldToTexture;
				Stage.TextureTransformsEnabled = 1;
				Stage.TexCoordIndex = D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR;				
			}
			break;
		case TCS_CameraEnvMapCoords:
			{
				if( ModifierInfo->Matrix != FMatrix::Identity )
				{
					Stage.TextureTransformMatrix = *(D3DMATRIX*)&ModifierInfo->Matrix;		
					Stage.TextureTransformsEnabled = 1;
				}
				else
					Stage.TextureTransformsEnabled = 0;
				Stage.TexCoordIndex = D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR;				
			}
			break;			
		default:
			appErrorf( TEXT("Unknown TexCoordSource %d"), ModifierInfo->TexCoordSource );
		}
	}

	unguard;
}

//
// Handle opacity bitmap
//
void FD3DRenderInterface::HandleOpacityBitmap( FD3DMaterialStateStage& Stage, UBitmapMaterial* Bitmap, UBOOL ModulateAlpha )
{
	SetShaderBitmap( Stage, Bitmap );
	Stage.AlphaArg1 = D3DTA_TEXTURE;
	Stage.AlphaArg2 = D3DTA_CURRENT;
	Stage.AlphaOp   = ModulateAlpha ? D3DTOP_MODULATE : D3DTOP_SELECTARG1;
	Stage.ColorArg2 = D3DTA_CURRENT;
	Stage.ColorOp   = D3DTOP_SELECTARG2;
}

//
// Handle vertex opacity
//
void FD3DRenderInterface::HandleVertexOpacity( FD3DMaterialStateStage& Stage, UVertexColor* VertexColor )
{
	Stage.AlphaArg1 = D3DTA_DIFFUSE;
	Stage.AlphaOp = D3DTOP_SELECTARG1;
	Stage.ColorArg2 = D3DTA_CURRENT;
	Stage.ColorOp = D3DTOP_SELECTARG2;
}

//
// Handle self-illumination in a single pass
//
void FD3DRenderInterface::HandleSelfIllumination_SP( FD3DMaterialStateStage& Stage, UBitmapMaterial* Bitmap )
{
	SetShaderBitmap( Stage, Bitmap );
	Stage.AlphaArg1 = D3DTA_CURRENT;
	Stage.AlphaOp   = D3DTOP_SELECTARG1;
	Stage.ColorArg1	= D3DTA_TEXTURE;
	Stage.ColorArg2	= D3DTA_CURRENT;
	Stage.ColorOp   = D3DTOP_BLENDCURRENTALPHA;
}

//
// Handle specular in a single pass
//
void FD3DRenderInterface::HandleSpecular_SP( FD3DMaterialStateStage& Stage, UBitmapMaterial* Bitmap, UBOOL UseSpecularity, UBOOL UseConstantSpecularity )
{
	SetShaderBitmap( Stage, Bitmap );

	Stage.AlphaArg1 = D3DTA_CURRENT;
	Stage.AlphaOp   = D3DTOP_SELECTARG1;

	if( !UseSpecularity )
	{
		Stage.ColorArg1 = D3DTA_CURRENT;
		Stage.ColorArg2 = D3DTA_TEXTURE;
		Stage.ColorOp   = D3DTOP_ADD;
	}
	else
	if( UseConstantSpecularity )
	{
		Stage.ColorArg1 = D3DTA_TFACTOR;
		Stage.ColorArg2 = D3DTA_TEXTURE;
		Stage.ColorArg0 = D3DTA_CURRENT;
		Stage.ColorOp   = D3DTOP_MULTIPLYADD;
	}
	else
	{
		Stage.ColorArg1 = D3DTA_CURRENT;
		Stage.ColorArg2 = D3DTA_TEXTURE;
		if( RenDev->DeviceCaps8.TextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR )
			Stage.ColorOp = D3DTOP_MODULATEALPHA_ADDCOLOR;
		else
			Stage.ColorOp = DUMMY_MODULATE2X;
	}
}

//
// Handle complex material
//
UBOOL FD3DRenderInterface::HandleCombinedMaterial( UMaterial* InMaterial, INT& PassesUsed, INT& StagesUsed, INT& FreeStagesUsed, FD3DModifierInfo ModifierInfo, UBOOL InvertOutputAlpha, FString* ErrorString, UMaterial** ErrorMaterial )
{
	guard(FD3DRenderInterface::HandleCombinedMaterial);

	UVertexColor* VertexColor;
	UConstantMaterial* ConstantMaterial;
	UBitmapMaterial* BitmapMaterial;
	UCombiner* Combiner;

	INT MaxTextureStages = RenDev->DeviceCaps8.MaxSimultaneousTextures;
	INT MaxBlendStages   = RenDev->DeviceCaps8.MaxTextureBlendStages;
	INT MaxFreeStages = MaxBlendStages - MaxTextureStages;

	if( !InMaterial )
	{
		return 1;
	}
	else
	if( (VertexColor=CheckMaterial<UVertexColor>(this, InMaterial, &ModifierInfo)) != NULL )
	{
		if( StagesUsed >= MaxTextureStages+MaxFreeStages )
		{
			if(ErrorString ) *ErrorString = TEXT("No stages left for vertex color");
			if(ErrorMaterial) *ErrorMaterial = InMaterial;
			return 0;
		}
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG2;
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_DIFFUSE;
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG1;
		StagesUsed++;
		if( MaxFreeStages )
			FreeStagesUsed++;
		return 1;
	}
	else
	if( (ConstantMaterial=CheckMaterial<UConstantMaterial>(this, InMaterial, &ModifierInfo)) != NULL )
	{
		FColor	Color = ConstantMaterial->GetColor(Viewport->Actor->Level->TimeSeconds);

		if(CurrentState->MaterialPasses[PassesUsed]->TFactorColor == 0 || CurrentState->MaterialPasses[PassesUsed]->TFactorColor == Color.DWColor())
		{
			if( StagesUsed >= MaxTextureStages+MaxFreeStages )
			{
				if(ErrorString ) *ErrorString = TEXT("No stages left for constant color");
				if(ErrorMaterial) *ErrorMaterial = InMaterial;
				return 0;
			}
			CurrentState->MaterialPasses[PassesUsed]->TFactorColor = Color.DWColor();
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TFACTOR;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_TFACTOR;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG1;
			if( MaxFreeStages )
				FreeStagesUsed++;
			StagesUsed++;
			return 1;
		}
		else
		{
			if(ErrorString) *ErrorString = TEXT("Only one ConstantMaterial may be used per material.");
			if(ErrorMaterial) *ErrorMaterial = InMaterial;
			return 0;
		}
	}
	else
	if( (BitmapMaterial=CheckMaterial<UBitmapMaterial>(this, InMaterial, &ModifierInfo)) != NULL )
	{
		// handle simple case
		if( StagesUsed >= MaxTextureStages+FreeStagesUsed )
		{
			if(ErrorString ) *ErrorString = TEXT("No stages left for BitmapMaterial");
			if(ErrorMaterial) *ErrorMaterial = InMaterial;
			return 0;
		}
		SetShaderBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], BitmapMaterial );
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = InvertOutputAlpha ? (D3DTA_TEXTURE | D3DTA_COMPLEMENT) : D3DTA_TEXTURE;
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
		CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SELECTARG1;
		ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &ModifierInfo );
		StagesUsed++;
		return 1;
	}
	else
	if( (Combiner=CheckMaterial<UCombiner>(this, InMaterial, &ModifierInfo)) != NULL )
	{
		UBitmapMaterial* BitmapMaterial1 = CheckMaterial<UBitmapMaterial>(this, Combiner->Material1);
		UBitmapMaterial* BitmapMaterial2 = CheckMaterial<UBitmapMaterial>(this, Combiner->Material2);

		if( !Combiner->Material1 )
		{
			if(ErrorString ) *ErrorString = TEXT("Combiner must specify Material1");
			if(ErrorMaterial) *ErrorMaterial = InMaterial;
			return 0;
		}
		else
		if( !BitmapMaterial1 && !BitmapMaterial2 )
		{
			if(ErrorString ) *ErrorString = TEXT("Either Material1 or Material2 must be a simple bitmap material.");
			if(ErrorMaterial) *ErrorMaterial = InMaterial;
			return 0;
		}


		// Swap Material1/2 if it makes our life easier.
		UMaterial* Material1;
		UMaterial* Material2;
		UBOOL Swapped;

		// Rearrange arguments to ensure simple bitmap is the second operand.
		if( !BitmapMaterial2 )
		{
			Material1 = Combiner->Material2;
			Material2 = Combiner->Material1;
			Swapped = 1;
		}
		else
		{
			if( Combiner->InvertMask && BitmapMaterial1 && Combiner->Mask==Combiner->Material2 )
			{
				// Swap the arguments if they're both bitmaps, and we're also using arg2 as an inverted mask
				Material1 = Combiner->Material2;
				Material2 = Combiner->Material1;
				Swapped = 1;
			}
			else
			{
				// No need to swap
				Material1 = Combiner->Material1;
				Material2 = Combiner->Material2;
				Swapped = 0;
			}
		}

		// Process the complex first operand.
		if( !HandleCombinedMaterial( Material1, PassesUsed, StagesUsed, FreeStagesUsed, ModifierInfo, Combiner->Mask==Material1 && Combiner->InvertMask, ErrorString, ErrorMaterial ) )
			return 0;

		UBOOL	UseTextureAlpha = 0,
				UseFactorAlpha = 0,
				UseDiffuseAlpha = 0;
		if( Combiner->Mask )
		{
			if( Combiner->Mask == Material2 )
			{
				if( Combiner->InvertMask )
				{
					if( StagesUsed >= MaxTextureStages+FreeStagesUsed )
					{
						if(ErrorString ) *ErrorString = Swapped ? TEXT("No stages left for inverted Mask which is the same as Material1") : TEXT("No stages left for inverted Mask which is the same as Material2");
						if(ErrorMaterial) *ErrorMaterial = InMaterial;
						return 0;
					}

					// Unfortunately we need to load the texture again to support an inverted mask for argument 2, when argument1 is complex
					FD3DModifierInfo MaskModifier = ModifierInfo;
					SetShaderBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CheckMaterial<UBitmapMaterial>(this, Combiner->Mask, &MaskModifier) );
					ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &MaskModifier );
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_COMPLEMENT | D3DTA_TEXTURE;
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SELECTARG1;
					StagesUsed++;

					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT | (InvertOutputAlpha ? D3DTA_COMPLEMENT : 0);
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
				}
				else
				{
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE | (InvertOutputAlpha ? D3DTA_COMPLEMENT : 0);
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
					UseTextureAlpha = 1;
			}
			}
			else
			if( Combiner->Mask != Material1 )
			{
				UBitmapMaterial*	MaskBitmap;
				UVertexColor*		VertexColor;
				UConstantMaterial*	ConstantMaterial;

				if( (MaskBitmap=CheckMaterial<UBitmapMaterial>(this, Combiner->Mask)) != NULL )
				{
					// Process alpha channel from Mask Bitmap.
					if( StagesUsed >= MaxTextureStages+FreeStagesUsed )
					{
						if(ErrorString ) *ErrorString = TEXT("No stages left for Mask bitmap");
					if( ErrorMaterial ) *ErrorMaterial = InMaterial;
					return 0;
				}
				FD3DModifierInfo MaskModifier = ModifierInfo;
					SetShaderBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CheckMaterial<UBitmapMaterial>(this, Combiner->Mask, &MaskModifier) );
					ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &MaskModifier );
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = Combiner->InvertMask ? (D3DTA_COMPLEMENT | D3DTA_TEXTURE) : D3DTA_TEXTURE;
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SELECTARG2;
				StagesUsed++;

					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT | (InvertOutputAlpha ? D3DTA_COMPLEMENT : 0);
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
				}
				else if( (VertexColor=CheckMaterial<UVertexColor>(this, Combiner->Mask)) != NULL )
				{
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_DIFFUSE;
					CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
					UseDiffuseAlpha = 1;
				}
				else if( (ConstantMaterial=CheckMaterial<UConstantMaterial>(this, Combiner->Mask)) != NULL)
				{
					FColor	Color = ConstantMaterial->GetColor(Viewport->Actor->Level->TimeSeconds);

					if(CurrentState->MaterialPasses[PassesUsed]->TFactorColor == 0 || CurrentState->MaterialPasses[PassesUsed]->TFactorColor == Color.DWColor())
					{
						CurrentState->MaterialPasses[PassesUsed]->TFactorColor = Color.DWColor();
						CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TFACTOR;
						CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						UseFactorAlpha = 1;
					}
					else
					{
						if(ErrorString) *ErrorString = TEXT("Only one ConstantMaterial may be used per material.");
						if(ErrorMaterial) *ErrorMaterial = InMaterial;
						return 0;
					}
				}
				else
				{
					if( ErrorString ) *ErrorString = TEXT("Combiner Mask must be a bitmap material, a vertex color, a constant color, Material1 or Material2.");
					if( ErrorMaterial ) *ErrorMaterial = InMaterial;
					return 0;
				}
			}
		}

		if( StagesUsed >= MaxTextureStages+FreeStagesUsed )
		{
			if(ErrorString ) *ErrorString = Swapped ? TEXT("No stages left for Bitmap Material1") : TEXT("No stages left for Bitmap Material2");
			if(ErrorMaterial) *ErrorMaterial = InMaterial;
			return 0;
		}

		// Process Material2
		FD3DModifierInfo Material2Modifier = ModifierInfo;
		SetShaderBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CheckMaterial<UBitmapMaterial>(this, Material2, &Material2Modifier) );
		ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &Material2Modifier );

		switch( Combiner->CombineOperation )
		{
		case CO_Use_Color_From_Material1:
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = Swapped ? D3DTOP_SELECTARG1 : D3DTOP_SELECTARG2;
			break;
		case CO_Use_Color_From_Material2:
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = Swapped ? D3DTOP_SELECTARG2 : D3DTOP_SELECTARG1;
			break;
		case CO_Multiply:
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = Combiner->Modulate4X ? (D3DTOP_MODULATE4X) : (Combiner->Modulate2X ? DUMMY_MODULATE2X : D3DTOP_MODULATE);
			break;
		case CO_Add:
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_ADD;
			break;
		case CO_Subtract:
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = Swapped ? D3DTA_CURRENT : D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = Swapped ? D3DTA_TEXTURE : D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SUBTRACT;
			break;
		case CO_AlphaBlend_With_Mask:
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = Swapped ? D3DTA_CURRENT : D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = Swapped ? D3DTA_TEXTURE : D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = UseTextureAlpha ? D3DTOP_BLENDTEXTUREALPHA : UseFactorAlpha ? D3DTOP_BLENDFACTORALPHA : UseDiffuseAlpha ? D3DTOP_BLENDDIFFUSEALPHA : D3DTOP_BLENDCURRENTALPHA;
			break;
		case CO_Add_With_Mask_Modulation:
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = Swapped ? D3DTA_CURRENT : D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg2 = Swapped ? D3DTA_TEXTURE : D3DTA_CURRENT;
			if( RenDev->DeviceCaps8.TextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR )
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_MODULATEALPHA_ADDCOLOR;
			else
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_ADD;
			break;
		}
		
		switch( Combiner->AlphaOperation )
		{
		case AO_Use_Mask:
			break;
		case AO_Multiply:
			if( Combiner->Mask && Combiner->Mask != Combiner->Material1 && Combiner->Mask != Combiner->Material2 )
			{
				if( ErrorString ) *ErrorString = TEXT("Combiner Mask must be Material1, or Material2 or None when using Alpha Operation AO_Multiply.");
				if( ErrorMaterial ) *ErrorMaterial = InMaterial;
				return 0;
			}
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_MODULATE;
			if( InvertOutputAlpha )
			{
				StagesUsed++;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SELECTARG1;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT | D3DTA_COMPLEMENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
			}
			break;
		case AO_Add:
			if( Combiner->Mask && Combiner->Mask != Combiner->Material1 && Combiner->Mask != Combiner->Material2 )
			{
				if( ErrorString ) *ErrorString = TEXT("Combiner Mask must be Material1, or Material2 or None when using Alpha Operation AO_Add.");
				if( ErrorMaterial ) *ErrorMaterial = InMaterial;
				return 0;
			}
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_ADD;
			if( InvertOutputAlpha )
			{
				StagesUsed++;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SELECTARG1;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT | D3DTA_COMPLEMENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
			}
			break;
		case AO_Use_Alpha_From_Material1:
			if( Combiner->Mask && Combiner->Mask != Combiner->Material1 && Combiner->Mask != Combiner->Material2 )
			{
				if( ErrorString ) *ErrorString = TEXT("Combiner Mask must be Material1, or Material2 or None when using Alpha Operation AO_Use_Alpha_From_Material1.");
				if( ErrorMaterial ) *ErrorMaterial = InMaterial;
				return 0;
			}
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = Swapped ? D3DTOP_SELECTARG1 : D3DTOP_SELECTARG2;
			if( InvertOutputAlpha )
			{
				StagesUsed++;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SELECTARG1;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT | D3DTA_COMPLEMENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
			}
			break;
		case AO_Use_Alpha_From_Material2:
			if( Combiner->Mask && Combiner->Mask != Combiner->Material1 && Combiner->Mask != Combiner->Material2 )
			{
				if( ErrorString ) *ErrorString = TEXT("Combiner Mask must be Material1, or Material2 or None when using Alpha Operation AO_Use_Alpha_From_Material2.");
				if( ErrorMaterial ) *ErrorMaterial = InMaterial;
				return 0;
			}
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
			CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = Swapped ? D3DTOP_SELECTARG2 : D3DTOP_SELECTARG1;
			if( InvertOutputAlpha )
			{
				StagesUsed++;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp   = D3DTOP_SELECTARG1;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT | D3DTA_COMPLEMENT;
				CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp   = D3DTOP_SELECTARG1;
			}
			break;
		}
		StagesUsed++;
	}

	return 1;
	unguard;
}

//
// FD3DRenderInterface::SetShaderMaterial
//
UBOOL FD3DRenderInterface::SetShaderMaterial( UShader* InShader, FD3DModifierInfo BaseModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial )
{
	guard(FD3DRenderInterface::SetShaderMaterial);

	INT MaxTextureStages = RenDev->DeviceCaps8.MaxSimultaneousTextures;
	INT MaxBlendStages   = RenDev->DeviceCaps8.MaxTextureBlendStages;
	INT MaxFreeStages = MaxBlendStages - MaxTextureStages;

	INT StagesUsed = 0;
	INT PassesUsed = 0;
	INT FreeStagesUsed = 0;

	//
	// 1. Process diffuse, opacity and lightmap
	//
	UMaterial* InDiffuse = InShader->Diffuse;

	UBOOL Unlit = 0;
	UBOOL NeedsLightmap	= CurrentState->Lightmap ? 1 : 0;
	UBOOL NeedsDiffuseLight = CurrentState->UseStaticLighting || CurrentState->UseDynamicLighting;
	
	// Check for an unlit material
	if( InShader->SelfIllumination && !InShader->SelfIlluminationMask )
	{
		InDiffuse = InShader->SelfIllumination;
		Unlit = 1;
		NeedsDiffuseLight = 0;
		NeedsLightmap = 0;
	}

	// Check for an opacity override
	UMaterial* InOpacity = InShader->Opacity;
	UBOOL OpacityUseBaseModifier = 1;
	
	if( BaseModifierInfo.ModifyOpacity )
	{
		InOpacity = BaseModifierInfo.Opacity;
		OpacityUseBaseModifier = !BaseModifierInfo.OpacityOverrideTexModifier;
	}

	// Only simple shaders for those cards.
	if( RenDev->Is3dfx )
	{
		InShader->Specular			= NULL;
		InShader->SpecularityMask	= NULL;
	}

	// Simple case: no opacity!
	if( !InOpacity )
	{
		// Process diffuse channel
		if( !HandleCombinedMaterial( InDiffuse, PassesUsed, StagesUsed, FreeStagesUsed, BaseModifierInfo, 0, ErrorString, ErrorMaterial ) )
			return 0;

		if( StagesUsed == 1 )
		{
			// patch in diffuse to stage[0]
			if( NeedsDiffuseLight )
				HandleDiffuse_Patch( CurrentState->MaterialPasses[PassesUsed]->Stages[0] );

			// add a lightmap
			if( NeedsLightmap )
			{
				HandleLightmap_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CurrentState->Lightmap );
				StagesUsed++;
			}
		}
		else
		if( StagesUsed<MaxTextureStages && NeedsLightmap && !NeedsDiffuseLight )
		{
			HandleLightmap_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CurrentState->Lightmap );
			StagesUsed++;
		}
		else
		if( StagesUsed<MaxTextureStages && !NeedsLightmap && NeedsDiffuseLight )
		{
			if( StagesUsed )
		{
				HandleDiffuse_Stage( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], InShader->ModulateStaticLighting2X );
			StagesUsed++;
		}
		}
		else
		{
			// Perform lighting in a seperate pass
			if( NeedsDiffuseLight || NeedsLightmap )
			{
				NEWPASS();
				HandleLighting_MP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CurrentState->Lightmap, NeedsDiffuseLight );
				StagesUsed++;

				// Set lighting framebuffer blending
				if( NeedsDiffuseLight && !NeedsLightmap && !InShader->ModulateStaticLighting2X )
				{
					CurrentState->MaterialPasses[PassesUsed]->SrcBlend				= D3DBLEND_DESTCOLOR;
					CurrentState->MaterialPasses[PassesUsed]->DestBlend				= D3DBLEND_ZERO;
				}
				else
				{
					CurrentState->MaterialPasses[PassesUsed]->SrcBlend				= D3DBLEND_DESTCOLOR;
					CurrentState->MaterialPasses[PassesUsed]->DestBlend				= D3DBLEND_SRCCOLOR;
					CurrentState->MaterialPasses[PassesUsed]->OverriddenFogColor	= FColor( 127, 127, 127, 0 );
					CurrentState->MaterialPasses[PassesUsed]->OverrideFogColor		= 1;
				}

				CurrentState->MaterialPasses[PassesUsed]->TwoSided		= InShader->TwoSided;
				CurrentState->MaterialPasses[PassesUsed]->AlphaBlending	= 1;
			}
		}
	

		//
		// 2. Self-illumination
		//
		UBOOL SinglePassSelfIllum=0;
		if( !Unlit && InShader->SelfIllumination )
		{
			FD3DModifierInfo SelfIlluminationModifierInfo = BaseModifierInfo;
			FD3DModifierInfo SelfIlluminationMaskModifierInfo = BaseModifierInfo;

			UBitmapMaterial* SelfIlluminationBitmap = CheckMaterial<UBitmapMaterial>(this, InShader->SelfIllumination, &SelfIlluminationModifierInfo );
			UBitmapMaterial* SelfIlluminationMaskBitmap = CheckMaterial<UBitmapMaterial>(this, InShader->SelfIlluminationMask, &SelfIlluminationMaskModifierInfo );

			if( PassesUsed==0 && StagesUsed<MaxTextureStages-1 && SelfIlluminationBitmap && SelfIlluminationMaskBitmap )
			{
				// we have room to add the self-illumination in the single pass
				HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SelfIlluminationMaskBitmap );
				ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed],&SelfIlluminationMaskModifierInfo );
				StagesUsed++;

				HandleSelfIllumination_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SelfIlluminationBitmap);
				ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed],&SelfIlluminationModifierInfo );
				StagesUsed++;
				SinglePassSelfIllum=1;
			}
			else
			{
				// we need to do the self-illumination in its own pass
				NEWPASS();
				CurrentState->MaterialPasses[PassesUsed]->TwoSided				= InShader->TwoSided;

				// Load SelfIllumination
				if( !HandleCombinedMaterial( InShader->SelfIllumination, PassesUsed, StagesUsed, FreeStagesUsed, BaseModifierInfo, 0, ErrorString, ErrorMaterial ) )
					return 0;
	
				if( InShader->SelfIlluminationMask == InShader->SelfIllumination )
				{
					// Self-illumination alpha channel is in SelfIllumination.  Just blend it to the framebuffer.
					CurrentState->MaterialPasses[PassesUsed]->SrcBlend			= D3DBLEND_SRCALPHA;
					CurrentState->MaterialPasses[PassesUsed]->DestBlend			= D3DBLEND_INVSRCALPHA;
					CurrentState->MaterialPasses[PassesUsed]->AlphaBlending		= 1;
				}
				else
				if( SelfIlluminationMaskBitmap )
				{
					if( StagesUsed < MaxTextureStages )
					{
						HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SelfIlluminationMaskBitmap );
						ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed],&SelfIlluminationMaskModifierInfo );
						StagesUsed++;
					}
					else
					{
						if( ErrorString ) *ErrorString = FString::Printf(TEXT("No room for SelfIllimunationMask in multipass SelfIllumination (StagesUsed was %d)"), StagesUsed);
						if( ErrorMaterial ) *ErrorMaterial = InShader;
						return 0;
					}

					// Add SelfIllumination to the framebuffer
					CurrentState->MaterialPasses[PassesUsed]->SrcBlend			= D3DBLEND_SRCALPHA;
					CurrentState->MaterialPasses[PassesUsed]->DestBlend			= D3DBLEND_INVSRCALPHA;
					CurrentState->MaterialPasses[PassesUsed]->AlphaBlending		= 1;
				}
				else
				{
					if( ErrorString ) *ErrorString = TEXT("SelfIllimunationMask must be a simple bitmap");
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}				
			}
		}

		//
		// 3. Detail Texture
		//
		if( RenDev->DetailTextures && CurrentState->UseDetailTexturing )
		{
			UMaterial* DetailMaterial = NULL;
			FLOAT DetailScale = 8.f;
			if( InShader->Detail )
			{
				DetailMaterial = InShader->Detail;
				DetailScale = InShader->DetailScale;
			}
			else
			{
				UTexture* DiffuseTex = CheckMaterial<UTexture>(this, InDiffuse);
				if( DiffuseTex )
				{
					DetailMaterial = DiffuseTex->Detail;
					DetailScale = DiffuseTex->DetailScale;
				}
			}

			if( DetailMaterial )
			{
				FD3DModifierInfo DetailModifierInfo = BaseModifierInfo;
				DetailModifierInfo.SetDetailTextureScale( DetailScale );
				UBitmapMaterial* DetailBitmap = CheckMaterial<UBitmapMaterial>(this, DetailMaterial, &DetailModifierInfo);
				if( DetailBitmap )
					HandleDetail( DetailBitmap, PassesUsed, StagesUsed, FreeStagesUsed, DetailModifierInfo );
			}
		}

		//
		// 4. Process specular and mask
		//
		if( InShader->Specular )
		{
			FD3DModifierInfo SpecularModifierInfo = BaseModifierInfo;
			FD3DModifierInfo SpecularityMaskModifierInfo = BaseModifierInfo;

			UBitmapMaterial* SpecularBitmap = CheckMaterial<UBitmapMaterial>(this, InShader->Specular, &SpecularModifierInfo );
			UBitmapMaterial* SpecularityMaskBitmap = CheckMaterial<UBitmapMaterial>(this, InShader->SpecularityMask, &SpecularityMaskModifierInfo );
			UConstantMaterial* SpecularityMaskConstant = CheckMaterial<UConstantMaterial>(this, InShader->SpecularityMask, &SpecularityMaskModifierInfo );

			if(	PassesUsed==0 && StagesUsed<MaxTextureStages && SpecularBitmap && ((!InShader->SpecularityMask)||(InShader->SpecularityMask==InShader->Diffuse&&!SinglePassSelfIllum)||SpecularityMaskConstant) )
			{
				// We can fit single-pass specular
				if( SpecularityMaskConstant )
				{
					BYTE SpecularAlpha = SpecularityMaskConstant->GetColor(Viewport->Actor->Level->TimeSeconds).A;
					//!!MAT check for TFactor use
					CurrentState->MaterialPasses[PassesUsed]->TFactorColor = (D3DCOLOR)(FColor(SpecularAlpha,SpecularAlpha,SpecularAlpha,SpecularAlpha));
				}
				HandleSpecular_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SpecularBitmap, InShader->SpecularityMask != NULL, SpecularityMaskConstant != NULL );
				ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &SpecularModifierInfo );
				StagesUsed++;
			}
			else
			if(	PassesUsed==0 && StagesUsed<MaxTextureStages-1 && SpecularBitmap && SpecularityMaskBitmap )
			{
				// We have room for the specular and mask in the same pass pass as diffuse
				HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SpecularityMaskBitmap );
				ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &SpecularityMaskModifierInfo );
				StagesUsed++;
				HandleSpecular_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SpecularBitmap, 1, 0 );
				ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &SpecularModifierInfo );
				StagesUsed++;
			}
			else
			{
				// We need to perform specular in its own pass
				NEWPASS();
				CurrentState->MaterialPasses[PassesUsed]->TwoSided = InShader->TwoSided;

				// Load specular
				if( !HandleCombinedMaterial( InShader->Specular, PassesUsed, StagesUsed, FreeStagesUsed, BaseModifierInfo, 0, ErrorString, ErrorMaterial ) )
					return 0;
	
				if( !InShader->SpecularityMask )
				{
					// Add specular to the framebuffer
					CurrentState->MaterialPasses[PassesUsed]->SrcBlend				= D3DBLEND_ONE;
					CurrentState->MaterialPasses[PassesUsed]->DestBlend				= D3DBLEND_ONE;
					CurrentState->MaterialPasses[PassesUsed]->AlphaBlending			= 1;
					CurrentState->MaterialPasses[PassesUsed]->OverrideFogColor		= 1;
					CurrentState->MaterialPasses[PassesUsed]->OverriddenFogColor	= FColor( 0, 0, 0, 0 );
				}
				else
				if( SpecularityMaskConstant )
				{
					//!!MAT
					if( ErrorString ) *ErrorString = TEXT("TODO: constant SpecularityMask with multipass specular");
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}
				else
				if( SpecularityMaskBitmap )
				{
					if( StagesUsed < MaxTextureStages )
					{
						HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SpecularityMaskBitmap );
						ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &SpecularityMaskModifierInfo );
						StagesUsed++;
					}
					else
					{
						if( ErrorString ) *ErrorString = FString::Printf(TEXT("No room for SpecularityMask in multipass specular (StagesUsed was %d)"), StagesUsed);
						if( ErrorMaterial ) *ErrorMaterial = InShader;
						return 0;
					}

					// Add specular to the framebuffer
					CurrentState->MaterialPasses[PassesUsed]->SrcBlend				= D3DBLEND_SRCALPHA;
					CurrentState->MaterialPasses[PassesUsed]->DestBlend			= D3DBLEND_ONE;
					CurrentState->MaterialPasses[PassesUsed]->AlphaBlending		= 1;
					CurrentState->MaterialPasses[PassesUsed]->OverrideFogColor		= 1;
					CurrentState->MaterialPasses[PassesUsed]->OverriddenFogColor	= FColor( 0, 0, 0, 0 );
				}
				else
				{
					if( ErrorString ) *ErrorString = TEXT("SpecularityMask must be a simple bitmap");
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}				
			}
		}

	} // no opacity
	else
	{	
		//
		// 1. Handle diffuse
		//
		if( !HandleCombinedMaterial( InDiffuse, PassesUsed, StagesUsed, FreeStagesUsed, BaseModifierInfo, 0, ErrorString, ErrorMaterial ) )
			return 0;

		// Handle the vertex color
		if( NeedsDiffuseLight )
		{
			if( StagesUsed==1 )
				HandleDiffuse_Patch( CurrentState->MaterialPasses[PassesUsed]->Stages[0] );
			else
			{
				if( StagesUsed < MaxTextureStages+MaxFreeStages )
				{
					HandleDiffuse_Stage( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], InShader->ModulateStaticLighting2X );
				StagesUsed++;
					if( MaxFreeStages )
						FreeStagesUsed++;
				}
				else
				{
					if( ErrorString ) *ErrorString = TEXT("No stages left for vertex lighting stage");
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}
			}
		}

		// Handle the lightmap
		if( NeedsLightmap )
		{
			if( StagesUsed < MaxTextureStages+FreeStagesUsed )
			{
				HandleLightmap_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CurrentState->Lightmap );
				StagesUsed++;
			}
			else
			{
				if( ErrorString ) *ErrorString = TEXT("No stages left for lightmap");
				if( ErrorMaterial ) *ErrorMaterial = InShader;
				return 0;
			}
		}

		// Load the opacity if needed
		if( InOpacity != InDiffuse )
		{
			FD3DModifierInfo OpacityModifierInfo = OpacityUseBaseModifier ? BaseModifierInfo : FD3DModifierInfo();
			UBitmapMaterial* OpacityBitmap;
			UConstantMaterial* ConstantMaterial;
			UVertexColor* VertexColor;
			if( (OpacityBitmap=CheckMaterial<UBitmapMaterial>(this, InOpacity, &OpacityModifierInfo )) != NULL )
			{
				if( StagesUsed < MaxTextureStages+FreeStagesUsed )
				{
					// The opacity is a simple texture:
					HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], OpacityBitmap );
					ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &OpacityModifierInfo );
					StagesUsed++;
				}
				else
				{
					if( ErrorString ) *ErrorString = TEXT("No stages left for opacity bitmap");
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}
			}
			else if( (ConstantMaterial=CheckMaterial<UConstantMaterial>(this, InOpacity, &OpacityModifierInfo )) != NULL )
			{
				if( StagesUsed < MaxTextureStages+FreeStagesUsed )
				{
					FColor	Color = ConstantMaterial->GetColor(Viewport->Actor->Level->TimeSeconds);

					if(CurrentState->MaterialPasses[PassesUsed]->TFactorColor == 0 || CurrentState->MaterialPasses[PassesUsed]->TFactorColor == Color.DWColor())
					{
						CurrentState->MaterialPasses[PassesUsed]->TFactorColor = Color.DWColor();
						CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaArg1 = D3DTA_TFACTOR;
						CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
						CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG1;
						StagesUsed++;
					}
					else
					{
						if(ErrorString) *ErrorString = TEXT("Only one ConstantMaterial may be used per material.");
						if(ErrorMaterial) *ErrorMaterial = InShader;
						return 0;
					}
				}
				else
				{
					if( ErrorString ) *ErrorString = TEXT("No stages left for constant opacity");
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}
			}
			else if( (VertexColor=CheckMaterial<UVertexColor>(this, InOpacity, &OpacityModifierInfo )) != NULL )
			{
				if( StagesUsed < MaxTextureStages+FreeStagesUsed )
				{
					HandleVertexOpacity( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], VertexColor );
					StagesUsed++;
				}
				else
				{
					if( ErrorString ) *ErrorString = TEXT("No stages left for vertex opacity");
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}
			}
			else
			{
				// don't handle this case yet!
				if( ErrorString ) *ErrorString = TEXT("Cannot handle complex opacity which is not the same as diffuse material");
				if( ErrorMaterial ) *ErrorMaterial = InOpacity;
				return 0;
			}
		}


		//
		// 2. Self-illumination
		//
		if( !Unlit && InShader->SelfIllumination )
		{
			// we need to do the self-illumination in its own pass
			NEWPASS();
			CurrentState->MaterialPasses[PassesUsed]->TwoSided = InShader->TwoSided;

			FD3DModifierInfo OpacityModifierInfo = OpacityUseBaseModifier ? BaseModifierInfo : FD3DModifierInfo();
			UBitmapMaterial* OpacityBitmap = CheckMaterial<UBitmapMaterial>(this, InOpacity, &OpacityModifierInfo );

			if( !OpacityBitmap )
			{
				if( ErrorString ) *ErrorString = TEXT("Opacity must be a simple bitmap when using SelfIllumination");
				if( ErrorMaterial ) *ErrorMaterial = InOpacity;
				return 0;
			}

			FD3DModifierInfo SelfIlluminationModifierInfo = BaseModifierInfo;
			FD3DModifierInfo SelfIlluminationMaskModifierInfo = BaseModifierInfo;

			UBitmapMaterial* SelfIlluminationMaskBitmap = CheckMaterial<UBitmapMaterial>(this, InShader->SelfIlluminationMask, &SelfIlluminationMaskModifierInfo );

			// Load SelfIllumination
			if( !HandleCombinedMaterial( InShader->SelfIllumination, PassesUsed, StagesUsed, FreeStagesUsed, BaseModifierInfo, 0, ErrorString, ErrorMaterial ) )
				return 0;

			if( SelfIlluminationMaskBitmap )
			{
				if( StagesUsed < MaxTextureStages-1 )
				{
					HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SelfIlluminationMaskBitmap );
					ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed],&SelfIlluminationMaskModifierInfo );
					StagesUsed++;

					HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], OpacityBitmap, 1 );
					ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &OpacityModifierInfo );
					StagesUsed++;
				}
				else
				{
					if( ErrorString ) *ErrorString = FString::Printf(TEXT("No room for SelfIllimunationMask in multipass SelfIllumination (StagesUsed was %d)"), StagesUsed);
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}

				// Add SelfIllumination to the framebuffer
				CurrentState->MaterialPasses[PassesUsed]->SrcBlend				= D3DBLEND_SRCALPHA;
				CurrentState->MaterialPasses[PassesUsed]->DestBlend			= D3DBLEND_INVSRCALPHA;
				CurrentState->MaterialPasses[PassesUsed]->AlphaBlending		= 1;
			}
			else
			{
				if( ErrorString ) *ErrorString = TEXT("SelfIllimunationMask must be a simple bitmap");
				if( ErrorMaterial ) *ErrorMaterial = InShader;
				return 0;
			}				
		}

		//
		// 3. Specular
		//
		if( InShader->Specular )
		{
			FD3DModifierInfo SpecularModifierInfo = BaseModifierInfo;
			FD3DModifierInfo SpecularityMaskModifierInfo = BaseModifierInfo;

			UBitmapMaterial* SpecularityMaskBitmap = CheckMaterial<UBitmapMaterial>(this, InShader->SpecularityMask, &SpecularityMaskModifierInfo );
			UConstantMaterial* SpecularityMaskConstant = CheckMaterial<UConstantMaterial>(this, InShader->SpecularityMask, &SpecularityMaskModifierInfo );

			// We need to perform specular in its own pass
			NEWPASS();
			CurrentState->MaterialPasses[PassesUsed]->TwoSided = InShader->TwoSided;

			// Load specular
			if( !HandleCombinedMaterial( InShader->Specular, PassesUsed, StagesUsed, FreeStagesUsed, BaseModifierInfo, 0, ErrorString, ErrorMaterial ) )
				return 0;

			if( !InShader->SpecularityMask )
			{
				// No specularity mask, do nothing.
			}
			else
			if( SpecularityMaskConstant )
			{
				//!!MAT
				if( ErrorString ) *ErrorString = TEXT("TODO: constant SpecularityMask with multipass specular");
				if( ErrorMaterial ) *ErrorMaterial = InShader;
				return 0;
			}
			else
			if( SpecularityMaskBitmap )
			{
				if( StagesUsed < MaxTextureStages )
				{
					HandleOpacityBitmap( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], SpecularityMaskBitmap );
					ApplyTexModifier( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], &SpecularityMaskModifierInfo );
					StagesUsed++;
				}
				else
				{
					if( ErrorString ) *ErrorString = FString::Printf(TEXT("No room for SpecularityMask in multipass specular (StagesUsed was %d)"), StagesUsed);
					if( ErrorMaterial ) *ErrorMaterial = InShader;
					return 0;
				}
			}
			else
			{
				if( ErrorString ) *ErrorString = TEXT("SpecularityMask must be a simple bitmap");
				if( ErrorMaterial ) *ErrorMaterial = InShader;
				return 0;
			}				

			// Add specular to the framebuffer
			CurrentState->MaterialPasses[PassesUsed]->SrcBlend			= InShader->SpecularityMask ? D3DBLEND_SRCALPHA : D3DBLEND_ONE;
			CurrentState->MaterialPasses[PassesUsed]->SrcBlend			= CurrentState->MaterialPasses[PassesUsed]->SrcBlend;
			CurrentState->MaterialPasses[PassesUsed]->DestBlend			= D3DBLEND_ONE;
			CurrentState->MaterialPasses[PassesUsed]->AlphaBlending		= 1;
			CurrentState->MaterialPasses[PassesUsed]->OverrideFogColor		= 1;
			CurrentState->MaterialPasses[PassesUsed]->OverriddenFogColor	= FColor( 0, 0, 0, 0 );
		}
	}

	//
	// Framebuffer blending
	//
	if( BaseModifierInfo.ModifyFramebufferBlending )
	{
		ApplyFinalBlend( &BaseModifierInfo );
	}
	else
	{
		switch( InShader->OutputBlending )
		{
		case OB_Normal :
			if( InOpacity )
			{
				CurrentState->MaterialPasses[0]->SrcBlend		= D3DBLEND_SRCALPHA;
				CurrentState->MaterialPasses[0]->DestBlend		= D3DBLEND_INVSRCALPHA;
				CurrentState->MaterialPasses[0]->AlphaBlending	= 1;
				CurrentState->MaterialPasses[0]->AlphaTest		= 1;
				CurrentState->MaterialPasses[0]->ZWrite		= 0;
			}
			else
			{
				CurrentState->MaterialPasses[0]->SrcBlend		= D3DBLEND_ONE;
				CurrentState->MaterialPasses[0]->DestBlend		= D3DBLEND_ZERO;
				CurrentState->MaterialPasses[0]->AlphaBlending	= 0;
				CurrentState->MaterialPasses[0]->AlphaTest		= 0;
				CurrentState->MaterialPasses[0]->ZWrite		= 1;
			}
			break;
		case OB_Masked:
			CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ZERO;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 0;
			CurrentState->MaterialPasses[0]->AlphaTest			= 1;
			CurrentState->MaterialPasses[0]->AlphaRef			= 127;
			CurrentState->MaterialPasses[0]->ZWrite			= 1;
			break;
		case OB_Modulate:
			CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_DESTCOLOR;
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_SRCCOLOR;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
			CurrentState->MaterialPasses[0]->ZWrite			= 0;
			CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
			CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 127, 127, 127, 0 );
			break;
		case OB_Translucent:
			CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ONE;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
			CurrentState->MaterialPasses[0]->ZWrite			= 0;
			CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
			CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
			break;
		case OB_Brighten:
			CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_INVSRCCOLOR;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
			CurrentState->MaterialPasses[0]->ZWrite			= 0;
			CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
			CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
			break;
		case OB_Darken:
			CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ZERO;
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_INVSRCCOLOR;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
			CurrentState->MaterialPasses[0]->ZWrite			= 0;
			CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
			CurrentState->MaterialPasses[0]->OverriddenFogColor= FColor( 0, 0, 0, 0 );
			break;
		case OB_Invisible:
			CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ZERO;
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ONE;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
			CurrentState->MaterialPasses[0]->ZWrite			= 0;
			break;
		}
		CurrentState->MaterialPasses[0]->TwoSided = InShader->TwoSided;
	}

	// Fillmode.
	CurrentState->MaterialPasses[0]->FillMode = InShader->Wireframe ? FM_Wireframe : FM_Solid;

	// Color modifier
	if( BaseModifierInfo.ModifyColor )
	{
		if(	BaseModifierInfo.AlphaBlend									&&
			(CurrentState->MaterialPasses[0]->SrcBlend  == D3DBLEND_ONE)	&&
			(CurrentState->MaterialPasses[0]->DestBlend == D3DBLEND_ZERO)
		)
		{
				CurrentState->MaterialPasses[0]->SrcBlend	= D3DBLEND_SRCALPHA;
				CurrentState->MaterialPasses[0]->DestBlend	= D3DBLEND_INVSRCALPHA;
		}
		HandleTFactor_SP(CurrentState->MaterialPasses[0]->Stages[StagesUsed]);
		CurrentState->MaterialPasses[0]->TFactorColor		 = BaseModifierInfo.TFactorColor;
		CurrentState->MaterialPasses[0]->AlphaBlending		|= BaseModifierInfo.AlphaBlend;
		CurrentState->MaterialPasses[0]->TwoSided			|= BaseModifierInfo.TwoSided;
		StagesUsed++;
	}

	// Done.
	CurrentState->MaterialPasses[PassesUsed++]->StagesUsed	= StagesUsed;
	CurrentState->NumMaterialPasses			= PassesUsed;
	
	return 1;
	unguard;
}

/*----------------------------------------------------------------------------
	SetSimpleMaterial.
----------------------------------------------------------------------------*/

UBOOL FD3DRenderInterface::SetSimpleMaterial( UMaterial* InMaterial, FD3DModifierInfo InModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial )
{
	guard(FD3DRenderInterface::SetSimpleMaterial);

	// Handle masked/alpha texture UTexture
	UTexture* Texture;
	UMaterial* DetailMaterial = NULL;
	FLOAT DetailScale = 8.f;
	if( !InModifierInfo.ModifyFramebufferBlending && (Texture=Cast<UTexture>(InMaterial))!=NULL )
	{
		if( Texture->bMasked )
		{
			InModifierInfo.ModifyFramebufferBlending	= 1;
			if( InModifierInfo.ModifyColor && InModifierInfo.AlphaBlend )
				InModifierInfo.FrameBufferBlending		= FB_AlphaBlend;
			else
				InModifierInfo.FrameBufferBlending		= FB_Overwrite;
			InModifierInfo.ZWrite		= 1;
			InModifierInfo.ZTest		= 1;
			InModifierInfo.AlphaTest	= 1;
			InModifierInfo.AlphaRef		= 127;
			if( !InModifierInfo.ModifyColor )
				InModifierInfo.TwoSided	= Texture->bTwoSided;
		}
		else
		if( Texture->bAlphaTexture )
		{
			InModifierInfo.ModifyFramebufferBlending	= 1;
			InModifierInfo.FrameBufferBlending			= FB_AlphaBlend;
			InModifierInfo.ZWrite		= 1;
			InModifierInfo.ZTest		= 1;
			InModifierInfo.AlphaTest	= 1;
			InModifierInfo.AlphaRef		= 0;
			if( !InModifierInfo.ModifyColor )
				InModifierInfo.TwoSided	= Texture->bTwoSided;
		}
		else
		{
			if( !InModifierInfo.ModifyColor )
				InModifierInfo.TwoSided	= Texture->bTwoSided;
		}

		DetailMaterial = Texture->Detail;
		DetailScale = Texture->DetailScale;
	}

	INT StagesUsed = 0;
	INT PassesUsed = 0;
	INT FreeStagesUsed = 0;

	INT MaxTextureStages = RenDev->DeviceCaps8.MaxSimultaneousTextures;
	INT MaxBlendStages   = RenDev->DeviceCaps8.MaxTextureBlendStages;
	INT MaxFreeStages	= MaxBlendStages - MaxTextureStages;

	if( !HandleCombinedMaterial( InMaterial, PassesUsed, StagesUsed, FreeStagesUsed, InModifierInfo, 0, ErrorString, ErrorMaterial ) )
		return 0;

	if( CurrentState->UseStaticLighting || CurrentState->UseDynamicLighting )
		HandleDiffuse_Patch( CurrentState->MaterialPasses[PassesUsed]->Stages[0] );

	if( CurrentState->Lightmap )
	{
		if( StagesUsed < MaxTextureStages )
			HandleLightmap_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed++], CurrentState->Lightmap );
		else
		{
			NEWPASS();
			HandleLighting_MP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed], CurrentState->Lightmap, CurrentState->UseStaticLighting );
			StagesUsed++;

			CurrentState->MaterialPasses[PassesUsed]->SrcBlend				= D3DBLEND_DESTCOLOR;
			CurrentState->MaterialPasses[PassesUsed]->DestBlend				= D3DBLEND_SRCCOLOR;
			CurrentState->MaterialPasses[PassesUsed]->OverriddenFogColor	= FColor( 127, 127, 127, 0 );
			CurrentState->MaterialPasses[PassesUsed]->OverrideFogColor		= 1;
			
			CurrentState->MaterialPasses[PassesUsed]->TwoSided				= InModifierInfo.TwoSided;
			CurrentState->MaterialPasses[PassesUsed]->AlphaBlending			= 1;
		}
	}

	// Set framebuffer blending
	if( InModifierInfo.ModifyFramebufferBlending )
	{
		ApplyFinalBlend( &InModifierInfo );
	}

	// Color modifier
	if( InModifierInfo.ModifyColor )
	{
		if( StagesUsed < MaxTextureStages+MaxFreeStages )
		{
			if(	InModifierInfo.AlphaBlend											&&
			    (CurrentState->MaterialPasses[PassesUsed]->SrcBlend  == D3DBLEND_ONE)	&&
			    (CurrentState->MaterialPasses[PassesUsed]->DestBlend == D3DBLEND_ZERO)
		    )
		    {
			    CurrentState->MaterialPasses[PassesUsed]->SrcBlend		= D3DBLEND_SRCALPHA;
			    CurrentState->MaterialPasses[PassesUsed]->DestBlend	= D3DBLEND_INVSRCALPHA;
		    }
		    HandleTFactor_SP(CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed]);
		    CurrentState->MaterialPasses[PassesUsed]->TFactorColor		= InModifierInfo.TFactorColor;
		    CurrentState->MaterialPasses[PassesUsed]->AlphaBlending   |= InModifierInfo.AlphaBlend;
		    CurrentState->MaterialPasses[PassesUsed]->TwoSided		   |= InModifierInfo.TwoSided;
		    StagesUsed++;
			if( MaxFreeStages )
				FreeStagesUsed++;
		}
		else
		{
			if( ErrorString ) *ErrorString = TEXT("No stages left for constant color modifier.");
			if( ErrorMaterial ) *ErrorMaterial = InMaterial;
			return 0;				
		}
	}

	// Detail textures.
	if( RenDev->DetailTextures && CurrentState->UseDetailTexturing && DetailMaterial )
	{
		FD3DModifierInfo DetailModifierInfo = InModifierInfo;
		DetailModifierInfo.SetDetailTextureScale( DetailScale );
		UBitmapMaterial* DetailBitmap = CheckMaterial<UBitmapMaterial>(this, DetailMaterial, &DetailModifierInfo);
		if( DetailBitmap )
			HandleDetail( DetailBitmap, PassesUsed, StagesUsed, FreeStagesUsed, DetailModifierInfo );
	}

	CurrentState->MaterialPasses[PassesUsed]->StagesUsed = StagesUsed;
	StagesUsed = 0;
	PassesUsed++;
	CurrentState->NumMaterialPasses = PassesUsed;

	return 1;
	unguard;
}

/*----------------------------------------------------------------------------
	SetLightingOnlyMaterial.
----------------------------------------------------------------------------*/

void FD3DRenderInterface::SetLightingOnlyMaterial()
{
	guard(FD3DRenderInterface::SetLightingOnlyMaterial);
	INT StagesUsed = 0;
	INT PassesUsed = 0;

	HandleDiffuse_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed++] );

	if( CurrentState->Lightmap )
		HandleLightmap_SP( CurrentState->MaterialPasses[PassesUsed]->Stages[StagesUsed++], CurrentState->Lightmap );

	CurrentState->MaterialPasses[PassesUsed]->StagesUsed = StagesUsed;
	StagesUsed = 0;
	PassesUsed++;
	CurrentState->NumMaterialPasses = PassesUsed;

	unguard;
}

/*----------------------------------------------------------------------------
	SetTerrainMaterial.
----------------------------------------------------------------------------*/

UBOOL FD3DRenderInterface::SetTerrainMaterial( UTerrainMaterial* TerrainMaterial, FD3DModifierInfo InModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial )
{
	guard(FD3DRenderInterface::SetTerrainMaterial);

	if( TerrainMaterial->RenderMethod == RM_AlphaMap )
	{
		if( TerrainMaterial->UseFallback )
		{
			// Previous shader we tried wasn't renderable.  Try the Shader's FallbackMaterial.
			TerrainMaterial->Layers(0).Texture->UseFallback = 1;
			TerrainMaterial->Layers(0).Texture = TerrainMaterial->Layers(0).Texture->CheckFallback();
			TerrainMaterial->UseFallback = 0;
		}
		
		UBitmapMaterial* BitmapMaterial;
		FD3DModifierInfo TexModifier;
		TexModifier.ModifyTextureTransforms = 1;
		TexModifier.Matrix = TerrainMaterial->Layers(0).TextureMatrix;
		TexModifier.TexCoordSource = TCS_WorldCoords;
		if( (BitmapMaterial = CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(0).Texture, &TexModifier )) != NULL )
		{
		// This is a regular bitmap material.
			FD3DMaterialStateStage* Stage = &CurrentState->MaterialPasses[0]->Stages[0];

		// Alpha-blend with the framebuffer, using AlphaWeight as an alphamap
		if( TerrainMaterial->FirstPass )
		{
				CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_ONE;
				CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ZERO;
				CurrentState->MaterialPasses[0]->AlphaBlending		= 0;
				CurrentState->MaterialPasses[0]->AlphaTest			= 0;
		}
		else
		{
				CurrentState->MaterialPasses[0]->SrcBlend			= D3DBLEND_SRCALPHA;
				CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_INVSRCALPHA;
				CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
				CurrentState->MaterialPasses[0]->AlphaTest			= 1;
		}
			CurrentState->MaterialPasses[0]->OverrideFogColor	= 0;
			CurrentState->MaterialPasses[0]->ZWrite			= 1;
			CurrentState->MaterialPasses[0]->ZTest				= 1;
			CurrentState->MaterialPasses[0]->TwoSided			= 0;
			CurrentState->MaterialPasses[0]->AlphaRef			= 0;


		// load the texture for the layer
		SetShaderBitmap( *Stage, BitmapMaterial );
		Stage->ColorArg1 = D3DTA_TEXTURE;
		Stage->ColorArg2 = D3DTA_DIFFUSE;
		Stage->AlphaArg1 = D3DTA_TEXTURE;
		Stage->ColorOp   = (CurrentState->UseStaticLighting || CurrentState->UseDynamicLighting) ? DUMMY_MODULATE2X : D3DTOP_SELECTARG1;
		Stage->AlphaOp   = D3DTOP_SELECTARG1;
		ApplyTexModifier( *Stage, &TexModifier );
		Stage++;

		// use the alphamap's alpha for the layer
		SetShaderBitmap( *Stage, TerrainMaterial->Layers(0).AlphaWeight );
		Stage->ColorArg1 = D3DTA_CURRENT;
		Stage->ColorOp   = D3DTOP_SELECTARG1;
		Stage->AlphaArg1 = D3DTA_TEXTURE;
			Stage->AlphaOp   = BitmapMaterial->IsTransparent() ? D3DTOP_MODULATE : D3DTOP_SELECTARG1 ;
			FD3DModifierInfo AlphaModifier;
		AlphaModifier.ModifyTextureTransforms = 1;
		AlphaModifier.TexCoordSource = TCS_Stream0;
		ApplyTexModifier( *Stage, &AlphaModifier );
		Stage++;

			CurrentState->MaterialPasses[0]->StagesUsed = Stage - &CurrentState->MaterialPasses[0]->Stages[0];
		}
		else
		{
			// This is a shader.  
			DECLARE_STATIC_UOBJECT( UTexMatrix, LayerTexCoordModifier, {} );	
			LayerTexCoordModifier->Material = TerrainMaterial->Layers(0).Texture;
			LayerTexCoordModifier->Matrix = TerrainMaterial->Layers(0).TextureMatrix;
			LayerTexCoordModifier->TexCoordSource = TCS_WorldCoords;

			DECLARE_STATIC_UOBJECT( UTexMatrix, AlphaTexCoordModifier, {} );
			AlphaTexCoordModifier->Material = TerrainMaterial->Layers(0).AlphaWeight;
			AlphaTexCoordModifier->Matrix = FMatrix::Identity;
			AlphaTexCoordModifier->TexCoordSource = TCS_Stream0;

			DECLARE_STATIC_UOBJECT( UOpacityModifier, OpacityModifier, { OpacityModifier->bOverrideTexModifier = 1; } );
			OpacityModifier->Material = LayerTexCoordModifier;
			OpacityModifier->Opacity = AlphaTexCoordModifier;

			UMaterial* InMaterial;
			if( TerrainMaterial->FirstPass )
				InMaterial = LayerTexCoordModifier;
			else
				InMaterial = OpacityModifier;

			FD3DModifierInfo ShaderModifierInfo;
			UShader* Shader;
			if( (Shader = CheckMaterial<UShader>(this, InMaterial, &ShaderModifierInfo )) != NULL )
			{
				if( !SetShaderMaterial( Shader, ShaderModifierInfo, ErrorString, ErrorMaterial ) )
					return 0;
			}
			else
			{
				if( ErrorString ) *ErrorString = TEXT("Terrain layers must be Shaders or a BitmapMaterials.");
				if( ErrorMaterial ) *ErrorMaterial = TerrainMaterial->Layers(0).Texture;
				return 0;				
			}
		}
	}
	else
	{
		FD3DMaterialStateStage* Stage = &CurrentState->MaterialPasses[0]->Stages[0];

		// Add to the framebuffer, using AlphaWeight as a weightmap

		// Set framebuffer blending.
		CurrentState->MaterialPasses[0]->SrcBlend		= D3DBLEND_ONE;
		if( TerrainMaterial->FirstPass )
		{
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ZERO;
			CurrentState->MaterialPasses[0]->OverrideFogColor	= 0;
			CurrentState->MaterialPasses[0]->ZWrite			= 1;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 0;
		}
		else
		{
			CurrentState->MaterialPasses[0]->DestBlend			= D3DBLEND_ONE;
			CurrentState->MaterialPasses[0]->OverrideFogColor	= 1;
			CurrentState->MaterialPasses[0]->OverriddenFogColor = FColor( 0, 0, 0, 0 );
			CurrentState->MaterialPasses[0]->ZWrite			= 0;
			CurrentState->MaterialPasses[0]->AlphaBlending		= 1;
		}
		CurrentState->MaterialPasses[0]->AlphaTest		= 0;
		CurrentState->MaterialPasses[0]->ZTest			= 1;
		CurrentState->MaterialPasses[0]->TwoSided		= 0;
		CurrentState->MaterialPasses[0]->AlphaRef		= 0;

		// We're rendering with a combined weightmap and a pixelshader
		if( TerrainMaterial->RenderMethod == RM_CombinedWeightMap )
		{
			FD3DModifierInfo TexModifier, WeightMapModifier;

			SetShaderBitmap( *Stage, TerrainMaterial->Layers(0).AlphaWeight );
			WeightMapModifier.ModifyTextureTransforms = 1;
			WeightMapModifier.TexCoordSource = TCS_Stream0;
			ApplyTexModifier( *Stage, &WeightMapModifier );
			Stage++;

			TexModifier.ModifyTextureTransforms = 1;
			TexModifier.Matrix = TerrainMaterial->Layers(0).TextureMatrix;
			TexModifier.TexCoordSource = TCS_WorldCoords;
			SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(0).Texture, &TexModifier ) );
			ApplyTexModifier( *Stage, &TexModifier );
			Stage++;
			
			if( TerrainMaterial->Layers.Num() > 1 )
			{
				TexModifier = FD3DModifierInfo();
				TexModifier.ModifyTextureTransforms = 1;
				TexModifier.Matrix = TerrainMaterial->Layers(1).TextureMatrix;
				TexModifier.TexCoordSource = TCS_WorldCoords;
				SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(1).Texture, &TexModifier ) );
				ApplyTexModifier( *Stage, &TexModifier );
				Stage++;

				if( TerrainMaterial->Layers.Num() > 2 )
				{
					TexModifier = FD3DModifierInfo();
					TexModifier.ModifyTextureTransforms = 1;
					TexModifier.Matrix = TerrainMaterial->Layers(2).TextureMatrix;
					TexModifier.TexCoordSource = TCS_WorldCoords;
					SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(2).Texture, &TexModifier ) );
					ApplyTexModifier( *Stage, &TexModifier );
					Stage++;

					if( TerrainMaterial->Layers.Num() > 3 )
					{
						TexModifier = FD3DModifierInfo();
						TexModifier.ModifyTextureTransforms = 1;
						TexModifier.Matrix = TerrainMaterial->Layers(3).TextureMatrix;
						TexModifier.TexCoordSource = TCS_WorldCoords;
						SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(3).Texture, &TexModifier ) );
						ApplyTexModifier( *Stage, &TexModifier );
						Stage++;
					}
				}
			}

			switch( TerrainMaterial->Layers.Num() ) 
			{
			case 3:
				CurrentState->MaterialPasses[0]->PixelShader = PS_Terrain3Layer;
				break;
			case 4:
				CurrentState->MaterialPasses[0]->PixelShader = PS_Terrain4Layer;
				break;
			default:
				appErrorf(TEXT("too many layers at once"));
			}
		}
		else
		{
			FD3DModifierInfo TexModifier, WeightMapModifier;
			
			//!!powervr_aaron: KYRO does not support D3DTA_TEMP, and it takes an extra stage to blend in diffuse without it.
			if( !(RenDev->DeviceCaps8.PrimitiveMiscCaps & D3DPMISCCAPS_TSSARGTEMP) )
			{
				// load the texture for the first layer
				Stage->ColorArg1 = D3DTA_TEXTURE;
				Stage->ColorArg2 = D3DTA_DIFFUSE;
				Stage->ColorOp	 = TerrainMaterial->Layers.Num() == 1 ? DUMMY_MODULATE2X : D3DTOP_SELECTARG1;
				Stage->AlphaArg1 = D3DTA_DIFFUSE;
				Stage->AlphaOp   = D3DTOP_SELECTARG1;
				TexModifier.ModifyTextureTransforms = 1;
				TexModifier.Matrix = TerrainMaterial->Layers(0).TextureMatrix;
				TexModifier.TexCoordSource = TCS_WorldCoords;
				SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(0).Texture, &TexModifier ) );
				ApplyTexModifier( *Stage, &TexModifier );
				Stage++;

				// multiply by the weightmap
				SetShaderBitmap( *Stage, TerrainMaterial->Layers(0).AlphaWeight );
				Stage->ColorArg1 = D3DTA_CURRENT;
				Stage->ColorArg2 = D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE;
				Stage->ColorOp   = D3DTOP_MODULATE;
				Stage->AlphaArg1 = D3DTA_CURRENT;
				Stage->AlphaOp   = D3DTOP_SELECTARG1;
				WeightMapModifier.ModifyTextureTransforms = 1;
				WeightMapModifier.TexCoordSource = TCS_Stream0;
				ApplyTexModifier( *Stage, &WeightMapModifier );
				Stage++;

				if( TerrainMaterial->Layers.Num() > 1 )
				{
					// Render each subsequent layer.
					for( INT l=1;l<TerrainMaterial->Layers.Num();l++ )
					{
						// load the weightmap to alpha
						Stage->ColorArg1 = D3DTA_CURRENT;
						Stage->ColorOp   = D3DTOP_SELECTARG1;
						Stage->AlphaArg1 = D3DTA_TEXTURE;
						Stage->AlphaOp   = D3DTOP_SELECTARG1;
						WeightMapModifier.ModifyTextureTransforms = 1;
						WeightMapModifier.TexCoordSource = TCS_Stream0;
						SetShaderBitmap( *Stage, TerrainMaterial->Layers(l).AlphaWeight );
						ApplyTexModifier( *Stage, &WeightMapModifier );
						Stage++;

						// current += alpha * texture
						SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(l).Texture, &TexModifier ) );
						Stage->ColorArg1 = D3DTA_CURRENT;
						Stage->ColorArg2 = D3DTA_TEXTURE;
						Stage->ColorOp	 = D3DTOP_MODULATEALPHA_ADDCOLOR;
						Stage->AlphaArg1 = D3DTA_CURRENT;
						Stage->AlphaOp   = D3DTOP_SELECTARG1;
						TexModifier = FD3DModifierInfo();
						TexModifier.ModifyTextureTransforms = 1;
						TexModifier.Matrix = TerrainMaterial->Layers(l).TextureMatrix;
						TexModifier.TexCoordSource = TCS_WorldCoords;
						ApplyTexModifier( *Stage, &TexModifier );
						Stage++;
					}

					// modulate landscape by vertex diffuse colours
					Stage->ColorArg1 = D3DTA_DIFFUSE;
					Stage->ColorArg2 = D3DTA_CURRENT;
					Stage->ColorOp	 = DUMMY_MODULATE2X;
					Stage->AlphaArg1 = D3DTA_CURRENT;
					Stage->AlphaOp   = D3DTOP_SELECTARG1;
					Stage->TexCoordIndex = 0;
					Stage->TextureTransformsEnabled = 0;
					Stage++;
				}
			}
			else
			{
			// load the texture for the first layer
			Stage->ColorArg1 = D3DTA_TEXTURE;
			Stage->ColorArg2 = D3DTA_DIFFUSE;
			Stage->ColorOp   = DUMMY_MODULATE2X;
			TexModifier.ModifyTextureTransforms = 1;
			TexModifier.Matrix = TerrainMaterial->Layers(0).TextureMatrix;
			TexModifier.TexCoordSource = TCS_WorldCoords;
			SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(0).Texture, &TexModifier ) );
			ApplyTexModifier( *Stage, &TexModifier );
			Stage++;

			// multiply the weightmap and save it in the temp register
			SetShaderBitmap( *Stage, TerrainMaterial->Layers(0).AlphaWeight );
			Stage->ColorArg1 = D3DTA_CURRENT;
			Stage->ColorArg2 = D3DTA_TEXTURE;
			Stage->ColorOp   = D3DTOP_MODULATE;
			Stage->ResultArg = TerrainMaterial->Layers.Num() == 1 ? D3DTA_CURRENT : D3DTA_TEMP;
			WeightMapModifier.ModifyTextureTransforms = 1;
			WeightMapModifier.TexCoordSource = TCS_Stream0;
			ApplyTexModifier( *Stage, &WeightMapModifier );
			Stage++;

			// Render each subsequent layer.
			for( INT l=1;l<TerrainMaterial->Layers.Num();l++ )
			{
				// load the texture
				Stage->ColorArg1 = D3DTA_TEXTURE;
				Stage->ColorArg2 = D3DTA_DIFFUSE;
				Stage->ColorOp   = DUMMY_MODULATE2X;
				TexModifier = FD3DModifierInfo();
				TexModifier.ModifyTextureTransforms = 1;
				TexModifier.Matrix = TerrainMaterial->Layers(l).TextureMatrix;
				TexModifier.TexCoordSource = TCS_WorldCoords;
				SetShaderBitmap( *Stage, CheckMaterial<UBitmapMaterial>(this, TerrainMaterial->Layers(l).Texture, &TexModifier ) );
				ApplyTexModifier( *Stage, &TexModifier );
				Stage++;

				// multiply the weightmap and add to the temporary register
				SetShaderBitmap( *Stage, TerrainMaterial->Layers(l).AlphaWeight );
				Stage->ColorArg1 = D3DTA_TEXTURE;
				Stage->ColorArg2 = D3DTA_CURRENT;
				Stage->ColorArg0 = D3DTA_TEMP;
				Stage->ColorOp	 = D3DTOP_MULTIPLYADD;
				Stage->ResultArg = l == TerrainMaterial->Layers.Num()-1 ? D3DTA_CURRENT : D3DTA_TEMP;
				WeightMapModifier.ModifyTextureTransforms = 1;
				WeightMapModifier.TexCoordSource = TCS_Stream0;
				ApplyTexModifier( *Stage, &WeightMapModifier );
				Stage++;
			}
			}
		}
		CurrentState->MaterialPasses[0]->StagesUsed = Stage - &CurrentState->MaterialPasses[0]->Stages[0];
	}
	CurrentState->NumMaterialPasses	= 1;
	return 1;
	unguard;
}

/*----------------------------------------------------------------------------
	SetProjectorMaterial.
----------------------------------------------------------------------------*/

//
//	FProjectorCubeMapFace
//

class FProjectorCubeMapFace : public FTexture
{
public:

	FVector	Base,
			FaceX,
			FaceY;
	INT		Size;

	// Constructor.

	FProjectorCubeMapFace(FVector InBase,FVector InFaceX,FVector InFaceY,INT InSize)
	{
		Base = InBase;
		FaceX = InFaceX;
		FaceY = InFaceY;
		Size = InSize;
	}

	// FRenderResource interface.

	virtual QWORD GetCacheId() { return 0; }
	virtual INT GetRevision() { return 1; }

	// FBaseTexture interface.

	virtual INT GetWidth() { return Size; }
	virtual INT GetHeight() { return Size; }
	virtual INT GetFirstMip() { return 0; }
	virtual INT GetNumMips() { return 2; }
	virtual ETextureFormat GetFormat() { return TEXF_RGBA8; }

	virtual ETexClampMode GetUClamp() { return TC_Clamp; }
	virtual ETexClampMode GetVClamp() { return TC_Clamp; }

	// FTexture interface

	virtual void* GetRawTextureData(INT MipIndex) { return NULL; }
	virtual void UnloadRawTextureData(INT MipIndex) {}

	void GetTextureData(INT MipIndex,void* Dest,INT DestStride,ETextureFormat DestFormat,UBOOL ColoredMips)
	{
		guard(FSpecularCubeMapFace::GetTextureData);

		check(DestFormat == TEXF_RGBA8);

		INT		MipSize = Size >> MipIndex;
		FVector	DiffX = FaceX / (MipSize - 1),
				DiffY = FaceY / (MipSize - 1);

		for(INT Y = 0;Y < MipSize;Y++)
		{
			FColor*	DestPtr = (FColor*) ((BYTE*)Dest + Y * DestStride);
			FVector	Vector = Base + DiffY * (Y + 0.5f) + DiffX * 0.5f;

			for(INT X = 0;X < MipSize;X++)
			{
				FVector	Normal = Vector.SafeNormal();

				*DestPtr++ = FColor(
								Clamp(appFloor(Normal.Z * 255.0f),0,255),
								Clamp(appFloor(Normal.Z * 255.0f),0,255),
								Clamp(appFloor(Normal.Z * 255.0f),0,255),
								Clamp(appFloor(Normal.Z * 255.0f),0,255)
								);

				Vector += DiffX;
			}
		}

		unguard;
	}

	virtual UTexture* GetUTexture() { return NULL; }
};

//
//	FProjectorCubeMap
//

class FProjectorCubeMap : public FCubemap
{
public:

	FProjectorCubeMapFace	PositiveX,
							NegativeX,
							PositiveY,
							NegativeY,
							PositiveZ,
							NegativeZ;

	INT		Size;
	QWORD	CacheId;
	INT		Revision;

	// Constructor.

	FProjectorCubeMap(INT InSize) :
		PositiveX(FVector(+1,+1,+1),FVector(+0,+0,-2),FVector(+0,-2,+0),InSize),
		NegativeX(FVector(-1,+1,-1),FVector(+0,+0,+2),FVector(+0,-2,+0),InSize),
		PositiveY(FVector(-1,+1,-1),FVector(+2,+0,+0),FVector(+0,+0,+2),InSize),
		NegativeY(FVector(-1,-1,+1),FVector(+2,+0,+0),FVector(+0,+0,-2),InSize),
		PositiveZ(FVector(-1,+1,+1),FVector(+2,+0,+0),FVector(+0,-2,+0),InSize),
		NegativeZ(FVector(+1,+1,-1),FVector(-2,+0,+0),FVector(+0,-2,+0),InSize)
	{
		Size = InSize;
		CacheId = MakeCacheID(CID_RenderTexture);
		Revision = 1;
	}

	// FRenderResource interface.

	virtual QWORD GetCacheId() { return CacheId; }
	virtual INT GetRevision() { return Revision; }

	// FBaseTexture interface.

	virtual INT GetWidth() { return Size; }
	virtual INT GetHeight() { return Size; }
	virtual INT GetFirstMip() { return 0; }
	virtual INT GetNumMips() { return 2; }
	virtual ETextureFormat GetFormat() { return TEXF_RGBA8; }

	virtual ETexClampMode GetUClamp() { return TC_Clamp; }
	virtual ETexClampMode GetVClamp() { return TC_Clamp; }

	// FCubeMap interface.

	virtual FTexture* GetFace(INT FaceIndex)
	{
		switch(FaceIndex)
		{
		case 0:
			return &PositiveX;
		case 1:
			return &NegativeX;
		case 2:
			return &PositiveY;
		case 3:
			return &NegativeY;
		case 4:
			return &PositiveZ;
		case 5:
			return &NegativeZ;
		default:	
			return NULL;
		};
	}
};

UBOOL FD3DRenderInterface::SetProjectorMaterial( UProjectorMaterial* ProjectorMaterial, FD3DModifierInfo InModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial )
{
	guard(FD3DRenderInterface::SetProjectorMaterial);

	FD3DMaterialState*	MaterialState = CurrentState->MaterialPasses[0];
	INT					PassesUsed = 0,
						StagesUsed = 0,
						FreeStagesUsed = 0,
						MaxTextureStages = RenDev->DeviceCaps8.MaxSimultaneousTextures,
						MaxBlendStages   = RenDev->DeviceCaps8.MaxTextureBlendStages;

	// See if we have an alpha channel in the BaseMaterial.

	UShader*	BaseShader = Cast<UShader>(ProjectorMaterial->BaseMaterial);
	UTexture*	BaseTexture = Cast<UTexture>(ProjectorMaterial->BaseMaterial);	
	UMaterial*	BaseDiffuse = ProjectorMaterial->BaseMaterial;
	UMaterial*	BaseOpacity = NULL;

	if(BaseTexture && (BaseTexture->bMasked || BaseTexture->bAlphaTexture))
		BaseOpacity = BaseTexture;
	else if(BaseShader)
	{
		BaseDiffuse = BaseShader->Diffuse;
		BaseOpacity = BaseShader->Opacity;
		MaterialState->TwoSided = BaseShader->TwoSided;
	}

	if(ProjectorMaterial->bStaticProjector)
	{
		// See if the projected texture is simple.

		FD3DModifierInfo	ProjectedModifier;
		UBitmapMaterial*	ProjectedBitmap = CheckMaterial<UBitmapMaterial>(this,ProjectorMaterial->Projected,&ProjectedModifier,0);

		if(ProjectedBitmap)
		{
			// Handle the base material.

			if(ProjectorMaterial->BaseMaterialBlending != PB_None && BaseDiffuse)
			{
				FD3DModifierInfo BaseModifier			= InModifierInfo;
				BaseModifier.TexCoordSource				= TCS_Stream1;
				BaseModifier.ModifyTextureTransforms	= 1;
				if(!HandleCombinedMaterial(BaseDiffuse,PassesUsed,StagesUsed,FreeStagesUsed,BaseModifier,0,ErrorString,ErrorMaterial))
					return 0;
			}

			if(StagesUsed < MaxTextureStages + FreeStagesUsed)
			{
				SetShaderBitmap(MaterialState->Stages[StagesUsed],ProjectedBitmap);
				ProjectedModifier.ModifyTextureTransforms	= 1;
				ProjectedModifier.TexCoordCount				= TCN_3DCoords;
				ProjectedModifier.TexCoordProjected			= 1;
				ProjectedModifier.TexCoordSource			= TCS_Stream0;
				ApplyTexModifier(MaterialState->Stages[StagesUsed],&ProjectedModifier);

				switch(ProjectorMaterial->BaseMaterialBlending)
				{
					case PB_AlphaBlend:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
						break;
					}
					case PB_Modulate:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE2X;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
						break;
					}
					case PB_None:
					default:
					{
						switch(ProjectorMaterial->FrameBufferBlending)
						{
							case PB_Add:
							{
								MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE;
								MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE;
								MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TEXTURE;
								MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG2;
								MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_TEXTURE;
								break;
							}
							case PB_AlphaBlend:
							{
								MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG2;
								MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TEXTURE;
								MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_MODULATE;
								MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_DIFFUSE;
								MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_TEXTURE;
								break;
							}
							case PB_Modulate:
							default:
							{
								MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDDIFFUSEALPHA;
								MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
								MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TFACTOR;
								MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
								MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
								MaterialState->TFactorColor = FColor(127,127,127,0);
								break;
							}
						};
						break;
					}
				};

				StagesUsed++;
			}
			else
			{
				if(ErrorString)
					*ErrorString = TEXT("No stages left for projected texture");

				if(ErrorMaterial)
					*ErrorMaterial = ProjectorMaterial;

				return 0;
			}
		}
		else
		{
			// Handle the projected texture.

			ProjectedModifier.ModifyTextureTransforms = 1;
			ProjectedModifier.TexCoordCount = TCN_3DCoords;
			ProjectedModifier.TexCoordProjected = 1;
			ProjectedModifier.TexCoordSource = TCS_Stream0;

			if(!HandleCombinedMaterial(ProjectorMaterial->Projected,PassesUsed,StagesUsed,FreeStagesUsed,ProjectedModifier,0,ErrorString,ErrorMaterial))
				return 0;

			if(ProjectorMaterial->BaseMaterialBlending != PB_None && BaseDiffuse)
			{
				// Handle the base material.

				FD3DModifierInfo	BaseModifier = InModifierInfo;
				UBitmapMaterial*	BaseBitmap = CheckMaterial<UBitmapMaterial>(this,BaseDiffuse,&BaseModifier,0);

				if(!BaseBitmap)
				{
					if(ErrorString)
						*ErrorString = TEXT("Either projected texture or base material must be simple.");

					if(ErrorMaterial)
						*ErrorMaterial = ProjectorMaterial;

					return 0;
				}

				if(StagesUsed < MaxTextureStages + FreeStagesUsed)
				{
					SetShaderBitmap(MaterialState->Stages[StagesUsed],BaseBitmap);
					ApplyTexModifier(MaterialState->Stages[StagesUsed],&BaseModifier);
					MaterialState->Stages[StagesUsed].TexCoordIndex = 1;

					switch(ProjectorMaterial->BaseMaterialBlending)
					{
						case PB_AlphaBlend:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_BLENDTEXTUREALPHA;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_Modulate:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE2X;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_MODULATE2X;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_None:
						default:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG1;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							break;
						}		
					};

					StagesUsed++;
				}
				else
				{
					if(ErrorString)
						*ErrorString = TEXT("No stages left for base material");

					if(ErrorMaterial)
						*ErrorMaterial = BaseDiffuse;

					return 0;
				}
			}
		}

		if(BaseOpacity)
		{
			// Handle the base material opacity.

			if(StagesUsed < MaxTextureStages + FreeStagesUsed)
			{
				FD3DModifierInfo	BaseOpacityModifier = InModifierInfo;
				UBitmapMaterial*	BaseOpacityBitmap = CheckMaterial<UBitmapMaterial>(this,BaseOpacity);

				if(BaseOpacityBitmap)
				{
					SetShaderBitmap(MaterialState->Stages[StagesUsed],BaseOpacityBitmap);
					ApplyTexModifier(MaterialState->Stages[StagesUsed],&BaseOpacityModifier);
					MaterialState->Stages[StagesUsed].TexCoordIndex = 1;

					switch(ProjectorMaterial->FrameBufferBlending)
					{
						case PB_Add:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG2;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_AlphaBlend:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG2;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_MODULATE;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_Modulate:
						default:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TFACTOR;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT;
							MaterialState->TFactorColor = FColor(127,127,127,0);
							break;
						}
					};

					StagesUsed++;
				}
				else
				{
					if(ErrorString)
						*ErrorString = TEXT("Base opacity isn't bitmap");

					if(ErrorMaterial)
						*ErrorMaterial = BaseOpacity;

					return 0;
				}
			}
			else
			{
				if(ErrorString)
					*ErrorString = TEXT("No stages left for base opacity");

				if(ErrorMaterial)
					*ErrorMaterial = ProjectorMaterial;

				return 0;
			}
		}

		if( !RenDev->IsGeForce && (ProjectorMaterial->BaseMaterialBlending != PB_None || !ProjectedBitmap) && (ProjectorMaterial->bGradient || !ProjectorMaterial->bProjectOnBackfaces))
		{
			// Handle the vertex attenuation.

			if(StagesUsed < MaxBlendStages)
			{
				switch(ProjectorMaterial->FrameBufferBlending)
				{
					case PB_Add:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG2;
						MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
						break;
					}
					case PB_AlphaBlend:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG2;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_MODULATE;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_DIFFUSE;
						MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
						break;
					}
					case PB_Modulate:
					default:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDDIFFUSEALPHA;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TFACTOR;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT;
						MaterialState->TFactorColor = FColor(127,127,127,0);
						break;
					}
				};

				StagesUsed++;

				if(MaxBlendStages > MaxTextureStages)
					FreeStagesUsed++;
			}
			else
			{
				if(ErrorString)
					*ErrorString = TEXT("No stages left for attenuation");

				if(ErrorMaterial)
					*ErrorMaterial = ProjectorMaterial;

				return 0;
			}
		}
	}
	else
	{
		// Calculate the inverse view matrix.

		D3DXMATRIX	InvViewMatrix;

		D3DXMatrixInverse(&InvViewMatrix,NULL,(D3DXMATRIX*) &CurrentState->WorldToCamera);

		// Calculate the camera to projected texture matrix.

		D3DXMATRIX	CameraToTexture;

		D3DXMatrixMultiply(&CameraToTexture,&InvViewMatrix,(D3DXMATRIX*) &ProjectorMaterial->Matrix);

		// See if the projected texture is simple.

		FD3DModifierInfo	ProjectedModifier;
		UBitmapMaterial*	ProjectedBitmap = CheckMaterial<UBitmapMaterial>(this,ProjectorMaterial->Projected,&ProjectedModifier,0);

		if(ProjectedBitmap)
		{
			// Handle the base material.

			if(ProjectorMaterial->BaseMaterialBlending != PB_None && BaseDiffuse)
			{
				if(!HandleCombinedMaterial(BaseDiffuse,PassesUsed,StagesUsed,FreeStagesUsed,InModifierInfo,0,ErrorString,ErrorMaterial))
					return 0;
			}

			// Handle the projected texture.

			if(StagesUsed < MaxTextureStages + FreeStagesUsed)
			{
				SetShaderBitmap(MaterialState->Stages[StagesUsed],ProjectedBitmap);
				D3DXMatrixMultiply((D3DXMATRIX*)&MaterialState->Stages[StagesUsed].TextureTransformMatrix,&CameraToTexture,(D3DXMATRIX*)&ProjectedModifier.Matrix);
				MaterialState->Stages[StagesUsed].TexCoordCount = D3DTTFF_COUNT3;
				MaterialState->Stages[StagesUsed].TexCoordIndex = D3DTSS_TCI_CAMERASPACEPOSITION;
				MaterialState->Stages[StagesUsed].TextureTransformsEnabled = 1;

				if(ProjectorMaterial->bProjected)
					MaterialState->Stages[StagesUsed].TexCoordCount |= D3DTTFF_PROJECTED;

				switch(ProjectorMaterial->BaseMaterialBlending)
				{
					case PB_AlphaBlend:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
						break;
					}
					case PB_Modulate:
					{
						MaterialState->Stages[StagesUsed].ColorOp = DUMMY_MODULATE2X;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
						break;
					}
					case PB_None:
					default:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
						break;
					}
				};

				StagesUsed++;
			}
			else
			{
				if(ErrorString)
					*ErrorString = TEXT("No stages left for projected texture");

				if(ErrorMaterial)
					*ErrorMaterial = ProjectorMaterial;

				return 0;
			}
		}
		else
		{
			// Handle the projected texture.

			ProjectedModifier.ModifyTextureTransforms = 1;
			ProjectedModifier.TexCoordSource = TCS_WorldCoords;
			ProjectedModifier.TexCoordCount = TCN_3DCoords;
			ProjectedModifier.TexCoordProjected = ProjectorMaterial->bProjected;
			ProjectedModifier.Matrix = ProjectorMaterial->Matrix;

			if(!HandleCombinedMaterial(ProjectorMaterial->Projected,PassesUsed,StagesUsed,FreeStagesUsed,ProjectedModifier,0,ErrorString,ErrorMaterial))
				return 0;

			if(ProjectorMaterial->BaseMaterialBlending != PB_None && BaseDiffuse)
			{
				// Handle the base material.

				FD3DModifierInfo	BaseModifier = InModifierInfo;
				UBitmapMaterial*	BaseBitmap = CheckMaterial<UBitmapMaterial>(this,BaseDiffuse,&BaseModifier,0);

				if(!BaseBitmap)
				{
					if(ErrorString)
						*ErrorString = TEXT("Either projected texture or base material must be simple.");

					if(ErrorMaterial)
						*ErrorMaterial = ProjectorMaterial;

					return 0;
				}

				if(StagesUsed < MaxTextureStages + FreeStagesUsed)
				{
					SetShaderBitmap(MaterialState->Stages[StagesUsed],BaseBitmap);
					ApplyTexModifier(MaterialState->Stages[StagesUsed],&BaseModifier);

					switch(ProjectorMaterial->BaseMaterialBlending)
					{
						case PB_AlphaBlend:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_BLENDTEXTUREALPHA;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_Modulate:
						{
							MaterialState->Stages[StagesUsed].ColorOp = DUMMY_MODULATE2X;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = DUMMY_MODULATE2X;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_None:
						default:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG1;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							break;
						}		
					};

					StagesUsed++;
				}
				else
				{
					if(ErrorString)
						*ErrorString = TEXT("No stages left for base material");

					if(ErrorMaterial)
						*ErrorMaterial = BaseDiffuse;

					return 0;
				}
			}
		}

		if(ProjectorMaterial->bGradient)
		{
			// Setup camera to gradient transform.

			D3DXMATRIX	CameraToGradient;

			D3DXMatrixMultiply(&CameraToGradient,&InvViewMatrix,(D3DXMATRIX*) &ProjectorMaterial->GradientMatrix);

			// Handle the gradient texture.

			if(StagesUsed < MaxTextureStages + FreeStagesUsed)
			{
				SetShaderBitmap(MaterialState->Stages[StagesUsed],ProjectorMaterial->Gradient);

				MaterialState->Stages[StagesUsed].TextureTransformsEnabled = 1;
				MaterialState->Stages[StagesUsed].TexCoordIndex = D3DTSS_TCI_CAMERASPACEPOSITION;
				MaterialState->Stages[StagesUsed].TexCoordCount = D3DTTFF_COUNT3;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix = CameraToGradient;

				switch(ProjectorMaterial->FrameBufferBlending)
				{
					case PB_Add:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG2;
						MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
						break;
					}
					case PB_AlphaBlend:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG2;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_MODULATE;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
						break;
					}
					case PB_Modulate:
					default:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TFACTOR;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT;
						MaterialState->TFactorColor = FColor(127,127,127,0);
						break;
					}
				};

				StagesUsed++;
			}
			else
			{
				if(ErrorString)
					*ErrorString = TEXT("No stages left for gradient texture");

				if(ErrorMaterial)
					*ErrorMaterial = ProjectorMaterial;

				return 0;
			}
		}

		if(ProjectorMaterial->bProjectOnAlpha && BaseOpacity)
		{
			// Handle the gradient texture.

			if(StagesUsed < MaxTextureStages + FreeStagesUsed)
			{
				FD3DModifierInfo	BaseOpacityModifier = InModifierInfo;
				UBitmapMaterial*	BaseOpacityBitmap = CheckMaterial<UBitmapMaterial>(this,BaseOpacity);

				if(BaseOpacityBitmap)
				{
					SetShaderBitmap(MaterialState->Stages[StagesUsed],BaseOpacityBitmap);
					ApplyTexModifier(MaterialState->Stages[StagesUsed],&BaseOpacityModifier);

					switch(ProjectorMaterial->FrameBufferBlending)
					{
						case PB_Add:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG2;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_AlphaBlend:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG2;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_MODULATE;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
							MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
							break;
						}
						case PB_Modulate:
						default:
						{
							MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
							MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
							MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TFACTOR;
							MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
							MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT;
							MaterialState->TFactorColor = FColor(127,127,127,0);
							break;
						}
					};

					StagesUsed++;
				}
				else
				{
					if(ErrorString)
						*ErrorString = TEXT("Base opacity isn't bitmap");

					if(ErrorMaterial)
						*ErrorMaterial = BaseOpacity;

					return 0;
				}
			}
			else
			{
				if(ErrorString)
						*ErrorString = TEXT("No stages left for base opacity");

				if(ErrorMaterial)
					*ErrorMaterial = ProjectorMaterial;

				return 0;
			}
		}

		if(!ProjectorMaterial->bProjectOnBackfaces)
		{
			static FProjectorCubeMap	ProjectorCubeMap(32);

			// Handle the lighting modulation.

			if(StagesUsed < MaxTextureStages + FreeStagesUsed)
			{
				MaterialState->Stages[StagesUsed].Texture = CacheTexture(&ProjectorCubeMap);
				MaterialState->Stages[StagesUsed].TextureTransformsEnabled = 1;
				MaterialState->Stages[StagesUsed].TexCoordIndex = D3DTSS_TCI_CAMERASPACENORMAL;
				MaterialState->Stages[StagesUsed].TexCoordCount = D3DTTFF_COUNT3;
				MaterialState->Stages[StagesUsed].TextureAddressU = RenDev->CubemapTextureAddressing;
				MaterialState->Stages[StagesUsed].TextureAddressV = RenDev->CubemapTextureAddressing;
				MaterialState->Stages[StagesUsed].TextureAddressW = RenDev->CubemapTextureAddressing;

				FCoords	NormalCameraToTexture = ((FMatrix*)&CameraToTexture)->Coords();

				NormalCameraToTexture.XAxis.Normalize();
				NormalCameraToTexture.YAxis.Normalize();
				NormalCameraToTexture.ZAxis.Normalize();

				MaterialState->Stages[StagesUsed].TextureTransformMatrix._11 = NormalCameraToTexture.XAxis.X;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._21 = NormalCameraToTexture.XAxis.Y;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._31 = NormalCameraToTexture.XAxis.Z;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._41 = 0.0f;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._12 = NormalCameraToTexture.YAxis.X;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._22 = NormalCameraToTexture.YAxis.Y;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._32 = NormalCameraToTexture.YAxis.Z;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._42 = 0.0f;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._13 = -NormalCameraToTexture.ZAxis.X;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._23 = -NormalCameraToTexture.ZAxis.Y;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._33 = -NormalCameraToTexture.ZAxis.Z;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._43 = 0.0f;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._34 = 0.0f;
				MaterialState->Stages[StagesUsed].TextureTransformMatrix._44 = 1.0f;

				switch(ProjectorMaterial->FrameBufferBlending)
				{
					case PB_Add:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_MODULATE;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG2;
						MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
						break;
					}
					case PB_AlphaBlend:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_SELECTARG2;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_MODULATE;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_TEXTURE;
						MaterialState->Stages[StagesUsed].AlphaArg2 = D3DTA_CURRENT;
						break;
					}
					case PB_Modulate:
					default:
					{
						MaterialState->Stages[StagesUsed].ColorOp = D3DTOP_BLENDTEXTUREALPHA;
						MaterialState->Stages[StagesUsed].ColorArg1 = D3DTA_CURRENT;
						MaterialState->Stages[StagesUsed].ColorArg2 = D3DTA_TFACTOR;
						MaterialState->Stages[StagesUsed].AlphaOp = D3DTOP_SELECTARG1;
						MaterialState->Stages[StagesUsed].AlphaArg1 = D3DTA_CURRENT;
						MaterialState->TFactorColor = FColor(127,127,127,0);
						break;
					}
				};

				StagesUsed++;
			}
			else
			{
				if(ErrorString)
					*ErrorString = TEXT("No stages left for directional attenuation");

				if(ErrorMaterial)
					*ErrorMaterial = ProjectorMaterial;

				return 0;
			}
		}
	}

	// Handle frame buffer blending.

	MaterialState->TwoSided = ProjectorMaterial->bTwoSided;
	MaterialState->AlphaBlending = 1;
	MaterialState->ZWrite = 0;

	switch(ProjectorMaterial->FrameBufferBlending)
	{
		case PB_Add:
		{
			MaterialState->SrcBlend = D3DBLEND_ONE;
			MaterialState->DestBlend = D3DBLEND_ONE;
			MaterialState->OverrideFogColor	= 1;
			MaterialState->OverriddenFogColor = FColor(0,0,0,0);
			break;
		}
		case PB_AlphaBlend:
		{
			MaterialState->SrcBlend = D3DBLEND_SRCALPHA;
			MaterialState->DestBlend = D3DBLEND_INVSRCALPHA;
			break;
		}
		case PB_Modulate:
		default:
		{
			MaterialState->SrcBlend = D3DBLEND_DESTCOLOR;
			MaterialState->DestBlend = D3DBLEND_SRCCOLOR;
			MaterialState->OverrideFogColor	= 1;
			MaterialState->OverriddenFogColor = FColor(127,127,127,0);
			break;
		}
	};

	CurrentState->MaterialPasses[PassesUsed]->StagesUsed = StagesUsed;
	CurrentState->NumMaterialPasses = ++PassesUsed;

	if(PassesUsed == 1)
		return 1;
	else
		return 0;

	unguard;
}

UBOOL FD3DRenderInterface::SetHardwareShaderMaterial( UHardwareShader* InHardwareShader, FString* ErrorString, UMaterial** ErrorMaterial)
{
	// Check for vertex shader support
	if(RenDev->DeviceCaps8.VertexShaderVersion < 1)
	{
		if(ErrorString) *ErrorString = TEXT("No vertex shader support detected");
		if(ErrorMaterial) *ErrorMaterial = InHardwareShader->FallbackMaterial;
		return false;
	}
	// check for pixel shader support
	if(RenDev->DeviceCaps8.PixelShaderVersion < 1)
	{
		if(ErrorString) *ErrorString = TEXT("No pixle shader support detected");
		if(ErrorMaterial) *ErrorMaterial = InHardwareShader->FallbackMaterial;
		return false;
	}

	INT PassesUsed = 0;

	for(INT PassIndex = 0; PassIndex < InHardwareShader->NumPasses; PassIndex++)
	{
		INT StagesUsed = RenDev->DeviceCaps8.MaxSimultaneousTextures;

		if(PassIndex != 0)
		{
			NEWPASS();
		}

		// Set the hardware vertex shade
 		if(!SetProgrammableVertexShader(InHardwareShader->VertexShaders[PassIndex], PassesUsed, ErrorString))
		{
			if(ErrorString) *ErrorString = TEXT("Failed to set vertex shader");
			if(ErrorMaterial) *ErrorMaterial = InHardwareShader->FallbackMaterial;
			return false;
		}

		for(INT j = 0; j < NUM_VSHADER_CONSTANTS; j++)
		{
			CurrentState->MaterialPasses[PassIndex]->VSConstants[j] = InHardwareShader->VSConstants[j][PassIndex];
		}
		for(INT j = 0; j < NUM_PSHADER_CONSTANTS; j++)
		{
			CurrentState->MaterialPasses[PassIndex]->PSConstants[j] = InHardwareShader->PSConstants[j][PassIndex];
		}

		// Set the pixel shader
		CurrentState->MaterialPasses[PassesUsed]->PixelShader2 = LoadPixelShader(InHardwareShader->PixelShaders[PassIndex]);

		// Set the texture
		UBitmapMaterial* BitmapMaterial;
		FD3DModifierInfo ModifierInfo;
		for(UINT i = 0; i < RenDev->DeviceCaps8.MaxSimultaneousTextures; i++)
		{
			if((BitmapMaterial=CheckMaterial<UBitmapMaterial>(this, InHardwareShader->Textures[i + (PassIndex * MAX_TEXTURE_STAGES)], &ModifierInfo)) != NULL)
			{
				SetShaderBitmap(CurrentState->MaterialPasses[PassesUsed]->Stages[i], BitmapMaterial);
				ApplyTexModifier(CurrentState->MaterialPasses[PassesUsed]->Stages[i], &ModifierInfo);
			}
			CurrentState->MaterialPasses[PassesUsed]->Stages[i].TexCoordIndex = i;
		}
		StagesUsed = RenDev->DeviceCaps8.MaxSimultaneousTextures;

		// Set the render states
		CurrentState->MaterialPasses[PassesUsed]->AlphaBlending = InHardwareShader->AlphaBlending[PassesUsed];
		CurrentState->MaterialPasses[PassesUsed]->AlphaTest = InHardwareShader->AlphaTest[PassesUsed];
		CurrentState->MaterialPasses[PassesUsed]->AlphaRef = InHardwareShader->AlphaRef[PassesUsed];
		CurrentState->MaterialPasses[PassesUsed]->ZTest = InHardwareShader->ZTest[PassesUsed];
		CurrentState->MaterialPasses[PassesUsed]->ZWrite = InHardwareShader->ZWrite[PassesUsed];
		CurrentState->MaterialPasses[PassesUsed]->SrcBlend = (D3DBLEND)InHardwareShader->SrcBlend[PassesUsed];
		CurrentState->MaterialPasses[PassesUsed]->DestBlend = (D3DBLEND)InHardwareShader->DestBlend[PassesUsed];

		CurrentState->MaterialPasses[PassesUsed]->StagesUsed = StagesUsed;
	}

	CurrentState->NumMaterialPasses = ++PassesUsed;

	return true;
}

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
