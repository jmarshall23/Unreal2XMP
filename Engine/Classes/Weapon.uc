//=============================================================================
// Parent class of all weapons.
// FIXME - should bReplicateInstigator be true???
//=============================================================================
class Weapon extends Inventory
	abstract
	native
	nativereplication;

#exec Texture Import File=Textures\Weapon.pcx Name=S_Weapon Mips=Off MASKED=1

//-----------------------------------------------------------------------------
// animation support
var(Firing) float	ReloadTime,
					ReloadUnloadedTime,
					SelectTime,
					SelectUnloadedTime,
					DownTime,
					DownUnloadedTime;

var float FireTimer;
var float TempTimer; //!!tbd

var name LastAnim;

//-----------------------------------------------------------------------------
// flashskin support
var() array<Material> FireSkins;	// The skin to show when we just fired, and want to make the gun appear brighter from the flash
var() array<Material> FireSkinsTP;	// The skin to show when we just fired, and want to make the gun appear brighter from the flash (thirdperson)
var array<Material> OldSkins;
var float FireSkinTime;		// Time we set the fireskin, so we can turn it off in the next tick (using a timer caused it to be turned off before we even saw it ;)

//-----------------------------------------------------------------------------
// Weapon ammo information:
var class<ammunition> AmmoName[2];	// Type of ammo used (primary/alternate)
var int PickupAmmoCount;			// Amount of ammo initially in pick-up item.
var ammunition AmmoType;			// Inventory Ammo being used (set when fired, ammo counts copied over to keep sync'd)
var travel ammunition AmmoTypes[2];	// Inventory Ammo to use (primary/alternate)
var travel int AmmoIndex;			// Maintain correct ammo mode across level transitions (so ammo counts don't get lost)

//-----------------------------------------------------------------------------
// Debug flags:
var		bool	bUpdateCrosshair;
var		bool	bUpdateLocation;
var		bool	bDrawWeapon;
var		bool	bClearZ;

//-----------------------------------------------------------------------------
// Weapon firing/state information:
var		bool	bPointing;		// Indicates weapon is being pointed
var		bool	bWeaponUp;		// Used in Active State
var		bool	bChangeWeapon;	// Used in Active State
var		bool	bCanThrow;		// if true, player can toss this weapon out
var		bool	bLastRound;		// just fired last round of current clip

var Weapon OldWeapon;

var		float	StopFiringTime;	// repeater weapons use this
var		int		AutoSwitchPriority;
var     vector	FireOffset;		// Offset from first person eye position for projectile/trace start
var		Powerups Affector;		// powerup chain currently affecting this weapon

var		float	TraceAccuracy;

var vector TraceFireStartLocation;
var vector ProjectileFireStartLocation;

//-----------------------------------------------------------------------------
// AI information
var		bool	bMeleeWeapon;   // Weapon is only a melee weapon
var		bool	bSniping;		// weapon useful for sniping
var		float	AimError;		// Aim Error for bots (note this value doubled if instant hit weapon)
var		float	AIRating;
var		float	CurrentRating;
var		float	TraceDist;		// how far instant hit trace fires go
var		float	MaxRange;		// max range of weapon for non-trace hit attacks
var		Rotator	AdjustedAim;

//-----------------------------------------------------------------------------
// Sound Assignments
var() sound	SelectSound,
			ReloadSound,
			ReloadUnloadedSound;

//-----------------------------------------------------------------------------
// messages
var() Localized string MessageNoAmmo;
var   Localized string WeaponDescription;
var   Color NameColor;	// used when drawing name on HUD

var() array<name> TargetableTypes;	// Types of classes we can target (for crosshair glows).
var string Crosshair;				// UI event to send when switching to the weapon to show the crosshair.
var bool bGlowing;					// Whether the crosshair is currently glowing.
var string LastTargetName;			// Last viewed target name (for TargetID).

var		bool	bSteadyToggle;
var		bool bForceFire, bForceAltFire;

var float DisplayFOV;		// FOV for drawing the weapon in first person view

//-----------------------------------------------------------------------------
// Network replication
replication
{
	// Things the server should send to the client.
	reliable if( bNetOwner && bNetDirty && (Role==ROLE_Authority) )
		AmmoType, AmmoIndex, AmmoTypes, Affector;

	// Functions called by server on client
	reliable if( Role==ROLE_Authority )
		ClientWeaponSet;

	// Functions called by client on server
	reliable if( Role<ROLE_Authority )
		ServerFire, ServerRapidFire, ServerStopFiring;
}

simulated function float AmmoStatus()
{
	local float Amount,Max;
	Amount = AmmoType.AmmoAmount;
	Max = AmmoType.MaxAmmo;
	if (Max > 0)
		return Amount / Max;
	else
		return Amount;
}

simulated function float ResupplyStatus()
{
	local float Amount,Max;
	Amount = AmmoType.AmmoAmount;
	Max = AmmoType.MaxAmmo + AmmoType.ClipRoundsRemaining;
	if (Max > 0)
		return Amount / Max;
	else
		return Amount + AmmoType.EquipResidue;
}

simulated function bool AddAmmo( int NewAmmo )				{ return AmmoType.AddAmmo( NewAmmo ); }
simulated function bool HasAmmo()							{ return AmmoType.HasAmmo(); }
simulated function bool ReloadRequired()					{ return AmmoType.ReloadRequired(AmmoTypes[0].AmmoUsed) && AmmoType.ReloadRequired(AmmoTypes[1].AmmoUsed); }

