/*=============================================================================
	D3DRenderInterface.h: Unreal Direct3D rendering interface definition.
	Copyright 2001 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
=============================================================================*/

#ifndef HEADER_D3DRENDERINTERFACE
#define HEADER_D3DRENDERINTERFACE

///
//	FD3DModifierInfo
//
struct FD3DModifierInfo
{
	UBOOL				ModifyTextureTransforms;
	UBOOL				ModifyFramebufferBlending;
	UBOOL				ModifyColor;
	UBOOL				ModifyOpacity;

	// texture modifier
	FMatrix				Matrix;
	BYTE				TexCoordSource;
	BYTE				TexCoordCount;
	UBOOL				TexCoordProjected;

	// framebuffer blend modifier
    BYTE				FrameBufferBlending;
    UBOOL				ZWrite;
    UBOOL				ZTest;
    UBOOL				AlphaTest;
    UBOOL				TwoSided;
    BYTE				AlphaRef;

	// color modifier
	FColor				TFactorColor;
	UBOOL				AlphaBlend;

	// fallback info
	UMaterial*			BestFallbackPoint;

	// opacity modifier
	UMaterial*			Opacity;
	UBOOL				OpacityOverrideTexModifier;

	// Constructor
	FD3DModifierInfo();

	// FD3DModifierInfo interface
	void SetDetailTextureScale( FLOAT Scale );
};

///
//	FD3DMaterialStateStage
//
struct FD3DMaterialStateStage
{
	FD3DTexture*		Texture;
	D3DTEXTUREADDRESS	TextureAddressU,
						TextureAddressV,
						TextureAddressW;
    FLOAT               TextureMipLODBias;
	D3DTEXTUREOP 		ColorOp,
						AlphaOp;
	DWORD				ColorArg0,
						ColorArg1,
						ColorArg2,
						AlphaArg0,
						AlphaArg1,
						AlphaArg2,
						ResultArg;

	DWORD				TexCoordIndex;
	DWORD				TexCoordCount;
	UBOOL				TextureTransformsEnabled;
	D3DMATRIX			TextureTransformMatrix;

	FD3DMaterialStateStage();
};

///
//	FD3DMaterialState
//
class FD3DMaterialState
{
public:
	EPixelShader		PixelShader;
	UBOOL				AlphaBlending;			// Alpha blending is enabled
	UBOOL				AlphaTest;				// Alpha test is enabled?
	BYTE				AlphaRef;				// If alpha testing, the value to compare against
	UBOOL				ZTest;					// Test zbuffer
	UBOOL				ZWrite;					// Write to zbuffer
	UBOOL				TwoSided;
	EFillMode			FillMode;				// Wireframe, flatshaded or solid
	D3DCOLOR			TFactorColor;
	D3DBLEND			SrcBlend,
						DestBlend;
	UBOOL				OverrideFogColor;
	FColor				OverriddenFogColor;

	INT					StagesUsed;
	FD3DMaterialStateStage	Stages[8];

	INT	NumRefs;

	FD3DPixelShader2*	PixelShader2;
	FD3DVertexShader2*	VertexShader2;
	EVertexShader		VertexShader;

	FSConstantsInfo	VSConstants[NUM_VSHADER_CONSTANTS];
	FSConstantsInfo	PSConstants[NUM_PSHADER_CONSTANTS];

	FD3DMaterialState();
};

//
//	FD3DMaterialStatePool
//

class FD3DMaterialStatePool
{
public:

	TArray<FD3DMaterialState*>	FreeStates;

	// Destructor.

	~FD3DMaterialStatePool()
	{
		for(INT StateIndex = 0;StateIndex < FreeStates.Num();StateIndex++)
			delete FreeStates(StateIndex);
	}

	// AllocateState

	FD3DMaterialState* AllocateState(FD3DMaterialState* DefaultState)
	{
		FD3DMaterialState*	Result;

		if(FreeStates.Num())
			Result = FreeStates.Pop();
		else
			Result = new(TEXT("D3DMaterialState")) FD3DMaterialState();

		if(DefaultState)
			appMemcpy(Result,DefaultState,sizeof(FD3DMaterialState));

		Result->NumRefs = 1;

		return Result;
	}

	// FreeState

	void FreeState(FD3DMaterialState* State)
	{
		FreeStates.AddItem(State);
	}
};

//
//	FD3DRenderInterface
//
class FD3DRenderInterface : public FRenderInterface
{
public:

	///
	//	FD3DSavedState
	//
	class FD3DSavedState
	{
	public:

		IDirect3DSurface8*	RenderTargetSurface;
		IDirect3DSurface8*	DepthStencilSurface;

		INT					ViewportX,
							ViewportY,
							ViewportWidth,
							ViewportHeight;

		INT					ZBias;

