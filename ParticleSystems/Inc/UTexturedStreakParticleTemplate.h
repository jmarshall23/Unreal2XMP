
void InitParticle( UParticle* &P );
INT Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System );
void Tick( AParticleGenerator* System, FLOAT DeltaSeconds );
UParticleTemplate* Duplicate( UObject* NewOuter );

// Helpers.
FColor GetColor( FLOAT Pct );

