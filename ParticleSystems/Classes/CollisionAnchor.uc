//=============================================================================
// $Author: Mfox $
// $Date: 4/30/02 12:27p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	CollisionAnchor.uc
// Author:	Aaron R Leiby
// Date:	29 August 2000
//------------------------------------------------------------------------------
// Description:	Anchors particles at collision location.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class CollisionAnchor extends GeometryCollider;

//------------------------------------------------------------------------------
simulated event CollisionNotification( ParticleHandle P, CheckResult Hit )
{
	local AnchorForce Anchor;

	Anchor = new(Outer)class'AnchorForce';
	Anchor.bRotateWithActor = true;
	Outer.AddForce( Anchor );
	Anchor.SetParticle( P, Outer );
}


defaultproperties
{
	bCollisionNotify=true
}

