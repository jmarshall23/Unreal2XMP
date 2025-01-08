//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	CollisionSphereForce.uc
// Author:	Aaron R Leiby
// Date:	3 April 2000
//------------------------------------------------------------------------------
// Description:	See ParticleColliderSphere.uc
//------------------------------------------------------------------------------
// How to use this class: For use only by ParticleColliderSpheres.
//------------------------------------------------------------------------------

class CollisionSphereForce extends Force
	native;

var ParticleColliderSphere Parent;

defaultproperties
{
	Priority=7.000000
}

