/*=============================================================================
	ALevelInfo.h.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

	// Constructors.
	ALevelInfo() {}

	// AActor interface.
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
	void CheckForErrors();
	void PreNetReceive();
	void PostNetReceive();

	// Level functions
	void SetZone( UBOOL bTest, UBOOL bForceRefresh );
	void SetVolumes();
	APhysicsVolume* GetDefaultPhysicsVolume();
	APhysicsVolume* GetPhysicsVolume(FVector Loc, AActor *A, UBOOL bUseTouch);

	void RegisterExternalForce( UObject* Force );
	void UnRegisterExternalForce( UObject* Force );
	void RegisterAffectedTemplate( UObject* ParticleTemplate );
	void UnRegisterAffectedTemplate( UObject* ParticleTemplate );

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

