/*=============================================================================
	APlayerReplicationInfo.h.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

	// Constructors.
	APlayerReplicationInfo() {}

	// AActor interface.
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
	void PostNetReceive();

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

