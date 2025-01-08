
void SetEndpoints( UParticle* A, UParticle* B );
void Apply( AParticleGenerator* System, FLOAT DeltaTime );
void NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P );

void Destroy();

void Serialize( FArchive& Ar );

UForce* Duplicate( UObject* NewOuter );

