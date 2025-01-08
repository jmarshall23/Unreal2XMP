#include "U2.h"

void AIconNode::PostBeginPlay()
{
	guard(AIconNode::PostBeginPlay)
	Super::PostBeginPlay();
	if( GIsEditor )
		SetupExtraMeshes();
	unguard;
}

void AIconNode::SetupExtraMeshes()
{
	guard(AIconNode::SetupExtraMeshes);

	for( int i=0; i < ExtraStaticMeshes.Num(); i++ )
	{
		AStaticMeshActor* NewActor = (AStaticMeshActor*)GetLevel()->SpawnActor( AStaticMeshActor::StaticClass(), NAME_None );
		if( !NewActor )
		{
			debugf(TEXT("(%s)Icon Node -- unable to spawn extra mesh"), GetFullName() );
			return;
		}
		NewActor->StaticMesh = ExtraStaticMeshes(i);
		FVector NewLocation( Location );
		if( i < ExtraMeshLocations.Num() )
			NewLocation += ExtraMeshLocations(i);
		GetLevel()->FarMoveActor( NewActor, NewLocation );
	}

	unguard;
}

IMPLEMENT_CLASS(AIconNode);

