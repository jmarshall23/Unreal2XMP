//=============================================================================
// $Author: Mfox $
// $Date: 2/27/03 7:06p $
// $Revision: 25 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleGenerator.uc
// Author:	Aaron R Leiby
// Date:	4 March 2000
//------------------------------------------------------------------------------
// Description:	Base class of all particle generators.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ParticleGenerator extends Actor
	abstract
	native;

//////////////
// Versions //
//////////////

const PARTICLEVERSION_Current			= 3;
const PARTICLEVERSION_AutoLit			= 3;
const PARTICLEVERSION_AffectingForces	= 2;
const PARTICLEVERSION_NumParticlesFix	= 1;

///////////////
// Variables //
///////////////

var() editinline OrderedArray<ParticleTemplate> ParticleTemplates;

var() editinline OrderedArray<Force> Forces;		// All forces currently affecting the particles of this system.
													// (Wind and stuff may add its own forces to the list.)
													// (Many ParticleGenerators may all reference the same force.)

////////////////////////
// Advanced variables //
////////////////////////

var(ParticleAdvanced) Array< Class<Force> > DefaultForces;	// Forces automatically installed when spawned.

var(ParticleAdvanced) Array<Name>	IgnoredForces;	// Forces may be assigned a label.  This particle generator
													// will ignore forces with matching labels (unless explicitly
													// added to the Forces array by an LD).  Only enforced via the
													// AddForce function.

var(ParticleAdvanced) float		PrimeTime;			// If you want a ParticleGenerator to appear like it has been
													// running for a while already when it is first spawned, then 
													// set its PrimeTime accordingly (in seconds).  Note: Forces
													// will be evaluated in their current state at start-up.

var(ParticleAdvanced) float		MaxTickSize;		// Tick updates are quantized into chunks of this size (if 
													// they are larger).  This way we can interpolate the generator's
													// movement, rotation or whatever and make it look like it was
													// updated at 120fps (or whatever) rather than the actual 30fps.

var(ParticleAdvanced) float		IdleTime;			// How many seconds pass of not being drawn, before the tick code 
													// gets shut off.
var(ParticleAdvanced) float		ParticleLifeSpan;	// Since LifeSpan doesn't get updated if our zone hasn't been drawn
													// recently, we need to manually track it ourself.  Set this variable
													// when you want the generator to be destroyed in X seconds.

var(ParticleAdvanced) int		ParticleVersion;	// Used to maintain backward compatibility across builds.  Allows
													// old systems to be identified and fixed in newer builds.

var(ParticleAdvanced) bool		bInterpolate;		// Break ticks up into MaxTickSize chuncks and interpolate the
													// location accordingly.  (Makes for smoother distribution of 
													// particles.)

var(ParticleAdvanced) bool		bCollisionBound;	// Use collision cylinder for bounding box visibility instead of
													// true bounding box visiblilty calculated by LocationForces.

var(ParticleAdvanced) bool		bShowBoundingBox;	// Displays the combined bounding boxes of all ParticleTemplates
													// as used by the visibility code.

var(ParticleAdvanced) bool		bLockParticles;		// Disable forces, cleaning, and render relative to the system.

var	bool						bAttachment;		// Set if we are a golem attachment. (DEPRICATED)
var bool						bAttachDelayTick;	// Delay tick till render if attached to a skeletal mesh bone.
var bool						bForceDelayTick;	// Delay tick till render even if not attached to skeletal mesh bone.

var matrix						LockedLocalToWorld;	// Used interally for rendering locked systems relatively.

var transient float		LastDeltaSeconds;	// cached for latent ticks - i.e. weapon attachments.
var transient float     LastTimeDrawn;
var transient vector    LastLocation;
var transient float     LastPitch, LastYaw, LastRoll;
var transient float     LastBoundSizeSquared;


/////////////////////////
// Selection variables //
/////////////////////////

var(ParticleSelection) enum Select
{
	SELECT_Linear,									// Cyclically iterate through the ParticleTemplates array.
	SELECT_Random									// Randomly pick a particle from the ParticleTemplates array.

} SelectionMethod;

var(ParticleSelection) bool bLinearFrequenciesChanged;
													// If you are using Linear Distribution, you need to set this 
													// variable to true if you change any of the templates' weights.  
													// This tells the underlaying code to update itself accordingly.
													// Once the weights have been recalculated (i.e. the next time 
													// the frame is drawn), this variable will automatically be reset 
													// to false.

// Used internally.
var			float LargestWeight;					// The currently largest linear weight.
var			int   TemplateIndex;					// Current index into the particle template list.
var			int   TemplateIteration;				// Keeps track of how many times we have iterated through the 
													// template list in linear distribution.