simulated function int GetAmmoAmount()						{ return AmmoType.AmmoAmount; }
simulated function int GetMaxAmmo()							{ return AmmoType.MaxAmmo; }
simulated function int GetClipAmount()						{ return AmmoType.ClipRoundsRemaining; }
simulated function int GetClipSize()						{ return AmmoType.ClipSize; }

simulated function SwitchAway()								{ Instigator.Controller.SwitchToBestWeapon(); }
simulated function bool SplashJump()						{ return false; }

//-----------------------------------------------------------------------------
// cheats

function AllAmmo()
{
	if (!AmmoType) return;
	AmmoType.SetAmmoAmount(999);
	AmmoType.SetMaxAmmo(999);
}

//-----------------------------------------------------------------------------
// AI Functions

function float RangedAttackTime()
{
	return 0;
}

function bool RecommendRangedAttack()
{ 
	return false;
}

function bool FocusOnLeader(bool bLeaderFiring)
{
	return false;
}

// fixme - make IsFiring() weapon state based
function bool IsFiring()
{
	return ((Instigator != None) && (Instigator.Controller != None)
		&& ((Instigator.Controller.bFire !=0) || (Instigator.Controller.bAltFire != 0)));
}

/* IncrementFlashCount()
Tell WeaponAttachment to cause client side weapon firing effects
*/
simulated function IncrementFlashCount()
{
	local WeaponAttachment TP;
	TP = WeaponAttachment(ThirdPersonActor);
	if (TP?)
	{
		TP.FlashCount++;
		TP.ThirdPersonEffects();
	}
}

simulated event PostNetBeginPlay()
{
	if ( Role == ROLE_Authority )
		return;

	LinkMesh(default.Mesh);
}

//=============================================================================
// Inventory travelling across servers.

// Fix ARL: Sync ammo counts before traveling since we 
event TravelPostAccept()
{
	local int i;
	Super.TravelPostAccept();
	if (!Pawn(Owner))
		return;
	for (i=0; i<2; i++)
	{
		if (AmmoName[i]?)
		{
			AmmoTypes[i] = Ammunition(Pawn(Owner).FindInventoryType(AmmoName[i]));
			if (!AmmoTypes[i])
			{
				AmmoTypes[i] = Spawn(AmmoName[i],Owner);	// Create ammo type required		
				Pawn(Owner).AddInventory(AmmoTypes[i]);		// and add to player's inventory
				AmmoTypes[i].SetAmmoAmount( PickUpAmmoCount ); 
				AmmoTypes[i].GotoState('');
			}
		}
	}
	AmmoType = AmmoTypes[AmmoIndex];
	if (self == Pawn(Owner).Weapon)
		BringUp();
	else GotoState('');
}

function SetAITarget(Actor T);

function Destroyed()
{
	Super.Destroyed();
	if( (Pawn(Owner)!=None) && (Pawn(Owner).Weapon == self) )
		Pawn(Owner).Weapon = None;
}

function GiveTo(Pawn Other)
{
	Super.GiveTo(Other);
	bTossedOut = false;
	Instigator = Other;
	GiveAmmo(Other);
	ClientWeaponSet(true);
}

//=============================================================================
// Weapon rendering
// Draw first person view of inventory
simulated event RenderOverlays( canvas Canvas )
{
	if ( Instigator == None || !Instigator.IsFirstPerson() )
		return;

	if (bUpdateCrosshair)
		UpdateCrosshair( Canvas );

	if (bUpdateLocation)
	{
		SetLocation( Instigator.Location + Instigator.CalcDrawOffset(self) );
		SetRotation( Instigator.GetViewRotation() );
	}

	if (bDrawWeapon)
		Canvas.DrawActor(self, false, bClearZ, DisplayFOV);

	if( Level.TimeSeconds - FireSkinTime > 0.2 )
		UnsetFireSkin();
}

interface function string GetTargetName()
{
	return LastTargetName;
}

simulated function UpdateCrosshair( Canvas Canvas )
{
	local Actor HitActor;
	local vector ViewLoc;
	local rotator ViewRot;
	local Pawn Target;

	HitActor = PlayerController(Instigator.Controller).PlayerSeesEx( TraceDist, 0.25, ViewLoc, ViewRot );
	HandleTargetDetails( HitActor, Canvas, ViewLoc, ViewRot );

	if( !bGlowing && HitActor? && IsTargetable(HitActor) )
	{
		Target = Pawn(HitActor);
		if( Target? && !Target.bPendingDelete && Target.PlayerReplicationInfo? && Target.PlayerReplicationInfo.Team? && Target.PlayerReplicationInfo.Team.TeamIndex != Instigator.PlayerReplicationInfo.Team.TeamIndex )
		{
			LastTargetName = Target.PlayerReplicationInfo.PlayerName;
			SendEvent("TargetIDOn");
		}

		bGlowing=true;
		SendEvent("Glow");
	}
	else if( bGlowing && (!HitActor || !IsTargetable(HitActor)) )
	{
		bGlowing=false;
		SendEvent("UnGlow");
		SendEvent("TargetIDOff");
	}
}

simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot );

simulated function bool IsTargetable( Actor A )
{
	local int i;

	if( A==Owner )
		return false;
	if( A.bHidden )
		return false;
	if( A.Physics == PHYS_None ) // don't target carcasses & such?
		return false;

	for( i=0; i<TargetableTypes.Length; i++ )
		if( A.IsA( TargetableTypes[i] ) )
			return true;

	return false;
}

