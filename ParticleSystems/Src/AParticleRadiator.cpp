#include "ParticleSystems.h"

// Fix ARL: Most of this really needs to be factored out to a separate class with ParticleSalamander.

// Fix ARL: Interpolate vertex normals to generate smother direction vectors for particle velocities.

#define RADIATOR_ERROR(err) return;
//#define RADIATOR_ERROR(err) { debugf(TEXT("(%s) %s: %s"), GetFullName(), SklMesh ? SklMesh->GetFullName() : TEXT("NULL"), TEXT(err)); return; }

void AParticleRadiator::ParticleTick( FLOAT DeltaSeconds, FVector& Location, FRotator& Rotation, FLOAT AdditionalStasisTime )
{
	guard(AParticleRadiator::ParticleTick);
	NOTE(debugf(TEXT("(%s)AParticleRadiator::ParticleTick"), GetFullName() ));

	//
	// Get the appropriate actor to use the mesh from.
	//

	AActor* SklOwner = MeshOwner ? MeshOwner : this;
	if(bStopIfHidden && SklOwner->bHidden)
		return;

	//
	// Get mesh data.
	//

	USkeletalMesh* SklMesh = Cast<USkeletalMesh>(SklOwner->Mesh);
	if(!SklMesh) RADIATOR_ERROR("Mesh not set");

	USkeletalMeshInstance* SklMeshInstance = CastChecked<USkeletalMeshInstance>(SklMesh->MeshGetInstance(SklOwner));
	if(!SklMeshInstance) RADIATOR_ERROR("MeshInstance not set");
	FMatrix MeshToWorld = SklMeshInstance->MeshToWorld();

	if(!SklMesh->LODModels.Num()) RADIATOR_ERROR("No LOD levels");
	FStaticLODModel* Model = &SklMesh->LODModels( Min( LODLevel, SklMesh->LODModels.Num()-1 ) );
	INT NumWedges = Model->SmoothStreamWedges;

	clock(GStats.DWORDStats(GEngineStats.STATS_Particle_RadiatorTick));

	FMemMark Mark(GMem);
	FVector* Verts=NULL;
	INT NumFaces=0;

	UBOOL bMeshInitialized=false;

	//
	// Scale the volume based on the global ParticleDensity value from MinVolume to Volume.
	//

	FLOAT ScaledVolume = GetVolume();

	//
	// Create new particles as needed.
	//

	if
	(	bOn
	&&	ScaledVolume > 0.f
	&&	ParticleTemplates.Num() > 0
	)
	{
		FLOAT SecondsPerParticle = 1.f / ScaledVolume;

		ParticleTimer += DeltaSeconds;
		while( ParticleTimer >= SecondsPerParticle )
		{
			// Update the timer according to the set Volume.
			ParticleTimer -= SecondsPerParticle;

			// Get the next ParticleTemplate.
			UParticleTemplate* Template = GetNextTemplate();
			if(!Template) break;	// fail silently (if there are no valid templates defined).

			// Get a new (initialized) Particle from the ParticleManager.
			UParticle* Particle = Template->GetParticle();

			// Initialize mesh data.  (this is real slow, so do it at the last possible moment)
			if( !bMeshInitialized )
			{
				bMeshInitialized=true;

				// GetMeshVerts for skeletal meshes returns vertices in order of vertex buffer (i.e wedges; textured vertices.)
				Verts = New<FVector>(GMem,NumWedges);
				SklMeshInstance->GetMeshVerts( SklOwner, Verts, sizeof(FVector), NumWedges );

				// Ensure lazy arrays are loaded.
				Model->Wedges.Load();
				Model->Faces.Load();

				NumFaces = Model->Faces.Num();
			}

			// Select triangle.
			switch( TriSelectMethod )
			{
			case TS_Random:		iTri = appRand();									break;
			case TS_Linear:		iTri++;												break;
			case TS_Indexed:	iTri = TriIndices(iIndex++ % TriIndices.Num());		break;
			case TS_RandIndex:	iTri = TriIndices(appRand() % TriIndices.Num());	break;
			}
			iTri = iTri % NumFaces;
			FMeshFace& Face = Model->Faces(iTri);

			// Get current tri vert locations.
			FVector A = MeshToWorld.TransformFVector( Verts[ Face.iWedge[0] ] );
			FVector B = MeshToWorld.TransformFVector( Verts[ Face.iWedge[1] ] );
			FVector C = MeshToWorld.TransformFVector( Verts[ Face.iWedge[2] ] );

			// Calculate the spawn location.
			FVector P;
			if( bOrgiSpawn )	// Use triangle's midpoint.
			{
				P = (A + B + C) / 3.f;
			}
			else				// Randomly distribute across the surface of the triangle.
			{
				// Generate three randomly distributed numbers such that a+b+c=1.
				FLOAT a = appFrand();
				FLOAT b = appFrand();
				if( a+b > 1.f )
				{
					a = 1.f - a;
					b = 1.f - b;
				}
				FLOAT c = 1.f - a - b;

				P = A*a + B*b + C*c;
			}

			// Calculate the tri's normal vector.
			FVector Normal = ((A - B) ^ (C - A)) * 1000.f;
			Normal = Normal / appSqrt( Normal.SizeSquared() + 0.0001f );

			// Set initial position and direction.
			Template->SetParticleDirection( Particle, Normal );
			Template->SetParticleLocation( Particle, P );

			// Hack to limit particle's from going past a specified object (should be moved elsewhere).
			if( Template->LifeSpanLimitor )
				Particle->LifeSpan = (Template->LifeSpanLimitor->Location - Particle->Location).Size() / Particle->Velocity.Size();

			// Randomly distribute new particles.
			Particle->StasisTimer += (DeltaSeconds - ParticleTimer);
		}
	}

	Mark.Pop();

	unclock(GStats.DWORDStats(GEngineStats.STATS_Particle_RadiatorTick));
	unguardf(( TEXT("(%s)"), GetFullName() ));
}


IMPLEMENT_CLASS(AParticleRadiator);

