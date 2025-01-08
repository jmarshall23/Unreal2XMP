
void InitParticle( UParticle* &P );
INT Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System );

virtual INT DrawCoronas( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System );
UBOOL IsVisible( AParticleGenerator* System, class USpriteParticle* Particle, FVector& LocalLoc, FLevelSceneNode* Frame );

FLOAT GetParticleSize( UParticle* P );
void SetParticleSize( UParticle* P, FLOAT Size );

void UpdateIndex( UParticle* Particle );

