/*=============================================================================
	UnPawn.cpp: APawn AI implementation

  This contains both C++ methods (movement and reachability), as well as some 
  AI related natives

	Copyright 1997-2002 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Steven Polge 3/97
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"
#include "FConfigCacheIni.h"
#include "UnPath.h"
#include "xForceFeedback.h"

/*-----------------------------------------------------------------------------
	APawn object implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(APawn);

APawn* APawn::GetPlayerPawn() const
{
	guardSlow(APawn::GetPlayerPawn);

	if ( !Controller || !Controller->IsAPlayerController() )
		return NULL;

	// Players must have player objects.
	if ( ((APlayerController *)Controller)->Player == NULL )
		return NULL;

	return ((APawn *)this);
	unguardSlow;
}

UBOOL APawn::ValidPawn()
{
	guard(ValidPawn);
	return( !bDeleteMe && Health > 0 && !bHidden );
	unguard;
}

UBOOL APawn::IsPlayer() 
{ 
	return ( Controller && Controller->bIsPlayer ); 
}

UBOOL APawn::IsHumanControlled() 
{ 
	return ( Controller && Controller->IsAPlayerController() ); 
}

UBOOL APawn::IsLocallyControlled() 
{
	return ( Controller && Controller->LocalPlayerController() );
}

inline UBOOL APawn::ShouldTrace(AActor *SourceActor, DWORD TraceFlags)
{
	// Skip actors without bShadowCast when raytracing for shadows.
	if( !this->bShadowCast && (TraceFlags & TRACE_ShadowCast) )
		return false;

	if( this->bOnlyAffectPawns && SourceActor && !SourceActor->IsAPawn() )
		return false;

	if(TraceFlags & TRACE_AcceptProjectors)
		return bAcceptsProjectors;

	if((TraceFlags & 0x2000 /* TRACE_Corona */) && 
		StaticMesh && DrawType == DT_StaticMesh)
		return true;
	
	// inlined for speed on PS2
	return (TraceFlags & 0x001 /*TRACE_Pawns*/);
}

UBOOL APawn::DelayScriptReplication(FLOAT LastFullUpdateTime) 
{ 
	guard(APawn::DelayScriptReplication);

	return ( LastFullUpdateTime != Level->TimeSeconds );
	unguard;
}

void APawn::NotifyAnimEnd( int Channel )
{
	if ( Mesh && Channel == 0 )
    {
        bWaitForAnim = false;
		AnimAction = NAME_None;
        if ( bPhysicsAnimUpdate && Level->NetMode != NM_DedicatedServer && bIsIdle && !bPlayedDeath)
            PlayIdle();
    }
    
	if ( Controller && Controller->IsProbing(NAME_AnimEnd) )
		Controller->eventAnimEnd(Channel);
	else
		eventAnimEnd(Channel);
}

void APawn::SetAnchor(ANavigationPoint *NewAnchor)
{
	guardSlow(SetAnchor);

	Anchor = NewAnchor;
	if ( Anchor )
	{
		LastValidAnchorTime = Level->TimeSeconds;
		LastAnchor = Anchor;
	}
	unguardSlow;
}

#if 1 //NEW (mdf)
void APawn::execAddPawn( FFrame& Stack, RESULT_DECL )
{
	guardSlow(APawn::execAddPawn);

	P_FINISH;

	nextPawn = Level->PawnList;
	Level->PawnList = this;
	unguardSlow;
}

void APawn::execRemovePawn( FFrame& Stack, RESULT_DECL )
{
	guardSlow(APawn::execRemovePawn);

	P_FINISH;

	APawn* next = Level->PawnList;
	if( next == this )
		Level->PawnList = next->nextPawn;
	else
	{
		while ( next )
		{
			if ( next->nextPawn == this )
			{
				next->nextPawn = nextPawn;
				break;
			}
			next = next->nextPawn;
		}
	}
	unguardSlow;
}
#endif

/* PlayerCanSeeMe()
	returns true if actor is visible to some player
*/
void AActor::execPlayerCanSeeMe( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::PlayerCanSeeMe);
	P_FINISH;

	int seen = 0;
	if ( (Level->NetMode == NM_Standalone) || (Level->NetMode == NM_Client) )
	{
		// just check local player visibility
		seen = (GetLevel()->TimeSeconds - LastRenderTime < 1);
	}
	else
	{
		for ( AController *next=Level->ControllerList; next!=NULL; next=next->nextController )
			if ( next->IsAPlayerController() && TestCanSeeMe( Cast<APlayerController>(next) ) )
			{
				seen = 1;
				break;
			}
	}
	*(DWORD*)Result = seen;
	unguard;
}

int AActor::TestCanSeeMe( APlayerController *Viewer )
{
	guard(AActor::TestCanSeeMe);

	if ( !Viewer )
		return 0;
	if ( Viewer->GetViewTarget() == this )
		return 1;

	float distSq = (Location - Viewer->ViewTarget->Location).SizeSquared();
	return ( (distSq < 100000.f * (CollisionRadius + 3.6)) 
		&& (Viewer->bBehindView 
			|| (Square(Viewer->Rotation.Vector() | (Location - Viewer->ViewTarget->Location)) >= 0.25f * distSq))
		&& Viewer->LineOfSightTo(this) );

	unguard;
}

/*-----------------------------------------------------------------------------
	Pawn related functions.
-----------------------------------------------------------------------------*/

void APawn::execValidPawn( FFrame& Stack, RESULT_DECL )
{
	guardSlow(APawn::execValidPawn);
	P_GET_OBJECT(APawn,P);
	P_FINISH;

	*(UBOOL*)Result = P && P->ValidPawn();

	unguardSlow;
}

void APawn::execForceCrouch( FFrame& Stack, RESULT_DECL )
{
	guardSlow(APawn::execForceCrouch);

	P_FINISH;

	Crouch(false);
	unguardSlow;
}
void APawn::execReachedDestination( FFrame& Stack, RESULT_DECL )
{
	guardSlow(APawn::execReachedDestination);

	P_GET_ACTOR(GoalActor);
	P_FINISH;

	if ( GoalActor )
		*(DWORD*)Result = ReachedDestination(GoalActor->Location - Location, GoalActor);
	else
		*(DWORD*)Result = 0;
	unguardSlow;
}

/*MakeNoise
- check to see if other creatures can hear this noise
*/
void AActor::execMakeNoise( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execMakeNoise);

	P_GET_FLOAT(Loudness);
	P_FINISH;
	
	//debugf(" %s Make Noise with instigator", GetFullName(),Instigator->GetClass()->GetName());
	if ( (Level->NetMode != NM_Client) && Instigator )
	{
		if ( (Owner == Instigator) || (Instigator == this) )
			Loudness *= Instigator->SoundDampening;
		clock(GStats.DWORDStats(GEngineStats.STATS_Game_SeePlayerCycles));
		if ( Instigator->Visibility < 2 )
			Loudness *= 0.3f;
		CheckNoiseHearing(Loudness);
		unclock(GStats.DWORDStats(GEngineStats.STATS_Game_SeePlayerCycles));
	}
	unguardSlow;
}

//=================================================================================
void APawn::setMoveTimer(FLOAT MoveSize)
{
	guard(APawn::setMoveTimer);

	if ( !Controller )
		return;

	if ( DesiredSpeed == 0.f )
		Controller->MoveTimer = 0.5f;
	else
	{
		FLOAT Extra = 2.f;
		if ( bIsWalking || bIsCrouched )
			Extra = ::Max(Extra, 1.f/WalkingPct);
		Controller->MoveTimer = 0.5f + Extra * MoveSize/(DesiredSpeed * 0.6f * GetMaxSpeed()); 
	}
	if ( Controller->bPreparingMove && Controller->PendingMover )
		Controller->MoveTimer += 2.0;
	unguard;
}

FLOAT APawn::GetMaxSpeed()
{
	guard(APawn::GetMaxSpeed);

	FLOAT MaxSpeed = GroundSpeed; 

	if (Physics == PHYS_Flying)
		MaxSpeed = AirSpeed;
	else if (Physics == PHYS_Swimming)
		MaxSpeed = WaterSpeed;
	return MaxSpeed;
	unguard;
}

/* StartNewSerpentine()
pawn is using serpentine motion while moving to avoid being hit (while staying within the reachspec 
its using.  At this point change direction (either reverse, or go straight for a while
*/
void APawn::StartNewSerpentine(FVector Dir, FVector Start)
{
	guard(APawn::StartNewSerpentine);

	FVector NewDir(Dir.Y, -1.f * Dir.X, Dir.Z);
	if ( (NewDir | (Location - Start)) > 0.f )
		NewDir *= -1.f;
	SerpentineDir = NewDir;

	if ( !Controller->bAdvancedTactics )
	{
		SerpentineTime = 9999.f;
		SerpentineDist = appFrand();
		if ( appFrand() < 0.4f )
			SerpentineDir *= -1.f;
		SerpentineDist *= ::Max(0.f,Controller->CurrentPath->CollisionRadius - CollisionRadius);
		return;
	}
	
	if ( appFrand() < 0.2f )
	{
		SerpentineTime = 0.1f + 0.4f * appFrand();
		return;
	}
	SerpentineTime = 0.f;


	FLOAT ForcedStrafe = ::Min(1.f, 4.f * CollisionRadius/Controller->CurrentPath->CollisionRadius);
	SerpentineDist = (ForcedStrafe + (1.f - ForcedStrafe) * appFrand());
	SerpentineDist *= (Controller->CurrentPath->CollisionRadius - CollisionRadius);
	unguard;
}

/* ClearSerpentine()
completely clear all serpentine related attributes
*/
void APawn::ClearSerpentine()
{
	guard(APawn::ClearSerpentine);

	SerpentineTime = 999.f;
	SerpentineDist = 0.f;
	unguard;
}

