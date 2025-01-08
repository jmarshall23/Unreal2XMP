
class GrenadeProjectile extends Projectile;

#exec OBJ LOAD File=..\Particles\GrenadeFX.u

var bool bExploded;

var ParticleGenerator Trail;

var() ParticleGenerator ExplosionEffect;
var() sound ExplosionSound;

var bool bParticlesDoDamage;

//viewshake
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();
	if (bPendingDelete || Level.NetMode==NM_DedicatedServer)
		return;

	// rocket trail
	Trail = class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'GrenadeFX.ParticleSalamander2', Location );
	Trail.SetRotation( Rotation );
	Trail.SetBase( Self );
	Trail.TurnOn();
}

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local ParticleGenerator Explosion;

	bExploded = true;

	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer || bParticlesDoDamage)
	{
		if( ExplosionEffect != None )
		{
			Explosion = class'ParticleGenerator'.static.CreateNew( self, ExplosionEffect, HitLocation );
			Explosion.SetRotation( rotator(HitNormal) );
			Explosion.Trigger( self, Instigator );
			Explosion.ParticleLifeSpan = Explosion.GetMaxLifeSpan() + Explosion.TimerDuration + Explosion.RampUpTime + Explosion.RampDownTime;
		}
	}

	if (Level.NetMode != NM_DedicatedServer)
	{
		if( ExplosionSound != None )
			PlaySound( ExplosionSound, SLOT_Interact );

		class'UtilGame'.static.MakeShake( Self, HitLocation, ShakeRadius, ShakeMagnitude, ShakeDuration );
	}
}

simulated event Destroyed()
{
	if( Trail != None )
	{
		Trail.ParticleDestroy();
		Trail.SetPhysics(PHYS_None);
		Trail.LightType = LT_None;
		Trail = None;
	}

	if( !bExploded )
		Explode( Location, -normal(Velocity), None );

	Super.Destroyed();
}

defaultproperties
{
	AmbientSound=sound'WeaponsA.GrenadeLauncher.GL_Grenade'
	BounceSound=sound'WeaponsA.GrenadeLauncher.GL_Bounce'
	DrawScale=2.0
	Physics=PHYS_Falling
	Speed=2600
	MaxSpeed=2600
	LifeSpan=3.0
	AmbientGlow=96
	bUnlit=true
	SoundRadius=120.000000
	TransientSoundRadius=800.000000
	SoundVolume=255
	SoundPitch=100
	bFixedRotationDir=true
	RotationRate=(Roll=65536)
	ShakeRadius=1024
	ShakeMagnitude=60
	ShakeDuration=0.5
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'343M.Grenade'
	bUseCylinderCollision=true
}
