class HarbingerDriverTurret extends XMPVehicleTurret
	notplaceable;


defaultproperties
{
	bUsable=false
	bHardAttach=True
	DrawScale=1
	bArticulated=false
	bRelativeRotation=false
	CameraOffset=(X=-15.50,Y=40.08,Z=0)
	RotationCenter=(Pitch=0,Yaw=0,Roll=0)
	RemoteRole=ROLE_SimulatedProxy
	GunBarrelClass=None
	PitchConstraint=4096
	YawConstraint=8192
	MaxRotation=175000
	AlternateSkins(0)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Blue'
}