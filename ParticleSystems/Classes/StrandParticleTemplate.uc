//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	StrandParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	6 April 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class StrandParticleTemplate extends ParticleTemplate
	native;

/////////////////////////
// Initialization data //
/////////////////////////

var() color           Color;
var() float           ColorVariation;
var() float           Shinyness;	// Yes I spelled it wrong, but 'Shininess' looks stupid.  :P

defaultproperties
{
	ParticleType="StrandParticle"
}

