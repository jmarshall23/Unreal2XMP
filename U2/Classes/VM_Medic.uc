
class VM_Medic extends VoiceMessage;

static function ClientReceive( 
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if (RelatedPRI_1?) RelatedPRI_1.RequestMedicTime = P.Level.TimeSeconds + 6.0;
	Super.ClientReceive(P,Switch,RelatedPRI_1,RelatedPRI_2,OptionalObject);
}

defaultproperties
{
	MessageText="I need a medic!"
	MessageSound=sound'U2A.VoiceMenu.VMRequestMedic'
	MessageFont=font'HUD_Fonts.EuroExt14'
}

