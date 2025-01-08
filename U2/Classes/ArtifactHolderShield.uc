//=============================================================================
// ArtifactHolderShield.uc
//=============================================================================
class ArtifactHolderShield extends ArtifactHolder
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	Description="Shield Artifact"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Armour'
	PickupClass=class'ArtifactShield'
	InventoryClass=class'ArtifactInvShield'
	AlternateSkins(0)=Material'XMPT.AFT_ArmourFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_ArmourFX_003'
}
