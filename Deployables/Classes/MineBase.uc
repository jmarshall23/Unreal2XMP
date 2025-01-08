
class MineBase extends BreakableItem
	abstract;

//-----------------------------------------------------------------------------

var() ParticleGenerator ParticleHitEffect;		// The effect to spawn when an impact happens
var() ParticleSalamander ExplosionEffect;
var ParticleSalamander Explosion;
var() sound DeploySound;
var() sound ArmingSound;
var() sound TrippedSound;
var() sound AmbientNoiseSound;
var() float ArmingDelay;						// seconds needed, after landing, for the mine to be active
var() float ExplodeDelay;						// once tripped, seconds before exploding
var() float SelfDestructTimer, SelfDestructDuration;
var() float EnergyCostPerSec;
var int ClientTeamIndex;

var int ServerDamageCount, ClientDamageCount;
var vector ServerHitLocation;

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && ROLE == ROLE_Authority )
		ClientTeamIndex, ServerDamageCount, ServerHitLocation; // data replicated to client
}

//-----------------------------------------------------------------------------

simulated event PostNetReceive()
{
	//DMTN("PostNetReceive");
	if( ServerDamageCount > ClientDamageCount )
	{
		//DMTN("DamageCount has changed");
		ClientShowDamage( ServerHitLocation );
		ClientDamageCount = ServerDamageCount;
	}
}

//-----------------------------------------------------------------------------

simulated event PostBeginPlay()
{
	//DMTN( "In PostBeginPlay  Role: " $ EnumStr( enum'ENetRole', Role ) );
	
	Super.PostBeginPlay();
	if( Role < ROLE_Authority )
		ReplicationReadyCheck();

	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		Explosion = ParticleSalamander( class'ParticleGenerator'.static.CreateNew( self, ExplosionEffect, Location ) );
		//DMTN("Explosion effects created:" @ Explosion );
	}

	if( EnergyCostPerSec > 0 && Role == ROLE_Authority )
		XMPGame(Level.Game).RegisterConsumer( Self, EnergyCostPerSec );

	SetCollision(false,false,false);
}

//-----------------------------------------------------------------------------

