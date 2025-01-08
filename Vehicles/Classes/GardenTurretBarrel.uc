class GardenTurretBarrel extends XMPVehicleBarrel
	notplaceable;

function OnUse (Actor User)
{
	StationaryTurret(Owner).TryToDrive(Controller(User).Pawn);
}

simulated function PostBeginPlay()
{
	bCollideWorld=false;
	Super.PostBeginPlay();
}

defaultproperties
{
	AlternateSkins(0)=Shader'Arch_TurretsT.Medium.GardenTurretFX_TD_01_Red'
	AlternateSkins(1)=Shader'Arch_TurretsT.Medium.GardenTurretFX_TD_01_Blue'
	MyWeaponClass="Weapons.GardenMG"
	//bCollideWorld=true
	DefaultWeaponNames(0)="Weapons.GardenMG"
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.GardenTurretGun_TD_01'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=240.79,Y=0,Z=276)
	//FiringOffset=(X=300,Y=0,Z=0)
	FiringOffset=(X=550,Y=0,Z=0)
	//RemoteRole=ROLE_None
	CollisionHeight=276
	bUnlit=true
	//bNetInitialRotation=true
}