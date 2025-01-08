#include "ParticleSystems.h"
#include "FLineManager.h"

//------------------------------------------------------------------------------
void UActorCollisionNotifier::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UActorCollisionNotifier::Apply);

	// optimization: assumes these are only used for calling takedamage.
	if( !bDeleteOnContact && (System->Level->NetMode == NM_Client) )
		return;

	AActor* TraceActor = bIgnoreInstigator ? (AActor*)System->Instigator : System;

	if(1)//if( bUseTrace )		// Fix ARL: Move to a separate force?
	{
		PARTICLE_ITERATOR_BEGIN
		
			FVector Dir = Radius ? (Particle->Location-Particle->LastLocation).SafeNormal()*Radius : FVector(0,0,0);
			FCheckResult Hit(1.0f);
			System->GetLevel()->SingleLineCheck
			(	Hit
			,	TraceActor
			,	Particle->Location + Dir
			,	Particle->LastLocation
			,	TRACE_Pawns | TRACE_Others | TRACE_World | TRACE_OnlyProjActor | TRACE_Blocking
			);

			// NOTE[aleiby]: We have to trace geometry here so you cannot shoot pawns through walls,
			// but we ignore them afterwards since you want geometry colliders to still handle impact effects etc.

			if	(	Hit.Actor
			&&	!(	Hit.Actor->IsA(ALevelInfo::StaticClass())
				||	Hit.Actor->IsA(ATerrainInfo::StaticClass())
				||	Hit.Actor->IsA(AStaticMeshActor::StaticClass())
//				||	Hit.Actor->IsA(AMover::StaticClass())
				))
			{
				if( bDeleteOnContact )
					Particle->LifeSpan = 0.f;
				eventNotifyPenetratingActor( Hit.Actor, 1, Hit.Location );

				NOTE
				(
					debugf(TEXT("UActorCollisionNotifier: %s hit %s"),*Particle->GetClassName(),Hit.Actor->GetName());
					static INT Key = FLineManager::GetInstance()->AllocateLineList(true);
					FLineManager::GetInstance()->AddArrow(
						Key,
						Particle->LastLocation,
						Particle->Location + Dir,
						COLOR_YELLOW );
					FLineManager::GetInstance()->AddArrow(
						Key,
						Particle->LastLocation,
						Hit.Location,
						COLOR_BLUE );
				)
			}

			NOTE(ADDARROW(Particle->LastLocation,Particle->Location + Dir));
		
		PARTICLE_ITERATOR_END
	}
	else
	{
		for( INT i=0; i<System->GetLevel()->Actors.Num(); i++ )
		{
			AActor* Actor = System->GetLevel()->Actors(i);
			INT NumParticles = 0;
			FVector Origin(0,0,0);

			if
			(	Actor
			&&	Actor!=TraceActor
			&&	(Actor->bCollideActors
			||	Actor->bCollideWorld
			||	Actor->bBlockActors
			||	Actor->bBlockPlayers
			||	Actor->bProjTarget)
			)
			{
				if( Actor->CollisionRadius > 0.f && Actor->CollisionHeight > 0.f )
				{
					PARTICLE_ITERATOR_BEGIN

						FVector	Delta = Actor->Location - Particle->Location;

						if
						(	Delta.SizeSquared2D() <= Square(Actor->CollisionRadius + Radius)
						&&	Abs(Delta.Z) <= Actor->CollisionHeight + Radius
						)
						{
							NumParticles++;
							Origin += Particle->Location;
							if( bDeleteOnContact )
								Particle->LifeSpan = 0.f;
						}

					PARTICLE_ITERATOR_END

					if( NumParticles > 0 )
					{
						eventNotifyPenetratingActor( Actor, NumParticles, Origin / (FLOAT)NumParticles );
					}
				}
			}
		}
	}

    unguard;
}

IMPLEMENT_CLASS(UActorCollisionNotifier);

//------------------------------------------------------------------------------
void UAnchorForce::execSetParticle( FFrame& Stack, RESULT_DECL )
{
	guard(UAnchorForce::execSetParticle);

	P_GET_OBJECT(UParticle,A);
	P_GET_ACTOR(R);	
	P_FINISH;

	SetParticle( A, R );
	
	unguard;
}

//------------------------------------------------------------------------------
// Make sure you set the particle's location relative to the RelativeActor
// before calling this function.
// Also make sure bRotateWithActor is set appropriately.  If you change it, 
// you'll have to call this function again for the offsets to work the way 
// you expect.
//------------------------------------------------------------------------------
void UAnchorForce::SetParticle( UParticle* A, AActor* R )
{
	guard(UAnchorForce::SetParticle);

	ParticleA = A;
	RelativeActor = R;

	if( ParticleA && RelativeActor )
	{
		if( !bRotateWithActor )
		{
			RelativeOffset = ParticleA->Location - RelativeActor->Location;
		}
		else
		{
			 RelativeOffset = (ParticleA->Location - RelativeActor->Location).TransformVectorBy( GMath.UnitCoords / RelativeActor->Rotation );
		}
	}

    unguard;
}

//------------------------------------------------------------------------------
void UAnchorForce::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P )
{
	guard(UAnchorForce::NotifyParticleDestroyed);
	NOTE(debugf(TEXT("(%s)UAnchorForce::NotifyParticleDestroyed( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), P ? *P->GetClassName() : TEXT("NULL") ));

	if( P == ParticleA )
	{
		// Fix ARL: Set ParticleA to NULL?
		DeleteUObject();	// This will cause the anchor to remove itself.
	}

	unguard;
}

//------------------------------------------------------------------------------
void UAnchorForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UAnchorForce::Apply);
	NOTE(debugf(TEXT("(%s)UAnchorForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

	if( ParticleA && RelativeActor )
	{
		if( !bRotateWithActor )
		{
			ParticleA->Location = RelativeActor->Location + RelativeOffset;
		}
		else
		{
			ParticleA->Location = RelativeActor->Location + RelativeOffset.TransformVectorBy( GMath.UnitCoords * RelativeActor->Rotation );
		}

		ParticleA->LastLocation = ParticleA->Location;
		// Fix ARL: Zero out Velocity as well?
	}

    unguard;
}

//------------------------------------------------------------------------------
void UAnchorForce::Serialize( FArchive &Ar )
{
	guard(UAnchorForce::Serialize);

	Super::Serialize(Ar);

	if( !(Ar.IsLoading() && UClient::GetDefaultSettings()->ParticleSafeMode==3) )
		Ar << ParticleA;

	unguard;
}

IMPLEMENT_CLASS(UAnchorForce);

//------------------------------------------------------------------------------
void UAttractionForce::execSetEndpoints( FFrame& Stack, RESULT_DECL )
{
	guard(UAttractionForce::execSetEndpoints);

	P_GET_OBJECT(UParticle,A);
	P_GET_OBJECT(UParticle,B);
	P_FINISH;

	SetEndpoints( A, B );
	
	unguard;
}

//------------------------------------------------------------------------------
void UAttractionForce::SetEndpoints( UParticle* A, UParticle* B )
{
	guard(UAttractionForce::SetEndpoints);
	
	check(A!=NULL);
	check(B!=NULL);

	ParticleA = A;
	ParticleB = B;

	unguard;
}

//------------------------------------------------------------------------------
void UAttractionForce::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P )
{
	guard(UAttractionForce::NotifyParticleDestroyedEx);
	NOTE(debugf(TEXT("(%s)UAttractionForce::NotifyParticleDestroyedEx( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), P ? *P->GetClassName() : TEXT("NULL") ));

	if( P == ParticleA || P == ParticleB )
	{
		DeleteUObject();	// This will cause the spring to remove itself.
	}

	unguard;
}

//------------------------------------------------------------------------------
void UAttractionForce::Destroy()
{
	guard(UAttractionForce::Destroy);

	if( ParticleA )
		ParticleA = NULL;

	if( ParticleB )
		ParticleB = NULL;

	Super::Destroy();

	unguard;
}

//------------------------------------------------------------------------------
void UAttractionForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UAttractionForce::Apply);
	NOTE(debugf(TEXT("(%s)UAttractionForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

	// safecheck.
	if( !ParticleA || !ParticleB ) return;

	ParticleA->LastLocation = ParticleA->Location;
	ParticleB->LastLocation = ParticleB->Location;

	if( Rigidity > 0.0f )
	{
		FVector Diff = ParticleA->Location - ParticleB->Location;
		FVector Delta = Diff * DeltaSeconds / Rigidity;
		Diff /= 2.0f;

		if( Delta.SizeSquared() > Diff.SizeSquared() )	// do we really want to be clamping this?
			Delta = Diff;

		ParticleA->Location -= Delta;
		ParticleB->Location += Delta;
	}
	else	// linear
	{
		FVector Dir = (ParticleA->Location - ParticleB->Location).SafeNormal();
		FVector Delta = Dir * DeltaSeconds * Rigidity;

		ParticleA->Location += Delta;
		ParticleB->Location -= Delta;
	}

    unguard;
}

//------------------------------------------------------------------------------
void UAttractionForce::Serialize( FArchive &Ar )
{
	guard(UAttractionForce::Serialize);

	Super::Serialize(Ar);

	if( !(Ar.IsLoading() && UClient::GetDefaultSettings()->ParticleSafeMode==3) )
		Ar << ParticleA << ParticleB;

	unguard;
}

IMPLEMENT_CLASS(UAttractionForce);

//------------------------------------------------------------------------------
void UBlowerForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UBlowerForce::Apply);
	NOTE(debugf(TEXT("UBlowerForce::Apply")));

	AParticleBlower* Blower = CastChecked<AParticleBlower>(System);
	if( !Blower->bOn ) return;

	FLOAT RadiusSquaredMax = Blower->RadiusMax * Blower->RadiusMax;
	FLOAT RadiusSquaredMin = Blower->RadiusMin * Blower->RadiusMin;
	FLOAT DestroyRadiusSquared = Blower->DestroyRadius * Blower->DestroyRadius;

	FLOAT RadiusDiff = Blower->RadiusMax - Blower->RadiusMin;

	FLOAT MaxSpeedSquared = Blower->MaxSpeed * Blower->MaxSpeed;

	FLOAT CosTheta = appCos( RADIANS(Blower->Spread) );
	FVector Rot = Blower->Rotation.Vector();

	PARTICLE_ITERATOR_BEGIN

		FVector DeltaVect = Particle->Location - Blower->Location;
		FLOAT DistSquared = DeltaVect.SizeSquared();

		if( DistSquared <= RadiusSquaredMax && DistSquared >= RadiusSquaredMin )
		{
			FVector Dir = DeltaVect.SafeNormal();
			FLOAT DeltaTheta = Rot | Dir;
			if( DeltaTheta >= CosTheta )
			{
				FLOAT Pct = (appSqrt(DistSquared) - Blower->RadiusMin) / RadiusDiff;
				Particle->Velocity += Dir * appBlend( Blower->AccelMin, Blower->AccelMax, Pct ) * DeltaTime;
				if( Blower->MaxSpeed > 0.f )
				{
					FLOAT ParticleSpeedSquared = Particle->Velocity.SizeSquared();
					if( ParticleSpeedSquared > MaxSpeedSquared )
						Particle->Velocity *= appSqrt(MaxSpeedSquared / ParticleSpeedSquared);
				}
			}
		}

		// Fix ARL: Take location update into account?
		// Fix ARL: Do a quick trace to see if velocity will cross through DestroyRadius?
		if( DestroyRadiusSquared > 0.f && DistSquared <= DestroyRadiusSquared )
		{
			Particle->LifeSpan = 0.f;
		}

	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UBlowerForce);

//------------------------------------------------------------------------------
void UBoneLocatorForce::execSetParticle( FFrame& Stack, RESULT_DECL )
{
	guard(UBoneLocatorForce::execSetParticle);

	P_GET_OBJECT(UParticle,A);
	P_FINISH;

	SetParticle(A);
	
	unguard;
}

//------------------------------------------------------------------------------
void UBoneLocatorForce::SetParticle( UParticle* A )
{
	guard(UBoneLocatorForce::SetParticle);

	ParticleA = A;

    unguard;
}

//------------------------------------------------------------------------------
void UBoneLocatorForce::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P )
{
	guard(UBoneLocatorForce::NotifyParticleDestroyed);
	NOTE(debugf(TEXT("(%s)UBoneLocatorForce::NotifyParticleDestroyed( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), P ? *P->GetClassName() : TEXT("NULL") ));

	if( P == ParticleA )
	{
		// Fix ARL: Set ParticleA to NULL?
		DeleteUObject();	// This will cause the anchor to remove itself.
	}

	unguard;
}

//!!MERGE #include "UnLegendMesh.h"

#define BONE_ERROR(err) return;
//#define BONE_ERROR(err) { debugf(TEXT("(%s) %s: %s"), GetFullName(), Mesh ? Mesh->GetFullName() : TEXT("NULL"), TEXT(err)); return; }

//------------------------------------------------------------------------------
void UBoneLocatorForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UBoneLocatorForce::Apply);
	NOTE(debugf(TEXT("(%s)UBoneLocatorForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));
/*!!MERGE
	if(!ParticleA) BONE_ERROR("Particle not set.");

	AParticleSkeleton* Parent = CastChecked<AParticleSkeleton>(System);

	AActor* Owner = Parent->MeshOwner ? Parent->MeshOwner : Parent;

	ULegendMesh* Mesh = Cast<ULegendMesh>(Owner->Mesh);
	if(!Mesh) BONE_ERROR("Mesh not set");
	
	ULegendMeshInstance* MeshInstance = CastChecked<ULegendMeshInstance>(Mesh->MeshGetInstance(Owner));
	if(!MeshInstance) BONE_ERROR("MeshInstance not set");

	GLM::REntEntity* Entity = MeshInstance->GetEntity();
	if(!Entity) BONE_ERROR("Entity not set");

	BoneIndex = BoneIndex % Entity->EntityGetNodeCount();

	ParticleA->LifeSpan -= DeltaSeconds;
	if( ParticleA->LifeSpan < 0.0f )
	{
		ParticleA->LifeSpan += Parent->BoneSelectionTime.GetRand();
		if( Parent->bRandomBoneSelection )
			BoneIndex = appRand() % Entity->EntityGetNodeCount();
		else
		{
			INT BoneDist = appRandRange( Abs<INT>(Parent->BoneDist.A), Abs<INT>(Parent->BoneDist.B) );
			if( appFrand() > 0.5f )
				BoneDist = -BoneDist;

TraverseOtherDirection:

			if( BoneDist > 0 )	// down the tree.
			{
				while( BoneDist )
				{
					BoneDist--;

					GLM::REntNode* Bone = Entity->EntityGetNode(BoneIndex,false);
					if(!Bone) BONE_ERROR("Invalid bone index");

					GLM::REntNode* ChildBone = NULL;
					if( Bone->TreeHasChildren() )
					{
						INT ChildIndex = appRand() % Bone->TreeGetChildCount();
						ChildBone = Bone->TreeGetChild(ChildIndex);
					}
					if(!ChildBone)
					{
						BoneDist = -BoneDist;
						goto TraverseOtherDirection;
					}

					BoneIndex = ChildBone->NodeGetIndex();
				}
			}
			else if( BoneDist < 0 )	// up the tree.
			{
				while( BoneDist )
				{
					BoneDist++;

					GLM::REntNode* Bone = Entity->EntityGetNode(BoneIndex,false);
					if(!Bone) BONE_ERROR("Invalid bone index");

					GLM::REntNode* ParentBone = Bone->TreeGetParent();
					if(!ParentBone)
					{
						BoneDist = -BoneDist;
						goto TraverseOtherDirection;
					}

					BoneIndex = ParentBone->NodeGetIndex();
				}
			}
		}
	}

	GLM::REntNode* Bone = Entity->EntityGetNode(BoneIndex,false);
	if(!Bone) BONE_ERROR("Invalid bone index");

	GLM::REntNode* DefBone = Entity->EntityGetNode(BoneIndex,true);
	if(!DefBone) BONE_ERROR("Bone is a satellite or other runtime-created node");

	GLM::VCoords3 BoneCoords = Bone->NodeGetCoords(true);
	GLM::VVec3 Pivot = BoneCoords.t << Entity->EntityGetOriginCoords();

	FCoords MeshCoords = GMath.UnitCoords * (Owner->Location + Owner->PrePivot) * Owner->Rotation * FScale(Owner->DrawScale3D * Owner->DrawScale, 0.f, SHEER_None);

	ParticleA->LastLocation = ParticleA->Location;
	ParticleA->Location = FVector(Pivot.z, -Pivot.x, Pivot.y).TransformPointBy(MeshCoords);
*/
    unguard;
}

//------------------------------------------------------------------------------
void UBoneLocatorForce::Serialize( FArchive &Ar )
{
	guard(UBoneLocatorForce::Serialize);

	Super::Serialize(Ar);

	if( !(Ar.IsLoading() && UClient::GetDefaultSettings()->ParticleSafeMode==3) )
		Ar << ParticleA;

	unguard;
}

IMPLEMENT_CLASS(UBoneLocatorForce);

//------------------------------------------------------------------------------
void UCollisionDiscForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UCollisionDiscForce::Apply);
	check(Parent!=NULL);	//!!Slow

	FVector HitNormal = Parent->Rotation.Vector();

	PARTICLE_ITERATOR_BEGIN
		FVector HitLocation;
		if
		(	Parent->CollisionRadius > 0.f
		&&	DiscRayTrace( Parent->Location, HitNormal, Parent->CollisionRadius, Particle->LastLocation, Particle->Location, HitLocation )
		)
		{
			Particle->Velocity -= 2 * HitNormal * (Particle->Velocity | HitNormal) * Parent->Elasticity;	// Reflect velocity.
			FVector r = Particle->Velocity.SafeNormal() * Parent->Elasticity;
			Particle->Location     = HitLocation + r * (Particle->Location - HitLocation).Size();
			Particle->LastLocation = HitLocation - r * (HitLocation - Particle->LastLocation).Size();
		}
    PARTICLE_ITERATOR_END

    unguard;
}

// Fix ARL: Generalize and propogate to Epic's official build.

#define RAYEPS 1e-07						// Roundoff error tolerance.
//------------------------------------------------------------------------------
UBOOL UCollisionDiscForce::DiscRayTrace( FVector Origin, FVector Normal, FLOAT Radius, FVector P1, FVector P2, FVector& P )
{
	guard(UCollisionDiscForce::DiscRayTrace);

	FLOAT P1Face = ((P1 - Origin) | Normal);

	if( Abs(P1Face) < RAYEPS )	// On the surface.
	{
		P = P1;
		return 1;
	}

	FLOAT P2Face = ((P2 - Origin) | Normal);

	if( Abs(P2Face) < RAYEPS )	// On the surface.
	{
		P = P2;
		return 1;
	}

	if
	(	(P1Face > 0.f && P2Face < 0.f)
	||	(P1Face < 0.f && P2Face > 0.f)
	)
	{
		FLOAT t = Abs(P1Face) / Abs(P2Face);
		P = P1 + (P2 - P1).SafeNormal() * t;
		if( Radius == 0.f || (P - Origin).Size() <= Radius )
		{
			return 1;
		}
	}

	return 0;

	unguard;
}

IMPLEMENT_CLASS(UCollisionDiscForce);

//------------------------------------------------------------------------------
void UCollisionSphereForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UCollisionSphereForce::Apply);
	check(Parent!=NULL);	//!!Slow

	PARTICLE_ITERATOR_BEGIN
		FVector HitLocation;
		if
		(	Parent->CollisionRadius > 0.f
		&&	SphereRayTrace( Parent->Location, Parent->CollisionRadius, Particle->LastLocation, Particle->Location, HitLocation )
		)
		{
			FVector HitNormal = (HitLocation - Parent->Location).SafeNormal();
			Particle->Velocity -= 2 * HitNormal * (Particle->Velocity | HitNormal) * Parent->Elasticity;	// Reflect velocity.
			FVector r = Particle->Velocity.SafeNormal() * Parent->Elasticity;
			Particle->Location     = HitLocation + r * (Particle->Location - HitLocation).Size();
			Particle->LastLocation = HitLocation - r * (HitLocation - Particle->LastLocation).Size();
		}
    PARTICLE_ITERATOR_END

    unguard;
}

// Fix ARL: Generalize and propogate to Epic's official build.

#define RAYEPS 1e-07						// Roundoff error tolerance.
//------------------------------------------------------------------------------
UBOOL UCollisionSphereForce::SphereRayTrace( FVector Origin, FLOAT Radius, FVector P1, FVector P2, FVector& P )
{
	guard(UCollisionSphereForce::SphereRayTrace);
//	debugf(TEXT("SphereRayTrace( Origin=(%f,%f,%f), Radius=%f, P1=(%f,%f,%f), P2=(%f,%f,%f) )"), Origin.X, Origin.Y, Origin.Z, Radius, P1.X, P1.Y, P1.Z, P2.X, P2.Y, P2.Z );

	FLOAT rsq = Radius * Radius;			// Radius squared.
	FVector D = (P2 - P1).SafeNormal();		// Direction of ray.
	FVector V = (Origin - P1);				// Ray from ray origin to sphere center.
	FLOAT vsq = V|V;						// Length squared of V.
	FLOAT b   = V|D;						// Perpendicular scale of V.

	if( vsq > rsq && b < RAYEPS )			// Behind ray origin.
		return 0;

	FLOAT disc = b*b - vsq + rsq;			// Discriminate.
	if( disc < 0.f )						// Misses ray.
		return 0;

	disc = appSqrt(disc);					// Find intersection param.
	FLOAT t2 = b + disc;
	FLOAT t1 = b - disc;

	if( t2 <= RAYEPS )						// Behind ray origin.
		return 0;

	if( t1 > (P2 - P1).Size() )				// Beyond endpoint.		// Fix ARL: Optimize
		return 0;

	if( t1 > RAYEPS )						// Entering sphere.
	{
		P = P1 + D*t1;
//		debugf(TEXT("t1=%f disc=%f b=%f VSize=%f"),t1,disc,b,V.Size());
	}
#if 1
	else return 0;
#else
	Pout = P1 + D*t2;						// Exiting sphere.
#endif

	return 1;

	unguard;
}

IMPLEMENT_CLASS(UCollisionSphereForce);


//------------------------------------------------------------------------------
FColor UColorForce::GetColor( FLOAT Pct )
{
    guard(UColorForce::Apply);

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
void UColorForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UColorForce::Apply);

	PARTICLE_ITERATOR_BEGIN
		USpriteParticle* P = ParticleCast<USpriteParticle>(Particle);
		if(P) P->Color = GetColor( 1.0f - (P->LifeSpan / P->InitialLifeSpan) );
    PARTICLE_ITERATOR_END

    unguard;
}

IMPLEMENT_CLASS(UColorForce);


//------------------------------------------------------------------------------
void UDecayForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UDecayForce::Apply);
	NOTE(debugf(TEXT("UDecayForce::Apply")));
	
	// Update lifespans and destroy particles as appropriate.
	TEMPLATE_ITERATOR_BEGIN

		PARTICLE_ITERATOR_SAFE_BEGIN1

			NOTE(debugf(TEXT("Particle = %s"),*Particle->GetClassName()));
			Particle->LifeSpan -= DeltaTime;

			if( Particle->LifeSpan <= 0.0 && !Particle->bChild )
			{
				// Remove from particle list.
				Template->RecycleParticle( Particle );
				Template->ParticleList.Remove(i--);
			}

		PARTICLE_ITERATOR_SAFE_END1

	TEMPLATE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UDecayForce);


//------------------------------------------------------------------------------
void UMinDecayForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UMinDecayForce::Apply);
	NOTE(debugf(TEXT("UMinDecayForce::Apply")));

	// Update lifespans and destroy particles as appropriate.
	TEMPLATE_ITERATOR_BEGIN

		PARTICLE_ITERATOR_SAFE_BEGIN1

			NOTE(debugf(TEXT("Particle = %s"),*Particle->GetClassName()));
			Particle->LifeSpan -= DeltaTime;

			if( Particle->LifeSpan <= 0.0 && !Particle->bChild )
			{
				if( Particle->StasisTimer > 0.0 )				// if particle died on first tick (before rendered even once)...
				{
					FLOAT DeltaDecay = DeltaTime - MinDecay;
					Particle->LifeSpan += DeltaDecay;			// use mindecay instead.
					if( Particle->LifeSpan > 0.0 )				// if that kept us alive...
					{
						Particle->StasisTimer += DeltaDecay;	// update our stasis time so the rest of the forces use the correct delta,
						continue;								// and don't kill particle this tick.
					}
				}

				// Remove from particle list.
				Template->RecycleParticle( Particle );
				Template->ParticleList.Remove(i--);
			}

		PARTICLE_ITERATOR_SAFE_END1

	TEMPLATE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UMinDecayForce);


