#include "ParticleSystems.h"

//------------------------------------------------------------------------------
void AParticleAffector::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI )
{
	guard(AParticleAffector::Render);
	NOTE(debugf(TEXT("AParticleAffector::Render")));

	LastTimeDrawn = GetLevel()->GetLevelInfo()->TimeSeconds;	

	unguard;
}

//------------------------------------------------------------------------------
void AParticleAffector::ForceTick( FLOAT DeltaSeconds )
{
	guard(AParticleAffector::ForceTick);
	NOTE(debugf(TEXT("(%s)AParticleAffector::Render"),GetFullName()));

	if (!bOn) return;

	for( INT i=0; i<Forces.Num(); i++ )
		if( Forces(i) )
			Forces(i)->Apply( this, DeltaSeconds );

	unguard;
}

IMPLEMENT_CLASS(AParticleAffector);

//------------------------------------------------------------------------------
void AParticleBlower::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI )
{
	guard(AParticleBlower::Render);
	NOTE(debugf(TEXT("AParticleBlower::Render")));

	LastTimeDrawn = GetLevel()->GetLevelInfo()->TimeSeconds;	

	unguard;
}

IMPLEMENT_CLASS(AParticleBlower);

// Fix ARL: Factor code common with ParticleColliderSphere to a common superclass.

//------------------------------------------------------------------------------
UBOOL AParticleColliderDisc::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(AParticleColliderDisc::Tick);
	NOTE(debugf(TEXT("AParticleColliderDisc::Tick")));

	if( bCollectParticleGenerators )
	{
		CollectGenerators();
		bCollectParticleGenerators = false;
	}

	return Super::Tick( DeltaSeconds, TickType );

	unguard;
}

//------------------------------------------------------------------------------
void AParticleColliderDisc::CollectGenerators()
{
	guard(AParticleColliderDisc::CollectGenerators);

	// Remove existing.
	for( INT i = 0; i < AffectedSystems.Num(); i++ )
	{
		for( INT j = 0; j < AffectedSystems(i)->Forces.Num(); j++ )
		{
			UCollisionDiscForce* Force = Cast<UCollisionDiscForce>( AffectedSystems(i)->Forces(j) );
			if( Force && Force->Parent == this )
			{
				// remove force.
				Force->DeleteUObject();
				--j;
			}
		}
//		AffectedSystems(i)->DetachDestroyObserver( this );
	}
	AffectedSystems.Empty();

	// Attach new.
	for( INT i = 0; i < GetLevel()->Actors.Num(); i++ )
	{
		AParticleGenerator* Gen = Cast<AParticleGenerator>( GetLevel()->Actors(i) );
		if( Gen )
		{
			if( CullDistance == 0.f || (Gen->Location - Location).Size() <= CullDistance )
			{
				AffectedSystems.AddItem( Gen );
				UCollisionDiscForce* Force = (UCollisionDiscForce*)StaticConstructObject( UCollisionDiscForce::StaticClass(), Gen );
				Force->Parent = this;
				Gen->AddForce( Force );
//				Gen->AttachDestroyObserver( this );
			}
		}
	}

	unguard;
}

// SubjectDestroyed -- remove from AffectedSystems

IMPLEMENT_CLASS(AParticleColliderDisc);

//------------------------------------------------------------------------------
UBOOL AParticleColliderSphere::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(AParticleColliderSphere::Tick);
	NOTE(debugf(TEXT("AParticleColliderSphere::Tick")));

	if( bCollectParticleGenerators )
	{
		CollectGenerators();
		bCollectParticleGenerators = false;
	}

	return Super::Tick( DeltaSeconds, TickType );

	unguard;
}

//------------------------------------------------------------------------------
void AParticleColliderSphere::CollectGenerators()
{
	guard(AParticleColliderSphere::CollectGenerators);

	// Remove existing.
	for( INT i = 0; i < AffectedSystems.Num(); i++ )
	{
		for( INT j = 0; j < AffectedSystems(i)->Forces.Num(); j++ )
		{
			UCollisionSphereForce* Force = Cast<UCollisionSphereForce>( AffectedSystems(i)->Forces(j) );
			if( Force && Force->Parent == this )
			{
				// remove force.
				Force->DeleteUObject();
				--j;
			}
		}
//		AffectedSystems(i)->DetachDestroyObserver( this );
	}
	AffectedSystems.Empty();

	// Attach new.
	for( INT i = 0; i < GetLevel()->Actors.Num(); i++ )
	{
		AParticleGenerator* Gen = Cast<AParticleGenerator>( GetLevel()->Actors(i) );
		if( Gen )
		{
			if( CullDistance == 0.f || (Gen->Location - Location).Size() <= CullDistance )
			{
				AffectedSystems.AddItem( Gen );
				LoadClass<UCollisionSphereForce>( NULL, TEXT("ParticleSystems.CollisionSphereForce"), NULL, LOAD_NoWarn, NULL );	// Fix ARL: Needed?
				UCollisionSphereForce* Force = (UCollisionSphereForce*)StaticConstructObject( UCollisionSphereForce::StaticClass(), Gen );
				Force->Parent = this;
				Gen->AddForce( Force );
//				Gen->AttachDestroyObserver( this );
			}
		}
	}

	unguard;
}

// SubjectDestroyed -- remove from AffectedSystems

IMPLEMENT_CLASS(AParticleColliderSphere);

