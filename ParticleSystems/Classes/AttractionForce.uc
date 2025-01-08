//=============================================================================
// $Author: Aleiby $
// $Date: 3/08/02 9:51a $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	AttractionForce.uc
// Author:	Aaron R Leiby
// Date:	5 December 2001
//------------------------------------------------------------------------------
// Description:	Attraction between two particles.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class AttractionForce extends Force
	native;

var()			float			Rigidity;				// Number of seconds it takes for particles to reach each other at any given moment (negative numbers enforce linear rate).
var() native	ParticleHandle	ParticleA, ParticleB;	// Affected particles.

//------------------------------------------------------------------------------
native simulated function SetEndpoints( ParticleHandle A, ParticleHandle B );


defaultproperties
{
	Priority=10.000000
	Rigidity=4.000000
}

