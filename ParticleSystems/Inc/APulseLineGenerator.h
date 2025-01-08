
void InitTemplates();
void Spawned();
void Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI );
AParticleGenerator* Duplicate( ULevel* Level );

inline FColor GetBeamColor()
{
	return BeamColor.Scale( VolumeScale, bFadeAlphaOnly );
}

inline FColor GetSpriteColor()
{
	return SpriteJointColor.Scale( VolumeScale, bJointFadeAlphaOnly );
}
