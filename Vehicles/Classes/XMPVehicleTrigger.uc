class XMPVehicleTrigger extends ScriptedSequence
	notplaceable;

var float BTReTriggerDelay;
var	float TriggerTime;
var bool  bCarFlipTrigger;
var int   PassengerLocation;

function Touch( Actor Other )
{
	local Pawn User;

	if( Other.Instigator != None )
	{
		User = Pawn(Other);

		if(User == None)
			return;

		if ( BTReTriggerDelay > 0 )
		{
			if ( Level.TimeSeconds - TriggerTime < BTReTriggerDelay )
				return;
			TriggerTime = Level.TimeSeconds;
		}

		// Send a string message to the toucher.
		if(!bCarFlipTrigger)
			User.ReceiveLocalizedMessage(class'Vehicles.XMPVehicleMessage', 0);
		else
			User.ReceiveLocalizedMessage(class'Vehicles.XMPVehicleMessage', 3);
	}
}

function UsedBy( Pawn user )
{
	// Enter vehicle code
		XMPVehicle(Owner).TryToDrive(User, PassengerLocation);
}

defaultproperties
{
	bHardAttach=True
    bHidden=True
	bCollideActors=false
    bStatic=false
    CollisionRadius=+0080.000000
	CollisionHeight=+0400.000000
	bCollideWhenPlacing=False
	BTReTriggerDelay=0.1
	bOnlyAffectPawns=True
    RemoteRole=ROLE_None
    PassengerLocation=0
}