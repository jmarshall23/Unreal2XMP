#include "U2.h"
#include "FLineManager.h"

//!! fix excessive use of Cast<AU2PlayerController>(Controller) (see also TRYINGTOMANTLE macro)

//-----------------------------------------------------------------------------
// mantling

const ETraceFlags MantleTraceFlags			= TRACE_ProjTargets;	// mantle onto anything which blocks actors for now
const FLOAT TooFastMantleLockOutTime		= 0.5f;					// re-mantled locked out for this long after mantle fails due to speed
const FLOAT StuckMantleLockOutTime			= 1.0f;					// re-mantled locked out for this long after mantle fails due to stuck
const FLOAT LandedReachUpFudgeFactor		= 8.0f;					// used to handle mantling immediately upon landing
const FLOAT MantleDestZFudgeFactor			= 5.0f;					// make CC go a bit above height of ledge before ending mantling
const FLOAT TraceLenFudgeFactor				= 5.0f;					// trace a little further than strictly necessary (5.0 should be fine)

//const FLOAT TraceStartFudgeFactor			= 0.01f;				// mdf-tbr:
static INT MantleKey = -1;											// mdf-tbr -- debugging

#define TRYINGTOMANTLE (Cast<AU2PlayerController>(Controller) && Cast<AU2PlayerController>(Controller)->TryingToMantle())

//-----------------------------------------------------------------------------
// mantling support
//-----------------------------------------------------------------------------

void AU2Pawn::MantleBegin( const FCheckResult& HitLedge, UBOOL bStanding )
{
	MantleDestination = HitLedge.Location;
	MantleDestination.Z += CollisionHeight + MantleDestZFudgeFactor;

	// determine mantle height for synching animation (Note: CC's generally float above ground by ~4.6 units?)
	MantleHeight = (  bStanding ? HitLedge.Location.Z - (Location.Z-CollisionHeight) : 9999.0f );

	if( MantleDebugLevel >= 2 )
		debugf( L"%0.3f MANTLEDEBUG: Mantle begin (bStanding: %d) (hit: %s ledge height: %f units (%f, HitFloor Actor: %s)", TIMESECONDS, bStanding, HitLedge.Actor->GetName(), MantleHeight, HitLedge.Location.Z );

	bMantling = true;
	setPhysics( PHYS_Flying );
	Cast<AU2PlayerController>(Controller)->eventMantleBegin();
}

//-----------------------------------------------------------------------------

void AU2Pawn::MantleEnd()
{
	if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
		debugf( L"%0.3f MANTLEDEBUG: MantleEnd", TIMESECONDS );

	bMantling = false;
	if( Physics == PHYS_Flying )
	{
		if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
			debugf( L"%0.3f MANTLEDEBUG: MantleEnd setting physics to PHYS_Falling", TIMESECONDS );
		setPhysics( PHYS_Falling );
	}
	if( Cast<AU2PlayerController>(Controller) )
		Cast<AU2PlayerController>(Controller)->eventMantleEnd();
}

//-----------------------------------------------------------------------------

static void ShowTrace( APawn* P, const FVector& TraceStart, const FVector& TraceEnd, UBOOL bMantling, UBOOL bStanding )
{
	FPlane Color;

	if( bMantling )
	{
	 	Color = bStanding ? COLOR_BLUE : COLOR_GREEN;
	}
	else
	{
	 	Color = bStanding ? COLOR_WHITE : COLOR_RED;
	}

 	FLineManager::GetInstance()->AddArrow( MantleKey, TraceStart, TraceEnd, Color, Color );
}

//-----------------------------------------------------------------------------

