class MultiRocketProjectile extends Projectile;

#exec OBJ LOAD File=..\Particles\RocketEffects2.u

var ParticleGenerator Trail;
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;
var() sound ExplodeSound;
var bool bExploded;


simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if (bPendingDelete || Level.NetMode==NM_DedicatedServer)
		return;

	// rocket trail
	Trail = class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'RocketEffects2.ParticleSalamander0', Location );
	Trail.SetRotation( Rotation );
	Trail.SetBase( Self );
	Trail.TurnOn();
}

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local Actor Explosion;

	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
	{
		Explosion = Spawn( class'RL_ExplosionAlt', self,, Location, rotator(HitNormal) );
		Explosion.RemoteRole = ROLE_None;

		PlaySound( ExplodeSound, SLOT_Interact );

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

	Super.Destroyed();
}


defaultproperties 
{
	LifeSpan=12.0
	Speed=2400
	MaxSpeed=2400
	ExplodeSound=sound'WeaponsA.RocketLauncher.RL_ExplodeMini'
	SpawnSound=sound'WeaponsA.RocketLauncher.RL_ExplodeMini'
	AmbientSound=sound'WeaponsA.RocketLauncher.RL_Rocket'
	Damage=100
	DamageRadius=190
	MomentumTransfer=80000
	MyDamageType=class'RocketDamage'
	SoundRadius=30.000000
	TransientSoundRadius=500.000000
	SoundVolume=255
	SoundPitch=100
	LightType=LT_Steady
	LightEffect=LE_NonIncidence
	LightBrightness=255
	LightHue=28
	LightSaturation=0
	LightRadius=6
	ShakeRadius=512
	ShakeMagnitude=20
	ShakeDuration=0.3
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'343M.Rocket_Small'
	bUseCylinderCollision=true
}

