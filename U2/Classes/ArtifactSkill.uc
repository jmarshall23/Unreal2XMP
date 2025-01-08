//=============================================================================
// ArtifactSkill.uc
//=============================================================================
class ArtifactSkill extends ArtifactPickup;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Locator'
	InventoryType=class'ArtifactInvSkill'
	PickupMessage="You picked up the Skill Artifact."
	AlternateSkins(0)=Material'XMPT.AFT_LocatorFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_LocatorFX_003'
}
