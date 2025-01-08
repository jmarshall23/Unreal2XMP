
class Flamethrower extends Weapon;

#exec OBJ LOAD File=..\Particles\Flamethrower_Effects.u
#exec OBJ LOAD File=..\Particles\FlameAltFX.u

var ParticleGenerator PrimaryFire,SecondaryFire;
var ParticleGenerator PilotA,PilotB;

var FT_Light FireLight[3];

var float Pct;
var int CrossX,CrossY;
var() int MinCrossX,MinCrossY;
var() int MaxCrossX,MaxCrossY;

interface function Point GetCrossPosA(){ local Point P; P.X=-CrossX; P.Y=-CrossY; return P; }
interface function Point GetCrossPosB(){ local Point P; P.X= CrossX; P.Y=-CrossY; return P; }
interface function Point GetCrossPosC(){ local Point P; P.X=-CrossX; P.Y= CrossY; return P; }
interface function Point GetCrossPosD(){ local Point P; P.X= CrossX; P.Y= CrossY; return P; }

simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot )
{
	local float DesiredPct;
	local float Dist;

	DesiredPct = 0.5;

	if( A? && !LevelInfo(A) )
	{
		Dist = VSize(A.Location - ViewLoc);
		if( Dist < 768 )
		{
			DesiredPct = ABlend( 768, 256, Dist );
			DesiredPct = FClamp( DesiredPct, 0, 1 );
		}
	}

	Pct += (DesiredPct - Pct) * 0.1;	//!!arl framerate dependant

	CrossX = Blend( MinCrossX, MaxCrossX, Pct );
	CrossY = Blend( MinCrossY, MaxCrossY, Pct );
}

simulated function PlayAnimEx( name Sequence )
{
	Super.PlayAnimEx(Sequence);
	switch (Sequence)
	{
	case 'Fire':
		if (AmmoIndex==0)
		{
			PrimaryFire.Trigger(self,Instigator);
			TriggerLights();
		}
		else
		{
			SecondaryFire.Trigger(self,Instigator);
			PilotsOff();
		}
		break;
	};
}

simulated state Idle
{
	simulated function BeginState()
	{
		PilotsOn();
	}
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if( bPendingDelete )
		return;

	PrimaryFire = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Flamethrower_Effects.ParticleSalamander1', Location );
	SecondaryFire = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'FlameAltFX.ParticleSalamander0', Location );

	if( Level.NetMode != NM_DedicatedServer )
	{
		PilotA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Flamethrower_Effects.ParticleSalamander2', Location );
		AttachToBone(PilotA, '#Flamejet1');

		PilotB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Flamethrower_Effects.ParticleSalamander3', Location );
		AttachToBone(PilotB, '#Flamejet2');
	}
}

// overridden since we don't actually have a projectile to get these values from.
function float GetProjSpeed( bool bAlt )
{
	return 500;
}

function AuthorityFire()
{
	local FlamethrowerAttachment A;
	A = FlamethrowerAttachment(ThirdPersonActor);
	if (A?) A.bAltFire = (AmmoIndex == 1);
	if (AmmoIndex == 0 && (Instigator.PhysicsVolume.Gravity.Z != Instigator.PhysicsVolume.default.Gravity.Z))
		Instigator.AddVelocity( vector(PrimaryFire.Rotation) * -16.0 );
	Super.AuthorityFire();
}

