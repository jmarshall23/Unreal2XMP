//=============================================================================
// PowerStation.uc
//=============================================================================

class PowerStation extends PowerUp;

var() bool	  bLimited;					// If Limited, the station deactivates when units are consumed,
										// else stops working when  units are consumed, but will recharge
										// them over time.
var() float   RechargeRate;				// If !bLimited, rate at which units are recharged.
var() Sound   StationStartUsingSound;   // Pawn starts using the station
var() Sound   StationStopUsingSound;	// Pawn stops using the station
var() Sound   StationInUseSound;		// Station is being used by the pawn
var() Sound   StationErrorSound;		// Station cannot be used or stops
var() float   TeamEnergyCost;			// Energy drained from team energy when in use by a player
var   float   DelayTime;
var   float   LastRechargeTime;
var	  int     StartingEnergyUnits;
var   int     StartingHealthUnits;
var   int     StartingAmmoUnits;
var   CollisionProxy TouchingCollisionProxy;
var   float   TouchingHeight, TouchingRadius;

var() ParticleGenerator ParticleEffect;
var() vector StationEffectOffset;
var ParticleGenerator ActiveStationEffect;

//-----------------------------------------------------------------------------

event PreBeginPlay()
{
	Super.PreBeginPlay();

	if( !bLimited )
	{
		StartingHealthUnits = HealthUnits;
		StartingEnergyUnits = EnergyUnits;
		StartingAmmoUnits   = AmmoUnits;
		Disable('Tick'); // optimization
	}
	TouchingCollisionProxy = Spawn( class'CollisionProxy' );
	TouchingCollisionProxy.CP_SetCollision( true, false, false );
	TouchingCollisionProxy.CP_SetLocation( Location );
	TouchingCollisionProxy.CP_SetCollisionSize( TouchingRadius, TouchingHeight );
	TouchingCollisionProxy.CP_SetTouchTarget( Self );
	TouchingCollisionProxy.SetBase( Self );

}

//-----------------------------------------------------------------------------

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		if( ParticleEffect? )
		{
			ActiveStationEffect = class'ParticleGenerator'.static.CreateNew( self, ParticleEffect, Location + (StationEffectOffset << Rotation) );
			ActiveStationEffect.SetRotation( Rotation );
			ActiveStationEffect.TurnOn();
		}
	}
}

//-----------------------------------------------------------------------------

