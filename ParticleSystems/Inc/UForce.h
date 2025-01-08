
void Destroy();

virtual void AddTemplate( UParticleTemplate* T );
virtual void RemoveTemplate( UParticleTemplate* T, UBOOL bNoRemove=false );

virtual void AddExternalTemplate( UParticleTemplate* T );
virtual void RemoveExternalTemplate( UParticleTemplate* T, UBOOL bNoRemove=false );

virtual void Apply( AParticleGenerator* System, FLOAT DeltaTime ) {}

virtual void NotifyAttachedToSystem( AParticleGenerator* System ) {}
virtual void NotifyExternallyAttached( UParticleTemplate* Template ) {}
virtual void NotifyParticleCreated( AParticleGenerator* System, UParticle* &P ) {}

virtual void NotifyDetachedFromSystem( AParticleGenerator* System );
virtual void NotifyExternallyDetached( UParticleTemplate* Template ) {}
virtual void NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P ) {}

virtual UForce* Duplicate( UObject* NewOuter );

virtual FLOAT GetMaxLifeSpan( AParticleGenerator* System ){ return 0.0f; }	// override if you affect how long a particle system should be displayed (after it's turned off).

UBOOL IsLessThan( const UObject* A ) const { return Priority < CastChecked<UForce>(A)->Priority; }

