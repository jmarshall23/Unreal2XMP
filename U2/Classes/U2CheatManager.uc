//=============================================================================
// U2CheatManager -- LEGEND DEBUG/CHEAT EXEC FUNCTIONS
//=============================================================================

class U2CheatManager extends CheatManager
	config(user);

// trace debug facility
var private class<HitActorDebugInfoManager> DebugInfoManagerClass;
var private bool bTraceDebugEnabled; // set when trace debug enabled
var HitActorDebugInfoManager DebugInfoManager;
var private Actor LockedActor; // Actor used as target of many functions if set

// debug scoreboard
var private class<ScoreboardInfoManager> ScoreboardInfoManagerClass;
var ScoreboardInfoManager DebugScoreboard;

var	private bool bBehindViewEnabled;

var() private globalconfig string SummonPackageStrings[ 16 ];	// packages iterated in Summon to find actors
var() private globalconfig string GoodiesList[ 32 ];			// list of goodies to spawn with Goodies

//-----------------------------------------------------------------------------

simulated function Cleanup()
{
	if( DebugInfoManager != None )
	{
		DebugInfoManager.Cleanup();
		DebugInfoManager.Delete();
		DebugInfoManager = None;
	}
	bTraceDebugEnabled = false;

	if( DebugScoreboard != None )
	{
		DebugScoreboard.Cleanup();
		DebugScoreboard.Delete();
		DebugScoreboard = None;
	}
	
    Super.Cleanup();
}

//-----------------------------------------------------------------------------

function exec TestAssert()
{
	assert( false );
}

//------------------------------------------------------------------------------
// Note: should only be used for summoning stuff through exec functions (i.e. 
// with summon etc.) -- we don't want anything else that is summoned to be
// dependent on the SummonPackageStrings in particular .ini files..

static function class<Object> LoadPackageClass( string ClassName )
{
	local string GivenClassName;
	local class<Object> NewClass;
	local int i;

	GivenClassName = ClassName;
	if( instr( ClassName, "." ) >= 0 )
	{
		NewClass = class<Object>( DynamicLoadObject( ClassName, class'Class' ) );
	}
	else
	{
		// no package given -- try various packages
		for( i=0; i<ArrayCount(default.SummonPackageStrings); i++ )
		{
			if( default.SummonPackageStrings[ i ] != "" )
			{
				ClassName = default.SummonPackageStrings[ i ] $ "." $ GivenClassName;

				NewClass = class<Object>( DynamicLoadObject( ClassName, class'Class', true ) );
				if( NewClass != None )
				{
					break;
				}
			}
		}
	}

	return NewClass;
}

//-----------------------------------------------------------------------------

function Object SummonPackageClass( string ClassName, optional bool bUseViewRotation )
{
	local class<Object> NewClass;
	local Object Obj;
	
	Obj = None;
	if( ClassName == "" )
	{
		DM( "SummonPackageClass: no class given!" );
	}
	else
	{
		NewClass = LoadPackageClass( ClassName );
		Obj = class'Util'.static.SummonClass( NewClass, ClassName, Outer, Pawn, bUseViewRotation );
		HandleSpawnedObject( Obj, ClassName );
	}

	return Obj;
}

//-----------------------------------------------------------------------------

exec function Sum( string ClassName )
{
	DMTN( "Sum  Role: " $ EnumStr( enum'ENetRole', Role ) );
	SummonPackageClass( ClassName );
}

//-----------------------------------------------------------------------------
// Teleports player to where he is looking. 
//
// Generally used with a binding like:
//
//	bind x Teleport | OnRelease UnTeleport
//
// so holding down 'x' teleports player to the given location and will stay
// "stuck" there until 'x' key is released.
//
// However if UnTeleport isn't called (e.g. used with separate
//
//  bind x Teleport
//  bind y UnTeleport
//
// keys), player can use one key to teleport to given location (and stay
// stuck there) and another key to unteleport (or can re-teleport to
// a new location (using the fly/ghost cheat when teleporting is another
// way to teleport without falling when the teleport key is released).
//
// When reteleported, if the move fails, will remain teleported at current
// location (see state TeleportFixed).
//
// While teleported, should be able to turn, fire and Actor CC is correctly 
// rotated.

