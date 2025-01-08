
class JuggernautCannon extends TurretWeapon;

#exec OBJ LOAD File=..\Particles\Turret_Juggernaut_FX.u

var ParticleGenerator Muzzleflash;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		Muzzleflash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Juggernaut_FX.ParticleSalamander1', Location  );
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
	local Vector BarrelEnd;
	local Rotator FlashRotation;
	local Rotator NormalizedRotation;

	Super.PlayFiring();
	
	FlashRotation.Yaw=32767;
	FlashRotation.Pitch=0;
	FlashRotation.Roll=0;
	
	BarrelEnd = -1 * XMPVehicleBarrel(Instigator).FiringOffset;

	if (Muzzleflash?) 
	{
		MuzzleFlash.SetLocation(Instigator.Location);
		MuzzleFlash.SetBase(Instigator);
		MuzzleFlash.SetRelativeLocation(BarrelEnd);
		MuzzleFlash.SetRelativeRotation(FlashRotation);
		Muzzleflash.Trigger(self,Instigator);
	}
	if (ROLE == ROLE_Authority)
	{
		NormalizedRotation = Normalize(Instigator.Rotation);
		KVehicle(Instigator.ConglomerateID).KAddImpulse(vector(NormalizedRotation)*280000, Instigator.location);
	}
}

defaultproperties
{
	// text
    ItemName="Mortar"
	IconIndex=12
	ItemID="S"
	Crosshair="Crosshair_Pistol"

	// weapon data
	AutoSwitchPriority=10
	InventoryGroup=1
	GroupOffset=1
	//!! WeaponAnimationType=AT_Large
	PickupAmmoCount=9999

	// assets
	//Mesh=SkeletalMesh'WeaponsK.S_FP'
	AttachmentClass=class'JuggernautCannonAttachment'
	//!! FireSkins(0)=material'WeaponFXT.S_FP_Skin2FX'
	//!! FireSkinsTP(0)=material'WeaponFXT.S_TP_Skin2FX'
	PlayerViewOffset=(X=20,Y=4,Z=-32)
	FireOffset=(X=42.0,Y=9.0,Z=-34.0)
	//SelectSound=None
	//ReloadSound=None
	//ReloadUnloadedSound=None

	AmmoName(0)=class'JuggernautCannonAmmo'
	//AmmoName(1)=class'ShotgunAmmoAlt'
	EnergyCost=0.003;
	// additional timing
	//ReloadTime=2.93
	//ReloadUnloadedTime=1.8333
}

