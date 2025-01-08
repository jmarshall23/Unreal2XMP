
// native vars.
class particleClass* ParticleClass;

// interface.
virtual void InitParticle( UParticle* &P );
virtual INT Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System ){ return 0; }
virtual void Tick( AParticleGenerator* System, FLOAT DeltaSeconds ){}
virtual UParticleTemplate* Duplicate( UObject* NewOuter );
virtual void NotifyAddedToSystem( AParticleGenerator* System ){}
virtual void NotifyRemovedFromSystem( AParticleGenerator* System ){}
virtual void NotifyForceAttachment( UForce* F );
virtual void NotifyForceDetachment( UForce* F );
virtual void SetParticleDirection( UParticle* P, FVector Direction );
virtual void SetParticleLocation( UParticle* P, FVector Location );
virtual void SetParticleSize( UParticle* P, FLOAT Size ){}
virtual FLOAT GetParticleSize( UParticle* P ){ return 0.f; }
virtual FVector GetParticleNormal( UParticle* P );
virtual FLOAT GetMaxLifeSpan( AParticleGenerator* System ){ return InitialLifeSpan.GetMax() + StasisTime.GetMax(); }

virtual void RecycleParticle( class UParticle* P );

UParticle* GetParticle();
void Clean();

void Serialize( FArchive& Ar );
void Destroy();

// This is backwards because this needs to be sorted smallest to largest.
UBOOL IsLessThan( const UObject* A ) const { return DrawOrder > CastChecked<UParticleTemplate>(A)->DrawOrder; }


/*--------------------------------------------------------------------------
	Scratch vars.
--------------------------------------------------------------------------*/

// Fix ARL: Get rid of these.
#define MAX_PARTICLE_VERTICES 1024
static FVector Pts[MAX_PARTICLE_VERTICES*3]; // "* 3" is for AddQuadStrip code
static QuadPointInfo QuadPts[MAX_PARTICLE_VERTICES];  // "* 3" is for AddQuadStrip code

/*--------------------------------------------------------------------------
	Hardware lighting.
--------------------------------------------------------------------------*/

UBOOL NeedsHardwareLighting();
void SetupLighting( FRenderInterface* RI, TList<FDynamicLight*>* DynLights );
void TeardownLighting( FRenderInterface* RI );

/*--------------------------------------------------------------------------
	Software lighting.
--------------------------------------------------------------------------*/

FColor Light( UParticle* Particle, TList<FDynamicLight*>* Lights );

