//=============================================================================
// $Author: Aleiby $
// $Date: 9/11/02 2:16a $
// $Revision: 9 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	SpriteParticleTemplate.uc
// Author:	Aaron R Leiby
// Date:	9 March 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class SpriteParticleTemplate extends ParticleTemplate
	native;

/////////////////////////
// Initialization data //
/////////////////////////

var()         Material        Sprite;
var()         TextureArray    SpriteArray;
var()         array<int>      SpriteIndices;
var           int             SpriteIndex;	// current sprite index (used internally for linear sprite iteration).

var()         EDrawStyle      Style;

var()         Range           InitialSize;
var()         Range           InitialAlpha;

var()         bool	          bFadeAlphaOnly;	// Don't fade out RGB values.
var()         bool            bAnimateSprite;	// Linearly iterates across sprite indices over lifespan of particle.
var()         bool            bRandomSprite;	// Randomly select indices from the sprite array.
var(Corona)   bool            bAnimateCorona;	// Linearly iterates across corona indices over lifespan of particle.
var(Corona)   bool            bRandomCorona;	// Randomly select indices from the corona array.
var(Corona)   bool            CoronaOcclude;
var(Corona)   bool            bCoronaFadeAlphaOnly;

var(Corona)   Material        Corona;
var(Corona)   EDrawStyle      CoronaStyle;
var(Corona)   float           CoronaScale;		// relative to base sprite size.
var(Corona)   float           CoronaScaleGlow;	// relative to base sprite alpha.
var(Corona)   TextureArray    CoronaArray;
var(Corona)   array<int>      CoronaIndices;
var           int             CoronaIndex;	// current corona index (used internally for linear corona iteration).
var(Corona)   float           CoronaTraceRate;

defaultproperties
{
	ParticleType="SpriteParticle"
	InitialSize=(A=30.000000,B=30.000000)
	InitialAlpha=(A=1.000000,B=1.000000)
	CoronaTraceRate=0.15
}

