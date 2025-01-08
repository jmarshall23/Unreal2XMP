class CarnivalTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
	StationaryTurret(Owner).TryToDrive(Controller(User).Pawn);
}

defaultproperties
{
	MyWeaponClass="Weapons.CarnivalMG"
	DefaultWeaponNames(0)="Weapons.CarnivalMG"
	bBlockKarma=false
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.Turret_PostCannon'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=-125.27,Y=0,Z=44.82)
	FiringOffset=(X=144,Y=0,Z=0)
	bUnlit=true
}