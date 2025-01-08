/*=============================================================================
	UnRender.h: Rendering functions and structures
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#ifndef _UNRENDER_H_
#define _UNRENDER_H_

//
//  Defines.
//
#define ORTHO_LOW_DETAIL 40000.0f
#define DEPTH_COMPLEXITY_BITS(Viewport)	((Viewport)->IsDepthComplexity() ? 4 : 0)
#define DEPTH_COMPLEXITY_MASK(Viewport)	((Viewport)->IsDepthComplexity() ? 0x0f : 0)

#define MAX_RECURSION_DEPTH		4

//
//  FSceneNode
//
class ENGINE_API FSceneNode
{
public:

	UViewport*		Viewport;
	FRenderTarget*	RenderTarget;
	FSceneNode*		Parent;
	INT				Recursion;

	FMatrix			WorldToCamera,
					CameraToWorld,
					CameraToScreen,
					ScreenToCamera,
					WorldToScreen,
					ScreenToWorld;
	FVector			ViewOrigin,
					CameraX,
					CameraY;

	FLOAT			Determinant;		// WorldToScreen.Determinant();

	TArray<APawn*>	PawnList;			// For pawn specific rendering (ie: PlayerID)
										// Caveat/optimization: only pawns with skeletal meshes 

	// Constructor/destructor
	FSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget);
	FSceneNode(FSceneNode* InParent);
	virtual ~FSceneNode();

	// Project - Projects a point from the scene node's coordinate system into screen space.
	FPlane Project(FVector V);

	// Deproject - Deprojects a point from screen space into the scene node's coordinate system.
	FVector Deproject(FPlane P);

	// GetLodSceneNode - Determines the scenenode to use for determining LOD.

	virtual FSceneNode* GetLodSceneNode() { return this; }

	// Render
	virtual void Render(FRenderInterface* RI) = 0;

	// Subinterfaces.
	virtual class FLevelSceneNode* GetLevelSceneNode() { return NULL; }
	virtual class FCameraSceneNode* GetCameraSceneNode() { return NULL; }
	virtual class FActorSceneNode* GetActorSceneNode() { return NULL; }
	virtual class FSkySceneNode* GetSkySceneNode() { return NULL; }
	virtual class FMirrorSceneNode* GetMirrorSceneNode() { return NULL; }
	virtual class FWarpZoneSceneNode* GetWarpZoneSceneNode() { return NULL; }
};

//
//  FDynamicLight
//
class ENGINE_API FDynamicLight
{
public:

	AActor*				Actor;
	INT					Revision;

	FPlane				Color;
	FVector				Position,
						Direction;
	FLOAT				Radius;

	UBOOL				Dynamic,
						Changed;
	INT					SortKey;

    FLOAT               Alpha;

	// Constructor
	FDynamicLight(AActor* InActor = NULL);

	// Update - Calculates render data for the light.
	void Update();

	// SampleIntensity
	FLOAT SampleIntensity(FVector SamplePosition,FVector SampleNormal);

	// SampleLight
	FColor SampleLight(FVector SamplePosition,FVector SampleNormal);
};

//
//  FDynamicActor
//
class ENGINE_API FDynamicActor
{
public:

	AActor*	Actor;
	INT		Revision;

	FMatrix	LocalToWorld,
			WorldToLocal;
	FLOAT	Determinant;
	FBox	BoundingBox;
	FSphere	BoundingSphere;
	FColor	AmbientColor;
	UBOOL	Translucent;
	DWORD	VisibilityTag;
    FBox	PredictedBox;
    FVector AmbientVector;

	// Constructor
	FDynamicActor(AActor* InActor);

	// Update - Calculates render data for the actor.
	void Update();

	// Render
	void Render(FLevelSceneNode* SceneNode,TList<FDynamicLight*>* Lights,TList<FProjectorRenderInfo*>* Projectors,FRenderInterface* RI);
};

//
//  FLevelSceneNode
//
class ENGINE_API FLevelSceneNode : public FSceneNode
{
public:

	ULevel*		Level;
	UModel*		Model;
	AActor*		ViewActor;
	INT			ViewZone,
				InvisibleZone;
	DWORD		StencilMask;

	// Constructors/destructor.
	FLevelSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget);
	FLevelSceneNode(FLevelSceneNode* InParent,INT InViewZone,FMatrix LocalToParent);
	virtual ~FLevelSceneNode();

	// GetViewFrustum
	virtual FConvexVolume GetViewFrustum();

	// FilterActor
	virtual UBOOL FilterActor(AActor* Actor);
    virtual UBOOL FilterAttachment(AActor* AttachedActor) { return 1; }
	virtual UBOOL FilterProjector(AProjector* Actor);

	// FSceneNode interface.
	virtual void Render(FRenderInterface* RI);

	virtual FLevelSceneNode* GetLevelSceneNode() { return this; }
};

//
//  FCameraSceneNode
//
class ENGINE_API FCameraSceneNode : public FLevelSceneNode
{
public:

	// Constructor/destructor.
	FCameraSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget,AActor* CameraActor,FVector CameraLocation,FRotator CameraRotation,FLOAT CameraFOV);
	virtual ~FCameraSceneNode();

	// FSceneNode interface.
	virtual void Render(FRenderInterface* RI);

	virtual FCameraSceneNode* GetCameraSceneNode() { return this; }

	// Allows changing parameters on the fly.
	virtual void UpdateMatrices();

	FRotator	ViewRotation;
	FLOAT		ViewFOV;
};

//
//	FPlayerSceneNode
//
class ENGINE_API FPlayerSceneNode : public FCameraSceneNode
{
public:

	// Constructor.
	FPlayerSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget,AActor* CameraActor,FVector CameraLocation,FRotator CameraRotation,FLOAT CameraFOV);

	// FSceneNode interface.
	virtual void Render(FRenderInterface* RI);
};

//
//  FActorSceneNode
//
class ENGINE_API FActorSceneNode : public FCameraSceneNode
{
public:

	AActor*	RenderActor;

	// Constructor.
	FActorSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget,AActor* InActor,AActor* CameraActor,FVector CameraLocation,FRotator CameraRotation,FLOAT CameraFOV);

	// FSceneNode interface.
	virtual void Render(FRenderInterface* RI);

	virtual FActorSceneNode* GetActorSceneNode() { return this; }
};

//
//  FSkySceneNode
//
class ENGINE_API FSkySceneNode : public FLevelSceneNode
{
public:

	// Constructor.
	FSkySceneNode(FLevelSceneNode* InParent,INT InViewZone);

	// FSceneNode interface.
	virtual FSkySceneNode* GetSkySceneNode() { return this; }
};

//
//  FMirrorSceneNode
//
class ENGINE_API FMirrorSceneNode : public FLevelSceneNode
{
public:

	INT	MirrorSurface;

	// Constructor.
	FMirrorSceneNode(FLevelSceneNode* InParent,FPlane MirrorPlane,INT InViewZone,INT InMirrorSurface);

	// FSceneNode interface.
	virtual FMirrorSceneNode* GetMirrorSceneNode() { return this; }
};

//
//  FWarpZoneSceneNode
//
class ENGINE_API FWarpZoneSceneNode : public FLevelSceneNode
{
public:

	// Constructor.
	FWarpZoneSceneNode(FLevelSceneNode* InParent,AWarpZoneInfo* WarpZone);

	// FSceneNode interface.
	virtual FWarpZoneSceneNode* GetWarpZoneSceneNode() { return this; }
};

//
//  FVisibilityInterface
//
class FVisibilityInterface
{
public:

	// Destructor.
	virtual ~FVisibilityInterface() {}

	// Visible - Returns whether a bounding box or sphere is visible.
	virtual UBOOL Visible(FBox BoundingBox) = 0;
	virtual UBOOL Visible(FSphere BoundingSphere) = 0;
};


//
//	FSpriteVertexStream
//
class FSpriteVertexStream : public FVertexStream
{
public:

	//
	//	FSpriteVertex
	//
	struct FSpriteVertex
	{
		FVector	Position;
		FColor	Diffuse;
		FLOAT	U,
				V;
	};

	FSpriteVertex			Vertices[4];
	QWORD					CacheId;

	// Constructor.
	FSpriteVertexStream()
	{
		CacheId = MakeCacheID(CID_RenderVertices);
	}

	// GetCacheId
	virtual QWORD GetCacheId()
	{
		return CacheId;
	}

	// GetRevision
	virtual INT GetRevision()
	{
		return 1;
	}

	// GetSize
	virtual INT GetSize()
	{
		return 4 * sizeof(FSpriteVertex);
	}

	// GetStride
	virtual INT GetStride()
	{
		return sizeof(FSpriteVertex);
	}

	// GetComponents
	virtual INT GetComponents(FVertexComponent* OutComponents)
	{
		OutComponents[0].Type = CT_Float3;
		OutComponents[0].Function = FVF_Position;
		OutComponents[1].Type = CT_Color;
		OutComponents[1].Function = FVF_Diffuse;
		OutComponents[2].Type = CT_Float2;
		OutComponents[2].Function = FVF_TexCoord0;

		return 3;
	}

	// GetStreamData
	virtual void GetStreamData(void* Dest)
	{
		appMemcpy(Dest,&Vertices[0],4 * sizeof(FSpriteVertex));
	}

	// GetRawStreamData
	virtual void GetRawStreamData(void ** Dest, INT FirstVertex )
	{
		*Dest = &Vertices[FirstVertex];
	}
};

//
//	FLitSpriteVertexStream
//
class FLitSpriteVertexStream : public FVertexStream
{
public:

	//	FSpriteVertex
	struct FLitSpriteVertex
	{
		FVector	Position;
		FVector Normal;
		FColor	Diffuse;
		FLOAT	U,
				V;
	};

	FLitSpriteVertex		Vertices[4];
	QWORD					CacheId;

	// Constructor.
	FLitSpriteVertexStream()
	{
		CacheId = MakeCacheID(CID_RenderVertices);
	}

	// GetCacheId
	virtual QWORD GetCacheId()
	{
		return CacheId;
	}

	// GetRevision
	virtual INT GetRevision()
	{
		return 1;
	}

	// GetSize
	virtual INT GetSize()
	{
		return 4 * sizeof(FLitSpriteVertex);
	}

	// GetStride
	virtual INT GetStride()
	{
		return sizeof(FLitSpriteVertex);
	}

	// GetComponents
	virtual INT GetComponents(FVertexComponent* OutComponents)
	{
		OutComponents[0].Type = CT_Float3;
		OutComponents[0].Function = FVF_Position;
		OutComponents[1].Type = CT_Float3;
		OutComponents[1].Function = FVF_Normal;
		OutComponents[2].Type = CT_Color;
		OutComponents[2].Function = FVF_Diffuse;
		OutComponents[3].Type = CT_Float2;
		OutComponents[3].Function = FVF_TexCoord0;
		return 4;
	}

	// GetStreamData
	virtual void GetStreamData(void* Dest)
	{
		appMemcpy(Dest,&Vertices[0],4 * sizeof(FLitSpriteVertex));
	}

	// GetRawStreamData
	virtual void GetRawStreamData(void ** Dest, INT FirstVertex )
	{
		*Dest = &Vertices[FirstVertex];
	}
};


//
//	FCoronaRender
//
#define MAX_CORONA_LIGHTS 64
struct FCoronaRender
{
	struct FCoronaInfo
	{
		AActor* Actor;
		INT		iActor;
		FLOAT   Bright;
		DOUBLE	LastTrace;
		UBOOL	LastOccluded;

		FCoronaInfo()
		: Actor(NULL)
		, Bright(0)
		, iActor(0)
		, LastTrace(0)
		, LastOccluded(0)
		{}

		FCoronaInfo( AActor* InActor, FLOAT InBright, INT IniActor, DOUBLE InLastTrace, UBOOL InLastOccluded )
		: Actor(InActor)
		, Bright(InBright)
		, iActor(IniActor)
		, LastTrace(InLastTrace)
		, LastOccluded(InLastOccluded)
		{}
	};

	FCoronaInfo	CoronaLights[MAX_CORONA_LIGHTS];
	AActor*		VisibleLights[MAX_CORONA_LIGHTS];
	
	DOUBLE	LastTime;
	FLOAT	Delta;
	INT		iFree;
	INT		iVisible;

	FCoronaRender();

	virtual UBOOL IsOccluded( FLevelSceneNode* SceneNode, AActor* Light );
	virtual void AddVisibleLight( AActor* Light );
	virtual void AddCoronaLight( FLevelSceneNode* SceneNode, AActor* Light, FLOAT Delta );
	virtual void RenderCoronas( FLevelSceneNode* SceneNode, FRenderInterface* RI );
    void Reset();
};


/*-----------------------------------------------------------------------------
	Hit proxies.
-----------------------------------------------------------------------------*/