/* GetRating()
returns rating of weapon based on its current configuration (no configuration change)
*/
function float GetRating()
{
	return RateSelf(); // don't do this if RateSelf() changes weapon configuration
}

simulated function float RateSelf()
{
    if ( !HasAmmo() )
        CurrentRating = -2;
	else if ( Instigator.Controller == None )
		return 0;
	else
		CurrentRating = Instigator.Controller.RateWeapon(self);
	return CurrentRating;
}

function float GetAIRating()
{
	return AIRating;
}

// return delta to combat style
function float SuggestAttackStyle()
{
	return 0.0;
}

function float SuggestDefenseStyle()
{
	return 0.0;
}

static function bool IsValidDefaultInventory( pawn PlayerPawn ) { return true; }

/* HandlePickupQuery()
If picking up another weapon of the same class, add its ammo.
If ammo count was at zero, check if should auto-switch to this weapon.
*/
function bool HandlePickupQuery( Pickup Item )
{
	local int OldAmmo, NewAmmo;
	local Pawn P;

	if (Item.InventoryType == Class)
	{
		if ( WeaponPickup(item).bWeaponStay && ((item.inventory == None) || item.inventory.bTossedOut) )
			return true;
		P = Pawn(Owner);
		if ( AmmoType != None )
		{
			OldAmmo = AmmoType.AmmoAmount;
			if ( Item.Inventory != None )
				NewAmmo = Weapon(Item.Inventory).PickupAmmoCount;
			else
				NewAmmo = class<Weapon>(Item.InventoryType).Default.PickupAmmoCount;
			if ( AmmoType.AddAmmo(NewAmmo) && (OldAmmo == 0) 
				&& (P.Weapon.class != item.InventoryType) )
					ClientWeaponSet(true);
		}
		Item.AnnouncePickup(Pawn(Owner));
		return true;
	}
	if ( Inventory == None )
		return false;

	return Inventory.HandlePickupQuery(Item);
}

//
// Change weapon to that specificed by F matching inventory weapon's Inventory Group.
simulated function Weapon WeaponChange( byte F, Weapon CurrentChoice )
{
	local Weapon CurrentWeapon;

	CurrentWeapon = Pawn(Owner).PendingWeapon;	// allow fast cycling

	if( CurrentWeapon == None )
		CurrentWeapon = Pawn(Owner).Weapon;

	if ( HasAmmo() )
	{
		if( InventoryGroup == F )
		{
			// no current choice, we're the best so far.
			if( !CurrentChoice && Self != CurrentWeapon )
				CurrentChoice = Self;

			// if current weapon isn't in this group, pick one with lowest position in group
			else if( !CurrentWeapon || CurrentWeapon.InventoryGroup != F )
			{
				if( /*Self.*/LessThan(CurrentChoice))
					CurrentChoice = Self;
			}
			// current weapon is in current group, pick next weapon after it
			else if ( !CurrentChoice || CurrentChoice.LessThan(CurrentWeapon) )
			{
				if ( /*Self.*/LessThan(CurrentChoice) ||				// Self		Choice	Weapon
					 /*Self.*/GreaterThan(CurrentWeapon) )				// Choice	Weapon	Self
					CurrentChoice = Self;
			}

			else if ( CurrentWeapon.LessThan(Self) &&					// Weapon	Self	Choice
					  /*Self.*/LessThan(CurrentChoice) )
				CurrentChoice = Self;
		}
	}

	if( Inventory? )
		return Inventory.WeaponChange( F, CurrentChoice );
	else
		return CurrentChoice;
}


// Compare our group and position within the group to another weapon
simulated function bool LessThan( Weapon Other )
{
	if( Other == None ) //!!ARL (mdf) this is happening when switching from DP to an XMP item for example
		return false;

	if( InventoryGroup < Other.InventoryGroup )
		return true;
	else if( InventoryGroup == Other.InventoryGroup )
	{
		if( GroupOffset < Other.GroupOffset )
			return true;
		// weapons with same tray/priority sort alphabetically
		// tbd - might not be the same as comparing their strings but should be consistent
		else if( GroupOffset == Other.GroupOffset )
			if( ItemName < Other.ItemName )
				return true;
	}
	return false;
}

simulated function bool GreaterThan( Weapon Other )
{
	if ( InventoryGroup > Other.InventoryGroup )
		return true;
	else if ( InventoryGroup == Other.InventoryGroup )
	{
		if( GroupOffset > Other.GroupOffset )
			return true;
		// weapons with same tray/priority sort alphabetically
		// tbd - might not be the same as comparing their strings but should be consistent
		else if( GroupOffset == Other.GroupOffset )
			if( ItemName > Other.ItemName )
				return true;
	}
	return false;
}

// Find the previous weapon (using the Inventory group and the position within the group (GroupOffset)
simulated function Weapon PrevWeapon(Weapon CurrentChoice, Weapon CurrentWeapon)
{
	if ( HasAmmo() )
	{
		if ( !CurrentChoice )
		{
			if ( CurrentWeapon != self )
				CurrentChoice = self;
		}
		else if ( /*Self.*/LessThan(CurrentWeapon) )
		{
			if ( CurrentChoice.LessThan(Self) ||					// Choice	Self	Weapon	
				 CurrentChoice.GreaterThan(CurrentWeapon) )			// Self		Weapon	Choice
				CurrentChoice = self;
		}
		else if ( CurrentWeapon.LessThan(CurrentChoice) &&			// Weapon	Choice	Self
				  CurrentChoice.LessThan(Self) )
			CurrentChoice = self;
	}
	if ( !Inventory )
		return CurrentChoice;
	else
		return Inventory.PrevWeapon(CurrentChoice,CurrentWeapon);
}

