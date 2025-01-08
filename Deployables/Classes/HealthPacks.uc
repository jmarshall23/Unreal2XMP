
class HealthPacks extends PackBase;

defaultproperties
{
	ItemName="Health Packs"
	ItemID="HP"
	IconIndex=20

	DeployClass=class'U2.HealthPickup'
	DeploySkillCost=0.0050

	ActivateSound=sound'U2XMPA.HealthPacks.HP_Activate'
	SelectSound=sound'U2XMPA.HealthPacks.HP_Select'
	DeployFailedSound=Sound'U2XMPA.HealthPacks.HP_Failed'
}
