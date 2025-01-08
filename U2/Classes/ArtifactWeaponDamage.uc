//=============================================================================
// ArtifactWeaponDamage.uc
//=============================================================================
class ArtifactWeaponDamage extends ArtifactPickup;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Damage'
	InventoryType=class'ArtifactInvWeaponDamage'
	PickupMessage="You picked up the Weapon Damage Artifact."
	AlternateSkins(0)=Material'XMPT.AFT_DamageFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_DamageFX_003'
}
