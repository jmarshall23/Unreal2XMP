class JuggernautCannonProjectile extends Projectile;

#exec OBJ LOAD File=..\Particles\GrenadeFX.u

//viewshake
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;
var ParticleGenerator P;
var ParticleGenerator Trail;
var bool bExploded;


simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
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
	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
	{

		P = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Juggernaut_FX.ParticleSalamander0');
		P.ParticleLifeSpan = P.GetMaxLifeSpan() + P.TimerDuration;
		P.SetBase(self);
		P.SetRotation(rotator(HitNormal));
		P.Trigger( self, Instigator );
	}
}

simulated event Destroyed()
{
	if( P != None )
		P.ParticleDestroy();
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
	AmbientSound=sound'WeaponsA.GrenadeLauncher.GL_Grenade'
	BounceSound=sound'WeaponsA.GrenadeLauncher.GL_Bounce'
	DrawScale=2.0
	Damage=1300.0
	DamageRadius=800.0
	MyDamageType=class'JuggernautCannonDamage'
	MomentumTransfer=600000
	AmbientGlow=96
	bUnlit=true
	SoundRadius=120.000000
	TransientSoundRadius=800.000000
	SoundVolume=255
	SoundPitch=100
	ShakeRadius=1024
	ShakeMagnitude=80
	ShakeDuration=0.5
	Speed=9000
	MaxSpeed=8000
	LifeSpan=9.0
	bFixedRotationDir=True
	RotationRate=(Roll=50000)
	DesiredRotation=(Roll=30000)
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'343M.Grenade'
	bUseCylinderCollision=true
}

