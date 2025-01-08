//=============================================================================
// ArtifactHolderSkill.uc
//=============================================================================
class ArtifactHolderSkill extends ArtifactHolder
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	Description="Skill Artifact"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Locator'
	PickupClass=class'ArtifactSkill'
	InventoryClass=class'ArtifactInvSkill'
	AlternateSkins(0)=Material'XMPT.AFT_LocatorFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_LocatorFX_003'
}
