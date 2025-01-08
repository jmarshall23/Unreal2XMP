
class LandMineDeployable extends DeployableInventory;

function PostDeploy( actor DeployedActor, bool bAltActivate )
{
	DeployedActor.SetTeam( Owner.GetTeam() );
	DeployedActor.Instigator = Instigator;
}

defaultproperties
{
	ItemName="Land Mines"
	ItemID="M"
	IconIndex=18

	DeployClass=class'LandMines'
	PickupAmmoCount=5

	Mesh=SkeletalMesh'WeaponsK.Mine'

	AmmoName(0)=class'LandMineAmmo'
	AmmoName(1)=class'LandMineAmmo'

	DeploySkillCost=0.0050

	bTossProjectiles=true
	bCanIntersectDUs=true
	bCanIntersectPawns=true
	
	InventoryGroup=4
	GroupOffset=3

	ActivateSound=sound'U2XMPA.LandMines.M_Activate'
	SelectSound=sound'U2XMPA.LandMines.M_Select'
}