// Hit a Bsp surface.
struct ENGINE_API HBspSurf : public HHitProxy
{
	DECLARE_HIT_PROXY(HBspSurf,HHitProxy)
	INT iSurf;
	HBspSurf( INT iInSurf ) : iSurf( iInSurf ) {}
};

// Hit an actor.
struct ENGINE_API HActor : public HHitProxy
{
	DECLARE_HIT_PROXY(HActor,HHitProxy)
	AActor* Actor;
	HActor( AActor* InActor ) : Actor( InActor ) {}

	virtual AActor* GetActor()
	{
		return Actor;
	}
};

// Hit a brush vertex.
struct HBrushVertex : public HHitProxy
{
	DECLARE_HIT_PROXY(HBrushVertex,HHitProxy)
	ABrush* Brush;
	FVector Location;
	HBrushVertex( ABrush* InBrush, FVector InLocation ) : Brush(InBrush), Location(InLocation) {}
};

// Hit ray descriptor.
struct ENGINE_API HCoords : public HHitProxy
{
	DECLARE_HIT_PROXY(HCoords,HHitProxy)
	FVector	Origin,
			Direction;
	HCoords( FCameraSceneNode* InFrame )
	{
		FLOAT	X = (FLOAT) ((InFrame->Viewport->HitX+InFrame->Viewport->HitXL/2) - (InFrame->Viewport->SizeX / 2)) / (InFrame->Viewport->SizeX / 2.0f),
				Y = (FLOAT) ((InFrame->Viewport->HitY+InFrame->Viewport->HitYL/2) - (InFrame->Viewport->SizeY / 2)) / -(InFrame->Viewport->SizeY / 2.0f);

		Origin = InFrame->ViewOrigin;
		Direction = InFrame->Deproject(FPlane(X,Y,0.0f,InFrame->Viewport->IsOrtho() ? 1.0f : NEAR_CLIPPING_PLANE)) - InFrame->ViewOrigin;
	}
};

