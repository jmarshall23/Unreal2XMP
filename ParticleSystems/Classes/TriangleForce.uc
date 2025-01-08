//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	TriangleForce.uc
// Author:	Aaron R Leiby
// Date:	7 July 2000
//------------------------------------------------------------------------------
// Description:	Keeps TriangleParticles shaped correctly.  (This version 
// correctly accounts for torque applied by the forces on the particles.)
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class TriangleForce extends Force
	native;

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.TriangleParticleTemplate';
}

defaultproperties
{
	Priority=5.000000
}

