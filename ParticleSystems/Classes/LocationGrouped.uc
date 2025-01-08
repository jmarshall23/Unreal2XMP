//=============================================================================
// $Author: Aleiby $
// $Date: 9/05/02 4:49p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	LocationGrouped.uc
// Author:	Aaron R Leiby
// Date:	14 July 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class LocationGrouped extends Force
	native;

var() Actor  RelativeActor;
var   vector LastLocation;

var() bool  bInterpolationCorrection;

defaultproperties
{
	Priority=0.000000
}

