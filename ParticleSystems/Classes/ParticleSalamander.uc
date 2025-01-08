//=============================================================================
// $Author: Sbrown $
// $Date: 2/06/02 8:47p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleSalamander.uc
// Author:	Aaron R Leiby
// Date:	10 March 2000
//------------------------------------------------------------------------------
// Description:	A Salamander is an elemental being in the theory of Paracelsus 
// inhabiting fire.  A Salamander is also a type of kerosene heater used for
// heating up large areas -- like a warehouse or a circus tent.  What does this
// have to do with particles you ask?  Very little.  But it's a cool name, no?
// Basically, a ParticleSalamander is like a nozzel on the end of a hose which 
// sprays out particles of various types (as specified by the LD).  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ParticleSalamander extends ParticleGenerator
	placeable
	native;

////////////////////////
// Editable variables //
////////////////////////

var() float Spread;			// Defines a cone that the particles will be sprayed from.
							// An angle in degress centered on our rotation vector.

var() Range EmisionOffset;	// Distance from the center that particle are emmitted from.

var() float Volume;			// Number of particles emitted per second.
var   float ParticleTimer;	// Used internally to collect "emission residue" <-- fancy term for rounding errors between ticks.

var() float VolumeScalePct;	// Percentage of the visibility radius/height used to scale the volume of the particle sprayer.
							// A number from 0.0 to 1.0.  (Scaling begins at VolumeScalePct*VisibilityRadius units from
							// the generator, and is linearly interpolated until VisibilityRadius units from the generator.)
							// Scaling goes from Volume to 0 (not Volume to MinVolume).

var() float MinVolume;		// Minimum volume that Level->Engine-Client->ParticleDensity is allowed to scale us to.

var() bool  bLOSClip;		// Clip using line of sight check?

simulated event TurnOn()
{
	Super.TurnOn();
	ParticleTimer = 1.0 / Volume;	// force first particle to spit out on initial tick after turning on.
}

defaultproperties
{
	Texture=Texture'ParticleSystems.Icons.S_ParticleSalamander'
	Style=STY_Translucent
	DrawScale=0.250000
	bDirectional=true
}