/* moveToward()
move Actor toward a point.  Returns 1 if Actor reached point
(Set Acceleration, let physics do actual move)
*/
UBOOL APawn::moveToward(const FVector &Dest, AActor *GoalActor )
{
	guard(APawn::moveToward);

	if ( !Controller )
		return false;

	if ( Controller->bAdjusting )
		GoalActor = NULL;
	FVector Direction = Dest - Location;
	FLOAT ZDiff = Direction.Z;

	if (Physics == PHYS_Walking) 
		Direction.Z = 0.f;
	else if (Physics == PHYS_Falling)
	{
		// use air control if low grav
		if ( (Velocity.Z < 0.f) && (PhysicsVolume->Gravity.Z > 0.9f * ((APhysicsVolume *)PhysicsVolume->GetClass()->GetDefaultObject())->Gravity.Z) )
		{
			if ( ZDiff > 0.f )
		{
				if ( ZDiff > 2.f * MAXJUMPHEIGHT )
				{
					Controller->MoveTimer = -1.f;
					Controller->eventNotifyMissedJump();
				}
			}
			else
			{
				if ( (Velocity.X == 0.f) && (Velocity.Y == 0.f) )
					Acceleration = FVector(0.f,0.f,0.f);
				else
				{
					FLOAT Dist2D = Direction.Size2D();
			Direction.Z = 0.f;
					Acceleration = Direction;
					Acceleration = Acceleration.SafeNormal();
			Acceleration *= AccelRate;
					if ( (Dist2D < 0.5f * Abs(Direction.Z)) && ((Velocity | Direction) > 0.5f*Dist2D*Dist2D) )
						Acceleration *= -1.f;

					if ( Dist2D < (GoalActor ? ::Min(GoalActor->CollisionRadius, 1.5f*CollisionRadius) : 1.5f*CollisionRadius) )
					{
						Velocity.X = 0.f;
						Velocity.Y = 0.f;
					}
				}
			}
		}
		return false; // don't end move until have landed
	}
	else if ( (Physics == PHYS_Ladder) && OnLadder )
	{
		if ( ReachedDestination(Dest - Location, GoalActor) )
		{
			Acceleration = FVector(0.f,0.f,0.f);

			// if Pawn just reached a navigation point, set a new anchor
			ANavigationPoint *Nav = Cast<ANavigationPoint>(GoalActor);
			if ( Nav )
				SetAnchor(Nav);
			return true;
		}
		Acceleration = Direction.SafeNormal();
		if ( GoalActor && (OnLadder != GoalActor->PhysicsVolume)
			&& ((Acceleration | (OnLadder->ClimbDir + OnLadder->LookDir)) > 0.f)
			&& (GoalActor->Location.Z < Location.Z) )
			setPhysics(PHYS_Falling);
		Acceleration *= LadderSpeed;
		return false;
	}
	if ( Controller->MoveTarget && Controller->MoveTarget->IsA(APickup::StaticClass()) 
		 && (Abs(Location.Z - Controller->MoveTarget->Location.Z) < CollisionHeight)
		 && (Square(Location.X - Controller->MoveTarget->Location.X) + Square(Location.Y - Controller->MoveTarget->Location.Y) < Square(CollisionRadius)) )
		 Controller->MoveTarget->eventTouch(this);
	
	FLOAT Distance = Direction.Size();
	INT bGlider = ( !bCanStrafe && ((Physics == PHYS_Flying) || (Physics == PHYS_Swimming)) );
	FCheckResult Hit(1.f);

	if ( ReachedDestination(Dest - Location, GoalActor) )
	{
		if ( !bGlider )
			Acceleration = FVector(0.f,0.f,0.f);

		// if Pawn just reached a navigation point, set a new anchor
		ANavigationPoint *Nav = Cast<ANavigationPoint>(GoalActor);
		if ( Nav )
			SetAnchor(Nav);
		return true;
	}
	else if ( (Physics == PHYS_Walking) && (Distance < CollisionRadius)
			&& (!GoalActor || ((ZDiff > CollisionHeight + 2.f * UCONST_MAXSTEPHEIGHT)
				&& !GetLevel()->SingleLineCheck(Hit, this, Dest, Location, TRACE_World))) )
	{
		// failed - below target
		return true;
	}
	else if ( bGlider )
		Direction = Rotation.Vector();
	else if ( Distance > 0.f )
	{
		Direction = Direction/Distance;
		if ( Controller->CurrentPath )
		{
			if ( SerpentineTime > 0.f )
			{
				SerpentineTime -= AvgPhysicsTime;
				if ( SerpentineTime <= 0.f )
					StartNewSerpentine(Controller->CurrentPathDir,Controller->CurrentPath->Start->Location);
				else if ( SerpentineDist > 0.f )
				{
					if ( Distance < 2.f * SerpentineDist )
						ClearSerpentine();
					else
					{
						FVector Start = Controller->CurrentPath->Start->Location;
						FVector LineDir = Location - (Start + (Controller->CurrentPathDir | (Location - Start)) * Controller->CurrentPathDir);
						if ( (LineDir.SizeSquared() >= SerpentineDist * SerpentineDist) && ((LineDir | SerpentineDir) > 0.f) )
							Direction = (Dest - Location + SerpentineDir*SerpentineDist).SafeNormal();
						else
							Direction = (Direction + 0.2f * SerpentineDir).SafeNormal();
					}
				}
			}
			if ( SerpentineTime <= 0.f )
			{
				if ( Distance < 2.f * SerpentineDist )
					ClearSerpentine();
				else
				{
					FVector Start = Controller->CurrentPath->Start->Location;
					FVector LineDir = Location - (Start + (Controller->CurrentPathDir | (Location - Start)) * Controller->CurrentPathDir);
					if ( (LineDir.SizeSquared() >= SerpentineDist * SerpentineDist) && ((LineDir | SerpentineDir) > 0.f) )
						StartNewSerpentine(Controller->CurrentPathDir,Start);
					else
						Direction = (Direction + SerpentineDir).SafeNormal();
				}
			}
		}
	}

	Acceleration = Direction * AccelRate;

	if ( !Controller->bAdjusting && Controller->MoveTarget && Controller->MoveTarget->IsA(APawn::StaticClass()) )
	{
		if (Distance < CollisionRadius + Controller->MoveTarget->CollisionRadius + 0.8f * MeleeRange)
			return true;
		return false;
	}

	FLOAT speed = Velocity.Size(); 

	if ( !bGlider && (speed > 100.f) )
	{
		FVector VelDir = Velocity/speed;
		Acceleration -= 0.2f * (1 - (Direction | VelDir)) * speed * (VelDir - Direction); 
	}
	if ( Distance < 1.4f * AvgPhysicsTime * speed )
	{
		if ( !bReducedSpeed ) //haven't reduced speed yet
		{
			DesiredSpeed = 0.51f * DesiredSpeed;
			bReducedSpeed = 1;
		}
		if ( speed > 0 )
			DesiredSpeed = Min(DesiredSpeed, 200.f/speed);
		if ( bGlider ) 
			return true;
	}
	return false;
	unguard;
}

/* rotateToward()
rotate Actor toward a point.  Returns 1 if target rotation achieved.
(Set DesiredRotation, let physics do actual move)
*/
void APawn::rotateToward(AActor *Focus, FVector FocalPoint)
{
	guard(APawn::rotateToward);
	if ( bRollToDesired || (Physics == PHYS_Spider) )
		return;
	if ( !bCanStrafe 
		&& ((Physics == PHYS_Flying) 
			|| (Physics == PHYS_Swimming)) )
		Acceleration = Rotation.Vector() * AccelRate;

	if ( Focus )
	{
		ANavigationPoint *NavFocus = Cast<ANavigationPoint>(Focus);
		if ( NavFocus && Controller && Controller->CurrentPath && (Controller->MoveTarget == NavFocus) && !Velocity.IsZero() )
			FocalPoint = Focus->Location - Controller->CurrentPath->Start->Location + Location;
		else
			FocalPoint = Focus->Location;
	}
	FVector Direction = FocalPoint - Location;

	// Rotate toward destination
	DesiredRotation = Direction.Rotation();
	DesiredRotation.Yaw = DesiredRotation.Yaw & 65535;
	if ( (Physics == PHYS_Walking) && (!Controller || !Controller->MoveTarget || !Controller->MoveTarget->IsA(APawn::StaticClass())) )
		DesiredRotation.Pitch = 0;

	unguard;
}

/* HurtByVolume() - virtual	`
Returns true if pawn can be hurt by the zone Z.  By default, pawns are not hurt by pain zones
whose damagetype == their ReducedDamageType
*/
UBOOL APawn::HurtByVolume(AActor *A)
{
	guard(APawn::HurtByVolume);

	for ( INT i=0; i<A->Touching.Num(); i++ )
	{
		APhysicsVolume *V = Cast<APhysicsVolume>(A->Touching(i));
		if ( V && V->bPainCausing && (V->DamageType != ReducedDamageType) 
			&& (V->DamagePerSec > 0) )
			return true;
	}
	return false;
	unguard;
}

/* Scouts are never hurt by zones (allows paths to be created in pain zones)
*/
UBOOL AScout::HurtByVolume(AActor *A)
{
	guard(APawn::HurtByVolume);

	return false;
	unguard;
}

int APawn::actorReachable(AActor *Other, UBOOL bKnowVisible, UBOOL bNoAnchorCheck)
{
	guard(APawn::actorReachable);

	if ( !Other || Other->bDeleteMe )
		return 0;

	if ( (Other->Physics == PHYS_Flying) && !bCanFly )
		return 0;

	// If goal is on the navigation network, check if it will give me reachability
	AActor *RealActor = NULL;
	APickup * PickupDest = Cast<APickup>(Other);
	if ( PickupDest && PickupDest->myMarker )
	{
		RealActor = Other;
		Other = PickupDest->myMarker;
	}
	ANavigationPoint *Nav = Cast<ANavigationPoint>(Other);
	if ( Nav )
	{
		FVector Dir = Other->Location - Location;
		if ( !bNoAnchorCheck )
	{
			if ( ReachedDestination(Dir,Nav) )
		{
				Anchor = Nav;
				return 1;
		}
			if ( ValidAnchor() )
			{
				UReachSpec* Path = Anchor->GetReachSpecTo(Nav);
				return ( Path && Path->supports(CollisionRadius,CollisionHeight,calcMoveFlags(),MaxFallSpeed) );			
			}
		}
		if ( (Dir.SizeSquared() > 0.5f * MAXPATHDISTSQ) && !GIsEditor )
			return 0;
	}	
	if ( RealActor )
		Other = RealActor;

	FVector Dir = Other->Location - Location;
	FLOAT distsq = Dir.SizeSquared();

	if ( !GIsEditor ) 
	{
		// Use the navigation network if more than MAXPATHDIST units to goal
		if( distsq > MAXPATHDISTSQ ) 
			return 0;
		if ( HurtByVolume(Other) ) 
			return 0;
		if ( Other->PhysicsVolume && Other->PhysicsVolume->bWaterVolume )
		{
			if ( !bCanSwim )
				return 0;
		}
		else if ( !bCanFly && !bCanWalk )
			return 0;
	}

	//check other visible
	if ( !bKnowVisible )
	{
		FCheckResult Hit(1.f);
		FVector	ViewPoint = Location;
		ViewPoint.Z += BaseEyeHeight; //look from eyes
		GetLevel()->SingleLineCheck(Hit, this, Other->Location, ViewPoint, TRACE_World);
		if( Hit.Time!=1.f && Hit.Actor!=Other )
			return 0;
	}

	if (Other->IsA(APawn::StaticClass()))
	{
		FLOAT Threshold = CollisionRadius + ::Min(1.5f * CollisionRadius, MeleeRange) + Other->CollisionRadius;
		FLOAT Thresholdsq = Threshold * Threshold;
		if (distsq <= Thresholdsq)
			return 1;
	}
	FVector realLoc = Location;
	FVector aPoint = Other->Location; //adjust destination
	if ( Other->Physics == PHYS_Falling )
	{
		// check if ground below it
		FCheckResult Hit(1.f);
		GetLevel()->SingleLineCheck(Hit, this, Other->Location - FVector(0.f,0.f,400.f), Other->Location, TRACE_World);
		if ( Hit.Time == 1.f )
			return false;
		aPoint = Hit.Location + FVector(0.f,0.f,CollisionRadius + UCONST_MAXSTEPHEIGHT);
		if ( GetLevel()->FarMoveActor(this, aPoint, 1) )
		{
			aPoint = Location;
			GetLevel()->FarMoveActor(this, realLoc,1,1);
			FVector	ViewPoint = Location;
			ViewPoint.Z += BaseEyeHeight; //look from eyes
			GetLevel()->SingleLineCheck(Hit, this, aPoint, ViewPoint, TRACE_World);
			if( Hit.Time!=1.f && Hit.Actor!=Other )
				return 0;
		}
		else
			return 0;
	}
	else if ( ((CollisionRadius > Other->CollisionRadius) || (CollisionHeight > Other->CollisionHeight)) 
				&& GetLevel()->FarMoveActor(this, aPoint, 1) )
	{
		aPoint = Location;
		GetLevel()->FarMoveActor(this, realLoc,1,1);
	}
	return Reachable(aPoint, Other);
	unguard;
}

int APawn::pointReachable(FVector aPoint, int bKnowVisible)
{
	guard(APawn::pointReachable);

	if (!GIsEditor)
	{
		FVector Dir2D = aPoint - Location;
		Dir2D.Z = 0.f;
		if (Dir2D.SizeSquared() > MAXPATHDISTSQ) 
			return 0;
	}

	//check aPoint visible
	if ( !bKnowVisible )
	{
		FVector	ViewPoint = Location;
		ViewPoint.Z += BaseEyeHeight; //look from eyes
		FCheckResult Hit(1.f);
		GetLevel()->SingleLineCheck( Hit, this, aPoint, ViewPoint, TRACE_World|TRACE_StopAtFirstHit );
		if ( Hit.Actor )
			return 0;
	}

	FVector realLoc = Location;
	if ( GetLevel()->FarMoveActor(this, aPoint, 1) )
	{
		aPoint = Location; //adjust destination
		GetLevel()->FarMoveActor(this, realLoc,1,1);
	}
	return Reachable(aPoint, NULL);

	unguard;
}

