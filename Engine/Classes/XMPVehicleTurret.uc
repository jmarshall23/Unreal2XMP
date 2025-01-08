class XMPVehicleTurret extends Actor
	notplaceable;
	
var		vector					TurretOffset;	
var		Rotator					RotationCenter;
var		int						PitchConstraint;
var		int						YawConstraint;
var		int						RateOfFire;
var		class<Actor>			ProjectileClass;		
var		vector					CameraOffset;
var		vector					Barrel1FiringOffset;
var		vector					Barrel2FiringOffset;
var		class<XMPVehicleBarrel>	GunBarrelClass;
var		XMPVehicleBarrel		Barrel;
var		int						MaxRotation;
var		bool					bArticulated;
var		bool					bRelativeRotation;
var		bool					bUseMe;
var		int						TurretNumber;
var		float					LastTimeRotated;
var		sound					RotateStartSound;
var		sound					RotateSound;
replication
{
	reliable if(Role==ROLE_Authority)
		Barrel, TurretNumber;
}

simulated event EncroachedBy( actor Other )
{

}

simulated event Destroyed()
{

	// Clean up barrels
	if (Barrel?)
	Barrel.Destroy();
	//Super.Destroyed();
}

simulated function RotateTurret( Rotator TurretRotation, optional Rotator OldTurretRotation )
{
	local rotator TurretBaseRotation;
	local rotator TurretGunRotation;
	local bool bCompareToOld;
	if (OldTurretRotation?)
		bCompareToOld=true;

	if (!bArticulated)
	{
		if (TurretRotation != OldTurretRotation || !bCompareToOld)
			SetRelativeRotation(TurretRotation);
	} else 
	{
		TurretBaseRotation=TurretRotation;
		TurretBaseRotation.Pitch=0;
		TurretBaseRotation.Roll=0;
		TurretGunRotation=TurretRotation;
		TurretGunRotation.Roll=0;
		TurretGunRotation.Yaw=0;
		if (!bCompareToOld || TurretRotation.Yaw != OldTurretRotation.Yaw)
			SetRelativeRotation(TurretBaseRotation);
		if( Barrel? && (!bCompareToOld || TurretRotation.Pitch != OldTurretRotation.Pitch)) //mdf: warning spam
		{
			Barrel.SetRelativeRotation(TurretGunRotation);
		}
	}
	//PWC Enable turret rotation sounds if we get some decent sounds, GRANT
/*	if (level.NetMode != NM_DedicatedServer && bCompareToOld && TurretRotation != OldTurretRotation)
	{
		if (LastTimeRotated < (level.timeseconds-1))
			//it's been a while since we last rotated so we'll play the startrotating sound
		{
			if (StartRotateSound?)
				//PLAYSOUND startrotatesound
		} else if (LastTimeRotated < (level.timeseconds - 0.1)
			//play the rotating sound
		{
			if (RotateSound?)
			//PLAYSOUND rotatesound
		}
		//otherwise we're still playing the last sound
		LastTimeRotated = level.timeseconds;
	}*/
}

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
		ConglomerateID.TakeDamage(Damage, instigatedBy, hitlocation, momentum, damageType);
}

simulated event PostNetBeginPlay()
{
	super.PostNetBeginPlay();
	if (ConglomerateID? && KVehicle(ConglomerateID).Turrets[TurretNumber] != Self)
		KVehicle(ConglomerateID).Turrets[TurretNumber] = Self;
	if (Barrel? && Barrel.Base != Self)
		Barrel.SetBase(Self);
	if (Barrel? && Barrel.MyTurret != Self)
		Barrel.MyTurret = Self;
	if (Barrel? && Barrel.RelativeLocation != Barrel.BarrelOffset)
	{
		Barrel.SetBase(Self);
		Barrel.SetRelativeLocation(Barrel.BarrelOffset);
	}
	if (ConglomerateID? && Barrel? && !Barrel.ConglomerateID?)
		Barrel.ConglomerateID = ConglomerateID;
	if (ConglomerateID? && Base != ConglomerateID)
		SetBase(ConglomerateID);
	if (ConglomerateID? && RelativeLocation != TurretOffset)
		SetRelativeLocation(TurretOffset);
	if (ConglomerateID? && Barrel?)
		RotateTurret(KVehicle(ConglomerateID).TurretRotation[TurretNumber]);
}

simulated event postnetreceive()
{
	super.postnetreceive();
	if (ConglomerateID? && KVehicle(ConglomerateID).Turrets[TurretNumber] != Self)
		KVehicle(ConglomerateID).Turrets[TurretNumber] = Self;
	if (Barrel? && Barrel.MyTurret != Self)
		Barrel.MyTurret = Self;
	if (Barrel? && Barrel.Base != Self)
		Barrel.SetBase(Self);
	if (Barrel? && Barrel.RelativeLocation != Barrel.BarrelOffset)
	{
		Barrel.SetBase(Self);
		Barrel.SetRelativeLocation(Barrel.BarrelOffset);
	}
	if (ConglomerateID? && Barrel? && !Barrel.ConglomerateID?)
		Barrel.ConglomerateID = ConglomerateID;
	if (ConglomerateID? && Base != ConglomerateID)
		SetBase(ConglomerateID);
	if (ConglomerateID? && RelativeLocation != TurretOffset)
		SetRelativeLocation(TurretOffset);
	if (ConglomerateID? && Barrel?)
		RotateTurret(KVehicle(ConglomerateID).TurretRotation[TurretNumber]);
}

defaultproperties
{
	bUseMe=false
	bCollideWorld=false
	DrawType=DT_None
	bHardAttach=True
	DrawScale=1
	bArticulated=false
	bRelativeRotation=true
	bUsable=false
	GunBarrelClass=class'XMPVehicleBarrel'
	Barrel1FiringOffset=(X=0,Y=0,Z=0)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	CameraOffset=(X=0,Y=0,Z=0)
	TurretOffset=(X=0,Y=0,Z=0)
	RateOfFire=0.2
	RotationCenter=(Pitch=0,Yaw=0,Roll=0)
	RemoteRole=ROLE_SimulatedProxy
	bNetInitialRotation=false
	MaxRotation=35000
	bShouldBaseAtStartup=true
	Physics=PHYS_Flying
	bReplicateMovement=false
	bOnlyRelevantToOwner=false
	bStasis=False
	bIgnoreEncroachers=true
	//bUpdateSimulatedPosition=true
	bNetNotify=true
	bBlockKarma=false
	//bAlwaysRelevant=True
}