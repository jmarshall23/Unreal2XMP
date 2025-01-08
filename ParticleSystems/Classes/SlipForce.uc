//=============================================================================
// $Author: Aleiby $
// $Date: 3/08/02 9:51a $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	SlipForce.uc
// Author:	Aaron R Leiby
// Date:	30 August 2000
//------------------------------------------------------------------------------
// Description:	Keeps two particles within the given distance of each other.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class SlipForce extends Force
	native;

var()			Range          Distance;
var() native	ParticleHandle ParticleA, ParticleB;

//------------------------------------------------------------------------------
// Set up Distance for you automatically (based on current particle locations).
//------------------------------------------------------------------------------
native simulated function SetEndpoints( ParticleHandle A, ParticleHandle B, optional float SlipDistance );


defaultproperties
{
	Priority=1.000000
}

