/*=============================================================================
	UnLevTic.cpp: Level timer tick function
	Copyright 1997-2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"
#include "UnPath.h"
/*-----------------------------------------------------------------------------
	Helper classes.
-----------------------------------------------------------------------------*/

//
// Priority sortable list.
//
struct FActorPriority
{
	INT			    Priority;	// Update priority, higher = more important.
	AActor*			Actor;		// Actor.
	UActorChannel*	Channel;	// Actor channel.
	FActorPriority()
	{}
	FActorPriority( FVector& ViewPos, FVector& ViewDir, UNetConnection* InConnection, UActorChannel* InChannel, AActor* InActor )
	{
		guard(FActorPriority::FActorPriority);

		Actor       = InActor;
		Channel     = InChannel;
		FLOAT Time  = Channel ? (InConnection->Driver->Time - Channel->LastUpdateTime) : InConnection->Driver->SpawnPrioritySeconds;
		if ( !Actor->bHidden )
		{
			FVector Dir = Actor->Location - ViewPos;
			if ( (ViewDir | Dir) < 0.f )
				Time *= 0.5f;
		}
		Priority = appRound(65536.0f * Actor->NetPriority * Time);
		unguard;
	}
	friend INT Compare( const FActorPriority* A, const FActorPriority* B )
	{
		return B->Priority - A->Priority;
	}
};

/*-----------------------------------------------------------------------------
	Tick a single actor.
-----------------------------------------------------------------------------*/

UBOOL AActor::CheckOwnerUpdated()
{
	guardSlow(AActor::CheckOwnerUpdated);
	if( Owner && (INT)Owner->bTicked!=GetLevel()->Ticked )
	{
		GetLevel()->NewlySpawned = new(GEngineMem)FActorLink(this,GetLevel()->NewlySpawned);
		return 0;
	}
	return 1;
	unguardSlow;
}

UBOOL APawn::CheckOwnerUpdated()
{
	guardSlow(APawn::CheckOwnerUpdated);
	if( Owner && (INT)Owner->bTicked!=GetLevel()->Ticked )
	{
		GetLevel()->NewlySpawned = new(GEngineMem)FActorLink(this,GetLevel()->NewlySpawned);
		return 0;
	}
	// Handle controller-first updating.
	if( Controller && (INT)Controller->bTicked!=GetLevel()->Ticked )
	{
		GetLevel()->NewlySpawned = new(GEngineMem)FActorLink(this,GetLevel()->NewlySpawned);
		return 0;
	}
	return 1;
	unguardSlow;
}

void ALineOfSightTrigger::TickAuthoritative( FLOAT DeltaSeconds )
{
	guardSlow(ALineOfSightTrigger::TickAuthoritative);

	AActor::TickAuthoritative(DeltaSeconds);

	if ( bEnabled && !bTriggered && SeenActor
		&& (SeenActor->LastRenderTime >= OldTickTime) )
	{
		// find player pawn, and check if close enough
		for ( AController *C=Level->ControllerList; C; C=C->nextController )
		{
			if ( C->LocalPlayerController() )
			{
			    APlayerController *PC = Cast<APlayerController>(C);
				if ( (PC->GetViewTarget()->Location - Location).SizeSquared() < MaxViewDist*MaxViewDist )
				{
					// check if looking directly enough at target
					FVector Dir = (SeenActor->Location - PC->ViewTarget->Location).SafeNormal();
					if ( (PC->Rotation.Vector() | Dir) > RequiredViewDir )
					{
						FCheckResult Hit(1.f);
						GetLevel()->SingleLineCheck( Hit, this, SeenActor->Location, PC->ViewTarget->Location, TRACE_World|TRACE_StopAtFirstHit );
						if ( !Hit.Actor )
							eventPlayerSeesMe(PC);
					}
					break;
				}
			}
		}
	}
	OldTickTime = Level->TimeSeconds;

	unguardSlow;
}

void AActor::TickAuthoritative( FLOAT DeltaSeconds )
{
	guardSlow(AActor::TickAuthoritative);

	// Tick the nonplayer.
	//clockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));
	eventTick(DeltaSeconds);
	//unclockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));

	// Update the actor's script state code.
	ProcessState( DeltaSeconds );

	UpdateTimers(DeltaSeconds );

	// Update LifeSpan.
	if( LifeSpan!=0.f )
	{
		LifeSpan -= DeltaSeconds;
		if( LifeSpan <= 0.0001f )
		{
			// Actor's LifeSpan expired.
			GetLevel()->DestroyActor( this );
			return;
		}
	}

	// Perform physics.
	if ( !bDeleteMe && (Physics!=PHYS_None) && (Role!=ROLE_AutonomousProxy) )
		performPhysics( DeltaSeconds );
	unguardSlow;
}

void AActor::TickSimulated( FLOAT DeltaSeconds )
{
	guardSlow(AActor::TickSimulated);

	TickAuthoritative(DeltaSeconds);
	unguardSlow;
}

void APawn::TickSimulated( FLOAT DeltaSeconds )
{
	guardSlow(APawn::TickSimulated);

	// Simulated Physics for pawns
	// simulate gravity
	Acceleration = Velocity.SafeNormal();
	if ( PhysicsVolume->bWaterVolume )
		Physics = PHYS_Swimming;
	else if ( bCanClimbLadders && PhysicsVolume->IsA(ALadderVolume::StaticClass()) )
		Physics = PHYS_Ladder;
	else if ( bSimulateGravity )
		Physics = PHYS_Walking;	// set physics mode guess for use by animation
	else 
		Physics = PHYS_Flying;

	//simulated pawns just predict location, no script execution
	moveSmooth(Velocity * DeltaSeconds);

	// if simulated gravity, check if falling
	if ( bSimulateGravity && !bSimGravityDisabled )
	{
		FCheckResult Hit(1.f);
		if ( Velocity.Z == 0.f )
			GetLevel()->SingleLineCheck(Hit, this, Location - FVector(0.f,0.f,1.5f * CollisionHeight), Location, TRACE_AllBlocking, FVector(CollisionRadius,CollisionRadius,4.f));
		else if ( Velocity.Z < 0.f )
			GetLevel()->SingleLineCheck(Hit, this, Location - FVector(0.f,0.f,8.f), Location, TRACE_AllBlocking, GetCylinderExtent());

		if ( (Hit.Time == 1.f) || (Hit.Normal.Z < UCONST_MINFLOORZ) )
		{
			if ( Velocity.Z == 0.f )
				Velocity.Z = 0.15f * PhysicsVolume->Gravity.Z;
			Velocity += 0.5f * PhysicsVolume->Gravity * DeltaSeconds;
			Physics = PHYS_Falling; 
		}
		else
			Velocity.Z = 0.f;
	}

	// Tick the nonplayer.
	//clockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));
	eventTick(DeltaSeconds);
	//unclockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));

	unguardSlow;
}

void AActor::TickSpecial( FLOAT DeltaSeconds )
{
}

