//=============================================================================
// ArtifactHealth.uc
//=============================================================================
class ArtifactHealth extends ArtifactPickup;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Health'
	InventoryType=class'ArtifactInvHealth'
	PickupMessage="You picked up the Health Artifact."
	AlternateSkins(0)=Material'XMPT.AFT_HealthFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_HealthFX_003'
}
