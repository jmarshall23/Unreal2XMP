//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	FadeForce.uc
// Author:	Aaron R Leiby
// Date:	15 March 2000
//------------------------------------------------------------------------------
// Description:	Modifies particles' Alpha over time.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class FadeForce extends Force
	native;

var() Range           AlphaRate;	// Unit change in particle alpha per second.
var() byte            Phase;		// Number of 'flips' during the lifespan.

defaultproperties
{
	Priority=90.000000
	Phase=1
}