void APawn::TickSpecial( FLOAT DeltaSeconds )
{
	guardSlow(APawn::TickSpecial);

	if ( !bInterpolating && bPhysicsAnimUpdate && Mesh )
		UpdateMovementAnimation(DeltaSeconds);

	// update weapon location (in case its playing sounds, etc.)
	// bAttachedMove = true (last switch in FarMoveActor) for weapon.
	if ( Weapon && !Weapon->bDeleteMe ) 
	{
		if ( !IsLocallyControlled() || !IsHumanControlled() 
			|| (Controller->GetViewTarget() != this) || ((APlayerController *)Controller)->bBehindView )
			GetLevel()->FarMoveActor( Weapon, Location, 0, 0, 1 ); 
	}
	// update eyeheight if someone is viewing through this pawn's eyes
	if ( bUpdateEyeheight || IsHumanControlled() )
		eventUpdateEyeHeight(DeltaSeconds);

	if( (Role==ROLE_Authority) && (BreathTime > 0.f) )
	{
		BreathTime -= DeltaSeconds;
		if (BreathTime < 0.001f)
		{
			BreathTime = 0.0f;
			eventBreathTimer();
		}
	}
	unguardSlow;
}

UBOOL AActor::PlayerControlled()
{
	return 0;
}

UBOOL APawn::PlayerControlled()
{
	return ( IsLocallyControlled() );
}

UBOOL AActor::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(AActor::Tick);

	// Ignore actors in stasis
	if
	(	bStasis 
	&&	((Physics==PHYS_None) || (Physics == PHYS_Rotating))
	&&	(GetLevel()->TimeSeconds - LastRenderTime > 5)
	&&	(Level->NetMode == NM_Standalone) )
	{
		bTicked = GetLevel()->Ticked;
		return 1;
	}

	// Handle owner-first updating.
	if ( !CheckOwnerUpdated() )
		return 0;
	bTicked = GetLevel()->Ticked;

	// Non-player update.
	if( (TickType==LEVELTICK_ViewportsOnly) && !PlayerControlled() )
		return 1;

	// Update all animation, including multiple passes if necessary.
	// Meshes are allowed autonomous updating.	
#if 1 //NEW (mdf) Legend stats	
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_ActorTickCycles));
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_AnimationCycles));
	UpdateAnimation(DeltaSeconds);
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_AnimationCycles));
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_ActorTickCycles));
#else	
	UpdateAnimation(DeltaSeconds);
#endif

	// This actor is tickable.
	if( RemoteRole == ROLE_AutonomousProxy ) 
	{
        APlayerController *PC = GetTopPlayerController();//Cast<APlayerController>(GetTopOwner());
		if ( (PC && PC->LocalPlayerController()) || Physics == PHYS_Karma )
			TickAuthoritative(DeltaSeconds);
		else
		{
			eventTick(DeltaSeconds);

			// Update the actor's script state code.
			ProcessState( DeltaSeconds );
			// Server handles timers for autonomous proxy.
			UpdateTimers( DeltaSeconds );

			if ( !bDeleteMe && (Physics!=PHYS_None) && PC && (Level->TimeSeconds - PC->ServerTimeStamp > 0.5f * PC->MaxResponseTime) 
				&& (PC->ServerTimeStamp != 0.f) )
			{
				// force position update
				if ( !Velocity.IsZero() )
				{
					performPhysics( Level->TimeSeconds - PC->ServerTimeStamp );
				}
				PC->ServerTimeStamp = Level->TimeSeconds;
			}
		}
	}
	else if ( Role>ROLE_SimulatedProxy )
		TickAuthoritative(DeltaSeconds);
	else if ( Role == ROLE_SimulatedProxy )
		TickSimulated(DeltaSeconds);
	else if ( !bDeleteMe && ((Physics == PHYS_Falling) || (Physics == PHYS_Rotating)) ) // dumbproxies simulate falling if client side physics set
		performPhysics( DeltaSeconds );

	if ( bDeleteMe )
		return 1;

	TickSpecial(DeltaSeconds);	// perform any tick functions unique to an actor subclass
	return 1;
	unguard;
}


/* Controller Tick
Controllers are never animated, and do not look for an owner to be ticked before them
Non-player controllers don't support being an autonomous proxy
*/
UBOOL AController::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(AController::Tick);

	// Ignore controllers with no pawn, or pawn is in stasis
	if
	(	bStasis
	|| (TickType==LEVELTICK_ViewportsOnly) 
	|| (Pawn && Pawn->bStasis 
	&&	((Pawn->Physics==PHYS_None) || (Pawn->Physics == PHYS_Rotating))
	&&	(GetLevel()->TimeSeconds - Pawn->LastRenderTime > 5)
	&&	(Level->NetMode == NM_Standalone)) )
	{
		bTicked = GetLevel()->Ticked;
		return 1;
	}

	bTicked = GetLevel()->Ticked;

	if( Role>=ROLE_SimulatedProxy )
		TickAuthoritative(DeltaSeconds);
	
	// Update eyeheight and send visibility updates
	// with PVS, monsters look for other monsters, rather than sending msgs

	if( Role==ROLE_Authority && TickType==LEVELTICK_All )
	{
#if 1 //NEW (mdf) XMP approach
		if( Pawn && Pawn->Health > 0 )
		{
			UBOOL bValidEnemy = ValidPawn( Enemy ); // note: doesn't take changes in attitude to enemy into account
			
			if( !bValidEnemy )
			{
				if( Enemy )
				{
					// current enemy is set but isn't valid
					if( IsProbing( NAME_EnemyInvalid ) )
						eventEnemyInvalid();
				}
			}
			else
			{
				// has a valid enemy
	
				// check for other range transiton
				if( Ranges.Num() > 0 )
				{
					if( RangeCounter < 0.0f )
					{
						if( IsProbing( NAME_EnemyRangeTransition ) )
							CheckForEnemyRangeTransition();
						RangeCounter += 0.25f;
					}
					RangeCounter = RangeCounter - DeltaSeconds; 
	 			}
			}

			// check for loss of LOS
			if( SightCounter < 0.0f )
			{
				/*
				mdf-tbd: I'm going to try allowing this code to check enemy visibility all 
				the time for help with weapon firing or other code that cares about LOS to 
				the target. Keep an eye out for greatly increased see times, but afaik
				most of the time EnemyNotVisible is being probed when the NPC has an enemy
				anyway so this shouldn't add much overhead. I want to do this because
				several states (e.g. retreating) ignore EnemyNotVisible even though the
				enemy will often be visible which means that (without this change) the 
				NPC's LastSeenTime etc. won't be getting updated. 
				*/
				if( bValidEnemy )
				{
					CheckEnemyVisible();
					SightCounter += 0.10f;
				}
			}
			
			SightCounter = SightCounter - DeltaSeconds; 
			
			//NOTE: ShowSelf now called in ULevel::ShowControllers
		}
#else 
/*
		if( SightCounter < 0.0f )
		{
			if( IsProbing(NAME_EnemyNotVisible) )
			{
				CheckEnemyVisible();
				SightCounter = 0.05f + 0.1f * appFrand();
			}
			else
				SightCounter += 0.15f + 0.1f * appFrand();
		}

		SightCounter = SightCounter - DeltaSeconds; 
		// for best performance, players show themselves to players and non-players (e.g. monsters),
		// and monsters show themselves to players
		// but monsters don't show themselves to each other
		// also

		if( Pawn && !Pawn->bHidden && !Pawn->bAmbientCreature )
			ShowSelf();
*/
#endif
	}

	if ( Pawn )
	{
		// rotate pawn toward focus
		if ( Pawn->bRotateToDesired )
			Pawn->rotateToward(Focus, FocalPoint);

		// face same direction as pawn
		Rotation = Pawn->Rotation;
	}
	if ( MonitoredPawn )
	{
		if ( !Pawn || MonitoredPawn->bDeleteMe || !MonitoredPawn->Controller )
			eventMonitoredPawnAlert();
		else
		{
			// quit if further than max dist, moving away fast, or has moved far enough
			if ( ((MonitoredPawn->Location - Pawn->Location).SizeSquared() > MonitorMaxDistSq)
				|| ((MonitoredPawn->Location - MonitorStartLoc).SizeSquared() > 0.25f * MonitorMaxDistSq) )
				eventMonitoredPawnAlert();
			else if ( (MonitoredPawn->Velocity.SizeSquared() > 0.6f * MonitoredPawn->GroundSpeed)
				&& ((MonitoredPawn->Velocity | (MonitorStartLoc - Pawn->Location)) > 0.f)
				&& ((MonitoredPawn->Location - Pawn->Location).SizeSquared() > 0.25f * MonitorMaxDistSq) )
				eventMonitoredPawnAlert();
		}
	}
	return 1;
	unguard;
}



