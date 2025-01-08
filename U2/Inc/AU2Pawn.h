/*=============================================================================
	AU2Pawn.h:
=============================================================================*/

	// mantling
	void MantleBegin( const FCheckResult& Hit, UBOOL bStanding );
	void MantleEnd();
	UBOOL ActorFits( const FVector& Destination );
	void GetBaseHandsInfo( const FVector& BaseLocation, const FRotator& BaseRotation, FVector& BaseHandsLocation, FVector& BaseHandsVector );
	UBOOL FindMantleSpot( FCheckResult& Hit, const FVector& Start, const FVector& End );
	UBOOL TryToReachLedge( const FVector& HandsLocation );
	UBOOL TryToCatchLedge( const FVector& TraceStart, const FVector& TraceEnd );
	void CheckForLedge( UBOOL bStanding, const FVector& OldLocation=FVector(0,0,0), const FRotator& OldRotation=FRotator(0,0,0) );
	void MantleCheck( UBOOL bStanding, const FVector& OldLocation=FVector(0,0,0), const FRotator& OldRotation=FRotator(0,0,0) );
	UBOOL CheckHitHead();
	void setPhysics( BYTE NewPhysics, AActor *NewFloor = NULL, FVector NewFloorV = FVector(0,0,1) );
	void performPhysics( FLOAT DeltaSeconds );
	void physFalling( FLOAT deltaTime, INT Iterations );
	void physFlying( FLOAT deltaTime, INT Iterations );
