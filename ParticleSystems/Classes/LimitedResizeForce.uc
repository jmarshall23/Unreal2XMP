//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	LimitedResizeForce.uc
// Author:	Aaron R Leiby
// Date:	17 April 2000
//------------------------------------------------------------------------------
// Description:	Scales particles' size from 'FromSize' to 'ToSize' over the 
// particles' LifeSpan.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class LimitedResizeForce extends Force
	native;

var() Range FromSize;
var() Range ToSize;

defaultproperties
{
	Priority=80.000000
	FromSize=(A=0.000000,B=0.000000)
	ToSize=(A=1.000000,B=1.000000)
}

