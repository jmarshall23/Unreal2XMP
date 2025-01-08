/*=============================================================================
	ALadder.h: Class functions residing in the ALadder class.
	Copyright 2000 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

	INT ProscribedPathTo(ANavigationPoint *Dest);
	void addReachSpecs(APawn * Scout, UBOOL bOnlyChanged=false);
	void InitForPathFinding();
	void ClearPaths();

