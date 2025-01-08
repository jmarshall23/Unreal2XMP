class XMPVehicleBarrel extends Pawn
	notplaceable;

var		vector					BarrelOffset;
var		bool	bExternalView;
var		Vector	FiringOffset;
var		bool	bUseMe;
var		String	MyWeaponClass;
var		Weapon		MyWeapon;
var ammunition      MyAmmoType;
var int				BarrelNumber;
var		XMPVehicleTurret	MyTurret;
var				StaticMesh					BaseMesh;
var				StaticMesh					DriverViewMesh;

replication
{
	reliable if(Role==ROLE_Authority)
		ClientKDriverEnter, ClientKDriverLeave;
	reliable if(Role==ROLE_Authority && bNetInitial)
		MyWeapon, BarrelNumber, MyTurret;
}

function name GetWeaponBoneFor(Inventory I)
{
	return '';
}

Simulated function Destroyed()
{
	if ( Level.NetMode == NM_Client )
		return;
	MyWeapon=None;
	MyAmmoType=None;
}

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	if (ConglomerateID != None)
		ConglomerateID.TakeDamage(Damage, instigatedBy, hitlocation, momentum, damageType);
}

function Fire( optional float F )
{
    if( Weapon!=None )
	{
        Weapon.Fire();
		//DMTNS("Firing cost: "$ class(MyWeaponClass).default.EnergyCost);

	}
	/*else if (MyWeapon != None)
	{
		MyWeapon.Fire();
		DMTNS("Firing cost: "$TurretWeapon(MyWeapon).EnergyCost);
	}*/
}

simulated event EncroachedBy( actor Other )
{
}

function PostBeginPlay()
{
	super.PostBeginPlay();

	if (Role == ROLE_Authority)
	{
		GiveWeapon(MyWeaponClass);
		MyWeapon=Weapon;
		//MyAmmoType=Weapon.AmmoTypes[0];
		MyWeapon.SetOwner(self);
		//SetNetDirty(true);
	}
	
}

function PossessedBy(Controller C)
{
	Controller = C;

	if ( C.PlayerReplicationInfo != None )
	{
		PlayerReplicationInfo = C.PlayerReplicationInfo;
		OwnerName = PlayerReplicationInfo.PlayerName;
	}
	if ( C.IsA('PlayerController') )
	{
		if ( Level.NetMode != NM_Standalone )
			RemoteRole = ROLE_AutonomousProxy;
			KVehicle(ConglomerateID).Turrets[BarrelNumber].RemoteRole = ROLE_AutonomousProxy;
			KVehicle(ConglomerateID).Turrets[BarrelNumber].SetOwner(C);
		//BecomeViewTarget();
	}
	else
		RemoteRole = Default.RemoteRole;

	SetOwner(Controller);	// for network replication
}

function UnPossessed()
{	
	PlayerReplicationInfo = None;
	SetOwner(ConglomerateID);
	KVehicle(ConglomerateID).Turrets[BarrelNumber].SetOwner(ConglomerateID);
	Controller = None;
}

function vector EyePosition()
{
	local Rotator OffsetRotation;
	OffsetRotation=Rotation;
	OffsetRotation.Yaw+=32767;
	OffsetRotation.Pitch= -(Rotation.Pitch);
	OffsetRotation.Roll= -(Rotation.Roll);
	return (FiringOffset >> OffsetRotation);
}

// Special calc-view for barrels
simulated function bool SpecialCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation )
{
	local PlayerController pc;

	if (Level.NetMode != NM_DedicatedServer && Controller?)
	{
		pc = PlayerController  (Controller);
		if(pc == None )
		{
			DMTNS("Controller for turret is set to "$Controller$" and is not a PlayerController");
			return false;
		}
		CameraRotation = Rotation;
		CameraRotation.Yaw+=32767;
		CameraRotation.Pitch= -(Rotation.Pitch);
		CameraRotation.Roll= -(Rotation.Roll);
		if (!pc.DrivenVehicle.TurretClass[pc.PassengerNumber].default.bRelativeRotation)
			CameraLocation = pc.DrivenVehicle.Location + pc.DrivenVehicle.TurretClass[pc.PassengerNumber].default.CameraOffset >> pc.DrivenVehicle.Rotation;
		else
			CameraLocation = Location + ((pc.DrivenVehicle.Turrets[pc.PassengerNumber].CameraOffset - (pc.DrivenVehicle.Turrets[pc.PassengerNumber].TurretOffset + BarrelOffset)) >> Rotation);
		CameraLocation = CameraLocation + pc.ShakeOffset;
		return true;
	} 
}

