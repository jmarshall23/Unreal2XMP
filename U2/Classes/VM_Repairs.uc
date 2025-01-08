
class VM_Repairs extends VoiceMessage;

static function ClientReceive( 
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if (RelatedPRI_1?) RelatedPRI_1.RequestRepairsTime = P.Level.TimeSeconds + 6.0;
	Super.ClientReceive(P,Switch,RelatedPRI_1,RelatedPRI_2,OptionalObject);
}

defaultproperties
{
	MessageText="I need repairs!"
	MessageSound=sound'U2A.VoiceMenu.VMRequestRepairs'
	MessageFont=font'HUD_Fonts.EuroExt14'
}

