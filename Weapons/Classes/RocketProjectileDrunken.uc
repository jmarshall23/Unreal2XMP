
class RocketProjectileDrunken extends Projectile;

#exec OBJ LOAD File=..\Particles\RocketEffects2.u

var ParticleGenerator Trail;

//viewshake
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;
var() sound ExplodeSound;
var bool bExploded;

var int Index;

var() Range xMagRange, yMagRange;	// spiral parameters.
var float xMag, yMag;

var() range PeriodRange;
var float xPeriods, yPeriods;

var float TimePassed;
var vector StartLocation, StartVelocity;

replication
{
	reliable if( bNetInitial && bNetDirty && Role==ROLE_Authority )
		xMag, yMag, xPeriods, yPeriods, TimePassed, StartLocation, StartVelocity, Index;
}

simulated function PreBeginPlay()
{
	local RocketLauncher RL;

	Super.PreBeginPlay();
	if (bPendingDelete || Level.NetMode==NM_DedicatedServer)
		return;

//	PlayAnim( 'Wing', 0.2 );	//ARL

	Trail = class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'RocketEffects2.ParticleSalamander0', Location );
	Trail.SetRotation( Rotation );
	Trail.SetBase( Self );
	Trail.TurnOn();

	if (Owner?)
	{
		RL = RocketLauncher(Pawn(Owner).Weapon);
		if (RL?) RL.AltRockets[Index]=Self;
	}
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (Trail != None)
	{
		Trail.ParticleDestroy();
		Trail.SetPhysics(PHYS_None);
		Trail.LightType = LT_None;
		Trail = None;
	}
}

function TakeDamage( int Damage, Pawn InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType ){}

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

simulated event Tick( float DeltaTime )
{
	TimePassed += DeltaTime;
	SetCorrectLocation( DeltaTime );
}

function SetupData()
{
	xMag = BlendR(xMagRange, FRand());
	if (FRand() > 0.5) xMag = -xMag;

	yMag = BlendR(yMagRange, FRand());
	if (FRand() > 0.5) yMag = -yMag;

	// calc estimated time to target (clamped to a reasonable value).
	xPeriods = BlendR(PeriodRange, FRand());
	yPeriods = BlendR(PeriodRange, FRand());

	StartLocation = Location;
	StartVelocity = normal(vector(Rotation)) * Speed;
}

simulated function SetCorrectLocation( optional float DeltaTime )
{
	local vector NewLocation;
	NewLocation = CalcLocation();
	Move( NewLocation - Location );
	if (DeltaTime > 0.0)
		Velocity = (NewLocation - Location) / DeltaTime;
}

simulated function vector CalcLocation()
{
	local vector Center;
	local vector Offset;

	Center = StartLocation + (StartVelocity * TimePassed);
	Offset.Y = xMag * sin(TimePassed * PI * xPeriods);
	Offset.Z = yMag * sin(TimePassed * PI * yPeriods);

	return Center + (Offset >> Rotation);	// if we make the rocket spin, we may want to ignore Rotation.Roll here. 
}


defaultproperties
{
	PeriodRange=(A=0.8,B=1.5)
	xMagRange=(A=32,B=140)
	yMagRange=(A=32,B=140)
	LifeSpan=12.0
	Speed=1200
	MaxSpeed=1200
	ExplodeSound=sound'WeaponsA.RocketLauncher.RL_ExplodeMini'
	SpawnSound=sound'WeaponsA.RocketLauncher.RL_ExplodeMini'
	AmbientSound=sound'WeaponsA.RocketLauncher.RL_Rocket'
	Damage=70
	DamageRadius=80
	MomentumTransfer=25000
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

