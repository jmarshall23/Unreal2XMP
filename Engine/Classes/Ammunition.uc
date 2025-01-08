//=============================================================================
// Ammunition: the base class of weapon ammunition
//
// This is a built-in Unreal class and it shouldn't be modified.
//=============================================================================

class Ammunition extends Inventory
	abstract
	native
	nativereplication;

var travel int MaxAmmo;						// Max amount of ammo
var travel int AmmoAmount;					// Amount of Ammo current available
var travel int PickupAmmo;					// Amount of Ammo to give when this is picked up for the first time	
var travel int ClipRoundsRemaining;
// Use interface functions to set the above properies: SetMaxAmmo, SetAmmoAmmount, SetClipRoundsRemaining
// The proper thing to do would be to make them const and use native functions to set the values.

var int ClipSize;	// automatically initialized based on AmmoAmount

var int AmmoUsed;	// ammo consumed per shot

var float	FireTime,
			FireLastReloadTime,
			FireLastDownTime,
			FireLastRoundTime;

var name	AnimFire,
			AnimFireEnd,
			AnimFireLastReload,
			AnimFireLastDown,
			AnimFireLastRound;

var float	ShakeMag,
			ShakeTime;

var range	FirePitch;
var sound	FireSound,
			FireEndSound,
			FireLastRoundSound;		// played for both FireLastReload (followed by ReloadUnloaded), and FireLastDown.

var bool bAmbientFireSound;

// Used by Bot AI

var		bool	bRecommendSplashDamage;
var		bool	bWarnTarget;
var		bool	bTossed;
var		bool	bTrySplash;
var		bool	bLeadTarget;
var		bool	bInstantHit;
var		bool	bSplashDamage;	

var float TraceSpread;

var bool bRepeatFire;	// automatic refire support
var bool bRapidFire;	// used by pawn animations in determining firing animation, and for net replication (set for high rate of fire weapons)

// Damage and Projectile information

var int Damage;
var float MomentumTransfer;
var class<Projectile> ProjectileClass;
var class<DamageType> MyDamageType;
var float WarnTargetPct;
var float RefireRate;

var float MaxAutoAimRange; 

var float EquipResidue;

// Squib classes

var() class<Actor>	BloodEffect,
					SparkEffect,
					DirtEffect;

// Network replication
//

replication
{
	// Things the server should send to the client.
	reliable if( bNetOwner && bNetDirty && (Role==ROLE_Authority) )
		AmmoAmount, ClipRoundsRemaining;
}

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();
	ClipSize = AmmoAmount;
	RefireRate = 1.0 / FireTime;
}

simulated function bool HasAmmo()
{
	return ( AmmoAmount > 0 );
}

function float RateSelf(Pawn Shooter, out name RecommendedFiringMode)
{
	return 0.5;
}

function WarnTarget(Actor Target,Pawn P ,vector FireDir)
{
	if ( bInstantHit )
		return;
	if ( (FRand() < WarnTargetPct) && (Pawn(Target) != None) && (Pawn(Target).Controller != None) ) 
		Pawn(Target).Controller.ReceiveWarning(P, ProjectileClass.Default.Speed, FireDir); 
}

function Projectile SpawnProjectile(vector Start, rotator Dir)
{
	return Spawn(ProjectileClass,Owner,, Start,Dir);	
}

function ProcessTraceHit(Actor Other, Vector HitLocation, Vector HitNormal)
{
	Other.TakeDamage( Damage, Pawn(Owner), HitLocation, -MomentumTransfer * HitNormal, MyDamageType );

	// Hit effects.
	if (Other.bHasBlood)
	{
		if (BloodEffect?)
			Spawn( BloodEffect,,, HitLocation, rotator(HitNormal) );
	}
	else if (Other.IsA('TerrainInfo'))
	{
		if (DirtEffect?)
			Spawn( DirtEffect,,, HitLocation, rotator(HitNormal) );
	}
	else
	{
		if (SparkEffect?)
			Spawn( SparkEffect,,, HitLocation, rotator(HitNormal) );
	}
}

simulated function DisplayDebug(Canvas Canvas, out float YL, out float YPos)
{
	Canvas.DrawText("Ammunition "$GetItemName(string(self))$" amount "$AmmoAmount$" Max "$MaxAmmo);
	YPos += YL;
	Canvas.SetPos(4,YPos);
}

simulated function ShakeView()
{
	local PlayerController P;
	if (Pawn(Owner)?)
		P = PlayerController(Pawn(Owner).Controller);
	if (P?)
	{
		if (ShakeMag>0 && ShakeTime>0)
			P.ShakeView(ShakeMag,ShakeTime);
	}
}
	
function bool HandlePickupQuery( pickup Item )
{
	if ( class == item.InventoryType ) 
	{
		if (AmmoAmount==MaxAmmo) 
			return true;
		item.AnnouncePickup(Pawn(Owner));
		AddAmmo(Ammo(item).AmmoAmount);
        item.SetRespawn(); 
		return true;				
	}
	if ( Inventory == None )
		return false;

	return Inventory.HandlePickupQuery(Item);
}

