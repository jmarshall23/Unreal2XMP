class JuggernautFrontTurretBarrel extends XMPVehicleBarrel
	notplaceable;

defaultproperties
{
	FiringOffset=(X=100,Y=0,Z=0)
	MyWeaponClass="Weapons.JuggernautFlameThrower"
	DefaultWeaponNames(0)="Weapons.JuggernautFlameThrower"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.JuggernautFrontTurretBarrel_TD_001'
	bHardAttach=True
	DrawScale=1
	//BarrelOffset=(X=0,Y=0,Z=2.05)
	BarrelOffset=(X=0,Y=0,Z=2.05)
	AlternateSkins(0)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Blue'
}