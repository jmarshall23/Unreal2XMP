class ProximitySensorController extends StationaryPawnControllerOffensive;

const	ScanningState		= 'Scanning';
const	TrackingTargetState = 'TrackingTarget';
const	ActionState			= 'Action';
const	DeactivatedState	= 'Deactivated';
const	BeginLabel			= 'Begin';
const	ActionLabel			= 'Action';
const	TL_Off				= 0;
const	TL_Scanning			= 1;
const	TL_Tracking			= 2;
const	TL_LockedOn			= 3;

//-----------------------------------------------------------------------------

var ProximitySensor PS;
var	float CheckFOVRate, NextCheckFOVTime;

//-----------------------------------------------------------------------------

function Possess( Pawn P )
{
	PS = ProximitySensor( P );
	Super.Possess( P );
}

//-----------------------------------------------------------------------------

function NotifyDeployed()
{	
	if( PS.IsEnabled() )
	{
		if( PS.AmbientSound == None )
			PS.AmbientSound = PS.AmbientNoiseSound;
		GotoState( 'Scanning' );
	}
	else
		Deactivate();
}

//-----------------------------------------------------------------------------

function float GetTrackingRange()
{
	return ( PS.SightRadius * PS.TrackingRangeMultiplier );
}

//-----------------------------------------------------------------------------

function bool ValidEnemy( Pawn Enemy )
{
	if( !Pawn || 
		!Enemy ||
		Pawn.Health <= 0 ||
		Enemy.Health <= 0 ||
		Pawn.SameTeam( Enemy ) )
		return false;
		
	return true;
}

//-----------------------------------------------------------------------------

function bool CanPerformAction()
{
	return( ValidEnemy(Enemy) && 
			VSize(Enemy.Location - Location) <= GetTrackingRange() && 
			PS.IsEnabled() );
}

//-----------------------------------------------------------------------------

function SetTargetingEffect( int TrackLevel );

//-----------------------------------------------------------------------------

function HandleEnemyLost()
{
	Enemy = None;
	GotoState( 'Scanning' );		
}

//-----------------------------------------------------------------------------

function bool InVisibilityCone( Pawn Other )
{
	if( Pawn == None || Other == None )
		return false;
		
	// If this device has a 360 degree visual range
	if( Pawn.PeripheralVision == 0 )
		return true;

	if( (Normal( Other.Location - Pawn.Location ) dot vector( Rotation ) ) < Pawn.PeripheralVision )
		return false;
	else
		return true;
}

//-----------------------------------------------------------------------------

function Activate()
{
	NotifyDeployed();	
}

//-----------------------------------------------------------------------------

function DeActivate()
{
	GotoState( 'Deactivated' );
}

//-----------------------------------------------------------------------------

state Deactivated
{
	ignores SeeEnemy;
	
	event BeginState()
	{
		SetTargetingEffect( TL_Off );
		PS.AmbientSound = None;
	}
}

//-----------------------------------------------------------------------------

auto state Active
{
	function SeeEnemy( Pawn Seen )
	{
		// use this event to control acquiring an enemy
	}
}

//-----------------------------------------------------------------------------

state Scanning
{
	//-------------------------------------------------------------------------

	function SeeEnemy( Pawn Seen )
	{
		if( ValidEnemy( Seen ) && PS.IsActive() && PS.IsEnabled() )
		{
			Enemy = Seen;
			GotoState( 'TrackingTarget' );
		}		
	}

	event BeginState()
	{
		SetTargetingEffect( TL_Scanning );
	}

	event EndState()
	{
		SetTargetingEffect( TL_Off );
	}
}

//-----------------------------------------------------------------------------

state TrackingTarget
{
	ignores SeeEnemy;

	event EnemyNotVisible()
	{
		HandleEnemyLost();
	}

	event EnemyInvalid()
	{
		HandleEnemyLost();
	}
	
	event Tick( float DeltaTime )
	{
		Global.Tick( DeltaTime );

		if( Level.TimeSeconds > NextCheckFOVTime )
		{
			NextCheckFOVTime = Level.TimeSeconds + CheckFOVRate;
			//DMTNS( "Degrees: " $ Acos( (Normal( Enemy.Location - Pawn.Location ) dot vector( Rotation ) ) ) * RadiansToDegrees );
			if( Pawn != None && Enemy != None )
			{
				if( !InVisibilityCone( Enemy ) || !PS.IsEnabled() )
					HandleEnemyLost();
			}
		}
	}

	event BeginState()
	{
		SetTargetingEffect( TL_Tracking );
	}

	event EndState()
	{
		SetTargetingEffect( TL_Off );
	}
	
Begin:
	Pawn.PlaySound( PS.TrackingSound, SLOT_None, 0.15 );
	Sleep( PS.TimeToTrack );
	GotoState( 'Action' );
}

//-----------------------------------------------------------------------------

state Action extends TrackingTarget
{
	ignores SeeEnemy;

	event BeginState()
	{
		SetTargetingEffect( TL_LockedOn );
	}

	event EndState()
	{
		SetTargetingEffect( TL_Off );
		StopAction();
	}
	
	function StopAction();

	function PerformAction()
	{
		Pawn.PlaySound( PS.ActionSound, SLOT_None );
	}

Begin:
	if( PS.ActiveAlertSound != None )
	{
		Pawn.PlaySound( PS.ActiveAlertSound, SLOT_None );
		Sleep( GetSoundDuration( PS.ActiveAlertSound ) );
	}

	if( PS.bAmbientActionSound )
		PS.AmbientSound = PS.ActionSound;

Action:
	if( CanPerformAction() )
		PerformAction();
	else
		HandleEnemyLost();
		
	Sleep( PS.ActionRate );
	Goto( ActionLabel );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	CheckFOVRate=0.5
}