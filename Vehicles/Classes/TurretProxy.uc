//=============================================================================
// TurretProxy.uc
// $Author$
// $Date$
// $Revision$
//=============================================================================

class TurretProxy extends UseProxy
	placeable;

var () float TurretDamageScale;
var () TurretProxyBall MyProxyBall;
var	class<TurretProxyBall>	ProxyBallClass;
var		float					SinceLastChecked;
var	(TurretProxy)	float					RespawnDelay;
var	(TurretProxy) int		TeamNumber;
var	(TurretProxy) int		TurretCost;
var		bool				MyTurretDead;

replication
{
	reliable if ( Role==ROLE_Authority )
		MyProxyBall;
}



event PostBeginPlay()
{
	Super.PostBeginPlay();
	if (bPendingDelete)
		return;
	if( UseMaster?)
	{
		UseMaster.NetDependents.Insert(0,1);
		UseMaster.NetDependents[0] = self;
	}
	else
		Warn(Self @ "was not given a use master - must be fixed in the editor");
	if( UseMaster != None )
	{
		//SetRotation(UseMaster.Rotation);
		XMPStationaryTurret(UseMaster).MyTurretProxy=self;
		XMPStationaryTurret(UseMaster).MyTeam=TeamNumber;
		XMPStationaryTurret(UseMaster).MyTurretCost=TurretCost;
		XMPStationaryTurret(UseMaster).RespawnDelay=RespawnDelay;
		if (Role == Role_Authority)
		{
			MyProxyBall = spawn(ProxyBallClass, self,, Location + ((ProxyBallClass.default.BallOffset * DrawScale) >> Rotation)  );
			MyProxyBall.SetDrawScale(DrawScale);
			//MyProxyBall.SetRotation(XMPStationaryTurret(UseMaster).Barrel.Rotation);
			MyProxyBall.SetBase(self);
		}
	}
}

//-----------------------------------------------------------------------------

event Destroyed()
{
	local int i;
	/*if( UseMaster? && StationaryTurret(UseMaster).Barrel? )
	{
		for( i=StationaryTurret(UseMaster).Barrel.NetDependents.Length-1; i >= 0; i-- )
			if( StationaryTurret(UseMaster).Barrel.NetDependents[i] == Self )
				StationaryTurret(UseMaster).Barrel.NetDependents.Remove(i,1);
		for( i=UseMaster.NetDependents.Length-1; i >= 0; i-- )
			if( UseMaster.NetDependents[i] == StationaryTurret(UseMaster).Barrel )
				UseMaster.NetDependents.Remove(i,1);
	}*/
		for( i=StationaryTurret(UseMaster).NetDependents.Length-1; i >= 0; i-- )
			if( StationaryTurret(UseMaster).NetDependents[i] == Self )
				StationaryTurret(UseMaster).NetDependents.Remove(i,1);
	Super.Destroyed();
}


function OnUse( Actor Other )
{
	if( UseMaster != None && XMPStationaryTurret(UseMaster).Health>0)
	{
		UseMaster.OnUse( Other );
	}
}

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	if( UseMaster != None )
		UseMaster.TakeDamage(Damage * TurretDamageScale, instigatedBy, hitlocation, momentum, damageType);
}

function Tick(float Delta)
{
	super.Tick(Delta);
	if (MyTurretDead)
	{
		SinceLastChecked+=Delta;
		if (SinceLastChecked > RespawnDelay)
		{
			SinceLastChecked=0;
			if (XMPGame(Level.Game).IsTeamFeatureOnline( 'XMPStationaryTurret', TeamNumber ))
			{
				if(XMPStationaryTurret(UseMaster) != None)
				{
					StationaryTurret(UseMaster).Resurrect();
				}
			}
		}
	} 
}

defaultproperties
{
	TurretCost=0.0050
	RespawnDelay=60
	ProxyBallClass=class'TurretProxyBall'
	TurretDamageScale=2.0
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.TurretControl.TurretControl_Arm_TD_01'
	bUsable=false
	bBlockActors=true
	bCollideActors=true
	bBlockPlayers=true
	bProjTarget=true
	bBlockNonZeroExtentTraces=true
	bBlockKarma=true
	MyTurretDead=false
	AlternateSkins(0)=Shader'Arch_TurretsT.TurretControl.TurretControlFX_TD_01_Red'
	AlternateSkins(1)=Shader'Arch_TurretsT.TurretControl.TurretControlFX_TD_01_Blue'
}
