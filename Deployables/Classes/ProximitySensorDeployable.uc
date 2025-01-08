class ProximitySensorDeployable extends DeployableInventory;

defaultproperties
{	
	ItemID="PS"
	IconIndex=18

	ItemName="Proximity Sensor"
	DeployClass=class'ProximitySensor'
	GroupOffset=4

	ActivateSound=sound'U2XMPA.ProximitySensor.ProximitySensorActivate'
	SelectSound=sound'U2XMPA.ProximitySensor.PS_Select'

	DeploySkillCost=0.0040
}
