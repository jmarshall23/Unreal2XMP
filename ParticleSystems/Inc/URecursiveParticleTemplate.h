
void InitParticle( UParticle* &P );
INT Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System );
void SetParticleDirection( UParticle* P, FVector Direction );
void SetParticleLocation( UParticle* P, FVector Location );

void RecycleParticle( class UParticle* P );

