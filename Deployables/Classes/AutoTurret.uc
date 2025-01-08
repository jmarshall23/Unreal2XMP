class AutoTurret extends Turret
	placeable;

#exec OBJ LOAD File=..\System\ParticleRes\RocketEffects.u
#exec OBJ LOAD File=..\System\ParticleRes\AssaultFX.u
#exec OBJ LOAD File=..\System\ParticleRes\Turret_WarningFX.u

#exec OBJ LOAD File=..\Textures\343T.utx
#exec OBJ LOAD File=..\StaticMeshes\343M.usx

//-----------------------------------------------------------------------------

var ParticleGenerator MuzzleflashA1, MuzzleflashA2;
var ParticleGenerator MuzzleflashB1, MuzzleflashB2;

var byte FireCount, ClientFireCount; // used to let clients know when to do firing effects?

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && ROLE == ROLE_Authority )
		FireCount;
}

//-----------------------------------------------------------------------------

simulated function PostBeginPlay()
{
	if( Level.NetMode != NM_DedicatedServer )
		SetupAttachments();
		
	Super.PostBeginPlay();
}

//-----------------------------------------------------------------------------

simulated function Destroyed()
{
	class'ParticleGenerator'.static.ZapParticleGenerator( MuzzleflashA1 );
	class'ParticleGenerator'.static.ZapParticleGenerator( MuzzleflashA2 );
	class'ParticleGenerator'.static.ZapParticleGenerator( MuzzleflashB1 );
	class'ParticleGenerator'.static.ZapParticleGenerator( MuzzleflashB2 );
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

simulated function SetupAttachments()
{
	MuzzleflashA1 = class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'AssaultFX.ParticleSalamander2', Location );
	MuzzleflashB1 = class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'AssaultFX.ParticleSalamander2', Location );
	MuzzleflashA2 = class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'AssaultFX.ParticleSalamander3', Location );
	MuzzleflashB2 = class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'AssaultFX.ParticleSalamander3', Location );
	
	AttachToBone( MuzzleflashA1, BoneLeftBarrelEnd );
	AttachToBone( MuzzleflashA2, BoneLeftBarrelEnd );
	AttachToBone( MuzzleflashB1, BoneRightBarrelEnd );
	AttachToBone( MuzzleflashB2, BoneRightBarrelEnd );
}

//-----------------------------------------------------------------------------

simulated function UpdateAttachments()
{
	MuzzleflashA1.Trigger( Self, Instigator );
	MuzzleflashA2.Trigger( Self, Instigator );
	MuzzleflashB1.Trigger( Self, Instigator );
	MuzzleflashB2.Trigger( Self, Instigator );
}

//-----------------------------------------------------------------------------

simulated event PostNetReceive()
{
	Super.PostNetReceive();
	
	if( FireCount != ClientFireCount )
	{
		UpdateAttachments();
		ClientFireCount = FireCount;
	}
}

//-----------------------------------------------------------------------------

function FiredWeapon()
{
	Super.FiredWeapon();
	if( Level.NetMode != NM_DedicatedServer	)
		UpdateAttachments();
	else
		FireCount++;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	ViewOffset=(Z=15.0)
	
	DrawType=DT_Mesh
	Mesh=Mesh'Weapons3rdPK.Turret_Cannon'
	CarcassMesh(0)=StaticMesh'Arch_TurretsM.Small.Deployable_Disabled_TD_01'
	WeaponType=class'Deployables.WeaponAutoTurret'
	Health=350
	
	//TurretArmsClass=class'AutoTurretArms'
	//TurretBarrelsClass=class'AutoTurretBarrels'
	CollisionHeight=50.000000
	CollisionRadius=40.000000
	DrawScale=0.75
	
	// Will need to be replaced by the node that is doing the targetting
	TrackingSound=Sound'U2XMPA.AutoTurret.AutoTurretTracking'
	ActionSound=Sound'U2XMPA.AutoTurret.AutoTurretFire'
	ActiveAlertSound=Sound'U2XMPA.AutoTurret.AutoTurretAlert'
	ShutdownSound=Sound'U2XMPA.AutoTurret.AutoTurretShutdown'
	DisabledSound=Sound'U2XMPA.AutoTurret.AutoTurretDisabled'
	EnabledSound=Sound'U2XMPA.AutoTurret.AutoTurretEnabled'
	DeploySound=Sound'U2XMPA.AutoTurret.AutoTurretActivate'
	AmbientNoiseSound=Sound'U2XMPA.AutoTurret.AutoTurretAmbient'
	ExplosionClass=class'ExplosionAutoTurret'

	Description="Auto Turret"
	PickupMessage="You picked up a Auto Turret."
	
	// AI
	PeripheralVision=0.5
	ActionRate=0.1

	InventoryType=class'AutoTurretDeployable'
	EnergyCostPerSec=0.0018
	
	RemoteRole=ROLE_SimulatedProxy
	bNetNotify=true //enable PostNetReceive events
}
