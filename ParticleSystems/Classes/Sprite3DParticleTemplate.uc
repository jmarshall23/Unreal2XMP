//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	Sprite3DParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	10 April 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class Sprite3DParticleTemplate extends SpriteParticleTemplate
	native;

var() bool bNoClip;		// draw with a screen-z of zero (like a corona).


defaultproperties
{
	ParticleType="SpriteParticle"
	InitialSize=(A=1.000000,B=1.000000)
	InitialAlpha=(A=1.000000,B=1.000000)
}