simulated function ClientKDriverEnter(PlayerController pc, int PassengerNum, Pawn p)
{
	local rotator StartingRotation;
	pc.DrivenVehicle = Kvehicle(ConglomerateID);
	P.weapon.putdown();
	pc.bBehindView = true;
	pc.bFixedCamera = false;
	pc.bFreeCamera = true;
	pc.PassengerNumber=PassengerNum;
    pc.TempTurretRotation=KVehicle(ConglomerateID).TurretRotation[PassengerNum];
	Kvehicle(ConglomerateID).MyPassengerNumber = PassengerNum;
    if (!PlayerController(Controller)?)
    	Controller = pc;
	if (!Weapon? && MyWeapon?)
		Weapon = MyWeapon;
	if (Weapon?)
		Weapon.Instigator = Self;
	if (Weapon.ThirdPersonActor?)
		Weapon.ThirdPersonActor.Instigator = Self;
	if( Weapon? && Weapon.CrossHair? )
		pc.SendEvent(Weapon.CrossHair);
	if (MyTurret? && MyTurret.Barrel != Self)
	{
		MyTurret.Barrel=Self;
		if (ConglomerateID?)
			MyTurret.RotateTurret(KVehicle(ConglomerateID).TurretRotation[PassengerNum]);
	}
	if (MyTurret? && Base != MyTurret)
		SetBase(MyTurret);
	if (MyTurret? && RelativeLocation != BarrelOffset)
		SetRelativeLocation(BarrelOffset);
	if (DriverViewMesh?)
		{
			Kvehicle(ConglomerateID).Turrets[PassengerNum].bHidden=true;
			BaseMesh=StaticMesh;
			SetStaticMesh(DriverViewMesh);
		}
	if (MyTurret? && ConglomerateID?)
	{
		MyTurret.RotateTurret(KVehicle(ConglomerateID).TurretRotation[PassengerNum]);
		StartingRotation = Rotation;
		StartingRotation.Yaw += 32768;
		StartingRotation.Pitch *=-1;
		StartingRotation.Roll *= -1;
		pc.SetRotation(StartingRotation);
	}
}

simulated function ClientKDriverLeave(PlayerController pc, Rotator exitLookDir)
{
	pc.bBehindView = false;
	pc.bFixedCamera = true;
	pc.bFreeCamera = false;
	pc.DrivenVehicle=None;
	pc.SetRotation(exitLookDir);
	pc.PassengerNumber=-1;
	if (DriverViewMesh?)
		{
		Kvehicle(ConglomerateID).Turrets[Kvehicle(ConglomerateID).MyPassengerNumber].bHidden=false;
		SetStaticMesh(BaseMesh);
		}
	Kvehicle(ConglomerateID).MyPassengerNumber = -1;
	//pc.setbase(None);
	Controller=None;
}

function ClientReStart()
{
	Velocity = vect(0,0,0);
	Acceleration = vect(0,0,0);
	SetPhysics(PHYS_None);
}

Simulated function Tick(float DeltaTime)
{
	super.tick(DeltaTime);
	if (MyTurret? && RelativeLocation != BarrelOffset)
		SetBase(MyTurret);
		SetRelativeLocation(BarrelOffset);
}

simulated function bool PointOfView()
{
	return true;
}

