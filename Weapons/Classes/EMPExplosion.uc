
class EMPExplosion extends Effects;

#exec OBJ LOAD File=..\Particles\ER_EMP.u

var() Sound ExplosionSound;

simulated function PreBeginPlay()
{
	local ParticleGenerator Particles;

	Super.PreBeginPlay();
	if (bPendingDelete || Level.NetMode == NM_DedicatedServer)
		return;

	Particles = class'ParticleGenerator'.static.CreateNew( self, ParticleGenerator'ER_EMP.ParticleSalamander0', Location );
	Particles.Trigger( Self, Instigator );
	Particles.ParticleLifeSpan = Particles.GetMaxLifeSpan() + Particles.TimerDuration;

	PlaySound( ExplosionSound );
}

simulated event Tick( float DeltaTime )
{
	LightBrightness = default.LightBrightness * LifeSpan / default.LifeSpan;
}


defaultproperties
{
	ExplosionSound=sound'WeaponsA.EnergyRifle.ER_EMPExplode'
	TransientSoundRadius=500.000000
	bGameRelevant=true
	bDynamicLight=true
	LifeSpan=0.5
	CollisionRadius=0.000000
	CollisionHeight=0.000000
	//LightType=LT_Flicker
	LightType=LT_Steady
	LightEffect=LE_NonIncidence
	LightBrightness=255
	LightHue=150
	LightSaturation=150
	LightRadius=32
	bHidden=true
}

