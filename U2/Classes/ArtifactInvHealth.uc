//=============================================================================
// ArtifactInvHealth.uc
//=============================================================================
class ArtifactInvHealth extends Artifact;

defaultproperties
{
	ItemName="Health Artifact"
	IconIndex=2
	PickupClass=class'ArtifactHealth'
	ArtifactIconName="ArtifactShowHealth"
	AttachmentClass=class'HealthAttachment'
}