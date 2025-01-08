//=============================================================================
// $Author: Aleiby $
// $Date: 3/08/02 9:51a $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	SpringForce.uc
// Author:	Aaron R Leiby
// Date:	27 March 2000
//------------------------------------------------------------------------------
// Description:	Simple spring force to connect two particles.
//------------------------------------------------------------------------------
// How to use this class:
// + These should probably be used in conjunction with a dampening force 
//   (FrictionForce) too keep things from getting too wild.
//------------------------------------------------------------------------------

// Fix ARL: Springs should be evaluated after SlipForces.  But we can't do that
// because currently we are velocity based and need to be evaluated before 
// the LocatorForce -- otherwise we'll never affect the particles' locations
// until the next tick.  SlipForces need to be evaluated after the LocatorForce
// because their main purpose is to fix the "mistakes" that the locator force
// makes (thus keeping the particle positions correctly relative to each other).
//
// Solution: Make a spring force that replaces the locator force, and correctly
// takes occilation into account based on the current velocity.

class SpringForce extends Force
	native;

var()		    float			Stiffness;				// Spring constant. (Larger values == stiffer spring)
var()		    float			SpringLength;			// Automatically set by SetEndpoint, 
														// but may also be set manually.
var() native	ParticleHandle	ParticleA, ParticleB;	// Affected particles.

var const		int			StiffnessPtr;			// Reference to a master spring constant -- only used if set.

//------------------------------------------------------------------------------
// Note: Spring length is automatically calculated when you call this funciton
// based upon the particle's existing relative locations.
//------------------------------------------------------------------------------
native simulated function SetEndpoints( ParticleHandle A, ParticleHandle B );

defaultproperties
{
	Priority=30.000000
	Stiffness=0.333333
	SpringLength=64.000000
}

