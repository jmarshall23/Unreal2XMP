class RaptorTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
XMPVehicle(ConglomerateID).TryToDrive(Controller(User).Pawn, 1);
}

defaultproperties
{
	MyWeaponClass="Weapons.RaptorMG"
	FiringOffset=(X=150,Y=0,Z=0)
	DefaultWeaponNames(0)="Weapons.RaptorMG"
	bUsable=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.RaptorTurret_TD_001'
	DriverViewMesh=StaticMesh'VehiclesM.RaptorGunCam_TD_001'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=0,Y=0,Z=36.5)
	BaseEyeHeight=0;

	AlternateSkins(0)=Shader'VehiclesT.Raptor.RaptorFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Raptor.RaptorFX_TD_01_Blue'
}