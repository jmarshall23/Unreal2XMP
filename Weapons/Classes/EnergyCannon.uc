
class EnergyCannon extends TurretWeapon;

#exec OBJ LOAD File=..\Particles\Turret_Harbinger_FX.u

var ParticleGenerator FireA,FireB;
var ParticleGenerator MuzzleflashA,MuzzleFlashB;
var Rotator EffectRotation;
var Vector EffectLocationA,EffectLocationB;

simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot );

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if( bPendingDelete )
		return;

	FireA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Harbinger_FX.ParticleSalamander4', Location );
	FireA.SetBase(Instigator);
	FireB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Harbinger_FX.ParticleSalamander4', Location );
	FireB.SetBase(Instigator);

	if( Level.NetMode != NM_DedicatedServer )
	{
		MuzzleflashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Harbinger_FX.ParticleSalamander0', Location );
		MuzzleFlashA.setBase(Instigator);	
		MuzzleflashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Harbinger_FX.ParticleSalamander0', Location );
		MuzzleFlashB.setBase(Instigator);	
	}
}

function AuthorityFire()
{
	local EnergyCannonAttachment A;
	A = EnergyCannonAttachment(ThirdPersonActor);
	Super.AuthorityFire();
}

simulated function EverywhereFire()
{
	Super.EverywhereFire();
	if (AmmoIndex == 1)
		SendEvent("ER_AltFire");
}

simulated function PlayFiring()
{
	Super.PlayFiring();
	if (EffectRotation.Yaw != 32767)
	{
		EffectRotation.Yaw=32767;
		EffectRotation.Pitch=0;
		EffectRotation.Roll=0;	
		if (XMPVehicleBarrel(Instigator)?)
			EffectLocationA = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
		EffectLocationA.Y -= 15;
		if (XMPVehicleBarrel(Instigator)?)
			EffectLocationB = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
		EffectLocationB.Y+= 15;
	}
	if (FireA?) FireA.setbase(Instigator);
	if (FireA?) FireA.SetRelativeLocation( EffectLocationA );
	if (FireA?) FireA.SetRelativeRotation( EffectRotation );
	if (FireB?) FireB.setbase(Instigator);
	if (FireB?) FireB.SetRelativeLocation( EffectLocationB );
	if (FireB?) FireB.SetRelativeRotation( EffectRotation );
	if (MuzzleFlashA?)MuzzleFlashA.setbase(Instigator);
	if (MuzzleFlashA?)MuzzleflashA.SetRelativeLocation( EffectLocationA );
	if (MuzzleFlashA?)MuzzleflashA.SetRelativeRotation( EffectRotation );
	if (MuzzleFlashB?)MuzzleFlashB.setbase(Instigator);
	if (MuzzleFlashB?)MuzzleflashB.SetRelativeLocation( EffectLocationB );
	if (MuzzleFlashB?)MuzzleflashB.SetRelativeRotation( EffectRotation );

	if (FireA?) FireA.Trigger(self,Instigator);
	if (FireB?) FireB.Trigger(self,Instigator);
	if (MuzzleflashA?) MuzzleflashA.Trigger(self,Instigator);
	if (MuzzleflashB?) MuzzleflashB.Trigger(self,Instigator);
}
	
simulated state Firing
{
ignores Fire,AltFire;

	simulated event BeginState()
	{
		Super.BeginState();
		if (AmmoIndex == 0)
		{
			Enable('Tick');
			bUpdateLocation = false;
			SendEvent("ER_Fire");
		}
	}
	simulated event EndState()
	{
		Super.EndState();
		if (AmmoIndex == 0)
		{
			Disable('Tick');
			bUpdateLocation = true;
			SendEvent("ER_UnFire");
		}
	}
	simulated event Tick( float DeltaTime )
	{
		local bool bDelayTick;

		if (AmmoIndex != 0)
			return;
		bDelayTick = (Instigator.IsLocallyControlled() && (Level.TimeSeconds - FireA.LastRenderTime) < 0.5);
		FireA.bForceDelayTick = bDelayTick;
		FireB.bForceDelayTick = bDelayTick;
	}
}

// overridden since we don't actually have a projectile to get these values from.
function float GetProjSpeed( bool bAlt )
{
	return 1200;
}

simulated function Destroyed()
{
	if (FireA?)
	{
		FireA.ParticleDestroy();
		FireA = None;
	}
	if (MuzzleflashA?)
	{
		MuzzleflashA.Destroy();
		MuzzleflashA = None;
	}
	if (FireB?)
	{
		FireB.ParticleDestroy();
		FireB = None;
	}
	if (MuzzleflashB?)
	{
		MuzzleflashB.Destroy();
		MuzzleflashB = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	// text
    ItemName="Energy Cannon"
	IconIndex=7
	ItemID="ER"
	Crosshair="Crosshair_ER"

	// weapon data
	AutoSwitchPriority=8
	InventoryGroup=4
	GroupOffset=1
	PickupAmmoCount=9999

	// assets
	//Mesh=SkeletalMesh'WeaponsK.ER_FP'
	AttachmentClass=class'EnergyCannonAttachment'
	//!! FireSkins(0)=material'WeaponFXT.ER_Skin2FX'
	PlayerViewOffset=(X=22,Y=1,Z=-34)
	FireOffset=(X=0,Y=0,Z=0)
	SelectSound=sound'WeaponsA.EnergyRifle.ER_Select'
	ReloadSound=sound'WeaponsA.EnergyRifle.ER_Reload'
	ReloadUnloadedSound=sound'WeaponsA.EnergyRifle.ER_ReloadUnloaded'
	SoundVolume=255

	AmmoName(0)=class'EnergyCannonAmmo'
	AmmoName(1)=class'EnergyRifleAmmoAlt'

	// additional timing
	ReloadTime=1.8
	ReloadUnloadedTime=1.8
	EnergyCost=0.001;
}

