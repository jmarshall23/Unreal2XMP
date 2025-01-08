//=============================================================================
// DeploymentPoint.uc
//=============================================================================

class DeploymentPoint extends HackTrigger
	native
	placeable;

//-----------------------------------------------------------------------------

var() Actor Camera;

var() int SpawnerCount;
var() vector SpawnerLocationOffset;
var DeploymentPoint nextDeploymentPoint;

var array<PlayerStart> Spawners;			// parallel array describing the player starts
var array<float> LastSpawnTimes;
var array<int> FailedSpawnAttempts;

var array<ParticleGenerator> SpawnEffectClasses;
var ParticleGenerator SpawnEffect;
var() float SpawnEffectsDelay;
var int SpawnEffectTeam;

var() float RespawnDelayPct;
var float NextRespawnTime;

var() StaticMesh		BaseStaticMesh;
var() int				BaseRepSkinIndex;
var() array<Material>	BaseTeamSkins;
var TeamMeshActor Base;

var() float SpinLeadInTime;
var DeploySpinner Wings;
var bool bSpinning;
var bool ServerSpawnEffectRequest, ClientSpawnEffectRequest;

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetInitial && (Role==ROLE_Authority) )
		Camera;

	reliable if( bNetDirty && (Role==ROLE_Authority) )
		ServerSpawnEffectRequest;
}

//-----------------------------------------------------------------------------

cpptext
{
	virtual void PostBeginPlay();
	virtual void PostEditChange();
	void SetupPlayerStarts();
	void SetupExtraMeshes();
}

//-----------------------------------------------------------------------------

event PreBeginPlay()
{
	Super.PreBeginPlay();

	LastSpawnTimes.Length = SpawnerCount;
	FailedSpawnAttempts.Length = SpawnerCount;
	NextRespawnTime = Level.TimeSeconds + RespawnDelayPct * XMPGame(Level.Game).RespawnDelaySeconds;
	SetupCamera();
}

//-----------------------------------------------------------------------------

function SetNextRespawnTime( DeploymentPoint DeploymentPointList )
{
	local DeploymentPoint DP, LatestDeployPoint;
	local float MaxPctDifference, CurrentPct, CurrDiff;

	// Find the latest future deployment time of all deploy points on our team.
	// Then use that to offset our next deploy time based on the relative RespawnDelayPct difference.
	for( DP=DeploymentPointList; DP!=None; DP=DP.nextDeploymentPoint )
	{
		if( DP != Self &&
			DP.GetTeam() == GetTeam() && 
			!(DP.RespawnDelayPct ~= RespawnDelayPct) )
		{
			CurrentPct = DP.RespawnDelayPct;
			if( CurrentPct < RespawnDelayPct )
				CurrentPct += 1.0;

			CurrDiff = CurrentPct - RespawnDelayPct;
			if( CurrDiff > MaxPctDifference || LatestDeployPoint == None )
			{
				LatestDeployPoint = DP;
				MaxPctDifference = CurrDiff;
			}
		}
	}

	// in case we fall through
	if( LatestDeployPoint == None )
	{
		LatestDeployPoint = Self;
		MaxPctDifference = 0.0;
	}

	assert( MaxPctDifference < 1.0 );
	NextRespawnTime = LatestDeployPoint.NextRespawnTime + (1.0 - MaxPctDifference) * XMPGame(Level.Game).RespawnDelaySeconds;
}

//-----------------------------------------------------------------------------

simulated event PostNetReceive()
{
	Super.PostNetReceive();
	//DMTN("PostNetReceive");
	//DMTN("ServerSpawnEffectRequest:" @ ServerSpawnEffectRequest @ "ClientSpawnEffectRequest:" @ ClientSpawnEffectRequest );
	if( ServerSpawnEffectRequest && ServerSpawnEffectRequest != ClientSpawnEffectRequest )
	{
		ClientSpawnEffects();
	}
	ClientSpawnEffectRequest = ServerSpawnEffectRequest;
}

//-----------------------------------------------------------------------------

function SetupCamera()
{
	if( Camera != None )
	{
		Camera.NetDependents.Insert(0,1);
		Camera.NetDependents[0] = Self;
	}
	else
	{
		Camera = Self;
	}
}

