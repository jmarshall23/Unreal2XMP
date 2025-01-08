//=============================================================================
// ArtifactHolderStamina.uc
//=============================================================================
class ArtifactHolderStamina extends ArtifactHolder
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	Description="Stamina Artifact"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Speed'
	PickupClass=class'ArtifactStamina'
	InventoryClass=class'ArtifactInvStamina'
	AlternateSkins(0)=Material'XMPT.AFT_SpeedFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_SpeedFX_003'
}
