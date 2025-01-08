//=============================================================================
// ArtifactStamina.uc
//=============================================================================
class ArtifactStamina extends ArtifactPickup;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Pickups.Aft_Speed'
	InventoryType=class'ArtifactInvStamina'
	PickupMessage="You picked up the Stamina Artifact."
	AlternateSkins(0)=Material'XMPT.AFT_SpeedFX_002'
	AlternateSkins(1)=Material'XMPT.AFT_SpeedFX_003'
}
