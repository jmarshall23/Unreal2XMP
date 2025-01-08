class HarbingerMessage extends XMPVehicleMessage;

var localized string	GetInDriverMessage;
var localized string	GetInTurretMessage;
var localized string	GetInBubbleTurretMessage;

static function string GetString(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject 
	)
{
	switch(Switch)
	{
	case 0:
		return Default.GetInDriverMessage;
		break;

	case 1:
		return Default.GetOutMessage;
		break;

	case 2:
		return Default.TooManyCarsMessage;
		break;

	case 3:
		return Default.FlipCarMessage;
		break;

	case 4:
		return Default.GetInTurretMessage;
		break;

	case 5:
		return Default.GetInBubbleTurretMessage;
		break;
	}
}

defaultproperties
{

	GetInDriverMessage="Press [Use] To Drive Harbinger."
	GetInTurretMessage="Press [Use] To Take the Main Turret."
	GetInBubbleTurretMessage="Press [Use] To Take the Bubble Turret."
}