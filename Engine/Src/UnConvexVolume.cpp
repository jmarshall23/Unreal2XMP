/*=============================================================================
	UnConvexVolume.cpp
	Copyright 1997-2001 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UConvexVolume);

//
//	UConvexVolume::UConvexVolume
//

UConvexVolume::UConvexVolume()
{
}

//
//	UConvexVolume::Serialize
//

void UConvexVolume::Serialize(FArchive& Ar)
{
	guard(UConvexVolume::Serialize);

	Super::Serialize(Ar);

	Ar	<< Faces
		<< Edges
		<< BoundingBox;

	unguard;
}