// Find the next weapon (using the Inventory group and the position within the group (GroupOffset)
simulated function Weapon NextWeapon(Weapon CurrentChoice, Weapon CurrentWeapon)
{
	if ( HasAmmo() )
	{
		if ( !CurrentChoice )
		{
			if ( CurrentWeapon != self )
				CurrentChoice = self;
		}
		else if ( CurrentChoice.LessThan(CurrentWeapon) )
		{
			if ( /*Self.*/LessThan(CurrentChoice) ||					// Self		Choice	Weapon
				 /*Self.*/GreaterThan(CurrentWeapon) )					// Choice	Weapon	Self
				CurrentChoice = self;
		}
		else if ( CurrentWeapon.LessThan(Self) &&					// Weapon	Self	Choice
				  /*Self.*/LessThan(CurrentChoice) )
			CurrentChoice = self;
	}
	if ( !Inventory )
		return CurrentChoice;
	else
		return Inventory.NextWeapon(CurrentChoice,CurrentWeapon);
}

function GiveAmmo( Pawn Other )
{
	local int i;
	for (i=0; i<2; i++)
	{
		if (AmmoName[i]?)
		{
			AmmoTypes[i] = Ammunition(Other.FindInventoryType(AmmoName[i]));
			if (AmmoTypes[i]?)
			{
				if (i == 0)	// if primary and alt share the same ammo, don't give pickup ammo twice.
					AmmoTypes[i].AddAmmo(PickUpAmmoCount);
			}
			else
			{
				AmmoTypes[i] = Spawn(AmmoName[i],Owner);	// Create ammo type required		
				Other.AddInventory(AmmoTypes[i]);			// and add to player's inventory
				AmmoTypes[i].SetAmmoAmount( PickUpAmmoCount ); 
			}
			AmmoTypes[i].Reload();
		}
	}
	AmmoType = AmmoTypes[AmmoIndex];
}

// Return the switch priority of the weapon (normally AutoSwitchPriority, but may be
// modified by environment (or by other factors for bots)
simulated function float SwitchPriority() 
{
	if ( !Instigator.IsHumanControlled() )
		return RateSelf();
	else if ( !HasAmmo() )
	{
		if ( Pawn(Owner).Weapon == self )
			return -0.5;
		else
			return -1;
	}
	else 
		return default.AutoSwitchPriority;
}

// Compare self to current weapon.  If better than current weapon, then switch
simulated function ClientWeaponSet(bool bOptionalSet)
{
	local weapon W;

	Instigator = Pawn(Owner); //weapon's instigator isn't replicated to client
	if ( Instigator == None )
	{
		GotoState('PendingClientWeaponSet');
		return;
	}
	else if ( IsInState('PendingClientWeaponSet') )
		GotoState('');
	if ( Instigator.Weapon == self )
		return;

	if ( Instigator.Weapon == None )
	{
		Instigator.PendingWeapon = self;
		Instigator.ChangedWeapon();
		return;	
	}
	if ( bOptionalSet && (Instigator.IsHumanControlled() && PlayerController(Instigator.Controller).bNeverSwitchOnPickup) )
		return;
	if ( Instigator.Weapon.SwitchPriority() < SwitchPriority() ) 
	{
		W = Instigator.PendingWeapon;
		//if( W.SwitchPriority() > SwitchPriority() )
		if( W? && W.SwitchPriority() > SwitchPriority() ) //!!ARL (mdf) warning fix?
			return;
		Instigator.PendingWeapon = self;
		GotoState('');

		if ( !Instigator.Weapon.PutDown() )
			Instigator.PendingWeapon = W;
		return;
	}
	GotoState('');
}

simulated function Weapon RecommendWeapon( out float rating )
{
	local Weapon Recommended;
	local float oldRating;

	if ( Instigator.IsHumanControlled() )
		rating = SwitchPriority();
	else
	{
		rating = RateSelf();
		if ( (self == Instigator.Weapon) && (Instigator.Controller.Enemy != None) && HasAmmo() )
			rating += 0.21; // tend to stick with same weapon
			rating += Instigator.Controller.WeaponPreference(self);
	}
	if ( inventory != None )
	{
		Recommended = inventory.RecommendWeapon(oldRating);
		if ( (Recommended != None) && (oldRating > rating) )
		{
			rating = oldRating;
			return Recommended;
		}
	}
	return self;
}


//**************************************************************************************
//
// Firing helper functions
//

function float GetProjSpeed( bool bAlt )
{
	if( bAlt )
		return AmmoTypes[1].ProjectileClass.default.Speed;
	else
		return AmmoTypes[0].ProjectileClass.default.Speed;
}

simulated function vector GetFireStart(vector X, vector Y, vector Z)
{
	return (Instigator.Location + Instigator.EyePosition() + FireOffset.X * X + FireOffset.Y * Y + FireOffset.Z * Z); 
}

function Projectile ProjectileFire()
{
	local Projectile SpawnedProjectile;
	
	AdjustedAim = Instigator.GetAimRotation();
	SpawnedProjectile = AmmoType.SpawnProjectile(ProjectileFireStartLocation,AdjustedAim);

	//AddArrow( ProjectileFireStartLocation, ProjectileFireStartLocation + 8192*vector(AdjustedAim), ColorBlue() );
	
	return SpawnedProjectile;
}

