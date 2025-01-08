
class Pistol extends Weapon;

#exec OBJ LOAD File=..\Particles\PistolEffects.u

var ParticleGenerator MuzzleflashA,MuzzleflashB;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		MuzzleflashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'PistolEffects.ParticleSalamander1', Location );
		AttachToBone(MuzzleflashA, '#Muzzleflash');

		MuzzleflashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'PistolEffects.ParticleSalamander16', Location );
		AttachToBone(MuzzleflashB, '#Muzzleflash');
	}
}

simulated function Destroyed()
{
	if (MuzzleflashA?)
	{
		MuzzleflashA.Destroy();
		MuzzleflashA = None;
	}
	if (MuzzleflashB?)
	{
		MuzzleflashB.Destroy();
		MuzzleflashB = None;
	}
	Super.Destroyed();
}

simulated function PlayFiring()
{
	Super.PlayFiring();
	if (MuzzleflashA?) MuzzleflashA.Trigger(self,Instigator);
	if (MuzzleflashB?) MuzzleflashB.Trigger(self,Instigator);
}

simulated state Firing
{
	simulated function Fire()
	{
		if( (LastAnim == AmmoType.AnimFire) && (FireTimer - LatentFloat) > 0.2 )
			Global.Fire();
	}
	simulated function AltFire()
	{
		if( (LastAnim == AmmoType.AnimFire) && (FireTimer - LatentFloat) > 0.2 )
			Global.AltFire();
	}
}

defaultproperties
{
	// text
    ItemName="Pistol"
	IconIndex=2
	ItemID="P"
	Crosshair="Crosshair_Pistol"

	// weapon data
	AutoSwitchPriority=7
	InventoryGroup=2
	GroupOffset=1
	PickupAmmoCount=45
	TraceDist=4800 // 100 yards

	// assets
	Mesh=SkeletalMesh'WeaponsK.P_FP'
	AttachmentClass=class'PistolAttachment'
	//!! FireSkins(0)=material'WeaponFXT.P_Skin2FX'
	PlayerViewOffset=(X=10,Y=1,Z=-22)
	FireOffset=(X=0,Y=0,Z=0)
	SelectSound=sound'WeaponsA.Pistol.P_Select'
	ReloadSound=sound'WeaponsA.Pistol.P_Reload'
	ReloadUnloadedSound=sound'WeaponsA.Pistol.P_ReloadUnloaded'

	AmmoName(0)=class'PistolAmmo'
	AmmoName(1)=class'PistolAmmoAlt'

	// additional timing
	ReloadTime=1.888
	ReloadUnloadedTime=1.33
}

