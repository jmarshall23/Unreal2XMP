/*-----------------------------------------------------------------------------
Turret base. Controls the turret, including arms and turret barrels which (for
now anyway) are based off of the base.
*/

class Turret extends ProximitySensor
	abstract;

#exec OBJ LOAD File=..\System\ParticleRes\RocketEffects.u
#exec OBJ LOAD File=..\System\ParticleRes\AssaultFX.u
#exec OBJ LOAD File=..\System\ParticleRes\Turret_WarningFX.u
#exec OBJ LOAD File=..\Textures\343T.utx
#exec OBJ LOAD File=..\StaticMeshes\343M.usx

var() class<Weapon> WeaponType;
var() bool bSplashDamage;
var() Sound	TurretMoveSound;
var Weapon MyWeapon;

var() name BonePitch;
var() name BoneYaw;
var() name BoneLeftBarrelEnd;
var() name BoneRightBarrelEnd;
var() name BoneElevatorEnd;
var vector ViewOffset;

var vector AimingVector, ClientAimingVector;
var rotator AimingRotation;

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && Role == ROLE_Authority )
		AimingVector;
}

//-----------------------------------------------------------------------------

function Initialize( bool bAltActivate )
{
	Super.Initialize( bAltActivate );
	
	if( ROLE == ROLE_Authority )
	{
		MyWeapon = Spawn( WeaponType );
		MyWeapon.GiveTo( Self );
		MyWeapon.BringUp();
		AimingRotation = Rotation;
	}
	
	SetAnimAction( 'Base' );
}

//-----------------------------------------------------------------------------

function FiredWeapon()
{
	SetAnimAction( 'Fire' );
}

//-----------------------------------------------------------------------------

simulated function SetYaw( rotator NewRotation )
{
	NewRotation.Pitch = 0;
	NewRotation.Roll = 0;
	SetBoneDirection( BoneYaw, NewRotation );
}

//-----------------------------------------------------------------------------

simulated function SetPitch( rotator NewRotation )
{
	NewRotation.Yaw = -NewRotation.Pitch;
	NewRotation.Roll = 0;
	NewRotation.Pitch = 0;
	SetBoneRotation( BonePitch, NewRotation );
}

//-----------------------------------------------------------------------------

simulated function ApplyAimingRotation()
{		
	local vector LocalDir, InvertedDir;
	local rotator AimRot;

	LocalDir = AimingVector << Rotation;
		
	InvertedDir.X = LocalDir.X;
	InvertedDir.Y = -LocalDir.Y;
	InvertedDir.Z = -LocalDir.Z;

	AimRot = rotator(InvertedDir);

	SetYaw( AimRot );
	SetPitch( AimRot );
}

//-----------------------------------------------------------------------------

function SetAimingRotation( vector AimVector )
{
	AimingVector = AimVector;
	AimingRotation = rotator(AimingVector);
	if( Level.NetMode != NM_DedicatedServer )
		ApplyAimingRotation();
}

//-----------------------------------------------------------------------------

simulated event PostNetReceive()
{
	// something was replicated to client ForceWall
	Super.PostNetReceive();

	if( AimingVector != ClientAimingVector )
	{
		ClientAimingVector = AimingVector;
		ApplyAimingRotation();
	}
}

//-----------------------------------------------------------------------------

function rotator GetAimingRotation()
{
	return AimingRotation;
}

//-----------------------------------------------------------------------------

function ShutDownDeployedUnit( bool bDestroyed, optional Pawn P )
{
	if( MyWeapon? )
	{
		MyWeapon.Destroy();
		MyWeapon = None;
	}
	Super.ShutDownDeployedUnit( bDestroyed, P );
}

//-----------------------------------------------------------------------------

function TakeDamage( int Damage, Pawn InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	if( Health > 0 )
	{
		//DM( "*** " $ Self $ " InVisibilityCone = " $ AutoTurretController(Controller).InVisibilityCone() );
		if( Controller.Enemy == None && 
			InstigatedBy != None && 
			!InstigatedBy.bDeleteMe && 
			TurretController(Controller).InVisibilityCone( InstigatedBy ) )
		{
			Controller.GotoState( 'Action' );
		}
	}

	Super.TakeDamage( Damage, InstigatedBy, HitLocation, Momentum, DamageType );
}

//-----------------------------------------------------------------------------

simulated function vector GetViewLocation()
{
	return GetBoneCoords( BoneElevatorEnd ).Origin - ViewOffset;
}

//-----------------------------------------------------------------------------

simulated event SetAnimAction( name NewAction )
{
	local name CurAnimName;
	local float CurAnimFrame, CurAnimRate;

	if( Health > 0 )
	{
		AnimAction = NewAction;
	    
		GetAnimParams( 0, CurAnimName, CurAnimFrame, CurAnimRate );
		
		if( AnimAction != CurAnimName || !IsAnimating( 0 ) )
		{
			PlayAnim( NewAction, , 0.1 );
			AnimBlendToAlpha( 1, 0.0, 0.05 );
		}
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	AlternateSkins(0)=Shader'Arch_TurretsT.Small.Metl_deployableturret_001_Red'
	AlternateSkins(1)=Shader'Arch_TurretsT.Small.Metl_deployableturret_001_Blue'
	RepSkinIndex=0
	SpamTexture0=Texture'JLSSRed'
	SpamTexture1=Texture'JLSSBlue'
	SpamTexture255=Texture'JLSSGrey'

	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.Small.Deployable_Base_TD_01'

	BonePitch=Elevator
	BoneYaw=Rotator
	BoneLeftBarrelEnd=LeftEnd
	BoneRightBarrelEnd=RightEnd
	BoneElevatorEnd=ElevatorEnd
	
	Health=500
	CollisionHeight=60.000000
	CollisionRadius=50.000000
	DrawScale=1.000000
	PrePivot=(X=0.0,Y=0.0,Z=0.0)
	
	Description="Turret"
	bHasAttack=true
	ControllerClass=class'TurretController'
	SightRadius=2500.000000
	PeripheralVision=0.5
	TimeToTrack=1.000000
	//BaseEyeHeight=25
	
	bNetNotify=true //enable PostNetReceive events
	ClientAimingVector=(X=9999999,Y=9999999,Z=9999999)
}
