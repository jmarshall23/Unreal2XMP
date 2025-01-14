//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	MaxVelocityForce.uc
// Author:	Aaron R Leiby
// Date:	7 April 2000
//------------------------------------------------------------------------------
// Description:	Used to clamp speed (not direction).
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class MaxVelocityForce extends Force
	native;

var() float MaxVelocity;

defaultproperties
{
	Priority=15.000000
	MaxVelocity=100.000000
}

