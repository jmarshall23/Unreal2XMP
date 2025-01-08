
void InitParticle( UParticle* &P );
INT Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System );
void SetParticleDirection( UParticle* P, FVector Direction );
FVector GetParticleNormal( UParticle* P );
FLOAT GetParticleSize( UParticle* P );
void SetParticleSize( UParticle* P, FLOAT Size );

