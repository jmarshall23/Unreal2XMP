class RaptorTrigger extends XMPVehicleTrigger
	notplaceable;

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
		if(bCarFlipTrigger)
			User.ReceiveLocalizedMessage(class'Vehicles.RaptorMessage', 3);
		else
		{
			if (PassengerLocation == 0)
				User.ReceiveLocalizedMessage(class'Vehicles.RaptorMessage', 0);
			else
				User.ReceiveLocalizedMessage(class'Vehicles.RaptorMessage', 4);
		}
	}
}

defaultproperties
{
}