function TraceFire( float TraceSpread )
{
	local vector End, SpreadVector;
	local vector HitLocation, HitNormal;
	local actor Other;

	AdjustedAim = Instigator.GetAimRotation();
	SpreadVector = vector(AdjustedAim + rotator(RandomSpreadVector( TraceSpread )));
	End = TraceFireStartLocation + TraceDist * SpreadVector;

	//AddArrow( TraceFireStartLocation, TraceFireStartLocation + 8192*vector(AdjustedAim), ColorGreen() );
	//AddArrow( TraceFireStartLocation, End, ColorRed() );
	
	Other = Trace(HitLocation,HitNormal,End,TraceFireStartLocation,true);	// Fix ARL: Maybe use TraceRecursive?
	if( !Other )
	{
		HitLocation = End;
		HitNormal = -SpreadVector;
	}
	HandleTraceImpact(Other,HitLocation,HitNormal);
}

function HandleTraceImpact( Actor Other, vector HitLocation, vector HitNormal )
{
	if( Other? )
		AmmoType.ProcessTraceHit(Other,HitLocation,HitNormal);
}

//-----------------------------------------------------------------------------
// Aiming
//-----------------------------------------------------------------------------

function bool AdjustTraceAim( float projSpeed, vector projStart, float FireSpread, bool bLeadTarget, bool bWarnTarget, bool bTossed, bool bTrySplash )
{
	return Instigator.AdjustAim( None, projSpeed, projStart, FireSpread, false, false, false, false );
}

function bool AdjustProjectileAim( bool bAltFire, class<Projectile> ProjectileClass, vector projStart, float FireSpread, bool bLeadTarget, bool bWarnTarget, bool bTossed, bool bTrySplash )
{
	return Instigator.AdjustAim( ProjectileClass, GetProjSpeed( bAltFire ), projStart, FireSpread, bLeadTarget, bWarnTarget, bTossed, bTrySplash );
}

function bool AdjustAim( bool bAltFire, bool bInstant, float FireSpread, class<projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	local bool bResult;
/*!!MERGE	
	// test for situation which doesn't require aiming (e.g. takkra alt fire)	
	if( !bAltFire && !bAimFire )
		bResult = true;
	else if( bAltFire && !bAimAltFire )
		bResult = true;
	else
*/
	if( bInstant )
		bResult = AdjustTraceAim( 1000000, TraceFireStartLocation, FireSpread, false, false, false, false );
	else
		bResult = AdjustProjectileAim( bAltFire, ProjClass, ProjectileFireStartLocation, FireSpread, true, bWarnTarget, ProjClass? && (ProjClass.default.Physics == PHYS_Falling), bTrySplash );
	return bResult;	
}

//-----------------------------------------------------------------------------
// Sets Trace/ProjectileFireStartLocations for next shot. NOTE: sets both since
// some weapons (e.g. LR) do both a trace and projectile fire.

function SetFireStartLocations()
{
	// trace from weapon's fireoffset (relative to Pawn) for projectile weapons
	if( Instigator.IsFirstPerson() )
	{
		// Fix ARL: Adjust for weapon bone attachments!!
		// trace from eyes for hitscan weapons (for players in 1st person only?)
		TraceFireStartLocation = Instigator.Location;
		TraceFireStartLocation.Z += Instigator.BaseEyeHeight;
		ProjectileFireStartLocation = class'UtilGame'.static.GetRotatedFireStart( Instigator, Instigator.Location, Instigator.GetViewRotation(), FireOffset /*+ FirstPersonOffset*/ );
	}
	else
	{
		ProjectileFireStartLocation = class'UtilGame'.static.GetRotatedFireStart( Instigator, Instigator.GetViewLocation(), Instigator.GetViewRotation(), FireOffset );
		TraceFireStartLocation = ProjectileFireStartLocation;
	}
}

/*-----------------------------------------------------------------------------
Most weapons use the default PreSetAimingParameters behavior (when owner begins 
firing the weapon, set fire start location, then set owner's aiming rotation 
for use when the weapon is actually fired. If false is returned, abort firing 
the weapon. 

The DP alt-fire fires its projectile when the owner *stops* alt-firing and in
this case the weapon is aimed just prior to spawning the projectile (at that
point it is too late to abort firing the weapon).
*/

function bool PreSetAimingParameters( bool bAltFire, bool bInstant, float FireSpread, class<projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	local bool bResult;
	SetFireStartLocations();
	bResult = AdjustAim( bAltFire, bInstant, FireSpread, ProjClass, bWarnTarget, bTrySplash );
	return bResult;
}

function PostSetAimingParameters( bool bAltFire, bool bInstant, float FireSpread, class<projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	// override to have this function do anything useful
}

simulated function SetAmmoType( int i )
{
	if( i == AmmoIndex )
		return;
	AmmoIndex = i;
	AmmoTypes[AmmoIndex].SetAmmoAmount( AmmoType.AmmoAmount );
	AmmoTypes[AmmoIndex].SetClipRoundsRemaining( GetClipAmount() );
	AmmoType = AmmoTypes[AmmoIndex];
}

//**************************************************************************************
//
// Firing
//

simulated function Fire()
{
	ExecuteFire();
}
simulated function AltFire()
{
	ExecuteFire(true);
}

