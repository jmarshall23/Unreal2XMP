class FieldGeneratorDeployable extends DeployableInventory;

defaultproperties
{
	ItemID="FG"
	IconIndex=17

	ItemName="Field Generator"
	DeployClass=class'FieldGenerator'
	GroupOffset=1
	PickupAmmoCount=5

	Mesh=SkeletalMesh'WeaponsK.FieldGenerator'

	AmmoName(0)=class'FieldGeneratorAmmo'
	AmmoName(1)=class'FieldGeneratorAmmo'

	ActivateSound=Sound'U2XMPA.FieldGenerator.FieldGeneratorActivate'
	SelectSound=Sound'U2XMPA.FieldGenerator.FG_Select'

	DeploySkillCost=0.0040
}
