//=============================================================================
// ArtifactHolderHealth.uc
//=============================================================================
class ArtifactHolderHealth extends ArtifactHolder
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	Description="Health Artifact"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Health'
	PickupClass=class'ArtifactHealth'
	InventoryClass=class'ArtifactInvHealth'
	AlternateSkins(0)=Material'XMPT.AFT_HealthFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_HealthFX_003'
}
