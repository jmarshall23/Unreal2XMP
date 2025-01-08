#include "ParticleSystems.h"

void AParticleSalamander::ParticleTick( FLOAT DeltaSeconds, FVector& Location, FRotator& Rotation, FLOAT AdditionalStasisTime )
{
	guard(AParticleSalamander::ParticleTick);
	NOTE(debugf(TEXT("AParticleSalamander::ParticleTick"));)

	// Local variables.
	FLOAT ScaledVolume;
	FLOAT SecondsPerParticle;
	UParticle* Particle;
	UParticleTemplate* Template;
	
	// Scale the volume based on the global ParticleDensity value from MinVolume to Volume.
	ScaledVolume = GetVolume();
	
/* Fix ARL: We need access to the camera in the tick function for this to work.
	// Scale volume based on the camera's distance from particle source, relational to VolumeScalePct and Visibility values.
	VolumeScalePct = Clamp( VolumeScalePct, 0.f, 1.f );
	if( Frame && bOn && VolumeScalePct )
	{
		if( VisibilityRadius )
		{
			FLOAT RadiusDistance = appSqrt((Location - Frame->ViewOrigin).SizeSquared2D());
			if( RadiusDistance < VisibilityRadius )
			{
				FLOAT ScaledRadius = VisibilityRadius * VolumeScalePct;
				ScaledVolume *= (RadiusDistance > VisibilityRadius - ScaledRadius ? (VisibilityRadius - RadiusDistance) / ScaledRadius : 1.f);
			}
			else
			{
				ScaledVolume = 0.f;
			}
		}
		if( VisibilityHeight )
		{
			FLOAT HeightDistance = Abs((Location - Frame->ViewOrigin).Z );
			if( HeightDistance < VisibilityHeight )
			{
				FLOAT ScaledHeight = VisibilityHeight * VolumeScalePct;
				ScaledVolume *= (HeightDistance > VisibilityHeight - ScaledHeight ? (VisibilityHeight - HeightDistance) / ScaledHeight : 1.f);
			}
			else
			{
				ScaledVolume = 0.f;
			}
		}
	}
*/
	//debugf( "(%s)ScaledVolume: %f", GetName(), ScaledVolume ); 

	//
	// Create new particles as needed.
	//

	if
	(	bOn
	&&	ScaledVolume > 0.0
	&&	ParticleTemplates.Num() > 0
	)
	{
		SecondsPerParticle = 1.f / ScaledVolume;

		FCoords Coords = GMath.UnitCoords * Rotation;

		ParticleTimer += DeltaSeconds;
		while( ParticleTimer >= SecondsPerParticle )
		{
			// Update the timer according to the set Volume.
			ParticleTimer -= SecondsPerParticle;

			// Get the next ParticleTemplate.
			Template = GetNextTemplate();
			if( !Template ) break;	// fail silently (if there are no valid templates defined).

			// Get a new (initialized) Particle from the ParticleManager.
			Particle = Template->GetParticle();

			// Set initial position and direction.
			FVector Dir = RandomSpreadVector( Spread ).SafeNormal().TransformVectorBy( Coords );
			Template->SetParticleDirection( Particle, Dir );

			FVector Loc = Location + Dir * EmisionOffset.GetRand();
			Template->SetParticleLocation( Particle, Loc );

			// Hack to limit particle's from going past a specified object (should be moved elsewhere).
			if( Template->LifeSpanLimitor )
				Particle->LifeSpan = (Template->LifeSpanLimitor->Location - Particle->Location).Size() / Particle->Velocity.Size();

			// Evenly distribute new particles.
			Particle->StasisTimer += (DeltaSeconds - ParticleTimer) + AdditionalStasisTime;
		}
	}

	unguardf(( TEXT("(%s)"), GetFullName() ));
}

IMPLEMENT_CLASS(AParticleSalamander);