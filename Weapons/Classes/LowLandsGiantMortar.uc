
class LowlandsGiantMortar extends TurretWeapon;
#exec OBJ LOAD File=..\Particles\Turret_Artillery_Plasma_FX.u

var ParticleGenerator Muzzledamage;
var Vector BarrelEnd;
var Rotator FlashRotation;
var	BendiBeamGenerator FireBeam;
var vector End;
var vector HitLocation, HitNormal;
var actor Other;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( bPendingDelete )
		return;

	Muzzledamage = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Artillery_Plasma_FX.ParticleSalamander0', Location  );
	Muzzledamage.SetBase(Instigator);

	FireBeam = BendiBeamGenerator( class'ParticleGenerator'.static.CreateNew( self, BendiBeamGenerator'Turret_Artillery_Plasma_FX.BendiBeamGenerator3', Location ) );
	FireBeam.SetBase(Instigator);
}

simulated function Destroyed()
{
	if (Muzzledamage?)
	{
		Muzzledamage.Destroy();
		Muzzledamage = None;
	}
	if (FireBeam?)
	{
		FireBeam.Destroy();
		FireBeam = None;
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
	
	GotoState('Firing');
}

simulated state Firing
{
ignores Fire, AltFire;

Begin:

	sleep (1.6);
	AdjustedAim = Instigator.Rotation;
	AdjustedAim.Yaw+=32767;
	AdjustedAim.Pitch= -(Instigator.Rotation.Pitch);
	AdjustedAim.Roll= -(Instigator.Rotation.Roll);

	if (FireBeam?)
	{		
		FireBeam.SetBase(Instigator);
		FireBeam.SetRelativeLocation(BarrelEnd);
		End = FireBeam.location + ((vect(1,0,0)*TraceDist) >> AdjustedAim);
		Other = Trace(HitLocation,HitNormal,End,FireBeam.location,true);
		if( !Other )
		{
			HitLocation = End;
			HitNormal = vect(-1,0,0) >> AdjustedAim;
		}
		FireBeam.SetEndpoints(FireBeam.location,HitLocation);
		FireBeam.Trigger(self,Instigator);

		if (Muzzledamage?) 
		{
			MuzzleDamage.SetRotation(Rotator(HitNormal));
			Muzzledamage.SetLocation(HitLocation);
			Muzzledamage.Trigger(self,Instigator);
		}
	}
	GotoState('');
}

// overridden since we don't actually have a projectile to get these values from.
function float GetProjSpeed( bool bAlt )
{
	return 1200;
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
	PickupAmmoCount=64
	TraceDist=28800 // 600 yards
	AttachmentClass=class'LowlandsGiantMortarAttachment'
	PlayerViewOffset=(X=20,Y=4,Z=-32)
	FireOffset=(X=42.0,Y=9.0,Z=-34.0)
	AmmoName(0)=class'LowlandsGiantMortarAmmo'
	EnergyCost=0.01;
}

