//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ViewVelocityScaledParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	16 April 2001
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ViewVelocityScaledParticleTemplate extends SpriteParticleTemplate
	native;

var() Range ScaleFactor;
var() Range StretchOffset;	// whether it stretches forward or backwards. 0=middle
var() float MaxStretch;

defaultproperties
{
	ParticleType="ViewVelocityScaledParticle"
	InitialSize=(A=1.000000,B=1.000000)
	InitialAlpha=(A=1.000000,B=1.000000)
	ScaleFactor=(A=1.000000,B=1.000000)
	MaxStretch=1024.000000
}

