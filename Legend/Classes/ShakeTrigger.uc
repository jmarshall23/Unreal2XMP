//=============================================================================
// ShakeTrigger.uc
// $Author: Aleiby $
// $Date: 8/25/02 11:34p $
// $Revision: 1 $
//
//=============================================================================
class ShakeTrigger extends Trigger;

var() float Duration;
var() float Magnitude;
var() float Radius;

//-----------------------------------------------------------------------------

state() Waiting
{
	function Trigger( actor Other, pawn EventInstigator )
	{
		class'UtilGame'.static.MakeShake( Self, Location, Radius, Magnitude, Duration );
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
     Duration=1.000000
     Magnitude=50
     Radius=1024.000000
     InitialState='Waiting'
}

