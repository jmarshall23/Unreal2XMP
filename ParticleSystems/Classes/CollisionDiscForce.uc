//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	CollisionDiscForce.uc
// Author:	Aaron R Leiby
// Date:	10 April 2000
//------------------------------------------------------------------------------
// Description:	See ParticleColliderDisc.uc
//------------------------------------------------------------------------------
// How to use this class: For use only by ParticleColliderDiscs.
//------------------------------------------------------------------------------

class CollisionDiscForce extends Force
	native;

var ParticleColliderDisc Parent;

defaultproperties
{
	Priority=7.000000
}

