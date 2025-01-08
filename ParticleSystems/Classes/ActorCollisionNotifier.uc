//=============================================================================
// $Author: Aleiby $
// $Date: 7/16/02 3:27p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ActorCollisionNotifier.uc
// Author:	Aaron R Leiby
// Date:	11 April 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class: 
//------------------------------------------------------------------------------

class ActorCollisionNotifier extends Force
	native;

var() float Radius;
var() bool bUseTrace;	// More accurate, but slower.  Good for trying to hit small collision cylinders.
var() bool bDeleteOnContact;	// Destroys the particle when it collides with something.
var() bool bIgnoreInstigator;

//------------------------------------------------------------------------------
event NotifyPenetratingActor( Actor Other, int NumParticles, vector Origin );

defaultproperties
{
	Priority=7.000000
	Radius=20.000000
	bIgnoreInstigator=true
}

