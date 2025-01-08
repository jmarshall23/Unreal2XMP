
class VoiceMessage extends LocalMessage;

static function string GetString(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if (RelatedPRI_1?)
		return RelatedPRI_1.PlayerName$": "$Default.MessageText;

	return Super.GetString(Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject);
}

defaultproperties
{
	MessageHolder="MessageHolder"
	Lifetime=6.0
	DrawColor=(R=0,G=255,B=0,A=255)
	MessageWrapX=400
}