simulated function DoTeleport()
{
	local vector HitLocation, HitNormal, TargetLocation;
	local Actor HitActor;
	local vector StartLocation;
	//mdf-tbd: local float SavedRadius, SavedHeight;
	local vector Direction;
	local vector Extents;
	local bool bSuccess;

	if( Pawn == None )
	{
		DM( "Can't teleport -- no Pawn!" );
		return;
	}

	StartLocation = Pawn.Location; 
	StartLocation.Z += Pawn.BaseEyeHeight;
	Direction = vector(GetViewRotation());
	//StartLocation += Direction*LPawn.CrouchRadius;
	HitActor = class'Util'.static.TraceRecursive( Pawn, HitLocation, HitNormal, StartLocation, true, 0.0, Direction );

	if( HitActor != None )
	{	
		/* mdf-tbd: (CC not getting correctly restored in XMP build) 
		// shrink CC to for teleporting
		SavedRadius = Pawn.CollisionRadius;
		SavedHeight = Pawn.CollisionHeight;
		*/
		
		//mdf-tbd:
		//if( class'Util'.static.AdjustCollisionSize( Pawn, Pawn.CrouchRadius, Pawn.CrouchHeight ) )
		if( true )
		{
			// static mesh and terrain hack -- these don't currently "pop" actors out of geometry correctly
			if( StaticMeshActor(HitActor) != None || TerrainInfo(HitActor) != None )
				HitLocation = HitLocation + HitNormal*FMax( Pawn.default.CollisionRadius, Pawn.default.CollisionHeight );

			TargetLocation = HitLocation;

			// try zero extent result location offset by Pawn's radius along HitNormal
			bSuccess = Pawn.SetLocation( TargetLocation + Pawn.CrouchRadius*HitNormal );
	
			if( !bSuccess )
			{
				// try extent trace back from hit location
				Extents.X = Pawn.CrouchRadius;
				Extents.Y = Pawn.CrouchRadius;
				Extents.Z = Pawn.CrouchHeight;

				if( StartLocation != TargetLocation )
				{
					HitActor = class'Util'.static.TraceRecursive( Pawn, HitLocation, HitNormal, TargetLocation, true, 0.0, -Direction, , Extents );
					bSuccess = Pawn.SetLocation( HitLocation );
				}

				if( !bSuccess )
				{
					// try extent trace forward towards hit location
					HitActor = class'Util'.static.TraceRecursive( Pawn, HitLocation, HitNormal, StartLocation, true, 0.0, Direction, , Extents );
					bSuccess = Pawn.SetLocation( HitLocation );
				}
			}

			if( bSuccess )
			{
				// stick to hit location
				//mdf-tbd: Pawn.bWantsToCrouch = true;
				Outer.GotoState( 'TeleportFixed' );
			}
			else
			{
				// try to restore saved CC (should always work if didn't move)
				//mdf-tbd: Pawn.SetCollisionSize( SavedRadius, SavedHeight );
				DM( "DoTeleport -- teleport failed!" );
			}
		}
		else
		{
			DM( "DoTeleport -- couldn't set crouching size!" );
		}
	}
	else
	{
		DM( "DoTeleport -- failed to hit anything!" );
	}
}

//-----------------------------------------------------------------------------

exec function Teleport()
{
	DoTeleport();
}

//-----------------------------------------------------------------------------

exec function UnTeleport()
{
	Outer.DoUnTeleport();
}

//-----------------------------------------------------------------------------

exec function GotoActor( name ObjectName )
{
	local Actor A;

	A = GetNamedActor( ObjectName );
	if( A != None )
	{
		if( !Pawn.SetLocation( A.Location ) )
		{
			ClientMessage( "GotoActor: couldn't set location (try ghosting first?)" );
		}
	}
	else
	{
		ClientMessage( "GotoActor: " $ ObjectName $ " not found!" );
	}
}

//-----------------------------------------------------------------------------

function Actor GetHitActor()
{
	return class'Util'.static.GetHitActor( Outer, true );
}

//-----------------------------------------------------------------------------

function Actor GetNamedActor( name N )
{
	return class'UtilGame'.static.GetNamedActor( Outer, N, true, true );
}

//-----------------------------------------------------------------------------

function Actor GetNamedOrHitActor( optional name N )
{
	return class'UtilGame'.static.GetNamedOrHitActor( Outer, N, true, true );
}

//-----------------------------------------------------------------------------
// Return LockedActor if set or Actor under crosshair if any.

