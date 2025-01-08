
void Apply( AParticleGenerator* System, FLOAT DeltaTime );
void SetParticle( UParticle* A, AActor* R );
void NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &P );

void Serialize( FArchive& Ar );
UForce* Duplicate( UObject* NewOuter );

