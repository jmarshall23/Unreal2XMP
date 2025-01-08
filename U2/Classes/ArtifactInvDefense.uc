//=============================================================================
// ArtifactInvDefense.uc
//=============================================================================

class ArtifactInvDefense extends Artifact;

defaultproperties
{
	ItemName="Defense Artifact"
	IconIndex=1
	PickupClass=class'ArtifactDefense'
	ArtifactIconName="ArtifactShowDefense"
	AttachmentClass=class'DefenseAttachment'
}