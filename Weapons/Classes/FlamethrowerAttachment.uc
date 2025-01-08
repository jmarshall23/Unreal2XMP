
class FlamethrowerAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\Flamethrower_Effects.u
#exec OBJ LOAD File=..\Particles\FlameAltFX.u

var ParticleGenerator PrimaryFire,SecondaryFire;

var FT_Light FireLight[3];

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
		if (!PrimaryFire)
			PrimaryFire = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Flamethrower_Effects.ParticleSalamander1', Location );

		if (!SecondaryFire)
		{
			SecondaryFire = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'FlameAltFX.ParticleSalamander0', Location );
			SecondaryFire.IdleTime = 0.0;
		}

		if (!bAltFire)
		{
			PrimaryFire.Trigger(Self,Instigator);
			TriggerLights();
		}
		else
			SecondaryFire.Trigger(Self,Instigator);

		Fire();
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

		if (!bAltFire)
		{
			PrimaryFire.SetLocation( EffectLocation );
			PrimaryFire.SetRotation( EffectRotation );
		}
		else
		{
			SecondaryFire.SetLocation( EffectLocation );
			SecondaryFire.SetRotation( EffectRotation );
		}
	}
}

simulated function CreateEffects()
{
	local int i;
	for( i=0; i<ArrayCount(FireLight); i++ )
	{
		FireLight[i] = Spawn( class'FT_Light', Self );
		if(i==0) FireLight[i].Offset.Z=-300; else
		if(i==1) FireLight[i].Offset.Z=-600; else
		if(i==2) FireLight[i].Offset.Z=-900;
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

simulated function Destroyed()
{
	local int i;
	for( i=0; i<ArrayCount(FireLight); i++ )
	{
		if( FireLight[i]? )
		{
			FireLight[i].Destroy();
			FireLight[i] = None;
		}
	}
	if (PrimaryFire?)
	{
		PrimaryFire.Destroy();
		PrimaryFire = None;
	}
	if (SecondaryFire?)
	{
		SecondaryFire.Destroy();
		SecondaryFire = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	Mesh=Mesh'Weapons3rdPK.FT_TP'
	bAlwaysRelevant=true
}

