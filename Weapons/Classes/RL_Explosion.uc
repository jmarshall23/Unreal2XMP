
class RL_Explosion extends Effects;

#exec OBJ LOAD File=..\Particles\RocketEffects.u

var() ParticleGenerator ParticleEffect;

simulated event PostBeginPlay()
{
	local ParticleGenerator Explosion;

	Super.PostBeginPlay();

	if( Level.NetMode != NM_DedicatedServer )
	{
		Explosion = class'ParticleGenerator'.static.CreateNew( self, ParticleEffect, Location-Velocity );
		Explosion.SetRotation( Rotation );
		Explosion.Trigger( Self, Instigator );
		Explosion.ParticleLifeSpan = 5.0;
	}
}

simulated event Tick( float DeltaTime )
{
	LightBrightness = default.LightBrightness * LifeSpan / default.LifeSpan;
}

defaultproperties
{
     ParticleEffect=ParticleSalamander'RocketEffects.ParticleSalamander2'
	 DrawType=DT_None
     LifeSpan=0.800000
     LightType=LT_Steady
     LightEffect=LE_NonIncidence
     LightBrightness=255
     LightHue=28
     LightSaturation=0
     LightRadius=12
	 bDynamicLight=true
}
