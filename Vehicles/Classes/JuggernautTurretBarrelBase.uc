class JuggernautTurretBarrelBase extends XMPVehicleBarrel
	notplaceable;

var		JuggernautTurretBarrel TurretBarrel;


defaultproperties
{
	bUsable=false
	bUseMe=false
	FiringOffset=(X=269.37,Y=0,Z=7.01)
	MyWeaponClass="Weapons.JuggernautCannon"
	DefaultWeaponNames(0)="Weapons.JuggernautCannon"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.JuggernautTurretBarrelBase_TD_001'
	DriverViewMesh=StaticMesh'VehiclesM.Juggernaut.JuggernautTurretCam_TD_001'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=-69.37,Y=-57.55,Z=7.01)
	AlternateSkins(0)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Blue'
}