//------------------------------------------------------------------------------
TArray<UDestroyOnDamage*> UDestroyOnDamage::Instances=TArray<UDestroyOnDamage*>();

//------------------------------------------------------------------------------
UDestroyOnDamage::UDestroyOnDamage()
: UForce()
{
	guard(UDestroyOnDamage::UDestroyOnDamage);
	NOTE(debugf(TEXT("UDestroyOnDamage::UDestroyOnDamage")));

	// Fix ARL: This list should really be stored on a per-level basis and serialized out with the level.
	UDestroyOnDamage::Instances.AddItem(this);

	unguard;
}

//------------------------------------------------------------------------------
void UDestroyOnDamage::Destroy()
{
	guard(UDestroyOnDamage::Destroy);
	NOTE(debugf(TEXT("UDestroyOnDamage::Destroy")));

	UDestroyOnDamage::Instances.RemoveItem(this);
	Super::Destroy();

	unguard;
}

//------------------------------------------------------------------------------
//void UDestroyOnDamage::Notify( FVector Origin, FLOAT DamageRadius, UClass* DamageType )
void UDestroyOnDamage::Notify( AActor* Other, FBox& BBox )
{
	guard(UDestroyOnDamage::Notify);
	NOTE(debugf(TEXT("(%s)UDestroyOnDamage::Notify( %s )"),GetFullName(),GetFullNameSafe(Other)));
/*
	if( DamageRadius<=0.f )
		return;

	if (DamageTypes.Num())
	{
		// Abort if the given DamageType is *not* in our list.
		UBOOL bInList=0;
		for (INT i=0;i<DamageTypes.Num();i++)
			if (DamageTypes(i)==DamageType)	// Fix ARL: Support subclasses? (ClassIsChildOf)
			{
				bInList=1;
				break;
			}
		if (!bInList)
			return;
	}

	AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());
	FLOAT DeltaSeconds = 9999.f;

	// Fix ARL: Add additional early reject based on System's Extents.

	PARTICLE_ITERATOR_BEGIN

		FVector Delta = Particle->Location - Origin;
		FLOAT Dist = Delta.Size();

		FLOAT Size =
			Particle->IsA(USpriteParticle::StaticClass()) ? ((USpriteParticle*)Particle)->Size :
			Particle->IsA(UParticleDecal::StaticClass()) ? ((UParticleDecal*)Particle)->Size : 1.f;

		Size *= Radius;

		NOTE(debugf(TEXT("%f %f %f"),DamageRadius,Size,Dist));

		if ((Size + DamageRadius) >= Dist)
		{
			Particle->LifeSpan = 0.f;
			Particle->StasisTimer = DestroyLifeSpan;
		}

	PARTICLE_ITERATOR_END
*/

	APawn* P = Cast<APawn>(Other);
	if (!P) return;

	APlayerReplicationInfo* PRI = P->PlayerReplicationInfo;
	if (!PRI) return;

	ATeamInfo* Team = PRI->Team;
	if (!Team) return;

	INT TeamIndex = Team->TeamIndex;


	AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());
	FLOAT DeltaSeconds = 9999.f;

	BBox = BBox.ExpandBy(Radius);		// decal template bounding boxes tend to be really thin on coplanar surfaces.
	//GTempLineBatcher->AddBox(BBox, FColor(255,0,0));

	TEMPLATE_ITERATOR_BEGIN

		//GTempLineBatcher->AddBox(Template->Extents, FColor(0,255,0));

		AParticleGenerator* TemplateSystem = CastChecked<AParticleGenerator>(Template->GetOuter());

		APawn* TemplateInstigator = TemplateSystem->Instigator;
		if (!TemplateInstigator) continue;

		APlayerReplicationInfo* TemplatePRI = TemplateInstigator->PlayerReplicationInfo;
		if (!TemplatePRI) continue;

		ATeamInfo* TemplateTeam = TemplatePRI->Team;
		if (!TemplateTeam) continue;

		INT TemplateTeamIndex = TemplateTeam->TeamIndex;
		

		if ((TemplateTeamIndex != TeamIndex) && BBox.Intersect(Template->Extents))
		{
			PARTICLE_ITERATOR_BEGIN1
/*
				FLOAT Size =	// Fix ARL: Someone throw me the polymorphism stick!
					Particle->IsA(USpriteParticle::StaticClass()) ? ((USpriteParticle*)Particle)->Size :
					Particle->IsA(UParticleDecal::StaticClass()) ? ((UParticleDecal*)Particle)->Size : 1.f;

				Size *= Radius;

				FBox PBox = FBox(Particle->Location,Particle->Location).ExpandBy(Size);
				//GTempLineBatcher->AddBox(PBox, FColor(0,0,255));
				if (BBox.Intersect(PBox))
*/
				if (FPointBoxIntersection(Particle->Location,BBox))
				{
					Particle->LifeSpan = 0.f;
					Particle->StasisTimer = DestroyLifeSpan;
				}


			PARTICLE_ITERATOR_END1
		}
		
	TEMPLATE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
void UDestroyOnDamage::execNotify( FFrame& Stack, RESULT_DECL )
{
	guard(UDestroyOnDamage::execNotify);
	NOTE(debugf(TEXT("UDestroyOnDamage::execNotify")));

	P_GET_OBJECT(ULevel,Level);		// NOTE[aleiby]: If we stored per-level lists we wouldn't need this to check against.
	P_GET_VECTOR(Origin);
	P_GET_FLOAT(DamageRadius);
	P_GET_OBJECT(UClass,DamageType);
	P_FINISH;

	//UDestroyOnDamage::StaticNotify( Level, Origin, DamageRadius, DamageType );

	unguard;
}

//------------------------------------------------------------------------------
//void UDestroyOnDamage::StaticNotify( ULevel* Level, FVector Origin, FLOAT DamageRadius, UClass* DamageType )
void UDestroyOnDamage::StaticNotify( AActor* Other )
{
	guard(UDestroyOnDamage::StaticNotify);
	//NOTE(debugf(TEXT("UDestroyOnDamage::StaticNotify: %s %s %f %s"),GetFullNameSafe(Level),*Origin.String(),DamageRadius,GetFullNameSafe(DamageType)));
	NOTE(debugf(TEXT("UDestroyOnDamage::StaticNotify( %s )"),GetFullNameSafe(Other)));
/*
	UObject* Package = Level ? Level->GetOuter() : NULL;
	for (INT i=0;i<UDestroyOnDamage::Instances.Num();i++)
		if (UDestroyOnDamage::Instances(i)->IsIn(Package))
			UDestroyOnDamage::Instances(i)->Notify( Origin, DamageRadius, DamageType );
*/

	UObject* Package = Other->GetOuter();
	FBox BBox = Other->GetPrimitive()->GetCollisionBoundingBox(Other);
	for (INT i=0;i<UDestroyOnDamage::Instances.Num();i++)
	{
		UDestroyOnDamage* Force = UDestroyOnDamage::Instances(i);
		if (Force->IsIn(Package))
			Force->Notify( Other, BBox );
	}

	unguard;
}

IMPLEMENT_CLASS(UDestroyOnDamage);


// Fix ARL: We shouldn't have to add a new ParticleClass check every time we add a new type of particle that can fade.

//------------------------------------------------------------------------------
#define FADEFORCE_APPLY(type) \
	if( Particle->IsA( type::StaticClass() ) ) \
	{ \
		type* P = ParticleCastChecked<type>(Particle); \
		GET_FORCEVARS_CHECKED(UFadeForceVars,Particle); \
		Vars->AlphaTimer += DeltaTime; \
		while( Vars->AlphaTime > 0.f && Vars->AlphaTimer >= Vars->AlphaTime ) \
		{ \
			NOTE(debugf(TEXT("Reversing fade direction for %s"), P->GetClassName() )); \
			Vars->AlphaTimer -= Vars->AlphaTime; \
			Vars->AlphaRate *= -1.f;	/* Reverse fade direction. */ \
		} \
		P->Alpha += Vars->AlphaRate * DeltaTime; \
		NOTE(debugf(TEXT("%s->Alpha = %f (AlphaRate: %f)"), P->GetClassName(), P->Alpha, Vars->AlphaRate )); \
		if( P->Alpha <= 0.f && Vars->AlphaRate <= 0.f && Phase == 1 ) \
			P->LifeSpan = 0.f; \
		continue; \
	}
