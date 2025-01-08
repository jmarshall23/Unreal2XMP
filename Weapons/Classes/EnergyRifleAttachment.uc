
class EnergyRifleAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\EnergyFX.u

var ParticleGenerator FireA,FireB;

var vector EffectLocation;
var rotator EffectRotation;
var bool bAltFire;

var float Timer;

replication
{
	unreliable if( bNetDirty && !bNetOwner && (Role==ROLE_Authority) )
		EffectLocation, EffectRotation;

	reliable if( bNetDirty && !bNetOwner && (Role==ROLE_Authority) )
		bAltFire;
}

simulated event ThirdPersonEffects()
{
    if (Level.NetMode == NM_Client && Instigator? && !Instigator.IsLocallyControlled())
	{
		if (!FireA)
			FireA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'EnergyFX.ParticleSalamander0', Location );

		if (!FireB)
			FireB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'EnergyFX.ParticleSalamander3', Location );

		if (!bAltFire)
		{
			FireA.Trigger(Self,Instigator);
			FireB.Trigger(Self,Instigator);
			Fire();
		}
    }

    Super.ThirdPersonEffects();
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

		FireA.SetLocation( EffectLocation );
		FireA.SetRotation( EffectRotation );

		FireB.SetLocation( EffectLocation );
		FireB.SetRotation( EffectRotation );
	}
}

simulated function Destroyed()
{
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

defaultproperties
{
	Mesh=Mesh'Weapons3rdPK.ER_TP'
	bAlwaysRelevant=true
}