		ECompareFunction	StencilTest;
		EStencilOp			StencilFailOp,
							StencilZFailOp,
							StencilPassOp;
		DWORD				StencilRef,
							StencilMask,
							StencilWriteMask;

		FMatrix				LocalToWorld,
							WorldToCamera,
							CameraToScreen;

		FD3DVertexShader*	VertexShader;
		FD3DVertexStream*	Streams[16];

		FShaderDeclaration  ShaderDeclaration;
		FD3DVertexShader2*  VertexShader2;

		FPlane				VertexShaderConstants[NUM_VSHADER_CONSTANTS];
		FPlane				PixelShaderConstants[NUM_PSHADER_CONSTANTS];

		INT					StreamStrides[16],
							NumStreams;

		FD3DIndexBuffer*	IndexBuffer;
		INT					IndexBufferBase;

		D3DCULL				CullMode;

		UBOOL				UseDetailTexturing;
		UBOOL				UseDynamicLighting;
		UBOOL				UseStaticLighting;
		UBOOL				LightingModulate2X;
		UBOOL				LightingOnly;
		FD3DTexture*		Lightmap;
		FSphere				LitSphere;
		FColor				AmbientLightColor;
		D3DLIGHT8			Lights[8];
		UBOOL				LightEnabled[8];
		FDynamicLight*		EngineLights[8]; // Used by vertex/pixel shaders to get useful versions of the lights.

		UBOOL				DistanceFogEnabled;
		FLOAT				DistanceFogStart,
							DistanceFogEnd;
		FColor				DistanceFogColor;

		FLOAT				NPatchTesselation;

		FD3DMaterialState*	MaterialPasses[8];
		INT					NumMaterialPasses;
		FD3DMaterialState*	CurrentMaterialState;
	
		FD3DSavedState();
	};


	UD3DRenderDevice*		RenDev;
	UViewport*				Viewport;
#define MAX_STATESTACKDEPTH	128
	FD3DSavedState			SavedStates[MAX_STATESTACKDEPTH];
	FD3DSavedState*			CurrentState;
	FD3DMaterialState		DefaultPass;

	INT						SavedStateIndex;

	FD3DMaterialStatePool	MaterialStatePool;

	EPrecacheMode			PrecacheMode;

	// Hit stack.
	TArray<BYTE>			HitStack;
	BYTE*					HitData;
	INT*					HitSize;
	INT						HitCount;
	DWORD					HitPixels[HIT_SIZE][HIT_SIZE];

	// Constructor.
	FD3DRenderInterface(UD3DRenderDevice* InRenDev);

	// FRenderInterface interface.
	virtual void PushState();
	virtual void PopState();

	virtual UBOOL SetRenderTarget(FRenderTarget* RenderTarget);
	virtual void SetViewport(INT X,INT Y,INT Width,INT Height);
	virtual void Clear(UBOOL UseColor,FColor Color,UBOOL UseDepth,FLOAT Depth,UBOOL UseStencil,DWORD Stencil);

	virtual void PushHit(const BYTE* Data,INT Count);
	virtual void PopHit(INT Count,UBOOL Force);

	virtual void SetCullMode(ECullMode CullMode);

	virtual void SetAmbientLight(FColor Color);
	virtual void EnableLighting(UBOOL UseDynamic, UBOOL UseStatic, UBOOL Modulate2X, FBaseTexture* LightmapTexture, UBOOL LightingOnly, FSphere LitSphere );
	virtual void SetLight(INT LightIndex,FDynamicLight* Light,FLOAT Scale);
 
	virtual void SetNPatchTesselation( FLOAT Tesselation );
	virtual void SetDistanceFog(UBOOL Enable,FLOAT FogStart,FLOAT FogEnd,FColor Color);
	virtual void SetGlobalColor(FColor Color);

	virtual void SetPointSprite( UBOOL EnablePointSprite );
	virtual void SetPointScale( UBOOL EnablePointScale, DWORD A, DWORD B, DWORD C );

	virtual void SetTransform(ETransformType Type,FMatrix Matrix);

	virtual void SetMaterial(UMaterial* Material, FString* ErrorString, UMaterial** ErrorMaterial, INT* NumPasses);
	virtual void SetZBias(INT ZBias);
	virtual void SetStencilOp(ECompareFunction Test,DWORD Ref,DWORD Mask,EStencilOp FailOp,EStencilOp ZFailOp,EStencilOp PassOp,DWORD WriteMask);
	
	virtual void SetPrecacheMode( EPrecacheMode PrecacheMode );
	
	virtual INT  SetVertexStreams(EVertexShader Shader,FVertexStream** Streams,INT NumStreams);
	virtual INT  SetDynamicStream(EVertexShader Shader,FVertexStream* Stream);

