//=============================================================================
// ArtifactShield.uc
//=============================================================================
class ArtifactShield extends ArtifactPickup;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Armour'
	InventoryType=class'ArtifactInvShield'
	PickupMessage="You picked up the Shield Artifact."
	AlternateSkins(0)=Material'XMPT.AFT_ArmourFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_ArmourFX_003'
}
