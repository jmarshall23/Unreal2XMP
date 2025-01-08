//=============================================================================
// EarthquakeTrigger.uc
// $Author: Mfox $
// $Date: 7/29/02 7:58p $
// $Revision: 8 $
//
//=============================================================================
class EarthquakeTrigger extends Trigger;

#exec Texture Import File=Textures\QuakeTrigger.pcx Name=S_QuakeTrigger Mips=Off MASKED=1

var() float Duration;				// how long quake should last
var() float ShakeMagnitude;			// roll Magnitudenitude at epicenter, 0 at radius
var() float BaseTossMagnitude;		// toss Magnitudenitude at epicenter, 0 at radius
var() float Radius;					// radius of quake
var() bool	bTossNPCs;				// if true, NPCs in area are tossed around
var() bool	bTossPCs;				// if true, PCs in area are tossed around
var() float	MaxTossedMass;			// if bTossPawns true, only pawns <= this mass are tossed
var() float	TimeBetweenShakes;		// time between each shake
var() string AmbientSoundStr;		// sound for duration of quake
var() string ShakeSoundStr;			// sound for each shake

var private float TimeRemaining;

//-----------------------------------------------------------------------------

state() Waiting
{
	function Trigger( Actor Other, Pawn EventInstigator )
	{
		Instigator = EventInstigator;

		TimeRemaining = Duration;
		SetTimer( TimeBetweenShakes, false );

		if( AmbientSoundStr != "" )
		{
			AmbientSound = Sound( DynamicLoadObject( AmbientSoundStr, class'Sound') );
		}
					
		Disable( 'Trigger' );
		Disable( 'Touch' );
	}

	//-------------------------------------------------------------------------

	function ShakePawn( Pawn P )
	{
		local float DistanceToEpicenter;
		local vector Momentum;
  		
		class'UtilGame'.static.MakeShake( Self, Location, Radius, ShakeMagnitude, Duration );

		if
		(	P.Mass <= MaxTossedMass
		&&	P.Physics == PHYS_Walking
//		&&	(	(P.IsPlayer() && bTossPCs)
//			||	(!P.IsPlayer() && bTossNPCs)
//			)
		)
		{
			DistanceToEpicenter	= VSize( Location - P.Location );

			Momentum = -0.5 * P.Velocity + 100 * VRand();
			Momentum.Z = BaseTossMagnitude/((0.4 * DistanceToEpicenter + 350) * P.Mass);
			Momentum.Z *= 5.0;

			P.AddVelocity( Momentum );
		}
	}

	//-------------------------------------------------------------------------

	event Timer()
	{
		local Pawn P;

		TimeRemaining -= TimeBetweenShakes;
				
		if ( TimeRemaining > TimeBetweenShakes )
		{
			SetTimer( TimeBetweenShakes, false );
			
			if( ShakeSoundStr != "" )
			{
				PlaySound( Sound( DynamicLoadObject( ShakeSoundStr, class'Sound') ), SLOT_Misc );
			}
		  		
			// shake/toss all pawns in the area every 0.5 seconds until quake over
			//!!mdf-tbd: iterate through pawn list (note: none exists currently and might not be faster than radiusactors anyway)
			ForEach RadiusActors( class 'Pawn', P, Radius )
			{
				ShakePawn( P );
			}
		}
		else
		{
			AmbientSound = None;
			Enable( 'Trigger' );
			Enable( 'Touch' );
		}
	}	
}

//-----------------------------------------------------------------------------

defaultproperties
{
     Duration=10.000000
     ShakeMagnitude=60
     BaseTossMagnitude=2500000.000000
     Radius=2048.000000
     bTossNPCs=True
     bTossPCs=True
     MaxTossedMass=500.000000
     TimeBetweenShakes=0.500000
     InitialState='Waiting'
	 Texture=S_QuakeTrigger
}
