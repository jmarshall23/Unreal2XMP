//===============================================================================
//  Gunner
//===============================================================================

class Gunner extends U2Pawn
	abstract;

//-----------------------------------------------------------------------------

var bool  bEquipping;					// distinguish between looking versus actually using another player
var float EquippingRate;				// pct ammo replenished per second
var float EquippingCost;				// skill energy needed per second of equipping
var float EquippingStartTime;			// time at which equipping began
var localized string EquippingMsg;		// message to display on hud while equipping players
var sound EquippingStartSound;			// sound when gunner has started resupplying a player
var sound EquippingStopSound;			// sound when full resupplying is completed
var sound EquippingInUseSound;			// ambient sound to play while resupplying
var sound EquippingErrorSound;			// sound if player has no need of supplies

//-----------------------------------------------------------------------------

interface function string GetSpecialUseDescription( Pawn Other )
{
	if( Other.Health > 0 )
	{
		if( GetTeam() == Other.GetTeam() )
			return EquippingMsg;
	}
	else
	{
		if( Other.CanBeRevived( Self ) )
			return ReviveMsg;
	}
	return "";
}

//-----------------------------------------------------------------------------

interface function float  GetSpecialUsePercent ( Pawn Other ) { if( Other? ) return Other.PlayerReplicationInfo.AmmoEquipedPct; else return 0.0; }
interface function int    GetSpecialUseIconIndex( Pawn Other ) { return 2; }

//-----------------------------------------------------------------------------

function NotifyUse( Actor Other )
{
	local U2Pawn P;

	Super.NotifyUse( Other );

	P = U2Pawn(Other);
	if( class'Pawn'.static.ValidPawn(P) && SameTeam( P ) )
	{
		// add back in any weapons that went offline, but are now supported by team energy
		Level.Game.AddDefaultWeapons( P );

		// start timer
		if( NeedsToBeUsed( Other ) )
		{
			bEquipping = true;
			EquippingStartTime = Level.TimeSeconds;
			PlaySound( EquippingStartSound );
			AmbientSound = EquippingInUseSound;
		}
		else
			PlaySound( EquippingErrorSound );
	}
}

//-----------------------------------------------------------------------------

function NotifyUnuse( actor Other )
{
	Super.NotifyUnuse( Other );
	if( bEquipping )
		HandleFinished( Other );
}

//-----------------------------------------------------------------------------

function HandleFinished( actor Other )
{
	if( Pawn(Other)? )
		XMPGame(Level.Game).NotifyTeammateResupplyTime( Controller, Pawn(Other).Controller, Level.TimeSeconds - EquippingStartTime );
	bEquipping = false;
	PlaySound( EquippingStopSound );
	AmbientSound = None;
}

//-----------------------------------------------------------------------------

function bool NeedsToBeUsed( actor Other )
{
	return( Pawn(Other)? && Pawn(Other).PlayerReplicationInfo.AmmoEquipedPct < 1.0 );
}

//-----------------------------------------------------------------------------

function HandleUsing( Pawn Other, float DeltaTime )
{
	local float Rate;

	if( NeedsToBeUsed( Other ) )
	{
		// Equipping ammo uses up skill energy.  If equipping player does not have enough
		// skill for all delta time, equip for as long as remaining skill energy permits
		if( !HasNeededSkill( EquippingCost * DeltaTime ) )
			DeltaTime = Skill / EquippingCost;

		// scale power up speed by # of ammo types that need ammo
		Rate = class'UtilGame'.static.ScaleAmmoResupplyRate( Other, EquippingRate );
        
		if( DeltaTime > 0 )
		{
			class'UtilGame'.static.AddAllAmmoPercent( Other, DeltaTime * Rate );
			UseSkill( EquippingCost * DeltaTime );
		}
	}
	else if( bEquipping )
	{
		HandleFinished( Other );
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DefaultWeaponNames(0)="Deployables.AmmoPacks"
	DefaultWeaponNames(1)="Deployables.LandMineDeployable"
	DefaultWeaponNames(2)="Deployables.LaserTripMineDeployable"
	DefaultWeaponNames(3)="Weapons.GrenadeLauncherHeavy"
	DefaultWeaponNames(4)="Weapons.Flamethrower"
	DefaultWeaponNames(5)="Weapons.RocketLauncherEx"
	DefaultPowerSuitName="U2.PowerSuitHeavy"

	GroundSpeed=400
	AccelRate=3500
	SprintRatio=1.4
	StaminaRechargeRate=10
	SprintCost=17.5
	JumpJetZ=1750
	JumpJetCost=0.0020
	JumpJetDuration=1.00
	JumpJetThrustSound=Sound'U2A.Jumping.GunnerJumpJetThrust'

	TimeBetweenFootSteps=0.40
    FootstepVolume=0.20
    FootstepPitch=0.95
    LandVolume=0.30
    LandPitch=0.95
	MovementPitchVariancePercent=10.0
	MovementVolumeVariancePercent=10.0
	MovementRadius=400.0
    SoundFootstepsDefault(0)=Sound'XMPPlayerA.MoveDefault1'
    SoundFootstepsDefault(1)=Sound'XMPPlayerA.MoveDefault2'
    SoundFootstepsDefault(2)=Sound'XMPPlayerA.MoveDefault3'
    SoundFootstepsDefault(3)=Sound'XMPPlayerA.MoveDefault4'
    SoundFootstepsTerrain(0)=Sound'XMPPlayerA.MoveTerrain1'
    SoundFootstepsTerrain(1)=Sound'XMPPlayerA.MoveTerrain2'
    SoundFootstepsTerrain(2)=Sound'XMPPlayerA.MoveTerrain3'
    SoundFootstepsTerrain(3)=Sound'XMPPlayerA.MoveTerrain4'
    SoundFootstepsWater(0)=Sound'XMPPlayerA.MoveWater1'
    SoundFootstepsWater(1)=Sound'XMPPlayerA.MoveWater2'
    SoundFootstepsWater(2)=Sound'XMPPlayerA.MoveWater3'
    SoundFootstepsWater(3)=Sound'XMPPlayerA.MoveWater4'
    SoundLandDefault(0)=Sound'XMPPlayerA.LandDefault1'
    SoundLandTerrain(0)=Sound'XMPPlayerA.LandTerrain1'
    SoundLandWater(0)=Sound'XMPPlayerA.LandWater1'

	IconIndex=2
	
	EquippingRate=0.2
	EquippingCost=0.0018
	EquippingMsg="Resupply"
	EquippingStartSound=Sound'U2A.Stations.AmmoStationActivate'
	EquippingStopSound=Sound'U2A.Stations.AmmoStationDeactivate'
	EquippingInUseSound=Sound'U2A.Stations.AmmoStationAmbient'
	EquippingErrorSound=Sound'U2A.Stations.AmmoStationError'
}