/////////////////////
// Trigger support //
/////////////////////

var(              ) bool  bOn;
var(TriggerSupport) bool  bInitiallyOn;
var(TriggerSupport) float TimerDuration;
var                 float InternalTimer;

var(TriggerSupport) int   MaxParticles;		// Maximum particles emitted per trigger.
var                 int   ParticleCount;	// Used internally to enforce ParticleCount.

var(TriggerSupport) Button Trig, UnTrig;

// Ramp support.
var(TriggerSupport) float RampUpTime;
var(TriggerSupport) float RampDownTime;
var                 float RampTimer;
var                 float VolumeScale;
var enum Ramp
{
	RAMP_None,
	RAMP_Up,
	RAMP_Down

} RampDir;

// Sound support.
var(Sound) sound SoundOn, SoundOff, SoundLoop;


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
simulated event PreBeginPlay()
{
	Super.PreBeginPlay();
	if( bDeleteMe )
		return;
	bOn = bInitiallyOn;
	if( bLockParticles )
		bHidden = !bOn;
}

//------------------------------------------------------------------------------
simulated event PostBeginPlay()
{
	Super.PostBeginPlay();
	if( bDeleteMe )
		return;
	RegisterExternallyAffectedTemplates();
}

//------------------------------------------------------------------------------
simulated event Destroyed()
{
	UnRegisterExternallyAffectedTemplates();
	Super.Destroyed();
}

//------------------------------------------------------------------------------
function AutoDestroy()
{
	ParticleTemplates.Length = 0;
	Forces.Length = 0;
}

/////////////////////
// Trigger support //
/////////////////////

//------------------------------------------------------------------------------
simulated event TurnOn()
{
	if(RampUpTime>0)
	{
		RampDir=RAMP_Up;
		RampTimer=RampUpTime;
	}
	else
	{
		RampDir=RAMP_None;
		RampTimer=0;
	}

	ParticleCount = MaxParticles;

	bOn = true;
	LastTimeDrawn = Level.TimeSeconds;

	// Force Linear frequencies to be recalculated if used.
//	bLinearFrequenciesChanged = true;	// this causes the concussion grenade ring to disappear (and other effects).  -- this was originally added to fix the kai/tosc transformation crash.
	TemplateIndex = 0;		// just resetting these two vars instead seems to also fix the kai/tosc transformation, but doesn't have the bad side effect of the above change.
	TemplateIteration = 1;

	if( bLockParticles )
		bHidden = false;

	if( SoundOn? )
		PlaySound( SoundOn );
	if( SoundLoop? )
		AmbientSound = SoundLoop;
}

//------------------------------------------------------------------------------
simulated event TurnOff()
{
	switch(RampDir)
	{
	case RAMP_None:
	case RAMP_Up:

		if(RampDownTime>0)
		{
			RampDir=RAMP_Down;
			RampTimer=RampDownTime;
		}
		else
		{
			RampDir=RAMP_None;
			RampTimer=0;
		}

		break;

	case RAMP_Down:

		if(RampUpTime>0)
		{
			RampDir=RAMP_Up;
			RampTimer=RampUpTime;
		}
		else
		{
			RampDir=RAMP_None;
			RampTimer=0;
		}

		break;

	default:
		warn("Invalid RampDir!");
		break;
	}

	bOn = RampDir!=RAMP_None;

	if( bLockParticles )
		bHidden = true;

	if( SoundOff? )
		PlaySound( SoundOff );
	if( SoundLoop? )
		AmbientSound = None;
}

//------------------------------------------------------------------------------
simulated function SendTrigger(){ Trigger(Self,Instigator); }
simulated function SendUnTrigger(){ UnTrigger(Self,Instigator); }

//------------------------------------------------------------------------------
// Toggles us on and off when triggered.
//------------------------------------------------------------------------------
simulated state() TriggerToggle
{
	simulated function Trigger( Actor Other, Pawn EventInstigator )
	{
		Instigator = EventInstigator;

		if(!bOn) TurnOn();
		else     TurnOff();
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
		Instigator = EventInstigator;

		if(!bInitiallyOn) TurnOn();
		else              TurnOff();
	}

	simulated function UnTrigger( Actor Other, Pawn EventInstigator )
	{
		if(bInitiallyOn) TurnOn();
		else             TurnOff();
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
		Instigator = EventInstigator;

		if(!bInitiallyOn) TurnOn();
		else              TurnOff();
		InternalTimer = TimerDuration;	// bOn will be toggled automatically internally.
	}
}

////////////////
// Interfaces //
////////////////

