//=============================================================================
// $Author: Aleiby $
// $Date: 11/22/02 1:00a $
// $Revision: 17 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	PulseLineGenerator.uc
// Author:	Aaron R Leiby
// Date:	15 August 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class PulseLineGenerator extends ParticleGenerator
	placeable
	native;

struct native TConnectionInfo
{
	var() vector				Start;
	var() vector				End;
	var   float					Length;

	var() Actor					StartActor;
	var() Actor					EndActor;
};

	//NOTE: This was originally added to the connection info,
	//but struct serialization sucks and this broke existing maps.
	var() Actor					MeshOwner;
	var() string				StartBone;
	var() string				EndBone;

var() array<TConnectionInfo>	Connections;

var(Damage) float				DamageTime;	// time between damage traces.
var         float				DamageTimer;
var(Damage) int					DamageAmount;
var(Damage) vector				DamageMomentum;
var(Damage) class<DamageType>	DamageType;
var(Damage) string				DamageEffect;
var(Damage) name				DamageEvent;
var(Damage) float				DamageSleep;

var() Material					BeamTexture;
var() Range						BeamSegLength;
var() color						BeamColor;
var() Range						BeamWidth;
var() float						BeamTextureWidth;
var() Range						NumBeams;
var() ERenderStyle				BeamStyle;

var() bool						bFadeAlphaOnly;		// for fading with RampUpTime/RampDownTime.
var() bool						bJointFadeAlphaOnly;

var() Material					SpriteJointTexture;
var() Range						SpriteJointSize;
var() color						SpriteJointColor;
var() ERenderStyle				SpriteJointStyle;

var() class<ParticleTemplate>	TemplateClass;

/*
replication
{
	unreliable if( Role==ROLE_Authority )
		Start, End;

	unreliable if( Role==ROLE_Authority && bNetInitial )
		BeamTexture
	,	BeamSegLength
	,	BeamColor
	,	BeamWidth
	,	BeamTextureWidth
	,	NumBeams
	,	BeamStyle
	,	SpriteJointTexture
	,	SpriteJointSize
	,	SpriteJointColor
	,	SpriteJointStyle
	;
}
*/

simulated function AddConnection( vector Start, vector End )
{
	local int i;
	i = Connections.length;
	Connections.length = i+1;
	SetEndpoints( Start, End, i );
}

simulated function SetEndpoints( vector Start, vector End, optional int i )
{
	Connections[i].Start = Start;
	Connections[i].End = End;
	UpdateRenderBoundingBox();
}

simulated native function UpdateRenderBoundingBox();

simulated function SetMeshOwner( int i, Actor A ){ /*Connections[i].*/MeshOwner=A; }

// Fix ARL: May not work well with RampUp/RampDown.
simulated event TurnOn(){ Super.TurnOn(); Enable('Tick'); }
simulated event TurnOff(){ Super.TurnOff(); Disable('Tick'); }

//------------------------------------------------------------------------------
event Tick( float DeltaTime )
{
	local Actor HitActor;
	local vector HitLocation, HitNormal;
	local int i;

	if( !bOn || DamageTime <= 0 )
	{
		Disable('Tick');
		return;
	}

	DamageTimer += DeltaTime;

	if( DamageTimer >= DamageTime )
	{
		DamageTimer = 0;

		for( i=0; i<Connections.length; i++ )
		{
			// could be optimized by tracing Pawns only
			HitActor = Trace( HitLocation, HitNormal, Connections[i].End, Connections[i].Start, true );

			if( HitActor? )
			{
				HandleHitActor( i, HitActor, HitLocation, HitNormal );
			}
		}
	}
}

//------------------------------------------------------------------------------
function HandleHitActor( int ConnectionIndex, actor HitActor, vector HitLocation, vector HitNormal )
{
	if (DamageAmount>0)
		HitActor.TakeDamage( DamageAmount, Instigator, HitLocation, DamageMomentum, DamageType );

	if (DamageEvent!='')
		TriggerEvent( DamageEvent, Self, None );

	if (Owner!=None)
		Owner.Touch( HitActor );

	SpawnSparks( HitLocation, HitNormal );

	DamageTimer = -DamageSleep;
}

//------------------------------------------------------------------------------
function SpawnSparks( vector HitLocation, vector HitNormal )
{
	local ParticleSalamander Sparks;

	if( DamageEffect != "" )
	{
		Sparks = ParticleSalamander(class'ParticleGenerator'.static.DynamicCreateNew( Self, class'ParticleSalamander', DamageEffect, HitLocation ));
		Sparks.SetRotation( rotator(HitNormal) );
		Sparks.ParticleLifeSpan = Sparks.TimerDuration + Sparks.GetMaxLifeSpan();
		Sparks.Trigger( Self, Instigator );
	}
}


defaultproperties
{
	CullDistance=0.000000
	RemoteRole=ROLE_SimulatedProxy
	bAlwaysRelevant=true
	BeamTexture=Texture'ParticleSystems.Pulse.GlowBeam'
	BeamSegLength=(A=3,B=8)
	BeamColor=(R=255,G=255,B=255,A=255)
	BeamWidth=(A=1,B=4)
	BeamTextureWidth=3
	NumBeams=(A=3,B=3)
	BeamStyle=STY_Translucent
	SpriteJointTexture=Texture'ParticleSystems.Pulse.GlowFuzz'
	SpriteJointSize=(A=4,B=6)
	SpriteJointColor=(R=128,G=128,B=128,A=255)
	SpriteJointStyle=STY_Translucent
	DamageAmount=1
	DamageEffect="Impact_Metal_AR.ParticleSalamander0"
//	DamageType='Electrical'	// Fix ARL: Use DamageType subclass.
	TemplateClass=class'ParticleSystems.PulseLineTemplate'
	DefaultForces=()
//	bAttachDelayTick=false
}

