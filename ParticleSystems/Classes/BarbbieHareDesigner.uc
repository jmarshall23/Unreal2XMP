//=============================================================================
// $Author: Sbrown $
// $Date: 2/06/02 8:47p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	BarbbieHareDesigner.uc
// Author:	Aaron R Leiby
// Date:	6 April 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class BarbbieHareDesigner extends ParticleGenerator
	placeable
	native;

var() float Spread;			// Arc angle that the hair is generated in.
var() float Radius;			// Distance from the center that the hair is generated.
var() int   NumStrands;		// Number of strands of hair.
var() Range Length;			// Length of a single strand of hair.
var() int   NumSegments;	// Number of segments each strand of hair is broken up into.
var() Range BendStrength;	// Spring coefficient for hair bendiness.
var() Range Stretchiness;	// Spring coefficient for hair stretchiness.

var() bool  bRecalc;		// Toggle this to true when you want the hair regenerated.

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.StrandParticleTemplate';
	Dependant = class'ParticleSystems.AnchorForce';
	Dependant = class'ParticleSystems.SpringForce';
}

defaultproperties
{
	Texture=Texture'ParticleSystems.Icons.S_ParticleHair'
	Style=STY_Masked
	DrawScale=0.250000
	bDirectional=true
	Spread=90.000000
	Radius=50.000000
	NumStrands=100
	Length=(A=100.000000,B=100.000000)
	NumSegments=10
	BendStrength=(A=10.000000,B=10.000000)
	Stretchiness=(A=15.000000,B=15.000000)
}

