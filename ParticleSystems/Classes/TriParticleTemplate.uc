//=============================================================================
// $Author: Aleiby $
// $Date: 5/02/02 3:27p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	TriParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	14 June 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class TriParticleTemplate extends ParticleTemplate
	native;

/////////////////////////
// Initialization data //
/////////////////////////

var() Material          Texture;
var() EDrawStyle        Style;

var() Range             InitialRadius;
var() Range             InitialAlpha;
var() Range             InitialEnergy;

var() bool              bEqualateral;

var   ParticleTemplate  AdditionalParticles;

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.TriForce';
}

defaultproperties
{
	ParticleType="TriParticle"
	InitialRadius=(A=5.000000,B=5.000000)
	InitialAlpha=(A=1.000000,B=1.000000)
}

