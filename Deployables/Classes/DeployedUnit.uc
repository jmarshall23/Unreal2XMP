class DeployedUnit extends StationaryPawn
	abstract;

#exec OBJ LOAD File=..\System\ParticleRes\Impact_Metal_AR.u
#exec OBJ LOAD File=..\Textures\XMPT.utx
#exec OBJ LOAD File=..\StaticMeshes\XMPM.usx
#exec OBJ LOAD File=..\Sounds\U2XMPA.uax

const NotifyDeployedTimerName = 'NotifyDeployedTimer';

//var() class<Pickup>	PickupClass;
var() class<DamageFilter> DamageFilterClass;
var() ParticleGenerator ParticleHitEffect;		// effect to spawn when an impact happens
var() class<ReplicatedEffect> ExplosionClass;	// class used to handle explosion effect, sound
var() Sound			ShutdownSound;
var() Sound			DestroyedAlertSound;
var() Sound			DeploySound;
var() Sound			DisabledSound;
var() Sound			EnabledSound;
var() Sound			AmbientNoiseSound;
var() localized string	Description;
var() float			DisableTime;				// Number of seconds the unit remains disabled.
var() bool			bEnabled;
var() Shader		DisabledSkin;				// Skin/Shader to use when unit is Disabled.
var() array<StaticMesh>	CarcassMesh;			// Mesh to spawn when unit is destroyed.
var	bool			bActive;					// Unit becomes active when it has eventLanded() and is on.
var	bool			bDeployed;					// Unit is deployed through NotifyDeployed()
var private float	LandedNotifyDeployedDelay;
var() class<Inventory> InventoryType;
var() localized string PickupMessage; // Human readable description when picked up.
var int ClientTeamIndex;
var Texture	SpamTexture0, SpamTexture1, SpamTexture255; 	// temp hack until we get skins for these

var int ServerDamageCount, ClientDamageCount;
var vector ServerHitLocation;

var() float EnergyCostPerSec;
var() float SelfDestructTimer, SelfDestructDuration;

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && ROLE == ROLE_Authority )
		ClientTeamIndex, ServerDamageCount, ServerHitLocation; // data replicated to client
}

//-----------------------------------------------------------------------------

simulated event PostBeginPlay()
{
	//DMTN( "In PostBeginPlay  Role: " $ EnumStr( enum'ENetRole', Role ) );
	
	Super.PostBeginPlay();
	if( Role < ROLE_Authority )
		ReplicationReadyCheck();

	if( EnergyCostPerSec > 0 && Role == ROLE_Authority )
		XMPGame(Level.Game).RegisterConsumer( Self, EnergyCostPerSec );
}

//-----------------------------------------------------------------------------