// Notify the player that an ammunition property has changed
function NotifyAmmoChanged()
{
	if (Pawn(Owner)?)
		Pawn(Owner).NotifyAmmoChanged();
}

// Interface functions for setting ammunition properties
simulated function SetAmmoAmount(int NewAmmoAmount)
{
	AmmoAmount = NewAmmoAmount;
	NotifyAmmoChanged();
}

simulated function SetClipRoundsRemaining( int NewClipRoundsRemaining )
{
	ClipRoundsRemaining = NewClipRoundsRemaining;
	NotifyAmmoChanged();
}

function SetMaxAmmo(int NewMaxAmmo)
{
	MaxAmmo = NewMaxAmmo;
	NotifyAmmoChanged();
}

// If we can, add ammo and return true.
// If we are at max ammo, return false
//

function bool AddAmmo(int AmmoToAdd)
{
	SetAmmoAmount( Clamp(AmmoAmount+AmmoToAdd, 0, MaxAmmo) );
	return true;
}

function Resupply( float AddedPercent )
{
	local int ResupplyMax;
	local float AddedAmmoAmount;

	// Special handling for items that can hold only one ammo (which gets put in their clip) and therefore have a MaxAmmo of zero.
	if( MaxAmmo == 0 && AmmoAmount == 0 )
	{
		EquipResidue += AddedPercent;
		if( EquipResidue >= 1.0 )
		{
			SetAmmoAmount(1);
			SetClipRoundsRemaining(1);
			EquipResidue = 0.0;
		}
		return;
	}

	ResupplyMax = MaxAmmo + ClipRoundsRemaining;
	if( AmmoAmount < ResupplyMax )
	{
		AddedAmmoAmount = AddedPercent * ResupplyMax + EquipResidue;
		AddedAmmoAmount = FClamp( AddedAmmoAmount, 0,  ResupplyMax );
		SetAmmoAmount( AmmoAmount + int(AddedAmmoAmount) );
		EquipResidue = AddedAmmoAmount - int(AddedAmmoAmount);
	}
}

function float GetDamageRadius()
{
	if ( ProjectileClass != None )
		return ProjectileClass.Default.DamageRadius;

	return 0;
}

simulated function bool CanReload()
{
	return ( AmmoAmount > ClipRoundsRemaining && ClipRoundsRemaining < ClipSize );
}

simulated function bool ReloadRequired( optional int AmountNeeded )
{
	if( AmountNeeded == 0 )
		AmountNeeded = AmmoUsed;
	return AmountNeeded > ClipRoundsRemaining;
}

//simulated so that the clip information will be correctly updated on the client
//	so that it will only play the reload animation once
simulated function Reload()
{
	SetClipRoundsRemaining( Min( ClipSize, AmmoAmount ) );

	if( Pawn(Owner)? && Pawn(Owner).IsRealPlayer() )
		PlayerController(Pawn(Owner).Controller).SendEvent("AmmoFlash");
}

simulated function bool UseAmmo( optional int AmountNeeded )
{
	if( AmountNeeded == 0 )
		AmountNeeded = AmmoUsed;
	if ((AmountNeeded <= ClipRoundsRemaining) && (AmmoAmount >= AmountNeeded))
	{
		SetAmmoAmount( AmmoAmount - AmountNeeded );
		SetClipRoundsRemaining( ClipRoundsRemaining - AmountNeeded );
		return true;
	}
	return false;
}

//NEW (mdf)
function Dump( coerce string PrependStr, coerce string AppendStr )
{
	Super.Dump( PrependStr, " AmmoAmount: " $ AmmoAmount $ " ClipRoundsRemaining: " $ ClipRoundsRemaining $ AppendStr );
}
//OLD

defaultproperties
{
	AmmoUsed=1
	FireTime=0.5
	FireLastReloadTime=1.0
	FireLastDownTime=1.0
	FireLastRoundTime=1.0
	ShakeMag=5.0
	ShakeTime=0.2
	FirePitch=(A=1,B=1)
	bRepeatFire=true
	Damage=10
	MomentumTransfer=10000
	MyDamageType=class'DamageType'
	WarnTargetPct=+0.5
	MaxAutoAimRange=5000.0

//	AnimFire='Fire'
//	AnimFireEnd='FireEnd'
//	AnimFireLastReload='FireLastReload'
//	AnimFireLastDown='FireLastDown'
//	AnimFireLastRound='FireLastRound'

//	AnimFire='AltFire'
//	AnimFireEnd='AltFireEnd'
//	AnimFireLastReload='AltFireLastReload'
//	AnimFireLastDown='AltFireLastDown'
//	AnimFireLastRound='AltFireLastRound'
}

