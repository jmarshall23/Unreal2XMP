class DeployableInventory extends Weapon
	abstract;

//-----------------------------------------------------------------------------

#exec OBJ LOAD File=..\Sounds\U2XMPA.uax

//-----------------------------------------------------------------------------

const DeployDropHeight		= 32.0;
const DeployHorizontalFudge =  8.0;

var class<Actor> DeployClass;
var Sound DeployFailedSound;
var float MinDUSeparation, MinPawnSeparation;
var float MaxDeployDistance;

var float DeploySkillCost;
var bool bTossProjectiles;
var bool bIgnoreOwnerYaw;				// hack to spawn meshes with particle attachments with their default orientation
var bool bCanIntersectDUs;				// allow deployed unit CCs to intersect
var bool bCanIntersectPawns;			// allow deployed unit CCs to intersect
var float EquipResidue;					// resuppling leftover between updates

var() Sound ActivateSound;				//
var() Sound DeActivateSound;			//
var() Sound EffectSound;				//

// disabled
//simulated event RenderOverlays( Canvas Canvas );
simulated event Tick( float DeltaSeconds );
simulated function PlayReloading();

//-----------------------------------------------------------------------------
// hack to make FireLastDown play better (complicated due to delayed ammo usage and potential failure to place)

simulated function PlayFiring()
{
	if( GetAmmoAmount() > 1 )
	{
		PlayAnimEx(AmmoType.AnimFire);
		FireTimer = AmmoType.FireTime;
		if(!AmmoType.bAmbientFireSound)
			PlayWeaponSound(AmmoType.FireSound,GetRand(AmmoType.FirePitch));
	}
	else if( Instigator.ShouldAutoSwitch() )
	{
		PlayAnimEx(AmmoType.AnimFireLastDown);
		FireTimer = AmmoType.FireLastDownTime - DownTime;
		PlayWeaponSound(AmmoType.FireLastRoundSound);
	}
	else
	{
		PlayAnimEx(AmmoType.AnimFireLastRound);
		FireTimer = AmmoType.FireLastRoundTime;
		PlayWeaponSound(AmmoType.FireLastRoundSound);
	}

	IncrementFlashCount();
	PawnAgentEvent('Fire');
}

//-----------------------------------------------------------------------------

//function Projectile ProjectileFire()
function TraceFire( float TraceSpread )
{
	SetTimer(0.5,false);
}

function Timer()
{
	Instigator.MakeNoise( Instigator.SoundDampening );
	AdjustedAim = Instigator.GetAimRotation();
	
	if( ActivateUnit(AmmoIndex == 1) )
		DeployableAmmo(AmmoType).DeployableUseAmmo(1);
}

//-----------------------------------------------------------------------------

function bool PreSetAimingParameters( bool bAltFire, bool bInstant, float FireSpread, class<Projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	return true;
}

//-----------------------------------------------------------------------------

function Activate( bool AltActivate ) // global interface (overridden in the 'Inactive' and 'Actived' states)
{
	assert( false ); // never called
}

//-----------------------------------------------------------------------------

function Deactivate() // global interface (overridden in the 'Inactive' and 'Actived' states)
{
	assert( false ); // never called
}

//-----------------------------------------------------------------------------

function AdjustDamage( out float Damage )
{
	Owner.PlaySound( EffectSound, SLOT_None );
}

//-----------------------------------------------------------------------------

function bool Deploy( vector DeployLocation, rotator DeployRotation, bool bAltActivate )
{
	local Actor A;

	// make sure owner won't be gibbed by item (e.g. if near wall & this causes item to move into player)
	Owner.SetCollision( false, false, false );
	if( bIgnoreOwnerYaw )
		DeployRotation.Yaw = 0;

	A = Spawn( DeployClass, , , DeployLocation, DeployRotation );
	
	Owner.SetCollision( Owner.default.bCollideActors, Owner.default.bBlockActors, Owner.default.bBlockPlayers );

	if( A == None )
		return false;
	
	// if the spawn succeeds, complete the deployment

	// common initialization
	UseSkill( DeploySkillCost );
	A.SetPhysics( PHYS_Falling );
	A.Instigator = Instigator;
	if( bTossProjectiles )
		TossDeployed( A, Instigator );

	// custom initialization
	PostDeploy( A, bAltActivate );

	return true;
}

//-----------------------------------------------------------------------------

function PostDeploy( Actor DeployedActor, bool bAltActivate )
{
	local DeployedUnit DU;
	
	DU = DeployedUnit(DeployedActor);
	if( DU? )
	{
		DU.SetActive( false );
		UpdateDeployedActor( DU );
		DU.Initialize( bAltActivate );
	}
}

//-----------------------------------------------------------------------------

function bool DeployDestinationIsValid( vector DeployLocation, rotator DeployRotation )
{
	local vector EndTrace, Extents, HitLocation, HitNormal;
	local Actor HitActor;

	EndTrace = DeployLocation;
	EndTrace.Z -= Owner.CollisionHeight + DeployDropHeight + MaxDeployDistance;
	Extents.X = DeployClass.default.CollisionRadius;
	Extents.Y = DeployClass.default.CollisionRadius;
	Extents.Z = DeployClass.default.CollisionHeight;

	HitActor = Trace( HitLocation, HitNormal, EndTrace, DeployLocation, true, Extents );

	return (HitActor != None && HitLocation.Z >= EndTrace.Z && HitActor.bWorldGeometry);
}

