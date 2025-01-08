class GardenMGAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\Turret_SwingArm_FX.u

var ParticleGenerator MuzzleflashA,MuzzleflashB, FireA, FireB;

var vector EffectLocationA,EffectLocationB;
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
			EffectLocationA = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
			EffectLocationA.Y -=30;
			EffectLocationB = -1 * XMPVehicleBarrel(Instigator).FiringOffset;
			EffectLocationB.Y +=30;
		}

		if (!MuzzleFlashA)
		{
			MuzzleFlashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_SwingArm_FX.ParticleSalamander0', Location  );
		}
		if (!MuzzleFlashB)
		{
			MuzzleFlashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_SwingArm_FX.ParticleSalamander0', Location  );
		}
		if (!FireA)
		{
			FireA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_SwingArm_FX.ParticleSalamander3', Location  );
		}
		if (!FireB)
		{
			FireB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_SwingArm_FX.ParticleSalamander3', Location  );
		}
		MuzzleFlashA.setBase(Instigator);
		MuzzleFlashB.setBase(Instigator);
		FireA.setBase(Instigator);
		FireB.setBase(Instigator);
		MuzzleFlashA.SetRelativeLocation( EffectLocationA );
		MuzzleFlashA.SetRelativeRotation( EffectRotation );
		MuzzleFlashB.SetRelativeLocation( EffectLocationB );
		MuzzleFlashB.SetRelativeRotation( EffectRotation );
		FireA.SetRelativeLocation( EffectLocationA );
		FireA.SetRelativeRotation( EffectRotation );
		FireB.SetRelativeLocation( EffectLocationB );
		FireB.SetRelativeRotation( EffectRotation );
		MuzzleFlashA.Trigger(Self,Instigator);
		MuzzleFlashB.Trigger(Self,Instigator);
		FireA.Trigger(Self,Instigator);
		FireB.Trigger(Self,Instigator);
		Fire();
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
	if (FireA?)
	{
		FireA.Destroy();
		FireA = None;
	}
	if (FireB?)
	{
		FireB.Destroy();
		FireB = None;
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