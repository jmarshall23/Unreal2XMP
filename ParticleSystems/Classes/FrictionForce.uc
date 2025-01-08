//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	FrictionForce.uc
// Author:	Aaron R Leiby
// Date:	27 March 2000
//------------------------------------------------------------------------------
// Description:	Used to dampen particle velocities.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class FrictionForce extends Force
	native;

var() float       Friction;				// (0.0..1.0) Percentage loss of velocity per second.
var() float       MinVelocity;			// If the particle's velocity is slower
												// than this, then it will be set to zero.
var   float       MinVeloSquared;		// Cached internally between frames (optimization).

defaultproperties
{
	Priority=25.000000
	Friction=0.100000
}

