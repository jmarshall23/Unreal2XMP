//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ResizeForce.uc
// Author:	Aaron R Leiby
// Date:	15 March 2000
//------------------------------------------------------------------------------
// Description:	Modifies particles' Size over time.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ResizeForce extends Force
	native;

var() Range           SizeRate;		// Unit change in particle size per second.
var() byte            Phase;		// Number of 'flips' during the lifespan.

defaultproperties
{
	Priority=80.000000
	Phase=1
}