/* Crouch()
*/
void APawn::Crouch(INT bTest)
{
	guard(APawn::Crouch);

	if ( (CollisionHeight == CrouchHeight) && (CollisionRadius == CrouchRadius) )
		return;

	FVector DefaultPrePivot = GetClass()->GetDefaultActor()->PrePivot;
	FLOAT OldHeight = CollisionHeight;
	FLOAT OldRadius = CollisionRadius;
	SetCollisionSize(CrouchRadius, CrouchHeight); // change collision height and/or radius
	UBOOL bEncroached = false;

	FLOAT HeightAdjust = OldHeight - CrouchHeight;
	if ( !bTest && ((CrouchRadius > OldRadius) || (CrouchHeight > OldHeight)) )
	{
		FMemMark Mark(GMem);
		FCheckResult* FirstHit = GetLevel()->Hash->ActorEncroachmentCheck( GMem, 
			this, Location - FVector(0,0,HeightAdjust), Rotation, TRACE_Pawns | TRACE_Movers | TRACE_Others, 0 );	
		for( FCheckResult* Test = FirstHit; Test!=NULL; Test=Test->GetNext() )
			if ( (Test->Actor != this) && IsBlockedBy(Test->Actor) )
			{
				bEncroached = true;
				break;
			}
		Mark.Pop();
	}
	if ( bEncroached )
	{
		PrePivot = DefaultPrePivot;
		SetCollisionSize(OldRadius,OldHeight);
		return;
	}


	GetLevel()->FarMoveActor(this, Location - FVector(0,0,HeightAdjust), bTest,false,true); //move collision box down
	if ( !bTest )
	{
		PrePivot = DefaultPrePivot + FVector(0,0,HeightAdjust); // move mesh within collision box
		bNetDirty = true; // prepivot and bIsCrouched replication controlled by bNetDirty
		bIsCrouched = true;
		eventStartCrouch(HeightAdjust);
	}


	unguard;
}

void APawn::UnCrouch(INT bTest)
{
	guard(APawn::UnCrouch);

	// see if space to uncrouch
	FCheckResult Hit(1.f);
	FLOAT HeightAdjust = ((AActor *)(GetClass()->GetDefaultObject()))->CollisionHeight - CollisionHeight;

	UBOOL bEncroached = false;
	SetCollisionSize(((AActor*)(GetClass()->GetDefaultObject()))->CollisionRadius, ((AActor *)(GetClass()->GetDefaultObject()))->CollisionHeight);
	if ( !bTest )
	{
		FMemMark Mark(GMem);
		FCheckResult* FirstHit = GetLevel()->Hash->ActorEncroachmentCheck( GMem, 
			this, Location + FVector(0.f,0.f,HeightAdjust), Rotation, TRACE_Pawns | TRACE_Movers | TRACE_Others, 0 );	
		for( FCheckResult* Test = FirstHit; Test!=NULL; Test=Test->GetNext() )
			if ( (Test->Actor != this) && IsBlockedBy(Test->Actor) )
			{
				bEncroached = true;
				break;
			}
		Mark.Pop();
	}

	if ( bEncroached || !GetLevel()->FarMoveActor(this, Location + FVector(0.f,0.f,HeightAdjust), bTest,false,true) )
		SetCollisionSize(CrouchRadius, CrouchHeight);
	else
	{
		// space enough to uncrouch, so stand up
		if ( !bTest )
		{
			PrePivot = GetClass()->GetDefaultActor()->PrePivot;
			bNetDirty = true; // prepivot and bIsCrouched replication controlled by bNetDirty
			bIsCrouched = false;
			eventEndCrouch(HeightAdjust);
		}
	}
	
	unguard;
}

int APawn::Reachable(FVector aPoint, AActor* GoalActor)
{
	guard(APawn::Reachable);

	UBOOL bShouldUncrouch = false;
	INT Result = 0;
	if ( bCanCrouch && !bIsCrouched )
	{
		// crouch for reach test
		bShouldUncrouch = true;
		Crouch(1);
	}
	if ( PhysicsVolume->bWaterVolume )
		Result = swimReachable(aPoint, 0, GoalActor);
	else if ( PhysicsVolume->IsA(ALadderVolume::StaticClass()) )
		Result = ladderReachable(aPoint, 0, GoalActor);
	else if ( (Physics == PHYS_Walking) || (Physics == PHYS_Swimming) || (Physics == PHYS_Ladder) || (Physics == PHYS_Falling) ) 
		Result = walkReachable(aPoint, 0, GoalActor);
	else if (Physics == PHYS_Flying)
		Result = flyReachable(aPoint, 0, GoalActor);

	if ( bShouldUncrouch )
	{
		//return to full height
		UnCrouch(1);
	}
	return Result;
	unguard;
}

/* 
	ReachedDestination()
	Return true if sufficiently close to destination.
*/
UBOOL APawn::ReachedDestination(FVector Dir, AActor* GoalActor)
{
	guard(APawn::ReachedDestination);

	// if crouched, Z threshold should be based on standing
	FLOAT FullHeight = ((AActor *)(GetClass()->GetDefaultObject()))->CollisionHeight;
	FLOAT UpThreshold = FullHeight + FullHeight - CollisionHeight;
	FLOAT DownThreshold = CollisionHeight;
	FLOAT Threshold = CollisionRadius;

	if ( GoalActor )
	{
		// Navigation points must have their center reached for efficient navigation
		if ( GoalActor->IsA(ANavigationPoint::StaticClass()) )
		{
			if ( GoalActor->IsA(ALadder::StaticClass()) )
			{
				// look at difference along ladder direction
				return ( OnLadder && (Abs(Dir | OnLadder->ClimbDir) < CollisionHeight) );
			}
			if ( GoalActor->bCollideActors && GoalActor->IsA(ATeleporter::StaticClass()) && !GIsEditor )
			{
				// must touch teleporter
				for ( INT i=0; i<GoalActor->Touching.Num(); i++ )
					if ( GoalActor->Touching(i) == this )
						return true;
				return false;
			}

			UpThreshold = ::Max<FLOAT>(UpThreshold,2.f + GoalActor->CollisionHeight - CollisionHeight + UCONST_MAXSTEPHEIGHT);
			DownThreshold = ::Max<FLOAT>(DownThreshold, 2.f + CollisionHeight + UCONST_MAXSTEPHEIGHT - GoalActor->CollisionHeight);
		}
		else if ( GoalActor->IsA(APawn::StaticClass()) )
		{
			UpThreshold = ::Max(UpThreshold,GoalActor->CollisionHeight);
			if ( GoalActor->Physics == PHYS_Falling )
				UpThreshold += 2.f * MAXJUMPHEIGHT;
			DownThreshold = ::Max(DownThreshold,GoalActor->CollisionHeight);
			Threshold = CollisionRadius + ::Min(1.5f * CollisionRadius, MeleeRange) + GoalActor->CollisionRadius;
		}
		else
		{
			UpThreshold += GoalActor->CollisionHeight;
			DownThreshold += GoalActor->CollisionHeight;
			if ( GoalActor->bBlockActors || GIsEditor )
				Threshold = CollisionRadius + GoalActor->CollisionRadius;
		}
		Threshold += DestinationOffset;
	}

	FLOAT Zdiff = Dir.Z;
	Dir.Z = 0.f;
	if ( Dir.SizeSquared() > Threshold * Threshold )
		return false;

	if ( (Zdiff > 0.f) ? (Abs(Zdiff) > UpThreshold) : (Abs(Zdiff) > DownThreshold) )
	{
		if ( (Zdiff > 0.f) ? (Abs(Zdiff) > 2.f * UpThreshold) : (Abs(Zdiff) > 2.f * DownThreshold) )
			return false;

		// Check if above or below because of slope
		FCheckResult Hit(1.f);
		GetLevel()->SingleLineCheck( Hit, this, Location - FVector(0.f,0.f,6.f), Location, TRACE_World, FVector(CollisionRadius,CollisionRadius,CollisionHeight));
		if ( (Hit.Normal.Z < 0.95f) && (Hit.Normal.Z >= UCONST_MINFLOORZ) )
		{
			// check if above because of slope
			if ( (Zdiff < 0) 
				&& (Zdiff * -1.f < FullHeight + CollisionRadius * appSqrt(1.f/(Hit.Normal.Z * Hit.Normal.Z) - 1.f)) )
				return true;
			else 
			{
				// might be below because on slope
				FLOAT adjRad = (GoalActor != NULL) ? GoalActor->CollisionRadius : ANavigationPoint::StaticClass()->GetDefaultActor()->CollisionRadius; 
				if ( (CollisionRadius < adjRad) 
					&& (Zdiff < FullHeight + (adjRad + 15.f - CollisionRadius) * appSqrt(1.f/(Hit.Normal.Z * Hit.Normal.Z) - 1.f)) ) 
					return true;
			}
		}
		return false;
	}
	return true;
	unguard;
}

/* ladderReachable()
Pawn is on ladder. Return false if no GoalActor, else true if GoalActor is on the same ladder
*/
int APawn::ladderReachable(FVector Dest, int reachFlags, AActor* GoalActor)
{
	guard(APawn::ladderReachable);

	if ( !OnLadder || !GoalActor || ((GoalActor->Physics != PHYS_Ladder) && !GoalActor->IsA(ALadder::StaticClass())) )
		return walkReachable(Dest, reachFlags, GoalActor);
	
	ALadder *L = Cast<ALadder>(GoalActor);
	ALadderVolume *GoalLadder = NULL;
	if ( L )
		GoalLadder = L->MyLadder;
	else
	{
		APawn *GoalPawn = GoalActor->IsAPawn() ? Cast<APawn>(GoalActor) : NULL;
		if ( GoalPawn && GoalPawn->OnLadder )
			GoalLadder = GoalPawn->OnLadder;
		else
			return walkReachable(Dest, reachFlags, GoalActor);
	}

	if ( GoalLadder == OnLadder )
		return (reachFlags | R_LADDER);
	return walkReachable(Dest, reachFlags, GoalActor);
	unguard;
}

int APawn::flyReachable(FVector Dest, int reachFlags, AActor* GoalActor)
{
	guard(APawn::flyReachable);

	reachFlags = reachFlags | R_FLY;
	int success = 0;
	FVector OriginalPos = Location;
	FVector realVel = Velocity;
	ETestMoveResult stillmoving = TESTMOVE_Moved;
	FVector Direction = Dest - Location;
	FLOAT Movesize = ::Max(200.f, CollisionRadius);
	FLOAT MoveSizeSquared = Movesize * Movesize;
	int ticks = 100; 

	while (stillmoving != TESTMOVE_Stopped ) 
	{
		Direction = Dest - Location;
		if ( !ReachedDestination(Direction, GoalActor) )  
		{
			if ( Direction.SizeSquared() < MoveSizeSquared ) 
				stillmoving = flyMove(Direction, GoalActor, 8.f);
			else
			{
				Direction = Direction.SafeNormal();
				stillmoving = flyMove(Direction * Movesize, GoalActor, MINMOVETHRESHOLD);
			}
			if ( stillmoving == TESTMOVE_HitGoal ) //bumped into goal
			{
				stillmoving = TESTMOVE_Stopped;
				success = 1;
			}
			if ( stillmoving && PhysicsVolume->bWaterVolume )
			{
				stillmoving = TESTMOVE_Stopped;
				if ( bCanSwim && !HurtByVolume(this) )
				{
					reachFlags = swimReachable(Dest, reachFlags, GoalActor);
					success = reachFlags;
				}
			}
		}
		else
		{
			stillmoving = TESTMOVE_Stopped;
			success = 1;
		}
		ticks--;
		if (ticks < 0)
			stillmoving = TESTMOVE_Stopped;
	}

	if ( !success && GoalActor && GoalActor->IsA(AWarpZoneMarker::StaticClass()) )
		success = ( Region.Zone == ((AWarpZoneMarker *)GoalActor)->markedWarpZone );
	GetLevel()->FarMoveActor(this, OriginalPos, 1, 1); //move actor back to starting point
	Velocity = realVel;	
	if (success)
		return reachFlags;
	else
		return 0;
	unguard;
}

