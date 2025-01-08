//=============================================================================
// $Author: Aleiby $
// $Date: 5/30/02 7:59p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	RepulsionForce.uc
// Author:	Aaron R Leiby
// Date:	30 May 2002
//------------------------------------------------------------------------------
// Description:	Repulsion between all affected particles.
// (useful for keeping particles spread out)
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class RepulsionForce extends Force
	native;

var() float			Strength;


defaultproperties
{
	Priority=50.000000
}

