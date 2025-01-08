//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleColliderSphere.uc
// Author:	Aaron R Leiby
// Date:	3 April 2000
//------------------------------------------------------------------------------
// Description:	Simple collider using spherical geometry.
//------------------------------------------------------------------------------
// How to use this class:
// + Set CollisionRadius.
// + Toggle bCollectParticleGenerators to attach to nearby generators.
// + Set CullDistance to determine ParticleGenerators affected.
//------------------------------------------------------------------------------

class ParticleColliderSphere extends Actor
	placeable	
	native;

var() float                    Elasticity;	// How well things bounce off of us.  (0.0..1.0)
var() bool                     bCollectParticleGenerators;
var() Array<ParticleGenerator> AffectedSystems;

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.CollisionSphereForce';
}

defaultproperties
{
	CollisionRadius=50.000000
	CullDistance=300.000000
	Elasticity=0.750000
	bHidden=true
}

