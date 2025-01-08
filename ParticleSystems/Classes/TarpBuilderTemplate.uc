//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	TarpBuilderTemplate.uc
// Author:	Aaron R Leiby
// Date:	29 August 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class TarpBuilderTemplate extends ParticleTemplate within TarpBuilder
	native;

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.AnchorForce';
	Dependant = class'ParticleSystems.SpringForce';
	Dependant = class'ParticleSystems.SlipForce';
	Dependant = class'ParticleSystems.MeshAnchor';
}

defaultproperties
{
	ParticleType="TarpParticle"
}
