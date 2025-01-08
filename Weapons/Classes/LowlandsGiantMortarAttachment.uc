class LowlandsGiantMortarAttachment extends U2WeaponAttachment;
#exec OBJ LOAD File=..\Particles\Turret_Artillery_Plasma_FX.u

var ParticleGenerator Muzzledamage;
var Vector BarrelEnd;
var Rotator FlashRotation;
var	BendiBeamGenerator FireBeam;
var vector End;
var vector HitLocation, HitNormal;
var actor Other;
var rotator AdjustedAim;
var float TraceDist;
var sound	ChargeUpSound;

simulated event ThirdPersonEffects()
{
	super.ThirdPersonEffects();
	if (Level.NetMode != NM_DedicatedServer)
		PlaySound(ChargeUpSound, SLOT_None, 255, , 255 );
    if (Level.NetMode == NM_Client && Instigator? && !(Instigator.IsLocallyControlled()))
	{
		Muzzledamage = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Artillery_Plasma_FX.ParticleSalamander0', Location  );
		Muzzledamage.SetBase(Instigator);

		FireBeam = BendiBeamGenerator( class'ParticleGenerator'.static.CreateNew( self, BendiBeamGenerator'Turret_Artillery_Plasma_FX.BendiBeamGenerator3', Location ) );
		FireBeam.SetBase(Instigator);
		Fire();
    }
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

simulated function Fire()
{
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

defaultproperties
{
	TraceDist=28800 // 600 yards
	bOnlyRelevantToOwner=false
	ChargeUpSound=Sound'U2Ambient2A.PlanetaryCannon.cannon_windup_dist_3'
}