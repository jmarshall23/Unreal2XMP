class HarbingerTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
XMPVehicle(ConglomerateID).TryToDrive(Controller(User).Pawn, 1);
}

defaultproperties
{
	bUsable=true
	FiringOffset=(X=-50,Y=0,Z=20)
	DefaultWeaponNames(0)="Weapons.MultiRocket"
	MyWeaponClass="Weapons.MultiRocket"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.HarbingerTurret_TD_001'
	DriverViewMesh=StaticMesh'VehiclesM.HarbingerTurretCam_TD_001'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=60.545,Y=0,Z=41.01)
	AlternateSkins(0)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Blue'
}