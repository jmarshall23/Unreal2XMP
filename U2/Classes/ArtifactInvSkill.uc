//=============================================================================
// ArtifactInvSkill.uc
//=============================================================================
class ArtifactInvSkill extends Artifact;

//ARL Import HUD icon
/*
var() float		SkillBoost;			// The percentage of SkillMax that Skill is increased

//-----------------------------------------------------------------------------

function BeginEffect( Pawn Other )
{
	// Play Begin Sound
	Super.BeginEffect( Other );

	Other.Skill += Other.Skill * (SkillBoost/default.SkillMax);
}

//-----------------------------------------------------------------------------

function EndEffect( Pawn Other )
{
	// Play End Sound
	Super.EndEffect( Other );

	if( Other.Skill > Other.default.SkillMax )
		Other.Skill = Other.default.SkillMax;
}

//-----------------------------------------------------------------------------
*/
defaultproperties
{
	ItemName="Speed Artifact"
	IconIndex=4
	PickupClass=class'ArtifactSkill'
	ArtifactIconName="ArtifactShowSkill"
	AttachmentClass=class'SkillAttachment'
//	SkillBoost=25
}
