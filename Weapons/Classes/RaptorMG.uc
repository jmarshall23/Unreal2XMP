
class RaptorMG extends TurretWeapon;

#exec OBJ LOAD File=..\Particles\Turret_Chaingun_FX.u

var ParticleGenerator Muzzleflash;
var ParticleGenerator Muzzledamage;
var Vector BarrelEnd;
var Rotator FlashRotation;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( bPendingDelete )
		return;

	if( Level.NetMode != NM_DedicatedServer )
	{
		Muzzleflash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Chaingun_FX.ParticleSalamander0', Location  );
		Muzzleflash.SetBase(Instigator);
	}
	Muzzledamage = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Chaingun_FX.ParticleSalamander3', Location  );
	Muzzledamage.SetBase(Instigator);
}

// overridden since we don't actually have a projectile to get these values from.
function float GetProjSpeed( bool bAlt )
{
	return 1200;
}

simulated function Destroyed()
{
	if (Muzzleflash?)
	{
		Muzzleflash.Destroy();
		Muzzleflash = None;
	}
	if (Muzzledamage?)
	{
		Muzzledamage.Destroy();
		Muzzledamage = None;
	}
	Super.Destroyed();
}

simulated function PlayFiring()
{
	Super.PlayFiring();
	
	if (FlashRotation.Yaw != 32767)
	{
		FlashRotation.Yaw=32767;
		FlashRotation.Pitch=0;
		FlashRotation.Roll=0;
		BarrelEnd = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
	}
	
	if (Muzzleflash?) 
	{
		MuzzleFlash.SetBase(Instigator);
		MuzzleFlash.SetRelativeLocation(BarrelEnd);
		MuzzleFlash.SetRelativeRotation(FlashRotation);
		Muzzleflash.Trigger(self,Instigator);
	}
	if (Muzzledamage?) 
	{
		Muzzledamage.SetBase(Instigator);
		Muzzledamage.SetRelativeLocation(BarrelEnd);
		Muzzledamage.SetRelativeRotation(flashRotation);
		Muzzledamage.Trigger(self,Instigator);
	}
}

defaultproperties
{
	// text
    ItemName="Heavy Machinegun"
	IconIndex=12
	ItemID="S"
	Crosshair="Crosshair_Pistol"

	// weapon data
	AutoSwitchPriority=10
	InventoryGroup=1
	GroupOffset=1
	PickupAmmoCount=9999
	AttachmentClass=class'RaptorMGAttachment'
	PlayerViewOffset=(X=20,Y=4,Z=-32)
	FireOffset=(X=42.0,Y=9.0,Z=-34.0)

	AmmoName(0)=class'RaptorMGAmmo'

	EnergyCost=0.0005;
}

