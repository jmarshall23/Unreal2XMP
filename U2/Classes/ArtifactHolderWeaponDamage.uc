//=============================================================================
// ArtifactHolderWeaponDamage.uc
//=============================================================================
class ArtifactHolderWeaponDamage extends ArtifactHolder
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	Description="Weapon Damage Artifact"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Damage'
	PickupClass=class'ArtifactWeaponDamage'
	InventoryClass=class'ArtifactInvWeaponDamage'
	AlternateSkins(0)=Material'XMPT.AFT_DamageFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_DamageFX_003'
}