int APawn::swimReachable(FVector Dest, int reachFlags, AActor* GoalActor)
{
	guard(APawn::swimReachable);

	//debugf("Swim reach to %f %f %f from %f %f %f",Dest.X,Dest.Y,Dest.Z,Location.X,Location.Y, Location.Z);
	reachFlags = reachFlags | R_SWIM;
	int success = 0;
	FVector OriginalPos = Location;
	FVector realVel = Velocity;
	ETestMoveResult stillmoving = TESTMOVE_Moved;
	FVector Direction = Dest - Location;
	FLOAT Movesize = ::Max(200.f, CollisionRadius);
	FLOAT MoveSizeSquared = Movesize * Movesize;
	int ticks = 100; 

	while ( stillmoving != TESTMOVE_Stopped ) 
	{
		Direction = Dest - Location;
		if ( !ReachedDestination(Direction,GoalActor) )  
		{
			if ( Direction.SizeSquared() < MoveSizeSquared ) 
				stillmoving = swimMove(Direction, GoalActor, 8.f);
			else
			{
				Direction = Direction.SafeNormal();
				stillmoving = swimMove(Direction * Movesize, GoalActor, MINMOVETHRESHOLD);
			}
			if ( stillmoving == TESTMOVE_HitGoal ) //bumped into goal
			{
				stillmoving = TESTMOVE_Stopped;
				success = 1;
			}
			if ( !PhysicsVolume->bWaterVolume )
			{
				stillmoving = TESTMOVE_Stopped;
				if (bCanFly)
				{
					reachFlags = flyReachable(Dest, reachFlags, GoalActor);
					success = reachFlags;
				}
				else if ( bCanWalk && (Dest.Z < Location.Z + HUMANHEIGHT + UCONST_MAXSTEPHEIGHT) ) 
				{
					FCheckResult Hit(1.f);
					FLOAT MaxStepHeight = UCONST_MAXSTEPHEIGHT;
					GetLevel()->MoveActor(this, FVector(0,0,::Max(CollisionHeight + MaxStepHeight,Dest.Z - Location.Z)), Rotation, Hit, 1, 1);
					if (Hit.Time == 1.f)
					{
						success = flyReachable(Dest, reachFlags, GoalActor);
						reachFlags = R_WALK | (success & !R_FLY);
					}
				}
			}
			else if ( HurtByVolume(this) )
			{
				stillmoving = TESTMOVE_Stopped;
				success = 0;
			}
		}
		else
		{
			stillmoving = TESTMOVE_Stopped;
			success = 1;
		}
		ticks--;
		if (ticks < 0)
			stillmoving = TESTMOVE_Stopped;
	}

	if ( !success && GoalActor && GoalActor->IsA(AWarpZoneMarker::StaticClass()) )
		success = ( Region.Zone == ((AWarpZoneMarker *)GoalActor)->markedWarpZone );

	GetLevel()->FarMoveActor(this, OriginalPos, 1, 1); //move actor back to starting point
	Velocity = realVel;	
	if (success)
		return reachFlags;
	else
		return 0;
	unguard;
}

/*walkReachable() -
walkReachable returns 0 if Actor cannot reach dest, and 1 if it can reach dest by moving in
 straight line
 actor must remain on ground at all times
 Note that Actor is not actually moved
*/
int APawn::walkReachable(FVector Dest, int reachFlags, AActor* GoalActor)
{
	guard(APawn::walkReachable);

	reachFlags = reachFlags | R_WALK;
	INT success = 0;
	FVector OriginalPos = Location;
	FVector realVel = Velocity;
	ETestMoveResult stillmoving = TESTMOVE_Moved;
	FLOAT Movesize = 16.f;
	FVector Direction;
	if (!GIsEditor)
	{
		if ( bJumpCapable )
			Movesize = ::Max(128.f, CollisionRadius);
		else
			Movesize = ::Max(12.f, CollisionRadius);
	}
	
	int ticks = 100; 
	FLOAT MoveSizeSquared = Movesize * Movesize;
	FCheckResult Hit(1.f);

	while ( stillmoving == TESTMOVE_Moved ) 
	{
		Direction = Dest - Location;
		if ( ReachedDestination(Direction, GoalActor) ) 
		{
			stillmoving = TESTMOVE_Stopped;
			success = 1;
		}
		else
		{
			Direction.Z = 0; //this is a 2D move
			FLOAT DistanceSquared = Direction.SizeSquared(); //2D size
			if ( DistanceSquared < MoveSizeSquared) 
				stillmoving = walkMove(Direction, Hit, GoalActor, 8.f);
			else
			{
				Direction = Direction.SafeNormal();
				Direction *= Movesize;
				stillmoving = walkMove(Direction, Hit, GoalActor, MINMOVETHRESHOLD);
			} 
			if (stillmoving != TESTMOVE_Moved)
			{
				if ( stillmoving == TESTMOVE_HitGoal ) //bumped into goal
				{
					stillmoving = TESTMOVE_Stopped;
					success = 1;
				}
				else if ( Region.ZoneNumber == 0 )
				{
					debugf(TEXT("Walked out of world!!!"));
					stillmoving = TESTMOVE_Stopped;
					success = 0;
				}
				else if (bCanFly)
				{
					stillmoving = TESTMOVE_Stopped;
					reachFlags = flyReachable(Dest, reachFlags, GoalActor);
					success = reachFlags;
				}
				else if ( bJumpCapable ) 
				{
					reachFlags = reachFlags | R_JUMP;	
					if (stillmoving == TESTMOVE_Fell) 
					{
						FVector Landing = Dest;
						if ( GoalActor )
							Landing.Z = Landing.Z - GoalActor->CollisionHeight + CollisionHeight;
						stillmoving = FindBestJump(Landing);
					}
					else if (stillmoving == TESTMOVE_Stopped)
					{
						stillmoving = FindJumpUp(Direction);
						if ( stillmoving == TESTMOVE_HitGoal ) //bumped into goal
						{
							stillmoving = TESTMOVE_Stopped;
							success = 1;
						}
					}
				}
				else if ( (stillmoving == TESTMOVE_Fell) && (Movesize > UCONST_MAXSTEPHEIGHT) ) //try smaller  
				{
					stillmoving = TESTMOVE_Moved;
					Movesize = UCONST_MAXSTEPHEIGHT;
				}
			}
			else if ( GIsEditor )// FIXME - make sure fully on path
			{
				FCheckResult Hit(1.f);
				GetLevel()->SingleLineCheck(Hit, this, Location + FVector(0,0,-1 * (0.5f * CollisionHeight + UCONST_MAXSTEPHEIGHT + 4.0)) , Location, TRACE_World|TRACE_StopAtFirstHit, 0.5f * GetCylinderExtent());
				if ( Hit.Time == 1.f )
					reachFlags = reachFlags | R_JUMP;	
			}
			
			if ( HurtByVolume(this) ) 
			{
				stillmoving = TESTMOVE_Stopped;
				success = 0;
			}
			else if ( PhysicsVolume->bWaterVolume ) 
			{
				//debugf("swim from walk");
				stillmoving = TESTMOVE_Stopped;
				if (bCanSwim )
				{
					reachFlags = swimReachable(Dest, reachFlags, GoalActor);
					success = reachFlags;
				}
			}
			else if ( bCanClimbLadders && PhysicsVolume->IsA(ALadderVolume::StaticClass()) 
					&& GoalActor && (GoalActor->PhysicsVolume == PhysicsVolume) )
			{
				stillmoving = TESTMOVE_Stopped;
				ALadderVolume *RealLadder = OnLadder;
				OnLadder = Cast<ALadderVolume>(PhysicsVolume);
				success = ( Abs((Dest - Location) | OnLadder->ClimbDir) < ::Max(CollisionHeight,GoalActor->CollisionHeight) );
				OnLadder = RealLadder;
			}

			ticks--;
			if (ticks < 0)
			{
				//debugf(TEXT("OUT OF TICKS"));
				stillmoving = TESTMOVE_Stopped;
			}
		}
	}
	if ( !success && GoalActor && GoalActor->IsA(AWarpZoneMarker::StaticClass()) )
		success = ( Region.Zone == ((AWarpZoneMarker *)GoalActor)->markedWarpZone );

	GetLevel()->FarMoveActor(this, OriginalPos, 1, 1); //move actor back to starting point
	Velocity = realVel;
	if (success)
		return reachFlags;
	else
		return 0;
	unguard;
}

int APawn::jumpReachable(FVector Dest, int reachFlags, AActor* GoalActor)
{
	guard(APawn::jumpReachable);

	// debugf(TEXT("Jump reach to %f %f %f from %f %f %f"),Dest.X,Dest.Y,Dest.Z,Location.X,Location.Y, Location.Z);
	FVector OriginalPos = Location;
	reachFlags = reachFlags | R_JUMP;
	if ( jumpLanding(Velocity, 1) == TESTMOVE_Stopped ) 
		return 0;
	int success = walkReachable(Dest, reachFlags, GoalActor);
	GetLevel()->FarMoveActor(this, OriginalPos, 1, 1); //move actor back to starting point
	return success;
	unguard;
}

/* jumpLanding()
determine landing position of current fall, given testVel as initial velocity.
Assumes near-zero acceleration by pawn during jump (make sure creatures do this in script 
- e.g. no air control)
*/
ETestMoveResult APawn::jumpLanding(FVector testVel, int movePawn)
{
	guard(APawn::jumpLanding);

	FVector OriginalPos = Location;
	int landed = 0;
	int ticks = 0;
	FLOAT tickTime = 0.1f;
	//debugf("Jump vel %f %f %f", testVel.X, testVel.Y, testVel.Z);
	while ( !landed )
	{
		FLOAT NetFluidFriction = PhysicsVolume->bWaterVolume ? PhysicsVolume->FluidFriction : 0.f;
		testVel = testVel * (1 - NetFluidFriction * tickTime) + PhysicsVolume->Gravity * tickTime; // FIXME - why not *0.5 for gravity?
		FVector Adjusted = (testVel + PhysicsVolume->ZoneVelocity) * tickTime;
		FCheckResult Hit(1.f);
		GetLevel()->MoveActor(this, Adjusted, Rotation, Hit, 1, 1);
		if( PhysicsVolume->bWaterVolume ) 
			landed = 1;
		else if ( testVel.Z < -1.f * MaxFallSpeed )
		{
			landed = 1;
			GetLevel()->FarMoveActor(this, OriginalPos, 1, 1); //move actor back to starting point
		}
		else if(Hit.Time < 1.f)
		{
			if( Hit.Normal.Z >= UCONST_MINFLOORZ )
				landed = 1;
			else
			{
				FVector OldHitNormal = Hit.Normal;
				FVector Delta = (Adjusted - Hit.Normal * (Adjusted | Hit.Normal)) * (1.f - Hit.Time);
				if( (Delta | Adjusted) >= 0 )
				{
					GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
					if (Hit.Time < 1.f) //hit second wall
					{
						if (Hit.Normal.Z >= UCONST_MINFLOORZ)
							landed = 1;	
						FVector DesiredDir = Adjusted.SafeNormal();
						TwoWallAdjust(DesiredDir, Delta, Hit.Normal, OldHitNormal, Hit.Time);
						GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
						if (Hit.Normal.Z >= UCONST_MINFLOORZ)
							landed = 1;
					}
				}
			}
		}
		ticks++;
		if ( (Region.ZoneNumber == 0) || (ticks > 35) ) 
		{
			GetLevel()->FarMoveActor(this, OriginalPos, 1, 1); //move actor back to starting point
			landed = 1;
		}
	}

	AScout *S = Cast<AScout>(this);
	if ( S )
		S->MaxLandingVelocity = ::Max(S->MaxLandingVelocity, -1.f * testVel.Z);
	FVector Landing = Location;
	if (!movePawn)
		GetLevel()->FarMoveActor(this, OriginalPos, 1, 1); //move actor back to starting point

	if ( Landing != OriginalPos )
		return TESTMOVE_Moved;
	else
		return TESTMOVE_Stopped;
	unguard;
}

