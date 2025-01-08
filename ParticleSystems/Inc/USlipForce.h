
void SetEndpoints( UParticle* A, UParticle* B, FLOAT SlipDistance=0 );
void Apply( AParticleGenerator* System, FLOAT DeltaTime );
void NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P );

void Serialize( FArchive& Ar );
UForce* Duplicate( UObject* NewOuter );

