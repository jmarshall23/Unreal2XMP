class MortarAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\Turret_Juggernaut_FX.u
var ParticleGenerator Muzzleflash;

var vector EffectLocation;
var rotator EffectRotation;
var	float	Timer;

simulated event ThirdPersonEffects()
{
	super.ThirdPersonEffects();
    if (Level.NetMode == NM_Client && Instigator? && !(Instigator.IsLocallyControlled()))
	{
		if (EffectRotation.Yaw != 32767)
		{
			EffectRotation.Yaw=32767;
			EffectRotation.Pitch=0;
			EffectRotation.Roll=0;	
			EffectLocation = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
		}

		if (!MuzzleFlash)
		{
			Muzzleflash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Artillery_FX.ParticleSalamander1', Location  );
		}

		MuzzleFlash.setBase(Instigator);
		MuzzleFlash.SetRelativeLocation( EffectLocation );
		MuzzleFlash.SetRelativeRotation( EffectRotation );
		MuzzleFlash.Trigger(Self,Instigator);
		Fire();
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

simulated function Fire()
{
	Timer = 0.2;
	GotoState('Firing');
}

simulated state Firing
{
	simulated event BeginState()
	{
		Enable('Tick');
	}
	simulated event EndState()
	{
		Disable('Tick');
	}
	simulated event Tick( float DeltaTime )
	{
		Timer -= DeltaTime;
		if( Timer <= 0 )
		{
			GotoState('');
			return;
		}		
	}
}

defaultproperties
{
	bOnlyRelevantToOwner=false
}