ETestMoveResult APawn::FindJumpUp(FVector Direction)
{
	guard(APawn::FindJumpUp);

	FCheckResult Hit(1.f);
	FVector StartLocation = Location;
	GetLevel()->MoveActor(this, FVector(0,0,MAXJUMPHEIGHT - UCONST_MAXSTEPHEIGHT), Rotation, Hit, 1, 1);
	ETestMoveResult success = walkMove(Direction, Hit, NULL, MINMOVETHRESHOLD);

	StartLocation.Z = Location.Z;
	if ( success != TESTMOVE_Stopped ) 
	{
		GetLevel()->MoveActor(this, FVector(0,0,-1.f * MAXJUMPHEIGHT), Rotation, Hit, 1, 1);
		// verify walkmove didn't just step down
		StartLocation.Z = Location.Z;
		if ((StartLocation - Location).SizeSquared() < MINMOVETHRESHOLD * MINMOVETHRESHOLD) 
			return TESTMOVE_Stopped;
	}
	else
		GetLevel()->FarMoveActor(this, StartLocation, 1, 1);

	return success;

	unguard;
}

FVector AActor::SuggestFallVelocity(FVector Dest, FVector Start, FLOAT XYSpeed, FLOAT BaseZ, FLOAT JumpZ, FLOAT MaxXYSpeed)
{
	guard(AActor::SuggestFallVelocity);

	FVector SuggestedVelocity = Dest - Start;
	FLOAT DistZ = SuggestedVelocity.Z;
	SuggestedVelocity.Z = 0.f;
	FLOAT XYDist = SuggestedVelocity.Size();
	if ( (XYDist == 0.f) || (XYSpeed == 0.f) )
		return FVector(0.f,0.f,::Max(BaseZ,JumpZ));
	SuggestedVelocity = SuggestedVelocity/XYDist;

	// check for negative gravity
	if ( PhysicsVolume->Gravity.Z >= 0 ) 
		return (SuggestedVelocity * XYSpeed);

	FLOAT GravityZ = 0.5f * PhysicsVolume->Gravity.Z;

	//determine how long I might be in the air 
	FLOAT ReachTime = XYDist/XYSpeed; 	

	// reduce time in low grav if above dest
	if ( (DistZ < 0.f) && (PhysicsVolume->Gravity.Z > ((APhysicsVolume *)(PhysicsVolume->GetClass()->GetDefaultActor()))->Gravity.Z) )
	{
		// reduce by time to fall DistZ
		ReachTime = ReachTime - appSqrt(DistZ/GravityZ);
	}

	// calculate starting Z velocity so end at dest Z position
	SuggestedVelocity.Z = DistZ/ReachTime - GravityZ * ReachTime;

	if ( (DistZ > 0.f) && (PhysicsVolume->Gravity.Z <= ((APhysicsVolume *)(PhysicsVolume->GetClass()->GetDefaultActor()))->Gravity.Z) )
		SuggestedVelocity.Z += 50.f;

	if ( SuggestedVelocity.Z < BaseZ )
	{
		// reduce XYSpeed
		// solve quadratic for ReachTime
		ReachTime = (-1.f * BaseZ + appSqrt(BaseZ * BaseZ + 4.f * GravityZ * DistZ))/(2.f * GravityZ);
		ReachTime = ::Max(ReachTime, 0.05f);
		XYSpeed = Min(MaxXYSpeed,XYDist/ReachTime);
		SuggestedVelocity.Z = BaseZ;
	}
	else if ( SuggestedVelocity.Z > BaseZ + JumpZ )
	{
		XYSpeed *= (BaseZ + JumpZ)/SuggestedVelocity.Z;
		SuggestedVelocity.Z = BaseZ + JumpZ;
	}

	SuggestedVelocity.X *= XYSpeed;
	SuggestedVelocity.Y *= XYSpeed;

	return SuggestedVelocity;
	unguard;
}

/* SuggestJumpVelocity()
returns a recommended Jump velocity vector, given a desired speed in the XY direction,
a minimum Z direction velocity, and a maximum Z direction jump velocity (JumpZ) to reach
the Dest point
*/
FVector APawn::SuggestJumpVelocity(FVector Dest, FLOAT XYSpeed, FLOAT BaseZ)
{
	guard(APawn::SuggestJumpVelocity);

	if ( GIsEditor )
		return SuggestFallVelocity(Dest, Location, XYSpeed, BaseZ, JumpZ, GroundSpeed);

	FLOAT GravityZ = 0.5f * PhysicsVolume->Gravity.Z;

	FVector SuggestedVelocity = Dest - Location;
	FLOAT DistZ = SuggestedVelocity.Z;
	SuggestedVelocity.Z = 0.f;
	FLOAT XYDist = SuggestedVelocity.Size();
	if ( XYDist == 0.f )
		return FVector(0.f,0.f,::Max(BaseZ,JumpZ));
	SuggestedVelocity = SuggestedVelocity/XYDist;

	// check for negative gravity
	if ( PhysicsVolume->Gravity.Z >= 0 ) 
	{
		SuggestedVelocity *= GroundSpeed;
		return SuggestedVelocity;
	}

	//FIXME - remove XYSpeed parameter
	// start with Z of either BaseZ or BaseZ+JumpZ
	FLOAT MinReachTime = XYDist/GroundSpeed; 
	FLOAT SuggestedZ = DistZ/MinReachTime - GravityZ * MinReachTime;
	if ( SuggestedZ > BaseZ )
	{
		if ( SuggestedZ > BaseZ + 0.5f * JumpZ )
			SuggestedZ = BaseZ + JumpZ;
		else
			SuggestedZ = BaseZ + 0.5f * JumpZ;
	}
	else
		SuggestedZ = BaseZ;

	FLOAT X = SuggestedZ * SuggestedZ + 4.f * DistZ * GravityZ;
	if ( X >= 0.f )
	{
		FLOAT ReachTime = (-1.f * SuggestedZ - appSqrt(X))/(2.f * GravityZ);
		XYSpeed = ::Min(GroundSpeed, XYDist/ReachTime);
	}
	SuggestedVelocity *= XYSpeed;
	SuggestedVelocity.Z = SuggestedZ;
	return SuggestedVelocity;

	unguard;
}

/* Find best jump from current position toward destination.  Assumes that there is no immediate 
barrier.  Sets Landing to the expected Landing, and moves actor if moveActor is set 
*/
ETestMoveResult APawn::FindBestJump(FVector Dest)
{
	guard(APawn::FindBestJump);

	FVector realLocation = Location;

	//debugf("Jump best to %f %f %f from %f %f %f",Dest.X,Dest.Y,Dest.Z,Location.X,Location.Y, Location.Z);
	FVector vel = SuggestJumpVelocity(Dest, GroundSpeed, 0.f);

	// Now imagine jump
	ETestMoveResult success = jumpLanding(vel, 1);
	if ( success != TESTMOVE_Stopped )
	{
		if ( HurtByVolume(this) ) 
			success = TESTMOVE_Stopped;
		else if (!bCanSwim && PhysicsVolume->bWaterVolume )
			success = TESTMOVE_Stopped;
		else
		{
			FVector olddist = Dest - realLocation;
			FVector dist = Dest - Location;
			FLOAT netchange = olddist.Size2D();
			netchange -= dist.Size2D();
			if ( netchange > 0.f )
				success = TESTMOVE_Moved;
			else
				success = TESTMOVE_Stopped;
		}
	}
	//debugf("New Loc %f %f %f success %d",Location.X, Location.Y, Location.Z, success);
	// FIXME? - if failed, imagine with no jumpZ (step out first)
	return success;
	unguard;
}

ETestMoveResult APawn::HitGoal(AActor *GoalActor)
{
	guard(APawn::HitGoal);

	if ( GoalActor->IsA(ANavigationPoint::StaticClass()) )
		return TESTMOVE_Stopped;

	return TESTMOVE_HitGoal; 
	unguard;
}

/* walkMove() 
- returns 1 if move happened, zero if it didn't because of barrier, and -1
if it didn't because of ledge
Move direction must not be adjusted.
*/
ETestMoveResult APawn::walkMove(FVector Delta, FCheckResult& Hit, AActor* GoalActor, FLOAT threshold)
{
	guard(APawn::walkMove);

	FVector StartLocation = Location;
	Delta.Z = 0.f;
	//-------------------------------------------------------------------------------------------
	//Perform the move
	FVector GravDir = FVector(0.f,0.f,-1.f);
	if ( PhysicsVolume->Gravity.Z > 0.f )
		GravDir.Z = 1.f; 
	FVector Down = GravDir * UCONST_MAXSTEPHEIGHT;

	GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
	if ( GoalActor && (Hit.Actor == GoalActor) )
		return HitGoal(GoalActor); 
	FVector StopLocation = Location;
	if(Hit.Time < 1.f) //try to step up
	{
		Delta = Delta * (1.f - Hit.Time);
		GetLevel()->MoveActor(this, -1.f * Down, Rotation, Hit, 1, 1); 
		GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
		if ( GoalActor && (Hit.Actor == GoalActor) )
			return HitGoal(GoalActor); 
		GetLevel()->MoveActor(this, Down, Rotation, Hit, 1, 1);
		if ( (Hit.Time < 1.f) && (Hit.Normal.Z < UCONST_MINFLOORZ) )
		{
			//Want only good floors, else undo move
			GetLevel()->FarMoveActor(this, StopLocation, 1, 1);
			return TESTMOVE_Stopped;
		}
	}

	//drop to floor
	FVector Loc = Location;
	Down = GravDir * (UCONST_MAXSTEPHEIGHT + 2.f);
	GetLevel()->MoveActor(this, Down, Rotation, Hit, 1, 1);
	if ( (Hit.Time == 1.f) || (Hit.Normal.Z < UCONST_MINFLOORZ) ) //then falling
	{
		GetLevel()->FarMoveActor(this, Loc, 1, 1);
		return TESTMOVE_Fell;
	}
	if ( GoalActor && (Hit.Actor == GoalActor) )
		return HitGoal(GoalActor); 

	//check if move successful
	if ((Location - StartLocation).SizeSquared() < threshold * threshold) 
		return TESTMOVE_Stopped;

	return TESTMOVE_Moved;
	unguard;
}

ETestMoveResult APawn::flyMove(FVector Delta, AActor* GoalActor, FLOAT threshold)
{
	guard(APawn::flyMove);

	FVector StartLocation = Location;
	FVector Down = FVector(0,0,-1) * UCONST_MAXSTEPHEIGHT;
	FVector Up = -1 * Down;
	FCheckResult Hit(1.f);

	GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
	if ( GoalActor && (Hit.Actor == GoalActor) )
		return HitGoal(GoalActor);
	if (Hit.Time < 1.f) //try to step up
	{
		Delta = Delta * (1.f - Hit.Time);
		GetLevel()->MoveActor(this, Up, Rotation, Hit, 1, 1); 
		GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
		if ( GoalActor && (Hit.Actor == GoalActor) )
			return HitGoal(GoalActor);
	}

	if ((Location - StartLocation).SizeSquared() < threshold * threshold) 
		return TESTMOVE_Stopped;

	return TESTMOVE_Moved;
	unguard;
}

ETestMoveResult APawn::swimMove(FVector Delta, AActor* GoalActor, FLOAT threshold)
{
	guard(APawn::swimMove);

	FVector StartLocation = Location;
	FVector Down = FVector(0,0,-1) * UCONST_MAXSTEPHEIGHT;
	FVector Up = -1 * Down;
	FCheckResult Hit(1.f);

	GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
	if ( GoalActor && (Hit.Actor == GoalActor) )
		return HitGoal(GoalActor);
	if ( !PhysicsVolume->bWaterVolume )
	{
		FVector End = findWaterLine(StartLocation, Location);
		if (End != Location)
			GetLevel()->MoveActor(this, End - Location, Rotation, Hit, 1, 1);
		return TESTMOVE_Stopped;
	}
	else if (Hit.Time < 1.f) //try to step up
	{
		Delta = Delta * (1.f - Hit.Time);
		GetLevel()->MoveActor(this, Up, Rotation, Hit, 1, 1); 
		GetLevel()->MoveActor(this, Delta, Rotation, Hit, 1, 1);
		if ( GoalActor && (Hit.Actor == GoalActor) )
			return HitGoal(GoalActor); //bumped into goal
	}

	if ((Location - StartLocation).SizeSquared() < threshold * threshold) 
		return TESTMOVE_Stopped;

	return TESTMOVE_Moved;
	unguard;
}

