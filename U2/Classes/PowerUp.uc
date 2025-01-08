//=============================================================================
// PowerUp.uc
//=============================================================================

class PowerUp extends Inventory
	abstract;

var() int       EnergyUnits;      // A pickup or station's number of EnergyUnits
var() int       HealthUnits;      // A pickup or station's number of HealthUnits
var() int		AmmoUnits;        // Percentage of total ammunition this powerup provides
var() int       TransferRate;     // The rate at which the units are passed to the PowerSuit or Player
							      // in terms of x units per second.
var() string    Description;	  // Popup description of this PowerUp
var   float     LastTransferTime; // Saves the time of the last Unit transfer
var   PowerSuit TargetPowerSuit;  // The PowerSuit that is being manipulated
var   Pawn      PowerUpOwner;     // The owner of the PowerSuit

//-----------------------------------------------------------------------------
// The following functions are explained in PowerStation

function ActivateStation( Pawn P );
function StopStation();
function DeactivateStation();
function MakeStationEmpty();
function OnUse( Actor Other );
function OnUnuse( Actor Other );
function bool IsUsable( Actor Other );

//-----------------------------------------------------------------------------
// Gets the current PowerSuit and records a time to reference the initial
// transfer from.

event PreBeginPlay()
{
	Super.PreBeginPlay();
	
	if( Pawn(Owner) != None )
	{
		PowerUpOwner = Pawn(Owner);
		TargetPowerSuit = PowerSuit(class'UtilGame'.static.GetInventoryItem( PowerUpOwner, class'PowerSuit' ));
	}
	LastTransferTime = Level.TimeSeconds;
}

//-----------------------------------------------------------------------------
// Checks to see if the player already has one of these items in his
// inventory. If he does, he receives an error message.

// FYI: False means good to pickup, true means do not pickup

function bool HandlePickupQuery( Pickup item )
{
	// duplicate powerups will merge their energies when given to player
	return false;
}

//-----------------------------------------------------------------------------
// Transfer units from a newly picked up PowerUp to a PowerUp currently held.

function TransferProperties( Pickup PickedUpItem )
{
	local PowerUpPickup P;

	P = PowerUpPickup(PickedUpItem);

	TransferRate = P.TransferRate;
	EnergyUnits  = P.EnergyUnits;
	HealthUnits  = P.HealthUnits;
	AmmoUnits    = P.AmmoUnits;
	Description  = P.Description;	
}

//-----------------------------------------------------------------------------

function GiveTo( pawn Other )
{
	local PowerUp	Inv;

	Inv = PowerUp(Other.FindInventoryType( Class ));
	if( Inv != None )
	{
		Inv.AddUnits( Self );
		Destroy();
	}
	else
	{
		super.GiveTo( Other );
	}
}

//-----------------------------------------------------------------------------
// Transfer units from a newly picked up PowerUp to a PowerUp currently held.
// PUI = PickedUpItem, shortened here to keep the line length readable.

function bool AddUnits( PowerUp PUI )
{
	local PowerSuit PS;
	local bool      bTransferred;
	local Pawn		P;

	P = Pawn(Owner);

	if( P != None && PUI != None )
	{
		// Must have a PowerSuit in order to pickup energy
		if( PUI.EnergyUnits > 0 )
		{
			PS = PowerSuit(class'UtilGame'.static.GetInventoryItem( P, class'PowerSuit' ));
			if( PS != None )
			{
				EnergyUnits += PUI.EnergyUnits;
				bTransferred = true;
			}
		}

		if( PUI.HealthUnits > 0 )
		{
			HealthUnits += PUI.HealthUnits;
			bTransferred = true;
		}

		if( PUI.AmmoUnits > 0 )
		{
			AmmoUnits += PUI.AmmoUnits;
			bTransferred = true;
		}
	}

	return bTransferred;
}

//-----------------------------------------------------------------------------
// Determines the amount of units to be transfered during this cycle

function float CalcPowerUp( int CalcUnits )
{
	local float ToTransfer;

	if( TransferRate == 0 )
	{
		//transfer all
		ToTransfer = CalcUnits;
	}
	else
	{
//DM( "@@@ " $ TransferRate $ " * " $ Level.TimeSeconds $ " - " $ LastTransferTime );
		if( LastTransferTime != 0 )
		{
			ToTransfer = TransferRate * (Level.TimeSeconds - LastTransferTime);
//DM( "@@@ ToTransfer = " $ ToTransfer );
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
// Manages the dispersal of powerup units

function TickPowerUp( out int UnitsAvailable, int MaxUnits, out int TargetsCurrentUnits )
{
	local float ToTransferPct;
	local int ToTransferUnits;

	// If max health has been reached
	if( TargetsCurrentUnits == MaxUnits )
		UnitsAvailable = 0;

	// Calculate
	ToTransferPct = CalcPowerUp( UnitsAvailable );

	// PowerUp units are ints, so wait until at least 1% can be resupplied
	if( int(ToTransferPct) > 0 )
	{
		// Convert the percentage of the max, into raw units
		ToTransferUnits = int(ToTransferPct * MaxUnits / 100.0);
		ToTransferUnits = Clamp( ToTransferUnits, 0, MaxUnits - TargetsCurrentUnits );

		// Transfer the power
		TargetsCurrentUnits += ToTransferUnits;
		// Remove power from powerup
		UnitsAvailable -= ToTransferPct;
		LastTransferTime = Level.TimeSeconds;
	}

	if( UnitsAvailable <= 0 )
		HandlePowerUpEmpty();
}

//-----------------------------------------------------------------------------

function HandleUserFulfilled();
function HandlePowerUpEmpty();

//-----------------------------------------------------------------------------

function CheckUserFulfulled( int MaxUnits, int TargetsCurrentUnits )
{
	if( TargetsCurrentUnits == MaxUnits )
		HandleUserFulfilled();
}

//-----------------------------------------------------------------------------
// Manages the dispersal of powerup units

event Tick(float DeltaTime)
{
	local int MaxPower;
	local int Power;
	local int OldPower;

	Super.Tick( DeltaTime );

	if( class'Pawn'.static.ValidPawn( PowerUpOwner ) )
	{
		if( EnergyUnits > 0 )
		{
			if( TargetPowerSuit != None )
			{
				MaxPower = int(TargetPowerSuit.MaxPower);
				Power = int(TargetPowerSuit.GetPower());
				TickPowerUp( EnergyUnits, MaxPower, Power );
				TargetPowerSuit.SetPower( Power );
				CheckUserFulfulled( MaxPower, Power );
			}
		}
		if( HealthUnits > 0 )
		{
			if( PowerUpOwner != None )
			{
				TickPowerUp( HealthUnits, PowerUpOwner.default.Health, PowerUpOwner.Health );
				CheckUserFulfulled( PowerUpOwner.default.Health, PowerUpOwner.Health );
			}
		}

		if( AmmoUnits > 0 )
		{
			if( PowerUpOwner != None && PowerUpOwner.PlayerReplicationInfo != None )
			{
				MaxPower = 100;
				Power = PowerUpOwner.PlayerReplicationInfo.AmmoEquipedPct * 100;
				OldPower = Power;
				TickPowerUp( AmmoUnits, MaxPower, Power );
				//DM("Ammo -- OldPct:" @ OldPower @ "New Power:" @ Power @ "added ammo pct:" @ float(Power - OldPower) / 100.0 );
				class'UtilGame'.static.AddAllAmmoPercent( PowerUpOwner, float(Power - OldPower) / 100.0 );
				CheckUserFulfulled( MaxPower, OldPower );
			}
		}
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bMergesCopies=true
}