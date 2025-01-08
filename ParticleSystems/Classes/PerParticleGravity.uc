//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	PerParticleGravity.uc
// Author:	Aaron R Leiby
// Date:	16 April 2000
//------------------------------------------------------------------------------
// Description:	Applies an acceleration in the -Z direction on a per particle
// basis.  Each particle is assigned a random gravity value within the given
// range.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class PerParticleGravity extends Force
	native;

var() Range Gravity;
var() bool  bUseZoneGravity;

defaultproperties
{
	Priority=50.000000
}

