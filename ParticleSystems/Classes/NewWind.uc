//------------------------------------------------------------------------------
// NewWind.uc
// $Author: Mfox $
// $Date: 6/25/02 11:40p $
// $Revision: 4 $
//
// Description:	Simulates wind for a ParticleGenerator.
//------------------------------------------------------------------------------
// How to use this class:
//
// + Place in level.
// + Set ImpulseTime and Windspeed.
// + Link to desired ParticleGenerator via Tag and Event.
//------------------------------------------------------------------------------
class NewWind extends Actor
	placeable
	native;

// How often (in seconds) we change windspeed.
var() float MaxImpulseTime;
var() float MinImpulseTime;

// Windspeed -- direction is dictated by rotation.
var() float MaxWindspeed;
var() float MinWindspeed;

var vector Windspeed;

// Trigger variables.
var() bool bInitiallyOn;
var() bool bOn;
var() float TimerDuration;
var float TriggerTime;

var() Button B_CollectGenerators;
var() Button B_TriggerWind;

var() bool bCollected;

//------------------------------------------------------------------------------
simulated event PreBeginPlay()
{
	Super.PreBeginPlay();

	if( !bCollected )
	{
		CollectGenerators();
	}

	bOn = bInitiallyOn;	
	Timer();
}

//------------------------------------------------------------------------------
simulated function CollectGenerators()
{
	local ParticleGenerator IterPG;
	local WindForce WindForce;

	bCollected = true;

	if( Event != '' )
	{
		foreach AllActors( class'ParticleGenerator', IterPG, Event )
		{
			WindForce = new(IterPG)class'WindForce';
			WindForce.Master = Self;
			IterPG.AddForce( WindForce );
		}
	}
}

//------------------------------------------------------------------------------
simulated function SetSpeed( float Speed )
{
	Windspeed = (vect(1,0,0) * Speed) >> Rotation;
}

//------------------------------------------------------------------------------
simulated function ResetSpeed()
{
	Windspeed = vect(0,0,0);
}

//------------------------------------------------------------------------------
simulated event Timer()
{
	if( bOn )
	{
		SetSpeed( RandRange( MinWindspeed, MaxWindspeed ) );
		SetTimer( RandRange( MinImpulseTime, MaxImpulseTime ), False );
	}
}

//------------------------------------------------------------------------------
simulated function EditTrigger()
{
	GotoState( InitialState );
	Trigger( None, None );
}

//------------------------------------------------------------------------------
// Toggles us on and off when triggered.
//------------------------------------------------------------------------------
simulated state() TriggerToggle
{
	simulated function Trigger( Actor Other, Pawn EventInstigator )
	{
		bOn = !bOn;

		if( bOn )	Timer();
		else		ResetSpeed();
	}
}

//------------------------------------------------------------------------------
// Toggled when Triggered.
// Toggled back to initial state when UnTriggered.
//------------------------------------------------------------------------------
simulated state() TriggerControl
{
	simulated function Trigger( Actor Other, Pawn EventInstigator )
	{
		bOn = !bInitiallyOn;
		
		if( bOn )	Timer();
		else		ResetSpeed();
	}

	simulated function UnTrigger( Actor Other, Pawn EventInstigator )
	{
		bOn = bInitiallyOn;
		
		if( bOn )	Timer();
		else		ResetSpeed();
	}
}

//------------------------------------------------------------------------------
// Toggled when triggered.
// Toggled back to initial state after TimerDuration seconds.
//------------------------------------------------------------------------------
simulated state() TriggerTimed
{
	simulated function Trigger( Actor Other, Pawn EventInstigator )
	{
		TriggerTime = Level.TimeSeconds + TimerDuration;

		bOn = !bInitiallyOn;
		
		if( bOn )	Timer();
		else		ResetSpeed();
	}
	
	simulated event Tick( float DeltaTime )
	{
		Global.Tick( DeltaTime );

		if( Level.TimeSeconds >= TriggerTime )
		{
			bOn = bInitiallyOn;
			
			if( bOn )	Timer();
			else		ResetSpeed();
		}
	}
}

defaultproperties
{
     bInitiallyOn=True
     InitialState='TriggerToggle'
	 B_CollectGenerators="CollectGenerators"
	 B_TriggerWind="EditTrigger"
     Physics=PHYS_None
	 bGameRelevant=true
	 CollisionRadius=+0.00000
	 CollisionHeight=+0.00000
     bHidden=True
     bDirectional=True
     RemoteRole=ROLE_None
     DrawType=DT_Sprite
}