function Actor GetLockedOrHitActor( bool bNeedsMesh, optional bool bVerbose )
{
	local Actor A;
	
	if( LockedActor == None )
	{
		A = GetNamedOrHitActor( '' );
		if( A != None && bNeedsMesh ) 
		{
			if( A.Mesh == None )
			{
				A = None;
			}
		}
	}
	else
	{
		A = LockedActor;
	}

	if( bVerbose && A == None )
		DM( "GetLockedOrHitActor: no target animation Actor!" );

	return A;
}

//-----------------------------------------------------------------------------

function Actor GetNamedLockedOrHitActor( optional name N, optional bool bNeedsMesh, optional bool bVerbose )
{
	if( N != '' )
		return GetNamedActor( N );
	else
		return GetLockedOrHitActor( bNeedsMesh, bVerbose );
}

//-----------------------------------------------------------------------------
// Return named Actor (tries to match name 1st, class 2nd) if given, locked
// Actor otherwise (which might not be set).

function Actor GetNamedOrLockedActor( optional name N )
{
	if( N != '' )
		return class'UtilGame'.static.GetNamedActor( Outer, N, true, true );
	else 
		return LockedActor;
}

//-----------------------------------------------------------------------------

exec function ToggleBehindView()
{
	bBehindViewEnabled = !bBehindViewEnabled;
	BehindView( bBehindViewEnabled );
}

//-----------------------------------------------------------------------------

exec function ToggleInvisibility()
{
	if( Pawn != None )
	{
		if( !Pawn.bHidden )
		{
			DM( "invisibility enabled" );
			ConsoleCommand( "invisible 1" );
			//!!MERGE: tell AI to forget about player: Pawn.ClearReferencesTo();
		}
		else
		{
			DM( "invisibility disabled" );
			ConsoleCommand( "invisible 0" );
		}
	}
	else
	{
		DM( "Can't toggleinvisibility -- Pawn not set!" );
	}
}

//-----------------------------------------------------------------------------

exec function ToggleScoreboard()
{
	if( DebugScoreboard == None )
	{
		DebugScoreboard = new ScoreboardInfoManagerClass;
		DebugScoreboard.Initialize( Outer );
	}

	DebugScoreboard.CycleEnabled();
}

//-----------------------------------------------------------------------------

function HitActorDebugInfoManager GetTracerInstance()
{
	if( DebugInfoManager == None )
	{
		DebugInfoManager = new DebugInfoManagerClass;
		DebugInfoManager.Initialize( Outer );
	}

	return DebugInfoManager;
}

//-----------------------------------------------------------------------------
// Enables on-screen information for Actor that the player is looking at.
// !!mdf-tbd: won't work in MP (myHUD not set)?

exec function TraceToggle( optional name TargetName )
{
	local string DMString;
	local Actor TargetActor;

	if( TargetName != '' )
	{
		TargetActor = GetNamedActor( TargetName );
		if( TargetActor != None )
		{
			// make sure we compare apples with apples
			if( Pawn(TargetActor) != None && Pawn(TargetActor).Controller != None )
				TargetActor = Pawn(TargetActor).Controller;

			if( GetTracerInstance().GetActorToShow() == TargetActor )
			{
				// unlock it
				DM( "TraceToggle: unlocked " $ TargetActor.Name );
				GetTracerInstance().SetActorToShow( None );
				bTraceDebugEnabled = false;
			}
			else
			{
				// lock it
				DM( "TraceToggle: locked " $ TargetActor.Name );
				GetTracerInstance().SetActorToShow( TargetActor );
				ForceTraceEnable();
				bTraceDebugEnabled = true;
			}	
		}
		else
		{
			DM( "TraceToggle: can't find " $ TargetName );
		}
	}
	else
	{
		bTraceDebugEnabled = !bTraceDebugEnabled;
	
		if( bTraceDebugEnabled )
		{
			// make sure allocated
			GetTracerInstance();
	
			DMString = "TraceToggle: Enabled (";
	
			DMString = DMString $ "Console: " $ GetTracerInstance().GetEchoToConsole();
			DMString = DMString $ ", Log: " $ GetTracerInstance().GetEchoToLog();
	
			DMString = DMString $ ")";
		}
		else
		{
			GetTracerInstance().SetActorToShow( None );
			DMString = "TraceToggle: Disabled";
		}
	
		GetTracerInstance().SetEnabled( byte(bTraceDebugEnabled) );
		DM( DMString );
	}
}

//-----------------------------------------------------------------------------

