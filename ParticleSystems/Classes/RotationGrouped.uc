//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	RotationGrouped.uc
// Author:	Aaron R Leiby
// Date:	14 July 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class RotationGrouped extends Force
	native;

var() Actor   RelativeActor;
var   rotator LastRotation;

defaultproperties
{
	Priority=0.000000
}

