class XMPVehicleMessage extends LocalMessage;

var localized string	GetInMessage;
var localized string	GetOutMessage;
var localized string	TooManyCarsMessage;
var localized string	FlipCarMessage;

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
		return Default.GetInMessage;
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
	}
}

defaultproperties
{
	bFadeMessage=True
	bIsSpecial=True
	bIsUnique=True
	Lifetime=6
	bBeep=True
    DrawColor=(R=128,G=128,B=255,A=255)

	GetInMessage="Press [Use] To Enter Vehicle."
	GetOutMessage="Press [Jump] To Exit Vehicle."
	TooManyCarsMessage="Too Many Cars Already!"
	FlipCarMessage="Press [Use] To Flip Vehicle."

    YPos=0.12
}