UBOOL AU2Pawn::ActorFits( const FVector& Destination )
{
	UBOOL bFits = false;
	
	FVector SavedLocation = Location;

	// try to place the actor at the mantle destination (1=don't actually encroach actors, 0=check for fit)
	if( GetLevel()->FarMoveActor( this, Destination, 1, 0 ) )
	{
		// mdf-tbd: also check that actor will be able to move vertically up?
		bFits = true;
	}
	else
	{
		if( MantleDebugLevel >= 3 && TRYINGTOMANTLE )
			debugf( L"%0.3f MANTLEDEBUG: Couldn't set location to mantle location", TIMESECONDS );
	}
					
	// restore saved location (1=don't actually encroach actors, 0=don't check for fit)
	if( !GetLevel()->FarMoveActor( this, SavedLocation, 1, 1 ) )
		debugf( NAME_Warning, L"%0.3f MANTLEDEBUG: AU2Pawn::ActorFits -- couldn't restore pre-mantle test location!", TIMESECONDS );
	
	return bFits;
}

//-----------------------------------------------------------------------------

void AU2Pawn::GetBaseHandsInfo( const FVector& BaseLocation, const FRotator& BaseRotation, FVector& BaseHandsLocation, FVector& BaseHandsVector )
{
	BaseHandsLocation = BaseLocation;
	BaseHandsLocation.Z += CollisionHeight + MantleHandsUpwardReach;

	BaseHandsVector = BaseRotation.Vector();
	BaseHandsVector.Z = 0;
	BaseHandsVector.Normalize();
}

//-----------------------------------------------------------------------------

UBOOL AU2Pawn::FindMantleSpot( FCheckResult& Hit, const FVector& Start, const FVector& End )
{
	GetLevel()->SingleLineCheck( Hit, 0, End, Start, MantleTraceFlags );

	if( MantleDebugLevel >= 3 && TRYINGTOMANTLE )
		debugf( L"%0.3f MANTLEDEBUG: Hit Actor: %s (Normal.Z: %f)", TIMESECONDS, Hit.Actor ? Hit.Actor->GetName() : L"None", Hit.Actor ? Hit.Normal.Z : 0.0 );

	return( Hit.Actor != NULL &&
			Hit.Actor->bCanMantle &&
			Hit.Actor != this &&
			(Hit.Actor->IsA(ALevelInfo::StaticClass()) || 
			 Hit.Actor->IsA(ATerrainInfo::StaticClass()) || 
			 Hit.Actor->bBlockActors || Hit.Actor->bBlockPlayers) &&
			Hit.Normal.Z >= MantleMinHitNormalDeltaZ );
}

//-----------------------------------------------------------------------------
// Standing -- is there a ledge within reach?

//mdf-tbi: UBOOL AU2Pawn::TryToReachLedge( const FVector& HandsLocation, BaseHandsLocation )
UBOOL AU2Pawn::TryToReachLedge( const FVector& HandsLocation )
{
	UBOOL bFoundLedge = false;
	FLOAT TraceLen = -32.0f;

	if( MantleDebugLevel >= 3 && TRYINGTOMANTLE )
		debugf( L"%0.3f MANTLEDEBUG: HandsLocation: %f,%f,%f", TIMESECONDS, HandsLocation.X, HandsLocation.Y, HandsLocation.Z );

	// make sure start point not in geometry (end point can be in geometry)
	if( GetLevel()->Model->PointRegion( GetLevel()->GetLevelInfo(), HandsLocation ).ZoneNumber != 0 )
	{
		// start point is in the level

		// trace from hands position down but not far enough to hit ledge that is jumpable from base of CC
		TraceLen = 2*CollisionHeight + MantleHandsUpwardReach - UCONST_MAXJUMPHEIGHT + TraceLenFudgeFactor;
   
   		// see if we hit an appropriate ledge
   		FCheckResult Hit(1.0f);
		if( FindMantleSpot( Hit, HandsLocation, HandsLocation - TraceLen * FVector(0,0,1) ) )
   		{
			if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
				debugf( L"%0.3f MANTLEDEBUG: Hit Actor: %s (Normal.Z: %f)", TIMESECONDS, Hit.Actor ? Hit.Actor->GetName() : L"None", Hit.Actor ? Hit.Normal.Z : 0.0 );

   			// make sure player can fit at that location
   			if( ActorFits( Hit.Location+CollisionHeight*FVector(0,0,1) ) )
   			{
				if( Velocity.Z < 0 && Velocity.Size() > MaxFallingMantleSpeed )
				{
					// too fast but let script code know mantle almost happened
					Cast<AU2PlayerController>(Controller)->eventMantleBegin();
			
					// slow pawn down a bit
					Velocity *= 0.67f;
			
					// lock out trying to mantle again for a bit
					NextCheckMantleTime = TIMESECONDS + TooFastMantleLockOutTime;

					bFoundLedge = true; // so doesn't try at alternate hand position
				}
				else
				{
					// start the mantle
	   				MantleBegin( Hit, true );
	   				bFoundLedge	= true;
				}
   			}
   		}
   	}
   	else
   	{
		if( MantleDebugLevel >= 3 && TRYINGTOMANTLE )
			debugf( L"%0.3f MANTLEDEBUG: HandsLocation not in level or not in same zone as player: %f,%f,%f", TIMESECONDS, HandsLocation.X, HandsLocation.Y, HandsLocation.Z );
   	}

	if( MantleDebugLevel >= 1 && TRYINGTOMANTLE )
		ShowTrace( this, HandsLocation, HandsLocation - TraceLen * FVector(0,0,1), bFoundLedge, true );
	
	return bFoundLedge;
}

