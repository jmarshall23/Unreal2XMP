//=============================================================================
// ArtifactInvWeaponDamage.uc
//=============================================================================
class ArtifactInvWeaponDamage extends Artifact;

//ARL Import HUD icon
//ARL Import special effect
/*
var() float		DamageIncreasePct;

//TBI Do we want to play a special sound or show an effect whenever damage is increased
// through this artifact?
//-----------------------------------------------------------------------------

function BeginEffect( Pawn Other )
{
	Super.BeginEffect( Other );

	U2Pawn(Other).AddWeaponDamageItem( Self );
}

//-----------------------------------------------------------------------------

function EndEffect( Pawn Other )
{
	Super.EndEffect( Other );

	U2Pawn(Other).RemoveWeaponDamageItem( Self );
}

//-----------------------------------------------------------------------------

function AdjustDamage( out float Damage )
{
	Super.AdjustDamage( Damage );
	//ARL Show visual effect here
	Damage += Damage * DamageIncreasePct;
}

//-----------------------------------------------------------------------------
*/
defaultproperties
{
	ItemName="Weapon Damage Artifact"
	IconIndex=0
	PickupClass=class'ArtifactWeaponDamage'
	ArtifactIconName="ArtifactShowDamage"
	AttachmentClass=class'WeaponDamageAttachment'
//	DamageIncreasePct=0.250000
//ARL Designate special effect
}
