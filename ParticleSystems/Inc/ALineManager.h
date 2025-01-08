
void Render( FDynamicActor* Owner, FLevelSceneNode* SceneNode, TList<FDynamicLight*>* Lights, FRenderInterface* RI );

UParticle* AddLine
	(	FVector	Start
	,	FVector	End
	,	FVector	Color		= FVector(0,1,0)
	,	UBOOL	bDirected	= false
	,	FLOAT	ArrowOffset	= 64.f
	,	FLOAT	ArrowWidth	= 2.f
	,	FLOAT	ArrowHeight	= 8.f
	,	FVector	ArrowColor	= FVector(0,1,0)
	);

