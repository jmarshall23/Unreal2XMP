/*=============================================================================
	UnLight.cpp: Bsp light mesh illumination builder code
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EditorPrivate.h"
#include "UnRender.h"

extern ENGINE_API FRebuildTools GRebuildTools;

//
//	UEditorEngine::shadowIlluminateBsp
//	Raytracing entry point.
//

void UEditorEngine::shadowIlluminateBsp(ULevel* Level,UBOOL SelectedOnly,UBOOL ChangedOnly)
{
	guard(UEditorEngine::shadowIlluminateBsp);

	DOUBLE	StartTime = appSeconds();
	UBOOL	RebuildVisible = GRebuildTools.GetCurrent()->Options & REBUILD_OnlyVisible;

	// Compute light visibility.

	TestVisibility(Level,Level->Model,0,0);

	// Prepare actors for raytracing.

	for(INT ActorIndex = 0;ActorIndex < Level->Actors.Num();ActorIndex++)
	{
		AActor*	Actor = Level->Actors(ActorIndex);

		if(Actor)
			Actor->PreRaytrace();
	}

	// Illuminate the level.

	Level->Model->Illuminate(Level->GetLevelInfo(),ChangedOnly);

	// Illuminate actors.

	GWarn->BeginSlowTask(TEXT("Illuminating actors"),1);

	for(INT ActorIndex = 0;ActorIndex < Level->Actors.Num();ActorIndex++)
	{
		AActor*	Actor = Level->Actors(ActorIndex);

		if(Actor && (!Actor->IsHiddenEd() || RebuildVisible))
		{
			// Put movers in their local raytrace location.

			AMover*	Mover = Cast<AMover>(Actor);

			if(Mover)
				Mover->SetBrushRaytraceKey();

			// Calculate lighting for the actor.

			if(Actor->GetPrimitive())
				Actor->GetPrimitive()->Illuminate(Actor,ChangedOnly);

			Actor->PostRaytrace();
		}
		
		GWarn->StatusUpdatef(ActorIndex,Level->Actors.Num(),TEXT("Illuminating actors"));
	}

	// Mark lights that changed since the last rebuild as unchanged.

	for(INT ActorIndex = 0;ActorIndex < Level->Actors.Num();ActorIndex++)
	{
		AActor*	Actor = Level->Actors(ActorIndex);

		if(Actor)
			Actor->bLightChanged = 0;
	}

	// Compress the lightmaps.
	if( Level && Level->Model )
	{
		Level->Model->CompressLightmaps();
	}
	
	GWarn->EndSlowTask();

	debugf(TEXT("Illumination: %f seconds"),appSeconds() - StartTime);

	unguard;
}

/*---------------------------------------------------------------------------------------
   The End
---------------------------------------------------------------------------------------*/
