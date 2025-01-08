
class JuggernautFlamethrower extends TurretWeapon;

#exec OBJ LOAD File=..\Particles\Turret_Juggernaut_Flame_FX.u

var ParticleGenerator PrimaryFire;
var Vector BarrelEnd;
var Rotator FlashRotation;
var FT_Light FireLight[3];

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( bPendingDelete )
		return;
	PrimaryFire = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Juggernaut_Flame_FX.ParticleSalamander1', Location );
	PrimaryFire.SetBase(Instigator);
}

// overridden since we don't actually have a projectile to get these values from.
function float GetProjSpeed( bool bAlt )
{
	return 500;
}

simulated function Destroyed()
{
	if (PrimaryFire?)
	{
		PrimaryFire.ParticleDestroy();
		PrimaryFire = None;
	}
	Super.Destroyed();
}

simulated function CreateEffects()
{
	local int i;
	for( i=0; i<ArrayCount(FireLight); i++ )
	{
		FireLight[i] = Spawn( class'FT_Light', Self );
		if(i==0) FireLight[i].Offset.X=300; else
		if(i==1) FireLight[i].Offset.X=600; else
		if(i==2) FireLight[i].Offset.X=900;
	}
}

simulated function TriggerLights()
{
	local int i;
	if( Level.NetMode == NM_DedicatedServer )
		return;
	if( FireLight[0] == None )
		CreateEffects();
	for( i=0; i<ArrayCount(FireLight); i++ )
		FireLight[i].Trigger(self,Instigator);
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
	
	if (PrimaryFire?) 
	{
		PrimaryFire.SetBase(Instigator);
		PrimaryFire.SetRelativeLocation(BarrelEnd);
		PrimaryFire.SetRelativeRotation(FlashRotation);
		PrimaryFire.Trigger(self,Instigator);
		TriggerLights();
	}
}

defaultproperties
{
	// text
    ItemName="JuggernautFlamethrower"
	IconIndex=4
	ItemID="FT"
	Crosshair="Crosshair_FT"

	// weapon data
	AutoSwitchPriority=4
	InventoryGroup=1
	GroupOffset=1
	PickupAmmoCount=9999
	DisplayFOV=90.0

	// assets
	//Mesh=SkeletalMesh'WeaponsK.FT_FP'
	AttachmentClass=class'JuggernautFlamethrowerAttachment'
	//!! FireSkins(0)=material'WeaponFXT.FT_Skin2FX'
	PlayerViewOffset=(X=-4,Y=-5,Z=-49)
	FireOffset=(X=0,Y=0,Z=0)
	AmmoName(0)=class'JuggernautFlamethrowerAmmo'
	AmmoName(1)=class'FlamethrowerAmmoAlt'
	EnergyCost=0.0005;
}

