class HarbingerTrigger extends XMPVehicleTrigger
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
			User.ReceiveLocalizedMessage(class'Vehicles.HarbingerMessage', 3);
		else
		{
			if (PassengerLocation == 0)
				User.ReceiveLocalizedMessage(class'Vehicles.HarbingerMessage', 0);
			else if (PassengerLocation == 1)
				User.ReceiveLocalizedMessage(class'Vehicles.HarbingerMessage', 4);
			else User.ReceiveLocalizedMessage(class'Vehicles.HarbingerMessage', 5);
		}
	}
}

defaultproperties
{
}