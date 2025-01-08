
class EnergyPacks extends PackBase;

defaultproperties
{
	ItemName="Energy Packs"
	ItemID="EP"
	IconIndex=20

	DeployClass=class'U2.EnergyPickup'
	DeploySkillCost=0.0050

	ActivateSound=sound'U2XMPA.EnergyPacks.EP_Activate'
	SelectSound=sound'U2XMPA.EnergyPacks.EP_Select'
	DeployFailedSound=Sound'U2XMPA.EnergyPacks.EP_Failed'
}
