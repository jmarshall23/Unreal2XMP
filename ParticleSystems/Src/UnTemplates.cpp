#include "D3DDrv.h"
#include "ParticleSystems.h"
#include "UnParticleRender.h"

#define LOCAL_TO_WORLD \
	FMatrix LocalToWorld = \
		(	System->bLockParticles \
		?	System->LockedLocalToWorld * (FRotationMatrix(System->Rotation) * FTranslationMatrix(System->Location)) \
		:	FMatrix::Identity	);

#define LOCAL_LOC \
	FVector LocalLoc = System->bLockParticles ? LocalToWorld.TransformFVector(Particle->Location) : Particle->Location;

#define PARTICLE_ITER_BEGIN(type) \
	NOTE(debugf(TEXT("%s -- (%s)PARTICLE_ITER_BEGIN(%s): %d"),System?System->GetFullName():TEXT("NULL"),GetFullName(),TEXT(#type),ParticleList.Num())); \
	for( INT iParticle=0; iParticle<ParticleList.Num(); iParticle++ ){ \
		type* Particle = ParticleCastChecked<type>(ParticleList(iParticle)); \
		if( Particle->LifeSpan > 0.f ){ \

#define PARTICLE_ITER_END } }

//------------------------------------------------------------------------------
INT UActorParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UActorParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UActorParticleTemplate::Render"),GetFullName()));

	//!!ARL
	return 0;
	if(!ParticleList.Num()) return 0;
	if(Template==NULL) return 0;
	if(Template->StaticMesh==NULL) return 0;

	INT NumParticlesDrawn=0;

	FVector OriginalLocation = Template->Location;
	FRotator OriginalRotation = Template->Rotation;

	for( INT i=0; i<ParticleList.Num(); i++ )
	{
		UParticle* Particle = ParticleList(i);

		Template->Location = Particle->Location;
		Template->Rotation = Particle->Velocity.Rotation();

		FDynamicActor(Template).Render(Frame,Lights,NULL,RI);

		NumParticlesDrawn++;
	}

	Template->Location = OriginalLocation;
	Template->Rotation = OriginalRotation;

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(UActorParticleTemplate);


//------------------------------------------------------------------------------
INT ULineManagerTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(ULineManagerTemplate::Render);
	NOTE(debugf(TEXT("(%s)ULineManagerTemplate::Render"),GetFullName()));

	if(!ParticleList.Num()) return 0;

	INT NumParticlesDrawn=0;

	FRenderInterface* RI = Frame->Viewport->RI;

	FLineBatcher LineBatcher(RI);
	RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);
	
	PROJECTIONS

	PARTICLE_ITER_BEGIN(ULineParticle)

		// Draw the line between the endpoints.
		Pts[0] = Particle->Location;
		Pts[1] = Particle->EndPoint;

		LineBatcher.DrawLine(Particle->Location,Particle->EndPoint,Particle->Color);

		// Draw the arrowhead.
		if( Particle->bDirected )
		{
			FVector Dir		= (Particle->EndPoint - Particle->Location).SafeNormal();
			FVector Base	= Particle->Location + Dir * Particle->ArrowOffset;
			FVector Offset	= (Dir ^ ProjFront) * Particle->ArrowWidth;

			Pts[0] = Base + Offset;
			Pts[1] = Base - Offset;
			Pts[2] = Base + Dir * Particle->ArrowHeight;

			LineBatcher.DrawLine(Pts[0],Pts[1],Particle->ArrowColor);
			LineBatcher.DrawLine(Pts[1],Pts[2],Particle->ArrowColor);
			LineBatcher.DrawLine(Pts[2],Pts[0],Particle->ArrowColor);
		}

		NumParticlesDrawn++;

	PARTICLE_ITER_END

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(ULineManagerTemplate);


//------------------------------------------------------------------------------
void UParticleDecalTemplate::InitParticle( UParticle* &P )
{
	guard(UParticleDecalTemplate::InitParticle);
	
	Super::InitParticle( P );

	UParticleDecal* Particle = ParticleCastChecked<UParticleDecal>(P);

	Particle->Offsets[0] = FVector(0,0,0);
	Particle->Offsets[1] = FVector(0,0,0);
	Particle->Offsets[2] = FVector(0,0,0);
	Particle->Offsets[3] = FVector(0,0,0);

	unguard;
}

//------------------------------------------------------------------------------
void UParticleDecalTemplate::SetParticleDirection( UParticle* P, FVector Dir )
{
	guard(UParticleDecalTemplate::SetParticleDirection);
	NOTE(debugf(TEXT("(%s)UParticleDecalTemplate::SetParticleDirection( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Dir.X, Dir.Y, Dir.Z ));
	
	Super::SetParticleDirection( P, Dir );

	if( Dir == FVector(0,0,0) ) return;

	UParticleDecal* Particle = ParticleCastChecked<UParticleDecal>(P);

	FVector Diag1(0,0,0);
	while( Diag1 == FVector(0,0,0) )
	{
		Diag1 = Dir ^ VRand();
	}
	Diag1.Normalize();
	FVector Diag2 = Dir ^ Diag1;
	Diag2.Normalize();

	Particle->Offsets[0] = Diag1;
	Particle->Offsets[1] = Diag2;
	Particle->Offsets[2] = -Particle->Offsets[0];
	Particle->Offsets[3] = -Particle->Offsets[1];

	// Fix ARL: Clip to surface like normal decals?

	unguard;
}

//------------------------------------------------------------------------------
FVector UParticleDecalTemplate::GetParticleNormal( UParticle* Particle )
{
	guard(UParticleDecalTemplate::GetParticleNormal);
	NOTE(debugf(TEXT("(%s)UParticleDecalTemplate::GetParticleNormal( %s )"), GetFullName(), Particle ? *Particle->GetClassName() : TEXT("None") ));
	UParticleDecal* P=ParticleCastChecked<UParticleDecal>(Particle);
	return ((P->Offsets[1]-P->Offsets[2])^(P->Offsets[0]-P->Offsets[1])).SafeNormal();
	unguard;
}

//------------------------------------------------------------------------------
void UParticleDecalTemplate::SetParticleSize( UParticle* Particle, FLOAT NewSize )
{
	guard(UParticleDecalTemplate::SetParticleSize);
	NOTE(debugf(TEXT("(%s)UParticleDecalTemplate::SetParticleSize( %s, %f )"), GetFullName(), Particle ? *Particle->GetClassName() : TEXT("None"), NewSize ));
	UParticleDecal* P=ParticleCastChecked<UParticleDecal>(Particle);
	P->Size = NewSize;
	unguard;
}

//------------------------------------------------------------------------------
FLOAT UParticleDecalTemplate::GetParticleSize( UParticle* P )
{
	guard(UParticleDecalTemplate::GetParticleSize);
	NOTE(debugf(TEXT("(%s)UParticleDecalTemplate::GetParticleSize"), GetFullName() ));
	return ParticleCastChecked<UParticleDecal>(P)->Size;
	unguard;
}

//------------------------------------------------------------------------------
INT UParticleDecalTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UParticleDecalTemplate::Render);
	NOTE(debugf(TEXT("(%s)UParticleDecalTemplate::Render"),GetFullName()));

	INT NumParticlesDrawn=0;

#if 0 //ProjDecals
	// Projector support.
	if( bUseProjectors )
	{
		if(!ProjActor)
		{
			ProjActor = CastChecked<AProjector>(System->GetLevel()->SpawnActor( LoadClass<AProjector>( NULL, TEXT("ParticleSystems.ParticleDecalProjector"), NULL, LOAD_NoWarn, NULL ), NAME_None, System->Location ));
#define COPY_PROP(p) ProjActor->p=p;
			COPY_PROP(MaterialBlendingOp)
			COPY_PROP(FrameBufferBlendingOp)
			COPY_PROP(FOV)
			COPY_PROP(MaxTraceDistance)
			COPY_PROP(bProjectBSP)
			COPY_PROP(bProjectTerrain)
			COPY_PROP(bProjectStaticMesh)
			COPY_PROP(bProjectParticles)
			COPY_PROP(bProjectActor)
			COPY_PROP(bClipBSP)
			COPY_PROP(bClipStaticMesh)
			COPY_PROP(bProjectOnUnlit)
			COPY_PROP(bGradient)
			COPY_PROP(bProjectOnBackfaces)
			COPY_PROP(bProjectOnAlpha)
			COPY_PROP(bProjectOnParallelBSP)
			COPY_PROP(ProjectTag)
			COPY_PROP(ExcludedActorsTag)
#undef COPY_PROP
			ProjActor->ProjTexture=Sprite;

			PARTICLE_ITER_BEGIN(UParticleDecal)

				ProjActor->RenderInfo = Particle->RenderInfo;
				ProjActor->Detach(1);

				ProjActor->SetLocation( Particle->Location );
				ProjActor->SetRotation( (Particle->Offsets[1] ^ Particle->Offsets[0]).Rotation() );
				ProjActor->DrawScale = Particle->Size;
				ProjActor->ScaleGlow = Particle->Alpha;

				ProjActor->Attach();
				Particle->RenderInfo = ProjActor->RenderInfo;

			PARTICLE_ITER_END
		}
	} else
#endif

	if( ParticleList.Num() && (Sprite || SpriteArray) ) { //goto Skip

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, SpriteArray ? SpriteArray : Sprite, Style );

	LOCAL_TO_WORLD

	PARTICLE_ITER_BEGIN(UParticleDecal)

		FColor Color = Light(Particle,Lights).Scale( Particle->Alpha, bFadeAlphaOnly );

		LOCAL_LOC

		SP.AddQuad(
			Color,
			LocalLoc + Particle->Offsets[0] * Particle->Size,
			LocalLoc + Particle->Offsets[1] * Particle->Size,
			LocalLoc + Particle->Offsets[2] * Particle->Size,
			LocalLoc + Particle->Offsets[3] * Particle->Size,
			SpriteArray ? SpriteArray->GetElementUV(Particle->GetSpriteIndex(this)) : FRectangle(0,0,1,1)
			);

		NumParticlesDrawn++;
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NumParticlesDrawn += DrawCoronas(Owner,Frame,Lights,RI,System);

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(UParticleDecalTemplate);

//------------------------------------------------------------------------------
void UPointParticleTemplate::InitParticle( UParticle* &P )
{
	guard(UPointParticleTemplate::InitParticle);
	
	Super::InitParticle( P );

	UPointParticle* Particle = ParticleCastChecked<UPointParticle>(P);

	Particle->Color = Color;
	Particle->Alpha = InitialAlpha.GetRand();

	unguard;
}

//------------------------------------------------------------------------------
INT UPointParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UPointParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UPointParticleTemplate::Render"),GetFullName()));

	if(!ParticleList.Num()) return 0;

	FDefaultSpriteRenderer<1> SP( Frame, Lights, RI, Owner, this, Texture, Style );
	RI->SetPointSprite( 1 );
	RI->SetPointScale( Scale, *(DWORD*)&ScaleA, *(DWORD*)&ScaleB, *(DWORD*)&ScaleC );
	
	LOCAL_TO_WORLD

	PARTICLE_ITER_BEGIN(UPointParticle)

		LOCAL_LOC
		SP.AddPoint( Color, LocalLoc );
	
	PARTICLE_ITER_END

	SP.Release();

	return SP.PointsDrawn();

	unguard;
}

IMPLEMENT_CLASS(UPointParticleTemplate);

//------------------------------------------------------------------------------
void URecursiveParticleTemplate::InitParticle( UParticle* &P )
{
	guard(URecursiveParticleTemplate::InitParticle);

	Super::InitParticle( P );

	URecursiveParticle* Particle = ParticleCastChecked<URecursiveParticle>(P);

	if( Particle->Generator )
	{
		Particle->Generator->bOn = true;
		Particle->Generator->PrimeTime = PrimeTime;
	}
	else if( ParticleGeneratorClass )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());
		Particle->Generator = CastChecked<AParticleGenerator>(System->GetLevel()->SpawnActor
			( 
				ParticleGeneratorClass,
				NAME_None, 
				System->Location,
				System->Rotation,
				NULL,
				0,
				0
			));
		PrimeTime = Particle->Generator->PrimeTime;
	}
	else if( ParticleGeneratorTemplate )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());
		Particle->Generator = ParticleGeneratorTemplate->Duplicate( System->GetLevel() );
		PrimeTime = Particle->Generator->PrimeTime;
	}
	else
	{
		Particle->LifeSpan = 0.f;
	}

	unguard;
}

//------------------------------------------------------------------------------
void URecursiveParticleTemplate::SetParticleDirection( UParticle* P, FVector Dir )
{
	guard(URecursiveParticleTemplate::SetParticleDirection);
	NOTE(debugf(TEXT("(%s)URecursiveParticleTemplate::SetParticleDirection( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Dir.X, Dir.Y, Dir.Z ));

	Super::SetParticleDirection( P, Dir );

	URecursiveParticle* Particle = ParticleCastChecked<URecursiveParticle>(P);

	if( Particle->Generator )
	{
		if( !Target )
		{
			Particle->Generator->SetRotation( Dir.Rotation() + AdditionalRotation );
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
void URecursiveParticleTemplate::SetParticleLocation( UParticle* P, FVector Loc )
{
	guard(URecursiveParticleTemplate::SetParticleLocation);
	NOTE(debugf(TEXT("(%s)URecursiveParticleTemplate::SetParticleLocation( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Loc.X, Loc.Y, Loc.Z ));

	Super::SetParticleLocation( P, Loc );

	URecursiveParticle* Particle = ParticleCastChecked<URecursiveParticle>(P);

	if( Particle->Generator )
	{
		Particle->Generator->SetLocation( Particle->Location );

		if( Target )
		{
			Particle->Generator->SetRotation( (Target->Location - Particle->Generator->Location).Rotation() + AdditionalRotation );
		}
	}
	
	unguard;
}

//------------------------------------------------------------------------------
void URecursiveParticleTemplate::RecycleParticle( UParticle* P )
{
	guard(URecursiveParticleTemplate::RecycleParticle);
	NOTE(debugf(TEXT("(%s)URecursiveParticleTemplate::RecycleParticle( %s )"), GetFullName(), P ? *P->GetClassName() : TEXT("None") ));

	URecursiveParticle* Particle = ParticleCastChecked<URecursiveParticle>(P);

	if( Particle->Generator )
	{
		Particle->Generator->bOn = false;
	}

	Super::RecycleParticle( P );

	unguard;
}

// Fix ARL: GetMaxLifeSpan should be overridden to account for additional LifeSpan of Template generator.

//------------------------------------------------------------------------------
INT URecursiveParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(URecursiveParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)URecursiveParticleTemplate::Render"),GetFullName()));

	FRotator Rot=FRotator(0,0,0);

	LOCAL_TO_WORLD

	PARTICLE_ITER_BEGIN(URecursiveParticle)

		if( Particle->Generator )
		{
			LOCAL_LOC

			Particle->Generator->SetLocation( LocalLoc );

			if( Target )
			{
				Rot = (Target->Location - Particle->Generator->Location).Rotation();
			}
			else if( Particle->Velocity!=FVector(0,0,0) )
			{
				Rot = Particle->Velocity.Rotation();
			}

			Rot += AdditionalRotation;

			Particle->Generator->SetRotation( Rot );
		}
		else
		{
			Particle->LifeSpan = 0.f;
		}
	
	PARTICLE_ITER_END

	return 0;

	unguard;
}

IMPLEMENT_CLASS(URecursiveParticleTemplate);

//------------------------------------------------------------------------------
void URotatingParticleTemplate::InitParticle( UParticle* &P )
{
	guard(URotatingParticleTemplate::InitParticle);
	
	Super::InitParticle( P );

	URotatingParticle* Particle = ParticleCastChecked<URotatingParticle>(P);

	Particle->Angle = InitialAngle.GetRand();

	unguard;
}

//------------------------------------------------------------------------------
INT URotatingParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(URotatingParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)URotatingParticleTemplate::Render"),GetFullName()));

	INT NumParticlesDrawn=0;

	if( ParticleList.Num() && (Sprite || SpriteArray) ) { //goto Skip
	
	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, SpriteArray ? SpriteArray : Sprite, Style, !bNoClip );
	
	LOCAL_TO_WORLD

	PROJECTIONS

	INT USize = SpriteArray ? SpriteArray->GetUSize() : Sprite->MaterialUSize();
	INT VSize = SpriteArray ? SpriteArray->GetVSize() : Sprite->MaterialVSize();

	FVector GlobalRight	= ProjRight	* ( USize >> 1 );
	FVector GlobalUp	= ProjUp	* ( VSize >> 1 );

	PARTICLE_ITER_BEGIN(URotatingParticle)

		if( Particle->Alpha>0.f && Particle->Size>0.f )
		{
			LOCAL_LOC

			if( !bNoClip || IsVisible( System, Particle, LocalLoc, Frame ) )
			{
				FColor Color = Light(Particle,Lights).Scale( Particle->Alpha, bFadeAlphaOnly );
				FVector Right	= Particle->Size * GlobalRight;
				FVector Up		= Particle->Size * GlobalUp;

				Up    =    Up.RotateAngleAxis(Particle->Angle * 65536.0f/360.0f, ProjFront);
				Right = Right.RotateAngleAxis(Particle->Angle * 65536.0f/360.0f, ProjFront);

				SP.AddQuad(
					Color,
					LocalLoc + Up - Right,
					LocalLoc + Up + Right,
					LocalLoc - Up + Right,
					LocalLoc - Up - Right,
					SpriteArray ? SpriteArray->GetElementUV(Particle->GetSpriteIndex(this)) : FRectangle(0,0,1,1)
					);

				NumParticlesDrawn++;
			}
		}
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NumParticlesDrawn += DrawCoronas(Owner,Frame,Lights,RI,System);

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(URotatingParticleTemplate);


//------------------------------------------------------------------------------
INT USprite3DParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(USprite3DParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)USprite3DParticleTemplate::Render"),GetFullName()));

	INT NumParticlesDrawn=0;

	if( ParticleList.Num() && (Sprite || SpriteArray) ) { //goto Skip

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, SpriteArray ? SpriteArray : Sprite, Style, !bNoClip );

	LOCAL_TO_WORLD

	PROJECTIONS

	INT USize = SpriteArray ? SpriteArray->GetUSize() : Sprite->MaterialUSize();
	INT VSize = SpriteArray ? SpriteArray->GetVSize() : Sprite->MaterialVSize();

	FVector GlobalRight	= ProjRight	* ( USize >> 1 );
	FVector GlobalUp	= ProjUp	* ( VSize >> 1 );
	
	PARTICLE_ITER_BEGIN(USpriteParticle)

		if( Particle->Alpha>0.f && Particle->Size>0.f )
		{
			LOCAL_LOC

			if( !bNoClip || IsVisible( System, Particle, LocalLoc, Frame ) )
			{
				FColor Color = Light(Particle,Lights).Scale( Particle->Alpha, bFadeAlphaOnly );
				FVector Right	= Particle->Size * GlobalRight;
				FVector Up		= Particle->Size * GlobalUp;

				SP.AddQuad(
					Color,
					LocalLoc + Up - Right,
					LocalLoc + Up + Right,
					LocalLoc - Up + Right,
					LocalLoc - Up - Right,
					SpriteArray ? SpriteArray->GetElementUV(Particle->GetSpriteIndex(this)) : FRectangle(0,0,1,1)
					);

				NumParticlesDrawn++;
			}
		}
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NumParticlesDrawn += DrawCoronas(Owner,Frame,Lights,RI,System);

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;
	
	unguard;
}

IMPLEMENT_CLASS(USprite3DParticleTemplate);

//------------------------------------------------------------------------------
INT USpriteParticle::GetSpriteIndex( USpriteParticleTemplate* Owner )
{
	guard(USpriteParticle::GetSpriteIndex);

	if( Owner->bAnimateSprite && Owner->SpriteArray && Owner->SpriteIndices.Num() )
	{
		INT Index;
		if( Owner->bRandomSprite )
			Index = appRand() % Owner->SpriteIndices.Num();
		else
			Index = Clamp( (INT)appBlend( Owner->SpriteIndices.Num(), 0.f, (LifeSpan / InitialLifeSpan) ), 0, Owner->SpriteIndices.Num()-1 );
		SpriteIndex = Owner->SpriteIndices( Index );
		NOTE(debugf(TEXT("%f) %s sprite using frame %d[%d]"), appSeconds(), *GetClassName(), SpriteIndex, Index ));
	}

	return SpriteIndex;

	unguard;
}

//------------------------------------------------------------------------------
INT USpriteParticle::GetCoronaIndex( USpriteParticleTemplate* Owner )
{
	guard(USpriteParticle::GetCoronaIndex);

	if( Owner->bAnimateCorona && Owner->CoronaArray && Owner->CoronaIndices.Num() )
	{
		INT Index;
		if( Owner->bRandomCorona )
			Index = appRand() % Owner->CoronaIndices.Num();
		else
			Index = Clamp( (INT)appBlend( Owner->CoronaIndices.Num(), 0.f, (LifeSpan / InitialLifeSpan) ), 0, Owner->CoronaIndices.Num()-1 );
		CoronaIndex = Owner->CoronaIndices( Index );
		NOTE(debugf(TEXT("%f) %s corona using frame %d[%d]"), appSeconds(), *GetClassName(), CoronaIndex, Index ));
	}

	return CoronaIndex;

	unguard;
}

//------------------------------------------------------------------------------
void USpriteParticleTemplate::InitParticle( UParticle* &P )
{
	guard(USpriteParticleTemplate::InitParticle);

	Super::InitParticle( P );

	USpriteParticle* Particle = ParticleCastChecked<USpriteParticle>(P);
	
	if( SpriteIndices.Num() )
		Particle->SpriteIndex = SpriteIndices( (bRandomSprite ? appRand() : SpriteIndex++) % SpriteIndices.Num() );

	if( CoronaIndices.Num() )
		Particle->CoronaIndex = CoronaIndices( (bRandomCorona ? appRand() : CoronaIndex++) % CoronaIndices.Num() );

	Particle->Color = FColor(255,255,255,255);
	Particle->Alpha = InitialAlpha.GetRand();
	Particle->Size = InitialSize.GetRand();

	Particle->InitialLifeSpan = Particle->LifeSpan;

	unguard;
}

//------------------------------------------------------------------------------
FLOAT USpriteParticleTemplate::GetParticleSize( UParticle* P )
{
	guard(USpriteParticleTemplate::GetParticleSize);
	NOTE(debugf(TEXT("(%s)USpriteParticleTemplate::GetParticleSize"), GetFullName() ));
	return ParticleCastChecked<USpriteParticle>(P)->Size;
	unguard;
}

//------------------------------------------------------------------------------
void USpriteParticleTemplate::SetParticleSize( UParticle* P, FLOAT NewSize )
{
	guard(USpriteParticleTemplate::SetParticleSize);
	NOTE(debugf(TEXT("(%s)USpriteParticleTemplate::SetParticleSize(%f)"), GetFullName(), NewSize ));
	ParticleCastChecked<USpriteParticle>(P)->Size = NewSize;
	unguard;
}

//------------------------------------------------------------------------------
INT USpriteParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(USpriteParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)USpriteParticleTemplate::Render"),GetFullName()));

	INT NumParticlesDrawn=0;

	if( ParticleList.Num() && (Sprite || SpriteArray) ) { //goto Skip

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, SpriteArray ? SpriteArray : Sprite, Style );

	// use ScreenSpace
	RI->SetTransform(TT_WorldToCamera,FMatrix::Identity);
	RI->SetTransform(TT_CameraToScreen,FMatrix::Identity);

	LOCAL_TO_WORLD

	PROJECTIONS

	FVector GlobalRight	= FVector(1,0,0) / Frame->Viewport->SizeX;
	FVector GlobalUp	= FVector(0,1,0) / Frame->Viewport->SizeY;

	PARTICLE_ITER_BEGIN(USpriteParticle)

		if( Particle->Alpha>0.f && Particle->Size>0.f )
		{
			LOCAL_LOC
			
			if( ((LocalLoc - Frame->ViewOrigin) | ProjFront) > 0.0f )
			{
				FColor Color	= Light(Particle,Lights).Scale( Particle->Alpha, bFadeAlphaOnly );
				FVector Point	= Frame->Project(LocalLoc);
				FVector Right	= Particle->Size * GlobalRight;
				FVector Up		= Particle->Size * GlobalUp;

				SP.AddQuad(
					Color,
					Point + Up - Right,
					Point + Up + Right,
					Point - Up + Right,
					Point - Up - Right,
					SpriteArray ? SpriteArray->GetElementUV(Particle->GetSpriteIndex(this)) : FRectangle(0,0,1,1)
					);

				NumParticlesDrawn++;
			}
		}
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NumParticlesDrawn += DrawCoronas(Owner,Frame,Lights,RI,System);

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;
	
	unguard;
}

//------------------------------------------------------------------------------
UBOOL USpriteParticleTemplate::IsVisible( AParticleGenerator* System, USpriteParticle* Particle, FVector& LocalLoc, FLevelSceneNode* Frame )
{
	guard(USpriteParticleTemplate::IsVisible);
	NOTE(debugf(TEXT("(%s)USpriteParticleTemplate::IsVisible: %s"),GetFullName(),*Frame->ViewOrigin.String()));

	ULevel* Level = System->GetLevel();
	ALevelInfo* LevelInfo = Level->GetLevelInfo();
	if( LevelInfo->TimeSeconds > Particle->LastTraceTime )
	{
		Particle->LastTraceTime = LevelInfo->TimeSeconds + CoronaTraceRate;

		FCheckResult Hit(1.0f);
		Level->SingleLineCheck
			(	Hit
			,	System
			,	LocalLoc
			,	Frame->ViewOrigin
			,	CoronaOcclude ? TRACE_AllColliding | TRACE_ProjTargets : TRACE_World
			);
		Particle->LastTraceResult = !Hit.Actor;

		NOTE( debugf(TEXT("IsVisible [%s to %s]: hit actor - '%s'"),*Frame->ViewOrigin.String(),*LocalLoc.String(),Hit.Actor->GetFullName()); )
	}

	return Particle->LastTraceResult;

	unguard;
}

//------------------------------------------------------------------------------
INT USpriteParticleTemplate::DrawCoronas( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(USpriteParticleTemplate::DrawCoronas);

	INT NumParticlesDrawn = 0;

	if( ParticleList.Num() && (Corona || CoronaArray) ) { //goto Skip

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, CoronaArray ? CoronaArray : Corona, CoronaStyle, 0 );

	LOCAL_TO_WORLD

	PROJECTIONS
	
	INT USize = CoronaArray ? CoronaArray->GetUSize() : Corona->MaterialUSize();
	INT VSize = CoronaArray ? CoronaArray->GetVSize() : Corona->MaterialVSize();

	FVector GlobalRight	= ProjRight	* ( USize >> 1 );
	FVector GlobalUp	= ProjUp	* ( VSize >> 1 );
	
	PARTICLE_ITER_BEGIN(USpriteParticle)

		if( Particle->Alpha>0.f && Particle->Size>0.f )
		{
			LOCAL_LOC

			if( ((LocalLoc - Frame->ViewOrigin) | ProjFront) > 0.f )
			{
				if( IsVisible( System, Particle, LocalLoc, Frame ) )
				{
					FColor Color = Light(Particle,Lights).Scale( Particle->Alpha * CoronaScaleGlow, bCoronaFadeAlphaOnly );
					FVector Right	= Particle->Size * CoronaScale * GlobalRight;
					FVector Up		= Particle->Size * CoronaScale * GlobalUp;

					SP.AddQuad(
						Color,
						LocalLoc + Up - Right,
						LocalLoc + Up + Right,
						LocalLoc - Up + Right,
						LocalLoc - Up - Right,
						CoronaArray ? CoronaArray->GetElementUV(Particle->GetCoronaIndex(this)) : FRectangle(0,0,1,1)
						);

					NumParticlesDrawn++;
				}
			}
		}
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s' [CORONAS]"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(USpriteParticleTemplate);

//------------------------------------------------------------------------------
void UStrandParticleTemplate::InitParticle( UParticle* &P )
{
	guard(UStrandParticleTemplate::InitParticle);
	
//	Super::InitParticle( P );

	UStrandParticle* Particle = ParticleCastChecked<UStrandParticle>(P);

	Particle->Color = Color;	// Fix ARL: Filter in ColorVariation.

	unguard;
}

//------------------------------------------------------------------------------
INT UStrandParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UStrandParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UStrandParticleTemplate::Render"),GetFullName()));

	if(!ParticleList.Num()) return 0;

	INT NumParticlesDrawn=0;

	FLineBatcher LineBatcher(RI);
	RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);
	
	PARTICLE_ITER_BEGIN(UStrandParticle)

		if( Particle->bRoot )
		{
			for( UStrandParticle* Strand = Particle; Strand->NextStrandParticle; Strand = Strand->NextStrandParticle )
			{
				FColor Color = Strand->Color.Plane();
				// Fix ARL: Adjust for shininess -- angle to camera.
				// Fix ARL: bLockParticles not accounted for.
				LineBatcher.DrawLine(Strand->Location,Strand->NextStrandParticle->Location,Strand->Color);
				NumParticlesDrawn++;
			}
		}
	
	PARTICLE_ITER_END

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(UStrandParticleTemplate);

//------------------------------------------------------------------------------
void UStreakParticleTemplate::InitParticle( UParticle* &P )
{
	guard(UStreakParticleTemplate::InitParticle);
	
	Super::InitParticle( P );

	UStreakParticle* Particle = ParticleCastChecked<UStreakParticle>(P);

	Particle->Color = Color;
	Particle->Alpha = InitialAlpha.GetRand();

	Particle->PrevLocations.Empty();
	Particle->PrevLocations.AddZeroed( appRandRange( (INT)Length.A, (INT)Length.B ) );

	unguard;
}

//------------------------------------------------------------------------------
INT UStreakParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UStreakParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UStreakParticleTemplate::Render"),GetFullName()));

	if(!ParticleList.Num()) return 0;

	INT NumParticlesDrawn=0;

	FLineBatcher LineBatcher(RI);
	RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);

	PARTICLE_ITER_BEGIN(UStreakParticle)

		// Store latest location.
		if( !System->bLockParticles && System->Level->TimeSeconds - System->LastRenderTime > 0.0f )
		{
			for( INT i = Particle->PrevLocations.Num() - 1; i > 0; i-- )
				Particle->PrevLocations(i) = Particle->PrevLocations(i-1);
			Particle->PrevLocations(0) = Particle->Location;
		}

		INT EndLimit = Particle->PrevLocations.Num();
		INT StartLimit = EndLimit-1;

		INT Start=0;
		while( Start < StartLimit && Particle->PrevLocations(Start) == Particle->PrevLocations(Start+1) )
			Start++;

		INT End=Start+1;
		while( End < EndLimit && Particle->PrevLocations(End) != FVector(0,0,0) )
			End++;

		// Get color.
		FColor Color = Particle->Color.Scale(Particle->Alpha,bFadeAlphaOnly);
		for( INT i=Start; i<End-1; i++ )
			LineBatcher.DrawLine(Particle->PrevLocations(i),Particle->PrevLocations(i+1),Color);

		NumParticlesDrawn++;
	
	PARTICLE_ITER_END

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(UStreakParticleTemplate);


//------------------------------------------------------------------------------
void UTexturedStreakParticleTemplate::InitParticle( UParticle* &P )
{
	guard(UTexturedStreakParticleTemplate::InitParticle);
	
	Super::InitParticle( P );

	UTexturedStreakParticle* Particle = ParticleCastChecked<UTexturedStreakParticle>(P);

	Particle->Alpha					= InitialAlpha.GetRand();
	Particle->WidthStart			= WidthStart.GetRand();
	Particle->WidthEnd				= WidthEnd.GetRand();

	Particle->PrevLocations.Empty();
	INT StreakLength = appRandRange( (INT)Length.A, (INT)Length.B );
	if( StreakLength>1 )
	{
		Particle->PrevLocations.AddZeroed( StreakLength );
	}
	else
	{
		Particle->LifeSpan = 0.0f;
	}

	unguard;
}

//------------------------------------------------------------------------------
FColor UTexturedStreakParticleTemplate::GetColor( FLOAT Pct )
{
    guard(UTexturedStreakParticleTemplate::Apply);

	FColor Color;

	INT NumColors = Colors.Num();

	if( NumColors == 0 )
		Color = FColor(0xFFFFFFFF);
	else if( NumColors == 1 || Pct <= 0.f )
		Color = Colors(0);
	else if( Pct >= 1.f )
		Color = Colors.Last();
	else
	{
		FLOAT fIndex = ((FLOAT)(NumColors-1)) * Pct;
		INT i = (INT)fIndex;
		Pct = fIndex - (FLOAT)i;
		if( Pct < 0.05f )
			Color = Colors(i);
		else
			Color = Colors(i).Blend(Colors(i+1),Pct);
	}

	return Color;

	unguard;
}

//------------------------------------------------------------------------------
void UTexturedStreakParticleTemplate::Tick( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UTexturedStreakParticleTemplate::Tick);

	// Update latest location.
	PARTICLE_ITER_BEGIN(UTexturedStreakParticle)

		if( !System->bLockParticles )
		{
			if( Particle->Location != Particle->PrevLocations(0) )
			{
				// push a new point on the front of the stack.
				for( INT i = Particle->PrevLocations.Num() - 1; i > 0; i-- )
					Particle->PrevLocations(i) = Particle->PrevLocations(i-1);
				Particle->PrevLocations(0) = Particle->Location;
			}
			else
			{
				// shrink the streak over time.
				for( INT i = 1; i < Particle->PrevLocations.Num(); i++ )
					if( Particle->PrevLocations(i) != FVector(0,0,0) )
						Particle->PrevLocations(i) += (Particle->PrevLocations(i-1) - Particle->PrevLocations(i)) * DeltaSeconds * 10.0f;
			}
		}
	
	PARTICLE_ITER_END

	unguard;
}

//------------------------------------------------------------------------------
INT UTexturedStreakParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UTexturedStreakParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UTexturedStreakParticleTemplate::Render"),GetFullName()));

	if(!Texture || !ParticleList.Num()) return 0;

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, Texture, Style );

	LOCAL_TO_WORLD
	RI->SetTransform(TT_LocalToWorld,LocalToWorld);

	INT NumParticlesDrawn = 0;

	PARTICLE_ITER_BEGIN(UTexturedStreakParticle)

		INT	NumVerts=0;
		while( NumVerts < Particle->PrevLocations.Num() && Particle->PrevLocations(NumVerts) != FVector(0,0,0) )
			NumVerts++;

		if( NumVerts<2 )	// we need at least two vertices to be drawn.
			continue;

		for( INT i=0; i<NumVerts; i++ )
		{
			FLOAT Pct = (FLOAT)i / (FLOAT)(NumVerts-1);
			QuadPts[i].Location = Particle->PrevLocations(i);
			QuadPts[i].Color = GetColor( Pct ).Scale(Particle->Alpha,bFadeAlphaOnly);
			QuadPts[i].Width = appBlend( Particle->WidthStart, Particle->WidthEnd, Pct );
		}
		static FLOAT Vs[4] = { 0.0f, 1.0f/3.0f, 2.0f/3.0f, 1.0f };
		// Beginning	= 0		->	1/3
		// Middle0		= 1/3	->	2/3
		// Middle1		= 2/3	->	1/3
		// End			= 2/3	->	1
		SP.AddQuadStrip( NumVerts, &Vs[0], ARRAY_COUNT(Vs) );

		NumParticlesDrawn++;
	
	PARTICLE_ITER_END

	SP.Release();

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

//------------------------------------------------------------------------------
UParticleTemplate* UTexturedStreakParticleTemplate::Duplicate( UObject* NewOuter )
{
	guard(UTexturedStreakParticleTemplate::Duplicate);
	NOTE(debugf(TEXT("(%s)UTexturedStreakParticleTemplate::Duplicate( %s )"), GetFullName(), NewOuter ? NewOuter->GetFullName() : TEXT("None") ));

	UTexturedStreakParticleTemplate* NewTemplate = CastChecked<UTexturedStreakParticleTemplate>(Super::Duplicate( NewOuter ));

	// Manually copy over the colors -- since Tim never implemented the copy contructor thingy for DynamicArrayProperties.
	if( NewTemplate )
	{
		NewTemplate->Colors.Empty();
		NewTemplate->Colors.Add( Colors.Num() );
		for( INT i=0; i<Colors.Num(); i++ )
			NewTemplate->Colors(i) = Colors(i);
	}

	return NewTemplate;

	unguard;
}

IMPLEMENT_CLASS(UTexturedStreakParticleTemplate);

//------------------------------------------------------------------------------
void UTriangleParticleTemplate::NotifyAddedToSystem( AParticleGenerator* System )
{
	guard(UTriangleParticleTemplate::NotifyAddedToSystem);
	Super::NotifyAddedToSystem( System );
	UTriangleForce* TriangleForce = (UTriangleForce*)StaticConstructObject( UTriangleForce::StaticClass(), System );
	TriangleForce->AddTemplate( this );
	System->AddForce( TriangleForce );
	unguard;
}

//------------------------------------------------------------------------------
void UTriangleParticleTemplate::InitParticle( UParticle* &P )
{
	guard(UTriangleParticleTemplate::InitParticle);
	
	Super::InitParticle( P );

	UTriangleParticle* Particle = ParticleCastChecked<UTriangleParticle>(P);

	Particle->Texture = Texture;
	Particle->Alpha   = InitialAlpha.GetRand();

	// Fix ARL: Update ResizeForce to account for TriangleParticles.

	if( !AdditionalParticles )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());
		AdditionalParticles = (UParticleTemplate*)StaticConstructObject( UParticleTemplate::StaticClass(), System );
		System->AddTemplate( AdditionalParticles );
	}

	Particle->P1      = AdditionalParticles->GetParticle();
	Particle->P2      = AdditionalParticles->GetParticle();

	// Rules for internal angles of a triangle
	// 1) Must be < 180
	// 2) Any 2 must add up to > 180
	// 3) All three must add up to 360

	FLOAT P01Ang  = bEqualateral ? RADIANS(120) : appRandRange( RADIANS(13), RADIANS(173) );			// Range: 13 to 173
	FLOAT P12Ang  = bEqualateral ? RADIANS(120) : appRandRange( RADIANS(187) - P01Ang, RADIANS(174) );	// Range: 14 to 174
	FLOAT P20Ang  = bEqualateral ? RADIANS(120) : 2*PI - P01Ang - P12Ang;								// Range: 13 to 173

	NOTE(debugf(TEXT("Angles: %d %d %d"), DEG(P01Ang), DEG(P12Ang), DEG(P20Ang) ));

	FLOAT P0Dist  = InitialRadius.GetRand();
	FLOAT P1Dist  = bEqualateral ? P0Dist : InitialRadius.GetRand();
	FLOAT P2Dist  = bEqualateral ? P0Dist : InitialRadius.GetRand();

	// Fix ARL: bLockParticles not accounted for.

	FLOAT InitialAng = appRandRange( 0.f, 2.f*(FLOAT)PI );
	Particle->Location     = FVector(0,appSin(InitialAng       )*P0Dist,appCos(InitialAng       )*P0Dist);
	Particle->P1->Location = FVector(0,appSin(InitialAng+P01Ang)*P1Dist,appCos(InitialAng+P01Ang)*P1Dist);
	Particle->P2->Location = FVector(0,appSin(InitialAng-P20Ang)*P2Dist,appCos(InitialAng-P20Ang)*P2Dist);

	Particle->Velocity     = VRand() * InitialEnergy.GetRand();
	Particle->P1->Velocity = VRand() * InitialEnergy.GetRand();
	Particle->P2->Velocity = VRand() * InitialEnergy.GetRand();

	Particle->P1->LifeSpan = Particle->LifeSpan;
	Particle->P2->LifeSpan = Particle->LifeSpan;

	unguard;
}

//------------------------------------------------------------------------------
void UTriangleParticleTemplate::SetParticleDirection( UParticle* P, FVector Dir )
{
	guard(UTriangleParticleTemplate::SetParticleDirection);
	NOTE(debugf(TEXT("(%s)UTriangleParticleTemplate::SetParticleDirection( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Dir.X, Dir.Y, Dir.Z ));

	UTriangleParticle* Particle = ParticleCastChecked<UTriangleParticle>(P);

	// Update velocities.
	FVector Velocity = InitialSpeed.GetRand() * Dir;
	Particle->Velocity     += Velocity;
	Particle->P1->Velocity += Velocity;
	Particle->P2->Velocity += Velocity;

	// Fix ARL: bLockParticles not accounted for.

	// Point triangle in proper direction.
	FRotator Rot = Dir.Rotation();
	Particle->Location     = Particle->Location    .TransformVectorBy( GMath.UnitCoords * Rot );
	Particle->P1->Location = Particle->P1->Location.TransformVectorBy( GMath.UnitCoords * Rot );
	Particle->P2->Location = Particle->P2->Location.TransformVectorBy( GMath.UnitCoords * Rot );

	unguard;
}

//------------------------------------------------------------------------------
void UTriangleParticleTemplate::SetParticleLocation( UParticle* P, FVector Loc )
{
	guard(UTriangleParticleTemplate::SetParticleLocation);
	NOTE(debugf(TEXT("(%s)UTriangleParticleTemplate::SetParticleLocation( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Loc.X, Loc.Y, Loc.Z ));

	UTriangleParticle* Particle = ParticleCastChecked<UTriangleParticle>(P);

	Particle->Location     += Loc;
	Particle->P1->Location += Loc;
	Particle->P2->Location += Loc;

	unguard;
}

//------------------------------------------------------------------------------
INT UTriangleParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UTriangleParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UTriangleParticleTemplate::Render"),GetFullName()));

	if(!Texture || !ParticleList.Num()) return 0;

	INT NumParticlesDrawn=0;

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, Texture, Style );

	PARTICLE_ITER_BEGIN(UTriangleParticle)

		// Fix ARL: Fix texture coordinates.
		SP.AddTriangle(
			FColor( Particle->Alpha, Particle->Alpha, Particle->Alpha ),
			Particle->Location,
			Particle->P1->Location,
			Particle->P2->Location
			);
		NumParticlesDrawn++;
	
	PARTICLE_ITER_END
	
	SP.Release();

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

// Fix ARL: Recycle helper particles when a TriParticle is recycled?
// (LifeSpan should take care of it)

//------------------------------------------------------------------------------
UParticleTemplate* UTriangleParticleTemplate::Duplicate( UObject* NewOuter )
{
	guard(UTriangleParticleTemplate::Duplicate);

	UTriangleParticleTemplate* NewTemplate = CastChecked<UTriangleParticleTemplate>(Super::Duplicate( NewOuter ));

	if( NewTemplate && NewTemplate->AdditionalParticles )
	{
		if( NewTemplate->AdditionalParticles->IsIn( NewTemplate ) )
			NewTemplate->AdditionalParticles->DeleteUObject();
		NewTemplate->AdditionalParticles = NewTemplate->AdditionalParticles->Duplicate( NewOuter );
	}

	return NewTemplate;

	unguard;
}

IMPLEMENT_CLASS(UTriangleParticleTemplate);

//------------------------------------------------------------------------------
void UTriParticleTemplate::NotifyAddedToSystem( AParticleGenerator* System )
{
	guard(UTriParticleTemplate::NotifyAddedToSystem);
	Super::NotifyAddedToSystem( System );
	UTriForce* TriForce = CastChecked<UTriForce>(StaticConstructObject( UTriForce::StaticClass(), System ));
	TriForce->AddTemplate( this );
	System->AddForce( TriForce );
	unguard;
}

//------------------------------------------------------------------------------
void UTriParticleTemplate::InitParticle( UParticle* &P )
{
	guard(UTriParticleTemplate::InitParticle);
	
	Super::InitParticle( P );

	UTriParticle* Particle = ParticleCastChecked<UTriParticle>(P);

	Particle->Texture = Texture;
	Particle->Alpha   = InitialAlpha.GetRand();

	// Fix ARL: Update FadeForce to account for TriParticles.
	// Fix ARL: Update ResizeForce to account for TriParticles.

	Particle->P0Dist  = InitialRadius.GetRand();
	Particle->P1Dist  = bEqualateral ? Particle->P0Dist : InitialRadius.GetRand();
	Particle->P2Dist  = bEqualateral ? Particle->P0Dist : InitialRadius.GetRand();

	// Rules for internal angles of a triangle
	// 1) Must be < 180
	// 2) Any 2 must add up to > 180
	// 3) All three must add up to 360

	Particle->P01Ang  = bEqualateral ? RADIANS(120) : appRandRange( RADIANS(13), RADIANS(173) );						// Range: 13 to 173
	Particle->P12Ang  = bEqualateral ? RADIANS(120) : appRandRange( RADIANS(187) - Particle->P01Ang, RADIANS(174) );	// Range: 14 to 174
	Particle->P20Ang  = bEqualateral ? RADIANS(120) : 2*PI - Particle->P01Ang - Particle->P12Ang;						// Range: 13 to 173

	NOTE(debugf(TEXT("Angles: %d %d %d"), DEG(Particle->P01Ang), DEG(Particle->P12Ang), DEG(Particle->P20Ang) ));

	if( !AdditionalParticles )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());
		AdditionalParticles = CastChecked<UParticleTemplate>(StaticConstructObject( UParticleTemplate::StaticClass(), System ));
		System->AddTemplate( AdditionalParticles );
	}

	Particle->P1      = AdditionalParticles->GetParticle();
	Particle->P2      = AdditionalParticles->GetParticle();

	Particle->Velocity     = VRand() * InitialEnergy.GetRand();
	Particle->P1->Velocity = VRand() * InitialEnergy.GetRand();
	Particle->P2->Velocity = VRand() * InitialEnergy.GetRand();

	Particle->P1->LifeSpan = Particle->LifeSpan;
	Particle->P2->LifeSpan = Particle->LifeSpan;

	unguard;
}

//------------------------------------------------------------------------------
void UTriParticleTemplate::SetParticleDirection( UParticle* P, FVector Dir )
{
	guard(UTriParticleTemplate::SetParticleDirection);
	NOTE(debugf(TEXT("(%s)UTriParticleTemplate::SetParticleDirection( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Dir.X, Dir.Y, Dir.Z ));

	UTriParticle* Particle = ParticleCastChecked<UTriParticle>(P);

	FVector Velocity = InitialSpeed.GetRand() * Dir;

	Particle->Velocity     += Velocity;
	Particle->P1->Velocity += Velocity;
	Particle->P2->Velocity += Velocity;

	unguard;
}

//------------------------------------------------------------------------------
void UTriParticleTemplate::SetParticleLocation( UParticle* P, FVector Loc )
{
	guard(UTriParticleTemplate::SetParticleLocation);
	NOTE(debugf(TEXT("(%s)UTriParticleTemplate::SetParticleLocation( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Loc.X, Loc.Y, Loc.Z ));

	UTriParticle* Particle = ParticleCastChecked<UTriParticle>(P);

	FRotator Rot = (Particle->Velocity + Particle->P1->Velocity + Particle->P2->Velocity).Rotation();

	FLOAT InitialAng = appRandRange( 0.f, 2.f*(FLOAT)PI );
	Particle->Location     = Loc + FVector(0,appSin(InitialAng                 )*Particle->P0Dist,appCos(InitialAng                 )*Particle->P0Dist).TransformVectorBy( GMath.UnitCoords * Rot );
	Particle->P1->Location = Loc + FVector(0,appSin(InitialAng+Particle->P01Ang)*Particle->P1Dist,appCos(InitialAng+Particle->P01Ang)*Particle->P1Dist).TransformVectorBy( GMath.UnitCoords * Rot );
	Particle->P2->Location = Loc + FVector(0,appSin(InitialAng-Particle->P20Ang)*Particle->P2Dist,appCos(InitialAng-Particle->P20Ang)*Particle->P2Dist).TransformVectorBy( GMath.UnitCoords * Rot );

	unguard;
}

//------------------------------------------------------------------------------
INT UTriParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UTriParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UTriParticleTemplate::Render"),GetFullName()));

	if(!Texture || !ParticleList.Num()) return 0;

	INT NumParticlesDrawn=0;

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, Texture, Style );

	PARTICLE_ITER_BEGIN(UTriParticle)

		SP.AddTriangle(
			Light(Particle,Lights).Scale(Particle->Alpha),
			Particle->Location,
			Particle->P1->Location,
			Particle->P2->Location
			);
			
		NumParticlesDrawn++;
	
	PARTICLE_ITER_END

	SP.Release();

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

// Fix ARL: Recycle helper particles when a TriParticle is recycled?
// (LifeSpan should take care of it)

//------------------------------------------------------------------------------
UParticleTemplate* UTriParticleTemplate::Duplicate( UObject* NewOuter )
{
	guard(UTriParticleTemplate::Duplicate);

	UTriParticleTemplate* NewTemplate = CastChecked<UTriParticleTemplate>(Super::Duplicate( NewOuter ));

	if( NewTemplate && NewTemplate->AdditionalParticles )
	{
		if( NewTemplate->AdditionalParticles->IsIn( NewTemplate ) )
			NewTemplate->AdditionalParticles->DeleteUObject();
		NewTemplate->AdditionalParticles = NewTemplate->AdditionalParticles->Duplicate( NewOuter );
	}

	return NewTemplate;

	unguard;
}

IMPLEMENT_CLASS(UTriParticleTemplate);

//------------------------------------------------------------------------------
void UViewVelocityScaledParticleTemplate::InitParticle( UParticle* &P )
{
	guard(UViewVelocityScaledParticleTemplate::InitParticle);
	
	Super::InitParticle( P );

	UViewVelocityScaledParticle* Particle = ParticleCastChecked<UViewVelocityScaledParticle>(P);

	Particle->ScaleFactor = ScaleFactor.GetRand();
	Particle->StretchOffset = StretchOffset.GetRand();

	unguard;
}

//------------------------------------------------------------------------------
INT UViewVelocityScaledParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UViewVelocityScaledParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UViewVelocityScaledParticleTemplate::Render"),GetFullName()));

	INT NumParticlesDrawn = 0;

	if( ParticleList.Num() && (Sprite || SpriteArray) ) { //goto Skip

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, SpriteArray ? SpriteArray : Sprite, Style );

	RI->SetTransform(TT_WorldToCamera,FMatrix::Identity);
	RI->SetTransform(TT_CameraToScreen,FMatrix::Identity);

	LOCAL_TO_WORLD

	PARTICLE_ITER_BEGIN(UViewVelocityScaledParticle)

		if( Particle->Alpha > 0.f && Particle->Size > 0.f )
		{
			FColor Color = Light(Particle,Lights).Scale( Particle->Alpha, bFadeAlphaOnly );

			LOCAL_LOC

			FVector LocalVelocity = System->bLockParticles ? LocalToWorld.TransformNormal(Particle->Velocity) : Particle->Velocity;

			// Get velocity in projected screen-space.
			FVector ScaledVelocity = LocalVelocity * Particle->ScaleFactor;
			FVector ParticleLoc = LocalLoc + (ScaledVelocity * Particle->StretchOffset);
			FPlane Projected0 = Frame->Project( ParticleLoc + ScaledVelocity );
			FPlane Projected1 = Frame->Project( ParticleLoc - ScaledVelocity );
			FLOAT DirX = Projected1.X - Projected0.X;
			FLOAT DirY = Projected1.Y - Projected0.Y;

			// Get width and height projected in screen-space.
			FLOAT Size = Particle->Size;
			FVector Diag = Frame->CameraToWorld.TransformNormal( FVector(Size,Size,0) );
			FPlane Corner0 = Frame->Project( ParticleLoc - Diag );
			FPlane Corner1 = Frame->Project( ParticleLoc + Diag );
			FLOAT DiagX = Corner1.X - Corner0.X;
			FLOAT DiagY = Corner1.Y - Corner0.Y;

			// Calculate UV direction vectors for the polygon.
			FLOAT Mag = appSqrt( DirX*DirX + DirY*DirY );
			FLOAT VDirX=0, VDirY=1, UDirX=1, UDirY=0;
			if( Mag>0 )
			{
				// normalize.
				FLOAT RMag = 1.f / Mag;
				VDirX = DirX * RMag;
				VDirY = DirY * RMag;
				// perpendicular.
				UDirX = -VDirY;
				UDirY = VDirX;
			}
			UDirX *= DiagX;
			UDirY *= DiagX; // yes X!
			Mag = Min( Mag, MaxStretch / 1024.f );	// are you feeling frisky?
			VDirX = (VDirX * DiagY) + (VDirX * Mag);
			VDirY = (VDirY * DiagY) + (VDirY * Mag);

			FPlane Quad0 = Frame->Project(ParticleLoc);

			// Fix ARL: Probably should adjust z value for proper clipping.
			// We could grab these above where DirX and DirY is calculated.

			FPlane Quad1 = Quad0;
			FPlane Quad2 = Quad0;
			FPlane Quad3 = Quad0;

			Quad0.X += (-UDirX +VDirX);
			Quad0.Y += (-UDirY +VDirY);
			Quad1.X += (+UDirX +VDirX);
			Quad1.Y += (+UDirY +VDirY);
			Quad2.X += (+UDirX -VDirX);
			Quad2.Y += (+UDirY -VDirY);
			Quad3.X += (-UDirX -VDirX);
			Quad3.Y += (-UDirY -VDirY);

			SP.AddQuad(
				Color,
				Quad0,
				Quad1,
				Quad2,
				Quad3,
				SpriteArray ? SpriteArray->GetElementUV(Particle->GetSpriteIndex(this)) : FRectangle(0,0,1,1)
				);

			NumParticlesDrawn++;
		}
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NumParticlesDrawn += DrawCoronas(Owner,Frame,Lights,RI,System);

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

//------------------------------------------------------------------------------
INT UViewVelocityScaledParticleTemplate::DrawCoronas( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UViewVelocityScaledParticleTemplate::DrawCoronas);

	INT NumParticlesDrawn = 0;

	if( ParticleList.Num() && (Corona || CoronaArray) ) { //goto Skip

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, CoronaArray ? CoronaArray : Corona, CoronaStyle, 0 );

	RI->SetTransform(TT_WorldToCamera,FMatrix::Identity);
	RI->SetTransform(TT_CameraToScreen,FMatrix::Identity);

	LOCAL_TO_WORLD

	PROJECTIONS

	PARTICLE_ITER_BEGIN(UViewVelocityScaledParticle)

		if( Particle->Alpha>0.f && Particle->Size>0.f )
		{
			LOCAL_LOC

			FVector LocalVelocity = System->bLockParticles ? LocalToWorld.TransformNormal(Particle->Velocity) : Particle->Velocity;

			if( ((LocalLoc - Frame->ViewOrigin) | ProjFront) > 0.f )
			{
				if( IsVisible( System, Particle, LocalLoc, Frame ) )
				{
					FColor Color = Light(Particle,Lights).Scale( Particle->Alpha * CoronaScaleGlow, bCoronaFadeAlphaOnly );

					// Get velocity in projected screen-space.
					FVector ScaledVelocity = LocalVelocity * Particle->ScaleFactor;
					FVector ParticleLoc = LocalLoc + (ScaledVelocity * Particle->StretchOffset);
					FPlane Projected0 = Frame->Project( ParticleLoc + ScaledVelocity );
					FPlane Projected1 = Frame->Project( ParticleLoc - ScaledVelocity );
					FLOAT DirX = Projected1.X - Projected0.X;
					FLOAT DirY = Projected1.Y - Projected0.Y;

					// Get width and height projected in screen-space.
					FLOAT Size = Particle->Size * CoronaScale;
					FVector Diag = Frame->CameraToWorld.TransformNormal( FVector(Size,Size,0) );
					FPlane Corner0 = Frame->Project( ParticleLoc - Diag );
					FPlane Corner1 = Frame->Project( ParticleLoc + Diag );
					FLOAT DiagX = Corner1.X - Corner0.X;
					FLOAT DiagY = Corner1.Y - Corner0.Y;

					// Calculate UV direction vectors for the polygon.
					FLOAT Mag = appSqrt( DirX*DirX + DirY*DirY );
					FLOAT VDirX=0, VDirY=1, UDirX=1, UDirY=0;
					if( Mag>0 )
					{
						// normalize.
						FLOAT RMag = 1.f / Mag;
						VDirX = DirX * RMag;
						VDirY = DirY * RMag;
						// perpendicular.
						UDirX = -VDirY;
						UDirY = VDirX;
					}
					UDirX *= DiagX;
					UDirY *= DiagX; // yes X!
					Mag = Min( Mag, MaxStretch / 1024.f );	// are you feeling frisky?
					VDirX = (VDirX * DiagY) + (VDirX * Mag);
					VDirY = (VDirY * DiagY) + (VDirY * Mag);

					FPlane Quad0 = Frame->Project(ParticleLoc);

					// Fix ARL: Probably should adjust z value for proper clipping.
					// We could grab these above where DirX and DirY is calculated.

					FPlane Quad1 = Quad0;
					FPlane Quad2 = Quad0;
					FPlane Quad3 = Quad0;

					Quad0.X += (-UDirX +VDirX);
					Quad0.Y += (-UDirY +VDirY);
					Quad1.X += (+UDirX +VDirX);
					Quad1.Y += (+UDirY +VDirY);
					Quad2.X += (+UDirX -VDirX);
					Quad2.Y += (+UDirY -VDirY);
					Quad3.X += (-UDirX -VDirX);
					Quad3.Y += (-UDirY -VDirY);

					SP.AddQuad(
						Color,
						Quad0,
						Quad1,
						Quad2,
						Quad3,
						CoronaArray ? CoronaArray->GetElementUV(Particle->GetCoronaIndex(this)) : FRectangle(0,0,1,1)
						);

					NumParticlesDrawn++;
				}
			}
		}
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s' [CORONAS]"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(UViewVelocityScaledParticleTemplate);

//------------------------------------------------------------------------------
INT UWetWindshildParticleTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UWetWindshildParticleTemplate::Render);
	NOTE(debugf(TEXT("(%s)UWetWindshildParticleTemplate::Render"),GetFullName()));

	INT NumParticlesDrawn = 0;

	if( ParticleList.Num() && (Sprite || SpriteArray) ) { //goto Skip

	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, SpriteArray ? SpriteArray : Sprite, Style );

	LOCAL_TO_WORLD

	PROJECTIONS

	INT USize = SpriteArray ? SpriteArray->GetUSize() : Sprite->MaterialUSize();
	INT VSize = SpriteArray ? SpriteArray->GetVSize() : Sprite->MaterialVSize();

	USize = ( USize >> 1 );
	VSize = ( VSize >> 1 );

	PARTICLE_ITER_BEGIN(USpriteParticle)

		if( Particle->Alpha > 0.f && Particle->Size > 0.f )
		{
			// Fix ARL: Account for SpriteProjForward?
			FColor Color = Light(Particle,Lights).Scale( Particle->Alpha, bFadeAlphaOnly );

			LOCAL_LOC

			FVector Diff = LocalLoc - Frame->ViewOrigin;

			FVector YDir = Particle->Size * VSize * (ProjFront ^ Diff).SafeNormal();
			FVector XDir = Particle->Size * USize * (Diff ^ YDir).SafeNormal();

			SP.AddQuad(
				Color,
				LocalLoc - XDir - YDir,
				LocalLoc - XDir + YDir,
				LocalLoc + XDir + YDir,
				LocalLoc + XDir - YDir,
				SpriteArray ? SpriteArray->GetElementUV(Particle->GetSpriteIndex(this)) : FRectangle(0,0,1,1)
				);

			NumParticlesDrawn++;
		}
	
	PARTICLE_ITER_END

	SP.Release();

		} //Skip

	NumParticlesDrawn += DrawCoronas(Owner,Frame,Lights,RI,System);
	
	NOTE(debugf(TEXT("NumParticlesDrawn: %d - '%s'"),NumParticlesDrawn,GetFullName()));
	return NumParticlesDrawn;
	
	unguard;
}

IMPLEMENT_CLASS(UWetWindshildParticleTemplate);