//------------------------------------------------------------------------------
void UFadeForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UFadeForce::Apply);
	NOTE(debugf(TEXT("(%s)UFadeForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("None"), DeltaSeconds ));
	
	if( Phase == 0 )
		return;

	PARTICLE_ITERATOR_BEGIN

		FADEFORCE_APPLY(USpriteParticle)
		FADEFORCE_APPLY(UTexturedStreakParticle)
		FADEFORCE_APPLY(UPointParticle)
		FADEFORCE_APPLY(UStreakParticle)
		FADEFORCE_APPLY(UTriParticle)
		FADEFORCE_APPLY(UTriangleParticle)
		FADEFORCE_APPLY(UParticleDecal)

	PARTICLE_ITERATOR_END
	
	unguard;
}

//------------------------------------------------------------------------------
void UFadeForce::InitParticle( UParticle* Particle )
{
	guard(UFadeForce::InitParticle);

	if
	(	Particle->IsA( USpriteParticle::StaticClass() )
	||	Particle->IsA( UTexturedStreakParticle::StaticClass() )
	||	Particle->IsA( UPointParticle::StaticClass() )
	||	Particle->IsA( UStreakParticle::StaticClass() )
	||	Particle->IsA( UTriParticle::StaticClass() )
	||	Particle->IsA( UTriangleParticle::StaticClass() )
	||	Particle->IsA( UParticleDecal::StaticClass() )
	)
	{
		ATTACH_FORCEVARS(UFadeForceVars,Particle);
		Vars->AlphaRate = AlphaRate.GetRand();
		Vars->AlphaTime = Phase ? Particle->LifeSpan / (FLOAT)Phase : 0.f;
		Vars->AlphaTimer = 0.f;
		NOTE(debugf(TEXT("(%s) Attached %s to %s | AlphaRate: %f AlphaTime: %f"), GetFullName(), *Vars->GetClassName(), *Particle->GetClassName(), Vars->AlphaRate, Vars->AlphaTime ));
	}

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UFadeForce);
IMPLEMENT_CLASS(UFadeForce);

// Fix ARL: Is there a way to take individual particles out of the "loop" once they reach their FinalAlpha?
// Fix ARL: We shouldn't have to add a new ParticleClass check every time we add a new type of particle that can fade.

//------------------------------------------------------------------------------
#define FADEINFORCE_APPLY(type) \
	if( Particle->IsA( type::StaticClass() ) ) \
	{ \
		type* P = ParticleCastChecked<type>(Particle); \
		GET_FORCEVARS_CHECKED(UFadeInForceVars,Particle); \
		if( Vars->FadeTime > 0 ) \
		{ \
			Vars->FadeTime = Max( Vars->FadeTime - DeltaTime, 0.f ); \
			P->Alpha = Vars->FinalAlpha * (1.f - Vars->FadeTime / Vars->InitialFadeTime); \
			NOTE(debugf(TEXT("%s->Alpha = %f"), *P->GetClassName(), P->Alpha )); \
		} \
		continue; \
	}
//------------------------------------------------------------------------------
void UFadeInForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UFadeInForce::Apply);
	NOTE(debugf(TEXT("(%s)UFadeInForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("None"), DeltaSeconds ));

	PARTICLE_ITERATOR_BEGIN

		FADEINFORCE_APPLY(USpriteParticle)
		FADEINFORCE_APPLY(UTexturedStreakParticle)
		FADEINFORCE_APPLY(UPointParticle)
		FADEINFORCE_APPLY(UStreakParticle)
		FADEINFORCE_APPLY(UTriParticle)
		FADEINFORCE_APPLY(UTriangleParticle)
		FADEINFORCE_APPLY(UParticleDecal)

	PARTICLE_ITERATOR_END
	
	unguard;
}

//------------------------------------------------------------------------------
#define FADEINFORCE_INIT(type) \
	if( Particle->IsA( type::StaticClass() ) ) \
	{ \
		type* P = ParticleCastChecked<type>(Particle); \
		ATTACH_FORCEVARS(UFadeInForceVars,P); \
		Vars->FadeTime = FadeTime.GetRand(); \
		Vars->InitialFadeTime = Vars->FadeTime; \
		Vars->FinalAlpha = P->Alpha; \
		NOTE(debugf(TEXT("(%s) FadeTime: %f InitialFadeTime: %g FinalAlpha: %f"), *P->GetClassName(), Vars->FadeTime, Vars->InitialFadeTime, Vars->FinalAlpha )); \
		return; \
	}
//------------------------------------------------------------------------------
void UFadeInForce::InitParticle( UParticle* Particle )
{
	guard(UFadeInForce::InitParticle);
	
	FADEINFORCE_INIT(USpriteParticle)
	FADEINFORCE_INIT(UTexturedStreakParticle)
	FADEINFORCE_INIT(UPointParticle)
	FADEINFORCE_INIT(UStreakParticle)
	FADEINFORCE_INIT(UTriParticle)
	FADEINFORCE_INIT(UTriangleParticle)
	FADEINFORCE_INIT(UParticleDecal)

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UFadeInForce);
IMPLEMENT_CLASS(UFadeInForce);

// Fix ARL: We shouldn't have to add a new ParticleClass check every time we add a new type of particle that can fade.

//------------------------------------------------------------------------------
#define FADEOUTFORCE_APPLY(type) \
	if( Particle->IsA( type::StaticClass() ) ) \
	{ \
		type* P = ParticleCastChecked<type>(Particle); \
		GET_FORCEVARS_CHECKED(UFadeOutForceVars,Particle); \
		if( P->LifeSpan < Vars->FadeTime ) \
			P->Alpha = Vars->InitialAlpha * P->LifeSpan / Vars->FadeTime; \
		NOTE(debugf(TEXT("%s->Alpha = %f"), *P->GetClassName(), P->Alpha )); \
		continue; \
	}
//------------------------------------------------------------------------------
void UFadeOutForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UFadeOutForce::Apply);
	NOTE(debugf(TEXT("(%s)UFadeOutForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("None"), DeltaSeconds ));

	PARTICLE_ITERATOR_BEGIN

		FADEOUTFORCE_APPLY(USpriteParticle)
		FADEOUTFORCE_APPLY(UTexturedStreakParticle)
		FADEOUTFORCE_APPLY(UPointParticle)
		FADEOUTFORCE_APPLY(UStreakParticle)
		FADEOUTFORCE_APPLY(UTriParticle)
		FADEOUTFORCE_APPLY(UTriangleParticle)
		FADEOUTFORCE_APPLY(UParticleDecal)

	PARTICLE_ITERATOR_END
	
	unguard;
}

//------------------------------------------------------------------------------
#define FADEOUTFORCE_INIT(type) \
	if( Particle->IsA( type::StaticClass() ) ) \
	{ \
		type* P = ParticleCastChecked<type>(Particle); \
		ATTACH_FORCEVARS(UFadeOutForceVars,P); \
		Vars->FadeTime = FadeTime.GetRand(); \
		Vars->InitialAlpha = P->Alpha; \
		NOTE(debugf(TEXT("(%s) FadeTime: %f InitialAlpha: %f"), *P->GetClassName(), Vars->FadeTime, Vars->InitialAlpha )); \
		return; \
	}
//------------------------------------------------------------------------------
void UFadeOutForce::InitParticle( UParticle* Particle )
{
	guard(UFadeOutForce::InitParticle);

	FADEOUTFORCE_INIT(USpriteParticle)
	FADEOUTFORCE_INIT(UTexturedStreakParticle)
	FADEOUTFORCE_INIT(UPointParticle)
	FADEOUTFORCE_INIT(UStreakParticle)
	FADEOUTFORCE_INIT(UTriParticle)
	FADEOUTFORCE_INIT(UTriangleParticle)
	FADEOUTFORCE_INIT(UParticleDecal)

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UFadeOutForce);
IMPLEMENT_CLASS(UFadeOutForce);

//------------------------------------------------------------------------------
void UFrictionForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UFrictionForce::Apply);

	MinVeloSquared = MinVelocity * MinVelocity;

	PARTICLE_ITERATOR_BEGIN
		Particle->Velocity -= Particle->Velocity * Friction * DeltaTime;
		if( MinVeloSquared > 0.f && Particle->Velocity.SizeSquared() <= MinVeloSquared )
			Particle->Velocity = FVector(0,0,0);
	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UFrictionForce);

inline DWORD GetTypeHash( const FVector A )	// assumes normalized vector
{
	// Fix ARL: Account for negatives. (x+1)/2
	INT X=(A.X==0.f)?0:(A.X==1.f)?((1<<11)-1):(INT)(A.X*(FLOAT)(1<<11));
	INT Y=(A.Y==0.f)?0:(A.Y==1.f)?((1<<11)-1):(INT)(A.Y*(FLOAT)(1<<11));
	INT Z=(A.Z==0.f)?0:(A.Z==1.f)?((1<<10)-1):(INT)(A.Z*(FLOAT)(1<<10));
	return (X)+(Y<<11)+(Z<<22);
}
#include "THashTable.h"
typedef THashTable< FVector, UParticle*, 10 > tNormalParticleHash;
typedef THashTable< FName, tNormalParticleHash*, 5 > tNormalParticleHashTypeTable;
static tNormalParticleHashTypeTable NormalParticleHashTypeTable=tNormalParticleHashTypeTable();

// hash particles according to normal.
// distance check new particles with those of same normal.
// verify location delta is perpendicular to normal (coplanar)
// merge particles if close enough
//    destroy existing particles
//    create new particle
//    average locations biased by size
//    combine sizes

//------------------------------------------------------------------------------
void UGeoCollideAndMerge::CollisionNotification( UParticle* P, FCheckResult &Hit, UParticleTemplate* Template )
{
	guard(UGeoCollideAndMerge::CollisionNotification);
	NOTE(debugf(TEXT("UGeoCollideAndMerge::CollisionNotification")));

	// get hash for this type of collider.
	tNormalParticleHash* Hash = NormalParticleHashTypeTable.FindRef( Type );
	if(!Hash)
	{
		Hash = new tNormalParticleHash();
		check(Hash!=NULL); //remove
		NormalParticleHashTypeTable.Set( Type, Hash );
		check(NormalParticleHashTypeTable.FindRef(Type)==Hash); //remove
	}

	FVector Normal=Template->GetParticleNormal(P);
	UBOOL bCollision = false;

	// distance check new particles with those of same normal.
	for( tNormalParticleHash::TCollisionIterator It(*Hash,Normal); It; ++It )
	{
		UParticle* Pi = *It;
		FVector Diff = Pi->Location - P->Location;
		FVector Dir = Diff.SafeNormal();
		FLOAT S = Template->GetParticleSize(P);
		FLOAT Si = Template->GetParticleSize(Pi);

		// verify location delta is perpendicular to normal (coplanar)
		bCollision = (Dir|Normal)<0.0001f && Diff.Size()<(S+Si)/2.f;
		if( bCollision )
		{
			// merge particles if close enough
			//    destroy existing particles
			P->LifeSpan=0.f;
			Pi->LifeSpan=0.f;

			//    create new particle
			UParticle* New = Template->GetParticle();

			//    average locations biased by size
			Template->SetParticleDirection( New, Dir );
			Template->SetParticleLocation( New, (P->Location*S)+(Pi->Location*Si)/(2*(S+Si)) );

			//    combine sizes
			Template->SetParticleSize( New, (S+Si)/2.f );

			// Fix ARL: Not true... what about two existing decals that don't touch, but a third one touches both?  We need to merge all three, not just the first two.
			// (instead of breaking, set P=*It; and continue iterating)
			break;	// theoretically, there will only ever be one real collision since these are added one at a time, checked each time, and don't move or rotate.
		}
	}

	// hash particles according to normal.
	if(!bCollision)
		Hash->Set( Normal, P );		// Fix ARL: Need to keep collisions (currently Value will be overwritten with matching Keys).  Update THashTable accordingly.

	unguard;
}

IMPLEMENT_CLASS(UGeoCollideAndMerge);

//------------------------------------------------------------------------------
void UGeometryCollider::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UGeometryCollider::Apply);
	NOTE(debugf(TEXT("UGeometryCollider::Apply")));

	// Set up for collision notifications.
	INT NumNotifications = NotificationsPerTick;
	UBOOL bUpdateNotificationTimer = 0;

	if( bCollisionNotify )
	{
		if( NotificationTimer > 0.f )
		{
			NotificationTimer -= DeltaSeconds;
			if( NotificationTimer > 0.f )
			{
				NumNotifications = 0;
			}
		}
	}

	AActor* TraceActor = bIgnoreInstigator ? (AActor*)System->Instigator : System;

	PARTICLE_ITERATOR_BEGIN

		if( Particle->Location == Particle->LastLocation )	// No movement == no collision.
			continue;

		FVector Dir = (Particle->Location - Particle->LastLocation).UnsafeNormal();

		FCheckResult Hit(1.0f);
		System->GetLevel()->SingleLineCheck
		(	Hit
		,	TraceActor
		,	Particle->Location
		,	Particle->LastLocation
		,	(bCollideActors ? TRACE_AllBlocking : TRACE_World) | TRACE_Material
		);

		NOTE(ADDARROW(Particle->LastLocation,Hit.Actor?Hit.Location:Particle->Location,Hit.Actor?COLOR_YELLOW:COLOR_BLUE));

		if( !Hit.Actor )
			continue;

		FVector HitLocation = Hit.Location;
		FVector HitNormal   = Hit.Normal.SafeNormal();

		FLOAT Incidence = (-Dir | HitNormal);

		// Fix HitNormal for stuff like glass.
		// Fix ARL: This should really be done deeper in the individual LineChecks of StaticMesh, etc.
		if( Incidence <= 0.0f )
		{
			UShader* Shader = Cast<UShader>(Hit.Material);
			UFinalBlend* FinalBlend = Cast<UFinalBlend>(Hit.Material);
			if( (Shader && Shader->TwoSided) || (FinalBlend && FinalBlend->TwoSided) )
			{
				HitNormal = -HitNormal;
				Incidence = (-Dir | HitNormal);
			}
		}

		if( Incidence <= 0.0f )	// Make sure we're going into geometry, not coming out.
			continue;

		if( bDeleteOnContact && Incidence > MinIncidence )
		{
			Particle->LifeSpan = 0.0f;
			Particle->Location = Hit.Location;
		}
		else if( bStopOnContact && Incidence > MinIncidence )
		{
			Particle->Velocity = FVector(0,0,0);
			Particle->Location = Hit.Location;
		}
		else
		{
			if( bDiffuse )
				Particle->Velocity = RandomSpreadVector(180.0f).SafeNormal().TransformVectorBy( GMath.UnitCoords * HitNormal.Rotation() ) * Particle->Velocity.Size();	// Fix ARL: optimize!   ?
			else
				Particle->Velocity -= 2 * HitNormal * (Particle->Velocity | HitNormal);	// Reflect velocity.

			Particle->Velocity *= Elasticity.GetRand();

			FVector NewDir = Particle->Velocity.SafeNormal();

			Particle->Location     = HitLocation + NewDir * (Particle->Location - HitLocation).Size();
			Particle->LastLocation = HitLocation - NewDir * (HitLocation - Particle->LastLocation).Size();
		}

		// Spawn impact effects.
		//ARL (mdf): if ImpactPercent is 1.0 this has a 1/32767 chance of failing when it pbly shouldn't?
		//NOTE[aleiby]: Don't spawn effects on movers since we have no way of attaching them.
		if( appFrand() < ImpactPercent && !Hit.Actor->IsA(AMover::StaticClass()) )
		{
			for( INT i=0; i<ImpactEffects.Num(); i++ )
			{
				UParticleTemplate* T = ImpactEffects(i);
				if( T )//&& T->SelectionWeight > 0.f && appFrand() >= T->SelectionWeight )
				{
					UParticle* P = T->GetParticle();
					T->SetParticleLocation( P, HitLocation );
					T->SetParticleDirection( P, HitNormal );
				}
			}
/*!!MERGE
			for( INT i=0; i<ProjectorSettings.Num(); i++ )
			{
				FProjSettings &Settings=ProjectorSettings(i);
				AProjector* ProjActor = CastChecked<AProjector>(System->GetLevel()->SpawnActor( LoadClass<AProjector>( NULL, TEXT("ParticleSystems.ParticleDecalProjector"), NULL, LOAD_NoWarn, NULL ), NAME_None, System->Location ));
#define COPY_PROP(p) ProjActor->p=Settings.p;
				COPY_PROP(MaterialBlendingOp)
				COPY_PROP(FrameBufferBlendingOp)
				COPY_PROP(ProjTexture)
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
				ProjActor->SetLocation( HitLocation );
				ProjActor->SetRotation( HitNormal.Rotation() );
				ProjActor->DrawScale = Settings.Size.GetRand();
				ProjActor->ScaleGlow = Settings.Alpha.GetRand();
				ProjActor->InitializeProjector();
			}
*/
			if( Sound && USound::Audio )
				USound::Audio->PlaySound( System, SLOT_None, Sound, HitLocation, Volume.GetRand(), Max( 1.f, Radius.GetRand() ), Pitch.GetRand(), SF_NoUpdates, 0.f );
		}

		// Send impact notifications.
		CollisionNotification( Particle, Hit, Template );	// native version is fast, so we can send all notifications.
		if( bCollisionNotify && NumNotifications > 0 )
		{
			NumNotifications--;
			bUpdateNotificationTimer = 1;
			eventCollisionNotification( Particle, Hit );

		}

	PARTICLE_ITERATOR_END

	if( bUpdateNotificationTimer )
	{
		NotificationTimer += MaxNotificationTime;
	}

	unguard;
}