/* PickWallAdjust()
Check if could jump up over obstruction (only if there is a knee height obstruction)
If so, start jump, and return current destination
Else, try to step around - return a destination 90 degrees right or left depending on traces
out and floor checks
*/
UBOOL APawn::PickWallAdjust(FVector WallHitNormal)
{
	guard(APawn::PickWallAdjust);

	if ( (Physics == PHYS_Falling) || !Controller )
		return false;

	if ( (Physics == PHYS_Flying) || (Physics == PHYS_Swimming) )
		return Pick3DWallAdjust(WallHitNormal);

	// first pick likely dir with traces, then check with testmove
	FCheckResult Hit(1.f);
	FVector ViewPoint = Location + FVector(0.f,0.f,BaseEyeHeight);
	FVector Dir = Controller->Destination - Location;
	FLOAT zdiff = Dir.Z;
	Dir.Z = 0.f;
	FLOAT AdjustDist = 1.5f * CollisionRadius + 16.f;
	AActor *MoveTarget = ( Controller->MoveTarget ? Controller->MoveTarget->AssociatedLevelGeometry() : NULL );

	if ( (zdiff < CollisionHeight) && ((Dir | Dir) - CollisionRadius * CollisionRadius < 0) )
		return false;
	FLOAT Dist = Dir.Size();
	if ( Dist == 0.f )
		return false;
	Dir = Dir/Dist;
	GetLevel()->SingleLineCheck( Hit, this, Controller->Destination, ViewPoint, TRACE_World|TRACE_StopAtFirstHit );
	if ( Hit.Actor && (Hit.Actor != MoveTarget) )
		AdjustDist += CollisionRadius;

	//look left and right
	FVector Left = FVector(Dir.Y, -1 * Dir.X, 0);
	INT bCheckRight = 0;
	FVector CheckLeft = Left * 1.4f * CollisionRadius;
	GetLevel()->SingleLineCheck(Hit, this, Controller->Destination, ViewPoint + CheckLeft, TRACE_World|TRACE_StopAtFirstHit); 
	if ( Hit.Actor && (Hit.Actor != MoveTarget) ) //try right
	{
		bCheckRight = 1;
		Left *= -1;
		CheckLeft *= -1;
		GetLevel()->SingleLineCheck(Hit, this, Controller->Destination, ViewPoint + CheckLeft, TRACE_World|TRACE_StopAtFirstHit); 
	}
	if ( Hit.Actor && (Hit.Actor != MoveTarget) ) //neither side has visibility
	{
		return false;
	}

	FVector Out = 14.f * Dir;
	if ( (Physics == PHYS_Walking) && bCanJump ) 
	{
		// try step up first
#if 0 //!!MERGE
		FLOAT JumpScale = bCanDoubleJump ? 2.f : 1.f;
#else
		FLOAT JumpScale = 1.f;
#endif
		FVector Up = FVector(0.f,0.f,JumpScale*MAXJUMPHEIGHT); 
		GetLevel()->SingleLineCheck(Hit, this, Location + Up, Location, TRACE_World, GetCylinderExtent());
		FLOAT FirstHit = Hit.Time;
		if ( FirstHit > 0.5f/JumpScale )
		{
			GetLevel()->SingleLineCheck(Hit, this, Location + Up * Hit.Time + Out, Location + Up * Hit.Time, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
			if ( (Hit.Time < 1.f) && (JumpScale > 1.f) && (FirstHit > 1.f/JumpScale) )
			{
				Up = FVector(0.f,0.f,MAXJUMPHEIGHT); 
				GetLevel()->SingleLineCheck(Hit, this, Location + Up + Out, Location + Up, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
			}
			if (Hit.Time == 1.f)
			{
				Dir = Controller->Destination - Location;
				Dir.Z = 0.f;
				Dir = Dir.SafeNormal();
				Velocity = GroundSpeed * Dir;
				Acceleration = AccelRate * Dir;
				Velocity.Z = JumpZ;
				bNoJumpAdjust = true; // don't let script adjust this jump again
				Controller->bJumpOverWall = true;
				Controller->bNotifyApex = true;
				setPhysics(PHYS_Falling);
				return 1;
			}
		}
	}

	//try step left or right
	Left *= AdjustDist;
	GetLevel()->SingleLineCheck(Hit, this, Location + Left, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
	if (Hit.Time == 1.f)
	{
		GetLevel()->SingleLineCheck(Hit, this, Location + Left + Out, Location + Left, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
		if (Hit.Time == 1.f)
		{
			Controller->SetAdjustLocation(Location + Left);
			return true;
		}
	}
	
	if ( !bCheckRight ) // if didn't already try right, now try it
	{
		CheckLeft *= -1;
		GetLevel()->SingleLineCheck(Hit, this, Controller->Destination, ViewPoint + CheckLeft, TRACE_World|TRACE_StopAtFirstHit); 
		if ( Hit.Time < 1.f )
			return false;
		Left *= -1;
		GetLevel()->SingleLineCheck(Hit, this, Location + Left, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
		if (Hit.Time == 1.f)
		{
			GetLevel()->SingleLineCheck(Hit, this, Location + Left + Out, Location + Left, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
			if (Hit.Time == 1.f)
			{
				Controller->SetAdjustLocation(Location + Left);
				return true;
			}
		}
	}
	return false;
	unguard;
}

/* Pick3DWallAdjust()
pick wall adjust when swimming or flying
*/
UBOOL APawn::Pick3DWallAdjust(FVector WallHitNormal)
{
	guard(APawn::Pick3DWallAdjust);

	// first pick likely dir with traces, then check with testmove
	FCheckResult Hit(1.f);
	FVector ViewPoint = Location + FVector(0.f,0.f,BaseEyeHeight);
	FVector Dir = Controller->Destination - Location;
	FLOAT zdiff = Dir.Z;
	Dir.Z = 0.f;
	FLOAT AdjustDist = 1.5f * CollisionRadius + 16.f;
	AActor *MoveTarget = ( Controller->MoveTarget ? Controller->MoveTarget->AssociatedLevelGeometry() : NULL );

	int bCheckUp = 0;
	if ( zdiff < CollisionHeight )
	{
		if ( (Dir | Dir) - CollisionRadius * CollisionRadius < 0 )
			return false;
		if ( Dir.SizeSquared() < 4 * CollisionHeight * CollisionHeight )
		{
			FVector Up = FVector(0,0,CollisionHeight);
			bCheckUp = 1;
			if ( Location.Z < Controller->Destination.Z )
			{
				bCheckUp = -1;
				Up *= -1;
			}
			GetLevel()->SingleLineCheck(Hit, this, Location + Up, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
			if (Hit.Time == 1.f)
			{
				FVector ShortDir = Dir.SafeNormal();
				ShortDir *= CollisionRadius;
				GetLevel()->SingleLineCheck(Hit, this, Location + Up + ShortDir, Location + Up, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
				if (Hit.Time == 1.f)
				{
					Controller->SetAdjustLocation(Location + Up);
					return true;
				}
			}
		}
	}

	FLOAT Dist = Dir.Size();
	if ( Dist == 0.f )
		return false;
	Dir = Dir/Dist;
	GetLevel()->SingleLineCheck( Hit, this, Controller->Destination, ViewPoint, TRACE_World|TRACE_StopAtFirstHit );
	if ( (Hit.Actor != MoveTarget) && (zdiff > 0) )
	{
		FVector Up = FVector(0,0, CollisionHeight);
		GetLevel()->SingleLineCheck(Hit, this, Location + 2 * Up, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
		if (Hit.Time == 1.f)
		{
			Controller->SetAdjustLocation(Location + Up);
			return true;
		}
	}

	//look left and right
	FVector Left = FVector(Dir.Y, -1 * Dir.X, 0);
	INT bCheckRight = 0;
	FVector CheckLeft = Left * 1.4f * CollisionRadius;
	GetLevel()->SingleLineCheck(Hit, this, Controller->Destination, ViewPoint + CheckLeft, TRACE_World|TRACE_StopAtFirstHit); 
	if ( Hit.Actor != MoveTarget ) //try right
	{
		bCheckRight = 1;
		Left *= -1;
		CheckLeft *= -1;
		GetLevel()->SingleLineCheck(Hit, this, Controller->Destination, ViewPoint + CheckLeft, TRACE_World|TRACE_StopAtFirstHit); 
	}

	if ( Hit.Actor != MoveTarget ) //neither side has visibility
		return false;

	FVector Out = 14.f * Dir;

	//try step left or right
	Left *= AdjustDist;
	GetLevel()->SingleLineCheck(Hit, this, Location + Left, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
	if (Hit.Time == 1.f)
	{
		GetLevel()->SingleLineCheck(Hit, this, Location + Left + Out, Location + Left, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
		if (Hit.Time == 1.f)
		{
			Controller->SetAdjustLocation(Location + Left);
			return true;
		}
	}
	
	if ( !bCheckRight ) // if didn't already try right, now try it
	{
		CheckLeft *= -1;
		GetLevel()->SingleLineCheck(Hit, this, Controller->Destination, ViewPoint + CheckLeft, TRACE_World|TRACE_StopAtFirstHit); 
		if ( Hit.Time < 1.f )
			return false;
		Left *= -1;
		GetLevel()->SingleLineCheck(Hit, this, Location + Left, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
		if (Hit.Time == 1.f)
		{
			GetLevel()->SingleLineCheck(Hit, this, Location + Left + Out, Location + Left, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
			if (Hit.Time == 1.f)
			{
				Controller->SetAdjustLocation(Location + Left);
				return true;
			}
		}
	}

	//try adjust up or down if swimming or flying
	FVector Up = FVector(0,0,CollisionHeight);

	if ( bCheckUp != 1 )
	{
		GetLevel()->SingleLineCheck(Hit, this, Location + Up, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
		if (Hit.Time == 1.f)
		{
			GetLevel()->SingleLineCheck(Hit, this, Location + Up + Out, Location + Up, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
			if (Hit.Time == 1.f)
			{
				Controller->SetAdjustLocation(Location + Up);
				return true;
			}
		}
	}

	if ( bCheckUp != -1 )
	{
		Up *= -1; //try adjusting down
		GetLevel()->SingleLineCheck(Hit, this, Location + Up, Location, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
		if (Hit.Time == 1.f)
		{
			GetLevel()->SingleLineCheck(Hit, this, Location + Up + Out, Location + Up, TRACE_World|TRACE_StopAtFirstHit, GetCylinderExtent());
			if (Hit.Time == 1.f)
			{
				Controller->SetAdjustLocation(Location + Up);
				return true;
			}
		}
	}

	return false;
	unguard;
}

int APawn::calcMoveFlags()
{
	guardSlow(APawn::calcMoveFlags);
	return ( R_FORCED + bCanWalk * R_WALK + bCanFly * R_FLY + bCanSwim * R_SWIM + bJumpCapable * R_JUMP 
			+ Controller->bCanOpenDoors * R_DOOR + Controller->bCanDoSpecial * R_SPECIAL 
			+ Controller->bIsPlayer * R_PLAYERONLY + bCanClimbLadders * R_LADDER); 
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Networking functions.
-----------------------------------------------------------------------------*/

FLOAT APawn::GetNetPriority( FVector& ViewPos, FVector& ViewDir, AActor* Recent, FLOAT Time, FLOAT Lag )
{
	guard(APawn::GetNetPriority);

	if
	(	Controller
	&&	Controller->bIsPlayer
	&&	Recent
	&&	Physics==PHYS_Walking
	&&	!Recent->bNetOwner
	&&  Recent->bHidden==bHidden )
	{
		FLOAT LocationError = ((Recent->Location+(Time+0.5f*Lag)*Recent->Velocity) - (Location+0.5f*Lag*Velocity)).SizeSquared();
		FVector Dir = Location - ViewPos;
		if ( (ViewDir | Dir) < 0.f )
			LocationError *= 0.2f;

		// Note: Lags and surges in position occur for other players because of
		// ServerMove/ClientAdjustPosition temporal wobble.
		Time = Time*0.5f + 2.f*LocationError / GroundSpeed;
	}
	return NetPriority * Time;
	unguard;
}

/* ZeroMovementAlpha()
Used by UpdateMovementAnimation() for zeroing out specified groups of channels
*/
void APawn::ZeroMovementAlpha(INT iStart, INT iEnd, FLOAT StepSize)
{
	guard(APawn::ZeroMovementAlpha);

	USkeletalMeshInstance *SkMeshInstance = Cast<USkeletalMeshInstance>(MeshInstance);
	UBOOL bZeroMovementAlpha = true;

	// blend alphas to zero 
	for ( INT i=iStart; i<iEnd; i++ )
		if ( SkMeshInstance->GetBlendAlpha(i) > 0.f )
		{
			bZeroMovementAlpha = false;
			SkMeshInstance->UpdateBlendAlpha(i,0.f,StepSize);
		}

	// if all are zero, stop movement animation
	if ( bZeroMovementAlpha )
		for ( INT i=iStart; i<iEnd; i++ )
		{
			SkMeshInstance->SetAnimRate(i, 0.f);
			SkMeshInstance->SetAnimFrame(i, 0.f);
		}
	unguard;
}

#if 1 //NEW (mdf) 2199 code
void APawn::UpdateMovementAnimation( FLOAT DeltaSeconds )
{
	guard(APawn::UpdateMovementAnimation);

	//debugf( L"%s APawn::UpdateMovementAnimation  Health: %d  AnimAction: %s", GetFullName(), Health, *AnimAction );
	
    if ( Level->NetMode == NM_DedicatedServer )
		return;

    if( bPlayedDeath )
        return;

    if (Level->TimeSeconds - LastRenderTime > 1.0)
    {
        FootRot = Rotation.Yaw;
        FootTurning = false;
        FootStill = false;
        return;
    }

	Mesh->MeshGetInstance(this);
    if (MeshInstance == NULL)
        return;

    BaseEyeHeight = ((APawn *)(GetClass()->GetDefaultActor()))->BaseEyeHeight;

    if ( bIsIdle && Physics != OldPhysics )
    {
        bWaitForAnim = false;
    }

    if ( !bWaitForAnim )
    {
        if ( Physics == PHYS_Swimming )
	    {
            BaseEyeHeight *= 0.7f;
            UpdateSwimming();
	    }
        else if ( Physics == PHYS_Falling || Physics == PHYS_Flying )
        {
            BaseEyeHeight *= 0.7f;
            UpdateInAir();
        }
        else if ( Physics == PHYS_Walking )
        {
            UpdateOnGround();
        }
    }
    
    if ( Physics != PHYS_Walking )
        bIsIdle = false;

    OldPhysics = Physics;
    OldVelocity = Velocity;

    if (bDoTorsoTwist)
        UpdateTwistLook( DeltaSeconds );

    unguard;
}

void APawn::UpdateSwimming( void )
{
	if( Health > 0 )
	{
		if ( (Velocity.X*Velocity.X + Velocity.Y*Velocity.Y) < 2500.0f )
			PlayAnim(0, IdleSwimAnim, 1.0f, 0.1f, true);
		else
			PlayAnim(0, SwimAnims[Get4WayDirection()], 1.0f, 0.1f, true);
	}
}

void APawn::UpdateInAir( void )
{
	FName NewAnim;
    bool bUp, bDodge;
    float DodgeSpeedThresh;
    int Dir;
    float XYVelocitySquared;

#if 1 //NEW (mdf) fix?    
	if( Health <= 0 )
		return;
#endif

	//HERES( "UpdateInAir" );
    XYVelocitySquared = (Velocity.X*Velocity.X)+(Velocity.Y*Velocity.Y);

    bDodge = false;
    if ( OldPhysics == PHYS_Walking )
    {
        DodgeSpeedThresh = ((GroundSpeed*DodgeSpeedFactor) + GroundSpeed) * 0.5f;
        if ( XYVelocitySquared > DodgeSpeedThresh*DodgeSpeedThresh )
        {
            bDodge = true;
        }
    }

    bUp = (Velocity.Z >= 0.0f);

    if (XYVelocitySquared >= 20000.0f)
    {
        Dir = Get4WayDirection();

        if (bDodge)
        {
            NewAnim = DodgeAnims[Dir];
            bWaitForAnim = true;
        }
        else if (bUp)
        {
            NewAnim = TakeoffAnims[Dir];
        }
        else
        {
			//HERES( "UpdateInAir -- select AirAnim" );
            NewAnim = AirAnims[Dir];
        }
    }
    else
    {
        if (bUp)
        {
            NewAnim = TakeoffStillAnim;
        }
        else
        {
            NewAnim = AirStillAnim;
        }
    }

	//debugf( L"UpdateInAir selected %s", *NewAnim );
	
	if ( NewAnim != MeshInstance->GetActiveAnimSequence(0) )
    {
		//debugf( L"UpdateInAir playing %s", *NewAnim );
    	PlayAnim(0, NewAnim, 1.0f, 0.1f, false);
    }
}

void APawn::UpdateOnGround( void )
{
#if 1 //NEW (mdf) fix? We don't "tear off" our carcasses  
	//debugf( L"UpdateOnGround:  Health: %d  bPlayedDeath: %d  AnimAction: %s  DeathAnim: %s", Health, bPlayedDeath, *AnimAction, *DeathAnim );
	if( Health <= 0 )
	{
		// hack
		if( DeathAnim != NAME_None )
		{			
			Mesh->MeshGetInstance(this);

			if( !bCanBeRevived )
			{
				// force last frame of death?
				// must be an easier way?
				HMeshAnim Anim = MeshInstance->GetAnimNamed( DeathAnim );
				FLOAT NumFrames = MeshInstance->AnimGetFrameCount( Anim );
				
				FLOAT AnimFrame = (NumFrames-1.0f)/NumFrames;
				MeshInstance->AnimForcePose( DeathAnim, AnimFrame, AnimFrame ); //wtf: 1.0f ==> get frame somewhere in middle of animation?
				MeshInstance->StopAnimating();
			}
			else if( MeshInstance && !MeshInstance->IsAnimating(0) )
			{
				// play a looping wounded animation while player can be revived or mercy killed
				FName PostDeathAnim;
				if( DeathAnim == TEXT("DeathB") )
					PostDeathAnim = TEXT("NotWellB");
				else if( DeathAnim == TEXT("DeathF") )
					PostDeathAnim = TEXT("NotWellF");
				else if( DeathAnim == TEXT("DeathL") )
					PostDeathAnim = TEXT("NotWellL");
				else if( DeathAnim == TEXT("DeathR") )
					PostDeathAnim = TEXT("NotWellR");
					
				PlayAnim( 0, PostDeathAnim, 1.0f, 0.1f, true );
			}
		}
		return;
	}
#endif

    // just landed
    if ( OldPhysics == PHYS_Falling || OldPhysics == PHYS_Flying )
    {
        PlayLand();
    }
    // standing still
    else if ( Velocity.SizeSquared() < 2500.0f /*&& Acceleration.SizeSquared() < 0.01f*/ )
    {
        if (!bIsIdle || FootTurning || bIsCrouched != bWasCrouched)
        {
            IdleTime = Level->TimeSeconds;
            PlayIdle();
        }
        bWasCrouched = bIsCrouched;
        bIsIdle = true;
    }
    // running
    else
    {
        if ( bIsIdle  )
            bWaitForAnim = false;

        PlayRunning();
        bIsIdle = false;
    }
}

void APawn::PlayIdle( void )
{
#if 1 //NEW (mdf) fix?    
	if( Health <= 0 )
		return;
#endif

    if (FootTurning)
    {
        if (TurnDir == 1)
        {
            if (bIsCrouched)
    		    PlayAnim(0, CrouchTurnRightAnim, 1.0f, 0.1f, true);
            else
    		    PlayAnim(0, TurnRightAnim, 1.0f, 0.1f, true);
        }
        else
        {
            if (bIsCrouched)
    		    PlayAnim(0, CrouchTurnLeftAnim, 1.0f, 0.1f, true);
            else
        	    PlayAnim(0, TurnLeftAnim, 1.0f, 0.1f, true);
        }
    }
    else
    {
        if (bIsCrouched)
        {
            PlayAnim(0, IdleCrouchAnim, 1.0f, 0.1f, true);
        }
        else
        {
            if (Level->TimeSeconds - IdleTime < 5.0f && IdleWeaponAnim != NAME_None)
            {
                PlayAnim(0, IdleWeaponAnim, 1.0f, 0.25f, true);
            }
            else
            {
	            PlayAnim(0, IdleRestAnim, 1.0f, 0.25f, true);
            }
        }
    }
}

void APawn::PlayRunning( void )
{
	FName NewAnim;
    int NewAnimDir;
    float AnimSpeed;

    NewAnimDir = Get4WayDirection();

    AnimSpeed = 1.1f * ((APawn *)(GetClass()->GetDefaultActor()))->GroundSpeed;
    if (bIsCrouched)
    {
        NewAnim = CrouchAnims[NewAnimDir];
        AnimSpeed *= WalkingPct;
    }
    else if (bIsWalking)
    {
        NewAnim = WalkAnims[NewAnimDir];
        AnimSpeed *= WalkingPct;
    }
    else
    {
        NewAnim = MovementAnims[NewAnimDir];
    }

    PlayAnim(0, NewAnim, Velocity.Size() / AnimSpeed, 0.1f, true);
    OldAnimDir = NewAnimDir;
}

void APawn::PlayLand( void )
{
    if (!bIsCrouched && Health > 0 )
    {
        PlayAnim(0, LandAnims[Get4WayDirection()], 1.0f, 0.1f, false);
        bWaitForAnim = true;
    }
}


void APawn::execGet4WayDirection( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGet4WayDirection);
	P_FINISH;
    *(int*)Result = Get4WayDirection();
    unguardSlow;
}

int APawn::Get4WayDirection( void )
{
    float forward, right;
    FVector V;
    int dir;

    V = Velocity;
    V.Z = 0.0f;

    if ( V.IsNearlyZero() )
		return 0;

	FCoords Coords = GMath.UnitCoords / Rotation;
    V.Normalize();
    forward = Coords.XAxis | V;
    if (forward > 0.82f) // 55 degrees
        dir = 0;
    else if (forward < -0.82f)
        dir = 1;
    else
    {
        right = Coords.YAxis | V;
        if (right > 0.0f)
            dir = 3;
        else
            dir = 2;
    }
	return dir;
}


// ----- torso twisting ----- //
void APawn::execSetTwistLook( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(twist);
	P_GET_INT(look);
	P_FINISH;
    SetTwistLook(twist, look);
}

void APawn::SetTwistLook( int twist, int look )
{
    FRotator r;

    if (!bDoTorsoTwist)
        return;

    r.Yaw = -twist;
    r.Pitch = 0;
    r.Roll = 0;
    MeshInstance->SetBoneRotation(RootBone, r, 0, 1.0f);

    r.Yaw = -twist / 3;
    r.Pitch = 0;
    r.Roll = look / 4;
    ((USkeletalMeshInstance*)MeshInstance)->SetBoneDirection(HeadBone, r, FVector(0.0f,0.0f,0.0f), 1.0f, 0);
    ((USkeletalMeshInstance*)MeshInstance)->SetBoneDirection(SpineBone1, r, FVector(0.0f,0.0f,0.0f), 1.0f, 0);
    ((USkeletalMeshInstance*)MeshInstance)->SetBoneDirection(SpineBone2, r, FVector(0.0f,0.0f,0.0f), 1.0f, 0);
}

void APawn::UpdateTwistLook( float DeltaTime )
{
    int t, look;

    if (Level->TimeSeconds - LastRenderTime > 1.0)
    {
        FootRot = Rotation.Yaw;
        FootTurning = false;
        FootStill = false;
    }
    else
    {
        t = (Rotation.Yaw - FootRot) & 65535;
        if (t > 32768) t -= 65536;

        
        if ((Velocity.X * Velocity.X + Velocity.Y * Velocity.Y) < 1000 && Physics == PHYS_Walking)
        {
            if (!FootStill)
            {
                FootStill = true;
                FootRot = Rotation.Yaw;
				t = 0;
            }
        }
        else
        {
            if (FootStill)
            {
                FootStill = false;
                FootTurning = true;
            }
        }

        if (FootTurning)
        {
            if (t > 12000)
            {
                FootRot = Rotation.Yaw - 12000;
                t = 12000;
            }
            else if (t > 2048)
            {
                FootRot += 16384*DeltaTime;
            }
            else if (t < -12000)
            {
                FootRot = Rotation.Yaw + 12000;
                t = -12000;
            }
            else if (t < -2048)
            {
                FootRot -= 16384*DeltaTime;
            }
            else
            {
                if (!FootStill)
                    t = 0;
                FootTurning = false;
            }
            FootRot = FootRot & 65535;
        }
        else if (FootStill)
        {
            if (t > 10923)
            {
                TurnDir = 1;
                FootTurning = true;
            }
            else if (t < -10923)
            {
                TurnDir = -1;
                FootTurning = true;
            }
        }
        else
        {
            t = 0;
        }

        look = ViewPitch * 256;     
        if (look > 32768) look -= 65536;

        SetTwistLook(t, look);
    }
}
#else
/* UpdateMovementAnimation()
When bPhysicsAnimUpdate is true, movement animations are smoothly blended based on the 
current physics status of the pawn.  Scripts set up an array of movement animations to 
use based on the current movement mode.  This code also executes for pawns on network clients,
resulting in good quality animations on the client without any animation property replication.
*/ 
/*
#define RIGHTTURNCHANNEL 2
#define LEFTTURNCHANNEL 3
#define FIRSTMOVEMENTCHANNEL 4

void APawn::UpdateMovementAnimation(FLOAT DeltaSeconds)
{
	guard(APawn::UpdateMovementAnimation);

	// no movement animation blending on dedicated server
	if ( Level->NetMode == NM_DedicatedServer )
		return;

	Mesh->MeshGetInstance(this);
	USkeletalMeshInstance *SkMeshInstance = Cast<USkeletalMeshInstance>(MeshInstance);
	if ( !SkMeshInstance )
		return;

	if ( !bInitializeAnimation )
	{
		// get movement animation channels playing anims with zero alpha
		bInitializeAnimation = true;
		PlayAnim(RIGHTTURNCHANNEL,TurnRightAnim,1.f,0.1f,true);
		PlayAnim(LEFTTURNCHANNEL,TurnLeftAnim,1.f,0.1f,true);
		for ( INT i=FIRSTMOVEMENTCHANNEL; i<FIRSTMOVEMENTCHANNEL+4; i++ )
			PlayAnim(i,MovementAnims[i-FIRSTMOVEMENTCHANNEL],1.f,0.1f,true); 
		for ( INT i=RIGHTTURNCHANNEL; i<FIRSTMOVEMENTCHANNEL+8; i++ )
		{
			SkMeshInstance->EnableChannelNotify(i,false);
			SkMeshInstance->SetBlendAlpha(i,0.f);
		}
		SkMeshInstance->EnableChannelNotify(FIRSTMOVEMENTCHANNEL,true);
	}
	// fixme - if not recently rendered, skip blending (except for zeroing out) secondary anims?
	FLOAT StepSize = DeltaSeconds/BlendChangeTime;

	if ( bTearOff )
	{
		// assume dead if bTearOff
		if ( !bPlayedDeath )
			eventPlayDying(HitDamageType,TakeHitLocation);
		ZeroMovementAlpha(RIGHTTURNCHANNEL,FIRSTMOVEMENTCHANNEL+8,StepSize);
		return;
	}

	// check for physics status transitions
	if ( (bWasCrouched != bIsCrouched) || (bWasWalking != bIsWalking) )
	{
		eventChangeAnimation();
		bWasCrouched = bIsCrouched;
		bWasWalking = bIsWalking;
	}
	if ( (OldPhysics==PHYS_Falling) != (Physics==PHYS_Falling) )
	{
		if ( Physics == PHYS_Walking )
			eventPlayLandingAnimation(Velocity.Z);
		else if ( bWasOnGround && (Velocity.Z > 0) )
			eventPlayJump();
		else
			eventPlayFalling();
	}
	if ( (Physics != OldPhysics) && (Physics != PHYS_Falling) )
		eventChangeAnimation();
	
	OldPhysics = Physics;
	bWasOnGround = (Physics == PHYS_Walking);

	if ( (Physics == PHYS_Falling) || (MovementBlendStartTime > Level->TimeSeconds) )
		ZeroMovementAlpha(RIGHTTURNCHANNEL,FIRSTMOVEMENTCHANNEL+8,StepSize);
	else if ( Acceleration.IsZero() )
	{
		if ( !OldAcceleration.IsZero() && (Physics != PHYS_Falling) )
			eventChangeAnimation();

		ZeroMovementAlpha(FIRSTMOVEMENTCHANNEL, FIRSTMOVEMENTCHANNEL+8,StepSize);
		// play turning in place animation if needed
		Rotation.Yaw = Rotation.Yaw & 65535;
		if ( Rotation.Yaw - OldRotYaw > 32768 )
			OldRotYaw += 65536;
		else if ( OldRotYaw - Rotation.Yaw > 32768 )
			OldRotYaw -= 65536;

		// FIXME - make turning in place work for net play
		if ( (Physics == PHYS_Walking) && (Role > ROLE_SimulatedProxy) ) 
		{
			FLOAT TurnRate = FLOAT(Rotation.Yaw - OldRotYaw)/DeltaSeconds;
			UBOOL bTurningRight = ( TurnRate > 1000.f );
			UBOOL bTurningLeft = ( TurnRate < -1000.f );

			if ( bTurningRight )
			{
				SkMeshInstance->SetAnimSequence(RIGHTTURNCHANNEL,TurnRightAnim);
				SkMeshInstance->UpdateBlendAlpha(RIGHTTURNCHANNEL,1.f,StepSize);
				SkMeshInstance->SetAnimRate(RIGHTTURNCHANNEL,Abs(TurnRate)/16384.f);
			}
			else
				ZeroMovementAlpha(RIGHTTURNCHANNEL,RIGHTTURNCHANNEL+1,StepSize);

			if ( bTurningLeft )
			{
				SkMeshInstance->SetAnimSequence(LEFTTURNCHANNEL,TurnLeftAnim);
				SkMeshInstance->UpdateBlendAlpha(LEFTTURNCHANNEL,1.f,StepSize);
				SkMeshInstance->SetAnimRate(LEFTTURNCHANNEL,Abs(TurnRate)/16384.f);
			}
			else
				ZeroMovementAlpha(LEFTTURNCHANNEL,LEFTTURNCHANNEL+1,StepSize);
		}
		if ( OldRotYaw != Rotation.Yaw )
			OldRotYaw = Rotation.Yaw;
	}
	else
	{
		// zero turning in place
		ZeroMovementAlpha(RIGHTTURNCHANNEL,LEFTTURNCHANNEL+1,StepSize);

		FVector LookDir = Rotation.Vector();
		if ( Physics == PHYS_Walking )
			LookDir.Z = 0.f;
		else if ( (Physics == PHYS_Ladder) && OnLadder )
			LookDir = OnLadder->ClimbDir;

		// determine desired alphas for 4 movement directions
		LookDir = LookDir.SafeNormal();
		FVector AccelDir = Acceleration.SafeNormal();
		FVector LeftDir = LookDir ^ FVector(0.f,0.f,1.f);
		LeftDir = LeftDir.SafeNormal();
		FLOAT ForwardPct = (LookDir | AccelDir);
		FLOAT LeftPct = (LeftDir | AccelDir);
		FLOAT BlendedMovementAnimRate = 1.f;
		FLOAT ForwardPctSq = ForwardPct * ForwardPct;
		FLOAT LeftPctSq = LeftPct * LeftPct;

		if ( ForwardPctSq > 0.f )
		{
			ForwardPctSq = 1.f;
			if ( LeftPctSq > 0.1f )
				LeftPctSq = (ForwardPct > 0.f) 
							? LeftPctSq + ForwardStrafeBias * (1.f - LeftPctSq)
							: LeftPctSq + BackwardStrafeBias * (1.f - LeftPctSq);
		}

		// FIXME - temp hack while playing with movement rates
		BlendedMovementAnimRate = GetMaxSpeed()/BaseMovementRate;

		// force all animations to play at the dominant frame rate
		FLOAT BlendedFrameRate = 0.f;
		INT BlendCount = 0;
		if ( ForwardPct != 0.f )
		{
			BlendCount++;
			BlendedFrameRate = (ForwardPct > 0.f) 
								? SkMeshInstance->GetAnimRateOnChannel(FIRSTMOVEMENTCHANNEL)
								: SkMeshInstance->GetAnimRateOnChannel(FIRSTMOVEMENTCHANNEL+2);
		}
		if ( LeftPct != 0.f )
		{
			BlendCount++;
			BlendedFrameRate += (LeftPct > 0.f) 
								? SkMeshInstance->GetAnimRateOnChannel(FIRSTMOVEMENTCHANNEL+1)
								: SkMeshInstance->GetAnimRateOnChannel(FIRSTMOVEMENTCHANNEL+3);
		}
		BlendedMovementAnimRate *= ((BlendCount == 2) ? BlendedFrameRate * 0.5f : BlendedFrameRate);

		// blend between changes in animation set
		// note that this won't look perfect for multiple quick changes
		for ( INT i=FIRSTMOVEMENTCHANNEL; i<FIRSTMOVEMENTCHANNEL+4; i++ )
			if ( MovementAnims[i-FIRSTMOVEMENTCHANNEL] != SkMeshInstance->GetAnimSequence(i) )
			{
				if ( SkMeshInstance->GetBlendAlpha(i+4) < SkMeshInstance->GetBlendAlpha(i) )
					SkMeshInstance->CopyAnimation(i,i+4);
				SkMeshInstance->SetAnimSequence(i,MovementAnims[i-FIRSTMOVEMENTCHANNEL]);
			}

		// update blending
		if ( ForwardPct >= 0.f )
		{
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL,ForwardPctSq,StepSize);
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL+1,0.f,StepSize);
		}
		else
		{
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL,0.f,StepSize);
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL+1,ForwardPctSq,StepSize);
		}
		if ( LeftPct >= 0.f )
		{
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL+2,LeftPctSq,StepSize);
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL+3,0.f,StepSize);
		}
		else
		{
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL+2,0.f,StepSize);
			SkMeshInstance->UpdateBlendAlpha(FIRSTMOVEMENTCHANNEL+3,LeftPctSq,StepSize);
		}
		for ( INT i=FIRSTMOVEMENTCHANNEL+4; i<FIRSTMOVEMENTCHANNEL+8; i++ )
			SkMeshInstance->UpdateBlendAlpha(i,0.f,StepSize);

		// synchronize animation frame and rate
		for ( INT i=FIRSTMOVEMENTCHANNEL; i<FIRSTMOVEMENTCHANNEL+8; i++ )
			SkMeshInstance->SetAnimRate(i,BlendedMovementAnimRate/SkMeshInstance->GetAnimRateOnChannel(i));

		// FIXME - update animframes to keep synchronized - is this needed?
		FLOAT SynchFrame = SkMeshInstance->GetAnimFrame(FIRSTMOVEMENTCHANNEL);
		for ( INT i=FIRSTMOVEMENTCHANNEL+1; i<FIRSTMOVEMENTCHANNEL+8; i++ )
			SkMeshInstance->SetAnimFrame(i,SynchFrame);
	}
	unguard;
}
*/
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

