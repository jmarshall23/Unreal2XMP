//=============================================================================
// $Author: Aleiby $
// $Date: 5/02/02 3:27p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	PointParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	10 March 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class PointParticleTemplate extends ParticleTemplate
	native;

/////////////////////////
// Initialization data //
/////////////////////////

var() EDrawStyle      Style;
var() color           Color;
var() Range           InitialAlpha;

var() bool            bFadeAlphaOnly;

var() Material        Texture;

var() float           Size;

var() bool            Scale;
var() float           ScaleA;
var() float           ScaleB;
var() float           ScaleC;


defaultproperties
{
	ParticleType="PointParticle"
	Color=(R=255,G=255,B=255,A=255)
	InitialAlpha=(A=1.000000,B=1.000000)
	Size=1.000000
	Scale=false
	ScaleA=1.000000
	ScaleB=1.000000
	ScaleC=1.000000
}