/* 
PlayerControllers
Controllers are never animated, and do not look for an owner to be ticked before them
*/
UBOOL APlayerController::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(APlayerController::Tick);

	bTicked = GetLevel()->Ticked;

	TimeSinceLastFogChange += DeltaSeconds;

	GetViewTarget();
	if( (RemoteRole == ROLE_AutonomousProxy) && !LocalPlayerController() ) 
	{
		// update viewtarget replicated info
		if( ViewTarget != Pawn )
		{
            APawn* TargetPawn = (ViewTarget && ViewTarget->IsAPawn()) ? Cast<APawn>(ViewTarget) : NULL; 
			if ( TargetPawn )
			{
				if ( TargetPawn->Controller && TargetPawn->Controller->IsA(APlayerController::StaticClass()) )
					TargetViewRotation = TargetPawn->Controller->Rotation;
				else
					TargetViewRotation = TargetPawn->Rotation;
				TargetEyeHeight = TargetPawn->BaseEyeHeight;
				if ( TargetPawn->Weapon )
					TargetWeaponViewOffset = TargetPawn->Weapon->PlayerViewOffset;
			}
		}

		// Update the actor's script state code.
		ProcessState( DeltaSeconds );
		// Server handles timers for autonomous proxy.
		UpdateTimers( DeltaSeconds );
	}
	else if( Role>=ROLE_SimulatedProxy )
	{
		// Player update.
		if( IsA(ACamera::StaticClass()) && !(ShowFlags & SHOW_PlayerCtrl) )
			return 1;

		// Process PlayerTick with input.
		if ( Player )
		{
			if ( !PlayerInput )
				eventInitInputSystem();
			if ( PlayerInput )
			{
				Player->ReadInput( DeltaSeconds );
				eventPlayerTick( DeltaSeconds );
				Player->ReadInput( -1.0f );
			}
		}

		// Update the actor's script state code.
		ProcessState( DeltaSeconds );

		UpdateTimers( DeltaSeconds );

		if ( bDeleteMe )
			return 1;

		// Perform physics.
		if( Physics!=PHYS_None && Role!=ROLE_AutonomousProxy )
			performPhysics( DeltaSeconds );
	}

	// During demo playback, setup view offsets for viewtarget
	if( GetLevel()->DemoRecDriver && GetLevel()->DemoRecDriver->ServerConnection )
	{
		if( Role == ROLE_Authority )
		{
			// update viewtarget replicated info
			if( !bBehindView )
			{
				APawn* TargetPawn = Cast<APawn>(ViewTarget);
				if ( TargetPawn )
				{
					TargetViewRotation = Rotation;
					TargetEyeHeight = TargetPawn->EyeHeight;
					if ( TargetPawn->Weapon )
						TargetWeaponViewOffset = TargetPawn->Weapon->PlayerViewOffset;
				}
			}
		}
	}

	// Update eyeheight and send visibility updates
	// with PVS, monsters look for other monsters, rather than sending msgs
	if( Role==ROLE_Authority && TickType==LEVELTICK_All )
	{
		if( SightCounter < 0.0f )
			SightCounter += 0.2f;

		SightCounter = SightCounter - DeltaSeconds; 
		if( Pawn && !Pawn->bHidden )
			ShowSelf();
	}

	return 1;
	unguard;
}

void AActor::UpdateTimers(FLOAT DeltaSeconds)
{
	guardSlow(AActor::UpdateTimers);

	// Update timers.
	if( TimerRate>0.0f && (TimerCounter+=DeltaSeconds)>=TimerRate )
	{
		// Normalize the timer count.
		INT TimerTicksPassed = 1;
		if( TimerRate > 0.0f )
		{
			TimerTicksPassed     = (int)(TimerCounter/TimerRate);
			TimerCounter -= TimerRate * TimerTicksPassed;
			if( TimerTicksPassed && !bTimerLoop )
			{
				// Only want a one-shot timer message.
				TimerTicksPassed = 1;
				TimerRate = 0.0f;
			}
		}

		// Call timer routine with count of timer events that have passed.
		eventTimer();
	}

	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Network client tick.
-----------------------------------------------------------------------------*/

void ULevel::TickNetClient( FLOAT DeltaSeconds )
{
	guard(ULevel::TickNetClient);
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_NetTickCycles));
	if( NetDriver->ServerConnection->State==USOCK_Open )
	{
		/* Don't replicate any properties from client to server
		for( TMap<AActor*,UActorChannel*>::TIterator ItC(NetDriver->ServerConnection->ActorChannels); ItC; ++ItC )
		{
			guard(UpdateLocalActors);
			UActorChannel* It = ItC.Value();
			APawn* PlayerPawn = It->GetActor() ? Cast<APawn>(It->GetActor()) : NULL; 
			if( PlayerPawn && PlayerPawn->Controller )
			{
				APlayerController* PC = Cast<APlayerController>(PlayerPawn->Controller);
				if ( PC && PC->Player )
					It->ReplicateActor();
			}
			unguard;
		}
		*/
	}
	else if( NetDriver->ServerConnection->State==USOCK_Closed )
	{
		// Server disconnected.

		// Determine if it's a map change or disconnect
/*FIXME_MERGE
		UGameEngine* GE = Cast<UGameEngine>(Engine);

		if (!(GE && GE->GPendingLevel))
			GE->SetProgress( *FString::Printf(TEXT("menu:%s"),*GE->DisconnectMenuClass), LocalizeError(TEXT("ConnectLost"),TEXT("Engine")), TEXT("") );			
*/
		check(Engine->Client->Viewports.Num());
		Engine->SetClientTravel( Engine->Client->Viewports(0), TEXT("?closed"), 0, TRAVEL_Absolute );
	}
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_NetTickCycles));
	unguard;
}

/*-----------------------------------------------------------------------------
	Network server ticking individual client.
-----------------------------------------------------------------------------*/

static TArray<AActor*> VisitedNetDependents;

