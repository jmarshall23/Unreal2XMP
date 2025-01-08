class MortarProjectile extends GrenadeProjectile;

//viewshake
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;
var ParticleGenerator P;

simulated function PostBeginPlay()
{
//	local ParticleGenerator P;

	Super.PostBeginPlay();
	if (bPendingDelete)
		return;

	P = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Turret_Artillery_FX.ParticleSalamander0');
	//P.Trigger( self, Instigator );
	P.ParticleLifeSpan = P.GetMaxLifeSpan() + P.TimerDuration;
	P.SetBase(self);
}

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
	{
		P.SetRotation(rotator(HitNormal));
		//Explosion = Spawn( class'RL_Explosion', self,, Location + ExploWallOut * HitNormal, rotator(HitNormal) );
		//Explosion.RemoteRole = ROLE_None;
		P.Trigger( self, Instigator );
	}
}

simulated event Destroyed()
{
	if( P != None )
		P.ParticleDestroy();

	Super.Destroyed();
}


defaultproperties 
{
	AmbientSound=sound'WeaponsA.RocketLauncher.RL_Rocket'
	DrawScale=10.0
	Speed=38400
	MaxSpeed=38400
	LifeSpan=9.0
	Damage=800.0
	DamageRadius=1200.0
	MyDamageType=class'MortarDamage'
	MomentumTransfer=296000
	AmbientGlow=96
	bUnlit=true
	SoundRadius=120.000000
	TransientSoundRadius=800.000000
	SoundVolume=255
	SoundPitch=100
	//bFixedRotationDir=True
	//RotationRate=(Roll=50000)
	//DesiredRotation=(Roll=30000)
	ShakeRadius=1400
	ShakeMagnitude=120
	ShakeDuration=0.5
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'343M.Grenade'
	bUseCylinderCollision=true
}

