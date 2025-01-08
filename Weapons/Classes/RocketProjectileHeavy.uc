
class RocketProjectileHeavy extends Projectile;

#exec OBJ LOAD File=..\Particles\RocketEffects.u

var ParticleSalamander Sal;
var RL_Blower Blower;
var RL_Sucker Sucker;
var() float ExplosionThreshold;

//viewshake
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;

simulated function PreBeginPlay()
{
	local RocketLauncher RL;

	Super.PreBeginPlay();
	if (bPendingDelete || Level.NetMode==NM_DedicatedServer)
		return;

	// backblast
	Sal = ParticleSalamander( class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'RocketEffects.ParticleSalamander0', Location - (vector(Rotation)*16) ) );
	Sal.SetRotation( Rotation );
	Sal.Trigger(Self,Instigator);
	Sal.ParticleLifeSpan = 5.0;

	// rocket trail
	Sal = ParticleSalamander( class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'RocketEffects.ParticleSalamander1', Location ) );
	Sal.SetRotation( Rotation );
	Sal.SetBase( Self );
	Sal.TurnOn();

	Blower = Spawn( class'RL_Blower',,, Location, Rotation );
	Blower.SetBase( Self );

	Sucker = Spawn( class'RL_Sucker',,, Location, rotator(-vector(Rotation)) );
	Sucker.SetBase( Self );

	if (Owner?)
	{
		RL = RocketLauncher(Pawn(Owner).Weapon);
		if (RL?) RL.LastFiredRocket=Self;
	}
}

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local Actor Explosion;

	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
	{
		Explosion = Spawn( class'RL_Explosion', self,, Location, rotator(HitNormal) );
		Explosion.RemoteRole = ROLE_None;

		Spawn( class'GenericExplosionBlower' );

		class'UtilGame'.static.MakeShake( Self, HitLocation, ShakeRadius, ShakeMagnitude, ShakeDuration );
	}
}

simulated event Destroyed()
{
	if( Sal != None )
	{
		Sal.ParticleDestroy();
		Sal.bIgnoreOutOfWorld = true;
		Sal.Velocity = Velocity;
		Sal.LightType = LT_None;
		Sal.SetPhysics( PHYS_Projectile );
		Sal = None;
	}
	if( Blower != None )
	{
		Blower.Destroy();
		Blower = None;
	}
	if( Sucker != None )
	{
		Sucker.Destroy();
		Sucker = None;
	}

	Super.Destroyed();
}


function TakeDamage( int Damage, Pawn EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	if (Damage > ExplosionThreshold)
		Explode( Location, vector(Rotation), None );
}


defaultproperties
{
	ExplosionThreshold=20
	AmbientSound=sound'WeaponsA.RocketLauncher.RL_Rocket'
	DrawScale=1.0
	Speed=3200
	MaxSpeed=3200
	LifeSpan=6.0
	Damage=190.0
	DamageRadius=384.0
	MyDamageType=class'RocketDamage'
	MomentumTransfer=80000
	AmbientGlow=96
	bUnlit=true
	SoundRadius=30.000000
	TransientSoundRadius=800.000000
	SoundVolume=255
	SoundPitch=100
//	LightType=LT_Steady
//	LightEffect=LE_NonIncidence
//	LightBrightness=255
//	LightHue=28
//	LightSaturation=0
//	LightRadius=6
	bFixedRotationDir=True
	RotationRate=(Roll=50000)
	DesiredRotation=(Roll=30000)
	ShakeRadius=1024
	ShakeMagnitude=40
	ShakeDuration=0.5
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'343M.Rocket_Whole'
	bUseCylinderCollision=true
}
