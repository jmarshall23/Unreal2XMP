//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	GlobalAccelerator.uc
// Author:	Aaron R Leiby
// Date:	24 March 2000
//------------------------------------------------------------------------------
// Description:	Applys a linear force that affects a particle's velocity over 
//				time.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class GlobalAccelerator extends Force
	native;

var() vector Force;
var() bool   bUseZoneGravity;

defaultproperties
{
	Priority=50.000000
}

