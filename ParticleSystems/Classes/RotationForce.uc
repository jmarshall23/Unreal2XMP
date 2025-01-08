//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	RotationRate.uc
// Author:	Aaron R Leiby
// Date:	30 April 2001
//------------------------------------------------------------------------------
// Description:	For use with RotationParticles.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class RotationForce extends Force
	native;

var() Range           RotationRate;
var() byte            Phase;		// Number of 'flips' during the lifespan.

defaultproperties
{
	Priority=90.000000
	Phase=1
}

