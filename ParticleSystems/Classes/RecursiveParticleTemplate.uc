//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	RecursiveParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	16 July 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class RecursiveParticleTemplate extends ParticleTemplate
	native;

/////////////////////////
// Initialization data //
/////////////////////////

var() class<ParticleGenerator> ParticleGeneratorClass;
var() ParticleGenerator        ParticleGeneratorTemplate;

var() rotator                  AdditionalRotation;			// Added to calculated direction.
var() Actor                    Target;						// Aim all particles toward this object's location.
var() float                    PrimeTime;


defaultproperties
{
	ParticleType="RecursiveParticle"
}