// Hit terrain.
struct ENGINE_API HTerrain : public HHitProxy
{
	DECLARE_HIT_PROXY(HTerrain,HHitProxy)
	ATerrainInfo* TerrainInfo;
	HTerrain( ATerrainInfo* InTerrainInfo ) : TerrainInfo( InTerrainInfo ) {}

	virtual AActor* GetActor()
	{
		return (AActor*)TerrainInfo;
	}
};

struct ENGINE_API HTerrainToolLayer : public HHitProxy
{
	DECLARE_HIT_PROXY(HTerrainToolLayer,HHitProxy)
	ATerrainInfo* TerrainInfo;
	UTexture* AlphaMap;			// The texture that will be painted on
	INT LayerNum;
	HTerrainToolLayer( ATerrainInfo* InTerrainInfo, INT InLayerNum, UTexture* InAlphaMap ) : TerrainInfo( InTerrainInfo ), LayerNum( InLayerNum ), AlphaMap(InAlphaMap) {}
};

struct ENGINE_API HMatineeTimePath : public HHitProxy
{
	ASceneManager* SceneManager;
	DECLARE_HIT_PROXY(HMatineeTimePath,HHitProxy)
	HMatineeTimePath( ASceneManager* InSceneManager ) : SceneManager(InSceneManager) {}
};