//------------------------------------------------------------------------------
UForce* UGeometryCollider::Duplicate( UObject* NewOuter )
{
	guard(UGeometryCollider::Duplicate);
	NOTE(debugf(TEXT("(%s)UGeometryCollider::Duplicate( %s )"), GetFullName(), NewOuter ? NewOuter->GetFullName() : TEXT("None") ));

	UGeometryCollider* NewForce = CastChecked<UGeometryCollider>( Super::Duplicate( NewOuter ) );

	// Manually copy the dynamic arrays since Tim isn't doing it for us.
	if( NewForce )
	{
		// Delete existing inner objects first.	
		{TArray<UParticleTemplate*> Remove=NewForce->ImpactEffects;
		for( INT i=0; i<Remove.Num(); i++ )
			if( Remove(i)->IsIn( NewForce ) )
				Remove(i)->DeleteUObject();}

		NewForce->ImpactEffects.Empty();
		NewForce->ImpactEffects.Add( ImpactEffects.Num() );
		for( INT i=0; i<ImpactEffects.Num(); i++ )
			NewForce->ImpactEffects(i) = ImpactEffects(i)->Duplicate( NewOuter );
	}

	return NewForce;

	unguard;
}

//------------------------------------------------------------------------------
void UGeometryCollider::Destroy()
{
	guard(UGeometryCollider::Destroy);
	ImpactEffects.Empty();
	Super::Destroy();
	unguard;
}

//------------------------------------------------------------------------------
FLOAT UGeometryCollider::GetMaxLifeSpan( AParticleGenerator* System )
{
	guard(UGeometryCollider::GetMaxLifeSpan);
	NOTE(debugf(TEXT("UGeometryCollider::GetMaxLifeSpan")));

	FLOAT MaxLifeSpan=0;

	TEMPLATE_ITERATOR_BEGIN

		for( INT i=0; i<ImpactEffects.Num(); i++ )
			MaxLifeSpan = Max( MaxLifeSpan, Template->GetMaxLifeSpan(System) + ImpactEffects(i)->GetMaxLifeSpan(System) );

	TEMPLATE_ITERATOR_END

	NOTE(debugf(TEXT("%s: %f"),GetName(),MaxLifeSpan));

	return MaxLifeSpan;

	unguard;
}

//------------------------------------------------------------------------------
void UGeometryCollider::AddImpactEffect( UParticleTemplate* T )
{
	guard(UGeometryCollider::AddImpactEffect);
	NOTE(debugf(TEXT("(%s)UGeometryCollider::AddImpactEffect( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None")));
	if( T )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>( GetOuter() );
		T->SelectionWeight = 0.f;
		ImpactEffects.AddItem( T );
		System->AddTemplate( T );
	}
	unguard;
}


//------------------------------------------------------------------------------
void UGeometryCollider::execAddImpactEffect( FFrame& Stack, RESULT_DECL )
{
	guard(UGeometryCollider::execAddImpactEffect);
	NOTE(debugf(TEXT("UGeometryCollider::execAddImpactEffect")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	AddImpactEffect( T );
	
	unguard;
}

IMPLEMENT_CLASS(UGeometryCollider);

//------------------------------------------------------------------------------
void UGeometryColliderEx::InitParticle( UParticle* Particle )
{
	guard(UGeometryColliderEx::InitParticle);

	ATTACH_FORCEVARS(UGeometryColliderExVars,Particle);
	Vars->Bounces = NumBounces.GetRand();

	unguard;
}

//------------------------------------------------------------------------------
void UGeometryColliderEx::CollisionNotification( UParticle* Particle, FCheckResult &Hit, UParticleTemplate* Template )
{
	guard(UGeometryColliderEx::CollisionNotification);

	GET_FORCEVARS_CHECKED(UGeometryColliderExVars,Particle);
	if( Vars->Bounces-- == 0 )
	{
		if( bDeleteOnContact )
		{
			Particle->LifeSpan = 0.0f;
			Particle->Location = Hit.Location;
		}
		else if( bStopOnContact )
		{
			Particle->Velocity = FVector(0,0,0);
			Particle->Location = Hit.Location;
		}
	}

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UGeometryColliderEx);
IMPLEMENT_CLASS(UGeometryColliderEx);

//------------------------------------------------------------------------------
void UGlobalAccelerator::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UGlobalAccelerator::Apply);
	NOTE(debugf(TEXT("UGlobalAccelerator::Apply"));)

	PARTICLE_ITERATOR_BEGIN

		if( bUseZoneGravity )
		{
			Particle->Velocity += System->GetLevel()->Model->PointRegion( System->GetLevel()->GetLevelInfo(), Particle->Location ).Zone->PhysicsVolume->Gravity * DeltaTime;
		}
		else
		{
			Particle->Velocity += Force * DeltaTime;
		}

	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UGlobalAccelerator);

//------------------------------------------------------------------------------
void UInertiaForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UInertiaForce::Apply);

	PARTICLE_ITERATOR_BEGIN
		//debugf( TEXT("UInertiaForce::Apply( %s )"), *Particle->GetClassName() );
		GET_FORCEVARS_CHECKED(UInertiaForceVars,Particle);
		Vars->LastVelocity += (Particle->Velocity - Vars->LastVelocity) * Weight;
		Particle->Velocity = Vars->LastVelocity;
    PARTICLE_ITERATOR_END

    unguard;
}

// Fix ARL: Detach our AdditionalVars when we are destroyed.  (This can be automated in the base UForce class.)

//------------------------------------------------------------------------------
void UInertiaForce::InitParticle( UParticle* Particle )
{
	guard(UInertiaForce::InitParticle);
	//debugf( TEXT("UInertiaForce::InitParticle( %s )"), *Particle->GetClassName() );

	ATTACH_FORCEVARS(UInertiaForceVars,Particle);
	Vars->LastVelocity = Particle->Velocity;

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UInertiaForce);
IMPLEMENT_CLASS(UInertiaForce);

//------------------------------------------------------------------------------
#define LIMITEDRESIZEFORCE_APPLY(type) \
	if( Particle->IsA( type::StaticClass() ) ) \
	{ \
		type* P = ParticleCastChecked<type>(Particle); \
		GET_FORCEVARS_CHECKED(ULimitedResizeForceVars,P); \
		P->Size += Vars->SizeRate * DeltaTime; \
		if( P->Size <= 0.f ) \
			P->LifeSpan = 0.f; \
		continue; \
	}
//------------------------------------------------------------------------------
void ULimitedResizeForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(ULimitedResizeForce::Apply);
	NOTE(debugf(TEXT("ULimitedResizeForce::Apply")));

	PARTICLE_ITERATOR_BEGIN

		LIMITEDRESIZEFORCE_APPLY(USpriteParticle)
		LIMITEDRESIZEFORCE_APPLY(UParticleDecal)

	PARTICLE_ITERATOR_END
	
	unguard;
}

//------------------------------------------------------------------------------
#define LIMITEDRESIZEFORCE_INIT(type) \
	if( Particle->IsA( type::StaticClass() ) ) \
	{ \
		type* P = ParticleCastChecked<type>(Particle); \
		ATTACH_FORCEVARS(ULimitedResizeForceVars,P); \
		P->Size = FromSize.GetRand(); \
		Vars->SizeRate = (ToSize.GetRand() - P->Size) / P->LifeSpan; \
		return; \
	}
//------------------------------------------------------------------------------
void ULimitedResizeForce::InitParticle( UParticle* Particle )
{
	guard(ULimitedResizeForce::InitParticle);
	NOTE(debugf(TEXT("ULimitedResizeForce::InitParticle")));

	LIMITEDRESIZEFORCE_INIT(USpriteParticle)
	LIMITEDRESIZEFORCE_INIT(UParticleDecal)

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(ULimitedResizeForce);
IMPLEMENT_CLASS(ULimitedResizeForce);

//------------------------------------------------------------------------------
void ULocationGrouped::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(ULocationGrouped::Apply);
	NOTE(debugf(TEXT("ULocationGrouped::Apply")));

	AActor* Relative = RelativeActor ? RelativeActor : System;

	if( LastLocation == FVector(0,0,0) )
	{
		LastLocation = Relative->Location;
		return;
	}

	if( Relative->Location == LastLocation ) return;

	FVector Delta = Relative->Location - LastLocation;

	if( bInterpolationCorrection )
	{
		PARTICLE_ITERATOR_BEGIN

			if( Particle->StasisTimer>0.f )
			{
				FLOAT PctLife = DeltaTime / DeltaSeconds;
				Particle->Location += Delta * PctLife;
			}
			else
			{
				Particle->Location += Delta;
			}

		PARTICLE_ITERATOR_END
	}
	else
	{
		PARTICLE_ITERATOR_BEGIN

			if( Particle->StasisTimer<=0.f )
				Particle->Location += Delta;

		PARTICLE_ITERATOR_END
	}

	LastLocation = Relative->Location;

	unguard;
}

IMPLEMENT_CLASS(ULocationGrouped);

//------------------------------------------------------------------------------
void ULocatorForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(ULocatorForce::Apply);
	NOTE(debugf(TEXT("ULocatorForce::Apply")));

	if( System->bCollisionBound )	// extents not needed, use collision cylinder instead.
	{
		PARTICLE_ITERATOR_BEGIN

			Particle->LastLocation = Particle->Location;
			Particle->Location += Particle->Velocity * DeltaTime;

		PARTICLE_ITERATOR_END
	}
	else
	{
		TEMPLATE_ITERATOR_BEGIN

			Template->Extents = FBox(0);

			PARTICLE_ITERATOR_BEGIN1

				Particle->LastLocation = Particle->Location;
				Particle->Location += Particle->Velocity * DeltaTime;

				Template->Extents += Particle->Location;

			PARTICLE_ITERATOR_END1

		TEMPLATE_ITERATOR_END
	}

	unguard;
}

IMPLEMENT_CLASS(ULocatorForce);

//------------------------------------------------------------------------------
void UMaxVelocityForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UMaxVelocityForce::Apply);

	PARTICLE_ITERATOR_BEGIN
		FLOAT Speed = Particle->Velocity.Size();
		if( Speed > MaxVelocity )
		{
			Particle->Velocity = Particle->Velocity.SafeNormal() * MaxVelocity;
		}
    PARTICLE_ITERATOR_END

    unguard;
}

IMPLEMENT_CLASS(UMaxVelocityForce);

