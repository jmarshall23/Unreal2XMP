class XMPStationaryTurret extends StationaryTurret
	placeable;

var() class<DamageFilter> DamageFilterClass;
var TurretProxy MyTurretProxy;
var ParticleGenerator	TurretExplosionType;
var ParticleGenerator Explosion;
var	(XMPStationaryTurret) int		MyTeam;
var	(XMPStationaryTurret) int		MyTurretCost;
var		float					SinceLastChecked;
var	(XMPStationaryTurret)	float	RespawnDelay;
var bool	ImDead;
var bool	bExploding;
var	float	ExplosionTime;

replication
{
	reliable if (Role==ROLE_Authority)
		ImDead, ExplosionTime;
}


simulated event Destroyed()
{
	if (Explosion?)
	{
		Explosion.ParticleDestroy();
		Explosion=None;
	}
	super.destroyed();

}

simulated function ClientKDriverEnter(PlayerController pc, Pawn P)
{
	super.ClientKDriverEnter(pc,p);
	if (Barrel.DriverViewMesh?)
		{
			Self.bHidden=true;
			Barrel.BaseMesh=Barrel.StaticMesh;
			Barrel.SetStaticMesh(Barrel.DriverViewMesh);
		}
}

simulated function ClientKDriverLeave(PlayerController pc, Pawn P)
{
	Super.ClientKDriverLeave(pc, p);
	if (Barrel.DriverViewMesh?)
		{
			Self.bHidden=false;
			Barrel.SetStaticMesh(Barrel.BaseMesh);
		}
}

function KDriverEnter(Pawn p)
{
Super.KDriverEnter(p);
if (bProxyControlled)
	MyTurretProxy.MyProxyBall.bUsable=false;
Online = XMPGame(Level.Game).IsTeamFeatureOnline( 'XMPStationaryTurret', GetTeam() );
}

function bool KDriverLeave(bool bForceLeave)
{
	if (Super.KDriverLeave(bForceLeave))
	{
	if (bProxyControlled)
		MyTurretProxy.MyProxyBall.bUsable=true;
	return true;
	} else return false;
}

simulated function FeignDeath()
{
	self.SetCollision(false, false, false);
	self.bHidden= True;
	self.SetOwner(MyTurretProxy);
	ExplosionTime = level.TimeSeconds;
	
	Barrel.SetCollision(false, false, false);
	Barrel.bHidden= True;
	Barrel.SetOwner(self);
	
	if (Role == ROLE_Authority )
	{
		if (MyTurretProxy?)
			MyTurretProxy.MyTurretDead = true;
		ImDead=true;
		if(level.netmode != NM_DedicatedServer && TurretExplosionType? && !Explosion?)
		{
			Explosion = class'ParticleGenerator'.static.CreateNew( self, TurretExplosionType, Location );
			Explosion.SetRotation( Rotation );
		}
		if (Explosion? && !bExploding)
			Explosion.Trigger( Self, Self );
		bExploding=true;
	}
	if (MyTurretProxy?)
		MyTurretProxy.MyProxyBall.bUsable = false;

}

simulated event PostNetReceive()
{
	super.PostNetReceive();
	if (ImDead && !bExploding && level.timeseconds < (ExplosionTime+1))
	{
		if (level.netmode != NM_DedicatedServer)
		{
			// Trigger any effects for destruction
			if(TurretExplosionType? && !Explosion?)
			{
				Explosion = class'ParticleGenerator'.static.CreateNew( self, TurretExplosionType, Location );
				Explosion.SetRotation( Rotation );
			}
			Explosion.Trigger( Self, Self );
			bExploding=true;
		}
	}
}

simulated function Resurrect()
{
	self.SetCollision(true,false,true);
	self.Velocity = vect(0,0,0);
	self.SetPhysics(PHYS_None);
	self.bHidden= false;
	
	Barrel.SetCollision(true, false, true);
	Barrel.bHidden= false;
	
	Health=class.default.Health;

	if (Role == ROLE_Authority)
	{
		if (MyTurretProxy?)
		{
			MyTurretProxy.MyTurretDead = false;
			MyTurretProxy.MyProxyBall.bUsable = true;
		}
		ImDead=false;
		bExploding=false;
		//XMPGame(Level.Game).DrainTeamEnergy( MyTeam, MyTurretCost, 'TurretResurrect' );
	}
}

function Tick(float Delta)
{
	super.Tick(Delta);
	if (ImDead && !bProxyControlled)
	{
		SinceLastChecked+=Delta;
		if (SinceLastChecked > RespawnDelay)
		{
			SinceLastChecked=0;
//			if (XMPGame(Level.Game).IsTeamFeatureOnline( 'XMPStationaryTurret', MyTeam ))
//			{
				Resurrect();
//			}
		}
	} 
}

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	if (Imdead)
		Return;
	
	if( DamageFilterClass? )
		DamageFilterClass.static.ApplyFilter( Damage, momentum, DamageType );

	super.TakeDamage(Damage, instigatedBy, hitlocation, momentum, damageType);
}

function SetTeam( int NewTeam )
{
	Super.SetTeam( NewTeam );
	if (MyTurretProxy != None)
	{
		class'UtilGame'.static.SetTeamSkin( MyTurretProxy, NewTeam );
		if (MyTurretProxy.MyProxyBall != None)
			class'UtilGame'.static.SetTeamSkin( MyTurretProxy.MyProxyBall, NewTeam );
	}
}

defaultproperties
{
	TurretExplosionType=ParticleSalamander'Vehicle_Explosion_FX.ParticleSalamander0'
	ImDead=false
	MyTeam=255
	MyTurretCost=50
	RespawnDelay=60
	DamageFilterClass=class'DamageFilterVehicles'
	RotateStartSound=Sound'U2AmbientA.Doors_Opening.OpeningPowerUp_05'
	RotateSound=Sound'U2AmbientA.Doors_Ambient.DoorSlide_12'
}