UBOOL AActor::IsNetRelevantFor( APlayerController* RealViewer, AActor* Viewer, FVector SrcLocation )
{
	guard(AActor::IsNetRelevantFor);
	VisitedNetDependents.Empty();
	return RecurseIsNetRelevantFor( RealViewer, Viewer, SrcLocation );
	unguard;
}

UBOOL AActor::RecurseIsNetRelevantFor( APlayerController* RealViewer, AActor* Viewer, FVector SrcLocation )
{
	guard(AActor::RecurseIsNetRelevantFor);

	if( bAlwaysRelevant || IsOwnedBy(Viewer) || IsOwnedBy(RealViewer) || this==Viewer || Viewer==Instigator || RealViewer->bAllActorsRelevant )
		return 1;
	else if( AmbientSound 
			&& ((Location-Viewer->Location).SizeSquared() < Square(GAudioMaxRadiusMultiplier*SoundRadius)) )
		return 1;
	else if ( (Owner && (Base == Owner) && !bOnlyOwnerSee)
			|| (Base && (AttachmentBone != NAME_None) && Cast<USkeletalMesh>(Base->Mesh)) )
		return Base->RecurseIsNetRelevantFor( RealViewer, Viewer, SrcLocation );

	// check for actors that rely on us being relevant
	for( int iDependent=0; iDependent < NetDependents.Num(); iDependent++ )
		if( NetDependents(iDependent) && NetDependents(iDependent) != this && !NetDependents(iDependent)->IsPendingKill() )
		{
			bool bAlreadyVisited = false;
			for( int iVisited=0; iVisited < VisitedNetDependents.Num(); iVisited++ )
				if( NetDependents(iDependent) == VisitedNetDependents(iVisited) )
					bAlreadyVisited = true;

			if( !bAlreadyVisited )
			{
				VisitedNetDependents.AddItem( NetDependents(iDependent) );
				if( NetDependents(iDependent)->RecurseIsNetRelevantFor( RealViewer, Viewer, SrcLocation ) )
				{
					return 1;
				}
			}
		}

	if( (bHidden || bOnlyOwnerSee) && !bBlockPlayers && !AmbientSound )
		return 0;
	else
	{
		// check distance fog
		if ( Region.Zone->bDistanceFog && ((Location - SrcLocation).SizeSquared() > Region.Zone->DistanceFogEnd * Region.Zone->DistanceFogEnd) )
			return 0;
		if ( GetLevel()->Model->FastLineCheck(Location,SrcLocation) )
		{
			/*
			// check against terrain
			// FIXME - Jack, it would be nice to make this faster (a linecheck that just returns whether there was a hit)
			AZoneInfo* Z = Viewer->Region.Zone;
			if( Z && Z->bTerrainZone ) 
			{
				FVector End = Location;
				End.Z += 2.f * CollisionHeight;
				FCheckResult Hit(1.f);
				for( INT t=0;t<Z->Terrains.Num();t++ )
				{
					if( Z->Terrains(t)->LineCheck( Hit, End, SrcLocation, FVector(0.f,0.f,0.f) )==0 )
					{
						FPointRegion HitRegion = GetLevel()->Model->PointRegion( GetLevel()->GetLevelInfo(), Hit.Location );
						if( HitRegion.Zone == Z )
							return 0;
					}
				}
			} */
			/*
			// check against antiportal volumes
			FCheckResult Hit(1.f);
			for ( INT i=0; i<GetLevel()->AntiPortals.Num(); i++ )
				if ( GetLevel()->AntiPortals(i) && GetLevel()->AntiPortals(i)->GetPrimitive()->LineCheck( Hit, GetLevel()->AntiPortals(i), Location, SrcLocation, FVector(0.f,0.f,0.f), 0, TRACE_StopAtFirstHit )==0 )
					return 0;
			*/
			return 1;
		}
		return 0;
	}
	unguard;
}

UBOOL APlayerController::IsNetRelevantFor( APlayerController* RealViewer, AActor* Viewer, FVector SrcLocation )
{
	guardSlow(APlayerController::IsNetRelevantFor);

	return ( this==RealViewer );
	unguardSlow;
}

UBOOL APawn::CacheNetRelevancy(UBOOL bIsRelevant, APlayerController* RealViewer, AActor* Viewer)
{
	guardSlow(APawn::CacheNetRelevancy);

	bCachedRelevant = bIsRelevant;
	NetRelevancyTime = Level->TimeSeconds;
	LastRealViewer = RealViewer;
	LastViewer = Viewer;
	return bIsRelevant;
	unguardSlow;
}

UBOOL APawn::IsNetRelevantFor( APlayerController* RealViewer, AActor* Viewer, FVector SrcLocation )
{
	guardSlow(APawn::IsNetRelevantFor);
	VisitedNetDependents.Empty();
	return RecurseIsNetRelevantFor( RealViewer, Viewer, SrcLocation );
	unguardSlow;
}

