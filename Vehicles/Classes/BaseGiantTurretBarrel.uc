class BaseGiantTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
	StationaryTurret(Owner).TryToDrive(Controller(User).Pawn);
}

defaultproperties
{
	MyWeaponClass="Weapons.Mortar"
	DefaultWeaponNames(0)="Weapons.Mortar"
	bBlockKarma=true
	bUnlit=true
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.TurretBaseGiantBarrel_TD_001'
	RepSkinIndex=0
	AlternateSkins(0)=Shader'Arch_TurretsT.Giant.TurretBaseGiantFX_TD_Red'
	AlternateSkins(1)=Shader'Arch_TurretsT.Giant.TurretBaseGiantFX_TD_Blue'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=0,Y=0,Z=380.03)
	FiringOffset=(X=600,Y=0,Z=0)
	//RemoteRole=ROLE_None
}