	virtual INT  SetIndexBuffer(FIndexBuffer* IndexBuffer,INT BaseVertexIndex);
	virtual INT  SetDynamicIndexBuffer(FIndexBuffer* IndexBuffer,INT BaseVertexIndex);

	virtual INT LockDynamicStream(FVertexStream* Stream,BYTE** VertexBufferContents);
	virtual void UnlockDynamicStream(FVertexStream* Stream);

	virtual INT LockDynamicIndexBuffer(FIndexBuffer* IndexBuffer,BYTE** IndexBufferContents);
	virtual void UnlockDynamicIndexBuffer(FIndexBuffer* IndexBuffer,INT BaseIndex);

	virtual void SetVertexShader(EVertexShader Shader,FVertexStream* Stream);

	virtual void DrawPrimitive(EPrimitiveType PrimitiveType,INT FirstIndex,INT NumPrimitives,INT MinIndex,INT MaxIndex);

	// Helper function.
	virtual D3DSTENCILOP GetD3DStencilOp( EStencilOp StencilOp );
	virtual D3DCMPFUNC   GetD3DCompFunc( ECompareFunction CompFunc );

	// FD3DRenderInterface interface.
	void Locked( UViewport* InViewport, BYTE* InHitData,INT* InHitSize);
	void Unlocked();

private:
	FD3DTexture* CacheTexture(FBaseTexture* Texture);
	void SetMaterialBlending( FD3DMaterialState* NewMaterialState, D3DCULL CullMode );
	
	// Various material handlers
	UBOOL SetShaderMaterial( UShader* InShader, FD3DModifierInfo InModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial );
	UBOOL SetSimpleMaterial( UMaterial* InMaterial, FD3DModifierInfo InModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial );
	UBOOL SetTerrainMaterial( UTerrainMaterial* InTerrainMaterial, FD3DModifierInfo InModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial );
	UBOOL SetProjectorMaterial( UProjectorMaterial* ProjectorMaterial, FD3DModifierInfo InModifierInfo, FString* ErrorString, UMaterial** ErrorMaterial );
	UBOOL SetHardwareShaderMaterial( UHardwareShader* InHardwareShader, FString* ErrorString, UMaterial** ErrorMaterial);
	FD3DPixelShader2* LoadPixelShader(FPixelShader* Shader);
	FD3DVertexShader2* LoadVertexShader(FVertexShader* Shader);
	UBOOL SetProgrammableVertexShader(FVertexShader* InShader, INT Pass, FString* ErrorString);
	void SetFixedFunctionVertexShader();
	void SetShaderConstants(FPlane ConstantArray[], FSConstantsInfo Constants[], INT NumConstants);
	void  SetLightingOnlyMaterial();

	// Helpers
	inline UBOOL HandleCombinedMaterial( UMaterial* InMaterial, INT& PassesUsed, INT& StagesUsed, INT& FreeStagesUsed, FD3DModifierInfo ModifierInfo, UBOOL InvertOutputAlpha=0, FString* ErrorString=NULL, UMaterial** ErrorMaterial=NULL );
	inline void  SetShaderBitmap( FD3DMaterialStateStage& Stage, UBitmapMaterial* BitmapMaterial );
	inline void  HandleOpacityBitmap( FD3DMaterialStateStage& Stage, UBitmapMaterial* Bitmap, UBOOL ModulateAlpha=0 );
	inline void  HandleVertexOpacity( FD3DMaterialStateStage& Stage, UVertexColor* VertexColor );
	inline void  HandleSpecular_SP( FD3DMaterialStateStage& Stage, UBitmapMaterial* Bitmap, UBOOL UseSpecularity, UBOOL UseConstantSpecularity );
	inline void  HandleSelfIllumination_SP( FD3DMaterialStateStage& Stage, UBitmapMaterial* Bitmap );
	inline void  HandleLighting_MP( FD3DMaterialStateStage& Stage, FD3DTexture* Lightmap, UBOOL UseDiffuse );
	inline void  HandleLightmap_SP( FD3DMaterialStateStage& Stage, FD3DTexture* Lightmap );
	inline void  HandleDiffuse_Patch( FD3DMaterialStateStage& Stage, UBOOL Modulate2X = 0 );
	inline void  HandleDiffuse_SP( FD3DMaterialStateStage& Stage );
	inline void  HandleDiffuse_Stage( FD3DMaterialStateStage& Stage, UBOOL Modulate2X = 0 );
	inline void  HandleDetail( UBitmapMaterial* DetailBitmap, INT& PassesUsed, INT& StagesUsed, INT& FreeStagesUsed, FD3DModifierInfo InModifierInfo, UBOOL SinglePassOnly=0 );
	inline void	 HandleTFactor_SP( FD3DMaterialStateStage& Stage );
	inline void  ApplyTexModifier( FD3DMaterialStateStage& Stage, FD3DModifierInfo* ModifierInfo );
	inline void  ApplyFinalBlend( FD3DModifierInfo* InModifierInfo );