UBOOL APawn::RecurseIsNetRelevantFor( APlayerController* RealViewer, AActor* Viewer, FVector SrcLocation )
{
	guardSlow(APawn::RecurseIsNetRelevantFor);

	if ( (NetRelevancyTime == Level->TimeSeconds) && (RealViewer == LastRealViewer) && (Viewer == LastViewer) )
		return bCachedRelevant;
	if( bAlwaysRelevant || IsOwnedBy(Viewer) || IsOwnedBy(RealViewer) || this==Viewer || Viewer==Instigator || RealViewer->bAllActorsRelevant )
		return CacheNetRelevancy(true,RealViewer,Viewer);
	if( AmbientSound 
			&& ((Location-Viewer->Location).SizeSquared() < Square(GAudioMaxRadiusMultiplier*SoundRadius)) )
		return CacheNetRelevancy(true,RealViewer,Viewer);

	// check for actors that rely on us being relevant
	for( int iDependent=0; iDependent < NetDependents.Num(); iDependent++ )
		if( NetDependents(iDependent) && NetDependents(iDependent) != this && !NetDependents(iDependent)->IsPendingKill() )
		{
			bool bAlreadyVisited = false;
			for( int iVisited=0; iVisited < VisitedNetDependents.Num(); iVisited++ )
				if( NetDependents(iDependent) == VisitedNetDependents(iVisited) )
					bAlreadyVisited = true;

			if( !bAlreadyVisited )
			{
				VisitedNetDependents.AddItem( NetDependents(iDependent) );
				if( NetDependents(iDependent)->RecurseIsNetRelevantFor( RealViewer, Viewer, SrcLocation ) )
				{
					return CacheNetRelevancy(true,RealViewer,Viewer);
				}
			}
		}

	if( (bHidden || bOnlyOwnerSee) && !bBlockPlayers && !AmbientSound )
		return CacheNetRelevancy(false,RealViewer,Viewer);
	else
	{
		// check distance fog
		if ( Region.Zone->bDistanceFog && ((Location - SrcLocation).SizeSquared() > Region.Zone->DistanceFogEnd * Region.Zone->DistanceFogEnd) )
			return CacheNetRelevancy(false,RealViewer,Viewer);

		// check against BSP - check head and center
		//debugf(TEXT("Check relevance of %s"),*(PlayerReplicationInfo->PlayerName));
		if ( !GetLevel()->Model->FastLineCheck(Location + FVector(0.f,0.f,CollisionHeight),SrcLocation) 
			 && !GetLevel()->Model->FastLineCheck(Location,SrcLocation)  )
			return CacheNetRelevancy(false,RealViewer,Viewer);
		/*
		if ( CollisionRadius <= COMMONRADIUS )
			return 0;

		// if large collision volume, check edges
		if ( GetLevel()->Model->FastLineCheck(Location + FVector(0.f,CollisionRadius,0.f),SrcLocation) )
			return CacheNetRelevancy(true,RealViewer,Viewer);
		if ( GetLevel()->Model->FastLineCheck(Location - FVector(0.f,CollisionRadius,0.f),SrcLocation) )
			return CacheNetRelevancy(true,RealViewer,Viewer);
		if ( GetLevel()->Model->FastLineCheck(Location + FVector(CollisionRadius,0.f,0.f),SrcLocation) )
			return CacheNetRelevancy(true,RealViewer,Viewer);
		if ( GetLevel()->Model->FastLineCheck(Location - FVector(CollisionRadius,0.f,0.f),SrcLocation) )
			return CacheNetRelevancy(true,RealViewer,Viewer);
		return 0;
		*/
		/*
		// check against terrain
		// FIXME - Jack, it would be nice to make this faster (a linecheck that just returns whether there was a hit)
		AZoneInfo* Z = Viewer->Region.Zone;
		if( Z && Z->bTerrainZone ) 
		{
			FVector End = Location;
			End.Z += 2.f * CollisionHeight;
			FCheckResult Hit(1.f);
			for( INT t=0;t<Z->Terrains.Num();t++ )
			{
				if( Z->Terrains(t)->LineCheck( Hit, End, SrcLocation, FVector(0.f,0.f,0.f) )==0 )
				{
					FPointRegion HitRegion = GetLevel()->Model->PointRegion( GetLevel()->GetLevelInfo(), Hit.Location );
					if( HitRegion.Zone == Z )
					return CacheNetRelevancy(false,RealViewer,Viewer);
				}
			}
		}
		*/
		/*
		// check against antiportal volumes
		FCheckResult Hit(1.f);
		for ( INT i=0; i<GetLevel()->AntiPortals.Num(); i++ )
			if ( GetLevel()->AntiPortals(i) 
				&& GetLevel()->AntiPortals(i)->GetPrimitive()->LineCheck( Hit, GetLevel()->AntiPortals(i), Location + FVector(0.f,0.f,CollisionHeight), SrcLocation, FVector(0.f,0.f,0.f), 0, TRACE_StopAtFirstHit )==0 )
				return CacheNetRelevancy(false,RealViewer,Viewer);
		*/
		return CacheNetRelevancy(true,RealViewer,Viewer);
	}
	unguardSlow;
}

