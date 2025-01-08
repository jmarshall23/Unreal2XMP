//=============================================================================
// $Author: Mfox $
// $Date: 4/30/02 12:27p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	CollisionGlider.uc
// Author:	Aaron R Leiby
// Date:	23 July 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class CollisionGlider extends GeometryCollider;

var() Range   GlideFactor;

//------------------------------------------------------------------------------
simulated event CollisionNotification( ParticleHandle P, CheckResult Hit )
{
	local vector V;
	local rotator Dir;

	Dir = rotator(Hit.Normal);

	V = class'ParticleGenerator'.static.ParticleGetVelocity(P);
	V = V << Dir;
	V.X = 0;
	V = V >> Dir;
	V *= GetRand( GlideFactor );
	class'ParticleGenerator'.static.ParticleSetVelocity(P,V);
}


defaultproperties
{
	GlideFactor=(A=1.500000,B=1.500000)
	Elasticity=(A=0.500000,B=0.500000)
	bCollisionNotify=true
}

