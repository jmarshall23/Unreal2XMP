//=============================================================================
// $Author: Aleiby $
// $Date: 3/08/02 9:51a $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	AnchorForce.uc
// Author:	Aaron R Leiby
// Date:	6 April 2000
//------------------------------------------------------------------------------
// Description:	Locks a particle's position relative to an Actor.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class AnchorForce extends Force
	native;

var()			Actor			RelativeActor;
var() native	ParticleHandle	ParticleA;			// Affected particle.

var()		    vector			RelativeOffset;
var()		    bool			bRotateWithActor;

//------------------------------------------------------------------------------
// Make sure you set the particle's location relative to the RelativeActor
// before calling this function.
// Also make sure bRotateWithActor is set appropriately.  If you change it, 
// you'll have to call this function again for the offsets to work the way 
// you expect.
//------------------------------------------------------------------------------
native simulated function SetParticle( ParticleHandle Affected, Actor Relative );

defaultproperties
{
	Priority=0.000000
}

