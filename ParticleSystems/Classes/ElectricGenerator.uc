//=============================================================================
// $Author: Mfox $
// $Date: 10/11/02 5:43p $
// $Revision: 15 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ElectricGenerator.uc
// Author:	Aaron R Leiby
// Date:	21 August 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------
// Todo:
// + Make AffectedTypes stuff work.
// + Fix Triangle location calculation code.
//------------------------------------------------------------------------------

class ElectricGenerator extends Actor
	placeable
	native;

struct ElectricArc
{
	var Actor  Actor;		// Use the Actor's location if this is set.
	var vector Offset;		// Relative offset from above Actor.
	var vector Point;		// Use this location if Actor is not set.
	var float  LifeSpan;	// How long this electric arc last for.
	var int    iTri;		// Triangle index to use.
	var BendiBeamGenerator Arc;
};

var() Actor  Source;			// Use this Actor's mesh (defaults to Self).
var() bool   bStartAtOrigin;	// Set to true if you want all the electrical arcs to come from the origin (rather than from individual polys).
var() bool   bResetArcOnReuse;	// Try setting this to true if your electric arcs are coming out straight sometimes.
var() bool   bLatchGeometry;	// Do we attach to geometry?
var() bool	 bLatchAtOrigin;	// CDH: Latch at origin of actors instead of collision cylinder
var() float  AffectedRadius;	// How close an Actor (or geometry) must be in order to be latched on to.
var() float  MinArcLength;		// Arcs shorter than this get ignored.
var() Range  LatchRate;			// Number of seconds between latching.  (A latching is when a new electrical arc is created).
var() Range  ArcLifeSpan;		// How long an electrical arc lasts.
var() name   AffectedTypes[3];	// Types of Actors which can be latched on to.
var() Range  DamageRate;		// Amount of damage to apply to attached Actors (per second).
var() Range  DamageStart;		// CDH: Amount of damage to apply at initial latching
var() string SparkType;			// Name of the Package.ParticleGeneratorName to use.
var() class<BendiBeamGenerator> BeamType;
var() float  SpreadDegrees;		// CDH: Nonzero limits latch beams to a cone of this angle around the generator's rotation.  Zero uses full 360 degrees.

var ElectricArc ElectricArcs[32];
var float NextArcTime;

var int TickDelay;

// Trigger support.
var(              ) bool  bOn;
var(TriggerSupport) bool  bInitiallyOn;
var(TriggerSupport) float TimerDuration;
var                 float InternalTimer;

// Beam variable overrides.
var(BeamOverrides) Material BeamTexture;
var(BeamOverrides) Range    BeamSegLength;
var(BeamOverrides) color    BeamColor;
var(BeamOverrides) Range    BeamWidth;
var(BeamOverrides) Range    BeamTextureWidth;
var(BeamOverrides) Range    NumBeams;
var(BeamOverrides) Material SpriteJointTexture;
var(BeamOverrides) Range    SpriteJointSize;
var(BeamOverrides) color    SpriteJointColor;
var(BeamOverrides) Range    MetaBeamSegLength;
var(BeamOverrides) Range    MetaBeamWidth;
var(BeamOverrides) Range    MetaBeamRefreshTime;
var(BeamOverrides) float    DamageTime;
var(BeamOverrides) int      DamageAmount;
var(BeamOverrides) vector   DamageMomentum;
var(BeamOverrides) class<DamageType> DamageType;
var(BeamOverrides) string   DamageEffect;

/////////////////////
// Trigger support //
/////////////////////

//------------------------------------------------------------------------------
simulated function PostRecvBInitiallyOn()
{
	bOn = bInitiallyOn;
}

//------------------------------------------------------------------------------
// Toggles us on and off when triggered.
//------------------------------------------------------------------------------
simulated state() TriggerToggle
{
	simulated function Trigger( Actor Other, Pawn EventInstigator )
	{
		bOn = !bOn;
		Instigator = EventInstigator;
	}
}

//------------------------------------------------------------------------------
// Toggled when Triggered.
// Toggled back to initial state when UnTriggered.
//------------------------------------------------------------------------------
simulated state() TriggerControl
{
	simulated function Trigger( Actor Other, Pawn EventInstigator )
	{
		bOn = !bInitiallyOn;
		Instigator = EventInstigator;
	}

	simulated function UnTrigger( Actor Other, Pawn EventInstigator )
	{
		bOn = bInitiallyOn;
	}
}

//------------------------------------------------------------------------------
// Toggled when triggered.
// Toggled back to initial state after TimerDuration seconds.
//------------------------------------------------------------------------------
simulated state() TriggerTimed
{
	simulated function Trigger( Actor Other, Pawn EventInstigator )
	{
		bOn = !bInitiallyOn;
		InternalTimer = TimerDuration;	// bOn will be toggled automatically internally.
		Instigator = EventInstigator;
	}
}


