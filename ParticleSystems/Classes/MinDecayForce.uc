//=============================================================================
// $Author: Aleiby $
// $Date: 9/05/02 4:47p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	MinDecayForce.uc
// Author:	Aaron R Leiby
// Date:	05 September 2002
//------------------------------------------------------------------------------
// Description:	Limits decay for shortlived particles -- useful for getting
// muzzleflashes, etc. to show up in low framerate situations.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class MinDecayForce extends Force
	native;

var() float MinDecay;	// Set this to the average lifespan of particles -- generally 
						// the amount of time after triggering where the effect looks
						// best.  That way if the tick it too long, it'll just stop 
						// at that point.  Note: this only applies to particles on
						// their first tick.

defaultproperties
{
	Priority=100.000000
	MinDecay=0.02
}

