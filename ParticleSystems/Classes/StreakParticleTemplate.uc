//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	StreakParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	28 March 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class StreakParticleTemplate extends ParticleTemplate
	native;

/////////////////////////
// Initialization data //
/////////////////////////

var() color           Color;
var() Range           InitialAlpha;
var() bool            bFadeAlphaOnly;
var() Range           Length;	// Number of segments (integer) constructing 
										// the streak (must be greater than zero).

defaultproperties
{
	ParticleType="StreakParticle"
	InitialAlpha=(A=1.000000,B=1.000000)
	Length=(A=1.000000,B=1.000000)
}