INT ULevel::ServerTickClients( FLOAT DeltaSeconds )
{
	guard(ULevel::ServerTickClients);

	if ( NetDriver->ClientConnections.Num() == 0 )
		return 0;
	INT Updated=0;

	FMemMark Mark(GMem);
	// initialize connections
	for( INT i=NetDriver->ClientConnections.Num()-1; i>=0; i-- )
{
		UNetConnection* Connection = NetDriver->ClientConnections(i);
	check(Connection);
	check(Connection->State==USOCK_Pending || Connection->State==USOCK_Open || Connection->State==USOCK_Closed);

	// Handle not ready channels.
	if( Connection->Actor && Connection->IsNetReady(0) && Connection->State==USOCK_Open 
			&& (Connection->Driver->Time - Connection->LastReceiveTime < 1.5f) )
	{
			Connection->Viewer = Connection->Actor->GetViewTarget();
			Connection->OwnedConsiderList = new(GMem,Actors.Num()-iFirstNetRelevantActor+2)AActor*;
			Connection->OwnedConsiderListSize = 0;
		}
		else
			Connection->Viewer = NULL;
	}
			
	// make list of actors to consider
	AActor **ConsiderList = new(GMem,Actors.Num()-iFirstNetRelevantActor+2)AActor*;
	INT ConsiderListSize = 0;

	// Add LevelInfo to considerlist
	if( Actors(0) && (Actors(0)->RemoteRole!=ROLE_None) )
	{
		ConsiderList[0] = Actors(0);
		ConsiderListSize++;
	}
	FLOAT ServerTickTime = Engine->GetMaxTickRate();
	if ( ServerTickTime == 0.f )
		ServerTickTime = DeltaSeconds;
	else
		ServerTickTime = 1.f/ServerTickTime;

	for( INT i=iFirstNetRelevantActor; i<Actors.Num(); i++ )
	{
		AActor* Actor = Actors(i);
		if( Actor 
			&& (Actor->RemoteRole!=ROLE_None) 
			&& (TimeSeconds > Actor->NetUpdateTime) ) 
		{
			Actor->NetUpdateTime = TimeSeconds + appFrand() * ServerTickTime + 1.f/Actor->NetUpdateFrequency; // FIXME - cache 1/netupdatefreq
			if ( Actor->bAlwaysRelevant || !Actor->bOnlyRelevantToOwner ) 
			{
				ConsiderList[ConsiderListSize] = Actor;
				ConsiderListSize++;
			}
			else
			{
				AActor* ActorOwner = Actor->Owner;
				for ( INT j=0; j<NetDriver->ClientConnections.Num(); j++ )
				{
					UNetConnection* Connection = NetDriver->ClientConnections(j);
					if ( Connection->Viewer && ((ActorOwner == Connection->Viewer) || (ActorOwner == Connection->Actor)) )
	{
						Connection->OwnedConsiderList[Connection->OwnedConsiderListSize] = Actor;
						Connection->OwnedConsiderListSize++;
					}
				}
			}
		}
	}

	for( INT i=NetDriver->ClientConnections.Num()-1; i>=0; i-- )
	{
		UNetConnection* Connection = NetDriver->ClientConnections(i);

		if( Connection->Viewer )
		{
			AActor*      Viewer    = Connection->Viewer;
			APlayerController* InViewer  = Connection->Actor;
			InViewer->Level->ReplicationViewer = InViewer;
			InViewer->Level->ReplicationViewTarget = Viewer;

		// Get list of visible/relevant actors.
		FLOAT PruneActors = 0.f;
		clock(PruneActors);
		FMemMark Mark(GMem);
		NetTag++;
		Connection->TickCount++;

		// Set up to skip all sent temporary actors.
		guard(SkipSentTemporaries);
		for( INT i=0; i<Connection->SentTemporaries.Num(); i++ )
			Connection->SentTemporaries(i)->NetTag = NetTag;
		unguard;

		// Get viewer coordinates.
		FVector      Location  = Viewer->Location;
		FRotator     Rotation  = InViewer->Rotation;
		InViewer->eventPlayerCalcView( Viewer, Location, Rotation );
		check(Viewer);

		// Compute ahead-vectors for prediction.
		FVector Ahead = FVector(0,0,0);
		if( Connection->TickCount & 1 )
		{
			FLOAT PredictSeconds = (Connection->TickCount&2) ? 0.4f : 0.9f;
			Ahead = PredictSeconds * Viewer->Velocity;
			if( Viewer->Base )
				Ahead += PredictSeconds * Viewer->Base->Velocity;
			FCheckResult Hit(1.0f);
			Hit.Location = Location + Ahead;
			Viewer->GetLevel()->Model->LineCheck(Hit,NULL,Hit.Location,Location,FVector(0,0,0),NF_NotVisBlocking,0);
			Location = Hit.Location;
		}

		// Make list of all actors to consider.
		INT              ConsiderCount  = 0;
		FActorPriority*  PriorityList   = new(GMem,Actors.Num()-iFirstNetRelevantActor+2)FActorPriority;
		FActorPriority** PriorityActors = new(GMem,Actors.Num()-iFirstNetRelevantActor+2)FActorPriority*;
		FVector          ViewPos        = Viewer->Location;
		FVector          ViewDir        = InViewer->Rotation.Vector();

		guard(MakeConsiderList);

			for( INT i=0; i<ConsiderListSize; i++ )
				{
				AActor* Actor = ConsiderList[i];
				if( Actor->NetTag!=NetTag )
					{
				UActorChannel* Channel = Connection->ActorChannels.FindRef(Actor);
				if ( Actor->bOnlyDirtyReplication 
					&& Channel 
						&& !Channel->ActorDirty
							&& Channel->Recent.Num() 
						&& Channel->Dirty.Num() == 0 )
						{
							Channel->RelevantTime = NetDriver->Time;
						}
						else
						{
							Actor->NetTag                 = NetTag;
					PriorityList  [ConsiderCount] = FActorPriority( ViewPos, ViewDir, Connection, Channel, Actor );
							PriorityActors[ConsiderCount] = PriorityList + ConsiderCount++;
						}
					}
				}
			for( INT i=0; i<Connection->OwnedConsiderListSize; i++ )
		{
				AActor* Actor = Connection->OwnedConsiderList[i];
				if( Actor->NetTag!=NetTag )
					{
						UActorChannel* Channel = Connection->ActorChannels.FindRef(Actor);
						if ( Actor->bOnlyDirtyReplication 
							&& Channel
						&& !Channel->ActorDirty
							&& Channel->Recent.Num() 
						&& Channel->Dirty.Num() == 0 )
						{
							Channel->RelevantTime = NetDriver->Time;
						}
						else
						{
							Actor->NetTag                 = NetTag;
							PriorityList  [ConsiderCount] = FActorPriority( ViewPos, ViewDir, Connection, Channel, Actor );
							PriorityActors[ConsiderCount] = PriorityList + ConsiderCount++;
						}
					}
		}
			Connection->OwnedConsiderList = NULL;
			Connection->OwnedConsiderListSize = 0;
		Connection->LastRepTime = Connection->Driver->Time;
		unguard;

		FLOAT RelevantTime = 0.f;
		// Sort by priority.
		guard(SortConsiderList);
		Sort( PriorityActors, ConsiderCount );
		unguard;
			unclock(PruneActors);

		// Update all relevant actors in sorted order.
		guard(UpdateRelevant);
			clock(RelevantTime);
		for( INT j=0; j<ConsiderCount; j++ )
		{
			UActorChannel* Channel     = PriorityActors[j]->Channel;
			if ( !Channel || Channel->Actor ) //make sure didn't just close this channel
			{
				AActor*        Actor       = PriorityActors[j]->Actor;
			UBOOL          CanSee      = 0;
				// only check visibility on already visible actors every 1.0 + 0.5R seconds
			// bTearOff actors should never be checked
				if ( !Actor->bTearOff && (!Channel || NetDriver->Time-Channel->RelevantTime>1.f) )
				CanSee = Actor->IsNetRelevantFor( InViewer, Viewer, Location );
			if( CanSee || (Channel && NetDriver->Time-Channel->RelevantTime<NetDriver->RelevantTimeout) )
			{
				// Find or create the channel for this actor.
				GStats.DWORDStats(GEngineStats.STATS_Net_NumPV)++;
				if( !Channel && Connection->PackageMap->ObjectToIndex(Actor->GetClass())!=INDEX_NONE )
				{
					// Create a new channel for this actor.
					Channel = (UActorChannel*)Connection->CreateChannel( CHTYPE_Actor, 1 );
					if( Channel )
						Channel->SetChannelActor( Actor );
				}
				if( Channel )
				{
					if ( !Connection->IsNetReady(0) ) // here also???
						break;
					if( CanSee )
							Channel->RelevantTime = NetDriver->Time + 0.5f * appFrand();
					if( Channel->IsNetReady(0) )
					{
							//debugf(TEXT("Replicate %s"),Actor->GetName());
						Channel->ReplicateActor();
						Updated++;
					}
					if ( !Connection->IsNetReady(0) )
						break;
				}
			}
			else if( Channel )
				{
				Channel->Close();
					// skeletal attachments aren't relevant anymore either
					if ( Cast<USkeletalMesh>(Actor->Mesh) )
					{
						for ( INT k=0; k<Actor->Attached.Num(); k++ )
							if ( Actor->Attached(k) && !Actor->Attached(k)->bAlwaysRelevant 
								&& (Actor->Attached(k)->AttachmentBone != NAME_None) )
							{
								UChannel *AttachedChannel = Connection->ActorChannels.FindRef(Actor->Attached(k));
								if ( AttachedChannel )
									AttachedChannel->Close();
							}
					}
				}
			}
		}
		unguard;
		Mark.Pop();
		unclock(RelevantTime);
			//debugf(TEXT("Potential %04i ConsiderList %03i ConsiderCount %03i Prune=%01.4f Relevance=%01.4f"),Actors.Num()-iFirstNetRelevantActor, 
			//			ConsiderListSize, ConsiderCount, PruneActors * GSecondsPerCycle * 1000.f,RelevantTime * GSecondsPerCycle * 1000.f);
	}
	}
	Mark.Pop();
	return Updated;
	unguard;
}

/*-----------------------------------------------------------------------------
	Network server tick.
-----------------------------------------------------------------------------*/

void UNetConnection::SetActorDirty(AActor* DirtyActor )
{
	guardSlow(UNetConnection::SetActorDirty);
	if( Actor && State==USOCK_Open )
	{
		UActorChannel* Channel = ActorChannels.FindRef(DirtyActor);
		if ( Channel )
			Channel->ActorDirty = true;
	}
	unguardSlow;
}