function ServerFire(optional bool bAltFire)
{
	if( AmmoType.ReloadRequired(0) )	// sometimes server hasn't quite finished reloading yet, but client has so update ammo anyway so we don't misfire.
		AmmoType.Reload();
	ExecuteFire(bAltFire);
}
function ServerRapidFire(optional bool bAltFire)
{
	ServerFire(bAltFire);
	if( IsInState('Firing') )
		StopFiringTime = Level.TimeSeconds + 0.6;
}
function ServerStopFiring()
{
	StopFiringTime = Level.TimeSeconds;
}

simulated function ExecuteFire(optional bool bAltFire)
{
	if (!bAltFire)
		SetAmmoType(0);
	else
		SetAmmoType(1);

	if( Level.NetMode==NM_Client )
	{
		if( !AmmoType.bRapidFire )
			ServerFire(bAltFire);
		else if( StopFiringTime < Level.TimeSeconds + 0.3 )
		{
			StopFiringTime = Level.TimeSeconds + 0.6;
			ServerRapidFire(bAltFire);
		}
	}

/* -- bFire not replicated to server fast enough.  Maybe check bPlayerCanFire directly?
	//ensures that we can control who can fire and who cannot, based upon the function. bPlayerCanFire is now useful. :)
	if( AmmoIndex==0 && !Instigator.PressingFire() )
		return;
	if( AmmoIndex==1 && !Instigator.PressingAltFire() )
		return;
*/
/*!!MERGE
	if( bDisableFiring )
		return;
*/
	if( !AmmoType.ReloadRequired(0) )
	{
		// firing of the weapon can be aborted by the aiming code (e.g. due to obstacles)
		if( (Role < ROLE_Authority) || PreSetAimingParameters( (AmmoIndex==1), AmmoType.bInstantHit, AmmoType.TraceSpread, AmmoType.ProjectileClass, AmmoType.bWarnTarget, AmmoType.bRecommendSplashDamage ) )
		{
			if (Role == ROLE_Authority)
				AuthorityFire();
			EverywhereFire();
		}
	}
	else if ( HasAmmo() )
		Reload();
/*!!MERGE
	else if( HasAnyAmmo() )
		ChangeAmmoType();
*/
	else if( Instigator.ShouldAutoSwitch() )
		SwitchAway();
}

function AuthorityFire()
{
/*!!MERGE -- alt fire also?
	if( !IsInState('Firing') )
		Owner.CurrentStatus.NumFires = (Owner.CurrentStatus.NumFires+1) % 4;
*/
	bPointing = true;
/* SERVOTBD
	if( bRapidFire || FiringSpeed > 0 )
		Instigator.PlayRecoil( FiringSpeed );
*/
	// does nothing for most weapons unless shot fired when owner stops firing
	PostSetAimingParameters( (AmmoIndex==1), AmmoType.bInstantHit, AmmoType.TraceSpread, AmmoType.ProjectileClass, AmmoType.bWarnTarget, AmmoType.bRecommendSplashDamage );

	//AmmoType.UseAmmo();
	Instigator.MakeNoise( Instigator.SoundDampening );

	if( AmmoType.bInstantHit )
		TraceFire( AmmoType.TraceSpread );
	else if( AmmoType.ProjectileClass? )
		ProjectileFire();
}

simulated function EverywhereFire()
{
	AmmoType.UseAmmo();	// moved here from AuthorityFire so client knows when to play FireLastAnims.

	GotoState('Firing');

	if( Level.NetMode != NM_DedicatedServer )
	{
		SetFireSkin();
/*!!MERGE
		if( ShellClass? )
			EjectShell( ShellClass );
*/
		AmmoType.ShakeView();
	}
	if( Affector? )
		Affector.FireEffect();
	PlayFiring();
}

simulated function PlayFiring()
{
	bLastRound = AmmoType.ReloadRequired(1);

	if( !bLastRound )
	{
		PlayAnimEx(AmmoType.AnimFire);
		FireTimer = AmmoType.FireTime;
		if(!AmmoType.bAmbientFireSound)
			PlayWeaponSound(AmmoType.FireSound,GetRand(AmmoType.FirePitch));
	}
	else if( HasAmmo() )
	{
		PlayAnimEx(AmmoType.AnimFireLastReload);
		FireTimer = AmmoType.FireTime;
		ReloadTime = AmmoType.FireLastReloadTime - AmmoType.FireTime;
		PlayWeaponSound(AmmoType.FireLastRoundSound);
	}
	else if( Instigator.ShouldAutoSwitch() )
	{
		PlayAnimEx(AmmoType.AnimFireLastDown);
		FireTimer = AmmoType.FireLastDownTime - DownTime;
		PlayWeaponSound(AmmoType.FireLastRoundSound);
	}
	else
	{
		PlayAnimEx(AmmoType.AnimFireLastRound);
		FireTimer = AmmoType.FireLastRoundTime;
		PlayWeaponSound(AmmoType.FireLastRoundSound);
	}

	IncrementFlashCount();
	PawnAgentEvent('Fire');
}

