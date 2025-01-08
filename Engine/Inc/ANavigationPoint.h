/*=============================================================================
	ANavigationPoint.h: Class functions residing in the ANavigationPoint class.
	Copyright 2000 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

	virtual INT ProscribedPathTo(ANavigationPoint *Dest);
	virtual void addReachSpecs(APawn * Scout, UBOOL bOnlyChanged=false);
	virtual void PostaddReachSpecs(APawn * Scout) {};
	virtual void InitForPathFinding() {};
	virtual void SetupForcedPath(APawn* Scout, UReachSpec* Path) {};
	virtual void ClearPaths();
	virtual void FindBase();
	void PostEditMove();
	void Spawned();
	void Destroy();
	void CleanUpPruned();
	INT PrunePaths();
	UBOOL FindAlternatePath(UReachSpec* StraightPath, INT AccumulatedDistance);
	UReachSpec* GetReachSpecTo(ANavigationPoint *Nav);
	UBOOL ShouldBeBased();
	void CheckForErrors();
	virtual UBOOL IsIdentifiedAs(FName ActorName);
	virtual UBOOL ReviewPath(APawn* Scout);
	UBOOL CanReach(ANavigationPoint *Dest, FLOAT Dist);
	virtual void CheckSymmetry(ANavigationPoint* Other) {};
	virtual void ClearForPathFinding();
	virtual INT AddMyMarker(AActor *S);
    virtual class AInventorySpot* GetAInventorySpot() { return NULL; } 