simulated state Firing
{
ignores Fire,AltFire;

	simulated event BeginState()
	{
		Super.BeginState();
		Enable('Tick');
		bUpdateLocation = false;
	}
	simulated event EndState()
	{
		Super.EndState();
		Disable('Tick');
		bUpdateLocation = true;
	}
	simulated event Tick( float DeltaTime )
	{
		local FlamethrowerAttachment A;
		local vector EffectLocation;
		local rotator EffectRotation;
		local vector Dir,End,Start;
		local actor HitActor;
		local vector HitLocation,HitNormal;

		SetLocation( Instigator.Location + Instigator.CalcDrawOffset(self) );
		SetRotation( Instigator.GetViewRotation() );

		EffectLocation = GetBoneCoords('#Muzzleflash').Origin;
		EffectRotation = GetBoneRotation('#Muzzleflash');

		Dir = vector(EffectRotation);
		End = EffectLocation;
		Start = End - (Dir * 64.0);
		HitActor = Trace(HitLocation,HitNormal,End,Start,true);
		if( HitActor? )
			EffectLocation = HitLocation - Dir; //(Dir * 6.0);

		A = FlamethrowerAttachment(ThirdPersonActor);
		if (A?)
		{
			A.EffectLocation = EffectLocation;
			A.EffectRotation = EffectRotation;
		}

		if (AmmoIndex == 0)
		{
			PrimaryFire.SetLocation( EffectLocation );
			PrimaryFire.SetRotation( EffectRotation );
			PrimaryFire.bForceDelayTick = (Instigator.IsLocallyControlled() && (Level.TimeSeconds - PrimaryFire.LastRenderTime) < 0.5);
		}
		else
		{
			SecondaryFire.SetLocation( EffectLocation );
			SecondaryFire.SetRotation( EffectRotation );
			SecondaryFire.bForceDelayTick = (Instigator.IsLocallyControlled() && (Level.TimeSeconds - SecondaryFire.LastRenderTime) < 0.5);
		}
	}
}

simulated function PilotsOff()
{
	if (PilotA?) PilotA.bHidden = true;
	if (PilotB?) PilotB.bHidden = true;
}

simulated function PilotsOn()
{
	if (PilotA?) PilotA.bHidden = false;
	if (PilotB?) PilotB.bHidden = false;
}

simulated function CreateEffects()
{
	local int i;
	for( i=0; i<ArrayCount(FireLight); i++ )
	{
		FireLight[i] = Spawn( class'FT_Light', Self );
		if(i==0) FireLight[i].Offset.X=300; else
		if(i==1) FireLight[i].Offset.X=600; else
		if(i==2) FireLight[i].Offset.X=900;
	}
}

simulated function TriggerLights()
{
	local int i;
	if( Level.NetMode == NM_DedicatedServer )
		return;
	if( FireLight[0] == None )
		CreateEffects();
	for( i=0; i<ArrayCount(FireLight); i++ )
		FireLight[i].Trigger(self,Instigator);
}

simulated function Destroyed()
{
	local int i;
	for( i=0; i<ArrayCount(FireLight); i++ )
	{
		if( FireLight[i]? )
		{
			FireLight[i].Destroy();
			FireLight[i] = None;
		}
	}
	if (PrimaryFire?)
	{
		PrimaryFire.Destroy();
		PrimaryFire = None;
	}
	if (SecondaryFire?)
	{
		SecondaryFire.Destroy();
		SecondaryFire = None;
	}
	if (PilotA?)
	{
		PilotA.Destroy();
		PilotA = None;
	}
	if (PilotB?)
	{
		PilotB.Destroy();
		PilotB = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	// text
    ItemName="Flamethrower"
	IconIndex=4
	ItemID="FT"
	Crosshair="Crosshair_FT"

	// weapon data
	AutoSwitchPriority=4
	InventoryGroup=2
	GroupOffset=1
	PickupAmmoCount=480
	DisplayFOV=90.0

	// assets
	Mesh=SkeletalMesh'WeaponsK.FT_FP'
	AttachmentClass=class'FlamethrowerAttachment'
	//!! FireSkins(0)=material'WeaponFXT.FT_Skin2FX'
	PlayerViewOffset=(X=-4,Y=-5,Z=-49)
	FireOffset=(X=0,Y=0,Z=0)
	SelectSound=sound'WeaponsA.Flamethrower.FT_Select'
	ReloadSound=sound'WeaponsA.Flamethrower.FT_Reload'
	ReloadUnloadedSound=sound'WeaponsA.Flamethrower.FT_ReloadUnloaded'

	AmmoName(0)=class'FlamethrowerAmmo'
	AmmoName(1)=class'FlamethrowerAmmoAlt'

	// additional timing
	ReloadTime=2.0
	ReloadUnloadedTime=2.0
	DownTime=0.8

	// crosshair parms
	MinCrossX=24
	MinCrossY=12
	MaxCrossX=128
	MaxCrossY=64
}

