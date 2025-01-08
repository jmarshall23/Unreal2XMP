//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ElectricGenerator.uc
// Author:	Aaron R Leiby
// Date:	21 August 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class BendiBeamGenerator extends PulseLineGenerator
	placeable
	native;

var() Range MetaBeamSegLength;
var() Range MetaBeamWidth;
var() Range MetaBeamRefreshTime;


defaultproperties
{
	NumBeams=(A=1,B=1)
	MetaBeamSegLength=(A=40.000000,B=60.000000)
	MetaBeamWidth=(A=20.000000,B=35.000000)
	MetaBeamRefreshTime=(A=0.100000,B=0.500000)
	BeamSegLength=(A=8,B=16)
	BeamWidth=(A=3,B=6)
	BeamTextureWidth=12.000000
	SpriteJointSize=(A=6,B=8)
	TemplateClass=class'ParticleSystems.BendiBeamTemplate'
}