simulated event Destroyed()
{
	CleanupCollisionProxy( TouchingCollisionProxy );
	if( ActiveStationEffect? )
		ActiveStationEffect.Destroy();
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

function CleanupCollisionProxy( CollisionProxy CP )
{
	if( CP != None )
	{
		CP.Destroy();
		CP = None;
	}
}

//-----------------------------------------------------------------------------

function interface string GetDescription( Controller User )
{
	return Description;
}

//-----------------------------------------------------------------------------

function ClearProperties()
{
	AmbientSound = None;
	TargetPowerSuit = None;
	PowerUpOwner = None;
}

//-----------------------------------------------------------------------------
// This function occurs when a station is either empty, the pawn's units are
// full or potentially if the station is damaged (no determination on this yet).

function StationError()
{
	PlaySound( StationErrorSound, SLOT_None );
	ClearProperties();
	LastRechargeTime = Level.TimeSeconds;
//DM( "%%% " $ Self $ ".PowerStation.StationError() LastRechargeTime = " $ LastRechargeTime );
}

//-----------------------------------------------------------------------------
// Turn station off and set texture back to active

function SetStationActiveOff()
{
	ClearProperties();
	LastRechargeTime = Level.TimeSeconds;
//DM( "%%% " $ Self $ ".PowerStation.SetStationActiveOff() LastRechargeTime = " $ LastRechargeTime );
}

//-----------------------------------------------------------------------------
// Play in use sound, set ambient sound, change skin to in use and set the
// transfer time for the powerup calculation code.

function SetStationInUse()
{
	PlaySound( StationStartUsingSound, SLOT_None );
	AmbientSound = StationInUseSound;
	LastTransferTime = Level.TimeSeconds;
}

//-----------------------------------------------------------------------------

function MakeStationEmpty()
{
	StationError();
}

//-----------------------------------------------------------------------------

function StopStation()
{
	SetStationActiveOff();
	StationError();
}

//-----------------------------------------------------------------------------

function bool StationCanBeUsed( Pawn P )
{
	return( XMPGame(Level.Game).IsTeamFeatureOnline( 'PowerStation', P.GetTeam() ) &&
		    (  (HealthUnits > 0 && P.Health < P.default.Health)
			|| (EnergyUnits > 0 && TargetPowerSuit != None && TargetPowerSuit.GetPower() < TargetPowerSuit.MaxPower) 
			|| (AmmoUnits   > 0 && P.PlayerReplicationInfo != None && P.PlayerReplicationInfo.AmmoEquipedPct < 1.0) ) );
}

//-----------------------------------------------------------------------------
// Set up station for use

function ActivateStation( Pawn P )
{
	TargetPowerSuit = PowerSuit(class'UtilGame'.static.GetInventoryItem( P, class'PowerSuit' ));
	PowerUpOwner = P;

	if( StationCanBeUsed(P) )
	{
		SetStationInUse();
	}
	else // Container is full
	{
		StationError();
	}
}

//-----------------------------------------------------------------------------
// Resets a stations sounds and textures

function DeactivateStation()
{
	if( PowerUpOwner != None )
	{
		if( StationCanBeUsed(PowerUpOwner) )
		{
			SetStationActiveOff();
			PlaySound( StationStopUsingSound, SLOT_None );
		}
	}
}

//-----------------------------------------------------------------------------

function int CalcRecharge( int CalcUnits )
{
	local int ToTransfer;

	if( RechargeRate == 0 )
	{
		//recharge all
		ToTransfer = CalcUnits;
	}
	else
	{
		if( LastRechargeTime != 0 )
		{
			ToTransfer = RechargeRate * (Level.TimeSeconds - LastRechargeTime);
			if( ToTransfer > CalcUnits )
			{
				ToTransfer = CalcUnits;
			}
		}
		else
			ToTransfer = 0;
	}

	return ToTransfer;
}

//-----------------------------------------------------------------------------

event Touch( Actor Other )
{
	if( U2Pawn(Other) != None )
		ActivateStation( U2Pawn(Other) );
}

//-----------------------------------------------------------------------------

event UnTouch( Actor Other )
{
	if( U2Pawn(Other) != None )
		DeActivateStation();
}

//-----------------------------------------------------------------------------

event Tick(float DeltaTime)
{
	local float TimeSinceLastRecharge;

	Super.Tick( DeltaTime );

	if( !bLimited )
	{
		TimeSinceLastRecharge = Level.TimeSeconds - LastRechargeTime;
		if( TimeSinceLastRecharge >= DelayTime )
		{
			if( StartingEnergyUnits > 0 && EnergyUnits < StartingEnergyUnits && PowerUpOwner == None )
				EnergyUnits += CalcRecharge( StartingEnergyUnits - EnergyUnits );

			if( StartingHealthUnits > 0 && HealthUnits < StartingHealthUnits && PowerUpOwner == None )
				HealthUnits += CalcRecharge( StartingHealthUnits - HealthUnits );

			if( StartingAmmoUnits   > 0 && AmmoUnits   < StartingAmmoUnits   && PowerUpOwner == None )
				AmmoUnits += CalcRecharge( StartingAmmoUnits - AmmoUnits );

			LastRechargeTime = Level.TimeSeconds;
		}
	}
}

//-----------------------------------------------------------------------------

function TickPowerUp( out int UnitsAvailable, int MaxUnits, out int TargetsCurrentUnits )
{
	local int OldUnits;
	OldUnits = TargetsCurrentUnits;
	Super.TickPowerUp( UnitsAvailable, MaxUnits, TargetsCurrentUnits );
	if( PowerUpOwner? )
		XMPGame(Level.Game).DrainTeamEnergy( PowerUpOwner.GetTeam(), (TargetsCurrentUnits - OldUnits) * TeamEnergyCost, 'PowerStationTick' );
}

//-----------------------------------------------------------------------------

function HandleUserFulfilled()
{
	StopStation();
}

//-----------------------------------------------------------------------------

function HandlePowerUpEmpty()
{
	MakeStationEmpty();
}

//-----------------------------------------------------------------------------

defaultproperties
{
	StaticMesh=StaticMesh'XMPWorldItemsM.Items.Station_TD_001'
	StationEffectOffset=(X=0,Y=0,Z=16)
	PrePivot=(X=-20)
    bTravel=false
	bOnlyOwnerSee=false
	bHidden=false
	bAlwaysRelevant=true
	RemoteRole=ROLE_DumbProxy
	NetPriority=+1.4
	bBlockActors=false
	bBlockPlayers=false
	bBlockNonZeroExtentTraces=true
	bCanMantle=false
	bCollideWorld=true
	bCollideActors=false
	CollisionHeight=55
	CollisionRadius=80
	//!!MERGE bMustFace=false
	bFixedRotationDir=True
	bLimited=false
	RechargeRate=99999.000000
	DelayTime=1.000000
	TouchingHeight=55
	TouchingRadius=80
	TeamEnergyCost=0.0002
}