///////////
// Logic //
///////////

simulated event Destroyed()
{
	local int i;

	for( i=0; i<ArrayCount(ElectricArcs); i++ )
	{
		if (ElectricArcs[i].Arc != None)
		{
			ElectricArcs[i].Arc.Destroy();
			ElectricArcs[i].Arc = None;
		}
	}

	Super.Destroyed();
}
/*!!MERGE
//------------------------------------------------------------------------------
event Tick( float DeltaTime )
{
	local int i;

	// Hack: UMesh::GetFrame corrupts data (in UInput::ReadInput) if you try 
	// calling it on the first Tick during startup.
	if( TickDelay-- > 0 ) return;

	// Update Timer as needed.
	if( InternalTimer > 0 )
	{
		InternalTimer -= DeltaTime;

		if( InternalTimer <= 0 )
		{
			InternalTimer = 0;
			bOn = bInitiallyOn;
		}
	}

	// Update LifeSpans of existing Arcs.
	for( i=0; i<ArrayCount(ElectricArcs); i++ )
	{
		if( ElectricArcs[i].LifeSpan > 0 )
		{
			ElectricArcs[i].LifeSpan -= DeltaTime;
			if( ElectricArcs[i].LifeSpan <= 0 )
			{
				ElectricArcs[i].Arc.bHidden = true;
				ElectricArcs[i].Arc.DamageTime = 0.0;
			}
		}
	}

	// Add new arcs.
	if( bOn && Level.TimeSeconds >= NextArcTime )
	{
		NextArcTime = Level.TimeSeconds + GetRand( LatchRate );
		AddArc();
	}

	CheckForActorBreaches();

	// Update endpoints.
	for( i=0; i<ArrayCount(ElectricArcs); i++ )
	{
		if( ElectricArcs[i].LifeSpan > 0 )
		{
			ElectricArcs[i].Arc.Connections[0].Start = GetTriLocationEx( ElectricArcs[i].iTri );

			if( ElectricArcs[i].Actor == None )
				ElectricArcs[i].Arc.Connections[0].End = ElectricArcs[i].Point;
			else if( ElectricArcs[i].Offset == vect(0,0,0) )
				ElectricArcs[i].Arc.Connections[0].End = ElectricArcs[i].Actor.Location;
			else
				ElectricArcs[i].Arc.Connections[0].End = ElectricArcs[i].Actor.Location + (ElectricArcs[i].Offset >> ElectricArcs[i].Actor.Rotation);
		}
	}
}

//------------------------------------------------------------------------------
function AddArc()
{
	local vector End, Start;
	local CheckResult Hit;
	local Actor TraceActor;
	local int i;
	local int PolyFlags;
	local float Damage;

	i = GetUnusedArcIndex();
	if( IsValidArcIndex(i) )
	{
		TraceActor = GetSource();

		if (SpreadDegrees > 0.0)
			End = TraceActor.Location + (RandomSpreadVector(SpreadDegrees) >> TraceActor.Rotation) * AffectedRadius;
		else
			End = TraceActor.Location + VRand() * AffectedRadius;

		ElectricArcs[i].iTri = GetFacingTriIndexEx( End );

		Start = GetTriLocationEx( ElectricArcs[i].iTri );

		if( bLatchGeometry )
			PolyFlags = TRACE_AllColliding;
		else
			PolyFlags = TRACE_Pawns | TRACE_Others | TRACE_OnlyProjActor | TRACE_Blocking;

		if( !SingleLineCheck( Hit, TraceActor, End, Start, PolyFlags ) )
		{
			ElectricArcs[i].Actor = None;
			
			if( VSize(Hit.Location - Start)<MinArcLength )
			{
				// too short, do nothing...
			}
			if( bLatchGeometry && LevelInfo(Hit.Actor) != None )
			{
				ElectricArcs[i].Actor     = None;
				ElectricArcs[i].Offset    = vect(0,0,0);
				ElectricArcs[i].Point     = Hit.Location;
				ElectricArcs[i].LifeSpan @= ArcLifeSpan;
				InitBeam(i);
				SpawnSparks( Hit );
			}
			else if( ActorBreach( Hit.Actor, Hit.Location, i ) )
			{
				ElectricArcs[i].LifeSpan @= ArcLifeSpan;
				InitBeam(i);
				SpawnSparks( Hit );

				Damage @= DamageStart;
				if ((Hit.Actor!=None) && (Damage > 0))
					Hit.Actor.TakeDamage( Damage, Instigator, Hit.Location, DamageMomentum, DamageType );
			}
		}
	}
	else
	{
		warn("Used up all ElectricArc slots.  Array size needs to be increased.");
	}
}

//------------------------------------------------------------------------------
function ResetArcs()
{
	local int i;
	
	for( i=0; i<ArrayCount(ElectricArcs); i++ )
	{
		ElectricArcs[i].LifeSpan = 0.0;
		ElectricArcs[i].Actor = None;
		if( ElectricArcs[i].Arc? )
		{
			ElectricArcs[i].Arc.bHidden = true;
			ElectricArcs[i].Arc.DamageTime = 0.0;
		}
	}
}

//------------------------------------------------------------------------------
function CheckForActorBreaches()
{
	local CheckResult Hit;
	local Actor TraceActor;
	local int i;
	local int PolyFlags;

	TraceActor = GetSource();

	for( i=0; i<ArrayCount(ElectricArcs); i++ )
	{
		if( ElectricArcs[i].LifeSpan > 0 )
		{
			if( bLatchGeometry )
				PolyFlags = TRACE_AllColliding;
			else
				PolyFlags = TRACE_Pawns | TRACE_Others | TRACE_OnlyProjActor | TRACE_Blocking;

			if( !SingleLineCheck( Hit, TraceActor, ElectricArcs[i].Arc.Connections[0].End, ElectricArcs[i].Arc.Connections[0].Start, PolyFlags ) )
			{
				if( bLatchGeometry && LevelInfo(Hit.Actor) != None && VSize(ElectricArcs[i].Arc.Connections[0].End - Hit.Location) > 10 )
				{
					ElectricArcs[i].Actor	= None;
					ElectricArcs[i].Offset	= vect(0,0,0);
					ElectricArcs[i].Point	= Hit.Location;
					ElectricArcs[i].Arc.Clean();
					SpawnSparks( Hit );
				}
				else
				{
					ActorBreach( Hit.Actor, Hit.Location, i );
				}
			}
		}
	}
}
*/