	FString DescribeStage(FD3DMaterialState* MaterialState,INT StageIndex,UBOOL Alpha);
	FString DescribeArg(FD3DMaterialState* MaterialState,INT StageIndex,DWORD Arg,UBOOL Alpha);
};


/*----------------------------------------------------------------------------
	CheckMaterial.
----------------------------------------------------------------------------*/

//
// CHECKFALLBACK - return the Fallback material if it's decided we should use it.
//
#define CHECKFALLBACK( InMaterial )		\
			(UseFallbacks && InMaterial ? InMaterial->CheckFallback() : InMaterial)

//
// Attempt to cast a material/modifier chain to the appropriate class.
// If the chain ends with a material of the specfied class, remember any 
// modifier info along the way.
//
template<class C> C* CheckMaterial(FD3DRenderInterface* RI, UMaterial* InMaterial, FD3DModifierInfo* ModifierInfo=NULL, UBOOL UseFallbacks=0)
{
	C* Material = Cast<C>(CHECKFALLBACK(InMaterial));
	if( Material )
		return Material;

	// See if we have a chain of Modifiers eventually pointing to material of class C
	UModifier* Modifier = Cast<UModifier>(CHECKFALLBACK(InMaterial));
	while( Modifier )
	{
		Material = Cast<C>(CHECKFALLBACK(Modifier->Material));
		Modifier = Cast<UModifier>(CHECKFALLBACK(Modifier->Material));
	}

	// If we have a C, go through the Modifier list and combine the matrices for any TexModifiers.
	if( Material && ModifierInfo )
	{
		UBOOL NeedSource = 1;
		Modifier = Cast<UModifier>(CHECKFALLBACK(InMaterial));
		while( Modifier )
		{
			// Remember the most specific fallback we see.
			if( Modifier->HasFallback() && UseFallbacks )
				ModifierInfo->BestFallbackPoint = Modifier;

			// Check for TexModifier.
			UTexModifier* TexModifier = Cast<UTexModifier>(Modifier);
			if( TexModifier )
			{
				FMatrix* TexMatrix = TexModifier->GetMatrix(RI->Viewport->Actor->Level->TimeSeconds);
				if( TexMatrix )
				{
					ModifierInfo->ModifyTextureTransforms = 1;
					ModifierInfo->Matrix *= *TexMatrix;
				}
				// Locate the first non-passthrough texture coordinate source.
				if( NeedSource && TexModifier->TexCoordSource!=TCS_NoChange )
				{
					ModifierInfo->ModifyTextureTransforms = 1;
					ModifierInfo->TexCoordSource	= TexModifier->TexCoordSource;
					ModifierInfo->TexCoordCount		= TexModifier->TexCoordCount;
					ModifierInfo->TexCoordProjected	= TexModifier->TexCoordProjected;
					NeedSource = 0;
				}
			}

			// Check for FinalBlend modifier.
			UFinalBlend* FinalBlend = Cast<UFinalBlend>(Modifier);
			if( FinalBlend ) 	
			{
				ModifierInfo->ModifyFramebufferBlending = 1;

				ModifierInfo->FrameBufferBlending	= FinalBlend->FrameBufferBlending;
				ModifierInfo->ZWrite				= FinalBlend->ZWrite;
				ModifierInfo->ZTest					= FinalBlend->ZTest;
				ModifierInfo->AlphaTest				= FinalBlend->AlphaTest;
				ModifierInfo->TwoSided				= FinalBlend->TwoSided;
				ModifierInfo->AlphaRef				= FinalBlend->AlphaRef;
			}

			// Check for ColorModifier modifier.
			UColorModifier* ColorModifier = Cast<UColorModifier>(Modifier);
			if( ColorModifier )
			{
				ModifierInfo->ModifyColor	= 1;
				
				ModifierInfo->AlphaBlend	|= ColorModifier->AlphaBlend;
				ModifierInfo->TwoSided		|= ColorModifier->RenderTwoSided;
				ModifierInfo->TFactorColor	= ColorModifier->Color;
			}

			// Check for an OpacityModifier modifier.
			UOpacityModifier* OpacityModifier = Cast<UOpacityModifier>(Modifier);
			if( OpacityModifier )
			{
				ModifierInfo->ModifyOpacity = 1;
				ModifierInfo->Opacity = OpacityModifier->Opacity;
				ModifierInfo->OpacityOverrideTexModifier = OpacityModifier->bOverrideTexModifier;			
			}

			// Move to the next modifier in the chain
			Modifier = Cast<UModifier>(CHECKFALLBACK(Modifier->Material));
		}
	}

	return Material;
}


#endif