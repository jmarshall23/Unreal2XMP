//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	TarpBuilder.uc
// Author:	Aaron R Leiby
// Date:	29 August 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//
// + Set Skin to base texture.
// + Set Texture to Environment map texture, and bShiny to true.
//------------------------------------------------------------------------------

class TarpBuilder extends ParticleGenerator
	placeable
	native;

var() Button Reset;

var() float Width;
var() float Height;

var() int Columns;	// Number of squares across.
var() int Rows;		// Number of squares... the other direction.

var() float Rigidity;

var() bool bAdjustVerts;
var() bool bWireframe;
var() bool bShiny;
var() color EnvBrightness;

// Fix ARL: Move this to ParticleGenerator if it proves worthy of use.
var array<Force> ForcesToAdd;

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.TarpBuilderTemplate';
}

//------------------------------------------------------------------------------
native simulated function Build();
native simulated function AttachTo( Actor Other, int ID );


defaultproperties
{
	bInitiallyOn=false
	DefaultForces=(class'ParticleSystems.LocatorForce',class'ParticleSystems.TarpCollider',class'ParticleSystems.GlobalAccelerator')
	CullDistance=0.000000
	bDirectional=true
	Skins(0)=Texture'DefaultTexture'
	Texture=Texture'DefaultTexture'
	Reset="Clean"
	Width=500.000000
	Height=500.000000
	Columns=25
	Rows=25
	Rigidity=20.000000
	EnvBrightness=(R=255,G=255,B=255)
}

