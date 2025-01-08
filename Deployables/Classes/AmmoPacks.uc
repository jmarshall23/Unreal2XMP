
class AmmoPacks extends PackBase;

defaultproperties
{
	ItemName="Ammo Packs"
	ItemID="AP"
	IconIndex=20

	DeployClass=class'U2.AmmoPickup'
	DeploySkillCost=0.0050

	ActivateSound=sound'U2XMPA.AmmoPacks.AP_Activate'
	SelectSound=sound'U2XMPA.AmmoPacks.AP_Select'
	DeployFailedSound=Sound'U2XMPA.AmmoPacks.AP_Failed'
}
