
void Apply( AParticleGenerator* System, FLOAT DeltaTime );
UForce* Duplicate( UObject* NewOuter );

FLOAT GetMaxLifeSpan( AParticleGenerator* System );

void Destroy();

void AddImpactEffect( UParticleTemplate* T );

virtual void CollisionNotification( UParticle* P, FCheckResult &Hit, UParticleTemplate* Template ){}

