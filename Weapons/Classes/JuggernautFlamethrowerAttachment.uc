
class JuggernautFlamethrowerAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\Turret_Juggernaut_Flame_FX.u

var ParticleGenerator PrimaryFire;
var vector EffectLocation;
var rotator EffectRotation;
var	float	Timer;
var FT_Light FireLight[3];

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

		if (!PrimaryFire)
		{
			PrimaryFire = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Juggernaut_Flame_FX.ParticleSalamander1', Location );
		}

		PrimaryFire.setBase(Instigator);
		PrimaryFire.SetRelativeLocation( EffectLocation );
		PrimaryFire.SetRelativeRotation( EffectRotation );
		PrimaryFire.Trigger(Self,Instigator);
		TriggerLights();
		Fire();
    }
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
	Super.Destroyed();
}

defaultproperties
{
	bOnlyRelevantToOwner=false
}


