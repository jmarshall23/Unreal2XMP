//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	RotatingParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	30 April 2001
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class RotatingParticleTemplate extends Sprite3DParticleTemplate
	native;

var() Range InitialAngle;

defaultproperties
{
	ParticleType="RotatingParticle"
	InitialSize=(A=1.000000,B=1.000000)
	InitialAlpha=(A=1.000000,B=1.000000)
}

