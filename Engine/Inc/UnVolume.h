/*=============================================================================
	UnVolume.h: Volumes.
	Copyright 1997-2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall
=============================================================================*/

struct FDecorationType
{
	class UStaticMesh*	StaticMesh;
	FRange				Count;
	FRange				DrawScale;
	UBOOL				bAlign;
	UBOOL				bRandomPitch;
	UBOOL				bRandomYaw;
	UBOOL				bRandomRoll;
};

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/

