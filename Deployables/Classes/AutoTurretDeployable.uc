class AutoTurretDeployable extends DeployableInventory;

defaultproperties
{
	ItemID="AT"
	IconIndex=15

	ItemName="Auto Turret"
	DeployClass=class'AutoTurret'
	GroupOffset=3
	PickupAmmoCount=1

	Mesh=SkeletalMesh'WeaponsK.Turret'

	AmmoName(0)=class'AutoTurretAmmo'
	AmmoName(1)=class'AutoTurretAmmo'

	ActivateSound=sound'U2XMPA.AutoTurret.AutoTurretActivate'
	SelectSound=sound'U2XMPA.AutoTurret.AT_Select'

	DeploySkillCost=0.0050
}
