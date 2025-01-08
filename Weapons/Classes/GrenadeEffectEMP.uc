
class GrenadeEffectEMP extends Effects;

#exec OBJ LOAD File=..\Particles\GL_EMP.u

simulated function PreBeginPlay()
{
	local Actor A;
	local ParticleGenerator Particles;

	Super.PreBeginPlay();
	if (bPendingDelete)
		return;

	if( Level.NetMode != NM_DedicatedServer )
	{
		Particles = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'GL_EMP.ParticleSalamander0', Location );
		Particles.Trigger( Self, Instigator );
		Particles.ParticleLifeSpan = Particles.GetMaxLifeSpan() + Particles.TimerDuration;
	}

	A=Spawn( class'GrenadeEffectEMPbeams',,, Location );	A.Instigator=Instigator; A.Trigger(Self,Instigator);
	A=Spawn( class'GrenadeEffectEMPbeams',,, Location );	A.Instigator=Instigator; A.Trigger(Self,Instigator);
}

defaultproperties
{
	bGameRelevant=true
	bDynamicLight=true
	LifeSpan=1.0
	CollisionRadius=0.000000
	CollisionHeight=0.000000
	LightType=LT_Flicker
	LightEffect=LE_NonIncidence
	LightBrightness=255
	LightHue=150
	LightSaturation=150
	LightRadius=32
	bHidden=true
}

