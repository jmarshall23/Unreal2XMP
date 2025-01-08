
class TripLaser extends PulseLineGenerator
	placeable;

//-----------------------------------------------------------------------------

var TripLaserProxy EndProxy, MotionDetector;

//-----------------------------------------------------------------------------

simulated function AddConnectionEx( vector Start, vector End, actor EndBase )
{
	local int i;
	i = Connections.length;
	Connections.length = i+1;
	SetEndpoints( Start, End, i );

	EndProxy = Spawn(class'TripLaserProxy',,,End);
	EndProxy.SetBase( Owner );
	Connections[i].EndActor = EndProxy;
	//AddCylinder( EndProxy.Location, 12, 12, ColorRed() );

	if( Level.NetMode != NM_Client )
	{
		MotionDetector = Spawn(class'TripLaserProxy',,,End);
		MotionDetector.SetBase( EndBase );
		MotionDetector.bProjTarget = true;
		//AddCylinder( MotionDetector.Location, 8, 8, ColorBlue() );
	}
}

//------------------------------------------------------------------------------

event Tick( float DeltaTime )
{
	local Actor HitActor;
	local vector HitLocation, HitNormal;
	local int i;

	if( !bOn || DamageTime <= 0 )
	{
		Disable('Tick');
		return;
	}

	DamageTimer += DeltaTime;

	if( DamageTimer >= DamageTime )
	{
		DamageTimer = 0;

		for( i=0; i<Connections.length; i++ )
		{
			HitActor = Trace( HitLocation, HitNormal, Connections[i].End, Connections[i].Start, true );
			HandleHitActor( i, HitActor, HitLocation, HitNormal );
		}
	}
}

//-----------------------------------------------------------------------------

function HandleHitActor( int ConnectionIndex, actor HitActor, vector HitLocation, vector HitNormal )
{
	// either the mine of the laser end point have moved (not relative to each other)
	if( HitActor != MotionDetector )
	{
		//DMTN("Handle hit no motion detector:" @ HitActor);
		Super.HandleHitActor( ConnectionIndex, HitActor, HitLocation, HitNormal );
	}
}

//-----------------------------------------------------------------------------

event PostBeginPlay()
{
	Super.PostBeginPlay();
	if( Level.NetMode == NM_Client )
		Disable('Tick');
	Connections.Length = 0;
}

//-----------------------------------------------------------------------------

event Destroyed()
{
	if( EndProxy? )			{ EndProxy.Destroy();		EndProxy		= None; }
	if( MotionDetector? )	{ MotionDetector.Destroy(); MotionDetector	= None; }
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bBlockKarma=true
}
