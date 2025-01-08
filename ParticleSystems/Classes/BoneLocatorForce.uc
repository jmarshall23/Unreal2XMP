//=============================================================================
// $Author: Aleiby $
// $Date: 3/08/02 9:51a $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	BoneLocatorForce.uc
// Author:	Aaron R Leiby
// Date:	5 December 2001
//------------------------------------------------------------------------------
// Description:	Used in conjunction with ParticleSkeleton to keep anchor 
// particles attached to bones.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class BoneLocatorForce extends Force within ParticleSkeleton
	native;

var() int BoneIndex;
var() native ParticleHandle ParticleA;

//------------------------------------------------------------------------------
native simulated function SetParticle( ParticleHandle Affected );


defaultproperties
{
	Priority=20.000000
}