simulated state Firing
{
ignores Fire,AltFire;

	simulated event BeginState()
	{
		if( AmmoType.bAmbientFireSound ) //if we are a rapid-fire weapon, then make our FireSound be the ambient sound
			Instigator.AmbientSound = AmmoType.FireSound;
	}
	simulated event EndState()
	{
		if( AmmoType.bAmbientFireSound )
			Instigator.AmbientSound = None;
	}

Begin:
	//incoming animation set by EverywhereFire(). We wait for it to finish
	TempTimer=FireTimer;//!!
	Sleep(FireTimer);
	if( AmmoType.bRepeatFire )
	{
		//!!ARL - bFire not replicated to server any longer -- use Steve's ServerRapidFire method in UW2110 (StopFiringTime, etc.).
		if (AmmoIndex==0 && ((Role == ROLE_Authority && StopFiringTime > Level.TimeSeconds) || Instigator.PressingFire()))
			Global.Fire();		// if successful, this will put us back to the [Begin] label.
		if (AmmoIndex==1 && ((Role == ROLE_Authority && StopFiringTime > Level.TimeSeconds) || Instigator.PressingAltFire()))
			Global.AltFire();	// if successful, this will put us back to the [Begin] label.
		if (AmmoType.bRapidFire && !bPendingDelete)
			ServerStopFiring();

		//maybe wind-down our weapon...
		if (LastAnim == AmmoType.AnimFire)
		{
			PlayAnimEx(AmmoType.AnimFireEnd);
			if (AmmoType.FireEndSound?)
				PlayWeaponSound(AmmoType.FireEndSound);
		}
	}

	if( AmmoType.ReloadRequired(0) )
	{
		if( HasAmmo() )
			Reload();
/*!!MERGE
		else if( HasAnyAmmo() )
			ChangeAmmoType();
*/
		else if( Instigator.ShouldAutoSwitch() )
			SwitchAway();
	}

	GotoState('Idle');
}

//**************************************************************************************
//
// Reloading
//

//this is called to reload the gun, and can be called at any point in time
//it will correctly deal with reloading from the loaded and unloaded states
simulated function Reload()
{
	if (AmmoType.HasAmmo() && AmmoType.CanReload())
	{
		if (!AmmoType.ReloadRequired(1)) //if we are not completely out of ammo, then drop our current clip
		{
			//OutOfAmmo
		}

		//go to the state for the duration of the reload
		GotoState('Reloading');
	}
}

simulated state Reloading
{
ignores Fire,AltFire,Reload;

	function bool CanFire() { return false; }

	simulated event BeginState()
	{
	}
	simulated event EndState()
	{
		ReloadTime=default.ReloadTime;
		bLastRound=false;
	}

	simulated function bool PutDown()
	{
		if (Global.PutDown())
		{
			GotoState('DownWeapon');
			return true;
		}
		return false;
	}

Begin:
	if (bChangeWeapon)
		GotoState('DownWeapon');
	PlayReloading();
	TempTimer=ReloadTime;//!!
	Sleep(ReloadTime);
	AmmoType.Reload();
	GotoState('Idle');
}

//**************************************************************************************
//
// Idle
//

simulated state Idle
{
	simulated function bool PutDown()
	{
		if (Global.PutDown())
		{
			GotoState('DownWeapon');
			return true;
		}
		return false;
	}

Begin:
	if (bChangeWeapon)
		GotoState('DownWeapon');
	PlayIdleAnim();
	if (Instigator.PressingFire() && (AmmoTypes[0] == None || AmmoTypes[0].bRepeatFire) )
		Fire();
	if (Instigator.PressingAltFire() && (AmmoTypes[1] == None || AmmoTypes[1].bRepeatFire) )
		AltFire();
	bPointing = false;
}

/* Active
Bring newly active weapon up.
The weapon will remain in this state while its selection animation is being played (as well as any postselect animation).
While in this state, the weapon cannot be fired.
*/
simulated state Active
{
ignores Fire,AltFire;

	simulated function bool PutDown()
	{
		local name anim;
		local float frame,rate;
		GetAnimParams(0,anim,frame,rate);
		if ( bWeaponUp || (frame < 0.75) )
			GotoState('DownWeapon');
		else
			bChangeWeapon = true;
		return True;
	}

	simulated function BeginState()
	{
		Instigator = Pawn(Owner);
		bForceFire = false;
		bForceAltFire = false;
		bWeaponUp = false;
		bChangeWeapon = false;
	}

	simulated function EndState()
	{
		bForceFire = false;
		bForceAltFire = false;
	}

Begin:
	TempTimer=SelectTime;//!!
	Sleep(SelectTime);
	if( bChangeWeapon )
		GotoState('DownWeapon');
	bWeaponUp = true;
	if( HasAmmo() && ReloadRequired() )
		Reload();
	GotoState('Idle');
}

/* DownWeapon
Putting down weapon in favor of a new one.  No firing in this state
*/
simulated State DownWeapon
{
ignores Fire,AltFire;

	simulated function bool PutDown()
	{
		return true; //just keep putting it down
	}

	simulated function BeginState()
	{
		OldWeapon = None;
		bChangeWeapon = false;
		TweenDown();
	}

Begin:
	Sleep(DownTime);
	Instigator.ChangedWeapon();
	GotoState('');
}

/* PendingClientWeaponSet
Weapon on network client side may be set here by the replicated function ClientWeaponSet(), to wait,
if needed properties have not yet been replicated.  ClientWeaponSet() is called by the server to 
tell the client about potential weapon changes after the player runs over a weapon (the client 
decides whether to actually switch weapons or not.
*/
State PendingClientWeaponSet
{
	simulated function Timer()
	{
		if ( Pawn(Owner) != None )
			ClientWeaponSet(false);
	}

	simulated function BeginState()
	{
		SetTimer(0.05, true);
	}

	simulated function EndState()
	{
		SetTimer(0.0, false);
	}
}

