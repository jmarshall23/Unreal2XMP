//
// A Death Message.
//
// Switch 0: Kill
//	RelatedPRI_1 is the Killer.
//	RelatedPRI_2 is the Victim.
//	OptionalObject is the DamageType Class.
//

class DeathMessage extends LocalMessage;

var(Message) localized string KilledString, SomeoneString;

static function string GetString(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject 
	)
{
	local string KillerName, VictimName;

	if (Class<DamageType>(OptionalObject) == None)
		return "";

	if (RelatedPRI_2 == None)
		VictimName = Default.SomeoneString;
	else
		VictimName = RelatedPRI_2.PlayerName;

	if ( Switch == 1 )
	{
		// suicide
		return class'GameInfo'.Static.ParseKillMessage(
			KillerName, 
			VictimName,
			Class<DamageType>(OptionalObject).Static.SuicideMessage(RelatedPRI_2) );
	}

	if (RelatedPRI_1 == None)
		KillerName = Default.SomeoneString;
	else
		KillerName = RelatedPRI_1.PlayerName;


	if( Switch == 2 )
	{
		// mercy kill
		return class'GameInfo'.Static.ParseKillMessage(
			KillerName, 
			VictimName,
			Class<DamageType>(OptionalObject).Static.MercyKillMessage(RelatedPRI_2) );
	}

	return class'GameInfo'.Static.ParseKillMessage(
		KillerName, 
		VictimName,
		Class<DamageType>(OptionalObject).Static.DeathMessage(RelatedPRI_1, RelatedPRI_2) );
}

defaultproperties
{
	DrawColor=(R=255,G=0,B=0,A=255)
	KilledString="was killed by"
	SomeoneString="someone"
    bIsSpecial=false
	MessageHolder="DeathMessageHolder"
}

