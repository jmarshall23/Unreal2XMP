class LowlandsMediumWingedTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
	StationaryTurret(Owner).TryToDrive(Controller(User).Pawn);
}

defaultproperties
{
	MyWeaponClass="Weapons.WingedCannon"
	DefaultWeaponNames(0)="Weapons.WingedCannon"
	bBlockKarma=true
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.Turret_LowlandsWinged_Cannon'
	DriverViewMesh=StaticMesh'Arch_TurretsM.Turret_LowlandsWinged_1stPCannon'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=0,Y=0,Z=0)
	FiringOffset=(X=220,Y=0,Z=-170)
	bUnlit=true
	//RemoteRole=ROLE_None
	//bNetInitialRotation=true
}