struct ENGINE_API HMatineeScene : public HHitProxy
{
	ASceneManager* SceneManager;
	DECLARE_HIT_PROXY(HMatineeScene,HHitProxy)
	HMatineeScene( ASceneManager* InSceneManager ) : SceneManager(InSceneManager) {}
};

struct ENGINE_API HMatineeAction : public HHitProxy
{
	ASceneManager* SM;
	UMatAction* MatAction;
	DECLARE_HIT_PROXY(HMatineeAction,HHitProxy)
	HMatineeAction( ASceneManager* InSM, UMatAction* InMatAction ) : SM(InSM), MatAction(InMatAction) {}
};

struct ENGINE_API HMatineeSubAction : public HHitProxy
{
	UMatAction* MatAction;
	UMatSubAction* MatSubAction;
	DECLARE_HIT_PROXY(HMatineeSubAction,HHitProxy)
	HMatineeSubAction( UMatSubAction* InMatSubAction, UMatAction* InMatAction ) : MatSubAction(InMatSubAction), MatAction(InMatAction) {}
};

struct ENGINE_API HMaterialTree : public HHitProxy
{
	UMaterial* Material;
	DWORD hWnd;				// The HWND of the texture properties dialog
	DECLARE_HIT_PROXY(HMaterialTree,HHitProxy);
	HMaterialTree( UMaterial* InMaterial, DWORD InHwnd ) : Material(InMaterial), hWnd(InHwnd) {}
};

#endif

/*------------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------------*/

