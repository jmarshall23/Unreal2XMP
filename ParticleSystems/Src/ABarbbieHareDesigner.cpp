#include "ParticleSystems.h"

void ABarbbieHareDesigner::ParticleTick( FLOAT DeltaSeconds, FVector& Location, FRotator& Rotation, FLOAT AdditionalStasisTime )
{
	guard(ABarbbieHareDesigner::ParticleTick);
	NOTE(debugf(TEXT("ABarbbieHareDesigner::ParticleTick")));

	if( bRecalc )
	{
		bRecalc = false;

		// Remove old stuff (springforces/anchorforces)
		// Make sure there is no DecayForce attached.
		for( INT i = 0; i < Forces.Num(); /*below*/ )
			if( Cast<USpringForce>(Forces(i)) )
				Forces.Remove(i);
			else if( Cast<UAnchorForce>(Forces(i)) )
				Forces.Remove(i);
			else i++;

		// Remove any existing StrandParticleTemplates.
		for( INT i = 0; i < ParticleTemplates.Num(); /*below*/ )
			if( Cast<UStrandParticleTemplate>(ParticleTemplates(i)) )
				ParticleTemplates.Remove(i);
			else i++;

		UStrandParticleTemplate* Template = (UStrandParticleTemplate*)StaticConstructObject( UStrandParticleTemplate::StaticClass(), this );
		ParticleTemplates.AddItem( Template );

		UParticleTemplate* DummyTemplate = (UParticleTemplate*)StaticConstructObject( UParticleTemplate::StaticClass(), this );
		ParticleTemplates.AddItem( DummyTemplate );

		for( INT i = 0; i < NumStrands; i++ )
		{
			// Create root particle.
			UStrandParticle* Root = (UStrandParticle*)Template->GetParticle();
			UStrandParticle* Particle = Root;
			FVector Dir = RandomSpreadVector( Spread ).SafeNormal().TransformVectorBy( GMath.UnitCoords * Rotation );
			FVector BaseLocation = Location + Dir * Radius;
			Particle->Location = BaseLocation;
			Particle->bRoot = true;

			// Anchor root.
			UAnchorForce* Anchor = (UAnchorForce*)StaticConstructObject( UAnchorForce::StaticClass(), this );
			Anchor->bRotateWithActor = true;
			Anchor->SetParticle( Particle, this );
			AddForce( Anchor );

			// Add additional segments to strand.
			FLOAT SegLength = Length.GetRand() / NumSegments;
			FVector SegOffset = SegLength * Dir;
			for( INT j = 0; j < NumSegments; j++ )
			{
				Particle->NextStrandParticle = (UStrandParticle*)Template->GetParticle();
				Particle = Particle->NextStrandParticle;
				BaseLocation += SegOffset;
				Particle->Location = BaseLocation;

				// Spring/Achor secondary particles as well.
				if( j == 0 )
				{
					UParticle* Dummy = DummyTemplate->GetParticle();
					Dummy->Location = Particle->Location;

					UAnchorForce* Anchor = (UAnchorForce*)StaticConstructObject( UAnchorForce::StaticClass(), this );
					Anchor->bRotateWithActor = true;
					Anchor->SetParticle( Dummy, this );
					AddForce( Anchor );

					USpringForce* Spring = (USpringForce*)StaticConstructObject( USpringForce::StaticClass(), this );
					Spring->SetEndpoints( Particle, Dummy );
					Spring->Stiffness = BendStrength.GetRand();
					AddForce( Spring );
				}
			}

			// Hook up springs.
			FLOAT BendCo = BendStrength.GetRand();
			FLOAT StretchCo = Stretchiness.GetRand();
			for( Particle = Root; Particle->NextStrandParticle; Particle = Particle->NextStrandParticle )
			{
				// Stretchy spring (on consecutive particles).
				USpringForce* Spring = (USpringForce*)StaticConstructObject( USpringForce::StaticClass(), this );
				Spring->SetEndpoints( Particle, Particle->NextStrandParticle );
				Spring->Stiffness = StretchCo;
				AddForce( Spring );

				// Bendy spring (on alternating particles).
				if( Particle->NextStrandParticle->NextStrandParticle )
				{
					USpringForce* Spring = (USpringForce*)StaticConstructObject( USpringForce::StaticClass(), this );
					Spring->SetEndpoints( Particle, Particle->NextStrandParticle->NextStrandParticle );
					Spring->Stiffness = BendCo;
					AddForce( Spring );
				}
			}
		}
	}

	unguard;
}

IMPLEMENT_CLASS(ABarbbieHareDesigner);