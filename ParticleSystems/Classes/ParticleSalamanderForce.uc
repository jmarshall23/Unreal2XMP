//=============================================================================
// $Author: Aleiby $
// $Date: 5/30/02 1:05a $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleSalamanderForce.uc
// Author:	Aaron R Leiby
// Date:	7 May 2003
//------------------------------------------------------------------------------
// Description:	Force that spawns particles over time from other particles.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ParticleSalamanderForce extends Force
	native;

var() float Spread;			// Defines a cone that the particles will be sprayed from.
							// An angle in degress centered on our rotation vector.

var() Range EmisionOffset;	// Distance from the center that particle are emmitted from.

var() range Volume;			// Number of particles emitted per second (per emitting particle).

var() Array<ParticleTemplate> ForceTemplates;

var() bool bInheritVelocity;

//------------------------------------------------------------------------------
native event AddForceTemplate( ParticleTemplate T );


defaultproperties
{
}

