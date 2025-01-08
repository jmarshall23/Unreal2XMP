//=============================================================================
// TurretProxyBall.uc
// $Author$
// $Date$
// $Revision$
//=============================================================================

class TurretProxyBall extends Actor
	placeable;

var(TurretProxyBall) Vector BallOffset;

function OnUse( Actor Other )
{
		Owner.OnUse( Other );
}

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
		Owner.TakeDamage(Damage, instigatedBy, hitlocation, momentum, damageType);
}

defaultproperties
{
	BallOffset=(X=0,Y=0,Z=80)
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.TurretControl.TurretControl_Ball_TD_01'
	bUsable=true
	bBlockActors=true
	bCollideActors=true
	bProjTarget=true
	bBlockNonZeroExtentTraces=true
	bBlockKarma=true
	bBlockPlayers=true

	AlternateSkins(0)=Shader'Arch_TurretsT.TurretControl.TurretControlFX_TD_01_Red'
	AlternateSkins(1)=Shader'Arch_TurretsT.TurretControl.TurretControlFX_TD_01_Blue'
}
