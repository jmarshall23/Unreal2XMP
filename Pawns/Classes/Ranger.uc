//=============================================================================
// Ranger.uc
//=============================================================================

class Ranger extends U2Pawn
	abstract;

//-----------------------------------------------------------------------------

var float UsingStartTime;				// time at which healing began

var bool  bHealing;						// distinguish between looking versus actually using another player
var float HealingRate;					// health points healed per second
var float HealingCost;					// skill energy needed per second of healing
var float AccumHealing;					// stores leftover healing between ticks
var localized string HealingMsg;		// message to display on hud while healing players
var localized string MercyKillMsg;		// message to display on hud when mercy killing enemies
var sound HealingStartSound;			// sound when ranger has started to heal an injured player
var sound HealingStopSound;				// sound when full healing is completed
var sound HealingInUseSound;			// ambient sound to play while healing
var sound HealingErrorSound;			// sound if player has no need of healing

//-----------------------------------------------------------------------------

interface function int GetSpecialUseIconIndex( Pawn Other ) { return 0; }

//-----------------------------------------------------------------------------

interface function bool GetSpecialHasUseBar( Pawn Other )
{
	return( Other.CanBeMercyKilled( Self ) );
}

//-----------------------------------------------------------------------------

interface function string GetSpecialUseDescription( Pawn Other )
{
	if( Other.Health > 0 )
	{
		if( GetTeam() == Other.GetTeam() )
			return HealingMsg;
	}
	else
	{
		if( Other.CanBeMercyKilled( Self ) )
			return MercyKillMsg;
		else if( Other.CanBeRevived( Self ) )
			return ReviveMsg;
	}

	return "";
}

//-----------------------------------------------------------------------------

interface function float GetSpecialUsePercent ( Pawn Other )
{
	if( Other.Health > 0 && GetTeam() == Other.GetTeam() )
		return float(Other.Health) / float(Other.default.Health);
	else
		return 0.0;
}

//-----------------------------------------------------------------------------

function NotifyUse( actor Other )
{
	Super.NotifyUse( Other );
	if( CanBeHealed( Other ) )
	{
		bHealing = true;
		UsingStartTime = Level.TimeSeconds;
		PlaySound( HealingStartSound );
		AmbientSound = HealingInUseSound;
	}
	else
		PlaySound( HealingErrorSound );

}

//-----------------------------------------------------------------------------

function NotifyUnuse( actor Other )
{
	Super.NotifyUnuse( Other );
	if( bHealing? )
		HandleFinished( Other );
}

//-----------------------------------------------------------------------------

function HandleFinished( actor Other )
{
	if( Pawn(Other)? )
		XMPGame(Level.Game).NotifyTeammateHealTime( Controller, Pawn(Other).Controller, Level.TimeSeconds - UsingStartTime );
	bHealing = false;
	PlaySound( HealingStopSound );
	AmbientSound = None;
}

//-----------------------------------------------------------------------------

function bool CanBeHealed( actor Other )
{
	local Pawn P;
	P = U2Pawn(Other);
	return( P? && ValidPawn(P) && P.GetTeam() == GetTeam() && P.Health < P.default.Health );
}

//-----------------------------------------------------------------------------

function HandleUsing( Pawn Other, float DeltaTime )
{
	if( CanBeHealed( Other ) )
	{
		// Healing uses up skill energy.  If healing player does not have enough
		// skill for all delta time, heal for as long as remaining skill energy permits
		if( !HasNeededSkill( HealingCost * DeltaTime ) )
			DeltaTime = Skill / HealingCost;

		if( DeltaTime > 0 )
		{
			AccumHealing += DeltaTime * HealingRate;
			Other.Health += int(AccumHealing);
			AccumHealing -= int(AccumHealing);
			UseSkill( HealingCost * DeltaTime );
		}
	}
	else if( bHealing )
	{
		HandleFinished( Other );
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DefaultWeaponNames(0)="Deployables.HealthPacks"
	DefaultWeaponNames(1)="Weapons.EnergyRifle"
	DefaultWeaponNames(2)="Weapons.GrenadeLauncherLight"
	DefaultWeaponNames(3)="Weapons.Pistol"
	DefaultWeaponNames(4)="Weapons.SniperRifle"
	DefaultPowerSuitName="U2.PowerSuitLight"

	GroundSpeed=500
	AccelRate=8192
	SprintRatio=1.65
	StaminaRechargeRate=15
	SprintCost=35.0
	JumpJetZ=1000
	JumpJetCost=0.0025
	JumpJetDuration=0.12
	JumpJetThrustSound=Sound'U2A.Jumping.RangerJumpJetThrust'

	TimeBetweenFootSteps=0.22
    FootstepVolume=0.12
    FootstepPitch=1.05
    LandVolume=0.20
    LandPitch=1.05
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

	RevivedHealth=25
	HealingRate=15
	HealingCost=0.0020
	HealingMsg="Heal"
	HealingStartSound=Sound'U2A.Stations.HealthStationActivate'
	HealingStopSound=Sound'U2A.Stations.HealthStationDeactivate'
	HealingInUseSound=Sound'U2A.Stations.HealthStationAmbient'
	HealingErrorSound=Sound'U2A.Stations.HealthStationError'
	MercyKillMsg="Mercy Kill"

	IconIndex=0
}