event Destroyed()
{
	//DMTNS( "Destroyed" ); 
	RemoveAllTimers(); // in case timer(s) somehow still active
	if( EnergyCostPerSec > 0 && Role == ROLE_Authority )
		XMPGame(Level.Game).UnRegisterConsumer( Self );
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

function bool IsActive() { return bActive; }

//-----------------------------------------------------------------------------

function bool IsEnabled() { return bEnabled; }

//-----------------------------------------------------------------------------

function SetActive( bool bVal ) { bActive = bVal; }

//-----------------------------------------------------------------------------

function SetEnabled( bool bVal ) { bEnabled = bVal; }

//-----------------------------------------------------------------------------

function bool IsUsable( Actor Other )
{
	local bool bRes;
	
	//DMTN( "IsUsable  Other: " $ Other );
	
	if( bDeleteMe )
	{
		//DMTN( "  return false (bDeleteMe)" );
		return false;
	}
	
	if( Other == None )
	{
		//DMTN( "  return true (Other==None)" );
		return true;
	}
	
	if( bEnabled )
	{
		bRes = ( Controller(Other) != None && SameTeam( Controller(Other).Pawn ) );
		//DMTN( "  return " $ bRes $ " (team check)  My Team: " $ GetTeam() $ "  Other Team: " $ Controller(Other).GetTeam() );
		return bRes;
	}
	return false;
}

//-----------------------------------------------------------------------------

function OnUse( Actor Other )
{
	local PlayerController PC;
	
	//DMTN( "OnUse   Other: " $ Other );
	
	if( IsUsable( Other ) )
	{
		// units are returned to the player's inventory when used if on the same team,
		// if not on the same team but bFriendlyToPlayerTeam, they are disabled.	
		//DM( "OnUse " $ Self $ " Team = " $ GetTeam() $ " " $ Other $ ".Team = " $ Controller(Other).Pawn.GetTeam() );
		
		PC = U2PlayerController(Other);
		if( PC != None &&
			U2Pawn(PC.Pawn) != None &&
			U2Pawn(PC.Pawn).CanPickupItem( Self ) )
		{
			ShutDownDeployedUnit( false, Controller(Other).Pawn );
		}
		else
		{
			DisableUnit();
		}
	}
}

//-----------------------------------------------------------------------------

function interface string GetDescription( Controller User )
{
	return default.Description $ " (" $ Health $ ")";
}

//-----------------------------------------------------------------------------

function Initialize( bool bAltActivate )
{
	PlaySound( DeploySound, SLOT_None );
	if( Level.NetMode != NM_DedicatedServer )
		SetTextures( Self );
	// if team doesn't have enough energy, put this unit offline
	if( !XMPGame(Level.Game).IsTeamFeatureOnline( class.name, GetTeam() ) )
		GotoState( 'Offline' );
}

//-----------------------------------------------------------------------------

function SetSkin( bool bDisabledSkin );

//-----------------------------------------------------------------------------

function DisableUnit()
{
	// 2003.01.29 (mdf) fix for adding timers (etc.) to invalid (e.g. deleted) actors
	if( bDeleteMe )
		return;

	//DM( "@@@ DisableUnit Disabling " $ Self );
	// Turn off collision
	DisableCollision();
	// Disable
	SetEnabled( false );
	// Disabled skin
	SetSkin( true );
	if( DisabledSound != None )
		PlaySound( DisabledSound, SLOT_None );
	AddTimer( 'EnableUnit', DisableTime, false );
}

//-----------------------------------------------------------------------------

function EnableUnit()
{
	if( bDeleteMe )
		return;
			
	EnableCollision();
	SetEnabled( true );
	SetSkin( false );
	
	if( EnabledSound != None )
		PlaySound( EnabledSound, SLOT_None );
}

//-----------------------------------------------------------------------------

function KillInventory()
{
	local Inventory NextInv;

	while( Inventory != None )
	{
		NextInv = Inventory.Inventory;
		Inventory.Destroy();
		Inventory = NextInv;
	}	
	Weapon = None;
}

//-----------------------------------------------------------------------------

function ShutDownDeployedUnit( bool bDestroyed, optional Pawn P )
{
	local DeployableInventory DInv;

	// if being used and turned into a pickup
	if( !bDestroyed )
	{
		// Add one of these to the player's inventory
		if( Health > 0 )
			PlaySound( ShutdownSound, SLOT_None );

		// Make sure the deployable item is in owner's inventory
		DInv = DeployableInventory(class'UtilGame'.static.GiveInventoryClass( P, InventoryType ));
		DInv.AddAmmo( 1 );

		/*
		!!ARL (mdf): tbd - too spammy?		
		// Give player a pickup message
		if( P != None && P.IsRealPlayer() )
		{
			//!! MERGE class'UIConsole'.static.BroadcastStatusMessage( Self, PickupMessage );
		}
		*/
		
		// Destroy myself
		if( !bDeleteMe )
			KillInventory();
		Controller.PawnDied( Self );
		//DMTN( "ShutDownDeployedUnit #1" );
		Destroy();
	}
	else	// else, being destroyed
	{		
		//DMTN( "CarcassMesh.Length = " $ CarcassMesh.Length );
		if( CarcassMesh.Length > 0 && Health <= 0 )
		{
			if( Level.NetMode != NM_DedicatedServer )
			{
				SetDrawType( DT_StaticMesh );
				SetStaticMesh( CarcassMesh[ Rand(CarcassMesh.Length) ] );
			}
			GotoState( 'Decaying' );
		}
		else
		{
			if( Health > 0 )
				PlaySound( ShutdownSound, SLOT_None );
			if( !bDeleteMe )
				KillInventory();
			Controller.PawnDied( Self );
			//DMTN( "ShutDownDeployedUnit #2" );
			Destroy();
		}
	}
}

//-----------------------------------------------------------------------------

function NotifyDeployed();

//-----------------------------------------------------------------------------

simulated function ClientShowDamage( vector HitLocation )
{
	local ParticleGenerator Particles;

	//DMTN( "ClientShowDamage" );
	if( ParticleHitEffect != None )
	{
		Particles = class'ParticleGenerator'.static.CreateNew( Self, ParticleHitEffect, HitLocation );
		Particles.Trigger( Self, Instigator );
		Particles.ParticleLifeSpan = Particles.GetMaxLifeSpan() + Particles.TimerDuration;
	}
}

//-----------------------------------------------------------------------------

simulated event PostNetReceive()
{
	Super.PostNetReceive();
	
	if( ServerDamageCount > ClientDamageCount )
	{
		ClientShowDamage( ServerHitLocation );
		ClientDamageCount = ServerDamageCount;
	}
	
	if( !bDeleteMe && Health <= 0 && DrawType != DT_StaticMesh )
	{
		// hack alert
		SetDrawType( DT_StaticMesh );
		SetStaticMesh( CarcassMesh[ Rand(CarcassMesh.Length) ] );
	}
}

//-----------------------------------------------------------------------------

function TakeDamage( int Damage, Pawn InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	//DMTN( "TakeDamage" );
	if( bPendingDelete )
		return;
		
	if( Health > 0 )
	{
		//DMTN( "Health > 0" );
		
		if( DamageFilterClass? ) 
		 	DamageFilterClass.static.ApplyFilter( Damage, momentum, DamageType );

		if( Damage > 0 )
		{			
			//DMTN( "Damage > 0" );
			
			Health -= Damage;

			//DMTN( "bPendingDelete:    " $ bPendingDelete );
			//DMTN( "ParticleHitEffect: " $ ParticleHitEffect );
			//DMTN( "Level.NetMode:     " $ EnumStr( enum'ENetMode', Level.NetMode ) );

			//if( !bPendingDelete && Level.NetMode != NM_DedicatedServer && ParticleHitEffect != None )
			if( Health > 0 )
			{
				if( Level.NetMode != NM_DedicatedServer )
				{
					ClientShowDamage( HitLocation );
				}
				else
				{
					ServerDamageCount++;
					ServerHitLocation = HitLocation;
				}
			}
		}
	}

	if( Health <= 0 )
	{
		//DMTN( "Dead" );
		AmbientSound = None;
		if( DestroyedAlertSound != None )
			PlaySound( DestroyedAlertSound, SLOT_None );
		if( XMPGame(Level.Game)? )
			XMPGame(Level.Game).NotifyDeployableDestroyed( InstigatedBy.Controller, Self );
		Explode();
		ShutDownDeployedUnit( true );
	}
}

//-----------------------------------------------------------------------------

function Explode()
{
	local ReplicatedEffect Explosion;
	
	if( !bPendingDelete )
	{
		if( ExplosionClass != None )
			Explosion = Spawn( ExplosionClass );
	}
}

//-----------------------------------------------------------------------------

function NotifyDeployedTimer()
{
	// sanity checks
	if( !bDeleteMe )
	{
		// DMTN( "NotifyDeployedTimer up  Physics: " $ EnumStr( enum'EPhysics', Physics ) );
		if( Physics != PHYS_Falling )
		{
			//AddActor( Self, ColorGreen() );
			bActive = true;
			bDeployed = true;
			SetPhysics( PHYS_None ); // don't want to leave these in PHYS_Walking (unnecessary overhead)
			NotifyDeployed();
		}
		else
		{
			AddTimer( NotifyDeployedTimerName, LandedNotifyDeployedDelay, false );
		}
	}
}

//-----------------------------------------------------------------------------

event Landed( vector HitNormal )
{
	//DMTN( "Landed" );
	//AddActor( Self, ColorYellow() );
	if( bDeleteMe )
		return;
	
	// wait until location "settles down" before sending out NotifyDeployed
	AddTimer( NotifyDeployedTimerName, LandedNotifyDeployedDelay, false );
}

//-----------------------------------------------------------------------------

static function DamageEncroacher( Pawn P, Pawn DamageInstigator )
{
	//if( !P.bDeleteMe && P.Health > -9999 && !SameTeam( P ) && !P.IsRealPlayer() )
	//mdf-tbd: damage *any* Pawn that is in the way?
	if( !P.bDeleteMe && P.Health > -9999 )
		P.TakeDamage( 10000, DamageInstigator, P.Location, vect(0,0,0), class'Zapped' );
}

//-----------------------------------------------------------------------------

function CheckForEncroachingPawns();

//-------------------------------------------------------------------------

function EnableCollision()
{
	SetCollision( true, true, true );
	CheckForEncroachingPawns();
}

//-----------------------------------------------------------------------------

function DisableCollision()
{
	SetCollision( false, false, false );
}

//-----------------------------------------------------------------------------

event bool EncroachingOn( Actor Other )
{
	DamageEncroacher( Pawn(Other), Pawn(Owner) );
	return false;
}

//-----------------------------------------------------------------------------

simulated function SetTeam( int NewTeam )
{
	//DMTN( "SetTeam: " $ NewTeam );
	Super.SetTeam( NewTeam );
	ClientTeamIndex = NewTeam;
}

//-----------------------------------------------------------------------------

simulated function SetTextures( Actor TargetActor )
{
	//DMTN( "SetTextures  Team: " $ ClientTeamIndex );
	
	if( AlternateSkins.Length == 2 && AlternateSkins[ 0 ] != None && AlternateSkins[ 1 ] != None )
	{
		//DMTN( "AlternateSkins[ 0 ]: " $ AlternateSkins[ 0 ] );
		//DMTN( "AlternateSkins[ 1 ]: " $ AlternateSkins[ 1 ] );
		class'UtilGame'.static.SetTeamSkin( TargetActor, ClientTeamIndex );
	}
	else if( ClientTeamIndex == 0 && SpamTexture0 != None )
		class'Util'.static.SpamTextures( TargetActor, SpamTexture0 );
	else if( ClientTeamIndex == 1 && SpamTexture1 != None )
		class'Util'.static.SpamTextures( TargetActor, SpamTexture1 );
	else if( SpamTexture255 != None )
		class'Util'.static.SpamTextures( TargetActor, SpamTexture255 );
}

//-----------------------------------------------------------------------------

simulated function SetupClient()
{
	//DMTN( "SetupClient" );
	SetTextures( Self );
}

//-----------------------------------------------------------------------------

simulated function bool ReplicationReady() 
{ 
	//DMTN( "ReplicationReady: " $ ClientTeamIndex ); 
	return ( ClientTeamIndex != 255 ); 
}

//-----------------------------------------------------------------------------

simulated function ReplicationReadyCheck()
{
	// hack: assume replication complete once changes from defaults
	if( ReplicationReady() )
		SetupClient();
	else
		AddTimer( 'ReplicationReadyCheck', 0.2, false );
}

//-----------------------------------------------------------------------------

function NotifyTeamEnergyStatus( bool bEnabled )
{
	if( !bEnabled )
		GotoState( 'Offline' );
}

//-----------------------------------------------------------------------------

state Offline
{
	function OnUse( actor Other );

    //-------------------------------------------------------------------------

	event Timer()
	{
		local float Damage;
		// keep doing damage until unit is destroyed

		Damage = default.Health;
		if( SelfDestructDuration > 0 )
			Damage /= SelfDestructDuration;
		TakeDamage( Damage, None, vect(0,0,0), vect(0,0,0), class'Suicided' );
	}

    //-------------------------------------------------------------------------

	function NotifyTeamEnergyStatus( bool bEnabled )
	{
		if( bEnabled )					// back online
			GotoState( 'Armed' );
	}

	//-------------------------------------------------------------------------

Begin:
	Sleep( FRand() );		// so that all units don't appear in sync
	SetTimer( SelfDestructTimer, true );
}

//-----------------------------------------------------------------------------

state Decaying
{
	ignores Bump, HitWall, HeadVolumeChange, PhysicsVolumeChange, Falling, BreathTimer;

	event Timer()
	{
		if ( !PlayerCanSeeMe() )
		{
			//DMTN( "Destroy in Decaying" );
			Destroy();
		}
		else
			SetTimer( 2.0, false );	
	}

	event BeginState()
	{
		if ( bTearOff && (Level.NetMode == NM_DedicatedServer) )
			LifeSpan = 1.0;
		else
			SetTimer( 18.0, false );
		bInvulnerableBody = true;
		if ( Controller != None )
		{
			//NEW (mdf) -- PawnDied should be called for non players too -- and let PawnDied decided whether to destroy the Controller
			// Not calling this also means that the Pawn isn't correctly unpossessed?
			Controller.PawnDied( Self );
		}
		SetEnabled( false );
		//DM( "@@@ Decaying.BeginState Disabling " $ Self );
		DisableCollision();
		KillInventory();
	}

Begin:
//	DM( "@@@-------------------------- Starting DecayingState" );
	Sleep(0.2);
	bInvulnerableBody = false;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	SpamTexture0=Texture'JLSSRed'
	SpamTexture1=Texture'JLSSBlue'
	SpamTexture255=Texture'JLSSGrey'

	Health=1000
	bCollideActors=true
	bCollideWorld=true
	bBlockActors=true
	bBlockPlayers=true
	bProjTarget=true
	bHidden=false
	bActive=true
	bDeployed=false
	bCanMantle=false
	bActorShadows=false
	LandedNotifyDeployedDelay=0.5
	DisableTime=3.000000
	Physics=PHYS_Falling // so always get Landed notification even for items placed on ground via editor
	ParticleHitEffect=ParticleGenerator'Impact_Metal_AR.ParticleSalamander0'
	DamageFilterClass=class'DamageFilterDeployed'
	bAnimByOwner=true
	bUsable=true
	TeamIndex=255
	ClientTeamIndex=255
	
	//networking
	bNetNotify=true //enable PostNetReceive events
	SelfDestructTimer=1.0
	SelfDestructDuration=8.0
}
