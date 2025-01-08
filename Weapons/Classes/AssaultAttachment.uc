
class AssaultAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\AssaultFX.u

var ParticleGenerator MuzzleflashA,MuzzleflashB;

var PulseLineGenerator Tracer;
var vector HitLocation,LastHitLocation;

replication
{
	unreliable if( bNetDirty && !bNetOwner && (Role==ROLE_Authority) )
		HitLocation;
}

// Fix ARL: Also #altmuzzleflash?  (Use FireMode to differentiate)

simulated event ThirdPersonEffects()
{
    if (Level.NetMode != NM_DedicatedServer)
	{
		if (!MuzzleflashA)
		{
			MuzzleflashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander2', Location );
			AttachToBone(MuzzleflashA, '#Muzzleflash');
		}
		if (!MuzzleflashB)
		{
			MuzzleflashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander3', Location );
			AttachToBone(MuzzleflashB, '#Muzzleflash');
		}

		MuzzleflashA.Trigger(Self,Instigator);
		MuzzleflashB.Trigger(Self,Instigator);

		// Update tracer.	(maybe only update every 5th round?)

		if (!Tracer)
		{
			Tracer = PulseLineGenerator( class'ParticleGenerator'.static.CreateNew( self, PulseLineGenerator'AssaultFX.PulseLineGenerator1', Location ) );
			AttachToBone(Tracer, '#Muzzleflash');
			if (Tracer.Connections.length!=1)
				Tracer.Connections.length = 1;
			Tracer.Connections[0].StartActor = Tracer;
		}

		if (HitLocation != LastHitLocation)		// make sure we got a new location update.
		{
			LastHitLocation = HitLocation;
			Tracer.Connections[0].End = HitLocation;
			Tracer.Trigger(Self,Instigator);
		}
    }

    Super.ThirdPersonEffects();
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
	if (Tracer?)
	{
		Tracer.Destroy();
		Tracer = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	Mesh=Mesh'Weapons3rdPK.AR_TP'
}

