//=============================================================================
// $Author: Aleiby $
// $Date: 5/17/02 8:28p $
// $Revision: 10 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	9 March 2000
//------------------------------------------------------------------------------
// Description:  ParticleTemplates are used to store initialization data instead 
// of Particles or ParticleGenerators.  There is a 1 to 1 relationship of 
// Particle subclasses to ParticleTemplate subclasses.  Each Particle subclass 
// must implement its own ParticleTemplate which is responsible for fully 
// intializing Particles of its associated type (also taking into account that 
// the particle it is inializing could possibly have been recycled, thus we need
// to inialize *everything* just in case).
//------------------------------------------------------------------------------
// How to use this class:
//
// + If you subclass a Particle, you also need to subclass its ParticleTemplate
//   to properly initialize all the variables you add to your subclass.
//------------------------------------------------------------------------------

class ParticleTemplate extends Object within ParticleGenerator
//	abstract
	native;

//////////////////
// Enumerations //
//////////////////

enum EDrawStyle
{
	STYLE_Translucent,
	STYLE_Modulated,
	STYLE_Normal,
	STYLE_Alpha,
	STYLE_AlphaModulate,
	STYLE_Brighten,
	STYLE_Darken,
	STYLE_Masked,
	STYLE_TrueTranslucent,
};

/////////////////////////
// Initialization data //
/////////////////////////

var() Range           InitialSpeed;
var() Range           InitialLifeSpan;
var() Range           StasisTime;		// Number of seconds before forces are allowed to affect a newly created particle.
var() Actor           LifeSpanLimitor;	// LifeSpan calculated as a function of speed and initial distance from this Actor.

////////////////////
// Selection data //
////////////////////

var() float           SelectionWeight;	// Part of the selection criteria.
var   float           Frequency;		// Used internally for linear distributions.
var   float           CumulativeFreq;	// Used internally for linear distributions.

//////////////
// Lighting //
//////////////

var(Lighting) array<Actor>    Lights;			// Add lights to use as lighting.
var(Lighting) array<Actor>    RaytracedLights;	// Add lights to use as lighting (raytraced for shadows).
var(Lighting) bool            bDynamicLit;		// Use lights marked as bLightParticles.
var(Lighting) int             MaxDynamicLights;	// Maximum number of lights to use when colorizing particles.

////////////////
// Additional //
////////////////

var() bool            bExternallyAffected;	// Register as affected by external forces (such as blowers).
var() bool            bReverseDrawOrder;	// If true will draw the particles within this template in reverse order -- that is to say from oldest to newest.
var() bool            bRandomDrawOrder;		// Randomly insert the new particles either at the start or the end of the list.
var() bool            ChangeClass;			// Dummy var for hanging drop down dialog box off of for changing this template's class.
var() float           DrawOrder;			// Order that this template is drawn within our containing System (be sure to hit Sort after changing this value).

////////////////////////
// Internal variables //
////////////////////////

var   const   string          ParticleType;			// Type of particle this template is responsible for initializing.

var native Array<ParticleHandle> ParticleList;		// List of all currently active particles generated by this generator.
													// We can safely assume that all particles in this like will be of
													// class ParticleType.
var() const/*editconst*/ Box	Extents;			// Bounding box.

var() editinline Array<Force>	AffectingForces;	// Templates that this Force affects the particles of.

var const int HardwareLightsUsed;					// Number of Hardware Lights used, so we know how many to un-set

var const native noexport int ParticleClass;		// internal particleClass cache (as specified by ParticleType string).

defaultproperties
{
	ParticleType="Particle"
	InitialLifeSpan=(A=1.000000,B=1.000000)
	SelectionWeight=1.000000
	DrawOrder=1.000000
	MaxDynamicLights=8
}
