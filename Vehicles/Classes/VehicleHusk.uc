
class VehicleHusk extends Decoration;

var() ParticleGenerator SmokeTemplate;
var ParticleGenerator Smoke;

var() ParticleGenerator ExplosionTemplate;

var bool bExplode;

replication
{
	reliable if( bNetDirty && bNetInitial && Role==ROLE_Authority && LifeSpan > default.LifeSpan-1.0 )
		bExplode;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	bExplode = (Role == ROLE_Authority);
	if( bPendingDelete || Level.NetMode==NM_DedicatedServer )
		return;
	Smoke = class'ParticleGenerator'.static.CreateNew( Self, SmokeTemplate, Location );
	Smoke.SetRotation( Rotation );
	Smoke.SetBase( Self );
	Smoke.TurnOn();
}

simulated function PostNetBeginPlay()
{
	local ParticleGenerator Explosion;
	Super.PostNetBeginPlay();
	if( !bExplode )
		return;
	Explosion = class'ParticleGenerator'.static.CreateNew( self, ExplosionTemplate, Location );
	Explosion.SetRotation( Rotation );
	Explosion.Trigger(self, Instigator);
}

simulated function Destroyed()
{
	if( Smoke? )
	{
		Smoke.ParticleDestroy();
		Smoke = None;
	}
}

defaultproperties
{
	bStatic=false
	//bStasis=false
	bCollideWorld=true
	bAutoAlignToTerrain=true
	bShouldBaseAtStartup=false
	bStaticLighting=true
	bUnlit=true
	bBlockKarma=false
	//bPushable=true
	//bDamageable=true
	//Health=1500
	DrawType=DT_StaticMesh
	LifeSpan=60.0
	SmokeTemplate=ParticleSalamander'Vehicle_Damage_FX.ParticleSalamander4'
	ExplosionTemplate=ParticleSalamander'Vehicle_Explosion_FX.ParticleSalamander0'
	Physics=PHYS_Falling
	//Physics=PHYS_Karma
	Velocity=(Z=600)
	Mass=20000
}