//-----------------------------------------------------------------------------
// Falling (up or down) -- is there a ledge within reach?

UBOOL AU2Pawn::TryToCatchLedge( const FVector& TraceStart, const FVector& TraceEnd )
{
	UBOOL bFoundLedge = false;

	// make sure start point not in geometry (end point can be in geometry)
	if( GetLevel()->Model->PointRegion( GetLevel()->GetLevelInfo(), TraceStart ).ZoneNumber != 0 )
	{
		// see if we hit level or mover (mdf-tbd)
		FCheckResult Hit(1.0f);
		GetLevel()->SingleLineCheck( Hit, 0, TraceEnd, TraceStart, MantleTraceFlags );
		
		if( FindMantleSpot( Hit, TraceStart, TraceEnd ) )
		{
	   		// make sure player can fit at that location
	   		if( ActorFits( Hit.Location + CollisionHeight*FVector(0,0,1) ) )
	   		{	
	   			FLOAT VSize = Velocity.Size();
	   			
				if( (Velocity.Z  < 0.0f && VSize > MaxFallingMantleSpeed) ||
					(Velocity.Z >= 0.0f && VSize > MaxRisingMantleSpeed) )
				{
					if( MantleDebugLevel >= 2 )
						debugf( L"%0.3f MANTLEDEBUG: found ledge but player %s too fast!", TIMESECONDS, Velocity.Z < 0.0f ? L"falling" : L"rising" );
						
					if( Velocity.Z < 0.0f )
					{
						// too fast but let script code know mantle almost happened
						Cast<AU2PlayerController>(Controller)->eventMantleBegin();
			
						// slow pawn down a bit
						Velocity *= 0.67f;
						
					}
					
					// lock out trying to mantle again for a bit
					NextCheckMantleTime = TIMESECONDS + TooFastMantleLockOutTime;

					bFoundLedge = true; // so doesn't try at alternate hand position
				}
				else
				{
		   			// "roll" actor position back (the longer the trace, the more we roll back)
		   			FLOAT DistanceAlongTrace = (TraceEnd - Hit.Location).Size();
		
					// mdf-tbd: should we interpolate rotation (between old and new values)?
		   			FVector TraceVector = Rotation.Vector();
					TraceVector.Z = 0;
		   			TraceVector.Normalize();
		
					// move out along rotation (not along trace)
		   			FVector NewLocation = Location - DistanceAlongTrace*TraceVector;
					NewLocation.Z = Hit.Location.Z - (CollisionHeight + MantleHandsUpwardReach);
		
					// try to place an appropriate distance away from the mantle destination (1=don't actually encroach actors, 0=check for fit)
					//ADDACTOR( this, COLOR_GREY );
					
		   			if( GetLevel()->FarMoveActor( this, NewLocation, 1, 0 ) )
		   			{
	   					// try to cozy the actor up to the ledge as much as possible (otherwise
	   					// with low FPS CC will be way out from the ledge)

	   					FCheckResult Hit2(1.0f);
	   					
	   					// 2003.06.23 (mdf) first check if we need to move actor up so shift towards ledge will actually hit it
	   					if( (Location.Z + CollisionHeight) < Hit.Location.Z )
	   					{
		   					FVector ShiftVector(0.f,0.f,0.f);
		   					ShiftVector.Z = Hit.Location.Z - (Location.Z + CollisionHeight);
		   					GetLevel()->MoveActor( this, ShiftVector, Rotation, Hit2, 0 );
						}
						
						// now shift actor towards ledge	   					
	   					FVector ShiftVector = Hit.Location - Location;
	   					ShiftVector.Z = 0;
	   					ShiftVector.Normalize();
	   	
	   					// try to slide actor up to ledge -- (0=send touch notifications)
	   					GetLevel()->MoveActor( this, ShiftVector*128.0f, Rotation, Hit2, 0 );
	   	
	   	   				MantleBegin( Hit, false );
	   	   				bFoundLedge	= true;
		   			}
					//ADDACTOR( this, COLOR_PINK );
		   		}
			}
		}
	}

	if( MantleDebugLevel >= 1 && TRYINGTOMANTLE )
		ShowTrace( this, TraceStart, TraceEnd, bFoundLedge, false );
	
	return bFoundLedge;
}

