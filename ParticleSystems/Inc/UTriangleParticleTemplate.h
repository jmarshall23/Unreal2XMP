
void InitParticle( UParticle* &P );
INT Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System );
UParticleTemplate* Duplicate( UObject* NewOuter );
void NotifyAddedToSystem( AParticleGenerator* System );
void SetParticleDirection( UParticle* P, FVector Direction );
void SetParticleLocation( UParticle* P, FVector Location );

