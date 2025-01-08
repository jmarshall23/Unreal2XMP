
class RocketProjectileSeeking extends Projectile;

#exec OBJ LOAD File=..\Particles\RocketEffects2.u

var ParticleGenerator Trail;

//viewshake
var() float ShakeRadius, ShakeMagnitude, ShakeDuration;
var() sound ExplodeSound;
var bool bExploded;

var int Index;

var Actor PaintedTarget;
var() float TurnRate;
var() float TurnRateRampUp;

var() float MidPointOffset;

var vector MidPoint;
var float MidPointTimer;

var float RocketContinueNotifyTimer;

replication
{
	reliable if( bNetInitial && bNetDirty && Role==ROLE_Authority )
		PaintedTarget, TurnRateRampUp, MidPoint, MidPointTimer, Index;
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
	local Pawn P;
	local PlayerController PC;
	Super.Destroyed();

	if (Trail != None)
	{
		Trail.ParticleDestroy();
		Trail.SetPhysics(PHYS_None);
		Trail.LightType = LT_None;
		Trail = None;
	}

	// Clear our targetting indicator - if another set is still targetting it'll get turned back on by them.
	P = Pawn(PaintedTarget);
	if (P? && P.IsLocallyControlled())
	{
		PC = PlayerController(P.Controller);
		if (PC?)
		{
			switch (Index)
			{
			case 0: PC.SendEvent("RocketNotifyHideA"); break;
			case 1: PC.SendEvent("RocketNotifyHideB"); break;
			case 2: PC.SendEvent("RocketNotifyHideC"); break;
			case 3: PC.SendEvent("RocketNotifyHideD"); break;
			};
		}
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
	local Pawn P;
	local PlayerController PC;
	local vector Dir,Desired,Delta,TargetLoc;
	if (!PaintedTarget || PaintedTarget.bPendingDelete)
	{
		Explode(Location,vector(Rotation),None);
		return;
	}
	TurnRateRampUp -= DeltaTime;
	TurnRate = (1.0-(TurnRateRampUp/default.TurnRateRampUp)) * default.TurnRate;
	Dir = vector(Rotation);
	MidPointTimer -= DeltaTime;
	if (MidPointTimer>0)	TargetLoc=MidPoint;
	else					TargetLoc=PaintedTarget.Location;
	Desired = normal(TargetLoc - Location);
	Delta = Desired - Dir;
	Dir += Delta * (TurnRate * DeltaTime);
	SetRotation( rotator(Dir) );
	Velocity = Dir * Speed;

	// Keep the triangle notifies up while we continue seeking.
	P = Pawn(PaintedTarget);
	if( P? && P.IsLocallyControlled() )
	{
		RocketContinueNotifyTimer -= DeltaTime;
		if( RocketContinueNotifyTimer <= 0.0 )
		{
			RocketContinueNotifyTimer = 1.0;
			PC = PlayerController(P.Controller);
			if (PC?) switch (Index)
			{
			case 0: PC.SendEvent("RocketNotifyFlashA"); break;
			case 1: PC.SendEvent("RocketNotifyFlashB"); break;
			case 2: PC.SendEvent("RocketNotifyFlashC"); break;
			case 3: PC.SendEvent("RocketNotifyFlashD"); break;
			};
		}
	}
}

function SetTarget( Actor A )
{
	local vector Offset,Diff;
	PaintedTarget = A;
	Diff = A.Location - Location;
	MidPoint = Location + (Diff * RandRange(0.3,0.6));
	Offset.Y = RandRange(-MidPointOffset,MidPointOffset);
	Offset.Z = RandRange(0,MidPointOffset);
	MidPoint += Offset >> rotator(Diff);
	MidPointTimer = VSize(MidPoint - Location) / Speed;
}

defaultproperties
{
	MidPointOffset=512.0
	TurnRateRampUp=0.6
	TurnRate=0.38
	//TurnRate=0.6	//Aaron's fun value
	//TurnRate=2.0	//Scott's dumb value
	LifeSpan=18.0
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