//-----------------------------------------------------------------------------

function TossDeployed( Actor A, Pawn Parent )
{
	local vector TossVel;
	if( Parent? && Parent.Controller? )
	{
		TossVel = Vector(Parent.Controller.GetViewRotation()) + 0.2*VRand();
		TossVel = TossVel * ((Parent.Velocity Dot TossVel) + 500) + Vect(0,0,200);
		A.Velocity = TossVel;
	}
}

//-----------------------------------------------------------------------------

function bool HasNeededSkill( float SkillNeeded )
{
	return( U2Pawn(Owner)? && U2Pawn(Owner).HasNeededSkill( SkillNeeded ) );
}

//-----------------------------------------------------------------------------

function UseSkill( float SkillNeeded )
{
	if( U2Pawn(Owner)? )
		U2Pawn(Owner).UseSkill( SkillNeeded );
}

//-----------------------------------------------------------------------------

static function bool IsValidDefaultInventory( pawn PlayerPawn )
{
	return( !XMPGame(PlayerPawn.Level.Game).TeamFeatureExists( default.DeployClass.Name ) ||
		    XMPGame(PlayerPawn.Level.Game).IsTeamFeatureOnline( default.DeployClass.Name, PlayerPawn.GetTeam() ));
}

//-----------------------------------------------------------------------------

function bool CanDeploy( vector DeployLocation, rotator DeployRotation )
{
	local Pawn P;
	local float CylinderCylinderDistance;
	
	if( !HasNeededSkill( DeploySkillCost ) )
		return false;
	
	if( !bCanIntersectDUs || !bCanIntersectPawns )
	{
		// don't allow placing DU which overlaps with another DU?
		for( P=Level.PawnList; P!=None; P=P.NextPawn )
		{
			if( P != Self && (!bCanIntersectPawns || DeployedUnit(P) != None) )
			{
				CylinderCylinderDistance = class'Util'.static.GetDistanceBetweenCylinders( 
												DeployLocation, 
												DeployClass.default.CollisionRadius, 
												DeployClass.default.CollisionHeight, 
												P.Location, 
												P.CollisionRadius, 
												P.CollisionHeight );
												
				//AddCylinder( DeployLocation, DeployClass.default.CollisionRadius, DeployClass.default.CollisionHeight, ColorBlue() );
				if( (DeployedUnit(P) != None && CylinderCylinderDistance < MinDUSeparation) ||
				    (DeployedUnit(P) == None && CylinderCylinderDistance < MinPawnSeparation) )
				{
					return false;
				}
			}
		}
	}

	return DeployDestinationIsValid( DeployLocation, DeployRotation );
}

//-----------------------------------------------------------------------------

function bool CalcDeploySpot( out vector DeployLocation, out rotator DeployRotation )
{
	local vector X, Y, Z;

	DeployRotation.Roll = 0;
	DeployRotation.Pitch = 0;
	DeployRotation.Yaw = Owner.Rotation.Yaw;
	
	GetAxes( DeployRotation, X, Y, Z );
	
	DeployLocation = Owner.Location + ( DeployHorizontalFudge + Owner.CollisionRadius + DeployClass.default.CollisionRadius ) * X + DeployDropHeight * Z;

	return true;
}

//-----------------------------------------------------------------------------

function bool ActivateUnit( bool bAltActivate )
{
	local vector DeployLocation;
	local rotator DeployRotation;
	local bool bSuccess;

	if( CalcDeploySpot( DeployLocation, DeployRotation ) &&
		CanDeploy( DeployLocation, DeployRotation ) )
	{
		bSuccess =  Deploy( DeployLocation, DeployRotation, bAltActivate );
	}

	if( !bSuccess )
		Owner.PlaySound( DeployFailedSound, SLOT_None );
	
	return bSuccess;
}

//-----------------------------------------------------------------------------

function UpdateDeployedActor( DeployedUnit A )
{
	assert( Pawn(Owner) != None );
	A.SetTeam( Pawn(Owner).GetTeam() );
}

//-----------------------------------------------------------------------------

auto state Inactive
{
	function Activate( bool AltActivate )
	{
		Owner.PlaySound( ActivateSound );
		GotoState('Activated');
	}

	function Deactivate()
	{
	}
}

//-----------------------------------------------------------------------------

state Activated
{
	function Activate( bool AltActivate )
	{
		Deactivate();
	}

	function Deactivate()
	{
		Owner.PlaySound( DeactivateSound );
		GotoState('Inactive');
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	// text
	ItemName="Deployable Item"
	ItemID="XX"
	Crosshair="Crosshair_XMP"

	// weapon data
	AutoSwitchPriority=0
	InventoryGroup=4
	PickupAmmoCount=10

	MaxDeployDistance=60
	MinDUSeparation=16.0
	MinPawnSeparation=1.0
	DeploySkillCost=0.0040

	// assets
	DrawType=DT_Mesh
	DrawScale=1.0
	PlayerViewOffset=(X=22,Y=7,Z=-45)
	FireOffset=(X=0.0,Y=0.0,Z=0.0)
	DeployFailedSound=Sound'U2XMPA.XMPUnit.DeployFailed'

	AmmoName(0)=class'DeployableAmmo'
	AmmoName(1)=class'DeployableAmmo'

	// additional timing
	ReloadTime=0
	ReloadUnloadedTime=0
	SelectUnloadedTime=0.36
	DownTime=0.6
}

