/*=============================================================================
	UnMover.cpp: Keyframe mover actor code
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	AMover implementation.
-----------------------------------------------------------------------------*/

AMover::AMover()
{}
void AMover::Spawned()
{
	guard(AMover::Spawned);
	Super::Spawned();

	BasePos = Location;
	BaseRot	= Rotation;

	unguard;
}
void AMover::PostLoad()
{
	guard(AMover::PostLoad);
	AActor::PostLoad();

	// For refresh.
	SavedPos = FVector(-12345,-12345,-12345);
	SavedRot = FRotator(123,456,789);

	// Fix brush poly iLinks which were broken.
	if( Brush && Brush->Polys )
		for( INT i=0; i<Brush->Polys->Element.Num(); i++ )
			Brush->Polys->Element(i).iLink = i;

	unguard;
}
void AMover::PostEditMove()
{
	guard(AMover::PostEditMove);
	Super::PostEditMove();
	if( KeyNum == 0 )
	{
		// Changing location.
		BasePos  = Location - OldPos;
		BaseRot  = Rotation - OldRot;
	}
	else
	{
		// Changing displacement of KeyPos[KeyNum] relative to KeyPos[0].
		KeyPos[KeyNum] = Location - (BasePos + KeyPos[0]);
		KeyRot[KeyNum] = Rotation - (BaseRot + KeyRot[0]);

		// Update Old:
		OldPos = KeyPos[KeyNum];
		OldRot = KeyRot[KeyNum];
	}
	Location = BasePos + KeyPos[KeyNum];
	unguard;
}
void AMover::PostEditChange()
{
	guard(AMover::PostEditChange);
	Super::PostEditChange();

	// Validate KeyNum.
	KeyNum = Clamp( (INT)KeyNum, (INT)0, (INT)ARRAY_COUNT(KeyPos)-1 );

	// Update BasePos.
	BasePos  = Location - OldPos;
	BaseRot  = Rotation - OldRot;

	// Update Old.
	OldPos = KeyPos[KeyNum];
	OldRot = KeyRot[KeyNum];

	// Update Location.
	Location = BasePos + OldPos;
	Rotation = BaseRot + OldRot;

	PostEditMove();

	unguard;
}
void AMover::PreRaytrace()
{
	guard(AMover::PreRaytrace);
	Super::PreRaytrace();

	// Place this brush in position to raytrace the world.
	SavedPos = FVector(0,0,0);
	SavedRot = FRotator(0,0,0);

	SetWorldRaytraceKey();

	unguard;
}
void AMover::SetWorldRaytraceKey()
{
	guard(AMover::SetWorldRaytraceKey);
	if( WorldRaytraceKey!=255 )
	{
		WorldRaytraceKey = Clamp((INT)WorldRaytraceKey,0,(INT)ARRAY_COUNT(KeyPos)-1);
		if( bCollideActors && GetLevel()->Hash ) GetLevel()->Hash->RemoveActor( this );
		Location = BasePos + KeyPos[WorldRaytraceKey];
		Rotation = BaseRot + KeyRot[WorldRaytraceKey];
		if( bCollideActors && GetLevel()->Hash ) GetLevel()->Hash->AddActor( this );
	}
	unguard;
}
void AMover::SetBrushRaytraceKey()
{
	guard(AMover::SetBrushRaytraceKey);

	BrushRaytraceKey = Clamp((INT)BrushRaytraceKey,0,(INT)ARRAY_COUNT(KeyPos)-1);
	if( bCollideActors && GetLevel()->Hash ) GetLevel()->Hash->RemoveActor( this );
	Location = BasePos + KeyPos[BrushRaytraceKey];
	Rotation = BaseRot + KeyRot[BrushRaytraceKey];
	if( bCollideActors && GetLevel()->Hash ) GetLevel()->Hash->AddActor( this );

	unguard;
}
void AMover::PostRaytrace()
{
	guard(AMover::PostRaytrace);
	Super::PostRaytrace();

	// Called before/after raytracing session beings.
	if( bCollideActors && GetLevel()->Hash ) GetLevel()->Hash->RemoveActor( this );
	Location = BasePos + KeyPos[KeyNum];
	Rotation = BaseRot + KeyRot[KeyNum];
	if( bCollideActors && GetLevel()->Hash ) GetLevel()->Hash->AddActor( this );
	SavedPos = FVector(0,0,0);
	SavedRot = FRotator(0,0,0);

	unguard;
}
IMPLEMENT_CLASS(AMover);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

