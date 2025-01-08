//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	GeometryColliderEx.uc
// Author:	Aaron R Leiby
// Date:	20 November 2001
//------------------------------------------------------------------------------
// Description:	Restricts number of bounces per particle.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class GeometryColliderEx extends GeometryCollider
	native;

var() Range NumBounces;


defaultproperties
{
	bDeleteOnContact=true
	MinIncidence=1.000000
	NumBounces=(A=5.000000,B=5.000000)
}

