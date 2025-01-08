
class EMPProjectile extends Projectile;

#exec OBJ LOAD File=..\Particles\ER_Alt_Projectile_volatile.u

var ParticleSalamander Effect;
var float ActivationTime;
var bool bExploded;

//viewshake
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();

	if (bPendingDelete || Level.NetMode==NM_DedicatedServer)
		return;

	Effect = ParticleSalamander( class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'ER_Alt_Projectile_volatile.ParticleSalamander0', Location ) );
	Effect.Trigger(Self,Instigator);
	Effect.SetBase( Self );

	if( Role == ROLE_Authority )
		SetTimer(ActivationTime,false);
	Disable('Tick');
}

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local Actor Explosion;

	bExploded = true;

	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
	{
		Explosion = Spawn( class'EMPExplosion', self,, Location, rotator(HitNormal) );
		Explosion.RemoteRole = ROLE_None;

		class'UtilGame'.static.MakeShake( Self, HitLocation, ShakeRadius, ShakeMagnitude, ShakeDuration );
	}
}

simulated event Destroyed()
{
	if( Effect!=None )
	{
		Effect.Destroy();
		Effect = None;
	}

	if( !bExploded )
		Explode( Location, -normal(Velocity), None );

	Super.Destroyed();
}

simulated event Timer()
{
	Enable('Tick');
}

event Tick( float DeltaTime )
{
	local pawn Victims;
	if(default.LifeSpan-LifeSpan<ActivationTime) return;	// ignore first call when we are spawned.
	foreach VisibleCollidingActors( class'Pawn', Victims, DamageRadius * 0.5, Location )
		if( (Victims != self) && (Victims.Role == ROLE_Authority) )
			Destroy();
}


defaultproperties
{
	bHidden=true
	AmbientSound=sound'WeaponsA.EnergyRifle.ER_EMP'
	SoundRadius=80.000000
	SoundVolume=218
	TransientSoundRadius=500.000000
	LifeSpan=6.0
	Speed=1400
	MaxSpeed=1400
	Damage=120
	DamageRadius=290
	ActivationTime=0.9
	bNoFalloff=true
	MomentumTransfer=5000
	MyDamageType=class'DamageTypeEMP'
	ShakeRadius=1024
	ShakeMagnitude=30
	ShakeDuration=1
}
