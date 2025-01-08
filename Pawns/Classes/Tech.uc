//===============================================================================
//  Tech
//===============================================================================

class Tech extends U2Pawn
	abstract;

//-----------------------------------------------------------------------------

var bool  bRepairing;					// distinguish between looking versus actually using another player
var float RepairingRate;				// amount of shield replenished per second
var float RepairingCost;				// skill energy needed per second of Repairing
var float RepairingStartTime;			// time at which repairing began
var float AccumRepairing;				// stores leftover Repairing energy between ticks
var localized string RepairingMsg;		// message to display on hud while resupplying players
var sound RepairingStartSound;			// sound when tech has started replenishing a player's shields
var sound RepairingStopSound;			// sound when full repairing is completed
var sound RepairingInUseSound;			// ambient sound to play while repairing
var sound RepairingErrorSound;			// sound if player has no need of repairing

//-----------------------------------------------------------------------------

interface function string GetSpecialUseDescription( Pawn Other )
{
	if( Other.Health > 0 )
	{
		if( GetTeam() == Other.GetTeam() )
			return RepairingMsg;
	}
	else
	{
		if( Other.CanBeRevived( Self ) )
			return ReviveMsg;
	}
	return "";
}

//-----------------------------------------------------------------------------

interface function int    GetSpecialUseIconIndex( Pawn Other ) { return 1; }
interface function float  GetSpecialUsePercent ( Pawn Other )
{
	local U2Pawn P;
	P = U2Pawn(Other);
	if( P? && P.PowerSuitMax > 0 )
		return P.PowerSuitPower / P.PowerSuitMax;
	else
		return 0.0;
}

//-----------------------------------------------------------------------------

function NotifyUse( actor Other )
{
	Super.NotifyUse( Other );
	if( NeedsToBeUsed( Other ) )
	{
		bRepairing = true;
		RepairingStartTime = Level.TimeSeconds;
		PlaySound( RepairingStartSound );
		AmbientSound = RepairingInUseSound;
	}
	else
		PlaySound( RepairingErrorSound );
}

//-----------------------------------------------------------------------------

function NotifyUnuse( actor Other )
{
	Super.NotifyUnuse( Other );
	if( bRepairing )
		HandleFinished( Other );
}

//-----------------------------------------------------------------------------

function HandleFinished( Actor Other )
{
	if( Pawn(Other)? )
		XMPGame(Level.Game).NotifyTeammateRepairTime( Controller, Pawn(Other).Controller, Level.TimeSeconds - RepairingStartTime );
	bRepairing = false;
	PlaySound( RepairingStopSound );
	AmbientSound = None;
}

//-----------------------------------------------------------------------------

function bool NeedsToBeUsed( actor Other )
{
	local U2Pawn U2P;
	U2P = U2Pawn(Other);
	return( U2P? && ValidPawn(U2P) && U2P.PowerSuitPower < U2P.PowerSuitMax );
}

//-----------------------------------------------------------------------------

function HandleUsing( Pawn Other, float DeltaTime )
{
	local U2Pawn U2P;
	local float Power, Max;
	local float AddedPower, AddedPowerPct;

	U2P = U2Pawn(Other);
	if( U2P == None )
		return;

	Power = U2P.PowerSuitPower;
	Max   = U2P.PowerSuitMax;

	if( Power < Max )
	{
		// Replenishing shields uses up skill energy.  If Repairing player does not have enough
		// skill for all delta time, replenish shields for as long as remaining skill energy permits
		if( !HasNeededSkill( RepairingCost * DeltaTime ) )
			DeltaTime = Skill / RepairingCost;

		if( DeltaTime > 0 )
		{
			// Replenish shield by a percentage of the shield's maximum power, so as not to
			// punish those with heavy powersuits
			AddedPowerPct = AccumRepairing + DeltaTime * RepairingRate;
			AddedPower = AddedPowerPct * Max;

			Power += AddedPower;
			if( Power > Max ) Power = Max;
			U2P.SetPowerSuitPower( Power );

			// save off the unused Repairing time as a percentage of total shield
			AccumRepairing = (AddedPower - int(AddedPower)) / U2P.GetPowerSuitMax();
			UseSkill( RepairingCost * DeltaTime );
		}
	}
	else if( bRepairing )
	{
		HandleFinished( Other );
	}
}

//-----------------------------------------------------------------------------

function bool CanPickupItem( Actor Item )
{
	return ( DeployedUnit(Item) != None );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DefaultWeaponNames(0)="Deployables.FieldGeneratorDeployable"
	DefaultWeaponNames(1)="Deployables.RocketTurretDeployable"
	DefaultWeaponNames(2)="Deployables.AutoTurretDeployable"
	DefaultWeaponNames(3)="Deployables.EnergyPacks"
	DefaultWeaponNames(4)="Weapons.GrenadeLauncherMedium"
	DefaultWeaponNames(5)="Weapons.AssaultRifle"
	DefaultWeaponNames(6)="Weapons.Shotgun"
	DefaultPowerSuitName="U2.PowerSuitMedium"

	GroundSpeed=450
	AccelRate=5000
	SprintRatio=1.5
	StaminaRechargeRate=12.5
	SprintCost=25.0
	JumpJetZ=1200
	JumpJetCost=0.0015
	JumpJetDuration=0.50
	JumpJetThrustSound=Sound'U2A.Jumping.TechJumpJetThrust'
	
	TimeBetweenFootSteps=0.40
    FootstepVolume=0.15
    FootstepPitch=1.0
    LandVolume=0.25
    LandPitch=1.0
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
	
	RepairingRate=0.15
	RepairingCost=0.0020
	RepairingMsg="Repair"
	RepairingStartSound=Sound'U2A.Stations.EnergyStationActivate'
	RepairingStopSound=Sound'U2A.Stations.EnergyStationDeactivate'
	RepairingInUseSound=Sound'U2A.Stations.EnergyStationAmbient'
	RepairingErrorSound=Sound'U2A.Stations.EnergyStationError'

	HackRate=1.0

	IconIndex=1
}
