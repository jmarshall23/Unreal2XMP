//=============================================================================
// $Author: Aleiby $
// $Date: 5/02/02 3:27p $
// $Revision: 5 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	TexturedStreakParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	19 July 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class TexturedStreakParticleTemplate extends ParticleTemplate
	native;

/////////////////////////
// Initialization data //
/////////////////////////

var() Material        Texture;	// Divided horizontally into thirds (Head, Middle [looped], Tail).
var() EDrawStyle      Style;

var() Range           Length;	// Number of segments.
var() Range           WidthStart;
var() Range           WidthEnd;

var() Range           InitialAlpha;
var() bool            bFadeAlphaOnly;
var() Array<color>    Colors;


defaultproperties
{
	ParticleType="TexturedStreakParticle"
	InitialAlpha=(A=1.000000,B=1.000000)
	Length=(A=5.000000,B=5.000000)
	WidthStart=(A=10.000000,B=10.000000)
	WidthEnd=(A=5.000000,B=5.000000)
	Texture=Texture'ParticleSystems.Streaks.Streak01'
}

