//=============================================================================
// ArtifactHolderDefense.uc
//=============================================================================
class ArtifactHolderDefense extends ArtifactHolder
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	Description="Defense Artifact"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Invisible'
	PickupClass=class'ArtifactDefense'
	InventoryClass=class'ArtifactInvDefense'
	AlternateSkins(0)=Material'XMPT.AFT_InvisibleFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_InvisibleFX_003'
}
