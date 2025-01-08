//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	WindForce.uc
// Author:	Aaron R Leiby
// Date:	17 July 2000
//------------------------------------------------------------------------------
// Description:
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class WindForce extends Force
	native;

var() NewWind Master;

/*
// How often (in seconds) we change windspeed.
var() float MaxImpulseTime;
var() float MinImpulseTime;

// Windspeed -- direction is dictated by rotation.
var() float MaxWindspeed;
var() float MinWindspeed;

var vector Windspeed;
*/

defaultproperties
{
	Priority=51.000000
}