native event AddForce				( Force AddedForce );
native event RemoveForce			( Force RemovedForce );
native event RemoveForceType		( name ClassName, optional bool bAndSubclasses );
native event AddTemplate			( ParticleTemplate T );
native event RemoveTemplate			( ParticleTemplate T );
native event RemoveTemplateType		( name ClassName, optional bool bAndSubclasses );
native event GetParticleTemplates	( out Object T );	// (TArray<UParticleTemplate*>*)
native event Conform				( ParticleGenerator Image, optional bool bDeleteExisting /* =true */ );
native event Duplicate				( out ParticleGenerator D, Level L );
native event ExchangeTemplate		( ParticleTemplate Old, ParticleTemplate New );
native event Clean();
native event LockParticles();
native event UnLockParticles();
native event ValidateComponents();
native event RegisterExternallyAffectedTemplates();
native event UnRegisterExternallyAffectedTemplates();
native function float GetMaxLifeSpan();
native event ManualTick( float DeltaSeconds );

static native function vector ParticleGetLocation( ParticleHandle P );
static native function ParticleSetLocation( ParticleHandle P, vector Loc );
static native function vector ParticleGetVelocity( ParticleHandle P );
static native function ParticleSetVelocity( ParticleHandle P, vector Vel );

function SetAttachment( bool B ){ bAttachment=B; }


/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
// Use this instead of calling Destroy() to give existing particles to 
// run their course.
//------------------------------------------------------------------------------
simulated function ParticleDestroy()
{
	bOn = false;
	ParticleLifeSpan = GetMaxLifeSpan();
}

//------------------------------------------------------------------------------
static function ParticleGenerator CreateNew( actor Helper, ParticleGenerator Template, optional vector Location )
{
	local ParticleGenerator Image;
	local ParticleGenerator NewSystem;

	assert(Template!=None);

	if( Location == vect(0,0,0) )
		Location = Helper.Location;

	Image = Template;

	// Fix ARL: When we spawn a new ParticleGenerator, its PostBeginPlay will call RegisterExternallyAffectedTemplates on all the old templates before they are conformed!!
	// (This is especially bad when those templates are from another package like AssaultFX and cause cross level dependencies which result in garbage collection leaks.)
	// (For now I've hacked it by adding an outer check in ALevelInfo::RegisterAffectedTemplate.)
	// (Might be good to check if this can happen to forces as well.)

	NewSystem = Helper.Spawn( Image.Class,,, Location, Image.Rotation, Image );
	if( NewSystem.bDeleteMe ) NewSystem = None;
	if( NewSystem != None )
	{
		NewSystem.Conform( Image, false );
		NewSystem.RegisterExternallyAffectedTemplates();
		NewSystem.IdleTime = 0.5;
	}

	return NewSystem;
}

//------------------------------------------------------------------------------
static function ParticleGenerator DynamicCreateNew( actor Helper, class Type, string TemplateName, optional vector Location )
{
	local ParticleGenerator Image;
	local ParticleGenerator NewSystem;

	if( Location == vect(0,0,0) )
		Location = Helper.Location;

	Image = ParticleGenerator(DynamicLoadObject( TemplateName, Type ));

	if( Image != None )
	{
		NewSystem = Helper.Spawn( Image.Class,,, Location, Image.Rotation, Image );
		if( NewSystem.bDeleteMe ) NewSystem = None;
		if( NewSystem != None )
		{
			NewSystem.Conform( Image, false );
			NewSystem.RegisterExternallyAffectedTemplates();
			NewSystem.IdleTime = 0.5;
		}
	}
	else
	{
		warn( TemplateName$" not found." );
	}

	return NewSystem;
}

//-----------------------------------------------------------------------------
static function ZapParticleGenerator( out ParticleGenerator PG )
{
	local Actor A;
	
	A = PG;
	ZapActor(A);
	PG = None;
}

defaultproperties
{
	DrawType=DT_Custom
	Style=STY_Translucent
	bUnlit=true
	SelectionMethod=SELECT_Random
	bLinearFrequenciesChanged=true
	bOn=true
	bInitiallyOn=true
	Trig="SendTrigger"
	UnTrig="SendUnTrigger"
	CullDistance=5000.000000
	MaxTickSize=0.008333
	DefaultForces(0)=class'ParticleSystems.DecayForce'
	DefaultForces(1)=class'ParticleSystems.LocatorForce'
	RemoteRole=ROLE_None
	bAcceptsProjectors=true
	bBlockZeroExtentTraces=false
	bBlockNonZeroExtentTraces=false
	LockedLocalToWorld=(XPlane=(X=1,Y=0,Z=0,W=0),YPlane=(X=0,Y=1,Z=0,W=0),ZPlane=(X=0,Y=0,Z=1,W=0),WPlane=(X=0,Y=0,Z=0,W=1))
	bCanTeleport=true
	bAttachDelayTick=true
}

