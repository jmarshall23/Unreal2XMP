//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	RigidBody.uc
// Author:	Aaron R Leiby
// Date:	19 June 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class RigidBody extends Force
	native;

var float  Pitch;
var float  Yaw;
var vector Center;

defaultproperties
{
	Priority=5.000000
}

