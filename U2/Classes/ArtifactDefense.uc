//=============================================================================
// ArtifactDefense.uc
//=============================================================================
class ArtifactDefense extends ArtifactPickup;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Invisible'
	InventoryType=class'ArtifactInvDefense'
	PickupMessage="You picked up the Defense Artifact."
	PrePivot=(Z=10.0)
	AlternateSkins(0)=Material'XMPT.AFT_InvisibleFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_InvisibleFX_003'
}
