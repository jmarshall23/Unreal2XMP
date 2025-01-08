
class Shotgun extends Weapon;

#exec OBJ LOAD File=..\Particles\ShotgunFX.u

var ParticleGenerator Muzzleflash;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		Muzzleflash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'ShotgunFX.ParticleSalamander0', Location );
		AttachToBone(Muzzleflash, '#Muzzleflash');
	}
}

simulated function Destroyed()
{
	if (Muzzleflash?)
	{
		Muzzleflash.Destroy();
		Muzzleflash = None;
	}
	Super.Destroyed();
}

simulated function PlayFiring()
{
	Super.PlayFiring();
	if (Muzzleflash?) Muzzleflash.Trigger(self,Instigator);
}

defaultproperties
{
	// text
    ItemName="Shotgun"
	IconIndex=12
	ItemID="S"
	Crosshair="Crosshair_S"

	// weapon data
	AutoSwitchPriority=10
	InventoryGroup=2
	GroupOffset=1
	//!! WeaponAnimationType=AT_Large
	PickupAmmoCount=42

	// assets
	Mesh=SkeletalMesh'WeaponsK.S_FP'
	AttachmentClass=class'ShotgunAttachment'
	//!! FireSkins(0)=material'WeaponFXT.S_FP_Skin2FX'
	//!! FireSkinsTP(0)=material'WeaponFXT.S_TP_Skin2FX'
	PlayerViewOffset=(X=20,Y=4,Z=-32)
	FireOffset=(X=42.0,Y=9.0,Z=-34.0)
	SelectSound=sound'WeaponsA.Shotgun.S_Select'
	ReloadSound=sound'WeaponsA.Shotgun.S_Reload'
	ReloadUnloadedSound=sound'WeaponsA.Shotgun.S_ReloadUnloaded'

	AmmoName(0)=class'ShotgunAmmo'
	AmmoName(1)=class'ShotgunAmmoAlt'

	// additional timing
	ReloadTime=2.21
	ReloadUnloadedTime=2.21
}

