class HarbingerBubbleTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
XMPVehicle(ConglomerateID).TryToDrive(Controller(User).Pawn, 2);
}

defaultproperties
{
	bUsable=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.HarbingerBubbleTurret_TD_001'
	DriverViewMesh=StaticMesh'VehiclesM.HarbingerBubbleTurretCam_TD_001'
	FiringOffset=(X=100,Y=0,Z=-20)
	DefaultWeaponNames(0)="Weapons.EnergyCannon"
	MyWeaponClass="Weapons.EnergyCannon"
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=0.00,Y=0.00,Z=0)
	AlternateSkins(0)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Blue'
}