//------------------------------------------------------------------------------
void UMeshAnchor::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P )
{
	guard(UMeshAnchor::NotifyParticleDestroyed);
	NOTE(debugf(TEXT("(%s)UMeshAnchor::NotifyParticleDestroyed( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), P ? *P->GetClassName() : TEXT("NULL") ));

	if( P == ParticleA )
	{
		// Fix ARL: Set ParticleA to NULL?
		DeleteUObject();	// This will cause the anchor to remove itself.
	}

	unguard;
}

//------------------------------------------------------------------------------
void UMeshAnchor::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(UMeshAnchor::Apply);
	NOTE(debugf(TEXT("(%s)UMeshAnchor::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

/* Fix ARL: Modify to work with Golem.

	if( ParticleA && RelativeActor && RelativeActor->Mesh )
	{
		RelativeActor->Mesh->GetFrame( &UParticleTemplate::Pts[0], sizeof(FVector), GMath.UnitCoords, RelativeActor );
		ParticleA->Location = UParticleTemplate::Pts[VertexIndex];
		ParticleA->LastLocation = ParticleA->Location;
	}
*/

    unguard;
}

//------------------------------------------------------------------------------
void UMeshAnchor::Serialize( FArchive &Ar )
{
	guard(UMeshAnchor::Serialize);

	Super::Serialize(Ar);

	if( !(Ar.IsLoading() && UClient::GetDefaultSettings()->ParticleSafeMode==3) )
		Ar << ParticleA;

	unguard;
}

IMPLEMENT_CLASS(UMeshAnchor);

//------------------------------------------------------------------------------
void UMoveZig::InitParticle( UParticle* Particle )
{
	guard(UMoveZig::InitParticle);

	ATTACH_FORCEVARS(UMoveZigVars,Particle);
	Vars->bAffected = false;

	// NOTE[aleiby]: It would be nice (and a lot faster) simply to update location here.
	// The problem with that is that the ParticleGenerator updates the particle's location
	// *after* we get this notification so all changes are lost.

	// Fix ARL: Perhaps add a PostInitParticle notification and move code there to speed things up?

	unguard;
}

//------------------------------------------------------------------------------
void UMoveZig::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UMoveZig::Apply);

	PARTICLE_ITERATOR_BEGIN

		GET_FORCEVARS_CHECKED(UMoveZigVars,Particle);
		if( !Vars->bAffected )
		{
			Vars->bAffected = true;

			FVector Dir = Particle->Velocity.SafeNormal();

			if
			(	DirMax.X
			||	DirMax.Y
			||	DirMax.Z
			||	DirMin.X
			||	DirMin.Y
			||	DirMin.Z
			)
			{
				Dir = FVector
					(	appRandRange( DirMin.X, DirMax.X )
					,	appRandRange( DirMin.Y, DirMax.Y )
					,	appRandRange( DirMin.Z, DirMax.Z )
					).SafeNormal();

				if( bRelative )
					Dir = Dir.TransformVectorBy( GMath.UnitCoords / System->Rotation );
			}

			FVector Start = Particle->Location;
			FVector End = Start + (Dir * Zig.GetRand());

			if( bTrace )
			{
				FCheckResult Hit(1.0f);
				System->GetLevel()->SingleLineCheck
				(	Hit
				,	System
				,	End
				,	Start
				,	TRACE_AllBlocking
				);
				if( Hit.Actor )
					End = Hit.Location;
			}

			Template->SetParticleLocation( Particle, End );
		}

	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UMoveZig);
IMPLEMENT_CLASS(UMoveZig);

//------------------------------------------------------------------------------
void UMultiTemplateForce::InitParticle( UParticle* Particle )
{
	guard(UMultiTemplateForce::InitParticle);

	ATTACH_FORCEVARS(UMultiTemplateForceVars,Particle);
	Vars->Attached.Empty();
	Vars->Offset.Empty();

	for( INT i=0; i<AttachedTemplates.Num(); i++ )
	{
		UParticle* P = AttachedTemplates(i)->GetParticle();
		P->bChild=1;
		//if( P->LifeSpan < Particle->LifeSpan )
		//	appErrorf(TEXT("AttachedTemplates(%s) of MultiTemplateForces(%s) must have an equal or larger LifeSpan than the Force's AffectedTemplate!"),AttachedTemplates(i)->GetFullName(),GetFullName());
		Vars->Attached.AddItem( P );
		Vars->Offset.AddItem( TemplateOffsets(i).GetRand() );
	}

	unguard;
}

//------------------------------------------------------------------------------
void UMultiTemplateForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UMultiTemplateForce::Apply);
	NOTE(debugf(TEXT("(%s)UMultiTemplateForce::Apply( %s, %f )"),GetFullName(),System->GetFullName(),DeltaSeconds));

	PARTICLE_ITERATOR_BEGIN

		GET_FORCEVARS_CHECKED(UMultiTemplateForceVars,Particle);
		for( INT i=0; i<Vars->Attached.Num(); i++ )
		{
			if( Vars->Offset(i) != FVector(0,0,0) )
				Vars->Attached(i)->Location = Particle->Location + Vars->Offset(i).TransformVectorBy( GMath.UnitCoords / Particle->Velocity.Rotation() );
			else
				Vars->Attached(i)->Location = Particle->Location;
		}

	PARTICLE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
UForce* UMultiTemplateForce::Duplicate( UObject* NewOuter )
{
	guard(UMultiTemplateForce::Duplicate);
	NOTE(debugf(TEXT("(%s)UMultiTemplateForce::Duplicate( %s )"), GetFullName(), NewOuter ? NewOuter->GetFullName() : TEXT("None") ));

	UMultiTemplateForce* NewForce = CastChecked<UMultiTemplateForce>( Super::Duplicate( NewOuter ) );

	// Manually copy the dynamic arrays since Tim isn't doing it for us.
	if( NewForce )
	{
		// Delete existing inner objects first.	
		{TArray<UParticleTemplate*> Remove=NewForce->AttachedTemplates;
		for( INT i=0; i<Remove.Num(); i++ )
			if( Remove(i)->IsIn( NewForce ) )
				Remove(i)->DeleteUObject();}

		NewForce->AttachedTemplates.Empty();
		NewForce->AttachedTemplates.Add( AttachedTemplates.Num() );
		for( INT i=0; i<AttachedTemplates.Num(); i++ )
			NewForce->AttachedTemplates(i) = AttachedTemplates(i)->Duplicate( NewOuter );
	}

	return NewForce;

	unguard;
}

//------------------------------------------------------------------------------
void UMultiTemplateForce::Destroy()
{
	guard(UMultiTemplateForce::Destroy);
	AttachedTemplates.Empty();
	Super::Destroy();
	unguard;
}

//------------------------------------------------------------------------------
void UMultiTemplateForce::AttachTemplate( UParticleTemplate* T )
{
	guard(UMultiTemplateForce::AttachTemplate);
	NOTE(debugf(TEXT("(%s)UMultiTemplateForce::AttachTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None")));
	if( T )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>( GetOuter() );
		T->SelectionWeight = 0.f;
		AttachedTemplates.AddItem( T );
		System->AddTemplate( T );
		TemplateOffsets.AddZeroed();
	}
	unguard;
}


//------------------------------------------------------------------------------
void UMultiTemplateForce::execAttachTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(UMultiTemplateForce::execAttachTemplate);
	NOTE(debugf(TEXT("UMultiTemplateForce::execAttachTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	AttachTemplate( T );
	
	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS_PREOVERRIDE(UMultiTemplateForce,{},{},{},{},
{
	// If we get destroyed early (i.e. particle collision) ensure that our attached particles get destroyed as well.
	GET_FORCEVARS_CHECKED(UMultiTemplateForceVars,Particle);
	for( INT i=0; i<Vars->Attached.Num(); i++ )
	{
		Vars->Attached(i)->LifeSpan = 0.0f;
		Vars->Attached(i)->bChild=0;
	}
})
IMPLEMENT_CLASS(UMultiTemplateForce);

#include "TMatrix.h"

//------------------------------------------------------------------------------
struct NeuralNet
{
	static FLOAT Sigmoid( FLOAT u )
	{ return 1.f/(1.f+appExp(-u)); }

	static FLOAT Initial;
	static FLOAT LearnInit()
	{ return appRandRange(-Initial,Initial); }

	static FString ToString( FLOAT f )
	{ return FString::Printf(TEXT("%f"),f); }

	// E = 1/2 (Ci - Oi)^2
	static FLOAT Error( TMatrix<FLOAT> Output, TMatrix<FLOAT> Correct )
	{
		TMatrix<FLOAT> Diff = Correct - Output;
		return (0.5f * (Diff.Transpose() ^ Diff))(0,0);
	}
};

FLOAT NeuralNet::Initial=0.f;

//------------------------------------------------------------------------------
UBOOL UNeuralNetForce::Learn( TArray< TArray<FLOAT> > In, TArray< TArray<FLOAT> > Out, INT NumHidden, FLOAT alpha, FLOAT Initial )
{
	guard(UNeuralNetForce::Learn);

//	TMatrix<FLOAT>::Test(GLog);	//!!DEBUG

	check(In.Num()==Out.Num());
	check(In.Num()>0);
	check(NumHidden>0);

	INT NumSamples = In.Num();
	INT NumInputs = In(0).Num();
	INT NumOutputs = Out(0).Num();

	NeuralNet::Initial=Initial;
	TMatrix<FLOAT> v=TMatrix<FLOAT>(NumHidden,NumInputs,&NeuralNet::LearnInit);
	TMatrix<FLOAT> w=TMatrix<FLOAT>(NumOutputs,NumHidden,&NeuralNet::LearnInit);

	v.Dump(*GLog,&NeuralNet::ToString);
	w.Dump(*GLog,&NeuralNet::ToString);

	for( INT count=0; count<1000; count++ )
	{
		INT x=appRand()%NumSamples;
		TMatrix<FLOAT> i = TMatrix<FLOAT>(In(x)).Transpose();
		TMatrix<FLOAT> c = TMatrix<FLOAT>(Out(x)).Transpose();

		// flow forward
		// h = sigmoid[v . i]
		// o = sigmoid[w . h]
		TMatrix<FLOAT> h = (v^i).ApplyAll(&NeuralNet::Sigmoid);
		TMatrix<FLOAT> o = (w^h).ApplyAll(&NeuralNet::Sigmoid);

		// flow backward
		// odelta = (c - o) o (1 - o)
		// hdelta = (h (1 - h) (tr[w] . odelta) )
		// w = w + alpha (odelta . tr[h])
		// v = v + alpha (hdelta . tr[i])
		TMatrix<FLOAT> odelta = o * (1 - o) * (c - o);
		TMatrix<FLOAT> hdelta = h * (1 - h) * (w.Transpose() ^ odelta);
		w += alpha * (odelta ^ h.Transpose());
		v += alpha * (hdelta ^ i.Transpose());

		FLOAT E=NeuralNet::Error(o,c);
		NOTE1(debugf(TEXT("Error(%d): %f"),count,E));
		if( E < KINDA_SMALL_NUMBER )
			goto SUCCESS;
	}

//FAILURE:
	return 0;
SUCCESS:
	return 1;

	unguard;
}

//------------------------------------------------------------------------------
void UNeuralNetForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UNeuralNetForce::Apply);

	if(!bLearn) return;
	bLearn=false;

	TArray< TArray<FLOAT> > In;
	TArray< TArray<FLOAT> > Out;

	INT i=0;

#if 1 //AND

	new(In)TArray<FLOAT>(2);
	In(i)(0) = 0; In(i)(1) = 0;
	new(Out)TArray<FLOAT>(1);
	Out(i)(0) = 0; i++;

	new(In)TArray<FLOAT>(2);
	In(i)(0) = 0; In(i)(1) = 1;
	new(Out)TArray<FLOAT>(1);
	Out(i)(0) = 0; i++;

	new(In)TArray<FLOAT>(2);
	In(i)(0) = 1; In(i)(1) = 0;
	new(Out)TArray<FLOAT>(1);
	Out(i)(0) = 0; i++;

	new(In)TArray<FLOAT>(2);
	In(i)(0) = 1; In(i)(1) = 1;
	new(Out)TArray<FLOAT>(1);
	Out(i)(0) = 1; i++;

#elif 1 //PARTICLES

	PARTICLE_ITERATOR_BEGIN

		new(In)TArray<FLOAT>(7);
		In(i)(0) = Particle->LastLocation.X;
		In(i)(1) = Particle->LastLocation.Y;
		In(i)(2) = Particle->LastLocation.Z;
		In(i)(3) = Particle->Velocity.X;
		In(i)(4) = Particle->Velocity.Y;
		In(i)(5) = Particle->Velocity.Z;
		In(i)(6) = Particle->LifeSpan;
//		In(i)(7) = Particle->StasisTimer;

		new(Out)TArray<FLOAT>(3);
		Out(i)(0) = Particle->Location.X;
		Out(i)(1) = Particle->Location.Y;
		Out(i)(2) = Particle->Location.Z;

		i++;

	PARTICLE_ITERATOR_END

#endif

	debugf(TEXT("Learned: %s"), Learn(In,Out,NumHidden,Alpha,Initial) ? TEXT("True") : TEXT("False"));

	unguard;
}

IMPLEMENT_CLASS(UNeuralNetForce);


//------------------------------------------------------------------------------
UForce* UOnDestroyForce::Duplicate( UObject* NewOuter )
{
	guard(UOnDestroyForce::Duplicate);
	NOTE(debugf(TEXT("(%s)UOnDestroyForce::Duplicate( %s )"), GetFullName(), NewOuter ? NewOuter->GetFullName() : TEXT("None") ));

	UOnDestroyForce* NewForce = CastChecked<UOnDestroyForce>( Super::Duplicate( NewOuter ) );

	// Manually copy the dynamic arrays since Tim isn't doing it for us.
	if( NewForce )
	{
		// Delete existing inner objects first.	
		{TArray<UParticleTemplate*> Remove=NewForce->OnDestroyTemplates;
		for( INT i=0; i<Remove.Num(); i++ )
			if( Remove(i)->IsIn( NewForce ) )
				Remove(i)->DeleteUObject();}

		NewForce->OnDestroyTemplates.Empty();
		NewForce->OnDestroyTemplates.Add( OnDestroyTemplates.Num() );
		for( INT i=0; i<OnDestroyTemplates.Num(); i++ )
			NewForce->OnDestroyTemplates(i) = OnDestroyTemplates(i)->Duplicate( NewOuter );
	}

	return NewForce;

	unguard;
}

//------------------------------------------------------------------------------
void UOnDestroyForce::Destroy()
{
	guard(UOnDestroyForce::Destroy);
	OnDestroyTemplates.Empty();
	Super::Destroy();
	unguard;
}

//------------------------------------------------------------------------------
void UOnDestroyForce::AddDestroyTemplate( UParticleTemplate* T )
{
	guard(UOnDestroyForce::AddDestroyTemplate);
	NOTE(debugf(TEXT("(%s)UOnDestroyForce::AddDestroyTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None")));
	if( T )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>( GetOuter() );
		T->SelectionWeight = 0.f;
		OnDestroyTemplates.AddItem( T );
		System->AddTemplate( T );
	}
	unguard;
}


//------------------------------------------------------------------------------
void UOnDestroyForce::execAddDestroyTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(UOnDestroyForce::execAddDestroyTemplate);
	NOTE(debugf(TEXT("UOnDestroyForce::execAddDestroyTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	AddDestroyTemplate( T );
	
	unguard;
}

//------------------------------------------------------------------------------
void UOnDestroyForce::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &Particle )
{
	guard(UOnDestroyForce::NotifyParticleDestroyed);

	if( PurgingGarbage() )
		return;

	for( INT i=0; i<OnDestroyTemplates.Num(); i++ )
	{
		UParticleTemplate* T = OnDestroyTemplates(i);
		if( T )
		{
			UParticle* P = T->GetParticle();
			P->Location = Particle->Location;
			if( bInheritVelocity )
				P->Velocity = Particle->Velocity;
		}
	}

	if( Sound && USound::Audio )
		USound::Audio->PlaySound( System, SLOT_None, Sound, Particle->Location, Volume.GetRand(), Max( 1.f, Radius.GetRand() ), Pitch.GetRand(), SF_NoUpdates, 0.f );

	if( DamageRadius )
	{
		static UFunction* HurtRadius = NULL;
		if( HurtRadius==NULL )
			HurtRadius = System->FindFunctionChecked( FName(TEXT("HurtRadius"),FNAME_Intrinsic) );
		struct TParms { FLOAT DamageAmount; FLOAT DamageRadius; UClass* DamageType; FLOAT Momentum; FVector HitLocation; };
		TParms Parms;
		Parms.DamageAmount	=DamageAmount;
		Parms.DamageRadius	=DamageRadius;
		Parms.DamageType	=DamageType;
		Parms.Momentum		=Momentum;
		Parms.HitLocation	=Particle->Location;
		System->ProcessEvent(HurtRadius,&Parms);
		//UDestroyOnDamage::StaticNotify( System->GetLevel(), Particle->Location, DamageRadius, DamageType );
	}

	unguard;
}

IMPLEMENT_CLASS(UOnDestroyForce);


//------------------------------------------------------------------------------
void UOscillatingPerParticleForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UOscillatingPerParticleForce::Apply);

	PARTICLE_ITERATOR_BEGIN

		GET_FORCEVARS_CHECKED(UOscillatingPerParticleForceVars,Particle);

		// Update time (ensuring CurrentTime stays within the range 0==>Period).
		Vars->CurrentTime.X += DeltaTime;
		Vars->CurrentTime.Y += DeltaTime;
		Vars->CurrentTime.Z += DeltaTime;

#if 1 //FAST
		Vars->CurrentTime.X = appFmod( Vars->CurrentTime.X, Vars->Period.X );
		Vars->CurrentTime.Y = appFmod( Vars->CurrentTime.Y, Vars->Period.Y );
		Vars->CurrentTime.Z = appFmod( Vars->CurrentTime.Z, Vars->Period.Z );
#else
		while( Vars->CurrentTime.X > Vars->Period.X )
			Vars->CurrentTime.X -= Vars->Period.X;
		while( Vars->CurrentTime.Y > Vars->Period.Y )
			Vars->CurrentTime.Y -= Vars->Period.Y;
		while( Vars->CurrentTime.Z > Vars->Period.Z )
			Vars->CurrentTime.Z -= Vars->Period.Z;
#endif

		// Scale CurrentTime from -1 ==> +1.
#if 1 //FAST
		FLOAT PctX = (2.0f * GMath.SinTab(appRound(Vars->CurrentTime.X / Vars->Period.X * 65536.f))) - 1.0f;
		FLOAT PctY = (2.0f * GMath.SinTab(appRound(Vars->CurrentTime.Y / Vars->Period.Y * 65536.f))) - 1.0f;
		FLOAT PctZ = (2.0f * GMath.SinTab(appRound(Vars->CurrentTime.Z / Vars->Period.Z * 65536.f))) - 1.0f;
#else
		FLOAT PctX = (2.0f * appSin(Vars->CurrentTime.X / Vars->Period.X * PI*2)) - 1.0f;
		FLOAT PctY = (2.0f * appSin(Vars->CurrentTime.Y / Vars->Period.Y * PI*2)) - 1.0f;
		FLOAT PctZ = (2.0f * appSin(Vars->CurrentTime.Z / Vars->Period.Z * PI*2)) - 1.0f;
#endif

		FVector Force;
		Force.X = Vars->Force.X * PctX;
		Force.Y = Vars->Force.Y * PctY;
		Force.Z = Vars->Force.Z * PctZ;

		// Allow user to toggle between acting as an accelerator or as a direct force.
		if( bAccelerator )
		{
			Particle->Velocity += Force * DeltaTime;
		}
		else
		{
			Particle->Velocity -= Vars->LastAddition;	// NOTE[aleiby]: This might cause weird behavior when used with other forces that act as functions upon the particle's current velocity.  Ideally, this should be a Locator force, that never even mucks with the particle velocity.
			Particle->Velocity += Force;
			Vars->LastAddition = Force;
		}

	PARTICLE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
void UOscillatingPerParticleForce::InitParticle( UParticle* Particle )
{
	guard(UOscillatingPerParticleForce::InitParticle);

	ATTACH_FORCEVARS(UOscillatingPerParticleForceVars,Particle);

	Vars->Force			= FVector
							(	ForceX.GetRand()
							,	ForceY.GetRand()
							,	ForceZ.GetRand()	);

	Vars->Period		= FVector
							(	Max( Abs( PeriodX.GetRand() ), 0.0001f )
							,	Max( Abs( PeriodY.GetRand() ), 0.0001f )
							,	Max( Abs( PeriodZ.GetRand() ), 0.0001f )	);

	Vars->CurrentTime	= FVector(0,0,0);
	Vars->LastAddition	= FVector(0,0,0);

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UOscillatingPerParticleForce);
IMPLEMENT_CLASS(UOscillatingPerParticleForce);


//------------------------------------------------------------------------------
UForce* UParticleSalamanderForce::Duplicate( UObject* NewOuter )
{
	guard(UParticleSalamanderForce::Duplicate);
	NOTE(debugf(TEXT("(%s)UParticleSalamanderForce::Duplicate( %s )"), GetFullName(), NewOuter ? NewOuter->GetFullName() : TEXT("None") ));

	UParticleSalamanderForce* NewForce = CastChecked<UParticleSalamanderForce>( Super::Duplicate( NewOuter ) );

	// Manually copy the dynamic arrays since Tim isn't doing it for us.
	if( NewForce )
	{
		// Delete existing inner objects first.	
		{TArray<UParticleTemplate*> Remove=NewForce->ForceTemplates;
		for( INT i=0; i<Remove.Num(); i++ )
			if( Remove(i)->IsIn( NewForce ) )
				Remove(i)->DeleteUObject();}

		NewForce->ForceTemplates.Empty();
		NewForce->ForceTemplates.Add( ForceTemplates.Num() );
		for( INT i=0; i<ForceTemplates.Num(); i++ )
			NewForce->ForceTemplates(i) = ForceTemplates(i)->Duplicate( NewOuter );
	}

	return NewForce;

	unguard;
}

//------------------------------------------------------------------------------
void UParticleSalamanderForce::Destroy()
{
	guard(UParticleSalamanderForce::Destroy);
	ForceTemplates.Empty();
	Super::Destroy();
	unguard;
}

//------------------------------------------------------------------------------
void UParticleSalamanderForce::AddForceTemplate( UParticleTemplate* T )
{
	guard(UParticleSalamanderForce::AddForceTemplate);
	NOTE(debugf(TEXT("(%s)UParticleSalamanderForce::AddForceTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None")));
	if( T )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>( GetOuter() );
		T->SelectionWeight = 0.f;
		ForceTemplates.AddItem( T );
		System->AddTemplate( T );
	}
	unguard;
}


