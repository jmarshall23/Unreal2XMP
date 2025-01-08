#include "ParticleSystems.h"
//!!MERGE #include "UnLegendMesh.h"

//------------------------------------------------------------------------------
void APulseLineGenerator::Spawned()
{
	guard(APulseLineGenerator::Spawned);
	NOTE(debugf(TEXT("APulseLineGenerator::Spawned")));

	Connections.AddZeroed();
	Super::Spawned();

	unguard;
}

//------------------------------------------------------------------------------
static FVector GetNodeLocation( AActor* MeshOwner, const TCHAR* NodeName )
{
	return FVector(0,0,0);
/*!!MERGE
	UMesh* Mesh = MeshOwner->Mesh;
	if(!Mesh)	return FVector(0,0,0);
	UMeshInstance* MInst = Mesh->MeshGetInstance(MeshOwner);
	if(!MInst)	return FVector(0,0,0);
	HMeshNode Node = MInst->GetNodeNamed(NodeName);
	if(!Node)	return FVector(0,0,0);
	return MInst->NodeGetTranslation(Node);
*/
}

//------------------------------------------------------------------------------
void APulseLineGenerator::InitTemplates()
{
	guard(APulseLineGenerator::InitTemplates);
	NOTE(debugf(TEXT("APulseLineGenerator::InitTemplates")));

	for( INT i=0; i<Connections.Num(); i++ )
	{
		if( Connections(i).StartActor )
			Connections(i).Start = Connections(i).StartActor->Location;

		else if( /*Connections(i).*/MeshOwner && /*Connections(i).*/StartBone!=TEXT("") )
			Connections(i).Start = GetNodeLocation( /*Connections(i).*/MeshOwner, * /*Connections(i).*/StartBone );

		if( Connections(i).EndActor )
			Connections(i).End = Connections(i).EndActor->Location;

		else if( /*Connections(i).*/MeshOwner && /*Connections(i).*/EndBone!=TEXT("") )
			Connections(i).End = GetNodeLocation( /*Connections(i).*/MeshOwner, * /*Connections(i).*/EndBone );

		NOTE(debugf(TEXT("%s: Connection[%d]: Start=%s End=%s Length=%f"),GetFullName(),i,*Connections(i).Start.String(),*Connections(i).End.String(),(Connections(i).End-Connections(i).Start).Size()));
	}

	if( ParticleTemplates.Num()!=Connections.Num() )
	{
		while( ParticleTemplates.Num() )
			ParticleTemplates.Last()->DeleteUObject();
		for( INT i=0; i<Connections.Num(); i++ )
			AddTemplate( CastChecked<UParticleTemplate>(StaticConstructObject( TemplateClass, this )) );
	}

	unguard;
}

//------------------------------------------------------------------------------
void APulseLineGenerator::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI )
{
	guard(APulseLineGenerator::Render);
	NOTE(debugf(TEXT("APulseLineGenerator::Render")));

	InitTemplates();
	Super::Render(Owner,Frame,Lights,RI);

	unguard;
}

//------------------------------------------------------------------------------
AParticleGenerator* APulseLineGenerator::Duplicate( ULevel* Level )
{
	guard(APulseLineGenerator::Duplicate);
	NOTE(debugf(TEXT("(%s)APulseLineGenerator::Duplicate"), GetFullName() ));

	AParticleGenerator* NewGenerator = Super::Duplicate( Level );

	while( ParticleTemplates.Num() )
		ParticleTemplates.Last()->DeleteUObject();

	return NewGenerator;

	unguard;
}

//------------------------------------------------------------------------------
void APulseLineGenerator::execUpdateRenderBoundingBox( FFrame& Stack, RESULT_DECL )
{
	guard(APulseLineGenerator::execUpdateRenderBoundingBox);
	NOTE(debugf(TEXT("APulseLineGenerator::execUpdateRenderBoundingBox")));

	P_FINISH;
	
	InitTemplates();
	for( INT i=0; i<Connections.Num(); i++ )
	{
		UParticleTemplate* Template = ParticleTemplates(i);
		Template->Extents = FBox(0);
		Template->Extents += Connections(i).Start;
		Template->Extents += Connections(i).End;
	}
	ClearRenderData();

	unguardexec;
}


IMPLEMENT_CLASS(APulseLineGenerator);
IMPLEMENT_CLASS(ABendiBeamGenerator);
