//=============================================================================
// DeploySpinner
//=============================================================================

class DeploySpinner extends Mover
	native
	placeable;

// This class is native so that they can be spawned in the editor in native code (ie: for deploy points)

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPWorldItemsM.DeployPoint.DeployPointWings'
	AlternateSkins(0)=Shader'XMPWorldItemsT.Base.Poly_Pulse_Red'
	AlternateSkins(1)=Shader'XMPWorldItemsT.Base.Poly_Pulse_Blue'
	RepSkinIndex=1

	InitialState=TriggerToggle
	EncroachDamage=5
	MoverEncroachType=ME_IgnoreWhenEncroach
	KeyRot(1)=(Yaw=-360448)
	MoveTime=4
}