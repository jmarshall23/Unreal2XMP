
class PackBase extends DeployableInventory;

//-----------------------------------------------------------------------------

function bool CanDeploy( vector DeployLocation, rotator DeployRotation )
{
	return( Super.CanDeploy(DeployLocation, DeployRotation) &&
			XMPGame(Level.Game).IsTeamFeatureOnline( 'PackBase', Owner.GetTeam() ) );
}

//-----------------------------------------------------------------------------

function PostDeploy( actor DeployedActor, bool bAltActivate )
{
	local Pickup PU;
	PU = Pickup(DeployedActor);
	if( PU? )
	{
		PU.InitDroppedPickupFor(None);
		PU.SetOwner( Pawn(Owner).Controller );
	}
}

//-----------------------------------------------------------------------------

function bool DeployDestinationIsValid( vector DeployLocation, rotator DeployRotation )
{
	return true;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bTossProjectiles=true
	bIgnoreOwnerYaw=true
	bCanIntersectDUs=true
	bCanIntersectPawns=true
	PickupAmmoCount=19998	// so ammo looks "full"

	Mesh=SkeletalMesh'WeaponsK.Pickups'

	AmmoName(0)=class'PackBaseAmmo'
	AmmoName(1)=class'PackBaseAmmo'

	InventoryGroup=5
	GroupOffset=1
}