simulated function PostNetBeginPlay()
{
	super.PostNetBeginPlay();
	Acceleration = vect(0, 0, 0);
	SetPhysics(PHYS_None);
	if (!Weapon? && MyWeapon?)
	{
		Weapon = MyWeapon;
	}

	if (MyTurret? && MyTurret.Barrel != Self)
	{
		MyTurret.Barrel=Self;
		if (ConglomerateID?)
			MyTurret.RotateTurret(KVehicle(ConglomerateID).TurretRotation[BarrelNumber]);
	}
	if (MyTurret? && Base != MyTurret)
		SetBase(MyTurret);
	if (MyTurret? && RelativeLocation != BarrelOffset)
		SetBase(MyTurret);
		SetRelativeLocation(BarrelOffset);
	if (StationaryTurret(ConglomerateID)?)
	{
		if (StationaryTurret(ConglomerateID).Barrel != Self)
			StationaryTurret(ConglomerateID).Barrel = Self;
		if (RelativeLocation != BarrelOffset)
			SetRelativeLocation(BarrelOffset);
	}
}

simulated event postnetreceive()
{
	super.postnetreceive();
	if (!Weapon? && MyWeapon?)
	{
		Weapon = MyWeapon;
	}

	if (MyTurret? && MyTurret.Barrel != Self)
		MyTurret.Barrel=Self;
	if (MyTurret? && Base != MyTurret)
		SetBase(MyTurret);
	if (MyTurret? && RelativeLocation != BarrelOffset)
		SetRelativeLocation(BarrelOffset);
	if ((StationaryTurret(ConglomerateID)?) && RelativeLocation != BarrelOffset)
		SetRelativeLocation(BarrelOffset);
}

function interface float GetHealthPct()				{ return KVehicle(ConglomerateID).GetHealthPct(); }
function interface float GetHealth()				{ return KVehicle(ConglomerateID).GetHealth(); }

function interface bool GetPassengerEnabled0()		{ return KVehicle(ConglomerateID).GetPassengerEnabled0(); }
function interface bool GetPassengerEnabled1()		{ return KVehicle(ConglomerateID).GetPassengerEnabled1(); }
function interface bool GetPassengerEnabled2()		{ return KVehicle(ConglomerateID).GetPassengerEnabled2(); }

function interface string GetPassengerName0()		{ return KVehicle(ConglomerateID).GetPassengerName0(); }
function interface string GetPassengerName1()		{ return KVehicle(ConglomerateID).GetPassengerName1(); }
function interface string GetPassengerName2()		{ return KVehicle(ConglomerateID).GetPassengerName2(); }

function interface int GetPassengerClassIndex0()	{ return KVehicle(ConglomerateID).GetPassengerClassIndex0(); }
function interface int GetPassengerClassIndex1()	{ return KVehicle(ConglomerateID).GetPassengerClassIndex1(); }
function interface int GetPassengerClassIndex2()	{ return KVehicle(ConglomerateID).GetPassengerClassIndex2(); }

function interface int GetPassengerIconIndex0()		{ return KVehicle(ConglomerateID).GetPassengerIconIndex0(); }
function interface int GetPassengerIconIndex1()		{ return KVehicle(ConglomerateID).GetPassengerIconIndex1(); }
function interface int GetPassengerIconIndex2()		{ return KVehicle(ConglomerateID).GetPassengerIconIndex2(); }

defaultproperties
{
	bStasis=False
	bUseEnabled=false
	bCrawler=true
	MyWeaponClass="Weapons.RaptorMG"
	bCanPickupInventory=false
	bUseMe=true
	bCollideWorld=false
	bBlockActors=false
	bCollideActors=true	
	bBlockPlayers=false
	bBlockZeroExtentTraces=true
	bExternalView=false
	BarrelOffset=(X=0,Y=0,Z=0)
	DrawType=DT_StaticMesh
	StaticMesh=None
	bHardAttach=True
	DrawScale=1
	RemoteRole=ROLE_SimulatedProxy
	bNetInitialRotation=false
	bSpecialCalcView=True
	bCanBeBaseForPawns=true
	ControllerClass=None
	Physics=PHYS_Flying
	Velocity=(X=0,Y=0,Z=0)
	Acceleration=(X=0,Y=0,Z=0)
	bShouldBaseAtStartup=true
	bNetNotify=true
	bOnlyRelevantToOwner=false
	bSimGravityDisabled=true
	bIgnoreEncroachers=true
	bCollideWhenPlacing=false
	bBlockKarma=false
	LandMovementState=PlayerDriving
	//bAlwaysRelevant=True
	bOwnerNoSee=false
}