simulated function BringUp(optional weapon PrevWeapon)
{
	if ( (PrevWeapon != None) && PrevWeapon.HasAmmo() )
		OldWeapon = PrevWeapon;
	else
		OldWeapon = None;
	if ( Instigator.IsHumanControlled() )
	{
		LinkMesh(default.Mesh);
		PlayerController(Instigator.Controller).EndZoom();
	}	
	bWeaponUp = false;
	PlaySelect();
	GotoState('Active');
}

simulated function bool PutDown()
{
	bChangeWeapon = true;
	return true; 
}

simulated function TweenDown()
{
	local name Anim;
	local float frame,rate;

	if ( IsAnimating() && (AnimIsInGroup(0,'Select') || AnimIsInGroup(0,'SelectUnloaded')) )
	{
		GetAnimParams(0,Anim,frame,rate);
		TweenAnim( Anim, frame * 0.4 );
	}
	else if( !bLastRound )
	{
		if (ReloadRequired())
		{
			PlayAnim('DownUnloaded', 1.0, 0.05);
			DownTime = default.DownUnloadedTime;
		}
		else
		{
			PlayAnim('Down', 1.0, 0.05);
			DownTime = default.DownTime;
		}
	}
	SendEvent("HideCrosshair");
}

simulated function PlayReloading()
{
	SendEvent("WeaponFlash");
	PawnAgentEvent('Reload');
	if( !bLastRound )
	{
		PlayAnimEx('Reload');
		PlayWeaponSound(ReloadSound);
	}
	else
	{
		if( LastAnim!=AmmoType.AnimFireLastReload )
		{
			PlayAnimEx('ReloadUnloaded');
			if( default.ReloadUnloadedTime!=0.0 )
				ReloadTime=default.ReloadUnloadedTime;
		}
		PlayWeaponSound(ReloadUnloadedSound);
	}
}

simulated function PlaySelect()
{
	bForceFire = false;
	bForceAltFire = false;
	if ( !IsAnimating() || !(AnimIsInGroup(0,'Select') || AnimIsInGroup(0,'SelectUnloaded')) )
	{
		if (ReloadRequired())
		{
			PlayAnim('SelectUnloaded',1.0,0.0);
			SelectTime = default.SelectUnloadedTime;
		}
		else
		{
			PlayAnim('Select',1.0,0.0);
			SelectTime = default.SelectTime;
		}
	}
	PlayWeaponSound(SelectSound);
	// show crosshair
	SendEvent("HideCrosshair");
	if(Crosshair?)
		SendEvent(Crosshair);
}

simulated function PlayIdleAnim()
{
	PawnAgentEvent('Ambient');
	if (ReloadRequired())
		LoopAnim('AmbientUnloaded',1.0,0.25);
	else
		LoopAnim('Ambient',1.0,0.25);
}

simulated function PawnAgentEvent(name InEventName);
simulated function PlayAnimEx(name Sequence)
{
	local float TweenTime;

	if (Sequence!='')
	{
		if (Sequence != AmmoType.AnimFire)
			TweenTime = 0.1;
		PlayAnim(Sequence, 1.0, TweenTime);
		LastAnim=Sequence;
	}
}

function PlayWeaponSound( sound Sound, optional float Pitch )
{
	if( Pitch == 0.0 )
		Pitch = 1.0;

	Instigator.PlaySound(Sound,SLOT_Misc,1.0,,,Pitch);
}

// Send an event to the UI system.
simulated function SendEvent( string EventName )
{
	if( Instigator.IsLocallyControlled() )
		PlayerController(Instigator.Controller).SendEvent(EventName);
}

//make our skin show the flash of the shot
simulated function SetFireSkin()
{
	local bool bFirstPerson;

	if( FireSkins.length > 0 )
	{
		OldSkins = Skins;
		bFirstPerson = true;
/*!!MERGE
		if( FireSkinsTP? )
			bFirstPerson = U2PCOwner? && Viewport(U2PCOwner.Player)? && U2PCOwner.ViewingSelf() && !U2PCOwner.ThirdPerson();
*/
		if( bFirstPerson )
			Skins = FireSkins;
		else
			Skins = FireSkinsTP;

		FireSkinTime = Level.TimeSeconds;
	}
}

simulated function UnsetFireSkin()
{
	if( FireSkins.length > 0 )
		Skins = OldSkins;
}

//NEW (mdf)
function Dump( coerce string PrependStr, coerce string AppendStr )
{
	Super.Dump( PrependStr, " AmmoType[0]: " $ AmmoTypes[0] $ " AmmoType[1]:" $ AmmoTypes[1] $ AppendStr );
}
//OLD

defaultproperties
{
	AttachmentClass=class'WeaponAttachment'
	bReplicateInstigator=true
	DisplayFOV=60.0
	DrawType=DT_Mesh
	bCanThrow=false
	aimerror=550.000000
	AIRating=0.500000
	CurrentRating=0.50
	ItemName="Weapon"
	MessageNoAmmo=" has no ammo."
	AutoSwitchPriority=1
	InventoryGroup=1
	PlayerViewOffset=(X=30.000000,Z=-5.000000)
	Icon=Texture'Engine.S_Weapon'
	NameColor=(R=255,G=255,B=255,A=255)
	TargetableTypes(0)=Pawn
	TraceDist=10000 // 208 yards
	MaxRange=+8000.0
	ReloadTime=0.5
	ReloadUnloadedTime=0.25
	SelectTime=0.6
	SelectUnloadedTime=0.6
	DownTime=0.533
	DownUnloadedTime=0.533
	bUpdateCrosshair=true
	bUpdateLocation=true
	bDrawWeapon=true
	bClearZ=true
}
