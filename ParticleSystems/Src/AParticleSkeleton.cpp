#include "ParticleSystems.h"

//------------------------------------------------------------------------------
void AParticleSkeleton::Reset()
{
	guard(AParticleSkeleton::Reset);
	NOTE(debugf(TEXT("(%s)AParticleSkeleton::Reset"), GetFullName() ));

	if( AnchorTemplate )
	{
		AnchorTemplate->DeleteUObject();	// This will kill off all BoneLocatorForces and AttractionForces.
		AnchorTemplate = NULL;
	}

	for( INT i=0; i<ParticleTemplates.Num(); i++ )
		ParticleTemplates(i)->Clean();

	unguard;
}

//------------------------------------------------------------------------------
void AParticleSkeleton::ParticleTick( FLOAT DeltaSeconds, FVector& Location, FRotator& Rotation, FLOAT AdditionalStasisTime )
{
	guard(AParticleSkeleton::ParticleTick);
	NOTE(debugf(TEXT("(%s)AParticleSkeleton::ParticleTick"), GetFullName() ));

	if( bInitialize )
	{
		bInitialize = false;

		// Clean up existing stuff.
		Reset();

		// Don't continue unless we have at least one user defined particle template.
		if( !ParticleTemplates.Num() )
			return;

		// Create new.
		AnchorTemplate = CastChecked<UParticleTemplate>(StaticConstructObject( UParticleTemplate::StaticClass(), this ));
		AnchorTemplate->SelectionWeight = 0.0f;	// don't select particles from this template unless we explicitly request one.
		AddTemplate( AnchorTemplate );

		for( INT i=0; i<NumParticles; i++ )
		{
			UParticle* Anchor = AnchorTemplate->GetParticle();

			UBoneLocatorForce* BoneLocator = CastChecked<UBoneLocatorForce>(StaticConstructObject( UBoneLocatorForce::StaticClass(), this ));
			AddForce( BoneLocator );
			BoneLocator->BoneIndex = appRand();
			BoneLocator->SetParticle( Anchor );

			// Get the next ParticleTemplate.
			UParticleTemplate* Template = GetNextTemplate();
			if( !Template ) break;	// fail silently (if there are no valid templates defined).
			if( Template==AnchorTemplate ) break;	// should never happen due to SelectionWeight, but just in case.

			// Get a new (initialized) Particle from the ParticleManager.
			UParticle* Particle = Template->GetParticle();

			UAttractionForce* AttractionForce = CastChecked<UAttractionForce>(StaticConstructObject( UAttractionForce::StaticClass(), this ));
			AddForce( AttractionForce );
			AttractionForce->Rigidity = Rigidity.GetRand();
			AttractionForce->SetEndpoints( Anchor, Particle );
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
AParticleGenerator* AParticleSkeleton::Duplicate( ULevel* Level )
{
	guard(AParticleSkeleton::Duplicate);
	NOTE(debugf(TEXT("(%s)AParticleSkeleton::Duplicate"), GetFullName() ));

	AParticleSkeleton* NewGenerator = Cast<AParticleSkeleton>(Super::Duplicate( Level ));

	if( NewGenerator )
		NewGenerator->Reset();

	return NewGenerator;

	unguard;
}


IMPLEMENT_CLASS(AParticleSkeleton);

