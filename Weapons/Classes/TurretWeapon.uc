class TurretWeapon extends Weapon;

var	float	EnergyCost;
var	Actor	MyThirdPersonActor;
var sound	OfflineSound;

replication
{
	reliable if(Role==ROLE_Authority && bNetInitial)
		MyThirdPersonActor;
}

/* DownWeapon
For some reason the juggernaut cannon was going into this state.  Since we ship in two hours I don't have time to properly fix this
Therefore I'll just mangle this state.
*/
simulated State DownWeapon
{
ignores Fire,AltFire;

	simulated function bool PutDown()
	{
		return true; //just keep putting it down
	}

	simulated function BeginState()
	{
	}

Begin:
	GotoState('');
}


simulated function AttachToPawn(Pawn P)
{
	Super.AttachToPawn(P);
	MyThirdPersonActor = ThirdPersonActor;
}

simulated function UseAmmo();

simulated function ExecuteFire(optional bool bAltFire)
{
	local bool IsOnline;

	IsOnline=false;
	if (XMPVehicle(XMPVehicleBarrel(Instigator).ConglomerateID)? && XMPVehicle(XMPVehicleBarrel(Instigator).ConglomerateID).Online)
		IsOnline=true;
	if (StationaryTurret(XMPVehicleBarrel(Instigator).ConglomerateID)? && StationaryTurret(XMPVehicleBarrel(Instigator).ConglomerateID).Online)
		IsOnline=true;
	SetAmmoType(0);
	if (IsOnline)
	{
		if( Level.NetMode==NM_Client )
		{
			if( !AmmoType.bRapidFire )
				ServerFire(bAltFire);
			else if( StopFiringTime < Level.TimeSeconds + 0.3 )
			{
				StopFiringTime = Level.TimeSeconds + 0.6;
				ServerRapidFire(bAltFire);
			}
		}
		if( (Role < ROLE_Authority) || PreSetAimingParameters( (AmmoIndex==1), AmmoType.bInstantHit, AmmoType.TraceSpread, AmmoType.ProjectileClass, AmmoType.bWarnTarget, AmmoType.bRecommendSplashDamage ) )
			{
				if (Role == ROLE_Authority)
				{
					AuthorityFire();
					XMPGame(Level.Game).DrainTeamEnergy( XMPVehicleBarrel(Instigator).GetTeam(), EnergyCost, 'TurretFire' );
				}
				EverywhereFire();
			}
	} else if (Level.NetMode != NM_DedicatedServer && Instigator? && (Instigator.IsLocallyControlled()))
		PlaySound(OfflineSound, SLOT_None, 255, , 128 );
}


simulated function SendEvent( string EventName )
{
	if (Instigator? && Instigator.Controller?)
		if( Instigator.IsLocallyControlled() )
			PlayerController(Instigator.Controller).SendEvent(EventName);
}

function Projectile ProjectileFire()
{
	local Projectile SpawnedProjectile;
	if (XMPVehicleBarrel(Owner)? && StationaryTurret(XMPVehicleBarrel(Owner).ConglomerateID)?)
		AdjustedAim = StationaryTurret(XMPVehicleBarrel(Owner).ConglomerateID).GetAimRotation();
	else
		AdjustedAim = Instigator.GetAimRotation();
	SpawnedProjectile = AmmoType.SpawnProjectile(ProjectileFireStartLocation,AdjustedAim);
	
	return SpawnedProjectile;
}

function TraceFire( float TraceSpread )
{
	local vector End, SpreadVector;
	local vector HitLocation, HitNormal;
	local actor Other;
	if (XMPVehicleBarrel(Owner)? && StationaryTurret(XMPVehicleBarrel(Owner).ConglomerateID)?)
		AdjustedAim = StationaryTurret(XMPVehicleBarrel(Owner).ConglomerateID).GetAimRotation();
	else
		AdjustedAim = Instigator.GetAimRotation();
	SpreadVector = vector(AdjustedAim + rotator(RandomSpreadVector( TraceSpread )));
	End = TraceFireStartLocation + TraceDist * SpreadVector;

	Other = Trace(HitLocation,HitNormal,End,TraceFireStartLocation,true);	// Fix ARL: Maybe use TraceRecursive?
	if( !Other )
	{
		HitLocation = End;
		HitNormal = -SpreadVector;
	}
	HandleTraceImpact(Other,HitLocation,HitNormal);
}

simulated function PostNetReceive()
{
	super.postnetreceive();
	if ( ThirdPersonActor == None && MyThirdPersonActor?)
	{
		ThirdPersonActor = WeaponAttachment(MyThirdPersonActor);
	}
}

defaultproperties
{
	OfflineSound=Sound'VehiclesA.Generic.Error'
	bNetNotify=true
	bOnlyDirtyReplication=false
	bOnlyRelevantToOwner=false
	bAlwaysRelevant=true
	EnergyCost=0.0005;
}
