/*=============================================================================
	UnProjector.h: Projected textures & Decals
	Copyright 2000 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#define DECALREMOVETIME 1.f

class AProjector;

enum EProjectorFlags
{
	PRF_Projected		= 0x01,
	PRF_ProjectOnUnlit	= 0x02,
	PRF_Gradient		= 0x04,
	PRF_ProjectOnAlpha  = 0x08,
	PRF_ProjectOnBackfaces	= 0x10
};

struct FProjectorRenderInfo
{
	AProjector*			Projector;			// May not be valid.
	INT					ReferenceCount;		// Number of surfaces, actors etc referencing this data structure
	DOUBLE				LastRenderTime;		// Time of last render
	DOUBLE				Expires;			// Time the projector can be removed, 0 = never
	UMaterial*			Material;
	UMaterial*			Highlight;
	UTexture*			GradientTexture;

	AActor*				UseTarget;

	BYTE				MaterialBlendingOp,
						FrameBufferBlendingOp;
	FMatrix				Matrix,
						InverseMatrix;
	FVector				BoundingBoxCenter,
						BoundingBoxExtent;
	FPlane				FrustumPlanes[6];
	FMatrix				GradientMatrix;
	DWORD				ProjectorFlags;

	FProjectorRenderInfo( AProjector* InProjector, FLOAT InExpires = 0 );
	~FProjectorRenderInfo()
	{
		check(ReferenceCount==0);
	}
	inline FProjectorRenderInfo* AddReference()
	{
		ReferenceCount++;
		return this;
	}
	inline void RemoveReference()
	{
		if( --ReferenceCount == 0 )
			delete this;
	}
	inline UBOOL Render( DOUBLE Now )
	{
        if( 
			Expires!=0 
		&&  (		UTexture::__Client->Decals==0 
				||	UTexture::__Client->Projectors==0 
				||	UTexture::__Client->Engine->GRenDev->SupportsZBIAS==0
			) 
		)
        {
            RemoveReference();
			return 0;
        }

		// See if this decal has expired.  If so, remove it.
		if( Expires!=0 && (Expires<=Now || (LastRenderTime!=0.0f && (Now-LastRenderTime>DECALREMOVETIME))) )
		{
			RemoveReference();
			return 0;
		}

		LastRenderTime = Now;
		return 1;
	}
	UMaterial* GetMaterial(class FSceneNode* SceneNode,AActor* Owner,UMaterial* BaseMaterial);
	
	ENGINE_API friend FArchive& operator<<( FArchive& Ar, FProjectorRenderInfo* Info )
	{
		return Ar << Info->Material << Info->GradientTexture;
	}
};

struct ENGINE_API FStaticProjectorVertex
{
	FVector	WorldPosition;
	FLOAT	Attenuation;
};

struct ENGINE_API FStaticProjectorUV
{
	FLOAT	U,
			V;
};

struct ENGINE_API FStaticProjectorInfo
{
	FProjectorRenderInfo*			RenderInfo;
	UMaterial*						BaseMaterial;
	TArray<FStaticProjectorVertex>	Vertices;
	TArray<FStaticProjectorUV>		BaseUVs;	// Only if RenderInfo->BaseMaterialBlendingOp != PB_None or base material has opacity.
	TArray<_WORD>					Indices;
	UBOOL							TwoSided;

	ENGINE_API friend FArchive& operator<<(FArchive& Ar,FStaticProjectorInfo* ProjectorInfo)
	{
		return Ar << ProjectorInfo->RenderInfo << ProjectorInfo->BaseMaterial;
	}
};

class ENGINE_API UProjectorPrimitive : public UPrimitive
{
	DECLARE_CLASS(UProjectorPrimitive,UPrimitive,0,Engine);

	// UPrimitive interface
	virtual UBOOL UseCylinderCollision( const AActor* Owner );
	virtual FBox GetCollisionBoundingBox( const AActor* Owner ) const;
	virtual UBOOL PointCheck
	(
		FCheckResult	&Result,
		AActor			*Actor,
		FVector			Location,
		FVector			Extent,
		DWORD           ExtraNodeFlags
	);
	virtual UBOOL LineCheck
	(
		FCheckResult&	Result,
		AActor*			Owner,
		FVector			End,
		FVector			Start,
		FVector			Extent,
		DWORD           ExtraNodeFlags,
		DWORD			TraceFlags
	);
	virtual void Destroy();
	
	virtual FVector GetEncroachExtent(AActor* Owner) { return GetCollisionBoundingBox(Owner).GetExtent(); }
	virtual FVector GetEncroachCenter(AActor* Owner) { return GetCollisionBoundingBox(Owner).GetCenter(); }
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