function ForceTraceEnable()
{
	// if not tracing, enable it
	if( !bTraceDebugEnabled )
		TraceToggle();
}

//-----------------------------------------------------------------------------

exec function TraceLock( optional name TargetName )
{
	local Actor TargetActor;
	local bool bWasLocked;

	TargetActor = GetNamedOrHitActor( TargetName );
	if( TargetActor != None )
	{
		bWasLocked = (GetTracerInstance().GetActorToShow() != None);
		if( !bWasLocked || LevelInfo(TargetActor) == None || GetTracerInstance().GetShowLevel() )
		{
			DM( "TraceLock: locked " $ TargetActor.Name );
			GetTracerInstance().SetActorToShow( TargetActor );
			ForceTraceEnable();
		}
		else
		{
			if( bWasLocked )
				DM( "TraceLock: cleared locked Actor");
	
			GetTracerInstance().SetActorToShow( None );
		}
	}
	else
	{
		DM( "TraceLock: no target Actor!");
	}
}

//-----------------------------------------------------------------------------

exec function TraceToggleLevel()
{
	GetTracerInstance().SetShowLevel( !GetTracerInstance().GetShowLevel() );
		
	DM( "TraceToggleLevel: " $ GetTracerInstance().GetShowLevel() );
}

//-----------------------------------------------------------------------------

exec function TraceToggleConsole()
{
	GetTracerInstance().SetEchoToConsole( !GetTracerInstance().GetEchoToConsole() );
		
	DM( "TraceToggleConsole: " $ GetTracerInstance().GetEchoToConsole() );
}

//-----------------------------------------------------------------------------

exec function TraceToggleLog()
{
	GetTracerInstance().SetEchoToLog( !GetTracerInstance().GetEchoToLog() );
	
	DM( "TraceToggleLog: " $ GetTracerInstance().GetEchoToLog() );
}

//-----------------------------------------------------------------------------

exec function TraceToggleAnimInfo()
{
	GetTracerInstance().SetShowAnimInfo( !GetTracerInstance().GetShowAnimInfo() );
	
	DM( "TraceToggleAnimInfo: " $ GetTracerInstance().GetShowAnimInfo() );
}

//-----------------------------------------------------------------------------

