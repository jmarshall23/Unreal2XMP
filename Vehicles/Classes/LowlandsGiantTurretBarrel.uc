class LowlandsGiantTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
	StationaryTurret(Owner).TryToDrive(Controller(User).Pawn);
}

defaultproperties
{
	MyWeaponClass="Weapons.LowlandsGiantMortar"
	DefaultWeaponNames(0)="Weapons.LowlandsGiantMortar"
	bBlockKarma=true
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.TurretLowlandsGiantCannon2'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=0,Y=0,Z=0)
	FiringOffset=(X=1727.04,Y=0,Z=-53.67)
	bUnlit=true
	//bNetInitialRotation=true
	//RemoteRole=ROLE_None
}