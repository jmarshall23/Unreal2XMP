//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	PerParticleFriction.uc
// Author:	Aaron R Leiby
// Date:	16 April 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class PerParticleFriction extends Force
	native;

var() Range Friction;	// (0.0..1.0) Percentage loss of velocity per second.

defaultproperties
{
	Priority=25.000000
	Friction=(A=0.100000,B=0.200000)
}

