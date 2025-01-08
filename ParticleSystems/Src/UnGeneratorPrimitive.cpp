#include "ParticleSystems.h"
#include "UnGeneratorPrimitive.h"

UGeneratorPrimitive* GGeneratorPrimitive = NULL;

UPrimitive* AParticleGenerator::GetPrimitive()
{
	guard(AParticleGenerator::GetPrimitive);

	if( bCollisionBound )
		return Super::GetPrimitive();
	
	if( !GGeneratorPrimitive )
		GGeneratorPrimitive = ConstructObject<UGeneratorPrimitive>(UGeneratorPrimitive::StaticClass());
	return GGeneratorPrimitive;
	unguard;
}

UBOOL UGeneratorPrimitive::PointCheck
(
	FCheckResult	&Result,
	AActor			*Owner,
	FVector			Location,
	FVector			Extent,
	DWORD           ExtraNodeFlags
){
	return 0;
}

UBOOL UGeneratorPrimitive::LineCheck
(
	FCheckResult	&Result,
	AActor			*Owner,
	FVector			End,
	FVector			Start,
	FVector			Extent,
	DWORD           ExtraNodeFlags
){
	return 0;
}

FBox UGeneratorPrimitive::GetRenderBoundingBox( const AActor* Owner )
{
	guard(UGeneratorPrimitive::GetRenderBoundingBox);
	NOTE(debugf(TEXT("UGeneratorPrimitive::GetRenderBoundingBox")));

	AParticleGenerator* Gen = (AParticleGenerator*)Owner;
	FBox VisBox(0);

	for( INT i = 0; i < Gen->ParticleTemplates.Num(); i++ )
	{
		VisBox += Gen->ParticleTemplates(i)->Extents;
	}

	// Fix ARL: Doesn't correctly take particle size into account.  (Like what if you only have one particle?)
	if( VisBox.IsValid )
	{
		if( Gen->bLockParticles )
			VisBox = VisBox.TransformBy( Gen->LockedLocalToWorld * (FRotationMatrix(Gen->Rotation) * FTranslationMatrix(Gen->Location)) );
		VisBox = VisBox.ExpandBy(64.f).TransformBy( Gen->WorldToLocal() );
	}
	else VisBox = Super::GetRenderBoundingBox(Gen);

	return VisBox;

	unguard;
}

FSphere UGeneratorPrimitive::GetRenderBoundingSphere( const AActor* Owner )
{
	FVector Center,Extents;
	GetRenderBoundingBox( Owner ).GetCenterAndExtents( Center, Extents );
	return FSphere( Center, Extents.Size() );
}

FBox UGeneratorPrimitive::GetCollisionBoundingBox( const AActor* Owner ) const
{
	return FBox();
}

void UGeneratorPrimitive::Destroy()
{
	guard(UGeneratorPrimitive::Destroy);
	GGeneratorPrimitive = NULL;
	Super::Destroy();
	unguard;
}

IMPLEMENT_CLASS(UGeneratorPrimitive);
