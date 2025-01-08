//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	OscillatingPerParticleForce.uc
// Author:	Aaron R Leiby
// Date:	5 November 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class OscillatingPerParticleForce extends Force
	native;

var() Range ForceX;
var() Range ForceY;
var() Range ForceZ;

var() Range PeriodX;
var() Range PeriodY;
var() Range PeriodZ;

var() bool  bAccelerator;	// Set to true if you want this to act as an accelerator rather than a force.

defaultproperties
{
	Priority=30.000000
}