void ULevel::TickNetServer( FLOAT DeltaSeconds )
{
	guard(ULevel::TickNetServer);

	// Update all clients.
	INT Updated=0;

	// first, set which channels have dirty actors (need replication)
	AActor* Actor = Actors(0);
	if( Actor && Actor->bNetDirty )
	{
		for( INT j=NetDriver->ClientConnections.Num()-1; j>=0; j-- )
			NetDriver->ClientConnections(j)->SetActorDirty(Actor);
		Actor->bNetDirty = 0;
	}
	for( INT i=iFirstNetRelevantActor; i<Actors.Num(); i++ )
	{
		AActor* Actor = Actors(i);
		if( Actor && Actor->bNetDirty )
		{
			for( INT j=NetDriver->ClientConnections.Num()-1; j>=0; j-- )
				NetDriver->ClientConnections(j)->SetActorDirty(Actor);
			Actor->bNetDirty = 0;
		}
	}
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_NetTickCycles));
	Updated = ServerTickClients( DeltaSeconds );
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_NetTickCycles));

	// Log message.
	if( (INT)(TimeSeconds-DeltaSeconds)!=(INT)(TimeSeconds) )
		debugf( NAME_Title, LocalizeProgress(TEXT("RunningNet"),TEXT("Engine")), *GetLevelInfo()->Title, *URL.Map, NetDriver->ClientConnections.Num() );

	// Stats.
	if( Updated )
	{
		for( INT i=0; i<NetDriver->ClientConnections.Num(); i++ )
		{
			UNetConnection* Connection = NetDriver->ClientConnections(i);
			if( Connection->Actor && Connection->State==USOCK_Open )
			{
				if( Connection->UserFlags&1 )
				{
					// Send stats.
					INT NumActors=0;
					for( INT i=0; i<Actors.Num(); i++ )
						NumActors += Actors(i)!=NULL;

					FString Stats = FString::Printf
					(
						TEXT("r=%i cli=%i act=%03.1f (%i) net=%03.1f pv/c=%i rep/c=%i rpc/c=%i"),
						appRound(Engine->GetMaxTickRate()),
						NetDriver->ClientConnections.Num(),
						GSecondsPerCycle*1000*GStats.DWORDStats(GEngineStats.STATS_Game_ActorTickCycles),
						NumActors,
						GSecondsPerCycle*1000*GStats.DWORDStats(GEngineStats.STATS_Game_NetTickCycles),
						GStats.DWORDStats(GEngineStats.STATS_Net_NumPV)   / NetDriver->ClientConnections.Num(),
						GStats.DWORDStats(GEngineStats.STATS_Net_NumReps) / NetDriver->ClientConnections.Num(),
						GStats.DWORDStats(GEngineStats.STATS_Net_NumRPC)  / NetDriver->ClientConnections.Num()
					);
					Connection->Actor->eventClientMessage( *Stats, NAME_None );
				}
				if( Connection->UserFlags&2 )
				{
					FString Stats = FString::Printf
					(
						TEXT("snd=%02.1f recv=%02.1f"),
						GSecondsPerCycle*1000*Connection->Driver->SendCycles,
						GSecondsPerCycle*1000*Connection->Driver->RecvCycles
					);
					Connection->Actor->eventClientMessage( *Stats, NAME_None );
				}
			}
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	Demo Recording tick.
-----------------------------------------------------------------------------*/

INT ULevel::TickDemoRecord( FLOAT DeltaSeconds )
{
	guard(ULevel::TickDemoRecord);

	// All replicatable actors are assumed to be relevant for demo recording.
	UNetConnection* Connection = DemoRecDriver->ClientConnections(0);
	for( INT i=iFirstNetRelevantActor; i<Actors.Num(); i++ )
	{
		AActor* Actor = Actors(i);
		UBOOL IsNetClient = (GetLevelInfo()->NetMode == NM_Client);
		if
		(	Actor
		&&	( (IsNetClient && Actor->bTearOff) || Actor->RemoteRole!=ROLE_None || (IsNetClient && Actor->Role!=ROLE_None && Actor->Role != ROLE_Authority))
		&&  (!Actor->bNetTemporary || Connection->SentTemporaries.FindItemIndex(Actor)==INDEX_NONE)
		&&  (Actor->bStatic || !Actor->GetClass()->GetDefaultActor()->bStatic))
		{
			// Create a new channel for this actor.
			UActorChannel* Channel = Connection->ActorChannels.FindRef( Actor );
			if( !Channel && Connection->PackageMap->ObjectToIndex(Actor->GetClass())!=INDEX_NONE )
			{
				// Check we haven't run out of actor channels.
				Channel = (UActorChannel*)Connection->CreateChannel( CHTYPE_Actor, 1 );
				check(Channel);
				Channel->SetChannelActor( Actor );
			}
			if( Channel )
			{
				// Send it out!
				check(!Channel->Closing);
				if( Channel->IsNetReady(0) )
				{
					Actor->bDemoRecording = 1;
					Actor->bNetDirty = 1;
					Actor->bClientDemoRecording = IsNetClient;
					UBOOL TornOff = 0;
					if(IsNetClient)
					{
						if( Actor->bTearOff && Actor->Role==ROLE_Authority && Actor->RemoteRole==ROLE_None )
						{
							TornOff = 1;
							Actor->RemoteRole = ROLE_SimulatedProxy;
						}
						else
						Exchange(Actor->RemoteRole, Actor->Role);
					}
					Channel->ReplicateActor();
					if(IsNetClient)
					{
						if( TornOff )
							Actor->RemoteRole = ROLE_None;
						else
						Exchange(Actor->RemoteRole, Actor->Role);
					}
					Actor->bDemoRecording = 0;
					Actor->bClientDemoRecording = 0;
				}
			}
		}
	}
	return 1;
	unguard;
}
INT ULevel::TickDemoPlayback( FLOAT DeltaSeconds )
{
	guard(ULevel::TickDemoPlayback);
	if
	(	GetLevelInfo()->LevelAction==LEVACT_Connecting 
	&&	DemoRecDriver->ServerConnection->State!=USOCK_Pending )
	{
		GetLevelInfo()->LevelAction = LEVACT_None;
		Engine->SetProgress( TEXT(""), TEXT(""), 0.0f );
	} 
	if( DemoRecDriver->ServerConnection->State==USOCK_Closed && !DemoRecDriver->Loop )
	{
		// Demo stopped playing
		check(Engine->Client->Viewports.Num());
		Engine->SetClientTravel( Engine->Client->Viewports(0), TEXT("?entry"), 0, TRAVEL_Absolute );
	}
	return 1;
	unguard;
}

/*-----------------------------------------------------------------------------
	Main level timer tick handler.
-----------------------------------------------------------------------------*/
UBOOL ULevel::IsPaused()
{
	guardSlow(ULevel::IsPaused);
	
	return ( GetLevelInfo()->Pauser && (TimeSeconds >= GetLevelInfo()->PauseDelay) );
	unguardSlow;
}

#if 1 //NEW (mdf) randomize order in which controllers show themselves
void ULevel::ShowControllers()
{
	guard(ULevel::ShowControllers)

	// start in the controller list at a random offset
	INT NumControllers = 0;
	for ( AController* C=GetLevelInfo()->ControllerList; C; C=C->nextController )
		NumControllers++;

	INT StartOffset = appRandRange( 0, NumControllers );

	AController* StartController = GetLevelInfo()->ControllerList;
	for ( INT ii=0; ii<StartOffset; ii++ )
		StartController = StartController->nextController;

	for( INT ii=0; ii<NumControllers; ii++ )
	{
		if( StartController->Role==ROLE_Authority )
		{
			if( StartController->Pawn && !StartController->Pawn->bHidden )
			{
				//debugf( L"%0.3f calling ShowSelf #1", TimeSeconds );
				StartController->ShowSelf(); 
			}
		}
		StartController = StartController->nextController;
		if( !StartController )
			StartController = GetLevelInfo()->ControllerList; //wrap
	}

	unguard;
}
#endif

//
// Update the level after a variable amount of time, DeltaSeconds, has passed.
// All child actors are ticked after their owners have been ticked.
//
void ULevel::Tick( ELevelTick TickType, FLOAT DeltaSeconds )
{
	guard(ULevel::Tick);
	ALevelInfo* Info = GetLevelInfo();

	FMemMark Mark(GMem);
	FMemMark EngineMark(GEngineMem);
	GInitRunaway();
	InTick=1;

	UBOOL RecordDemoFrame = 0;
	guard(UpdateDemo);
	if( DemoRecDriver )
	{
		RecordDemoFrame = DemoRecDriver->UpdateDemoTime( &DeltaSeconds );
		DemoRecDriver->TickDispatch( DeltaSeconds );

		// Fetch demo playback packets from demo file.
		if( DemoRecDriver->ServerConnection )
			TickDemoPlayback( DeltaSeconds );
	}
	unguard;

	// Update the net code and fetch all incoming packets.
	guard(UpdatePreNet);
	if( NetDriver )
	{
		NetDriver->TickDispatch( DeltaSeconds );
		if( NetDriver->ServerConnection )
			TickNetClient( DeltaSeconds );
	}
	unguard;

	// Update collision.
	guard(UpdateCollision);
	if( Hash )
		Hash->Tick();
	unguard;

	// Update time.
	guard(UpdateTime);
	if( !GIsBenchmarking )
	DeltaSeconds *= Info->TimeDilation;
	if ( !IsPaused() )
		TimeSeconds += DeltaSeconds;
	Info->TimeSeconds = TimeSeconds;
/* GERMAN VERSION
	AGameInfo *DefaultGameActor = Cast<AGameInfo>((AGameInfo::StaticClass())->GetDefaultActor());
	if ( DefaultGameActor )
		DefaultGameActor->bAlternateMode = true;
	if ( Info->Game )
		Info->Game->bAlternateMode = true;
*/
	UpdateTime(Info);
	if( Info->bPlayersOnly )
		TickType = LEVELTICK_ViewportsOnly;
	unguard;

	// Clamp time between 2000 fps and 2.5 fps.
	DeltaSeconds = Clamp(DeltaSeconds,0.0005f,0.40f);

	// If caller wants time update only, or we are paused, skip the rest.
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_ActorTickCycles));
	if
	(	(TickType!=LEVELTICK_TimeOnly)
	&&	!IsPaused()
	&&	(!NetDriver || !NetDriver->ServerConnection || NetDriver->ServerConnection->State==USOCK_Open) )
	{
		// Tick all actors, owners before owned.
		guard(TickAllActors);
		NewlySpawned = NULL;
		INT Updated  = 1;

#ifdef WITH_KARMA
        /* KTODO: Should we do this _after_ actor physics? */
        KTickLevelKarma(this, DeltaSeconds);
#endif

		for( INT iActor=iFirstDynamicActor; iActor<Actors.Num(); iActor++ )
			if( Actors( iActor ) && !Actors(iActor)->bDeleteMe )
				Updated += Actors( iActor )->Tick(DeltaSeconds,TickType);
		while( NewlySpawned && Updated )
		{
			FActorLink* Link = NewlySpawned;
			NewlySpawned     = NULL;
			Updated          = 0;
			for( Link; Link; Link=Link->Next )
				if( Link->Actor->bTicked!=(DWORD)Ticked && !Link->Actor->bDeleteMe )
					Updated += Link->Actor->Tick( DeltaSeconds, TickType );
		}
#ifdef WITH_KARMA
		if(KGData)
			KGData->bDoTick = 0;
#endif
		ShowControllers(); //NEW (mdf)
		
		unguard;
	}
	else if( IsPaused() )
	{
		// Absorb input if paused.
		guard(AbsorbedPaused);
		NewlySpawned = NULL;
		INT Updated  = 1;
		for( INT iActor=iFirstDynamicActor; iActor<Actors.Num(); iActor++ )
		{
			APlayerController* PC=Cast<APlayerController>(Actors(iActor));
			if( PC && Cast<UViewport>(PC->Player) )
			{
				PC->Player->ReadInput( DeltaSeconds );
				PC->PlayerInput->eventPlayerInput( DeltaSeconds );
				for( TFieldIterator<UFloatProperty> It(PC->GetClass()); It; ++It )
					if( It->PropertyFlags & CPF_Input )
						*(FLOAT*)((BYTE*)PC + It->Offset) = 0.f;
				PC->bTicked = (DWORD)Ticked;
			}
			else if( Actors(iActor) )
			{
				if ( Actors(iActor)->bAlwaysTick && !Actors(iActor)->bDeleteMe )
					Actors(iActor)->Tick(DeltaSeconds,TickType);
				else
					Actors(iActor)->bTicked = (DWORD)Ticked;
			}
		}
		while( NewlySpawned && Updated )
		{
			FActorLink* Link = NewlySpawned;
			NewlySpawned     = NULL;
			Updated          = 0;
			for( Link; Link; Link=Link->Next )
				if( Link->Actor->bTicked!=(DWORD)Ticked && !Link->Actor->bDeleteMe )
				{
					if ( Link->Actor->bAlwaysTick )
						Updated += Link->Actor->Tick( DeltaSeconds, TickType );
					else
						Link->Actor->bTicked = (DWORD)Ticked;
				}
		}
		unguard;
	}
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_ActorTickCycles));
/*
	if ( (GStats.DWORDStats(GEngineStats.STATS_Game_ActorTickCycles) * GSecondsPerCycle * 1000.0f >= 30.f)
		|| (GScriptCycles * GSecondsPerCycle * 1000.0f > 10.f) )
 		debugf(TEXT("********** ActorTick Cycles %f at %f pathtime %f script time %f"), 
					GStats.DWORDStats(GEngineStats.STATS_Game_ActorTickCycles) * GSecondsPerCycle * 1000.0f, 
					TimeSeconds,
					GStats.DWORDStats(GEngineStats.STATS_Game_FindPathCycles) * GSecondsPerCycle * 1000.0f,
					GScriptCycles * GSecondsPerCycle * 1000.0f
					);
*/
//!!MERGE	if ( Engine->Client && Engine->Client->InteractionMaster )
//!!MERGE		Engine->Client->InteractionMaster->MasterProcessTick(DeltaSeconds);

	// Update net server and flush networking.
	guard(UpdateNetServer);
	if( NetDriver )
	{
		if( !NetDriver->ServerConnection )
			TickNetServer( DeltaSeconds );
		NetDriver->TickFlush();
	}
	unguard;

	// Demo Recording.
	guard(UpdatePostDemoRec);
	if( DemoRecDriver )
	{
		if( !DemoRecDriver->ServerConnection && RecordDemoFrame )
			TickDemoRecord( DeltaSeconds );
		DemoRecDriver->TickFlush();
	}
	unguard;

	// Finish up.
	Ticked = !Ticked;
	InTick = 0;
	Mark.Pop();
	EngineMark.Pop();

	clock(GStats.DWORDStats(GEngineStats.STATS_Game_CleanupDestroyedCycles));
	CleanupDestroyed( 0 );
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_CleanupDestroyedCycles));
	
	unguardf(( TEXT("(NetMode=%i)"), GetLevelInfo()->NetMode ));
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