//------------------------------------------------------------------------------
void UParticleSalamanderForce::execAddForceTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(UParticleSalamanderForce::execAddForceTemplate);
	NOTE(debugf(TEXT("UParticleSalamanderForce::execAddForceTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	AddForceTemplate( T );
	
	unguard;
}

//------------------------------------------------------------------------------
void UParticleSalamanderForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UParticleSalamanderForce::Apply);

	PARTICLE_ITERATOR_BEGIN

		GET_FORCEVARS_CHECKED(UParticleSalamanderForceVars,Particle);

		// Local variables.
		FLOAT ScaledVolume;
		FLOAT SecondsPerParticle;
		UParticle* P;
		UParticleTemplate* Template;
		
		// Scale the volume based on the global ParticleDensity value from MinVolume to Volume.
		ScaledVolume = Vars->Volume;

		//
		// Create new particles as needed.
		//

		if
		(	Particle->LifeSpan > 0.0
		&&	ScaledVolume > 0.0
		&&	ForceTemplates.Num() > 0
		)
		{
			SecondsPerParticle = 1.f / ScaledVolume;

			FCoords Coords = GMath.UnitCoords * Particle->Velocity.Rotation();

			Vars->ParticleTimer += DeltaTime;
			while( Vars->ParticleTimer >= SecondsPerParticle )
			{
				// Update the timer according to the set Volume.
				Vars->ParticleTimer -= SecondsPerParticle;

				// For each ForceTemplate.
				for( INT i=0; i<ForceTemplates.Num(); i++ )
				{
					Template = ForceTemplates(i);

					// Get a new (initialized) Particle from the ParticleManager.
					P = Template->GetParticle();

					// Set initial position and direction.
					FVector Dir = RandomSpreadVector( Spread ).SafeNormal().TransformVectorBy( Coords );
					Template->SetParticleDirection( P, Dir );

					FVector Loc = Particle->Location + Dir * EmisionOffset.GetRand();
					Template->SetParticleLocation( P, Loc );

					if( bInheritVelocity )
						P->Velocity = Particle->Velocity;

					// Evenly distribute new particles.
					P->StasisTimer += (DeltaTime - Vars->ParticleTimer);
				}
			}
		}

	PARTICLE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
void UParticleSalamanderForce::InitParticle( UParticle* Particle )
{
	guard(UParticleSalamanderForce::InitParticle);

	ATTACH_FORCEVARS(UParticleSalamanderForceVars,Particle);
	Vars->Volume = Volume.GetRand();
	Vars->ParticleTimer = 0.0;

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UParticleSalamanderForce);
IMPLEMENT_CLASS(UParticleSalamanderForce);


//------------------------------------------------------------------------------
void UPerParticleFriction::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UPerParticleFriction::Apply);

	PARTICLE_ITERATOR_BEGIN
		GET_FORCEVARS_CHECKED(UPerParticleFrictionVars,Particle);
		Particle->Velocity -= Particle->Velocity * Vars->Friction * DeltaTime;
	PARTICLE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
void UPerParticleFriction::InitParticle( UParticle* Particle )
{
	guard(UPerParticleGravity::InitParticle);

	ATTACH_FORCEVARS(UPerParticleFrictionVars,Particle);
	Vars->Friction = Friction.GetRand();

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UPerParticleFriction);
IMPLEMENT_CLASS(UPerParticleFriction);

//------------------------------------------------------------------------------
void UPerParticleGravity::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UPerParticleGravity::Apply);

	PARTICLE_ITERATOR_BEGIN

		if( bUseZoneGravity )
		{
			Particle->Velocity.Z += System->GetLevel()->Model->PointRegion( System->GetLevel()->GetLevelInfo(), Particle->Location ).Zone->PhysicsVolume->Gravity.Z * DeltaTime;
			NOTE(debugf(TEXT("(%s)Apply (ZoneGravity): %f"), *Particle->GetClassName(), System->GetLevel()->Model->PointRegion( System->GetLevel()->GetLevelInfo(), Particle->Location ).Zone->PhysicsVolume->Gravity.Z ));
		}
		else
		{
			GET_FORCEVARS_CHECKED(UPerParticleGravityVars,Particle);
			Particle->Velocity.Z -= Vars->Gravity * DeltaTime;
			NOTE(debugf(TEXT("(%s)Apply: %f"), *Particle->GetClassName(), Vars->Gravity ));
		}

	PARTICLE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
void UPerParticleGravity::InitParticle( UParticle* Particle )
{
	guard(UPerParticleGravity::InitParticle);

	ATTACH_FORCEVARS(UPerParticleGravityVars,Particle);
	Vars->Gravity = Gravity.GetRand();
	NOTE(debugf(TEXT("(%s)InitParticle: %f"), *Particle->GetClassName(), Vars->Gravity ));

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UPerParticleGravity);
IMPLEMENT_CLASS(UPerParticleGravity);


//------------------------------------------------------------------------------
void URepulsionForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(URepulsionForce::Apply);

	PARTICLE_ITERATOR_BEGIN

		Particle->Velocity += CalcRepulsion( System, DeltaSeconds, Particle ) * DeltaTime;

	PARTICLE_ITERATOR_END
	
	unguard;
}

//------------------------------------------------------------------------------
FVector URepulsionForce::CalcRepulsion( AParticleGenerator* System, FLOAT DeltaSeconds, UParticle* P )
{
	guard(URepulsionForce::CalcRepulsion);

	FVector Repulsion = FVector(0,0,0);

	PARTICLE_ITERATOR_BEGIN

		if (Particle!=P)
		{
			FVector Diff = P->Location - Particle->Location;
			FLOAT DistSq = Diff.SizeSquared();
			FLOAT Force = Strength / Max( DistSq, 1.f );
			FVector Dir = DistSq ? Diff / appSqrt(DistSq) : VRand();
			Repulsion += Dir * Force;
		}

	PARTICLE_ITERATOR_END

	return Repulsion;
	
	unguard;
}

IMPLEMENT_CLASS(URepulsionForce);


//------------------------------------------------------------------------------
#define RESIZEFORCE_APPLY \
	GET_FORCEVARS_CHECKED(UResizeForceVars,Particle); \
	Vars->SizeTimer += DeltaTime; \
	while( Vars->SizeTime > 0.f && Vars->SizeTimer >= Vars->SizeTime ) \
	{ \
		Vars->SizeTimer -= Vars->SizeTime; \
		Vars->SizeRate *= -1.f;	/* Reverse grow direction. */ \
	} \
	P->Size += Vars->SizeRate * DeltaTime; \
	if( P->Size <= 0.f && Vars->SizeRate <= 0.f && Phase == 1 ) \
		P->LifeSpan = 0.f;
//------------------------------------------------------------------------------
void UResizeForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UResizeForce::Apply);
	
	if( Phase == 0 )
		return;

	PARTICLE_ITERATOR_BEGIN
		if( Particle->IsA( USpriteParticle::StaticClass() ) )
		{
			USpriteParticle* P = ParticleCastChecked<USpriteParticle>(Particle);
			RESIZEFORCE_APPLY
		}
		else if( Particle->IsA( UParticleDecal::StaticClass() ) )
		{
			UParticleDecal* P = ParticleCastChecked<UParticleDecal>(Particle);
			FLOAT OldSize = P->Size;
			RESIZEFORCE_APPLY
			FLOAT Ratio = P->Size / OldSize;
			P->Offsets[0] *= Ratio;
			P->Offsets[1] *= Ratio;
			P->Offsets[2] = -P->Offsets[0];
			P->Offsets[3] = -P->Offsets[1];
		}
	PARTICLE_ITERATOR_END
	
	unguard;
}

#define SMALLEST_PHASEABLE_LIFESPAN 0.01
//------------------------------------------------------------------------------
void UResizeForce::InitParticle( UParticle* Particle )
{
	guard(UResizeForce::InitParticle);

	if
	(	Particle->IsA( USpriteParticle::StaticClass() )
	||	Particle->IsA( UParticleDecal::StaticClass() )
	)
	{
		ATTACH_FORCEVARS(UResizeForceVars,Particle);
		Vars->SizeRate = SizeRate.GetRand();
		Vars->SizeTime = (Phase > 0 && Particle->LifeSpan > SMALLEST_PHASEABLE_LIFESPAN) ? Particle->LifeSpan / (FLOAT)Phase : 0.f;
		Vars->SizeTimer = 0.f;
	}

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UResizeForce);
IMPLEMENT_CLASS(UResizeForce);

// Fix ARL: Go back and relearn Rotation II from Physics 201 and then try again.
// Fix ARL: Then rewrite the TriForce (or its compliment) to use the same math.

//------------------------------------------------------------------------------
void URigidBody::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(URigidBody::Apply);
	NOTE(debugf(TEXT("URigidBody::Apply")));

	FRotator Rotation(appRound(Pitch),appRound(Yaw),0);
	FCoords Coords = GMath.UnitCoords / Rotation;

	// Count particles.
	INT NumParticles=0;
	{
	PARTICLE_ITERATOR_BEGIN
		NumParticles++;
	PARTICLE_ITERATOR_END
	}
	FLOAT PerParticle = 1.f/(FLOAT)NumParticles;

	// Find initial center.
	UBOOL bInitializedCenter=0;
	if( Center == FVector(0,0,0) )
	{
	PARTICLE_ITERATOR_BEGIN
		Center += Particle->Location;
	PARTICLE_ITERATOR_END
	Center *= PerParticle;
	bInitializedCenter=1;
	}

	// See if we picked up any new particles.
	INT NumNewParticles=0;
	{
	PARTICLE_ITERATOR_BEGIN
		GET_FORCEVARS_CHECKED(URigidBodyVars,Particle);
		if( Vars->Offset == FVector(0,0,0) )
			NumNewParticles++;
	PARTICLE_ITERATOR_END
	}

	// Adjust center as necessary.
	if( NumNewParticles>0 && !bInitializedCenter )
	{
		// Move old particles back to original relative locations first.
		{
		PARTICLE_ITERATOR_BEGIN
			GET_FORCEVARS_CHECKED(URigidBodyVars,Particle);
			if( Vars->Offset != FVector(0,0,0) )
				Particle->Location = Center + Vars->Offset.TransformVectorBy( GMath.UnitCoords * Rotation );
		PARTICLE_ITERATOR_END
		}
	
		Center = FVector(0,0,0);
		PARTICLE_ITERATOR_BEGIN
			Center += Particle->Location;
		PARTICLE_ITERATOR_END
		Center *= PerParticle;
		bInitializedCenter=1;
	}

	// Calculate new offsets if necessary.
	if( bInitializedCenter )
	{
	PARTICLE_ITERATOR_BEGIN
		GET_FORCEVARS_CHECKED(URigidBodyVars,Particle);
		Vars->Offset = (Particle->Location - Center).TransformVectorBy( Coords );
	PARTICLE_ITERATOR_END
	}
	
	// Update location based on new velocities.
	FVector CumulativeVelocity(0,0,0);
	{
	PARTICLE_ITERATOR_BEGIN
		CumulativeVelocity += Particle->Velocity;
	PARTICLE_ITERATOR_END
	}
	Center += CumulativeVelocity * PerParticle * DeltaSeconds;

	// Calculate cumulative torque.
	FLOAT Theta=0;
	FLOAT Phi=0;
	{	
	PARTICLE_ITERATOR_BEGIN
		GET_FORCEVARS_CHECKED(URigidBodyVars,Particle);
		FLOAT Dist = Vars->Offset.Size();	// Fix ARL: Cache.
		FVector RelativeVelocity = Particle->Velocity.TransformVectorBy( Coords );
		Theta += Dist * appSqrt(Square(RelativeVelocity.X)+Square(RelativeVelocity.Y));
		Phi   += Dist * RelativeVelocity.Z;
	PARTICLE_ITERATOR_END
	}
	Pitch    += Phi   * 32768.f/PI * DeltaSeconds;
	Yaw      += Theta * 32768.f/PI * DeltaSeconds;
	Rotation =  FRotator(appRound(Pitch),appRound(Yaw),0);

	// Update offsets based on new direction.
	{	
	PARTICLE_ITERATOR_BEGIN
		GET_FORCEVARS_CHECKED(URigidBodyVars,Particle);
		Particle->Location = Center + Vars->Offset.TransformVectorBy( GMath.UnitCoords * Rotation );
	PARTICLE_ITERATOR_END
	}

	unguard;
}