function bool CheckNamedOrHitActor( string SourceFunctionName, Actor NamedOrHitActor, name TargetName )
{
	local bool bRetVal;

	bRetVal = true;
	if( NamedOrHitActor == None )
	{
		if( TargetName == '' )
		{
			DM( "CheckNamedOrHitActor (" $ SourceFunctionName $ ") -- no Actor under crosshair or no source Pawn!" );
		}
		else
		{
			DM( "CheckNamedOrHitActor (" $ SourceFunctionName $ ") -- named Actor not found or no source Pawn!" );
		}

		bRetVal = false;
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Traces to the Actor the crosshair is pointing at, and toggles their
// collision cylinder drawing on/off.

exec function ToggleCC( optional name TargetName )
{
	local Actor TargetActor;

	TargetActor = GetNamedLockedOrHitActor( TargetName );

	if( LevelInfo(TargetActor) == None && Mover(TargetActor) == None )
	{
		if( CheckNamedOrHitActor( "ToggleCC", TargetActor, TargetName ) )
		{
			if( (TargetActor.SpecialDisplayFlags & DF_Collision) == 0 )
			{
				DM( "ToggleCC: Showing CC for " $ TargetActor );
				TargetActor.SpecialDisplayFlags = TargetActor.SpecialDisplayFlags | DF_Collision;
			}
			else
			{
				DM( "ToggleCC: Hiding CC for " $ TargetActor );
				TargetActor.SpecialDisplayFlags = TargetActor.SpecialDisplayFlags & ~DF_Collision;
			}
		}
	}
	else
	{
		DM( "ToggleCC: can't show cc for " $ TargetActor );
	}
}

//-----------------------------------------------------------------------------

function SetCCs( class<Actor> TargetClass, bool bShow )
{
	local Actor A;
	
	foreach AllActors( TargetClass, A )
	{
		if( bShow )
		{
			if( A.bHidden )
				A.bHidden = false;
			A.SpecialDisplayFlags = A.SpecialDisplayFlags | DF_Collision;
		}
		else
		{
			if( !A.bHidden && A.default.bHidden )
				A.bHidden = true;
			A.SpecialDisplayFlags = A.SpecialDisplayFlags & ~DF_Collision;
		}
	}				
}

//-----------------------------------------------------------------------------

exec function ShowCCs( string TargetClassStr )
{
	local class<Actor> TargetClass;

	TargetClass = class<Actor>(LoadPackageClass( TargetClassStr ) );
	if( TargetClass != None )
	{
		DM( "ShowCCs: " $ TargetClass );
		SetCCs( TargetClass, true );
	}
	else
	{
		DM( "ShowCCs: can't find class " $ TargetClassStr );
	}
}

//-----------------------------------------------------------------------------

exec function HideCCs( string TargetClassStr )
{
	local class<Actor> TargetClass;

	TargetClass = class<Actor>(LoadPackageClass( TargetClassStr ) );
	if( TargetClass != None )
	{
		DM( "HideCCs: " $ TargetClass );
		SetCCs( TargetClass, false );
	}
	else
	{
		DM( "HideCCs: can't find class " $ TargetClassStr );
	}
}

//-----------------------------------------------------------------------------
// Toggle bHidden flag for all actors which match the given class name that are
// bHidden by default (so actors which are normally visible won't be affected).
// If bForce is true, *all* actors will be toggled regardless of whether they
// are normally hidden.

function DoToggleShowClass( class<Actor> ClassToShow, bool bForce )
{
	local Actor A;
	local bool bHideActors;

	// make a first pass through actors to see if any applicable actors
	// aren't hidden and if any found assume that we should hide these
	// otherwise assume that we should make these visible
	bHideActors = false;
	foreach AllActors( class'Actor', A )
	{
		if( ClassIsChildOf( A.Class, ClassToShow ) )
		{
			if( !A.bHidden && (bForce || A.default.bHidden) )
			{
				bHideActors = true;
				break;
			}
		}
	}

	if( bHideActors )
	{
		DM( "Hiding " $ ClassToShow.Name $ "s" $ " (bForce: " $ bForce $ ")" );
	}
	else
	{
		DM( "Showing " $ ClassToShow.Name $ "s" $ " (bForce: " $ bForce $ ")" );
	}
	
	// now apply flag
	foreach AllActors( class'Actor', A )
	{
		if( ClassIsChildOf( A.Class, ClassToShow ) )
		{
			if( !bHideActors && A.bHidden )
			{
				// make any hidden objects visible
				DM( "  Showing " $ A );
				A.bHidden = false;
			}
			else if( bHideActors && !A.bHidden && (bForce || A.default.bHidden) )
			{
				DM( "  Hiding " $ A );
				A.bHidden = true;
			}
			
			A.SetPropertyText( "StaticFilterState", "FS_Maybe", true );
		}
	}
}

//-----------------------------------------------------------------------------

exec function ToggleShowClass( class<Actor> ClassToShow )
{
	DoToggleShowClass( ClassToShow, false );
}

//-----------------------------------------------------------------------------

exec function ToggleShowClassF( class<Actor> ClassToShow )
{
	DoToggleShowClass( ClassToShow, true );
}

//-----------------------------------------------------------------------------

exec function ToggleShowAll()
{
	DoToggleShowClass( class'Actor', false );
}

//-----------------------------------------------------------------------------

exec function ToggleShowKPs()
{
	DoToggleShowClass( class'KeyPoint', false );
}

//-----------------------------------------------------------------------------

exec function ToggleShowNPs()
{
	DoToggleShowClass( class'NavigationPoint', false );
}

//-----------------------------------------------------------------------------

exec function Damage( int DamageAmount, optional name TargetName )
{
	local Actor TargetActor;

	TargetActor = GetNamedLockedOrHitActor( TargetName );
	if( Controller(TargetActor) != None )
		TargetActor = Controller(TargetActor).Pawn;

	if( Pawn(TargetActor) != None )
		Pawn(TargetActor).TakeDamage( DamageAmount, Pawn, TargetActor.Location, vect(0,0,0), class'DamageType' );
	else
		DM( "Damage: no valid target" );
}

//-----------------------------------------------------------------------------

exec function SetHealth( int NewHealth, optional name TargetName )
{
	local Actor TargetActor;
	local Controller C;

	if( TargetName == 'all' )
	{
		DM( "SetHealth - setting health for all Pawns to " $ NewHealth );
		for( C=Level.ControllerList; C!=None; C=C.NextController )
			if( C.Pawn != None )
				C.Pawn.Health = NewHealth;
	}
	else
	{
		TargetActor = GetNamedLockedOrHitActor( TargetName );
		if( Controller(TargetActor) != None )
			TargetActor = Controller(TargetActor).Pawn;

		if( Pawn(TargetActor) != None )
		{
			DM( "SetHealth - setting health for " $ Pawn(TargetActor).Name $ " to " $ NewHealth );
			Pawn(TargetActor).Health = NewHealth;
		}
		else
			DM( "SetHealth: no valid target" );
	}
}

//-----------------------------------------------------------------------------

exec function SetMyHealth( int NewHealth )
{
	SetHealth( NewHealth, Pawn.Name );
}

//-----------------------------------------------------------------------------

exec function HurtMe( int Damage )
{
	Pawn.TakeDamage( Damage, Pawn, Location, vect(0,0,0), class'Suicided' );
}

//-----------------------------------------------------------------------------

exec function HealMe( int Damage )
{
	Pawn.OnUse( Pawn );
}

//-----------------------------------------------------------------------------

exec function DumpInv()
{
	class'UtilGame'.static.DumpInventory( Pawn, "DumpInv" );
}

//-----------------------------------------------------------------------------

exec function Goodies( int First, int Last )
{
	local int ii;

	if( First < 0 )
		First = 0;
	if( Last >= ArrayCount(GoodiesList) )
		Last = ArrayCount(GoodiesList)-1;

	if( Pawn != None )
	{
		for( ii=First; ii<=Last; ii++ )
		{
			if( GoodiesList[ii] != "" )
			{
				// try shove it into player's inventory
				if( class'UtilGame'.static.GiveInventoryString( Pawn, GoodiesList[ii] ) != None )
				{
					DM( "Added " $ GoodiesList[ii] $ " to inventory" );
				}
				else
				{
					// spawn it on the ground
					SummonPackageClass( GoodiesList[ii] );
				}
			}
		}
	}
	else
	{
		DM( "Invalid Pawn: " $ Pawn );
	}
}

//-----------------------------------------------------------------------------

exec function TeamEnergy( int TeamNumber, float NewEnergyLevel )
{
	ServerSetTeamEnergy( TeamNumber, NewEnergyLevel );
}

//-----------------------------------------------------------------------------

exec function SetTeam( int NewTeam )
{
	Outer.SetTeam( NewTeam );
}

//-----------------------------------------------------------------------------

exec function CheatXMP()
{
	XMPGame(Level.Game).bCheatXMP = !XMPGame(Level.Game).bCheatXMP;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DebugInfoManagerClass=class'HitActorDebugInfoManager'
	ScoreboardInfoManagerClass=class'ScoreboardInfoManager'
	bTraceDebugEnabled=false
    SummonPackageStrings(0)="Engine"
    SummonPackageStrings(1)="U2"
    SummonPackageStrings(2)="Pawns"
    SummonPackageStrings(3)="Weapons"
    SummonPackageStrings(4)="Deployables"
    SummonPackageStrings(5)="Vehicles"
    SummonPackageStrings(6)=""
    SummonPackageStrings(7)="Legend"
    SummonPackageStrings(8)="ParticleSystems"
    GoodiesList( 0)="Weapons.AssaultRifle"
    GoodiesList( 1)="Weapons.EnergyRifle"
    GoodiesList( 2)="Weapons.FlameThrower"
    GoodiesList( 3)="Weapons.GrenadeLauncher"
    GoodiesList( 4)="Weapons.Pistol"
    GoodiesList( 5)="Weapons.RocketLauncherEx"
    GoodiesList( 6)="Weapons.Shotgun"
    GoodiesList( 7)="Weapons.SniperRifle"
    //GoodiesList( 8)="Weapons.weaponInvLaserRifle"
    //GoodiesList( 9)="Weapons.weaponInvLeechGun"
	//GoodiesList(10)="Weapons.weaponInvDispersion"
    //GoodiesList(11)="Weapons.weaponInvSingularityCannon"
    //GoodiesList(12)="Weapons.weaponInvTakkra"
    GoodiesList( 20)="Deployables.FieldGeneratorDeployable"
    GoodiesList( 21)="Deployables.RocketTurretDeployable"
    GoodiesList( 22)="Deployables.AutoTurretDeployable"
    GoodiesList( 23)="Deployables.AmmoPacks"
    GoodiesList( 24)="Deployables.EnergyPacks"
    GoodiesList( 25)="Deployables.HealthPacks"
    GoodiesList( 26)="Deployables.LandMineDeployable"
}
