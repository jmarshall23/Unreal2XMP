//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	InertiaForce.uc
// Author:	Aaron R Leiby
// Date:	1 April 2000
//------------------------------------------------------------------------------
// Description:	Objects in motion tend to stay in motion.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class InertiaForce extends Force
	native;

var() float Weight;	// Desire to change motion. 
					// (0.0: No motion changes --> 1.0: No inertia)

defaultproperties
{
	Priority=30.000000
	Weight=0.300000
}