/////////////
// Natives //
/////////////

//------------------------------------------------------------------------------
// Called when an affectable actor breaches an existing arc.
//------------------------------------------------------------------------------
event bool ActorBreach( Actor A, vector HitLocation, int Index )
{
	if( IsAffected( A ) && A != ElectricArcs[Index].Actor )
	{
		ElectricArcs[Index].Actor  = A;
		if (bLatchAtOrigin)
			ElectricArcs[Index].Offset = vect(0,0,0);
		else
			ElectricArcs[Index].Offset = (HitLocation - A.Location) << A.Rotation;
			
		if( ElectricArcs[Index].Arc != None ) //!!ARL (mdf) accessed none?
			ElectricArcs[Index].Arc.Clean();
			
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
native final function int GetFacingTriIndex( vector PointToFace );
             function int GetFacingTriIndexEx( vector PointToFace )
{
	if( bStartAtOrigin )
		return -1;
	else
		return GetFacingTriIndex( PointToFace );
}

//------------------------------------------------------------------------------
native final function vector GetTriLocation( int TriangleIndex );
             function vector GetTriLocationEx( int TriangleIndex )
{
	if( bStartAtOrigin || TriangleIndex == -1 )
		return GetSource().Location;
	else
		return GetTriLocation( TriangleIndex );
}


///////////////////////
// Public Interfaces //
///////////////////////
/*!!MERGE
//------------------------------------------------------------------------------
// Use this in conjunction with SingleLineCheck.
//------------------------------------------------------------------------------
function InitArcHit( CheckResult Hit )
{
	local int i;

	if( Hit.Actor == None || IsAffected( Hit.Actor ) )
	{
		i = GetUnusedArcIndex();
		if( IsValidArcIndex(i) )
		{
			ElectricArcs[i].iTri		= GetFacingTriIndexEx( Hit.Location );
			ElectricArcs[i].Actor		= Hit.Actor;

			if( (Hit.Actor != None) && (!bLatchAtOrigin) )
				ElectricArcs[i].Offset	= (Hit.Location - Hit.Actor.Location) << Hit.Actor.Rotation;
			else
				ElectricArcs[i].Offset	= vect(0,0,0);

			ElectricArcs[i].Point		= Hit.Location;
			ElectricArcs[i].LifeSpan	@= ArcLifeSpan;
			InitBeam(i);
			SpawnSparks( Hit );
		}
		else
		{
			warn("Used up all ElectricArc slots.  Array size needs to be increased.");
		}
	}
}

//------------------------------------------------------------------------------
// Use this if you just want to attach to a specific Actor (at its center).
//------------------------------------------------------------------------------
function InitArcActor( Actor HitActor )
{
	local int i;

	if( IsAffected( HitActor ) )
	{
		i = GetUnusedArcIndex();
		if( IsValidArcIndex(i) )
		{
			ElectricArcs[i].Actor     = HitActor;
			ElectricArcs[i].Offset    = vect(0,0,0);
			ElectricArcs[i].Point     = HitActor.Location;
			ElectricArcs[i].LifeSpan @= ArcLifeSpan;
			ElectricArcs[i].iTri      = GetFacingTriIndexEx( HitActor.Location );
			InitBeam(i);
		}
		else
		{
			warn("Used up all ElectricArc slots.  Array size needs to be increased.");
		}
	}
}
*/

/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
function Actor GetSource()
{
	if( Source != None ) return Source;
	else                 return Self;
}

//------------------------------------------------------------------------------
final function int GetUnusedArcIndex()
{
	local int i;

	for( i=0; i<ArrayCount(ElectricArcs); i++ )
		if( ElectricArcs[i].LifeSpan <= 0.0 )
			return i;

	return -1;
}

//------------------------------------------------------------------------------
final function bool IsValidArcIndex( int i )
{
	return i >=0 && i < ArrayCount(ElectricArcs);
}
	
//------------------------------------------------------------------------------
final function bool IsAffected( Actor A )
{
	local int i;

	if( A != None )
		for( i=0; i<ArrayCount(AffectedTypes); i++ )
			if( A.IsA( AffectedTypes[i] ) )
				return true;

	return false;
}

//------------------------------------------------------------------------------
function InitBeam( int i )
{
	if( ElectricArcs[i].Arc == None )
	{
		ElectricArcs[i].Arc = Spawn( BeamType, self );

		// Overrides.
		ElectricArcs[i].Arc.BeamTexture			= BeamTexture;
		ElectricArcs[i].Arc.BeamSegLength		= BeamSegLength;
		ElectricArcs[i].Arc.BeamColor			= BeamColor;
		ElectricArcs[i].Arc.BeamWidth			= BeamWidth;
		ElectricArcs[i].Arc.BeamTextureWidth   @= BeamTextureWidth;
		ElectricArcs[i].Arc.NumBeams			= NumBeams;
		ElectricArcs[i].Arc.SpriteJointTexture	= SpriteJointTexture;
		ElectricArcs[i].Arc.SpriteJointSize		= SpriteJointSize;
		ElectricArcs[i].Arc.SpriteJointColor	= SpriteJointColor;
		ElectricArcs[i].Arc.MetaBeamSegLength	= MetaBeamSegLength;
		ElectricArcs[i].Arc.MetaBeamWidth		= MetaBeamWidth;
		ElectricArcs[i].Arc.MetaBeamRefreshTime	= MetaBeamRefreshTime;
		ElectricArcs[i].Arc.DamageTime			= DamageTime;
		ElectricArcs[i].Arc.DamageAmount		= DamageAmount;
		ElectricArcs[i].Arc.DamageMomentum		= DamageMomentum;
		ElectricArcs[i].Arc.DamageType			= DamageType;
		ElectricArcs[i].Arc.DamageEffect		= DamageEffect;
	}
	else
	{
		ElectricArcs[i].Arc.bHidden = false;
		ElectricArcs[i].Arc.DamageTime = DamageTime;
		if (DamageTime > 0.0)
			ElectricArcs[i].Arc.Enable('Tick');
		if( bResetArcOnReuse )
			ElectricArcs[i].Arc.Clean();
	}

	ElectricArcs[i].Arc.SetLocation(Location);
}
/*!!MERGE
//------------------------------------------------------------------------------
function SpawnSparks( CheckResult Hit )
{
	local ParticleSalamander Sparks;

	// Fix ARL: SetBase to Hit.Actor?
	Sparks = ParticleSalamander(class'ParticleGenerator'.static.DynamicCreateNew( Self, class'ParticleSalamander', SparkType, Hit.Location ));
	Sparks.SetRotation( rotator(Hit.Normal) );
	Sparks.ParticleLifeSpan = Sparks.TimerDuration + Sparks.GetMaxLifeSpan();
	Sparks.Trigger( Self, Instigator );
}
*/

defaultproperties
{
	AffectedRadius=1200.000000
	LatchRate=(A=0.100000,B=0.300000)
	ArcLifeSpan=(A=0.100000,B=3.000000)
	AffectedTypes(0)='Pawn'
	AffectedTypes(1)='Decoration'
	AffectedTypes(2)='Projectile'
	DamageRate=(A=50.000000,B=100.000000)
	BeamType=Class'particlesystems.BendiBeamGenerator'
	SparkType="Sparks02.ParticleSalamander0"
	TickDelay=1
	bLatchGeometry=true
	bInitiallyOn=true
	bOn=true
	RemoteRole=ROLE_None
}
