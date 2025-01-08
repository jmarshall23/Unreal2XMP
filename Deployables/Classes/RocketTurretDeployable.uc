class RocketTurretDeployable extends DeployableInventory;

defaultproperties
{
	ItemID="RT"
	IconIndex=16

	ItemName="Rocket Turret"
	DeployClass=class'RocketTurret'
	GroupOffset=2
	PickupAmmoCount=1

	Mesh=SkeletalMesh'WeaponsK.Turret'

	AmmoName(0)=class'RocketTurretAmmo'
	AmmoName(1)=class'RocketTurretAmmo'

	ActivateSound=sound'U2XMPA.RocketTurret.AutoTurretActivate'
	SelectSound=sound'U2XMPA.RocketTurret.RT_Select'

	DeploySkillCost=0.0050
}
