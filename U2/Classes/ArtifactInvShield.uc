//=============================================================================
// ArtifactInvShield.uc
//=============================================================================
class ArtifactInvShield extends Artifact;

//ARL Import HUD icon
//ARL Import visual effect if necessary
/*
var() float		DamageDecreasePct;

// Do we want to play a special sound or show an effect whenever damage is reduced
// through this artifact?
//-----------------------------------------------------------------------------

function BeginEffect( Pawn Other )
{
	Super.BeginEffect( Other );

	U2Pawn(Other).AddAdditionalDamageFilter( Self );
}

//-----------------------------------------------------------------------------

function EndEffect( Pawn Other )
{
	Super.EndEffect( Other );

	U2Pawn(Other).RemoveArmorItem( Self );
}

//-----------------------------------------------------------------------------

function AdjustDamage( out float Damage )
{
	Super.AdjustDamage( Damage );
	//ARL Show visual effect here
	Damage -= Damage * DamageDecreasePct;
}

//-----------------------------------------------------------------------------
*/
defaultproperties
{
	ItemName="Shield Artifact"
	IconIndex=3
	PickupClass=class'ArtifactShield'
	ArtifactIconName="ArtifactShowShield"
//	DamageDecreasePct=0.250000
}