//------------------------------------------------------------------------------
void URigidBody::InitParticle( UParticle* Particle )
{
	guard(URigidBody::InitParticle);

	ATTACH_FORCEVARS(URigidBodyVars,Particle);
	Vars->Offset = FVector(0,0,0);	// uninitialized.

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(URigidBody);
IMPLEMENT_CLASS(URigidBody);

//------------------------------------------------------------------------------
void URotationForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(URotationForce::Apply);
	NOTE(debugf(TEXT("(%s)URotationForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("None"), DeltaSeconds ));
	
	if( Phase == 0 )
		return;

	PARTICLE_ITERATOR_BEGIN

		URotatingParticle* P = ParticleCastChecked<URotatingParticle>(Particle);
		if(P)
		{
			GET_FORCEVARS_CHECKED(URotationForceVars,Particle);
			Vars->RotationTimer += DeltaTime;
			while( Vars->RotationTime > 0.f && Vars->RotationTimer >= Vars->RotationTime )
			{
				NOTE(debugf(TEXT("Reversing Rotation direction for %s"), *P->GetClassName() ));
				Vars->RotationTimer -= Vars->RotationTime;
				Vars->RotationRate *= -1.f;	// Reverse Rotation direction.
			}
			P->Angle += Vars->RotationRate * DeltaTime;
			NOTE(debugf(TEXT("%s->Angle = %f (RotationRate: %f)"), *P->GetClassName(), P->Angle, Vars->RotationRate ));
		}

	PARTICLE_ITERATOR_END
	
	unguard;
}

//------------------------------------------------------------------------------
void URotationForce::InitParticle( UParticle* Particle )
{
	guard(URotationForce::InitParticle);

	if( Particle->IsA( URotatingParticle::StaticClass() ) )
	{
		ATTACH_FORCEVARS(URotationForceVars,Particle);
		Vars->RotationRate = RotationRate.GetRand();
		Vars->RotationTime = Phase ? Particle->LifeSpan / (FLOAT)Phase : 0.f;
		Vars->RotationTimer = 0.f;
		NOTE(debugf(TEXT("(%s) Attached %s to %s | RotationRate: %f RotationTime: %f"), GetFullName(), *Vars->GetClassName(), *Particle->GetClassName(), Vars->RotationRate, Vars->RotationTime ));
	}

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(URotationForce);
IMPLEMENT_CLASS(URotationForce);

//------------------------------------------------------------------------------
void URotationGrouped::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(URotationGrouped::Apply);
	NOTE(debugf(TEXT("(%s)URotationGrouped::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

	AActor* Relative = RelativeActor ? RelativeActor : System;

	if( LastRotation == FRotator(0,0,0) )
	{
		LastRotation = Relative->Rotation;
		return;
	}

	if( Relative->Rotation == LastRotation ) return;
	
	PARTICLE_ITERATOR_BEGIN

		NOTE(debugf(TEXT("    Updating %s"), *Particle->GetClassName() ));
		Particle->Location = Relative->Location + ((Particle->Location - Relative->Location).TransformVectorBy(GMath.UnitCoords / LastRotation)).TransformVectorBy(GMath.UnitCoords * Relative->Rotation);

	PARTICLE_ITERATOR_END

	LastRotation = Relative->Rotation;

	unguard;
}

IMPLEMENT_CLASS(URotationGrouped);

//------------------------------------------------------------------------------
void USlipForce::execSetEndpoints( FFrame& Stack, RESULT_DECL )
{
	guard(USlipForce::execSetEndpoints);

	P_GET_OBJECT(UParticle,A);
	P_GET_OBJECT(UParticle,B);
	P_GET_FLOAT_OPTX(SlipDistance,0.f);
	P_FINISH;

	SetEndpoints( A, B, SlipDistance );
	
	unguard;
}

//------------------------------------------------------------------------------
void USlipForce::SetEndpoints( UParticle* A, UParticle* B, FLOAT SlipDistance )
{
	guard(USlipForce::SetEndpoints);
	NOTE(debugf(TEXT("(%s)USlipForce::SetEndpoints( %s, %s, %f)"), GetFullName(), A ? *A->GetClassName() : TEXT("NULL"), B ? *B->GetClassName() : TEXT("NULL"), SlipDistance ));
	
	check(A!=NULL);
	check(B!=NULL);

	ParticleA = A;
	ParticleB = B;

	FLOAT Dist = (ParticleA->Location - ParticleB->Location).Size();
	Distance.A = Dist + SlipDistance;
	Distance.B = Dist - SlipDistance;

	unguard;
}

//------------------------------------------------------------------------------
void USlipForce::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P )
{
	guard(USlipForce::NotifyParticleDestroyed);
	NOTE(debugf(TEXT("(%s)USlipForce::NotifyParticleDestroyed( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), P ? *P->GetClassName() : TEXT("NULL") ));

	if( P == ParticleA || P == ParticleB )
	{
		// Fix ARL: Set ParticleA and ParticleB to NULL?
		DeleteUObject();	// This will cause the spring to remove itself.
	}

	unguard;
}

//------------------------------------------------------------------------------
void USlipForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(USlipForce::Apply);
	NOTE(debugf(TEXT("(%s)USlipForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

	// safecheck.
	if( !ParticleA || !ParticleB ) return;

	if(	ParticleA->Location == ParticleA->LastLocation
	&&	ParticleB->Location == ParticleB->LastLocation ) return;

	FVector Diff = ParticleA->Location - ParticleB->Location;
    FVector Dir  = Diff.SafeNormal();
	FLOAT   Dist = Diff.Size();

	if( Dist < Distance.GetMin() )
	{
		if( ParticleA->Location == ParticleA->LastLocation )
		{
			ParticleB->Location -= Dir * (Distance.GetMin() - Dist);
			ParticleB->Velocity = (ParticleB->Location - ParticleB->LastLocation) / DeltaSeconds;
		}
		else if( ParticleB->Location == ParticleB->LastLocation )
		{
			ParticleA->Location += Dir * (Distance.GetMin() - Dist);
			ParticleA->Velocity = (ParticleA->Location - ParticleA->LastLocation) / DeltaSeconds;
		}
		else
		{
			FVector Offset = Dir * (Distance.GetMin() - Dist) / 2.f;
			ParticleA->Location += Offset;
			ParticleB->Location -= Offset;
			ParticleA->Velocity = (ParticleA->Location - ParticleA->LastLocation) / DeltaSeconds;
			ParticleB->Velocity = (ParticleB->Location - ParticleB->LastLocation) / DeltaSeconds;
		}
	}
	else if( Dist > Distance.GetMax() )
	{
		if( ParticleA->Location == ParticleA->LastLocation )
		{
			ParticleB->Location -= Dir * (Distance.GetMax() - Dist);
			ParticleB->Velocity = (ParticleB->Location - ParticleB->LastLocation) / DeltaSeconds;
		}
		else if( ParticleB->Location == ParticleB->LastLocation )
		{
			ParticleA->Location += Dir * (Distance.GetMax() - Dist);
			ParticleA->Velocity = (ParticleA->Location - ParticleA->LastLocation) / DeltaSeconds;
		}
		else
		{
			FVector Offset = Dir * (Distance.GetMax() - Dist) / 2.f;
			ParticleA->Location += Offset;
			ParticleB->Location -= Offset;
			ParticleA->Velocity = (ParticleA->Location - ParticleA->LastLocation) / DeltaSeconds;
			ParticleB->Velocity = (ParticleB->Location - ParticleB->LastLocation) / DeltaSeconds;
		}
	}

	NOTE(debugf(TEXT("%f (%s)->(%s)"), (ParticleA->Location - ParticleB->Location).Size(), *ParticleA->GetClassName(), *ParticleB->GetClassName() ));

    unguard;
}

//------------------------------------------------------------------------------
void USlipForce::Serialize( FArchive &Ar )
{
	guard(USlipForce::Serialize);

	Super::Serialize(Ar);

	if( !(Ar.IsLoading() && UClient::GetDefaultSettings()->ParticleSafeMode==3) )
		Ar << ParticleA << ParticleB;

	unguard;
}

IMPLEMENT_CLASS(USlipForce);

//------------------------------------------------------------------------------
void USpringForce::execSetEndpoints( FFrame& Stack, RESULT_DECL )
{
	guard(USpringForce::execSetEndpoints);

	P_GET_OBJECT(UParticle,A);
	P_GET_OBJECT(UParticle,B);
	P_FINISH;

	SetEndpoints( A, B );
	
	unguard;
}

//------------------------------------------------------------------------------
void USpringForce::SetEndpoints( UParticle* A, UParticle* B )
{
	guard(USpringForce::SetEndpoints);
	
	check(A!=NULL);
	check(B!=NULL);

	ParticleA = A;
	ParticleB = B;

	SpringLength = (ParticleA->Location - ParticleB->Location).Size();

	{
		ATTACH_FORCEVARS(USpringForceVars,ParticleA);
		Vars->PivotOffset = FVector(0,0,0);	// uninitialized.
	}
	
	{
		ATTACH_FORCEVARS(USpringForceVars,ParticleB);
		Vars->PivotOffset = FVector(0,0,0);	// uninitialized.
	}

	unguard;
}

//------------------------------------------------------------------------------
void USpringForce::SetStiffnessRef( FLOAT &StiffnessRefVar )
{
	guard(USpringForce::SetStiffnessRef);
	StiffnessPtr = (INT)&StiffnessRefVar;
	unguard;
}

//------------------------------------------------------------------------------
void USpringForce::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P )
{
	guard(USpringForce::NotifyParticleDestroyedEx);
	NOTE(debugf(TEXT("(%s)USpringForce::NotifyParticleDestroyedEx( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), P ? *P->GetClassName() : TEXT("NULL") ));

	if( P == ParticleA || P == ParticleB )
	{
		DeleteUObject();	// This will cause the spring to remove itself.
	}

	unguard;
}

//------------------------------------------------------------------------------
void USpringForce::Destroy()
{
	guard(USpringForce::Destroy);

	if( ParticleA )
	{
		DETACH_FORCEVARS(USpringForceVars,ParticleA);
		ParticleA = NULL;
	}

	if( ParticleB )
	{
		DETACH_FORCEVARS(USpringForceVars,ParticleB);
		ParticleB = NULL;
	}

	Super::Destroy();

	unguard;
}

//------------------------------------------------------------------------------
void USpringForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
    guard(USpringForce::Apply);
	NOTE(debugf(TEXT("(%s)USpringForce::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

	// safecheck.
	if( !ParticleA || !ParticleB ) return;
	if( StiffnessPtr ) Stiffness = *(FLOAT*)StiffnessPtr;	// Fix ARL: Make Stiffness private and use GetStiffness() and SetStiffness().

#if 1
	FVector Diff     = ParticleA->Location - ParticleB->Location;
	FVector Dir      = Diff.SafeNormal();
	FLOAT   DiffSize = Diff.Size();
	FVector Mid      = Dir * DiffSize/2.0f;
    FLOAT   DeltaX   = DiffSize - SpringLength;
	FLOAT   ForceMag = DeltaX * Stiffness;
    FVector Force    = Dir * ForceMag;
	{
		GET_FORCEVARS(USpringForceVars,ParticleA);
		Vars->SpringForce -= Force;
		Vars->PivotOffset -= Mid;
	}{
		GET_FORCEVARS(USpringForceVars,ParticleB);
		Vars->SpringForce += Force;
		Vars->PivotOffset += Mid;
	}
#elif 1
	FVector Diff = ParticleA->Location - ParticleB->Location;
    FLOAT DeltaX = Diff.Size() - SpringLength;
	FLOAT ForceMag = DeltaX * Stiffness;
    FVector Force = Diff.SafeNormal() * ForceMag;

	//SAFECHECK: This isn't desireable, but it does keep our equations from becoming unbounded.  Need better solution.
	FVector AppliedMove = Force * DeltaSeconds;
	FLOAT AppliedMag = AppliedMove.Size();
	if( AppliedMag > DeltaX )
	{
		Force = Diff.SafeNormal() * DeltaX / DeltaSeconds;
	}
	//SAFECHECK

    ParticleA->Velocity -= Force;
    ParticleB->Velocity += Force;
#else
	// Fix ARL: Account for anchored particles.
	FVector	Diff	= ParticleB->Location - ParticleA->Location;
	FVector	Dir		= Diff.SafeNormal();
	FLOAT	Size	= Diff.Size();
	FLOAT	x0		= (Size - SpringLength) / 2.f;
	FLOAT	w		= appSqrt( Stiffness );
	FLOAT	v0		= (ParticleA->Velocity * Dir).Size();
	FLOAT	Phi		= (w&&x0) ? appAtan( -v0 / (w * x0) ) : 0.f;
	FLOAT	A		= appSqrt( Square(x0) + (Square(v0)/Square(w)) );
	FLOAT	x		= A * appCos( w * DeltaSeconds + Phi );
	FVector NewLoc	= (x - x0) * Dir;
	ParticleA->Velocity += (NewLoc - ParticleA->Location) / DeltaSeconds;

	Diff	= ParticleA->Location - ParticleB->Location;
	Dir		= Diff.SafeNormal();
	Size	= Diff.Size();
	x0		= (Size - SpringLength) / 2.f;
	w		= appSqrt( Stiffness );
	v0		= (ParticleB->Velocity * Dir).Size();
	Phi		= (w&&x0) ? appAtan( -v0 / (w * x0) ) : 0.f;
	A		= appSqrt( Square(x0) + (Square(v0)/Square(w)) );
	x		= A * appCos( w * DeltaSeconds + Phi );
	NewLoc	= (x - x0) * Dir;
	ParticleB->Velocity += (NewLoc - ParticleB->Location) / DeltaSeconds;
#endif

    unguard;
}

//------------------------------------------------------------------------------
void USpringForce::Serialize( FArchive &Ar )
{
	guard(USpringForce::Serialize);

	Super::Serialize(Ar);

	if( !(Ar.IsLoading() && UClient::GetDefaultSettings()->ParticleSafeMode==3) )
		Ar << ParticleA << ParticleB;

	unguard;
}

IMPLEMENT_CLASS(USpringForce);

//------------------------------------------------------------------------------
void USpringLocator::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(USpringLocator::Apply);
	NOTE(debugf(TEXT("USpringLocator::Apply")));

	Super::Apply( System, DeltaSeconds );	// Called first so we don't screw up LastLocation.

	PARTICLE_ITERATOR_BEGIN

		GET_FORCEVARS(USpringForceVars,Particle);
		if( Vars )
		{
			FVector Offset = Vars->SpringForce * DeltaTime;
			FLOAT Dist = Offset.Size();
			FLOAT SpringLen = Vars->PivotOffset.Size() * 2.0f;

			if( Dist > SpringLen )
			{
				Particle->Location += Vars->PivotOffset;	// Fix ARL:  Placeholder.  This isn't right.  We need to wrap the force correctly.
			}
			else
			{
				Particle->Location += Offset;
			}

			Vars->SpringForce = FVector(0,0,0);
			Vars->PivotOffset = FVector(0,0,0);
		}

	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(USpringLocator);

//------------------------------------------------------------------------------
void UTarpCollider::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UTarpCollider::Apply);
	NOTE(debugf(TEXT("(%s)UTarpCollider::Apply( %s, %f )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

	ATarpBuilder* Parent = CastChecked<ATarpBuilder>(System);

	PARTICLE_ITERATOR_BEGIN

		UTarpParticle* TarpParticle = ParticleCastChecked<UTarpParticle>(Particle);

		if( !TarpParticle->Anchor && TarpParticle->Location != TarpParticle->LastLocation )
		{
			FCheckResult Hit(1.0f);

			System->GetLevel()->SingleLineCheck
			(	Hit
			,	System
			,	TarpParticle->Location
			,	TarpParticle->LastLocation
			,	TRACE_ProjTargets
			);

			if( Hit.Actor )
			{
				TarpParticle->Location = Hit.Location;
//				FVector Dir = (TarpParticle->Location - TarpParticle->LastLocation).SafeNormal();
//				TarpParticle->Location = Hit.Location - Dir * 5.f;
#if 1
//				if( (Hit.Normal | Dir) < -0.9f )	// Only lock on to near perpendicular surfaces.
				if( (Hit.Normal | FVector(0,0,-1)) < -0.9f )
				{
					UAnchorForce* Anchor = (UAnchorForce*)StaticConstructObject( UAnchorForce::StaticClass(), System );
					Anchor->bRotateWithActor = true;
					Anchor->Priority = Priority+1;
					Anchor->SetParticle( TarpParticle, System );
					Parent->ForcesToAdd.AddItem( Anchor );
					TarpParticle->Anchor = Anchor;

					// Bump up the priority on the slip forces for this particle.
					for( INT j=0; j<TarpParticle->SlipForces.Num(); j++ )
						TarpParticle->SlipForces(j)->Priority = GetDefault<USlipForce>()->Priority+0.5f;
					Parent->Forces.Sort();
				}
#endif
			}
		}

	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UTarpCollider);

//------------------------------------------------------------------------------
void UTriangleForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UTriangleForce::Apply);
	NOTE(debugf(TEXT("(%s)UTriangleForce::Apply( %s, %f)"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), DeltaSeconds ));

	TEMPLATE_ITERATOR_BEGIN
		if( Template->IsA( UTriangleParticleTemplate::StaticClass() ) )
		{
		PARTICLE_ITERATOR_BEGIN1

			NOTE(debugf(TEXT("    %s"), Particle ? *Particle->GetClassName() : TEXT("NULL") ));

			UTriangleParticle* P = ParticleCast<UTriangleParticle>(Particle);

			NOTE
			(
				debugf(TEXT("%s->AdditionalVars:"), *P->GetClassName() );
				for( TMap<UForce*,UForceVars*>::TIterator It(P->AdditionalVars); It; ++It )
					debugf(TEXT("    Key: %s Value: %s"), 
						It.Key() ? It.Key()->GetFullName() : TEXT("NULL"), 
						It.Value() ? *It.Value()->GetClassName() : TEXT("NULL") );
			);

			GET_FORCEVARS_CHECKED(UTriangleForceVars,P);

			FCoords UnCoords = Vars->CenterMass.Transpose();
			FVector R1 = Vars->P1Offset.TransformVectorBy( UnCoords );
			FVector R2 = Vars->P2Offset.TransformVectorBy( UnCoords );
			FVector R3 = Vars->P3Offset.TransformVectorBy( UnCoords );

#if 0 //OLD
			FVector F1 = P->Velocity     - Vars->P1LastVelocity;
			FVector F2 = P->P1->Velocity - Vars->P2LastVelocity;
			FVector F3 = P->P2->Velocity - Vars->P3LastVelocity;

			Vars->P1LastVelocity = P->Velocity;
			Vars->P2LastVelocity = P->P1->Velocity;
			Vars->P3LastVelocity = P->P2->Velocity;

			FVector CumulativeForce = F1 + F2 + F3;	// optionally divide by 3?
			Vars->Velocity += CumulativeForce;
#else
			Vars->CenterMass.Origin += (P->Velocity + P->P1->Velocity + P->P2->Velocity)/3.f * DeltaTime;
#endif

#if 0 //OLD
			FVector Torque1 = Vars->P1Offset^P->Velocity;
			FVector Torque2 = Vars->P2Offset^P->P1->Velocity;
			FVector Torque3 = Vars->P3Offset^P->P2->Velocity;

			FVector CumulativeTorque = Torque1 + Torque2 + Torque3;

			FLOAT AngularVelocity = CumulativeTorque.Size();

			FRotator Rot = CumulativeTorque.Rotation();
			Rot.Roll = (INT)(AngularVelocity * (65536.f/(2*PI)) * DeltaTime);

			FVector Origin = Vars->CenterMass.Origin;
			Vars->CenterMass.Origin = FVector(0,0,0);
			Vars->CenterMass *= Rot;
			Vars->CenterMass.Origin = Origin;
#else
			FVector Torque1 = R1^P->Velocity;
			FVector Torque2 = R2^P->P1->Velocity;
			FVector Torque3 = R3^P->P2->Velocity;

			FVector CumulativeTorque = Torque1 + Torque2 + Torque3;
			FLOAT AngularVelocity = CumulativeTorque.Size();

//			AngularVelocity /= 10000.f;

			//debugf( TEXT("%s->AngularVelocity: %f"), *P->GetClassName(), AngularVelocity );
			if( Abs(AngularVelocity) > 0.0001 )
			{
				FVector Origin = Vars->CenterMass.Origin;
				Vars->CenterMass.Origin = FVector(0,0,0);

//				FVector Dir = FVector(0,0,1);
				FRotator Rot = CumulativeTorque.Rotation();

				Vars->CenterMass *= Rot;

//				FRotator Roll = FRotator(0,0,65536*DeltaTime);
				FRotator Roll = FRotator(0,0, (INT)(AngularVelocity * DeltaTime) );
				Vars->CenterMass *= Roll;

				Vars->CenterMass /= Rot;

				Vars->CenterMass.Origin = Origin;
			}
#endif

			UnCoords = Vars->CenterMass.Transpose();
			P->Location     = Vars->P1Offset.TransformPointBy( UnCoords );
			P->P1->Location = Vars->P2Offset.TransformPointBy( UnCoords );
			P->P2->Location = Vars->P3Offset.TransformPointBy( UnCoords );
/*
			// Update Velocities.
			P->Velocity     = (P->Location     - P->LastLocation    )/DeltaTime;
			P->P1->Velocity = (P->P1->Location - P->P1->LastLocation)/DeltaTime;
			P->P2->Velocity = (P->P2->Location - P->P2->LastLocation)/DeltaTime;
*/
		PARTICLE_ITERATOR_END1
		}
	TEMPLATE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
void UTriangleForce::InitParticle( UParticle* Particle )
{
	guard(UTriangleForce::InitParticle);
	NOTE(debugf(TEXT("(%s)UTriangleForce::InitParticle( %s )"), GetFullName(), Particle ? *Particle->GetClassName() : TEXT("NULL") ));

	ATTACH_FORCEVARS(UTriangleForceVars,Particle);

	if( Particle->IsA( UTriangleParticle::StaticClass() ) )
	{
		UTriangleParticle* P = ParticleCast<UTriangleParticle>(Particle);

		NOTE
		(
			debugf(TEXT("%s->AdditionalVars:"), *P->GetClassName() );
			for( TMap<UForce*,UForceVars*>::TIterator It(P->AdditionalVars); It; ++It )
				debugf(TEXT("    Key: %s Value: %s"), 
					It.Key() ? It.Key()->GetFullName() : TEXT("NULL"), 
					It.Value() ? *It.Value()->GetClassName() : TEXT("NULL") );
		);

		Vars->CenterMass = FCoords( (P->Location+P->P1->Location+P->P2->Location)/3.f );

		Vars->P1Offset = P->Location    .TransformPointBy( Vars->CenterMass );
		Vars->P2Offset = P->P1->Location.TransformPointBy( Vars->CenterMass );
		Vars->P3Offset = P->P2->Location.TransformPointBy( Vars->CenterMass );

		Vars->P1LastVelocity = P->Velocity;
		Vars->P2LastVelocity = P->P1->Velocity;
		Vars->P3LastVelocity = P->P2->Velocity;

//		Vars->Velocity = P->Velocity + P->P1->Velocity + P->P2->Velocity;
	}

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UTriangleForce);
IMPLEMENT_CLASS(UTriangleForce);

//------------------------------------------------------------------------------
// Maintain the triangle's shape (weighing each vertex equally).
//
// 1) Find the tri's center (P1+P2+P3)/3
// 2) Scale verts in according to PnDist.  [obsolete]
// 3) Calc Theta01, Theta12 and Theta20 (angles between rays)
// 4) Calc Diffs (PMNAng - ThetaMN)
// 5) Calc Deltas (d1 = Diff01, d2 = d1+Diff12, d3 = d2+Diff20)
// 6) Calc Theta (@ = -(d1+d2+d3)/3)
// 7) Update rays (+= dN+Theta)
//------------------------------------------------------------------------------
// Math background:
//
// The goal is to move all three internal rays from their current orientation
// (after being affected by outside forces -- collision, gravity, etc)
// to their desired orientation -- so we maintain the triangle's shape -- 
// moving each ray as little as possible (so each vertex is weighted equally).
//
// To do this, we first find the amount we have to move each ray back in order
// to get it back to its original shape.  Then we can rotate the triangle
// uniformly such that ray movement is minimized.
//
// This gives us the following equation:
//     |d1+Theta| + |d2+Theta| + |d3+Theta| = min
// (Theta being the amount we're rotating the fixed triangle by.)
//
// Rather than deal with absolute values, we'll just square the whole thing:
//     (d1+Theta)^2 + (d2+Theta)^2 + (d3+Theta)^2 = min
// ==> d1^2 + 2*d1*Theta + Theta^2
//   + d2^2 + 2*d2*Theta + Theta^2
//   + d3^2 + 2*d3*Theta + Theta^2
//
// Take the derivative of this, setting it to zero and solving for Theta
// will give us the value of Theta that minimizes movement of the rays.
//
//     2*d1 + 2*d2 + 2*d3 + 6*Theta = 0
// ==> d1 + d2 + d3 + 3*Theta = 0
// ==> Theta = -(d1+d2+d3)/3         [You should recognize this equation above.]
//
// And now we can update the rays using the 'fixing' delta plus this new magic
// Theta which minimizes the distance each ray must move to maintain shape.
//------------------------------------------------------------------------------
void UTriForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UTriForce::Apply);
	NOTE(debugf(TEXT("UTriForce::Apply")));

	PARTICLE_ITERATOR_BEGIN

		// Get verts.
		UTriParticle* P0 = ParticleCastChecked<UTriParticle>(Particle);
		UParticle*    P1 = P0->P1;
		UParticle*    P2 = P0->P2;

		FVector Loc0 = P0->Location;
		FVector Loc1 = P1->Location;
		FVector Loc2 = P2->Location;
		
		// Find center.
		FVector Center = (Loc0+Loc1+Loc2)/3.f;
/*
		// Scale verts in to their correct distances from center.
		P0->Location = (Loc0 - Center).SafeNormal() * P0->P0Dist;							// Not necessary (see UpdateRay).
		P1->Location = (Loc1 - Center).SafeNormal() * P0->P0Dist;
		P2->Location = (Loc2 - Center).SafeNormal() * P0->P0Dist;
*/
		// Calc angles between rays.
		FRotator Rot = ((Loc1-Loc0)^(Loc2-Loc0)).Rotation();								// Tri's Normal vector.

		FVector Ray0 = (Loc0 - Center).TransformVectorBy( GMath.UnitCoords / Rot );			// Translate into 2D space.
		FVector Ray1 = (Loc1 - Center).TransformVectorBy( GMath.UnitCoords / Rot );
		FVector Ray2 = (Loc2 - Center).TransformVectorBy( GMath.UnitCoords / Rot );

		FLOAT Ang0 = appAtan2( Ray0.Y, Ray0.Z );											// Current angles.
		FLOAT Ang1 = appAtan2( Ray1.Y, Ray1.Z );
		FLOAT Ang2 = appAtan2( Ray2.Y, Ray2.Z );

		FLOAT Theta01 = AccuteDifference( Ang0, Ang1 );
		FLOAT Theta12 = AccuteDifference( Ang1, Ang2 );
		FLOAT Theta20 = AccuteDifference( Ang2, Ang0 );

		// Calc the diffs between current and desired.
		FLOAT Diff01 = P0->P01Ang - Theta01;
		FLOAT Diff12 = P0->P12Ang - Theta12;
		FLOAT Diff20 = P0->P20Ang - Theta20;

		// Calc the 'fixing' deltas.
		FLOAT d0 = Diff01;
		FLOAT d1 = Diff12 + d0;
		FLOAT d2 = Diff20 + d1;

		// Calc the minimizing Theta.
		FLOAT Theta = -(d0+d1+d2)/3.f;

		// Update the rays.
		Ang0 += d0 + Theta;
		Ang1 += d1 + Theta;
		Ang2 += d2 + Theta;

		P0->Location = Center + FVector(0,appSin(Ang0)*P0->P0Dist,appCos(Ang0)*P0->P0Dist).TransformVectorBy( GMath.UnitCoords * Rot );
		P1->Location = Center + FVector(0,appSin(Ang1)*P0->P1Dist,appCos(Ang1)*P0->P1Dist).TransformVectorBy( GMath.UnitCoords * Rot );
		P2->Location = Center + FVector(0,appSin(Ang2)*P0->P2Dist,appCos(Ang2)*P0->P2Dist).TransformVectorBy( GMath.UnitCoords * Rot );

	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UTriForce);

//------------------------------------------------------------------------------
void UVariablePerParticleGravity::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UVariablePerParticleGravity::Apply);

	PARTICLE_ITERATOR_BEGIN
		GET_FORCEVARS_CHECKED(UVariablePerParticleGravityVars,Particle);
		if( Vars->ChangeRate != 0.f )
		{
			Vars->Gravity += Vars->ChangeRate * DeltaTime;
			if( Vars->ChangeRate > 0 )
			{
				if( Vars->Gravity > Vars->EndGravity )
				{
					Vars->Gravity = Vars->EndGravity;
					Vars->ChangeRate = 0.f;
				}
			}
			else
			{
				if( Vars->Gravity < Vars->EndGravity )
				{
					Vars->Gravity = Vars->EndGravity;
					Vars->ChangeRate = 0.f;
				}
			}
		}
		Particle->Velocity.Z -= Vars->Gravity * DeltaTime;
	PARTICLE_ITERATOR_END

	unguard;
}

//------------------------------------------------------------------------------
void UVariablePerParticleGravity::InitParticle( UParticle* Particle )
{
	guard(UVariablePerParticleGravity::InitParticle);

	FLOAT Time = ChangeTime.GetRand();

	ATTACH_FORCEVARS(UVariablePerParticleGravityVars,Particle);
	Vars->Gravity = StartGravity.GetRand();
	Vars->EndGravity = EndGravity.GetRand();
	if( Time > 0.f )
	{
		Vars->ChangeRate = (Vars->EndGravity - Vars->Gravity) / Time;
	}
	else
	{
		Vars->ChangeRate = 0.f;
		Vars->Gravity = Vars->EndGravity;
	}

	NOTE(debugf(TEXT("UVariablePerParticleGravity::InitParticle( %s ): Time = %f | ChangeRate = %f | StartGravity = %f | EndGravity = %f"), *Particle->GetClassName(), Time, Vars->ChangeRate, Vars->Gravity, Vars->EndGravity ));

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UVariablePerParticleGravity);
IMPLEMENT_CLASS(UVariablePerParticleGravity);

//------------------------------------------------------------------------------
void UVoidDestroyer::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UVoidDestroyer::Apply);
	NOTE(debugf(TEXT("UVoidDestroyer::Apply")));

	UModel* Model = System->GetLevel()->Model;
	ALevelInfo* LevelInfo = System->GetLevel()->GetLevelInfo();
	
	PARTICLE_ITERATOR_BEGIN

		if( Model->PointRegion( LevelInfo, Particle->Location ).ZoneNumber == 0 )
		{
			Particle->LifeSpan = 0.f;
		}

	PARTICLE_ITERATOR_END

	unguard;
}

IMPLEMENT_CLASS(UVoidDestroyer);

//------------------------------------------------------------------------------
void UWindForce::Apply( AParticleGenerator* System, FLOAT DeltaSeconds )
{
	guard(UWindForce::Apply);
	NOTE(debugf(TEXT("UWindForce::Apply")));

	if( Master )
	{
	PARTICLE_ITERATOR_BEGIN

		GET_FORCEVARS_CHECKED(UWindForceVars,Particle);

		Particle->Velocity -= Vars->LastWindspeed;
		Vars->LastWindspeed = Master->Windspeed;
		Particle->Velocity += Vars->LastWindspeed;

	PARTICLE_ITERATOR_END
	}

	unguard;
}

//------------------------------------------------------------------------------
void UWindForce::InitParticle( UParticle* Particle )
{
	guard(UWindForce::InitParticle);
	NOTE(debugf(TEXT("(%s)UWindForce::InitParticle( %s )"), GetFullName(), Particle ? *Particle->GetClassName() : TEXT("NULL") ));

	ATTACH_FORCEVARS(UWindForceVars,Particle);

	Vars->LastWindspeed = FVector(0,0,0);

	unguard;
}

IMPLEMENT_ADDITIONALVARS_CLASS(UWindForce);
IMPLEMENT_CLASS(UWindForce);
IMPLEMENT_CLASS(ANewWind);


////////////////////////////
// Additional duplication //
////////////////////////////

//------------------------------------------------------------------------------
UForce* USlipForce::Duplicate( UObject* NewOuter )
{
	USlipForce* NewForce = CastChecked<USlipForce>(Super::Duplicate(NewOuter));
	if(ParticleA) NewForce->ParticleA = ParticleA->Duplicate(NewOuter);
	if(ParticleB) NewForce->ParticleB = ParticleA->Duplicate(NewOuter);
	return NewForce;
}

//------------------------------------------------------------------------------
UForce* UMeshAnchor::Duplicate( UObject* NewOuter )
{
	UMeshAnchor* NewForce = CastChecked<UMeshAnchor>(Super::Duplicate(NewOuter));
	if(ParticleA) NewForce->ParticleA = ParticleA->Duplicate(NewOuter);
	return NewForce;
}

//------------------------------------------------------------------------------
UForce* UBoneLocatorForce::Duplicate( UObject* NewOuter )
{
	UBoneLocatorForce* NewForce = CastChecked<UBoneLocatorForce>(Super::Duplicate(NewOuter));
	if(ParticleA) NewForce->ParticleA = ParticleA->Duplicate(NewOuter);
	return NewForce;
}

//------------------------------------------------------------------------------
UForce* UAttractionForce::Duplicate( UObject* NewOuter )
{
	UAttractionForce* NewForce = CastChecked<UAttractionForce>(Super::Duplicate(NewOuter));
	if(ParticleA) NewForce->ParticleA = ParticleA->Duplicate(NewOuter);
	if(ParticleB) NewForce->ParticleB = ParticleB->Duplicate(NewOuter);
	return NewForce;
}

//------------------------------------------------------------------------------
UForce* UAnchorForce::Duplicate( UObject* NewOuter )
{
	UAnchorForce* NewForce = CastChecked<UAnchorForce>(Super::Duplicate(NewOuter));
	if(ParticleA) NewForce->ParticleA = ParticleA->Duplicate(NewOuter);
	return NewForce;
}

//------------------------------------------------------------------------------
UForce* USpringForce::Duplicate( UObject* NewOuter )
{
	USpringForce* NewForce = CastChecked<USpringForce>(Super::Duplicate(NewOuter));
	if(ParticleA) NewForce->ParticleA = ParticleA->Duplicate(NewOuter);
	if(ParticleB) NewForce->ParticleB = ParticleA->Duplicate(NewOuter);
	return NewForce;
}

