
class CarnivalMG extends TurretWeapon;

#exec OBJ LOAD File=..\Particles\Turret_Post_FX.u

var ParticleGenerator Muzzle1flash;
var ParticleGenerator Muzzle2flash;
var ParticleGenerator Muzzle1damage;
var ParticleGenerator Muzzle2damage;
var Vector Barrel1End;
var Vector Barrel2End;
var Rotator FlashRotation;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( bPendingDelete )
		return;

	if( Level.NetMode != NM_DedicatedServer )
	{
		Muzzle1flash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Post_FX.ParticleSalamander0', Location  );
		Muzzle1flash.SetBase(Instigator);
		Muzzle2flash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Post_FX.ParticleSalamander0', Location  );
		Muzzle2flash.SetBase(Instigator);
	}
	Muzzle1damage = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Post_FX.ParticleSalamander3', Location  );
	Muzzle1damage.SetBase(Instigator);
	Muzzle2damage = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Post_FX.ParticleSalamander3', Location  );
	Muzzle2damage.SetBase(Instigator);
}

simulated function Destroyed()
{
	if (Muzzle1flash?)
	{
		Muzzle1flash.Destroy();
		Muzzle1flash = None;
	}
	if (Muzzle2flash?)
	{
		Muzzle2flash.Destroy();
		Muzzle2flash = None;
	}
	if (Muzzle1damage?)
	{
		Muzzle1damage.Destroy();
		Muzzle1damage = None;
	}
	if (Muzzle2damage?)
	{
		Muzzle2damage.Destroy();
		Muzzle2damage = None;
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
		Barrel1End = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
		Barrel1End.Y += 30;
		Barrel2End = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
		Barrel2End.Y -= 30;
	}
	
	if (Muzzle1flash?) 
	{
		Muzzle1Flash.SetBase(Instigator);
		Muzzle1Flash.SetRelativeLocation(Barrel1End);
		Muzzle1Flash.SetRelativeRotation(FlashRotation);
		Muzzle1flash.Trigger(self,Instigator);
	}
	if (Muzzle2flash?) 
	{
		Muzzle2Flash.SetBase(Instigator);
		Muzzle2Flash.SetRelativeLocation(Barrel2End);
		Muzzle2Flash.SetRelativeRotation(FlashRotation);
		Muzzle2flash.Trigger(self,Instigator);
	}
	if (Muzzle1damage?) 
	{
		Muzzle1damage.SetBase(Instigator);
		Muzzle1damage.SetRelativeLocation(Barrel1End);
		Muzzle1damage.SetRelativeRotation(flashRotation);
		Muzzle1damage.Trigger(self,Instigator);
	}
	if (Muzzle2damage?) 
	{
		Muzzle2damage.SetBase(Instigator);
		Muzzle2damage.SetRelativeLocation(Barrel2End);
		Muzzle2damage.SetRelativeRotation(flashRotation);
		Muzzle2damage.Trigger(self,Instigator);
	}
}

// overridden since we don't actually have a projectile to get these values from.
function float GetProjSpeed( bool bAlt )
{
	return 1200;
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
	AttachmentClass=class'CarnivalMGAttachment'
	PlayerViewOffset=(X=20,Y=4,Z=-32)
	FireOffset=(X=42.0,Y=9.0,Z=-34.0)

	AmmoName(0)=class'CarnivalMGAmmo'
	EnergyCost=0.0005;

}

