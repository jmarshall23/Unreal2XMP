class RocketTurret extends Turret
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	WeaponType=class'Deployables.WeaponRocketTurret'
	Health=500

	DrawType=DT_Mesh
	Mesh=Mesh'Weapons3rdPK.Turret_Rocket'
	CarcassMesh(0)=StaticMesh'Arch_TurretsM.Small.Deployable_Disabled_TD_02'
	ViewOffset=(Z=34.0)
	
	// Assets
	TrackingSound=Sound'U2XMPA.RocketTurret.RocketTurretTracking'
	//ActionSound=Sound'U2XMPA.RocketTurret.RocketTurretFire'
	ActiveAlertSound=Sound'U2XMPA.RocketTurret.RocketTurretAlert'
	ShutdownSound=Sound'U2XMPA.RocketTurret.RocketTurretShutdown'
	AmbientNoiseSound=Sound'U2XMPA.RocketTurret.RocketTurretAmbient'
	DeploySound=Sound'U2XMPA.RocketTurret.AutoTurretActivate'
	TransientSoundRadius=1000.000000
	Description="Rocket Turret"
	PickupMessage="You picked up a Rocket Turret."
	ExplosionClass=class'ExplosionRocketTurret'

	// AI
	bSplashDamage=true
	ActionRate=2.0
	
	InventoryType=class'RocketTurretDeployable'
	EnergyCostPerSec=0.0020
}
