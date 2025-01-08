/*=============================================================================
	UnRender.cpp: High-level rendering code.
	Copyright 2001-2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRenderPrivate.h"

//
//	FSceneNode::FSceneNode
//
FSceneNode::FSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget)
{
	Parent = NULL;
	Viewport = InViewport;
	RenderTarget = InRenderTarget;
	Recursion = 0;
}

//
//	FSceneNode::FSceneNode
//
FSceneNode::FSceneNode(FSceneNode* InParent)
{
	Parent = InParent;
	Viewport = Parent->Viewport;
	RenderTarget = Parent->RenderTarget;
	Recursion = Parent->Recursion + 1;

	WorldToCamera = InParent->WorldToCamera;
	CameraToScreen = InParent->CameraToScreen;
	ScreenToCamera = InParent->ScreenToCamera;
	CameraToWorld = InParent->CameraToWorld;

	WorldToScreen = InParent->WorldToScreen;
	ScreenToWorld = InParent->ScreenToWorld;

	ViewOrigin = Parent->ViewOrigin;
	CameraX = InParent->CameraX;
	CameraY = InParent->CameraY;

	Determinant = WorldToScreen.Determinant();
}

//
//	FSceneNode::~FSceneNode
//
FSceneNode::~FSceneNode()
{
}

//
//	FSceneNode::Project
//
FPlane FSceneNode::Project(FVector V)
{
	FPlane	Result = WorldToScreen.TransformFPlane(FPlane(V,1));
	FLOAT	RHW = 1.0f / Result.W;

	return FPlane(Result.X * RHW,Result.Y * RHW,Result.Z * RHW,Result.W);
}

//
//	FSceneNode::Deproject
//
FVector FSceneNode::Deproject(FPlane P)
{
	return ScreenToWorld.TransformFPlane(FPlane(P.X * P.W,P.Y * P.W,P.Z * P.W,P.W));
}


//
//	FCoronaRender
//
FCoronaRender::FCoronaRender()
{
    Reset();
}

void FCoronaRender::Reset()
{
	Delta		= 0.f;
	LastTime	= 0;
	iFree		= 0;
	iVisible	= 0;
	appMemzero(CoronaLights, sizeof(CoronaLights));
	appMemzero(VisibleLights, sizeof(VisibleLights));
}

void FCoronaRender::AddVisibleLight( AActor* Light )
{
	INT i;
	for( i=0; i<iVisible; i++ )
		if( VisibleLights[i] == Light )
			break;
	if( i == iVisible )
		VisibleLights[iVisible++] = Light;

	iVisible %= MAX_CORONA_LIGHTS;
}

UBOOL FCoronaRender::IsOccluded( FLevelSceneNode* SceneNode, AActor* Actor )
{
	guard(FCoronaRender::IsOccluded);

	UBOOL	Occluded,
			Traced = false;

	INT i;
	for( i=0; i<MAX_CORONA_LIGHTS; i++ )
		if( CoronaLights[i].Actor == Actor )
			break;

	if( i<MAX_CORONA_LIGHTS && ((LastTime - CoronaLights[i].LastTrace) < (appFrand()*0.33f + 0.33f)) )
	{
		Occluded	= CoronaLights[i].LastOccluded;
	}
	else
	{
		FCheckResult Hit;
		Occluded = !Actor->GetLevel()->SingleLineCheck( 
						Hit, 
						SceneNode->Viewport->Actor->bBehindView ? NULL : SceneNode->ViewActor, 
						Actor->Location, 
						SceneNode->ViewOrigin, 
						TRACE_Corona | TRACE_World | TRACE_StopAtFirstHit | TRACE_Pawns, 
						FVector(0,0,0)
		);
		Traced = true;
	}
	
	if( i<MAX_CORONA_LIGHTS )
	{
		CoronaLights[i].LastOccluded = Occluded;
		if( Traced )
			CoronaLights[i].LastTrace = LastTime;
	}

	return Occluded;
	unguard;
}

void FCoronaRender::AddCoronaLight( FLevelSceneNode* SceneNode, AActor* Actor, FLOAT Delta )
{
	guard(FCoronaRender::AddCoronaLight);
	clock(GStats.DWORDStats(GEngineStats.STATS_Corona_CollisionCycles));

	if
	(	Actor
	&&	Actor->bCorona
	&&	Actor->Skins.Num()
	&&	Cast<UTexture>(Actor->Skins(0))
	&& !Actor->bDeleteMe
	&& !IsOccluded( SceneNode, Actor )
	)
	{		
		INT i;
		for( i=0; i<MAX_CORONA_LIGHTS; i++ )
			if( CoronaLights[i].Actor == Actor )
				break;
		if( i<MAX_CORONA_LIGHTS )
		{
			CoronaLights[i].Bright = Min(1.f,CoronaLights[i].Bright+2.f*Delta*CoronaLights[i].Actor->ScaleGlow ); 
		}
		else
		{
			while( iFree<MAX_CORONA_LIGHTS && CoronaLights[iFree].Actor )
				iFree++;
			if( iFree<MAX_CORONA_LIGHTS )
			{
				CoronaLights[iFree].Actor	= Actor;
				CoronaLights[iFree].iActor	= Actor->GetLevel()->GetActorIndex(Actor);
				CoronaLights[iFree].Bright			= Min(1.f,2.f*Delta*CoronaLights[iFree].Actor->ScaleGlow); 
				CoronaLights[iFree].LastOccluded	= 0;
				CoronaLights[iFree].LastTrace		= LastTime;
			}
		}
	}

	unclock(GStats.DWORDStats(GEngineStats.STATS_Corona_CollisionCycles));
	unguard;
}

void FCoronaRender::RenderCoronas( FLevelSceneNode* SceneNode, FRenderInterface* RI )
{
	guard(FCoronaRender::RenderCoronas);

	FSpriteVertexStream CoronaVertices;

	// Backward compatibility mojo.
	Delta	 = Min<FLOAT>( 1.0f, 3.0f * (SceneNode->Viewport->CurrentTime - LastTime) );
	LastTime = SceneNode->Viewport->CurrentTime;

	// Fade out existing coronas, and remove if they've faded out completely.
	for( INT i=0; i<MAX_CORONA_LIGHTS; i++ )
		if( CoronaLights[i].Actor && (CoronaLights[i].Bright -= Delta * CoronaLights[i].Actor->ScaleGlow ) < 0.f ) 
			CoronaLights[i].Actor = NULL;

	// Update coronas.
	for( INT i=0; i<MAX_CORONA_LIGHTS; i++ )
		AddCoronaLight( SceneNode, VisibleLights[i], Delta );

	// Reset arrays & indices.
	iFree = 0;

	// Coronas shouldn't be fogged.
	RI->SetDistanceFog( 0, 0, 0, 0 );

	// Set identity matrix.
	RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);

	// Render the coronas.
	INT Count = 0;
	for( INT i=0; i<MAX_CORONA_LIGHTS; i++ )
	{
		AActor* Actor = CoronaLights[i].Actor;
		ALight* Light = Cast<ALight>(Actor);

		if(		!Actor
			||	!((CoronaLights[i].iActor <= SceneNode->Viewport->Actor->GetLevel()->Actors.Num()) && 
				(SceneNode->Viewport->Actor->GetLevel()->Actors(CoronaLights[i].iActor)==Actor))
			||	Actor->bDeleteMe
			||	!Actor->Skins.Num()
		)
		{
			CoronaLights[i].Actor = NULL;
			continue;
		}
		
		PUSH_HIT(SceneNode->Viewport,HActor(Actor));

		FVector ProjBase     = SceneNode->Deproject(FPlane(0,0,0,1));	
		FVector ProjUp		 = SceneNode->Deproject(FPlane(0,-1,0,1)) - ProjBase;
		FVector ProjRight	 = SceneNode->Deproject(FPlane(1,0,0,1)) - ProjBase;
		FVector ProjFront	 = ProjRight ^ ProjUp;

		ProjUp.Normalize();
		ProjRight.Normalize();
		ProjFront.Normalize();

		BYTE    H		= Actor->LightHue;
		FVector Hue		= (H<86) ? FVector((85-H)/85.f,(H-0)/85.f,0) : (H<171) ? FVector(0,(170-H)/85.f,(H-85)/85.f) : FVector((H-170)/85.f,0,(255-H)/84.f);
		FLOAT	Alpha	= Actor->LightSaturation / 255.0f;
		FPlane  Color	= (Hue + Alpha * (FVector(1,1,1) - Hue)) * CoronaLights[i].Bright;
		FLOAT Distance	= FDist(SceneNode->ViewOrigin,Actor->Location);
		Distance		= Clamp<FLOAT>( Distance / 1000.f, 0.01f, 10.f );

		FVector Right, Up;

		if ( (Actor->DrawScale3D.X != Actor->DrawScale3D.Y) && (Actor->DrawScale3D.Z > 0) )
		{
			// Non square coronas.
			FVector X		= FVector(1,0,0).TransformVectorBy( GMath.UnitCoords * Actor->Rotation );
			FVector Y		= FVector(0,1,0).TransformVectorBy( GMath.UnitCoords * Actor->Rotation );
			FVector Z		= FVector(0,0,1).TransformVectorBy( GMath.UnitCoords * Actor->Rotation );
			FLOAT   Angle	= appSqrt((1 - Abs(ProjFront | X)) * (1 - Abs(ProjFront | Y)));
			Color			= FColor(Color * Angle);
			Right			= X;
			Up				= Y;
		}
		else
		{
			// Square Coronas.
			Right			= ProjRight;
			Up				= ProjUp;
			
			// Rotate corona dependend on distance.
			if( Light && (Light->CoronaRotation || Light->CoronaRotationOffset) )
			{
				FLOAT Angle;
				Angle	= Light->CoronaRotation * 16384.f * Distance + Light->CoronaRotationOffset;
				Right	= Right.RotateAngleAxis( Angle, ProjFront );
				Up		= Up.RotateAngleAxis( Angle, ProjFront );
			}
		}

		FLOAT	CoronaFudgeFactor	= 512.f,
				ScaleX				= Actor->DrawScale * Actor->DrawScale3D.X * Distance * CoronaFudgeFactor,
				ScaleY				= Actor->DrawScale * Actor->DrawScale3D.Y * Distance * CoronaFudgeFactor;

		// Clamp size.
		if( Light )
		{
			ScaleX = Clamp( ScaleX, Light->MinCoronaSize, Light->MaxCoronaSize );
			ScaleY = Clamp( ScaleY, Light->MinCoronaSize, Light->MaxCoronaSize );
		}

		Right	*= ScaleX;
		Up		*= ScaleY;

		FColor RenderColor	= Color;
		RenderColor			= RenderColor.RenderColor();
		RenderColor.A		= 255;

		CoronaVertices.Vertices[0].Position	= Actor->Location - Right - Up;
		CoronaVertices.Vertices[0].U		= 0.f;
		CoronaVertices.Vertices[0].V		= 0.f;
		CoronaVertices.Vertices[0].Diffuse	= RenderColor;
			
		CoronaVertices.Vertices[1].Position	= Actor->Location + Right - Up;
		CoronaVertices.Vertices[1].U		= 1.f;
		CoronaVertices.Vertices[1].V		= 0.f;
		CoronaVertices.Vertices[1].Diffuse	= RenderColor;
			
		CoronaVertices.Vertices[2].Position	= Actor->Location + Right + Up;
		CoronaVertices.Vertices[2].U		= 1.f;
		CoronaVertices.Vertices[2].V		= 1.f;
		CoronaVertices.Vertices[2].Diffuse	= RenderColor;
			
		CoronaVertices.Vertices[3].Position	= Actor->Location - Right + Up;
		CoronaVertices.Vertices[3].U		= 0.f;
		CoronaVertices.Vertices[3].V		= 1.f;
		CoronaVertices.Vertices[3].Diffuse	= RenderColor;

		UBOOL AlphaTest = 0;
		UTexture* Texture = Cast<UTexture>(Actor->Skins(0));
		if( Texture && Texture->bAlphaTexture )
			AlphaTest = 1;

		DECLARE_STATIC_UOBJECT( UFinalBlend, FinalBlend, {} );
		FinalBlend->Material			= Actor->Skins(0);
		FinalBlend->TwoSided			= 1;
		FinalBlend->FrameBufferBlending = FB_Brighten;
		FinalBlend->ZWrite				= 0;
		FinalBlend->ZTest				= 0;
		FinalBlend->AlphaTest			= AlphaTest;
		FinalBlend->AlphaRef			= 0;

		RI->EnableLighting(0,1);
		RI->SetMaterial( Light && Light->UseOwnFinalBlend ? Actor->Skins(0) : FinalBlend );

		// Had to move this because SetMaterial now sets up the vertex shader, which SetDynamicStreams tries to do
		INT BaseVertexIndex = RI->SetDynamicStream(VS_FixedFunction,&CoronaVertices);

		RI->SetIndexBuffer(NULL,0);
		RI->DrawPrimitive(PT_TriangleFan,BaseVertexIndex,2);

		POP_HIT( SceneNode->Viewport );

		Count++;
	}

	//!!SceneNode->Viewport->RenderStats.IntStat(TEXT("Coronas"),TEXT("rendered")) = Count;
	unguard;
}


//
//	DrawSprite
//
static void DrawSprite(AActor* Actor,FLevelSceneNode* SceneNode,FRenderInterface* RI)
{
	guard(DrawSprite);

	// Calculate the sprite right and down vectors.

	FVector	SpriteX = SceneNode->CameraX.SafeNormal() * Actor->DrawScale * Actor->DrawScale3D.X * Actor->Texture->MaterialUSize() / 2.0f,
			SpriteY = -SceneNode->CameraY.SafeNormal() * Actor->DrawScale * Actor->DrawScale3D.Y * Actor->Texture->MaterialUSize() / 2.0f;

	// Calculate the display color.

	FPlane	ColorPlane = FPlane(1.0f,1.0f,1.0f,1.0f);

	if(GIsEditor)
	{
		if(SceneNode->Viewport->Actor->ShowFlags & SHOW_SelectionHighlight && Actor->bSelected)
			ColorPlane = FPlane(0.5f,0.9f,0.5f,1.0f);
		else if(Actor->LightType != LT_None)
			ColorPlane = FGetHSV(Actor->LightHue,Actor->LightSaturation,255);
	}

	// Setup the sprite vertices.
	FSpriteVertexStream	SpriteVertices;
	FColor	Color = FColor(ColorPlane);
	FVector	SpriteBase = Actor->Location;

	UMaterial* Material = Actor->Texture;
	UTexture* Texture = Cast<UTexture>(Material);
	if( Texture )
	{
		DECLARE_STATIC_UOBJECT( UFinalBlend, FinalBlend, {} );
		FinalBlend->Material = Texture;
		FinalBlend->TwoSided = 1;

		if( Texture->bMasked || Actor->Style==STY_Masked )
		{
			FinalBlend->FrameBufferBlending = FB_Overwrite;
			FinalBlend->ZWrite				= 1;
			FinalBlend->ZTest				= 1;
			FinalBlend->AlphaTest			= 1;
			FinalBlend->AlphaRef			= 127;
		}
		else
		if( Texture->bAlphaTexture )
		{
			FinalBlend->FrameBufferBlending = FB_AlphaBlend;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= 1;
			FinalBlend->AlphaTest			= 1;
			FinalBlend->AlphaRef			= 0;
		}
		else
		if( Actor->Style==STY_Translucent )
		{
			FinalBlend->FrameBufferBlending = FB_Translucent;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= 1;
			FinalBlend->AlphaTest			= 0;
		}
		else
		if( Actor->Style==STY_Modulated )
		{
			FinalBlend->FrameBufferBlending = FB_Modulate;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= 1;
			FinalBlend->AlphaTest			= 0;
		}
		else
		if( Actor->Style==STY_Additive )
		{
			FinalBlend->FrameBufferBlending = FB_Brighten;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= 1;
			FinalBlend->AlphaTest			= 0;
		}
		else
		if( Actor->Style==STY_Subtractive )
		{
			FinalBlend->FrameBufferBlending = FB_Darken;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= 1;
			FinalBlend->AlphaTest			= 0;
		}

		Material = FinalBlend;
	}

	SpriteVertices.Vertices[0].Position = SpriteBase - SpriteX - SpriteY;
	SpriteVertices.Vertices[0].Diffuse = Color;
	SpriteVertices.Vertices[0].U = 0.0f;
	SpriteVertices.Vertices[0].V = 0.0f;

	SpriteVertices.Vertices[1].Position = SpriteBase + SpriteX - SpriteY;
	SpriteVertices.Vertices[1].Diffuse = Color;
	SpriteVertices.Vertices[1].U = 1.0f;
	SpriteVertices.Vertices[1].V = 0.0f;

	SpriteVertices.Vertices[2].Position = SpriteBase + SpriteX + SpriteY;
	SpriteVertices.Vertices[2].Diffuse = Color;
	SpriteVertices.Vertices[2].U = 1.0f;
	SpriteVertices.Vertices[2].V = 1.0f;

	SpriteVertices.Vertices[3].Position = SpriteBase - SpriteX + SpriteY;
	SpriteVertices.Vertices[3].Diffuse = Color;
	SpriteVertices.Vertices[3].U = 0.0f;
	SpriteVertices.Vertices[3].V = 1.0f;

	// Draw the sprite.

	INT BaseVertexIndex = RI->SetDynamicStream(VS_FixedFunction,&SpriteVertices);

	RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);
	//!!MAT
	//RI->SetBlending(PolyFlags | PF_TwoSided | Texture->PolyFlags);
	//RI->SetTexture(0,Texture->GetRenderInterface());
	RI->EnableLighting(0,1);
	RI->SetMaterial(Material);
	RI->SetIndexBuffer(NULL,0);

	RI->DrawPrimitive(PT_TriangleFan,BaseVertexIndex,2);

	unguard;
}

//
//	RequiresSorting
//

UBOOL RequiresSorting(AActor* Actor)
{
	guard(RequiresSorting);

	if(Actor->bDisableSorting)
		return 0;

	if(Actor->DrawType == DT_StaticMesh && Actor->StaticMesh)
	{
		for(INT MaterialIndex = 0;MaterialIndex < Actor->StaticMesh->Materials.Num();MaterialIndex++)
		{
			UMaterial*	Material = Actor->StaticMesh->GetSkin(Actor,MaterialIndex);

			if(Material && Material->RequiresSorting())
				return 1;
		}
	}
	else if(Actor->DrawType == DT_Mesh && Cast<ULodMesh>(Actor->Mesh))
	{
		ULodMesh*	LodMesh = Cast<ULodMesh>(Actor->Mesh);

		for(INT MaterialIndex = 0;MaterialIndex < LodMesh->Materials.Num();MaterialIndex++)
		{
			if(LodMesh->Materials(MaterialIndex) && LodMesh->Materials(MaterialIndex)->RequiresSorting())
				return 1;
		}
	}
	else if( Actor->DrawType == DT_Custom && Actor->Style == STY_Masked )
		return 1;
	else if( Actor->DrawType == DT_Custom && Actor->Style != STY_Normal )
		return -1;
	else if(Actor->DrawType == DT_Brush)
		return 1;
	else if(Actor->DrawType == DT_AntiPortal)
		return 1;
	else if(Actor->DrawType == DT_Sprite)
		return 1;

	if(Actor->DrawType == DT_Mesh || Actor->DrawType == DT_FluidSurface)
	{
		for(INT SkinIndex = 0;SkinIndex < Actor->Skins.Num();SkinIndex++)
		{
			if(Actor->Skins(SkinIndex) && Actor->Skins(SkinIndex)->RequiresSorting())
				return 1;
		}
	}

	for(INT AttachmentIndex = 0;AttachmentIndex < Actor->Attached.Num();AttachmentIndex++)
		if(Actor->Attached(AttachmentIndex) && RequiresSorting(Actor->Attached(AttachmentIndex)))
			return 1;

	return 0;

	unguard;
}

//
//	FDynamicActor::FDynamicActor
//

FDynamicActor::FDynamicActor(AActor* InActor)
{
	Actor = InActor;
	VisibilityTag = 0;
	Revision = INDEX_NONE;
    PredictedBox.Init();
}

//
//	FDynamicActor::Update
//
	
void FDynamicActor::Update()
{
	guard(FDynamicActor::Update);
	
	if( Actor->DrawType == DT_Mesh && Actor->Mesh )
	{
		// Set up specific mesh-to-world matrix. 
		LocalToWorld = Actor->Mesh->MeshGetInstance(Actor)->MeshToWorld();		
	}
	else
	{
		LocalToWorld = Actor->LocalToWorld();
	}

	WorldToLocal = LocalToWorld.Inverse();
	Determinant = LocalToWorld.Determinant();

	if(Actor->GetPrimitive())
	{
		BoundingBox = Actor->GetPrimitive()->GetRenderBoundingBox(Actor).TransformBy(LocalToWorld);
		BoundingSphere = Actor->GetPrimitive()->GetRenderBoundingSphere(Actor).TransformBy(LocalToWorld);
	}
	else
	{
		BoundingBox = FBox(0);
		BoundingBox += Actor->Location;
	}

	AmbientColor = FColor(0,0,0);
	Translucent = RequiresSorting(Actor);

	Revision = Actor->RenderRevision;

	unguard;
}

//
//	FDynamicActor::Render
//
void FDynamicActor::Render(FLevelSceneNode* SceneNode,TList<FDynamicLight*>* Lights,TList<FProjectorRenderInfo*>* Projectors,FRenderInterface* RI)
{
	guard(FDynamicActor::Render);

	PUSH_HIT(SceneNode->Viewport,HActor(Actor));

	if(SceneNode->Viewport->bShowBounds)
	{
		// Render bounding box.

		RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);
		FLineBatcher(RI).DrawBox(BoundingBox,FColor(255,0,0));
        FLineBatcher(RI).DrawBox(PredictedBox,FColor(0,255,0)); 
	}

	if( Actor->IsAPawn() )
		SceneNode->PawnList.AddItem( (APawn*)Actor );

	Actor->LastRenderTime = Actor->Level->TimeSeconds;
	if(Actor->DrawType == DT_StaticMesh && Actor->StaticMesh)
		RenderStaticMesh(this,SceneNode,Lights,Projectors,RI);
	else if(Actor->DrawType == DT_Mesh && Actor->Mesh)
	{
		// Draw mesh.
		Actor->Mesh->MeshGetInstance(Actor)->Render(this,SceneNode,Lights,Projectors,RI);			

		// Draw Skeletal-mesh specific attachments. - universal recursive support.
		USkeletalMesh *SkelMesh = Cast<USkeletalMesh>(Actor->Mesh);
		if ( SkelMesh )
		{
			for(INT i=0; i< Actor->Attached.Num();i++)
			{
				AActor* AttachedActor = Actor->Attached(i);
				if ( AttachedActor && !AttachedActor->bDeleteMe && AttachedActor->AttachmentBone != NAME_None )
				{
					// Temporarily set the bone to NAME_None to draw this attachment at its attached location/rotation.
					FName StoreBone = AttachedActor->AttachmentBone;
					AttachedActor->AttachmentBone = NAME_None;
					UBOOL bShouldDrawAttachment = SceneNode->FilterActor(AttachedActor); 
					AttachedActor->AttachmentBone = StoreBone;

			        if( bShouldDrawAttachment )
			            bShouldDrawAttachment = SceneNode->FilterAttachment(AttachedActor );

					if ( bShouldDrawAttachment && SkelMesh->SetAttachmentLocation(Actor, AttachedActor) )
					{
						AttachedActor->ClearRenderData(); 
				        AttachedActor->GetActorRenderData()->Render(SceneNode,Lights,Projectors,RI);
					}
                }
            }
        }
	}
	else if(Actor->DrawType == DT_Brush && Actor->IsA(ABrush::StaticClass()) && Actor->Brush)
		Actor->Brush->Render(this,SceneNode,RI);
	else if(Actor->DrawType == DT_Sprite && Actor->Texture && SceneNode->Viewport->Actor->RendMap!=REN_MatineePreview)
		DrawSprite(Actor,SceneNode,RI);
	else if(Actor->DrawType == DT_FluidSurface && Actor->IsA(AFluidSurfaceInfo::StaticClass()) )
		Cast<AFluidSurfaceInfo>(Actor)->Render(this, SceneNode, Lights, RI);
	else if(Actor->DrawType == DT_Custom)
		Actor->Render(this,SceneNode,Lights,RI);

	// AntiPortal
	if( (Actor->DrawType == DT_AntiPortal) && Actor->AntiPortal 
		&& ((GIsEditor && !(SceneNode->Viewport->Actor->ShowFlags & SHOW_PlayerCtrl))
		|| (!GIsEditor && (SceneNode->Viewport->Actor->ShowFlags & SHOW_Volumes))))
	{
		FLineBatcher	LineBatcher(RI,0);
		FColor			LineColor = Actor->bSelected ? FColor(255,128,0) : FColor(128,64,0);

		RI->SetTransform(TT_LocalToWorld,LocalToWorld);

		for(INT FaceIndex = 0;FaceIndex < Actor->AntiPortal->Faces.Num();FaceIndex++)
		{
			FConvexVolumeFace*	Face = &Actor->AntiPortal->Faces(FaceIndex);

			for(INT VertexIndex = 0;VertexIndex < Face->Vertices.Num();VertexIndex++)
				LineBatcher.DrawLine(Face->Vertices(VertexIndex),Face->Vertices((VertexIndex + 1) % Face->Vertices.Num()),LineColor);
		}
	}

	if(GIsEditor )
		Actor->RenderEditorInfo(SceneNode,RI,this);

	POP_HIT(SceneNode->Viewport);

	if(SceneNode->Viewport->bShowCollision &&
	   (Actor->bCollideActors || Actor->bCollideWorld || Actor->bUseCylinderCollision) &&
	   (Actor->DrawType == DT_Sprite ||
	   Actor->DrawType == DT_StaticMesh ||
	   Actor->DrawType == DT_Mesh) &&
	   (Actor->Physics != PHYS_Karma &&
	   Actor->Physics != PHYS_KarmaRagDoll)
	   )	
	{
		FLineBatcher LineBatcher(RI);
		RI->SetTransform(TT_LocalToWorld, FMatrix::Identity);
		LineBatcher.DrawCylinder(RI, Actor->Location, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), FColor(64,128,128), Actor->CollisionRadius, Actor->CollisionHeight, 20);
		LineBatcher.Flush();
	}

	unguard;
}

void AFluidSurfaceInfo::RenderEditorInfo(FLevelSceneNode* SceneNode,FRenderInterface* RI, FDynamicActor* FDA)
{
	guard(AFluidSurfaceInfo::RenderEditorInfo);

	Super::RenderEditorInfo(SceneNode,RI,FDA);
	if( Texture )
		DrawSprite(this,SceneNode,RI);
	unguard;
}

//
//	FLevelSceneNode::FLevelSceneNode
//
FLevelSceneNode::FLevelSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget) : FSceneNode(InViewport,InRenderTarget)
{
	Level = Viewport->Actor->XLevel;
	Model = Level->Model;
	ViewZone = INDEX_NONE;
	InvisibleZone = INDEX_NONE;
	ViewActor = NULL;

	StencilMask = 0;

	// If necessary, build model render data.
	if(!Model->Sections.Num())
		Model->BuildRenderData();

	if(!Level->BuiltRenderData)
		Level->BuildRenderData();
}

//
//	FLevelSceneNode::FLevelSceneNode
//
FLevelSceneNode::FLevelSceneNode(FLevelSceneNode* InParent,INT InViewZone,FMatrix LocalToParent) : FSceneNode(InParent)
{
	Level = InParent->Level;
	Model = InParent->Model;
	ViewZone = InViewZone;
	InvisibleZone = INDEX_NONE;
	ViewActor = NULL;

	StencilMask = Viewport->NextStencilMask;
	Viewport->NextStencilMask <<= 1;

	FMatrix	ParentToLocal = LocalToParent.Inverse();

	WorldToCamera = LocalToParent * Parent->WorldToCamera;
	CameraToWorld = Parent->CameraToWorld * ParentToLocal;

	WorldToScreen = WorldToCamera * CameraToScreen;
	ScreenToWorld = ScreenToCamera * CameraToWorld;

	Determinant = WorldToScreen.Determinant();

	ViewOrigin = ParentToLocal.TransformFVector(Parent->ViewOrigin);

	CameraX = ScreenToWorld.TransformNormal(FVector(2.0f / Viewport->SizeX,0,0));
	CameraY = ScreenToWorld.TransformNormal(FVector(0,2.0f / Viewport->SizeY,0));
}

//
//	FLevelSceneNode::~FLevelSceneNode
//
FLevelSceneNode::~FLevelSceneNode()
{
}

//
//	FLevelSceneNode::GetViewFrustum
//
FConvexVolume FLevelSceneNode::GetViewFrustum()
{
	FConvexVolume	ViewFrustum;
	FLOAT			Determinant = WorldToScreen.Determinant();

	if(Viewport->IsOrtho())
	{
		FVector	ViewSides[2][2][2];
		FLOAT	TempSigns[2] = { -1.0f, 1.0f };

		for(INT X = 0;X < 2;X++)
			for(INT Y = 0;Y < 2;Y++)
				for(INT Z = 0;Z < 2;Z++)
					ViewSides[X][Y][Z] = Deproject(FPlane(TempSigns[X],TempSigns[Y],TempSigns[Z],1.0f));

		if(Determinant < 0.0f)
		{
			ViewFrustum.BoundingPlanes[0] = FPlane(ViewSides[0][0][0],ViewSides[0][0][1],ViewSides[1][0][1]);
			ViewFrustum.BoundingPlanes[1] = FPlane(ViewSides[1][0][0],ViewSides[1][0][1],ViewSides[1][1][1]);
			ViewFrustum.BoundingPlanes[2] = FPlane(ViewSides[1][1][0],ViewSides[1][1][1],ViewSides[0][1][1]);
			ViewFrustum.BoundingPlanes[3] = FPlane(ViewSides[0][1][0],ViewSides[0][1][1],ViewSides[0][0][1]);
		}
		else
		{
			ViewFrustum.BoundingPlanes[0] = FPlane(ViewSides[0][0][0],ViewSides[1][0][1],ViewSides[0][0][1]);
			ViewFrustum.BoundingPlanes[1] = FPlane(ViewSides[1][0][0],ViewSides[1][1][1],ViewSides[1][0][1]);
			ViewFrustum.BoundingPlanes[2] = FPlane(ViewSides[1][1][0],ViewSides[0][1][1],ViewSides[1][1][1]);
			ViewFrustum.BoundingPlanes[3] = FPlane(ViewSides[0][1][0],ViewSides[0][0][1],ViewSides[0][1][1]);
		}

		ViewFrustum.NumPlanes = 4;
	}
	else
	{
		FVector	ViewSides[4];
		FLOAT	TempSigns[2] = { -1.0f, 1.0f };

		for(INT X = 0;X < 2;X++)
			for(INT Y = 0;Y < 2;Y++)
				ViewSides[X * 2 + Y] = Deproject(FPlane(TempSigns[X],TempSigns[Y],0.0f,NEAR_CLIPPING_PLANE));

		if(Determinant < 0.0f)
		{
			ViewFrustum.BoundingPlanes[0] = FPlane(ViewOrigin,ViewSides[1],ViewSides[0]);
			ViewFrustum.BoundingPlanes[1] = FPlane(ViewOrigin,ViewSides[0],ViewSides[2]);
			ViewFrustum.BoundingPlanes[2] = FPlane(ViewOrigin,ViewSides[2],ViewSides[3]);
			ViewFrustum.BoundingPlanes[3] = FPlane(ViewOrigin,ViewSides[3],ViewSides[1]);
		}
		else
		{
			ViewFrustum.BoundingPlanes[0] = FPlane(ViewOrigin,ViewSides[0],ViewSides[1]);
			ViewFrustum.BoundingPlanes[1] = FPlane(ViewOrigin,ViewSides[2],ViewSides[0]);
			ViewFrustum.BoundingPlanes[2] = FPlane(ViewOrigin,ViewSides[3],ViewSides[2]);
			ViewFrustum.BoundingPlanes[3] = FPlane(ViewOrigin,ViewSides[1],ViewSides[3]);
		}

		if(!Viewport->IsWire())
		{
			// Calculate the far clipping plane.

			AZoneInfo*	ViewZoneInfo = Level->GetZoneActor(ViewZone);		
			FLOAT		FarClip = (ViewZoneInfo->bDistanceFog && (Viewport->Actor->ShowFlags & SHOW_DistanceFog)) ? Max(ViewZoneInfo->DistanceFogEnd,Viewport->Actor->CurrentDistanceFogEnd) : FAR_CLIPPING_PLANE;
			FVector		Z = (Deproject(FPlane(0,0,0,NEAR_CLIPPING_PLANE)) - ViewOrigin).SafeNormal();

			ViewFrustum.BoundingPlanes[4] = FPlane(ViewOrigin + Z * FarClip,Z);
			ViewFrustum.NumPlanes = 5;
		}
		else
			ViewFrustum.NumPlanes = 4;
	}

	return ViewFrustum;
}

//
//	FLevelSceneNode::FilterActor
//
UBOOL FLevelSceneNode::FilterActor(AActor* Actor)
{
	if(!(Viewport->Actor->ShowFlags & SHOW_Actors) && !Actor->IsA(ABrush::StaticClass()))
		return 0;

	//WD: rearranged if() statement arguments
	if(!(Viewport->Actor->ShowFlags & SHOW_StaticMeshes) && Actor->StaticMesh)
		return 0;

	//WD: rearranged if() statement arguements
	if(!(Viewport->Actor->ShowFlags & SHOW_FluidSurfaces) && Actor->IsA(AFluidSurfaceInfo::StaticClass()))
		return 0;

	// Draw blocking volumes if SHOW_Collision or KDRAW_Collision is set.
	if( (Viewport->Actor->ShowFlags & SHOW_Collision) && Actor->IsA(ABlockingVolume::StaticClass()) )
		return 1;

	//WD: changed if(GIsEditor) to if(!GIsEditor), and swapped instruction blocks
	if( !GIsEditor )
	{
		if(Actor->bCorona)
		{
			Actor->DrawType = DT_None;
			return 1;
		}
		else
		{
			if((Viewport->Actor->ShowFlags & SHOW_Volumes) && Actor->IsVolumeBrush())
				return 1;
			else if(Actor->bHidden)
				return 0;
			else if ( Actor->bOnlyDrawIfAttached && !Actor->Base )
				return 0;

			if ( Actor->IsOwnedBy(ViewActor) ? (Actor->bOwnerNoSee && !Viewport->Actor->bBehindView) : Actor->bOnlyOwnerSee )
				return 0;
		}

		if((Actor->bHighDetail && Actor->Level->DetailMode == DM_Low) || (Actor->bSuperHighDetail && Actor->Level->DetailMode != DM_SuperHigh))
			return 0;
	}
	else
	{
		if( (Actor->IsVolumeBrush() || Cast<AAntiPortalActor>(Actor)) && !(Viewport->Actor->ShowFlags & SHOW_Volumes))
			return 0;

		if(Actor->IsMovingBrush() && !(Viewport->Actor->ShowFlags & SHOW_MovingBrushes))
			return 0;

		if( Actor->IsHiddenEd() )
			return 0;
	}

		// If attached to a base with a bone, an actor is only drawn along with its base.
	if( Actor->Base && (Actor->AttachmentBone != NAME_None) && Cast<USkeletalMesh>(Actor->Base->Mesh) ) 
			return 0;

	return 1;
	}


//
//	FLevelSceneNode::FilterProjector
//

UBOOL FLevelSceneNode::FilterProjector(AProjector* Actor)
{
    if( !Actor )
		return 1;

	if(Viewport->IsWire())
			return 0;

	if ( Actor->IsOwnedBy(ViewActor) ? (Actor->bOwnerNoSee && !Viewport->Actor->bBehindView) : Actor->bOnlyOwnerSee )
			return 0;

	if((Actor->bHighDetail && Actor->Level->DetailMode == DM_Low) || (Actor->bSuperHighDetail && Actor->Level->DetailMode != DM_SuperHigh))
			return 0;

	return 1;
}

//
//  FLevelSceneNode::Render
//

void FLevelSceneNode::Render(FRenderInterface* RI)
{
	guard(FLevelSceneNode::Render);

	DECLARE_STATIC_UOBJECT(
		UProxyBitmapMaterial,
		HackMaterial,
		{
			static FSolidColorTexture	BlackTexture(FColor(0,0,0));
			HackMaterial->SetTextureInterface(&BlackTexture);
		}
		);

	DECLARE_STATIC_UOBJECT(
		UFinalBlend,
		StencilFinalBlend,
		{
			StencilFinalBlend->Material = HackMaterial;
			StencilFinalBlend->FrameBufferBlending = FB_Invisible;
			StencilFinalBlend->ZWrite = 1;
			StencilFinalBlend->ZTest = 0;
		}
		);

	RI->PushState();

	if( StencilMask && (GIsEditor || Viewport->RenDev->UseStencil) )
	{
		RI->SetStencilOp(CF_NotEqual,StencilMask,StencilMask,SO_Keep,SO_Keep,SO_Increment,DEPTH_COMPLEXITY_MASK(Viewport));

		FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
			0,0,
			Viewport->SizeX,Viewport->SizeY,
			0,0,
			0,0,
			1,
			StencilFinalBlend,
			FColor(0,0,0)
			);
	}

	// Calculate distance fog.
	UBOOL bVolumeFog = 0;

	if ( !Parent && Viewport->Actor )
	{
		APhysicsVolume *V = Viewport->Actor->GetViewTarget()->PhysicsVolume;
		APawn *P = (Viewport->Actor->ViewTarget && Viewport->Actor->ViewTarget->IsAPawn()) ? Cast<APawn>(Viewport->Actor->ViewTarget) : NULL; 
		if ( P )
			V = P->HeadVolume;
		if ( V && V->bDistanceFog )
		{
			UBOOL bShowFog = GEdShowFogInViewports && !Viewport->IsWire();
			bVolumeFog = true;
			RI->SetDistanceFog( bShowFog, V->DistanceFogStart, V->DistanceFogEnd, V->DistanceFogColor );
		}
	}

	if(!bVolumeFog)
	{
		AZoneInfo*	ViewZoneInfo = Level->GetZoneActor(ViewZone);
		APlayerController*	Player			= Viewport->Actor;

		// Interpolate between fog zones.
		if( !Parent && Viewport->IsRealtime() )
		{
			// Handle zone changes.
			if( Player->LastZone != ViewZone )
			{
				// Use correct fog color if transitioning from a zone without fog.
				if( ViewZoneInfo->bDistanceFog && (Player->LastDistanceFogStart == FAR_CLIPPING_PLANE) )
					Player->LastDistanceFogColor = ViewZoneInfo->DistanceFogColor;

				// Don't transition out of zone 0
				if( Player->LastZone == 0 )
					Player->TimeSinceLastFogChange	= 1000000.f; //!! can't use FLT_MAX as value will be increased.
				else
					Player->TimeSinceLastFogChange	= 0.f;

				Player->LastZone = ViewZone;
			}

			FLOAT	Alpha = ViewZoneInfo->DistanceFogBlendTime ? Player->TimeSinceLastFogChange / ViewZoneInfo->DistanceFogBlendTime : 1.f;

			FLOAT	WantedDistanceFogStart,
					WantedDistanceFogEnd;
			FColor	WantedDistanceFogColor;

			// Handle case of transitioning from a fog zone to one without fog.
			if( ViewZoneInfo->bDistanceFog )
			{
				WantedDistanceFogStart	= ViewZoneInfo->DistanceFogStart;
				WantedDistanceFogEnd	= ViewZoneInfo->DistanceFogEnd;
				WantedDistanceFogColor	= ViewZoneInfo->DistanceFogColor;
			}
			else
			{
				WantedDistanceFogStart	= FAR_CLIPPING_PLANE;
				WantedDistanceFogEnd	= FAR_CLIPPING_PLANE;
				WantedDistanceFogColor	= Player->LastDistanceFogColor;
			}

			// Blend completed - also, don't transition into zone 0.
			if( Alpha > 1.f || ViewZone == 0 )
			{
				Player->LastDistanceFogStart	= WantedDistanceFogStart;
				Player->LastDistanceFogEnd		= WantedDistanceFogEnd;
				Player->LastDistanceFogColor	= WantedDistanceFogColor;
				Player->TimeSinceLastFogChange	= 1000000.f; //!! can't use FLT_MAX as value will be increased.
				Alpha = 1.f;
			}

			// Lerp between fog state.
			FLOAT	DistanceFogStart	= Lerp( Player->LastDistanceFogStart, WantedDistanceFogStart, Alpha ),
					DistanceFogEnd		= Lerp( Player->LastDistanceFogEnd  , WantedDistanceFogEnd  , Alpha );
			FColor	DistanceFogColor;	// Can't lerp directly as it will cast to DWORD and lerp there.
			
			DistanceFogColor.R = Lerp( Player->LastDistanceFogColor.R, WantedDistanceFogColor.R, Alpha );
			DistanceFogColor.G = Lerp( Player->LastDistanceFogColor.G, WantedDistanceFogColor.G, Alpha );
			DistanceFogColor.B = Lerp( Player->LastDistanceFogColor.B, WantedDistanceFogColor.B, Alpha );

			// Keep track of current max fog distance for culling.
			Player->CurrentDistanceFogEnd = DistanceFogEnd;

			UBOOL UseFog = ViewZoneInfo->bDistanceFog || Alpha < 1.f;
			RI->SetDistanceFog(UseFog && (Viewport->Actor->ShowFlags & SHOW_DistanceFog),DistanceFogStart,DistanceFogEnd,DistanceFogColor);
		}
		else
		{
			RI->SetDistanceFog(ViewZoneInfo->bDistanceFog && (Viewport->Actor->ShowFlags & SHOW_DistanceFog),ViewZoneInfo->DistanceFogStart,ViewZoneInfo->DistanceFogEnd,ViewZoneInfo->DistanceFogColor);
		}

	}

	RenderLevel(this,RI);

#ifdef WITH_KARMA
	if(!Parent && Level->KWorld)
	{
		// go through world drawing constraints
		// done here instead of tick, so we get contacts etc. even if Karma is paused etc.
		KLevelDebugDrawConstraints(Level);
	}
#endif

	if( StencilMask && (GIsEditor || Viewport->RenDev->UseStencil) )
	{
		FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
			0,0,
			Viewport->SizeX,Viewport->SizeY,
			0,0,
			0,0,
			1,
			StencilFinalBlend,
			FColor(0,0,0)
			);
	}

	// Render lines from elsewhere (collision debugging etc.)
	if(GTempLineBatcher)
	{
		RI->PushState();
		RI->SetTransform(TT_WorldToCamera,WorldToCamera);
		RI->SetTransform(TT_CameraToScreen,CameraToScreen);
		GTempLineBatcher->Render(RI);
		RI->PopState();
	}

	// Render the graph
	if(GStatGraph)
		GStatGraph->Render(Viewport, RI);

	RI->PopState();

	unguard;
}

//
//	FCameraSceneNode::FCameraSceneNode
//
FCameraSceneNode::FCameraSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget,AActor* CameraActor,FVector CameraLocation,FRotator CameraRotation,FLOAT CameraFOV) : FLevelSceneNode(InViewport,InRenderTarget)
{
	guard(FCameraSceneNode::FCameraSceneNode);

	// Determine which zone the camera is in.
	FPointRegion	CameraZone = Model->PointRegion(CameraActor->Level,CameraLocation);

	ViewOrigin		= CameraLocation;
	ViewActor		= CameraActor;
	ViewZone		= (GIsEditor && Viewport->IsWire()) || Viewport->Precaching ? 0 : Model->PointRegion(ViewActor->Level,ViewOrigin).ZoneNumber;
	ViewRotation	= CameraRotation;
	ViewFOV			= CameraFOV;

	UpdateMatrices();

	unguard;
}

//
//	FCameraSceneNode::UpdateMatrices
//
void FCameraSceneNode::UpdateMatrices()
{
	// Initialize the view matrix.
	WorldToCamera = FTranslationMatrix(-ViewOrigin);

	if(!Viewport->IsOrtho())
		WorldToCamera = WorldToCamera * FInverseRotationMatrix(ViewRotation);

	if(Viewport->Actor->RendMap == REN_OrthXY)
		WorldToCamera = WorldToCamera * FMatrix(
									FPlane(Viewport->ScaleX,	0,					0,					0),
									FPlane(0,					-Viewport->ScaleY,	0,					0),
									FPlane(0,					0,					-1,					0),
									FPlane(0,					0,					-ViewOrigin.Z,		1));
	else if(Viewport->Actor->RendMap == REN_OrthXZ)
		WorldToCamera = WorldToCamera * FMatrix(
									FPlane(Viewport->ScaleX,	0,					0,					0),
									FPlane(0,					0,					-1,					0),
									FPlane(0,					Viewport->ScaleY,	0,					0),
									FPlane(0,					0,					-ViewOrigin.Y,		1));
	else if(Viewport->Actor->RendMap == REN_OrthYZ)
		WorldToCamera = WorldToCamera * FMatrix(
									FPlane(0,					0,					1,					0),
									FPlane(Viewport->ScaleX,	0,					0,					0),
									FPlane(0,					Viewport->ScaleY,	0,					0),
									FPlane(0,					0,					ViewOrigin.X,		1));
	else
		WorldToCamera = WorldToCamera * FMatrix(
									FPlane(0,					0,					1,	0),
									FPlane(Viewport->ScaleX,	0,					0,	0),
									FPlane(0,					Viewport->ScaleY,	0,	0),
									FPlane(0,					0,					0,	1));

	CameraToWorld = WorldToCamera.Inverse();

	// Initialize the projection matrix.
	if(Viewport->IsOrtho())
	{
		FLOAT	Zoom = Viewport->Actor->OrthoZoom / (Viewport->SizeX * 15.0f);

		CameraToScreen = FOrthoMatrix(Zoom * RenderTarget->GetWidth() / 2,Zoom * RenderTarget->GetHeight() / 2,0.5f / HALF_WORLD_MAX,HALF_WORLD_MAX);
	}
	else
	{
		FLOAT		FOV		= ViewFOV * PI / 360.0f;
		CameraToScreen = FPerspectiveMatrix(
							FOV,
							FOV,
							1.f,
							(FLOAT) RenderTarget->GetWidth() / RenderTarget->GetHeight(),
							NEAR_CLIPPING_PLANE,
							FAR_CLIPPING_PLANE
							);
	}

	ScreenToCamera = CameraToScreen.Inverse();

	WorldToScreen = WorldToCamera * CameraToScreen;
	ScreenToWorld = ScreenToCamera * CameraToWorld;

	Determinant = WorldToScreen.Determinant();

	CameraX = ScreenToWorld.TransformNormal(FVector(2.0f / Viewport->SizeX,0,0));
	CameraY = ScreenToWorld.TransformNormal(FVector(0,2.0f / Viewport->SizeY,0));
}


//
//	FCameraSceneNode::~FCameraSceneNode
//
FCameraSceneNode::~FCameraSceneNode()
{
}

//
//	FCameraSceneNode::Render
//
void FCameraSceneNode::Render(FRenderInterface* RI)
{
	guard(FCameraSceneNode::Render);

	RI->PushState();

	if(Viewport->Actor->RendMap == REN_DepthComplexity)
		RI->SetStencilOp(CF_Always,0,0,SO_Keep,SO_Keep,SO_Increment,DEPTH_COMPLEXITY_MASK(Viewport));

	// Clear the screen.
	if( !GIsEditor && Viewport->IsWire() )
		RI->Clear();

	// Render the level.
	FLevelSceneNode::Render(RI);

	// Use the stencil buffer to render depth complexity.
	if(Viewport->Actor->RendMap == REN_DepthComplexity)
	{
		DECLARE_STATIC_UOBJECT(
			UProxyBitmapMaterial,
			HackMaterial,
			{
				static FSolidColorTexture	WhiteTexture(FColor(255,255,255));
				HackMaterial->SetTextureInterface(&WhiteTexture);
			}
			);

		DECLARE_STATIC_UOBJECT(
			UFinalBlend,
			DepthComplexityBlend,
			{
				DepthComplexityBlend->ZTest = 0;
				DepthComplexityBlend->Material = HackMaterial;
			}
			);

		FLOAT	LegendHeight = Viewport->SizeY / 16.0f;

		for(DWORD Stencil = 0;Stencil < 4;Stencil++)
		{
			RI->SetStencilOp(CF_Equal,(Stencil + 1) | ~DEPTH_COMPLEXITY_MASK(Viewport),DEPTH_COMPLEXITY_MASK(Viewport),SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				0,0,
				Viewport->SizeX - 32,Viewport->SizeY,
				0,0,
				0,0,
				0,
				DepthComplexityBlend,
				FColor(
					Stencil * 64,
					255,
					0
					)
				);

			RI->SetStencilOp(CF_Always,0,0,SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				Viewport->SizeX - 32,Stencil * LegendHeight,
				Viewport->SizeX,(Stencil + 1) * LegendHeight,
				0,0,
				0,0,
				0.0f,
				DepthComplexityBlend,
				FColor(
					Stencil * 64,
					255,
					0
					)
				);
		}

		for(DWORD Stencil = 4;Stencil < 16;Stencil++)
		{
			RI->SetStencilOp(CF_Equal,(Stencil + 1) | ~DEPTH_COMPLEXITY_MASK(Viewport),DEPTH_COMPLEXITY_MASK(Viewport),SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				0,0,
				Viewport->SizeX - 32,Viewport->SizeY,
				0,0,
				0,0,
				0,
				DepthComplexityBlend,
				FColor(
					255,
					231 - (Stencil - 4) * 21,
					0
					)
				);

			RI->SetStencilOp(CF_Always,0,0,SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				Viewport->SizeX - 32,Stencil * LegendHeight,
				Viewport->SizeX,(Stencil + 1) * LegendHeight,
				0,0,
				0,0,
				0.0f,
				DepthComplexityBlend,
				FColor(
					255,
					231 - (Stencil - 4) * 21,
					0
					)
				);
		}
	}

	UpdateVertexPools(Viewport);

	if ( Viewport->Canvas->pCanvasUtil ) 
        Viewport->Canvas->pCanvasUtil->Flush();

	RI->PopState();

	unguard;
}

//
//	FPlayerSceneNode::FPlayerSceneNode
//
FPlayerSceneNode::FPlayerSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget,AActor* CameraActor,FVector CameraLocation,FRotator CameraRotation,FLOAT CameraFOV) : FCameraSceneNode(InViewport,InRenderTarget,CameraActor,CameraLocation,CameraRotation,CameraFOV)
{
}

//
//	FPlayerSceneNode::Render
//
void FPlayerSceneNode::Render(FRenderInterface* RI)
{
	guard(FPlayerSceneNode::Render);

	APlayerController*	Controller = Viewport->Actor;

	RI->PushState();

	if(Viewport->Actor->RendMap == REN_DepthComplexity)
		RI->SetStencilOp(CF_Always,0,0,SO_Keep,SO_Keep,SO_Increment,DEPTH_COMPLEXITY_MASK(Viewport));

	// Clear the screen.
	if(!GIsEditor && Viewport->IsWire())
		RI->Clear();

	// Update the console.
	Viewport->Canvas->Update();

	if(Viewport->Console)
		Viewport->Console->PreRender(this);

    if ( Viewport->Canvas->pCanvasUtil ) 
        Viewport->Canvas->pCanvasUtil->Flush();

	// Fiddle with the projection matrix to produce a letterbox viewport.
	if( Viewport->bRenderCinematics )
	{
		RI->Clear( 1, FColor(0,0,0), 0, 0, 0, 0 );
		Viewport->ScaleY = Viewport->CinematicsRatio;
		UpdateMatrices();
		FLOAT SizeY	= Viewport->SizeY / Viewport->ScaleY;
		RI->SetViewport( 0, (Viewport->SizeY - SizeY ) / 2, Viewport->SizeX, SizeY );
	}

	// Prerender camera effects.
	INT	LastCameraEffect = 0;

	for(INT EffectIndex = Controller->CameraEffects.Num() - 1;EffectIndex >= 0;EffectIndex--)
	{
		Controller->CameraEffects(EffectIndex)->PreRender(Viewport,RI);
		LastCameraEffect = EffectIndex;

		if(Controller->CameraEffects(EffectIndex)->FinalEffect)
			break;
	}

	// Render the level.
	if( !Viewport->Console || Viewport->Console->GetDrawWorld() || Viewport->Precaching )
		FLevelSceneNode::Render(RI);	

	if ( Viewport->Canvas->pCanvasUtil ) 
        Viewport->Canvas->pCanvasUtil->Flush();

	// Undo the letterbox viewport.
	if( Viewport->bRenderCinematics )
	{
		Viewport->ScaleY = 1;
		UpdateMatrices();
		RI->SetViewport( 0, 0, Viewport->SizeX, Viewport->SizeY );
	}

	// PostRender camera effects.
	for(INT EffectIndex = LastCameraEffect;EffectIndex < Controller->CameraEffects.Num();EffectIndex++)
	{
		Controller->CameraEffects(EffectIndex)->PostRender(Viewport,RI);

		if(Controller->CameraEffects(EffectIndex)->FinalEffect)
			break;
	}

	RI->PixoSetHint( PIXOHINT_HUD );

	// Render the screen flash.
#if 0 //!!MERGE -- remove all other Flash code.
	FPlane	FlashScale = Viewport->GetOuterUClient()->ScreenFlashes ? Viewport->Actor->FlashScale : FVector(1.f,1.f,1.f),
			FlashFog   = Viewport->GetOuterUClient()->ScreenFlashes ? Viewport->Actor->FlashFog : FVector(0,0,0);

	FlashScale.X = Clamp( FlashScale.X, 0.f, 1.f );
	FlashScale.Y = Clamp( FlashScale.Y, 0.f, 1.f );
	FlashScale.Z = Clamp( FlashScale.Z, 0.f, 1.f );
	FlashFog.X   = Clamp( FlashFog.X  , 0.f, 1.f );
	FlashFog.Y   = Clamp( FlashFog.Y  , 0.f, 1.f );
	FlashFog.Z   = Clamp( FlashFog.Z  , 0.f, 1.f );

	if(FlashScale.X < 1.0f)
	{
		static FSolidColorTexture	WhiteTexture(FColor(255,255,255,255));

		DECLARE_STATIC_UOBJECT(
			UProxyBitmapMaterial,
			WhiteTextureMaterial,
			{
				WhiteTextureMaterial->SetTextureInterface(&WhiteTexture);
			}
			);

		DECLARE_STATIC_UOBJECT(
			UFinalBlend,
			ScreenFlashBlend,
			{
				ScreenFlashBlend->FrameBufferBlending = FB_AlphaBlend;
				ScreenFlashBlend->Material = WhiteTextureMaterial;
				ScreenFlashBlend->ZWrite = 0;
			}
			);

		FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
			0,0,
			Viewport->SizeX,Viewport->SizeY,
			0,0,
			0,0,
			0,
			ScreenFlashBlend,
			FColor(
				FPlane(
					FlashFog.X,
					FlashFog.Y,
					FlashFog.Z,
					1.0f - FlashScale.X
					)
				)
			);
	}
#endif

	// Render the HUD and first person weapon.
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_HUDPostRender));

		if ( Viewport->Actor->Pawn && Viewport->Actor->Pawn->Weapon )
			Viewport->Actor->Pawn->Weapon->eventRenderOverlays(Viewport->Canvas);

		if ( Viewport->Canvas->pCanvasUtil ) 
			Viewport->Canvas->pCanvasUtil->Flush();

		if( Viewport->Console )
			Viewport->Console->PostRender(this);

	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_HUDPostRender));

	// Render performance stats.
	clock(GStats.DWORDStats(GEngineStats.STATS_Stats_RenderCycles));
	GStats.Render( Viewport, Level->Engine );
	unclock(GStats.DWORDStats(GEngineStats.STATS_Stats_RenderCycles));

	// Use the stencil buffer to render depth complexity.
	if(Viewport->Actor->RendMap == REN_DepthComplexity)
	{
		DECLARE_STATIC_UOBJECT(
			UProxyBitmapMaterial,
			HackMaterial,
			{
				static FSolidColorTexture	WhiteTexture(FColor(255,255,255));
				HackMaterial->SetTextureInterface(&WhiteTexture);
			}
			);

		DECLARE_STATIC_UOBJECT(
			UFinalBlend,
			DepthComplexityBlend,
			{
				DepthComplexityBlend->ZTest = 0;
				DepthComplexityBlend->Material = HackMaterial;
			}
			);

		FLOAT	LegendHeight = Viewport->SizeY / 16.0f;

		for(DWORD Stencil = 0;Stencil < 4;Stencil++)
		{
			RI->SetStencilOp(CF_Equal,(Stencil + 1) | ~DEPTH_COMPLEXITY_MASK(Viewport),DEPTH_COMPLEXITY_MASK(Viewport),SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				0,0,
				Viewport->SizeX - 32,Viewport->SizeY,
				0,0,
				0,0,
				0,
				DepthComplexityBlend,
				FColor(
					Stencil * 64,
					255,
					0
					)
				);

			RI->SetStencilOp(CF_Always,0,0,SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				Viewport->SizeX - 32,Stencil * LegendHeight,
				Viewport->SizeX,(Stencil + 1) * LegendHeight,
				0,0,
				0,0,
				0.0f,
				DepthComplexityBlend,
				FColor(
					Stencil * 64,
					255,
					0
					)
				);
		}

		for(DWORD Stencil = 4;Stencil < 16;Stencil++)
		{
			RI->SetStencilOp(CF_Equal,(Stencil + 1) | ~DEPTH_COMPLEXITY_MASK(Viewport),DEPTH_COMPLEXITY_MASK(Viewport),SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				0,0,
				Viewport->SizeX - 32,Viewport->SizeY,
				0,0,
				0,0,
				0,
				DepthComplexityBlend,
				FColor(
					255,
					231 - (Stencil - 4) * 21,
					0
					)
				);

			RI->SetStencilOp(CF_Always,0,0,SO_Keep,SO_Keep,SO_Keep,0);

			FCanvasUtil(&Viewport->RenderTarget,RI).DrawTile(
				Viewport->SizeX - 32,Stencil * LegendHeight,
				Viewport->SizeX,(Stencil + 1) * LegendHeight,
				0,0,
				0,0,
				0.0f,
				DepthComplexityBlend,
				FColor(
					255,
					231 - (Stencil - 4) * 21,
					0
					)
				);
		}
	}

	UpdateVertexPools(Viewport);

	if ( Viewport->Canvas->pCanvasUtil ) 
        Viewport->Canvas->pCanvasUtil->Flush();

	RI->PopState();

	unguard;
}

//
//	FActorSceneNode::FActorSceneNode
//
FActorSceneNode::FActorSceneNode(UViewport* InViewport,FRenderTarget* InRenderTarget,AActor* InActor,AActor* CameraActor,FVector CameraLocation,FRotator CameraRotation,FLOAT CameraFOV) : FCameraSceneNode(InViewport,InRenderTarget,CameraActor,CameraLocation,CameraRotation,CameraFOV)
{
	RenderActor = InActor;
}

//
//	FActorSceneNode::Render
//
void FActorSceneNode::Render(FRenderInterface* RI)
{
	guard(FActorSceneNode::Render);

	FMemMark	MemMark(GSceneMem);

	RI->PushState();

	// Set the view and projection matrices.
	if( Viewport->Actor->UseFixedVisibility )
		RI->SetTransform(TT_WorldToCamera,Viewport->Actor->RenderWorldToCamera);
	else
		RI->SetTransform(TT_WorldToCamera,WorldToCamera);
	RI->SetTransform(TT_CameraToScreen,CameraToScreen);

	// Cache actor rendering data.
	FDynamicActor*	DynamicActor = RenderActor->GetActorRenderData();

	// Find lights that affect this actor.
	TList<FDynamicLight*>*	ActorLights = NULL;

	if(Viewport->IsLit())
	{
		FDynamicLight*	Consider[256];
		INT				NumConsider = 0;

		for(INT LeafIndex = 0;LeafIndex < RenderActor->Leaves.Num();LeafIndex++)
		{
			FLeaf&	Leaf = Model->Leaves(RenderActor->Leaves(LeafIndex));

			if(Leaf.iPermeating != INDEX_NONE && NumConsider < 256)
				for(INT PermeatingIndex = Leaf.iPermeating;Model->Lights(PermeatingIndex);PermeatingIndex++)
				{
					AActor*			LightActor = Model->Lights(PermeatingIndex);
					FDynamicLight*	DynamicLight = LightActor->GetLightRenderData();

					// Cull lights.
					if(RenderActor->DrawType == DT_StaticMesh && RenderActor->bStatic && !RenderActor->bLightChanged && !(DynamicLight->Dynamic || DynamicLight->Changed))
						continue;

					if(DynamicLight->Actor->bSpecialLit != RenderActor->bSpecialLit)
						continue;

					FLOAT	DistanceSquared = (DynamicLight->Position - DynamicActor->BoundingSphere).SizeSquared();

					if(LightActor->LightEffect != LE_Sunlight && DistanceSquared > Square(DynamicLight->Radius + DynamicActor->BoundingSphere.W))
						continue;

					UBOOL	Exists = 0;

					for(INT ConsiderIndex = 0;ConsiderIndex < NumConsider;ConsiderIndex++)
						if(Consider[ConsiderIndex] == DynamicLight)
						{
							Exists = 1;
							break;
						}

					if(Exists)
						continue;

					Consider[NumConsider++] = DynamicLight;

					if(NumConsider >= 256)
						break;
				}
		}

		ActorLights = GetRelevantLights(this,DynamicActor,DynamicActor->BoundingSphere,Consider,NumConsider);
	}

	// Find dynamic projectors that affect this actor.
	TList<FProjectorRenderInfo*>*	ActorProjectors = NULL;

	if(RenderActor->bAcceptsProjectors)
	{
		for(INT ProjectorIndex = 0;ProjectorIndex < Level->DynamicProjectors.Num();ProjectorIndex++)
		{
			FProjectorRenderInfo*	ProjectorInfo = Level->DynamicProjectors(ProjectorIndex);

			if(!ProjectorInfo->Render(Level->GetLevelInfo()->TimeSeconds))
			{
				// Replace this projector with the last dynamic projector, and remove an item from the end of the array.
				Level->DynamicProjectors(ProjectorIndex--) = Level->DynamicProjectors(Level->DynamicProjectors.Num() - 1);
				Level->DynamicProjectors.Remove(Level->DynamicProjectors.Num() - 1);
				continue;
			}

			check(ProjectorInfo->Projector); // There should never be a dynamic projector without an attached actor!

			if(FilterProjector(ProjectorInfo->Projector))
			{
				if(!ProjectorInfo->Projector->bProjectActor && RenderActor->Mesh)
					continue;

				if(!ProjectorInfo->Projector->bProjectStaticMesh && RenderActor->StaticMesh)
					continue;

				UBOOL	Inside = 1;
				FVector	ActorCenter = DynamicActor->BoundingBox.GetCenter(),
						ActorExtent = DynamicActor->BoundingBox.GetExtent();

				for(INT PlaneIndex = 0;PlaneIndex < 6;PlaneIndex++)
				{
					FLOAT	PushOut = FBoxPushOut(ProjectorInfo->FrustumPlanes[PlaneIndex],ActorExtent),
							Dist = ProjectorInfo->FrustumPlanes[PlaneIndex].PlaneDot(ActorCenter);

					if(Dist < -PushOut)
					{
						Inside = 0;
						break;
					}
				}

				if(Inside)
					ActorProjectors = new(GSceneMem) TList<FProjectorRenderInfo*>(ProjectorInfo,ActorProjectors);
			}
		}
	}

	// Draw the actor.
	DynamicActor->Render(this,ActorLights,ActorProjectors,RI);

	RI->PopState();

	MemMark.Pop();

	unguard;
}

//
//	FSkySceneNode::FSkySceneNode
//
FSkySceneNode::FSkySceneNode(FLevelSceneNode* InParent,INT InViewZone) :
	FLevelSceneNode(
		InParent,
		InViewZone,
		FTranslationMatrix(-InParent->Level->GetZoneActor(InViewZone)->Location) *
			FInverseRotationMatrix(InParent->Level->GetZoneActor(InViewZone)->Rotation) *
			FTranslationMatrix(InParent->ViewOrigin)
		)
{
}

//
// MirrorMatrix
//
FMatrix MirrorMatrix(FPlane Plane)
{
	FVector	Origin = -2 * Plane * -Plane.W,
			X = FVector(1,0,0) - 2 * Plane * (FVector(1,0,0) | Plane),
			Y = FVector(0,1,0) - 2 * Plane * (FVector(0,1,0) | Plane),
			Z = FVector(0,0,1) - 2 * Plane * (FVector(0,0,1) | Plane);
	FMatrix	Mirrored(
				FPlane(	X.X,			Y.X,			Z.X,			0	),
				FPlane(	X.Y,			Y.Y,			Z.Y,			0	),
				FPlane(	X.Z,			Y.Z,			Z.Z,			0	),
				FPlane(	-Origin | X,	-Origin | Y,	-Origin | Z,	1	)
				);

	return Mirrored;
}

//
//	FMirrorSceneNode::FMirrorSceneNode
//
FMirrorSceneNode::FMirrorSceneNode(FLevelSceneNode* InParent,FPlane MirrorPlane,INT InViewZone,INT InMirrorSurface) :
	FLevelSceneNode(
		InParent,
		InViewZone,
		MirrorMatrix(
			MirrorPlane
			)
		)
{
	MirrorSurface = InMirrorSurface;
}

//
//  FWarpZoneSceneNode::FWarpZoneSceneNode
//
FWarpZoneSceneNode::FWarpZoneSceneNode(FLevelSceneNode* InParent,AWarpZoneInfo* WarpZone) :
	FLevelSceneNode(
		InParent,
		WarpZone->OtherSideActor->Region.ZoneNumber,
		WarpZone->OtherSideActor->WarpCoords.Matrix() * WarpZone->WarpCoords.Transpose().Matrix()
		)
{
	InvisibleZone = ViewZone;
}

//
//  Globals.
//
FSceneMemStack	GSceneMem;

//
//  FSceneMemStack::FSceneMemStack
//
FSceneMemStack::FSceneMemStack()
{
	Init(32768);
}

//
//  FSceneMemStack::~FSceneMemStack
//
FSceneMemStack::~FSceneMemStack()
{
	Exit();
}