simulated event Destroyed()
{
	//DMTN("Destroyed");
	RemoveAllTimers();
	Explode();
	if( EnergyCostPerSec > 0 && Role == ROLE_Authority )
		XMPGame(Level.Game).UnRegisterConsumer( Self );
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

simulated function SetupClient()
{
	//DMTN( "SetupClient" );
	SetTextures( Self );
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
		AddTimer( 'ReplicationReadyCheck', 0.05, false );
}

//-----------------------------------------------------------------------------

simulated function int GetTeam() { return ClientTeamIndex; }
simulated function SetTeam( int NewTeam )
{
	//DMTN( "SetTeam: " $ NewTeam );
	Super.SetTeam( NewTeam );
	ClientTeamIndex = NewTeam;
	CheckTeamEnergy();
}

//-----------------------------------------------------------------------------

function CheckTeamEnergy()
{
	if( !XMPGame(Level.Game).IsTeamFeatureOnline( Class.Name, GetTeam() ))
	{
		//DMTN("Spawned when team doesn't have enough energy -- going offline");
		bDamageable = true;
		GotoState( 'Offline' );
	}
}
	
//-----------------------------------------------------------------------------

function DestroyTimer();

//-----------------------------------------------------------------------------

interface function bool   HasUseBar( Controller User )		{ return false; }
interface function string GetDescription( Controller User ) { return ""; }

//-----------------------------------------------------------------------------

singular function BaseChange()
{
	// hack to preserve instigator (engine version can clear the instigator)
	// it's safer to work around it than to change the engine code without knowing the side effects
	local Pawn SavedInstigator;
	SavedInstigator = Instigator;
	Super.BaseChange();
	Instigator = SavedInstigator;
}

//-----------------------------------------------------------------------------

simulated function ClientShowDamage( vector HitLocation )
{
	local ParticleGenerator Particles;

	if( ParticleHitEffect != None )
	{
		Particles = class'ParticleGenerator'.static.CreateNew( Self, ParticleHitEffect, HitLocation );
		Particles.Trigger( Self, Instigator );
		Particles.ParticleLifeSpan = Particles.GetMaxLifeSpan() + Particles.TimerDuration;
	}
}

//-----------------------------------------------------------------------------

function TakeDamage( int Damage, Pawn InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	if( bPendingDelete || !bDamageable )
		return;

	if( Health > 0 )
	{
		if( Damage > 0 )
		{
			Health -= Damage;

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
		Instigator = InstigatedBy;
		if( XMPGame(Level.Game)? && InstigatedBy? )
			XMPGame(Level.Game).NotifyDeployableDestroyed( InstigatedBy.Controller, Self );
		Destroy();
	}
}

//-----------------------------------------------------------------------------

function NotifyTeamEnergyStatus( bool bEnabled )
{
	if( !bEnabled )
		GotoState( 'Offline' );
}

//-----------------------------------------------------------------------------

simulated function ClientExplosion()
{
	if( Explosion? )
	{
		Explosion.SetLocation( Location );
		Explosion.SetRotation( Rotation );
		Explosion.Trigger( Self, Instigator );
	}
}

//-----------------------------------------------------------------------------

simulated function HandleTripped( actor Other );

//-----------------------------------------------------------------------------

simulated function Explode()
{
	if( Level.NetMode != NM_DedicatedServer )
	{
		ClientExplosion();
	}

	if( Role == ROLE_Authority )
	{
		if( DamageAmount>0 )
		{
			HurtRadius( DamageAmount, DamageRadius, DamageType, Momentum, Location );
		}
	}
}

//-----------------------------------------------------------------------------

auto state Deployed
{
	function BeginState()
	{
		PlaySound( DeploySound );
	}
}

//-----------------------------------------------------------------------------

state Armed
{
	simulated event Touch(Actor Other)
	{
		if( Pawn(Other)? )
			HandleTripped( Other );
	}

	//-------------------------------------------------------------------------

	simulated function HandleTripped( actor Other )
	{
		PlaySound( TrippedSound );
		AmbientSound = None;
		AddTimer( 'DestroyTimer', ExplodeDelay );
		Disable('Touch');
	}

	//-------------------------------------------------------------------------

	simulated function DestroyTimer()
	{
		Destroy();
	}

	//-------------------------------------------------------------------------

	simulated function PostArmed()
	{
		AmbientSound = AmbientNoiseSound;
		bDamageable = true;
		SetCollision(true,false,false);
	}

	//-------------------------------------------------------------------------

	simulated function PreArmed()
	{
		PlaySound( ArmingSound ); 
	}

	//-------------------------------------------------------------------------

Begin:
	PreArmed();
	Sleep( ArmingDelay );
	PostArmed();
}

//-----------------------------------------------------------------------------

state Offline
{
	function OnUse( actor Other );

    //-------------------------------------------------------------------------

	event Timer()
	{
		local float Damage;
		Damage = default.Health;

		if( SelfDestructDuration > 0 )
			Damage /= SelfDestructDuration;

		// keep doing damage until unit is destroyed
		TakeDamage( Damage, None, vect(0,0,0), vect(0,0,0), class'Suicided' );
	}

    //-------------------------------------------------------------------------

	function NotifyTeamEnergyStatus( bool bEnabled )
	{
		if( bEnabled )		// back online
		{
			DamageAmount = default.DamageAmount;
			GotoState( 'Armed' );
		}
	}

	//-------------------------------------------------------------------------

	event BeginState()
	{
		DamageAmount = 0;
	}

	//-------------------------------------------------------------------------

Begin:
	Sleep( FRand() );		// so that all units don't appear in sync
	SetTimer( SelfDestructTimer, true );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	// assets
	ExplosionEffect=ParticleSalamander'Turret_Juggernaut_FX.ParticleSalamander0'
	ParticleHitEffect=ParticleGenerator'Impact_Metal_AR.ParticleSalamander0'

	// physical
	Health=15
	bStasis=false
	bDirectional=true
	bCollideActors=true
	bCollideWorld=true
	bBlockActors=false
	bBlockPlayers=false
	bBlockKarma=false
 	bActorShadows=false
	bWorldGeometry=false
	bIgnoreEncroachers=true
	bAlwaysRelevant=true
	bReplicateInstigator=true
	bNetNotify=true //enable PostNetReceive events

	ClientTeamIndex=255
	SelfDestructTimer=1.0
	SelfDestructDuration=8.0
}