//-----------------------------------------------------------------------------

simulated function ClientSpawnEffects()
{
	local int TeamIndex;

	//DMTN("ClientSpawnEffects -- Team:" @ GetTeam() );

	TeamIndex = GetTeam();
	if( TeamIndex < SpawnEffectClasses.Length )
	{
		if( SpawnEffectTeam != TeamIndex )
		{
			if( SpawnEffect? )
			{
				SpawnEffect.Destroy();
				SpawnEffect = None;
			}
		}

		if( !SpawnEffect )
		{
			//DMTN("Effect created");
			SpawnEffect = class'ParticleGenerator'.static.CreateNew( self, SpawnEffectClasses[TeamIndex], Location );
			SpawnEffect.IdleTime=0;
		}
		SpawnEffectTeam = TeamIndex;

		// delay the effect
		AddTimer( 'ClientSpawnEffectsTimer', SpawnEffectsDelay );
	}
}

//-----------------------------------------------------------------------------

simulated function ClientSpawnEffectsTimer()
{
	//DMTN("Timer to trigger effect");
	SpawnEffect.Trigger( Self, Instigator );
}

//-----------------------------------------------------------------------------

function SetTeam( int NewTeam )
{
	Super.SetTeam( NewTeam );
	if( Base? )   Base.SetTeam( NewTeam );
	if( Wings? ) Wings.SetTeam( NewTeam );
}

//-----------------------------------------------------------------------------

function HandleHacked( int NewHackedTeam )
{
	local int OldTeam;
	OldTeam = GetTeam();
	Super.HandleHacked( NewHackedTeam );
	XMPGame( Level.Game ).NotifyDeployPointHacked( Self, OldTeam, None );
}

//-----------------------------------------------------------------------------

function RegisterHackScore( Controller PrimaryHacker, Controller SecondaryHacker )
{
	XMPGame(Level.Game).NotifyHackedDeployPoint( PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

function StartSpinning()
{
	//DMTN("StartSpinning");
	if( !bSpinning)
	{
		if( Wings? )
			Wings.Trigger( Self, Instigator );
		bSpinning = true;

		if( Level.NetMode != NM_DedicatedServer )
			ClientSpawnEffects();
		else
			ServerSpawnEffectRequest = true;
	}
}

//-----------------------------------------------------------------------------

function StopSpinning()
{
	//DMTN("StopSpinning");
	bSpinning = false;
	ServerSpawnEffectRequest = false;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPWorldItemsM.DeployPoint.DeployPointMain'
	SpawnEffectClasses(0)=ParticleSalamander'Deploy_FX.ParticleSalamander2'
	SpawnEffectClasses(1)=ParticleSalamander'Deploy_FX.ParticleSalamander1'
	AlternateSkins(0)=Shader'XMPWorldItemsT.Base.Poly_Pulse_Red'
	AlternateSkins(1)=Shader'XMPWorldItemsT.Base.Poly_Pulse_Blue'
	RepSkinIndex=1
	bStaticLighting=True
	bShadowCast=True
	bCollideActors=True
	bBlockActors=True
	bBlockPlayers=True
	bBlockKarma=True
	bBlockNonZeroExtentTraces=True
	bUseCylinderCollision=True
	CollisionRadius=30
	CollisionHeight=110
	bNetNotify=true //enable PostNetReceive events

	Description="Deployment Point"
	bKeepProgress=true;
	SpawnerCount=4
	SpawnerLocationOffset=(X=100,Y=100)

	BaseStaticMesh=StaticMesh'XMPWorldItemsM.DeployPoint.DeployPointBase'
	BaseTeamSkins(0)=Shader'XMPWorldItemsT.Base.Poly_DeployPointDSI_RW_001_Red'
	BaseTeamSkins(1)=Shader'XMPWorldItemsT.Base.Poly_DeployPointDSI_RW_001_Blue'
	BaseRepSkinIndex=2

	SpinLeadInTime=2
	SpawnEffectsDelay=1
	SpawnEffectTeam=255

	HackGainedMessage=class'DeployPointGainedMessage'
	HackLostMessage=class'DeployPointLostMessage'
}
