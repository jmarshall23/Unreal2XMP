class RaptorMessage extends XMPVehicleMessage;

var localized string	GetInDriverMessage;
var localized string	GetInTurretMessage;

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
	}
}

defaultproperties
{

	GetInDriverMessage="Press [Use] To Drive Raptor."
	GetInTurretMessage="Press [Use] To Take the Turret."
}