#include "U2.h"

void ADeploymentPoint::PostBeginPlay()
{
	guard(ADeploymentPoint::PostBeginPlay)
	Super::PostBeginPlay();
	if( GIsEditor )
	{
		SetupPlayerStarts();
		SetupExtraMeshes();
	}
	unguard;
}

void ADeploymentPoint::PostEditChange()
{
	guard(ADeploymentPoint::PostEditChange);
	SetupPlayerStarts();
	unguard;
}

void ADeploymentPoint::SetupExtraMeshes()
{
	guard(ADeploymentPoint::SetupExtraMeshes);
    TArray<class UMaterial*> NewTeamSkins;

	// base
	Base = (ATeamMeshActor*)GetLevel()->SpawnActor( ATeamMeshActor::StaticClass(), NAME_None, Location );
	if( !Base )
	{
		debugf(TEXT("(%s)DeploymentPoint -- unable to spawn base mesh"), GetFullName() );
		return;
	}
	Base->StaticMesh = BaseStaticMesh;
	Base->RepSkinIndex = BaseRepSkinIndex;
	for( int iSkin=0; iSkin < BaseTeamSkins.Num(); iSkin++ )
		Base->AlternateSkins.AddItem( BaseTeamSkins(iSkin) );


	// wings
	Wings = (ADeploySpinner*)GetLevel()->SpawnActor( ADeploySpinner::StaticClass(), NAME_None, Location );
	if( !Wings )
	{
		debugf(TEXT("(%s)DeploymentPoint -- unable to spawn wings mesh"), GetFullName() );
		return;
	}
	unguard;
}


void ADeploymentPoint::SetupPlayerStarts()
{
	guard(ADeploymentPoint::SetupPlayerStarts);
	AActor::PostEditChange();

	// check for bad values
	if( SpawnerCount < 0 )
		SpawnerCount = 0;

	// no level, can't do much
	if( !GetLevel() )
		return;

	// too many player starts
	if( Spawners.Num() > SpawnerCount )
	{
		guard(RemoveExtraPlayerStarts);
		for( int j=SpawnerCount; j < Spawners.Num(); j++ )
		{
			APlayerStart* PS = Spawners(j);
			if( PS )
			{
				if( PS->bCollideActors )
					GetLevel()->Hash->RemoveActor(PS);
				GetLevel()->EditorDestroyActor(PS);
			}
		}
		Spawners.Remove( SpawnerCount, Spawners.Num() - SpawnerCount );
		if( SpawnerCount < LastSpawnTimes.Num() )
			LastSpawnTimes.Remove( SpawnerCount, LastSpawnTimes.Num() - SpawnerCount );
		if( SpawnerCount < FailedSpawnAttempts.Num() )
			FailedSpawnAttempts.Remove( SpawnerCount, FailedSpawnAttempts.Num() - SpawnerCount );
		unguard;
	}
	else if( Spawners.Num() != SpawnerCount )
	{
		guard(AddNeededPlayerStarts);
		// too few player starts
		APlayerStart* StartingPoint;
		for( int i=Spawners.Num(); i < SpawnerCount; i++ )
		{
			guard(Spawn);
			StartingPoint = (APlayerStart*)GetLevel()->SpawnActor( APlayerStart::StaticClass(), NAME_None, Location, Rotation );
			if( !StartingPoint )
			{
				debugf(TEXT("(%s)DeploymentPoint -- unable to spawn playerstart at %s"), GetFullName(), *Location.String());
				return;
			}
			unguard;
			Spawners.AddItem( StartingPoint );
			LastSpawnTimes.AddItem( 0 );
			FailedSpawnAttempts.AddItem( 0 );
		}
		unguard;
	}

	if( !Spawners.Num() )
		return;

	if( Spawners.Num() )
	{
		guard(PositionPlayerStarts);
		// put the player starts in a ring around the center of the deployment point
		FRotator CurrRotation( Rotation );
		FCoords Coords;
		FVector X, Y, Z;
		float deltaYaw = 65535.0 / Spawners.Num();

		for( int i=Spawners.Num()-1; i >= 0 ; i-- )
		{
			if( Spawners(i) )
			{
				Coords = GMath.UnitCoords / CurrRotation;
				X = Coords.XAxis;
				Y = Coords.YAxis;
				Z = Coords.ZAxis;

				if( !GetLevel()->FarMoveActor( Spawners(i), Location + SpawnerLocationOffset.X * X
																	 + SpawnerLocationOffset.Y * Y
																	 + SpawnerLocationOffset.Z * Z ) )
					debugf(TEXT("unable to move spawner: %d"), i );

				// face player starts away from the deploy point
				Spawners(i)->SetRotation( (Spawners(i)->Location - Location).Rotation() );

				CurrRotation.Yaw = CurrRotation.Yaw + deltaYaw;
				Spawners(i)->TeamNumber = Team;
			}
			else
				Spawners.Remove(i);
		}
		unguard;
	}

	SpawnerCount = Spawners.Num();

	unguard;
}

IMPLEMENT_CLASS(ADeploymentPoint);