//-----------------------------------------------------------------------------

void AU2Pawn::CheckForLedge( UBOOL bStanding, const FVector& OldLocation, const FRotator& OldRotation )
{
	if( bStanding )
	{
		if( MaxRisingMantleSpeed >= 0.0f )
		{
			// actor is standing -- can he reach up & mantle?

			// trace down from location in front and above of player's CC
			FVector BaseHandsLocation, BaseHandsVector;

			// special case: if we just landed (velocity.Z is still -ve) physics tends to
			// overshoot final resting Z location somewhat so let player reach a bit higher
			// than usual.
			if( Velocity.Z < 0 )
				MantleHandsUpwardReach += LandedReachUpFudgeFactor;

			GetBaseHandsInfo( Location, Rotation, BaseHandsLocation, BaseHandsVector );

			FVector MinStartLocation = BaseHandsLocation + (CollisionRadius + MantleHandsForwardReachMin)*BaseHandsVector;

			// mdf-tbd: not worth the overhead -- should happen very, very rarely (only with fairly thin walls)?
			// mdf-tbd: do the point check first to see if start in geometry (fast)
			// make sure min location not obstructed from base location (so can't grab ledge through a wall)
			FCheckResult Hit(1.f);
			GetLevel()->SingleLineCheck( Hit, this, MinStartLocation, BaseHandsLocation, TRACE_World|TRACE_StopAtFirstHit );

			if( !Hit.Actor )
			{
				// try at min forward reach
				if( !TryToReachLedge( MinStartLocation ) )
				{
					// try at max forward reach
					TryToReachLedge( BaseHandsLocation + (CollisionRadius + MantleHandsForwardReachMax)*BaseHandsVector );
				}
			}

			if( Velocity.Z < 0 )
				MantleHandsUpwardReach -= LandedReachUpFudgeFactor;
		}
	}
	else
	{
		FVector OldBaseHandsLocation, OldBaseHandsVector;
		FVector NewBaseHandsLocation, NewBaseHandsVector;

		GetBaseHandsInfo( OldLocation, OldRotation, OldBaseHandsLocation, OldBaseHandsVector );
		GetBaseHandsInfo( Location, Rotation, NewBaseHandsLocation, NewBaseHandsVector );

		if( Velocity.Z>=0 )
		{
			// player is rising -- swap trace points
			FVector Temp( NewBaseHandsLocation );
			NewBaseHandsLocation = OldBaseHandsLocation;
			OldBaseHandsLocation = Temp;
		}

		FVector HandsVector = (CollisionRadius + MantleHandsForwardReachMin)*OldBaseHandsVector;

		FVector OldMinStartLocation = OldBaseHandsLocation + HandsVector;

		// mdf-tbd: not worth the overhead -- should only happen rarely (only with fairly thin walls)?
		// mdf-tbd: do the point check first to see if start in geometry (fast)
		// make sure old min location not obstructed from old base location (so can't grab ledge through a wall)
		FCheckResult Hit(1.f);
		GetLevel()->SingleLineCheck( Hit, this, OldMinStartLocation, OldBaseHandsLocation, TRACE_World|TRACE_StopAtFirstHit );

		if( !Hit.Actor )
		{
			// try at min forward reach
			if( !TryToCatchLedge( OldMinStartLocation, NewBaseHandsLocation + HandsVector ) )
			{
				// try at max forward reach
				HandsVector = (CollisionRadius + MantleHandsForwardReachMax)*OldBaseHandsVector;
				TryToCatchLedge( OldBaseHandsLocation + HandsVector, NewBaseHandsLocation + HandsVector );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Caller can check bMantling to determine if this results in mantling.

void AU2Pawn::MantleCheck( UBOOL bStanding, const FVector& OldLocation, const FRotator& OldRotation )
{
	guard(AU2Pawn::MantleCheck);

	if( MantleDebugLevel >= 1 && TRYINGTOMANTLE )
	{
		if( MantleKey == -1 )
			MantleKey = FLineManager::GetInstance()->AllocateLineList();
	}

	// make sure mantling not blocked out
	if( TIMESECONDS >= NextCheckMantleTime )
	{
		// mdf-tbd: (waterzone stuff) -- could possibly use mantling as a better way to get out of water
		// mdf-tbd: currently you can dodge-mantle (e.g. flip forward / dodge left into a mantle) any reason to disable this?

		if( TRYINGTOMANTLE &&
			(bStanding || (PhysicsVolume && !PhysicsVolume->bWaterVolume) ) )	// standing or not in a water zone
		{
			// see if there is an appropriate ledge in front of the player
			if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
				debugf( L"%0.3f MANTLEDEBUG: checking for ledge!", TIMESECONDS );
			CheckForLedge( bStanding, OldLocation, OldRotation );
		}
	}

	unguard;
}

//-----------------------------------------------------------------------------

void AU2Pawn::setPhysics( BYTE NewPhysics, AActor *NewFloor, FVector NewFloorV )
{
	BYTE OldPhysics	= Physics;

	if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
		debugf( L"%0.3f MANTLEDEBUG: setPhysics -- OldPhysics: %d  NewPhysics: %d  Velocity.Z: %0.3f Location.Z: %0.3f", TIMESECONDS, OldPhysics, NewPhysics, Velocity.Z, Location.Z );

	Super::setPhysics( NewPhysics, NewFloor );

	if( Health > 0 )
	{
		if( bMantling && NewPhysics != PHYS_Flying )
		{
			// abort mantling since physics no longer appropriate
			if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
				debugf( L"%0.3f MANTLEDEBUG: mantle end because physics no longer phys_flying", TIMESECONDS);
			MantleEnd();
		}

		if( OldPhysics == PHYS_Walking && NewPhysics == PHYS_Falling && Velocity.Z>0 )
		{
			// jumping -- check for mantling situation
			if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
				debugf( L"%0.3f MANTLEDEBUG: doing jumping mantle check Location.Z=%0.3f", TIMESECONDS, Location.Z );
			MantleCheck( true );
		}

		else if( OldPhysics == PHYS_Falling && NewPhysics == PHYS_Walking && Velocity.Z<0 )
		{
			// NOTE: Pawn's Z location is generally somewhat lower on landing vs standing
			if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
				debugf( L"%0.3f MANTLEDEBUG: doing landed mantle check Location.Z=%0.3f", TIMESECONDS, Location.Z );
			MantleCheck( true );
		}
	}
}

//-----------------------------------------------------------------------------

void AU2Pawn::physFlying( FLOAT deltaTime, INT Iterations )
{		
	if( bMantling )
	{
		if( !TRYINGTOMANTLE )
		{
			// player no longer trying to mantle
			if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
				debugf( L"%0.3f MANTLEDEBUG: mantle end because player no longer trying to mantle", TIMESECONDS );
		 	MantleEnd();
		}
		else
		{
			FVector OldLocation = Location;
	
    		Super::physFlying( deltaTime, Iterations );
	
			// see if vertical limit reached or player stuck
			UBOOL bEndMantle = false;
			if( Location.Z >= MantleDestination.Z && (Location - MantleDestination).Size2D() < 5.0f )
			{
				bEndMantle = true;
				if( MantleDebugLevel >= 2 )
					debugf( L"%0.3f MANTLEDEBUG: mantle end because destination reached", TIMESECONDS );
			}
			else if( (Location.Z - MantleDestination.Z) > CollisionRadius )
			{
				bEndMantle = true;
				if( MantleDebugLevel >= 2 )
					debugf( L"%0.3f MANTLEDEBUG: mantle end because got too high!", TIMESECONDS );
			}
			if( (Location - OldLocation).Size() < 0.001f )
			{
				bEndMantle = true;
				if( MantleDebugLevel >= 2 )
					debugf( L"%0.3f MANTLEDEBUG: mantle end because stuck", TIMESECONDS );
				if( MantleDebugLevel >= 3 )
					ADDACTOR( this,  COLOR_RED );
			}
			
			if( bEndMantle )
			{
				// lock out trying to mantle again for a bit
				NextCheckMantleTime = TIMESECONDS + StuckMantleLockOutTime;
				MantleEnd();
			}
		}
	}
	else
		Super::physFlying( deltaTime, Iterations );
}

//-----------------------------------------------------------------------------

void AU2Pawn::physFalling( FLOAT deltaTime, INT Iterations )
{
	guard(AU2Pawn::physFalling);

	FVector  OldLocation = Location;
	FRotator OldRotation = Rotation;

	//ADDACTOR( this, COLOR_MAGENTA );

	if( bDoubleJumping )
	{
		if( JumpJetDuration <= 0.1 )
		{
			Velocity.Z += JumpJetZ;
			bDoubleJumping = false;
		}
		else
		{
			if( Level->TimeSeconds - DoubleJumpStartSecs < JumpJetDuration )
			{
				//==================== For taking Owner.Mass into account =============
				// the 0.001 is to convert the mass to a reasonable number, 1- inverts it
				//	Velocity.Z += (Thrust * DeltaTime)*(1-(Owner.Mass*0.001));
				Velocity.Z += (JumpJetZ * deltaTime / JumpJetDuration);		//!! * Iterations
			}
			else
			{
				bDoubleJumping = false;
			}
		}
	}

	Super::physFalling( deltaTime, Iterations );

	// NOTE: physics might have changed in call to Super, e.g. if Pawn landed, so make sure still falling
	if( Health > 0 )
	{
		if( Physics == PHYS_Falling )
		{
			if( MantleDebugLevel >= 2 && TRYINGTOMANTLE )
				debugf( L"%0.3f MANTLEDEBUG: doing PHYS_Falling check Location.Z=%0.3f", TIMESECONDS, Location.Z );
			MantleCheck( false, OldLocation, OldRotation );
		}
	}
	
	unguard;
}

//-----------------------------------------------------------------------------

void AU2Pawn::performPhysics(FLOAT DeltaSeconds)
{
	guard(AU2Pawn::performPhysics);
	Super::performPhysics(DeltaSeconds);
	UDestroyOnDamage::StaticNotify(this);
	unguard;
}

//-----------------------------------------------------------------------------

IMPLEMENT_CLASS(AU2Pawn);
