//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	VariablePerParticleGravity.uc
// Author:	Aaron R Leiby
// Date:	27 July 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class VariablePerParticleGravity extends Force
	native;

var() Range StartGravity;
var() Range EndGravity;
var() Range ChangeTime;

defaultproperties
{
	Priority=50.000000
	ChangeTime=(A=1.000000,B=1.000000)
}

