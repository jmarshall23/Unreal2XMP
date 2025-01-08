class TurretController extends ProximitySensorController;

var Turret		MyTurret;
var vector		CurrentAimingLocation;			// where the satellite is
var vector		DesiredAimingLocation;			// where we'd like to move the satellite to
var float		MinHitNonPawnDistance;
var int			TrackLevel;
var float		TurningRateDegreesPerSecond;	// rate at which turrent can adjust its aiming direction
var bool		bSettled;

//-----------------------------------------------------------------------------

function vector GetStraightAheadLocation()
{
	return ( MyTurret.Location + 1024.0*vector(Rotation) );
}

//-----------------------------------------------------------------------------

function Possess( Pawn P )
{
	Super.Possess( P );
	MyTurret = Turret( P );
	CurrentAimingLocation = GetStraightAheadLocation();
}

//-----------------------------------------------------------------------------

function SetTargetingEffect( int NewTrackLevel )
{
	if( MyTurret != None )
	{
		if( TrackLevel == NewTrackLevel )
			return;
		TrackLevel = NewTrackLevel;
	}
}

//-----------------------------------------------------------------------------

function vector GetFireOffset()
{
	if( Pawn.Weapon == None )
		return MyTurret.Location;
	else
		return Pawn.Weapon.FireOffset;
}

//-----------------------------------------------------------------------------

function bool OKToHit( Actor HitActor, vector HitLocation, vector HitNormal )
{
	if( Pawn(HitActor) == None && VSize( MyTurret.Location - HitLocation ) < MinHitNonPawnDistance )
		return false;

	if( HitActor == None )
		return true;
		
	if( Pawn(HitActor) == None )
		return HitActor.CanHit( Self );

	if( HitActor == Enemy )
		return true;

	return Pawn.SameTeam( Pawn(HitActor) );
}

//-----------------------------------------------------------------------------

function bool CanPerformAction()
{
	local vector HitLocation, HitNormal;
	local Actor HitActor;
	
	if( !Super.CanPerformAction() )
		return false;
		
	HitActor = Trace( HitLocation, HitNormal, DesiredAimingLocation, GetFireOffset(), true );

	return OKToHit( HitActor, HitLocation, HitNormal );
}

//-----------------------------------------------------------------------------
// Returns true if aiming changed, false otherwise.

function UpdateAiming( float DeltaTime )
{
	local vector CurrentAimingDirectionNormal, DesiredAimingDirectionNormal;
	local vector Axis;
	local float Degrees, MaxDegrees;
	local vector StartLocation;

	// we would like to aim at TargetLocation but we're not allowed to
	// turn faster than a certain rate.
	if( CurrentAimingLocation != DesiredAimingLocation )
	{
		StartLocation = MyTurret.GetViewLocation();
		CurrentAimingDirectionNormal = vector(MyTurret.GetAimingRotation()); //Normal( CurrentAimingLocation - StartLocation );
		DesiredAimingDirectionNormal = Normal( DesiredAimingLocation - StartLocation );
		
		Degrees = ACos( CurrentAimingDirectionNormal dot DesiredAimingDirectionNormal ) * RadiansToDegrees;
		MaxDegrees = TurningRateDegreesPerSecond*DeltaTime;
		
		if( Degrees > MaxDegrees )
		{
			Axis = Normal( CurrentAimingDirectionNormal cross DesiredAimingDirectionNormal );
			CurrentAimingLocation = StartLocation + 1024*Normal( RotateAngleAxis( CurrentAimingDirectionNormal, Axis, MaxDegrees*DegreesToRotationUnits ) );
		}
		else
		{
			CurrentAimingLocation = DesiredAimingLocation;
		}

		MyTurret.SetAimingRotation( CurrentAimingLocation - StartLocation );
	}
}

//-----------------------------------------------------------------------------

function SetDesiredAimingLocation( vector NewDesiredAimingLocation )
{
	DesiredAimingLocation = NewDesiredAimingLocation;
	//AddCylinder( DesiredAimingLocation, 8, 8, ColorPink() );
}

//-----------------------------------------------------------------------------

function rotator GetAimRotation()
{
	return MyTurret.GetAimingRotation();
}

//-----------------------------------------------------------------------------

function rotator GetViewRotation()
{
	return MyTurret.GetAimingRotation();
}

//-----------------------------------------------------------------------------

function bool ValidEnemy( Pawn Enemy )
{
	local vector EnemyVector;

	if( !MyTurret )
		return false;
		
	if( !Super.ValidEnemy( Enemy ) )
		return false;
		
	EnemyVector = Normal( Enemy.Location - MyTurret.Location );
	if( EnemyVector dot vector(Rotation) < MyTurret.PeripheralVision )
		return false;

	return true;
}

//-----------------------------------------------------------------------------

state Scanning
{
	event Tick( float DeltaTime )
	{
		Global.Tick( DeltaTime );
		UpdateAiming( DeltaTime );
	}
	
	//-------------------------------------------------------------------------

	event BeginState()
	{
		Super.BeginState();
		SetDesiredAimingLocation( GetStraightAheadLocation() );
	}

Begin:	
	if( !bSettled )
	{
		WaitForLanding();
		bSettled = true;
		CurrentAimingLocation = GetStraightAheadLocation();
	}
}

//-----------------------------------------------------------------------------

state TrackingTarget
{
	ignores SeeEnemy;

	event Tick( float DeltaTime )
	{
		if( ValidEnemy( Enemy ) )
		{
			SetDesiredAimingLocation( Enemy.Location );
			UpdateAiming( DeltaTime );
		}
		else
			HandleEnemyLost();
			
		Super.Tick( DeltaTime );
	}
	
	event BeginState()
	{
		if( MyTurret.TurretMoveSound != None )
			PlaySound( MyTurret.TurretMoveSound, SLOT_None );
	}
}

//-----------------------------------------------------------------------------

state Action
{
	//-------------------------------------------------------------------------

	function StopAction()
	{
		//if( MyTurret != None )
		//	MyTurret.PlayAnim( 'FireEnd' );
		if( Pawn != None )
			Pawn.Weapon.GotoState( 'Idle' );
	}

	//-------------------------------------------------------------------------

	function PerformAction()
	{	
		//MyTurret.PlayAnim('Fire');
		Pawn.Weapon.Fire();
		MyTurret.FiredWeapon();
	}

	//-------------------------------------------------------------------------

	event Tick( float DeltaTime )
	{
		if( ValidEnemy( Enemy ) )
		{
			SetDesiredAimingLocation( Enemy.Location );
			UpdateAiming( DeltaTime );
		}
		else
			HandleEnemyLost();
			
		Global.Tick( DeltaTime );
	}
}

//-----------------------------------------------------------------------------

state Deactivated
{
	ignores SeeEnemy;
	
	//-------------------------------------------------------------------------

	event Tick( float DeltaTime )
	{
		Global.Tick( DeltaTime );
		UpdateAiming( DeltaTime );
	}
	
	//-------------------------------------------------------------------------

	event BeginState()
	{	
		local rotator SpecialRotation;

		Super.BeginState();
		
		SpecialRotation = Rotation;
		SpecialRotation.Pitch = -8000;
		SetDesiredAimingLocation( MyTurret.Location + 1024.0*vector(SpecialRotation) );
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	MinHitNonPawnDistance=512
	TurningRateDegreesPerSecond=270
}