class LowlandsRoundhouseTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
	StationaryTurret(Owner).TryToDrive(Controller(User).Pawn);
}

defaultproperties
{
	MyWeaponClass="Weapons.RoundHouseMG"
	DefaultWeaponNames(0)="Weapons.RoundHouseMG"
	bBlockKarma=false
	bUsable=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.RoundhouseTurretGunsCannons'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=-124.564,Y=0,Z=0)
	FiringOffset=(X=144,Y=0,Z=0)
	bUnlit=true
	//RemoteRole=ROLE_None
	//bNetInitialRotation=true
}