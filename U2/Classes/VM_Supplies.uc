
class VM_Supplies extends VoiceMessage;

static function ClientReceive( 
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if (RelatedPRI_1?) RelatedPRI_1.RequestSuppliesTime = P.Level.TimeSeconds + 6.0;
	Super.ClientReceive(P,Switch,RelatedPRI_1,RelatedPRI_2,OptionalObject);
}

defaultproperties
{
	MessageText="I need supplies!"
	MessageSound=sound'U2A.VoiceMenu.VMRequestSupplies'
	MessageFont=font'HUD_Fonts.EuroExt14'
}

