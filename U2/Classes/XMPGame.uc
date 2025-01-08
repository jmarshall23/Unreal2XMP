//=============================================================================
// XMPGame.uc
//=============================================================================

class XMPGame extends GameInfo;

//-----------------------------------------------------------------------------

const MaxEnergySources = 16;					// max # energy sources in a level

//-----------------------------------------------------------------------------

enum EFeatureState
{
	FS_Offline,
	FS_AlmostOffline,
	FS_Online
};

//-----------------------------------------------------------------------------

enum EArtifactState
{
	AS_Stored,
	AS_Dropped,
	AS_Carried
};

//-----------------------------------------------------------------------------

struct ArtifactInfo
{
	var int ID;									// Artifact ID
	var class<Artifact> Type;					// Artifact type
	var int Team;								// which team this artifact is registered to
	var Pawn Carrier;							// pawn carrying the artifact (using / stealing)
	var EArtifactState State;					// where the artifact is
	var float PickedUpTime;
};

//-----------------------------------------------------------------------------

struct SpawnInfoT
{
	var PlayerStart PS;
	var Controller C;
	var bool bWaiting;
};

//-----------------------------------------------------------------------------

struct ConsumerInfoT
{
	var Actor Consumer;
	var float EnergyCostPerSec;
};

//-----------------------------------------------------------------------------

struct FeatureThresholdT
{
	var name Feature;							// game play feature (stations, packs, vehicles, turrets)
	var float Threshold;						// amount of team energy needed to support feature
};

//-----------------------------------------------------------------------------

struct TeamEnergyInfoT
{
	var float LastTeamEnergy;					// team energy level when last checked
	var bool bWarned;							// has team received a warning that a feature is going offline
};

//-----------------------------------------------------------------------------

var array<ArtifactInfo> ArtInfo;				// bookkeeping of where the artifacts are
var int UniqueArtifactID;						// artifact ID generator;
var int MaxArtifactCarryTime;					// max time, in seconds, a player can hold on to an artifact

var NavigationPoint LastPlayerStartSpot;		// last place player looking for start spot started from
var NavigationPoint LastStartSpot;				// last place any player started from
var DeploymentPoint DeploymentPointList;
var int RespawnDelaySeconds;					// time to wait between respawning dead players
var int RespawnTimer;
var sound LockedInSound;						// sound when player has locked into a deploy point

var array<SpawnInfoT> SpawnList;				// list of all the controllers and the playerstarts they will use

var float TeamEnergyInitial;					// energy give to each team at the start of match
var float TeamEnergyMax;						// maximum energy a team can accumulate

var EnergySource EnergySources[MaxEnergySources];
var int NumEnergySources;
var array<FeatureThresholdT> EnergyThresholds;
var array<TeamEnergyInfoT>TeamEnergyInfo;
var float TeamFeatureWarningThreshold;

var int RemainingTime, ElapsedTime;
var globalconfig int RestartWait;
var float EndTime;
var int GameEndUIEvent;

var array<ConsumerInfoT> ConsumerList;			// list of actors which use energy

var globalconfig float JumpJetRechargeTime;			// ignored if < 0.0 !!ARL (mdf) tbr
var globalconfig bool bJumpJetRechargesOnLanding;	// ignored if < 0.0 !!ARL (mdf) tbr
var globalconfig bool bCheatXMP;					//!!ARL (mdf) tbr

var ScoreKeeper ScoreKeeper;

var int NextLevelLoadingUIEvent;

//-----------------------------------------------------------------------------

event PreBeginPlay()
{
	local int i;

	Super.PreBeginPlay(); // init GameReplicationInfo in GameInfo.uc PreBeginPlay()

	for( i = 0; i < ArrayCount(GameReplicationInfo.Teams); i++ )
	{
		GameReplicationInfo.Teams[i] = Spawn( class'TeamInfo' );
		GameReplicationInfo.Teams[i].TeamIndex = i;
		GameReplicationInfo.Teams[i].Size = 0;
		GameReplicationInfo.Teams[i].Score = 0;
		GameReplicationInfo.Teams[i].EnergyLevel = TeamEnergyInitial;

		TeamEnergyInfo.Insert(0,1);
		TeamEnergyInfo[0].LastTeamEnergy = TeamEnergyInitial;
	}
	ScoreKeeper = Spawn(class'ScoreKeeper');
}

//-----------------------------------------------------------------------------

event Destroyed()
{
	if( ScoreKeeper? )
	{
		ScoreKeeper.Destroy();
		ScoreKeeper = None;
	}
	RemoveAllTimers();
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

event SetInitialState()
{
	local int T;
	local Actor A;
	local DeploymentPoint DP;
	local EnergySource ES;
	local int ESIndex;
	local GameReplicationInfo GRI;

	Super.SetInitialState();

	foreach AllActors( class'Actor', A )
	{
		DP = DeploymentPoint(A);
		if( DP? )
		{
			DP.nextDeploymentPoint = DeploymentPointList;
			DeploymentPointList = DP;
		}

		ES = EnergySource(A);
		if( ES? )
		{
			EnergySources[ESIndex] = ES;
			ESIndex++;
		}
	}

	NumEnergySources = ESIndex;

	GRI = GameReplicationInfo;

	if( EnergySources[0]?  ) { GRI.bEnergySource0  = true; T=EnergySources[0].GetTeam();  GRI.bEnergySourceRed0  = T==0; GRI.bEnergySourceBlue0  = T==1; }
	if( EnergySources[1]?  ) { GRI.bEnergySource1  = true; T=EnergySources[1].GetTeam();  GRI.bEnergySourceRed1  = T==0; GRI.bEnergySourceBlue1  = T==1; }
	if( EnergySources[2]?  ) { GRI.bEnergySource2  = true; T=EnergySources[2].GetTeam();  GRI.bEnergySourceRed2  = T==0; GRI.bEnergySourceBlue2  = T==1; }
	if( EnergySources[3]?  ) { GRI.bEnergySource3  = true; T=EnergySources[3].GetTeam();  GRI.bEnergySourceRed3  = T==0; GRI.bEnergySourceBlue3  = T==1; }
	if( EnergySources[4]?  ) { GRI.bEnergySource4  = true; T=EnergySources[4].GetTeam();  GRI.bEnergySourceRed4  = T==0; GRI.bEnergySourceBlue4  = T==1; }
	if( EnergySources[5]?  ) { GRI.bEnergySource5  = true; T=EnergySources[5].GetTeam();  GRI.bEnergySourceRed5  = T==0; GRI.bEnergySourceBlue5  = T==1; }
	if( EnergySources[6]?  ) { GRI.bEnergySource6  = true; T=EnergySources[6].GetTeam();  GRI.bEnergySourceRed6  = T==0; GRI.bEnergySourceBlue6  = T==1; }
	if( EnergySources[7]?  ) { GRI.bEnergySource7  = true; T=EnergySources[7].GetTeam();  GRI.bEnergySourceRed7  = T==0; GRI.bEnergySourceBlue7  = T==1; }
	if( EnergySources[8]?  ) { GRI.bEnergySource8  = true; T=EnergySources[8].GetTeam();  GRI.bEnergySourceRed8  = T==0; GRI.bEnergySourceBlue8  = T==1; }
	if( EnergySources[9]?  ) { GRI.bEnergySource9  = true; T=EnergySources[9].GetTeam();  GRI.bEnergySourceRed9  = T==0; GRI.bEnergySourceBlue9  = T==1; }
	if( EnergySources[10]? ) { GRI.bEnergySource10 = true; T=EnergySources[10].GetTeam(); GRI.bEnergySourceRed10 = T==0; GRI.bEnergySourceBlue10 = T==1; }
	if( EnergySources[11]? ) { GRI.bEnergySource11 = true; T=EnergySources[11].GetTeam(); GRI.bEnergySourceRed11 = T==0; GRI.bEnergySourceBlue11 = T==1; }
	if( EnergySources[12]? ) { GRI.bEnergySource12 = true; T=EnergySources[12].GetTeam(); GRI.bEnergySourceRed12 = T==0; GRI.bEnergySourceBlue12 = T==1; }
	if( EnergySources[13]? ) { GRI.bEnergySource13 = true; T=EnergySources[13].GetTeam(); GRI.bEnergySourceRed13 = T==0; GRI.bEnergySourceBlue13 = T==1; }
	if( EnergySources[14]? ) { GRI.bEnergySource14 = true; T=EnergySources[14].GetTeam(); GRI.bEnergySourceRed14 = T==0; GRI.bEnergySourceBlue14 = T==1; }
	if( EnergySources[15]? ) { GRI.bEnergySource15 = true; T=EnergySources[15].GetTeam(); GRI.bEnergySourceRed15 = T==0; GRI.bEnergySourceBlue15 = T==1; }
}

//-----------------------------------------------------------------------------

function TeamInfo OtherTeam(TeamInfo Requester)
{
	local int i;
	for( i=0; i < ArrayCount(GameReplicationInfo.Teams); i++ )
		if( GameReplicationInfo.Teams[i] != Requester )
			return GameReplicationInfo.Teams[i];

	return Super.OtherTeam(Requester);
}

//-----------------------------------------------------------------------------
// Team Energy

function bool TeamFeatureExists( name Feature )
{
	local int i;

	if( bCheatXMP )
		return true;

	for( i=0; i < EnergyThresholds.Length; i++ )
		if( EnergyThresholds[i].Feature == Feature )
			return true;
			
	return false;
}

//-----------------------------------------------------------------------------

function bool IsTeamFeatureOnline( name Feature, int TeamNumber )
{   
	local int i;

	if( bCheatXMP )
		return true;

	if( TeamNumber == 255 )
	{
		Warn("TeamFeatureOnline called with a neutral team -- feature:" @ Feature @ " -- returning false");
		return false;
	}

	for( i=0; i < EnergyThresholds.Length; i++ )
		if( EnergyThresholds[i].Feature == Feature )
			return( GameReplicationInfo.Teams[TeamNumber].EnergyLevel >= EnergyThresholds[i].Threshold );

	Warn( "TeamFeatureOnline: Unable to find team feature:" @ Feature @ " -- returning false");
	return false;
}

//-----------------------------------------------------------------------------

function float DrainTeamEnergy( int TeamNumber, float EnergyAmount, name Reason )
{
	local int TeamSize;

	if( TeamNumber != 255 )
	{
		TeamSize = GameReplicationInfo.Teams[TeamNumber].Size;
		if( TeamSize > 0 )
		{
			// scale based on team size
			EnergyAmount /= TeamSize;

			EnergyAmount = FMin( EnergyAmount, GameReplicationInfo.Teams[TeamNumber].EnergyLevel );

			//DMT(Reason @ "used" @ EnergyAmount @ "of team energy");

			GameReplicationInfo.Teams[TeamNumber].EnergyLevel -= EnergyAmount;

			CheckForTeamEnergyWin( TeamNumber );

			// scale back for caller
			EnergyAmount *= TeamSize;
		}
	}

	return EnergyAmount;
}

//-----------------------------------------------------------------------------
// Energy Consumers

function CheckConsumerList( float DeltaSeconds )
{
	local int ii;
	
	for( ii=0; ii<ConsumerList.Length; ii++ )
	{
		if( !ConsumerList[ ii ].Consumer.bDeleteMe )
			DrainTeamEnergy( ConsumerList[ ii ].Consumer.GetTeam(), ConsumerList[ ii ].EnergyCostPerSec * DeltaSeconds, 'Consumable' );
	}
}

//-----------------------------------------------------------------------------

function RegisterConsumer( Actor Consumer, float EnergyCostPerSec )
{
	local int ii;

	// make sure not already in list?
	for( ii=0; ii<ConsumerList.Length; ii++ )
		if(  ConsumerList[ ii ].Consumer == Consumer )
			return;

	ConsumerList.Insert(0,1);
	ConsumerList[ 0 ].Consumer = Consumer;
	ConsumerList[ 0 ].EnergyCostPerSec = EnergyCostPerSec;
}

//-----------------------------------------------------------------------------

function UnregisterConsumer( Actor Consumer )
{
	local int ii;
	
	for( ii=0; ii<ConsumerList.Length; ii++ )
	{
		if(  ConsumerList[ ii ].Consumer == Consumer )
		{
			ConsumerList.Remove( ii, 1 );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Energy Sources

function NotifyHackedEnergySource( EnergySource HackedES, Controller PrimaryHacker, optional Controller SecondaryHacker )
{
	local int i, T;
	local GameReplicationInfo GRI;
	GRI = GameReplicationInfo;
	for( i=0; i < ArrayCount(EnergySources); i++ )
		if( EnergySources[i] == HackedES )
			break;

	switch(i)
	{
	case 0:		T=HackedES.GetTeam(); GRI.bEnergySourceRed0  = T==0; GRI.bEnergySourceBlue0  = T==1; break;
	case 1:		T=HackedES.GetTeam(); GRI.bEnergySourceRed1  = T==0; GRI.bEnergySourceBlue1  = T==1; break;
	case 2:		T=HackedES.GetTeam(); GRI.bEnergySourceRed2  = T==0; GRI.bEnergySourceBlue2  = T==1; break;
	case 3:		T=HackedES.GetTeam(); GRI.bEnergySourceRed3  = T==0; GRI.bEnergySourceBlue3  = T==1; break;
	case 4:		T=HackedES.GetTeam(); GRI.bEnergySourceRed4  = T==0; GRI.bEnergySourceBlue4  = T==1; break;
	case 5:		T=HackedES.GetTeam(); GRI.bEnergySourceRed5  = T==0; GRI.bEnergySourceBlue5  = T==1; break;
	case 6:		T=HackedES.GetTeam(); GRI.bEnergySourceRed6  = T==0; GRI.bEnergySourceBlue6  = T==1; break;
	case 7:		T=HackedES.GetTeam(); GRI.bEnergySourceRed7  = T==0; GRI.bEnergySourceBlue7  = T==1; break;
	case 8:		T=HackedES.GetTeam(); GRI.bEnergySourceRed8  = T==0; GRI.bEnergySourceBlue8  = T==1; break;
	case 9:		T=HackedES.GetTeam(); GRI.bEnergySourceRed9  = T==0; GRI.bEnergySourceBlue9  = T==1; break;
	case 10:	T=HackedES.GetTeam(); GRI.bEnergySourceRed10 = T==0; GRI.bEnergySourceBlue10 = T==1; break;
	case 11:	T=HackedES.GetTeam(); GRI.bEnergySourceRed11 = T==0; GRI.bEnergySourceBlue11 = T==1; break;
	case 12:	T=HackedES.GetTeam(); GRI.bEnergySourceRed12 = T==0; GRI.bEnergySourceBlue12 = T==1; break;
	case 13:	T=HackedES.GetTeam(); GRI.bEnergySourceRed13 = T==0; GRI.bEnergySourceBlue13 = T==1; break;
	case 14:	T=HackedES.GetTeam(); GRI.bEnergySourceRed14 = T==0; GRI.bEnergySourceBlue14 = T==1; break;
	case 15:	T=HackedES.GetTeam(); GRI.bEnergySourceRed15 = T==0; GRI.bEnergySourceBlue15 = T==1; break;
	default:	Warn("Unknown deploy point or Too many deploy points for HUD, limit = " @ MaxEnergySources ); break;
	}

	ScoreKeeper.NotifyHackedEnergySource( PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------
// register energy collected from the energy sources

function StoreEnergyFromSources( float DeltaSeconds )
{
	local int i, ESTeam;
	local EnergySource ES;

	for( i=0; i < ArrayCount(EnergySources); i++ )
	{
		ES = EnergySources[i];
		if( ES? )
		{
			ESTeam = ES.GetTeam();
			if( ESTeam < ArrayCount(GameReplicationInfo.Teams) &&
				GameReplicationInfo.Teams[ESTeam].EnergyLevel < TeamEnergyMax )
			{
				GameReplicationInfo.Teams[ESTeam].EnergyLevel += DeltaSeconds * ES.EnergyRate * GameReplicationInfo.Teams[ESTeam].Size / NumEnergySources;
			}
		}
	}
}

//-----------------------------------------------------------------------------

function CheckForTeamEnergyWin( int TeamNumber )
{
	local Controller C;
	local U2Pawn U2P;

	// team has energy -- keep playing
	if( GameReplicationInfo.Teams[TeamNumber].EnergyLevel > 0 )
		return;

	for ( C=Level.ControllerList; C!=None; C=C.nextController )
	{
		if( C.GetTeam() == TeamNumber )
		{
			// someone still has some skill energy left, games not over
			U2P = U2Pawn(C.Pawn);
			if( U2P? && U2P.ValidPawn(U2P) && U2P.Skill > 0.0001 )
				return;
		}
	}
	
	// no more team energy and nobody has any skill to hack, so end the game
	HandleTeamEnergyWin( TeamNumber );
}

//-----------------------------------------------------------------------------

function HandleTeamEnergyWin( int LosingTeam )
{
	local IconNode IN;

	// view smoking base of loser
	foreach AllActors( class'IconNode', IN )
		if( IN.GetTeam() == LosingTeam )
		{
			HandleTeamWon( None, IN, "Generator Ownage");

			if( LosingTeam == 0 ) GameEndUIEvent = 9;	//Red has no energy
			else				  GameEndUIEvent = 10;	//Blue has no energy
			break;
		}
}

//-----------------------------------------------------------------------------

function UpdateTeamEnergy( float DeltaSeconds )
{
	StoreEnergyFromSources( DeltaSeconds );
	CheckConsumerList( DeltaSeconds );
	CheckTeamEnergy();
}

//-----------------------------------------------------------------------------

function CheckTeamEnergy()
{
	local int iTeam, iFeature;
	local name CurrentFeature;
	local float CurrentTeamEnergy, PrevTeamEnergy, FeatureEnergyNeeded;

	for( iTeam=0; iTeam < ArrayCount(GameReplicationInfo.Teams); iTeam++ )
	{
		CurrentTeamEnergy = GameReplicationInfo.Teams[iTeam].EnergyLevel;
		PrevTeamEnergy = TeamEnergyInfo[iTeam].LastTeamEnergy;

		// if the list were sorted, this loop could go away.
		for( iFeature=0; iFeature < EnergyThresholds.Length; iFeature++ )
		{
			CurrentFeature = EnergyThresholds[iFeature].Feature;
			FeatureEnergyNeeded = EnergyThresholds[iFeature].Threshold;

			// check if a warning should be sent
			if( PrevTeamEnergy >= FeatureEnergyNeeded + TeamFeatureWarningThreshold )
			{
				if( CurrentTeamEnergy < FeatureEnergyNeeded + TeamFeatureWarningThreshold )
				{
					// prevent duplicate warnings
					if( !TeamEnergyInfo[iTeam].bWarned )
					{
						// send out a warning
						ChangeFeatureStatus( iFeature, FS_AlmostOffline, iTeam );
						TeamEnergyInfo[iTeam].bWarned = true;
					}
				}
			}
			// check if a feature need to be shutdown
			else if( PrevTeamEnergy >= FeatureEnergyNeeded )
			{
				if( CurrentTeamEnergy < FeatureEnergyNeeded )
				{
					// go offline
					ChangeFeatureStatus( iFeature, FS_Offline, iTeam );
					TeamEnergyInfo[iTeam].bWarned = false;
				}
			}
			else
			{
				if( CurrentTeamEnergy >= FeatureEnergyNeeded )
				{
					// go back online
					ChangeFeatureStatus( iFeature, FS_Online, iTeam );
				}
			}
		}
		TeamEnergyInfo[iTeam].LastTeamEnergy = CurrentTeamEnergy;
	}
}

//-----------------------------------------------------------------------------

function ChangeFeatureStatus( int FeatureIndex, EFeatureState Status, int TeamNumber )
{
	local Actor A;
	local name FeatureName;

	FeatureName = EnergyThresholds[FeatureIndex].Feature;

	switch( Status )
	{
	case FS_Offline:		DMTN("Taking" @ FeatureName @ "offline");		break;
	case FS_Online:			DMTN("Bringing" @ FeatureName @ "online");		break;
	case FS_AlmostOffline:	DMTN("Sending a warning for" @ FeatureName );	break;
	default:				DMTN("Unknown team status for" @ FeatureName ); break;
	}

	if( !bCheatXMP && Status != FS_AlmostOffline )
	{
		foreach AllActors( class'Actor', A )
			if( A.IsA( FeatureName ) && A.GetTeam() == TeamNumber )
				A.NotifyTeamEnergyStatus( Status==FS_Online );
	}

	SendFeatureMessage( FeatureIndex, Status, TeamNumber );
}

//-----------------------------------------------------------------------------

function SendFeatureMessage( int FeatureIndex, EFeatureState Status, int TeamNumber )
{
	local int MsgIndex;

	// messages sorted first by status, then by team, and lastly by feature
	MsgIndex = Status * EnergyThresholds.Length * ArrayCount(GameReplicationInfo.Teams) +
			   TeamNumber * EnergyThresholds.Length +
			   FeatureIndex;

	BroadcastLocalized( self, class'XMPGameMessage', MsgIndex, None, None, None );
}

//-----------------------------------------------------------------------------
// Redeployment

function int FindSpawnInfoByController( Controller keyC )
{
	local int i;
	for( i=0; i < SpawnList.Length; i++ )
		if( SpawnList[i].C == keyC )
			return i;
	return -1;
}

//-----------------------------------------------------------------------------

function int FindSpawnInfoByPlayerStart( PlayerStart keyPS )
{
	local int i;
	for( i=0; i < SpawnList.Length; i++ )
		if( SpawnList[i].PS == keyPS )
			return i;
	return -1;
}

//-----------------------------------------------------------------------------

function PlayerStart GetAvailPlayerStart( U2PlayerController U2PC )
{
	local int i, LRU;
	local float OldestTime;
	local DeploymentPoint DP;			// the deployment point whose player start we are searching

	DP = DeploymentPoint( U2PC.PlayerReplicationInfo.RespawnPoint );

	// use the least recently used from the list
	LRU = -1;
	OldestTime = Level.TimeSeconds;
	for( i=0; i < DP.Spawners.Length; i++ )
	{
		if( DP.Spawners[i].bEnabled &&
			DP.FailedSpawnAttempts[i] == 0 &&
			DP.LastSpawnTimes[i] < OldestTime )
			LRU = i;
	}

	if( LRU >= 0 )
		return DP.Spawners[LRU];
	else
		return None;
}

//-----------------------------------------------------------------------------

function LockInCurrentDeployPoint( U2PlayerController InC )
{
	local int Index;

	// Lock in the deploy point that the player is currently at
	InC.PlayerReplicationInfo.RespawnPoint = InC.RespawnPoint;
	InC.PlayerReplicationInfo.bLockedIn = true;

	// find existing entry for this player
	Index = FindSpawnInfoByController( InC );
	if( Index < 0 )
	{
		SpawnList.Length = SpawnList.Length + 1;
		Index = SpawnList.Length - 1;
		SpawnList[Index].C = InC;			// store player reference
	}

	// player locked in at the last second, spin up the deploy point
	if( InC.RespawnPoint.NextRespawnTime - InC.RespawnPoint.SpinLeadInTime < Level.TimeSeconds )
		InC.RespawnPoint.StartSpinning();

	// switch to behind view on the deploy point
	InC.SetDeployPointView();

	InC.PlayOwnedSound( LockedInSound );

	SpawnList[Index].bWaiting = true;
}

//-----------------------------------------------------------------------------

function UnLockCurrentDeployPoint( U2PlayerController InC )
{
	local int Index;

	InC.RespawnPoint = None;
	InC.PlayerReplicationInfo.RespawnPoint = None;
	InC.PlayerReplicationInfo.bLockedIn = false;

	// find existing entry for this player
	Index = FindSpawnInfoByController( InC );
	if( Index < 0 )
	{
		SpawnList.Length = SpawnList.Length + 1;
		Index = SpawnList.Length - 1;
		SpawnList[Index].C = InC;			// store player reference
	}

	SpawnList[Index].bWaiting = false;
}

//-----------------------------------------------------------------------------

function bool IsReadyToRedeploy( int Index )
{
	if( DeploymentPointList == None )		// no deploy point, so don't wait for player to lock in
		return true;

	return( SpawnList[Index].C!=None && SpawnList[Index].bWaiting );
}

//-----------------------------------------------------------------------------

function Deploy()
{
	local Controller C;
	local DeploymentPoint DP;
	local U2PlayerController U2PC;

	for( DP=DeploymentPointList; DP!=None; DP=DP.nextDeploymentPoint )
	{
		if( DP.NextRespawnTime < Level.TimeSeconds )
		{
			DeployPlayers( DP );
			DP.SetNextRespawnTime( DeploymentPointList );
			for( C=Level.ControllerList; C!=None; C=C.nextController )
			{
				U2PC = U2PlayerController(C);
				if( U2PC? && U2PC.RespawnPoint == DP )
					U2PC.ClientSetNextDeployTime( DP.NextRespawnTime - Level.TimeSeconds );
			}
			DP.StopSpinning();
		}
		else if( DP.NextRespawnTime - DP.SpinLeadInTime < Level.TimeSeconds )
		{
			if( NumPlayerLockedInAt(DP) > 0 )
				DP.StartSpinning();
		}
	}
}

//-----------------------------------------------------------------------------

function ForceDeploy( U2PlayerController aPlayer )
{
	LockInCurrentDeployPoint( aPlayer );
	DeployPlayers( DeploymentPoint( aPlayer.PlayerReplicationInfo.RespawnPoint ), aPlayer );
}

//-----------------------------------------------------------------------------

function int NumPlayerLockedInAt( DeploymentPoint ReadyToDeployPoint )
{
	local int i, numPlayers;
	local U2PlayerController U2PC;

	for( i=0; i < SpawnList.Length; i++ )
	{
		U2PC = U2PlayerController( SpawnList[i].C );
		if( U2PC? && !U2PC.bDeleteMe &&
			U2PC.PlayerReplicationInfo.bLockedIn &&
			U2PC.PlayerReplicationInfo.RespawnPoint == ReadyToDeployPoint )
		{
			numPlayers++;
		}
	}

	return numPlayers;
}

//-----------------------------------------------------------------------------

function DeployPlayers( DeploymentPoint ReadyToDeployPoint, optional Controller aPlayer )
{
	local int i, numTries;
	local U2PlayerController U2PC;
	local rotator NewRotation;
	local bool bAdjustDeployRotation;

	ReactivateOccupiedPlayerStarts();

	for( i=0; i < SpawnList.Length; i++ )
	{
		U2PC = U2PlayerController( SpawnList[i].C );

		if( aPlayer != None && U2PC != aPlayer )
			continue;

		if( U2PC? && !U2PC.bDeleteMe &&
			U2PC.PlayerReplicationInfo.bLockedIn &&
			U2PC.PlayerReplicationInfo.RespawnPoint == ReadyToDeployPoint &&
			IsReadyToRedeploy( i ) )
		{
			// spawn player in the direction he was viewing
			bAdjustDeployRotation = U2PC.PlayerReplicationInfo.RespawnPoint == U2PC.RespawnPoint;
			NewRotation = U2PC.GetViewRotation();

			numTries = 0;
			while( numTries < 5  )
			{
				RestartPlayer( U2PC );
				if( U2PC.Pawn? )
				{
					SpawnList[i].bWaiting = false;
					HandleDeploymentResult( U2PC, SpawnList[i].PS, true );
					U2PC.PlayerReplicationInfo.bLockedIn = false;

					if( bAdjustDeployRotation )
					{
						U2PC.Pawn.SetRotation(NewRotation);
						U2PC.Pawn.SetViewRotation(NewRotation);
						U2PC.Pawn.ClientSetRotation(NewRotation);
					}
					break;
				}
				else
				{
					HandleDeploymentResult( U2PC, SpawnList[i].PS, false );
				}
				SpawnList[i].PS = None;
				numTries++;
			}
		}
	}
}	

//-----------------------------------------------------------------------------

function HandleDeploymentResult( U2PlayerController U2PC, PlayerStart PS, bool bSuccess )
{
	local int i;
	local DeploymentPoint DP;

	DP = DeploymentPoint( U2PC.PlayerReplicationInfo.RespawnPoint );

	if( DP.Spawners.Length == 0 )
		Warn( DP @ "has no spawners" );

	for( i=0; i < DP.Spawners.Length; i++ )
	{
		if( DP.Spawners[i] == PS )
		{
			DP.LastSpawnTimes[i] = Level.TimeSeconds;
			if( !bSuccess )
			{
				DP.FailedSpawnAttempts[i] = DP.FailedSpawnAttempts[i] + 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------

function int GetLeastRecentlyUsedPS( DeploymentPoint DP )
{
	local int i, LRU;
	local float OldestTime;

	LRU = -1;
	OldestTime = Level.TimeSeconds;
	for( i=0; i < DP.Spawners.Length; i++ )
	{
		if( DP.LastSpawnTimes[i] < OldestTime )
			LRU = i;
	}
	return LRU;
}

//-----------------------------------------------------------------------------

function ReactivateOccupiedPlayerStarts()
{
	local int LRU;
	local DeploymentPoint DP;

	for( DP=DeploymentPointList; DP!=None; DP=DP.nextDeploymentPoint )
	{
		LRU = GetLeastRecentlyUsedPS( DP );
		if( LRU >= 0 )
		{
			if( DP.FailedSpawnAttempts[LRU] > 0 )
				DP.FailedSpawnAttempts[LRU]--;

			if( DP.FailedSpawnAttempts[LRU] == 0 )
				DP.Spawners[LRU].bEnabled = !DP.Spawners[LRU].IsOccupied();
		}
	}
}

//-----------------------------------------------------------------------------

function DeploymentPoint GetNextDeploymentPoint( DeploymentPoint prevDP, optional Controller C )
{
	local byte overflow;
	local DeploymentPoint DP;

	// start at next DP in the list, wrap if necessary
	if( prevDP != None ) DP = prevDP.nextDeploymentPoint;
	if(     DP == None ) DP = DeploymentPointList;

	// find a DP with the same team
	if( C != None )
	{
		overflow = 0;
		while( overflow < 2 && DP.GetTeam() != C.GetTeam() )
		{
			DP=DP.nextDeploymentPoint;
			if( DP == None )
			{
				DP = DeploymentPointList;
				overflow++;
			}
		}

		if( DP? && (DP.GetTeam() != C.GetTeam()) )
			DP = None;
	}

	return DP;
}

//-----------------------------------------------------------------------------

function NavigationPoint FindPlayerStart( Controller Player, optional byte InTeam, optional	string incomingName	)
{
	local int SpawnIndex;
	local NavigationPoint N;
	local U2PlayerController U2PC;

	U2PC = U2PlayerController(Player);
	if( U2PC? && U2PC.PlayerReplicationInfo.RespawnPoint? )
	{
		if ( GameRulesModifiers	!= None	)
		{
			N =	GameRulesModifiers.FindPlayerStart(Player,InTeam,incomingName);
			if ( N != None )
				return N;
		}

		SpawnIndex = FindSpawnInfoByController( Player );
		if( SpawnList[SpawnIndex].PS == None )
			SpawnList[SpawnIndex].PS = GetAvailPlayerStart( U2PC );
		return SpawnList[SpawnIndex].PS;
	}
	else
	{
		return Super.FindPlayerStart( Player, InTeam, incomingName);
	}
}

//-----------------------------------------------------------------------------

function NotifyDeployPointHacked( DeploymentPoint HackedDP, int OldTeam, Controller PrimaryHacker, optional Controller SecondaryHacker )
{
	local Controller C;
	local U2PlayerController U2PC;

	for( C=Level.ControllerList; C!=None; C=C.nextController )
	{
		U2PC = U2PlayerController(C);
		if( U2PC? && U2PC.GetTeam() == OldTeam )
		{
			if( U2PC.PlayerReplicationInfo.RespawnPoint == HackedDP )	// locked in
			{
				UnLockCurrentDeployPoint( U2PC );
				U2PC.NotifyDeployPointHacked();
			}
			else if( U2PC.RespawnPoint == HackedDP )					// just viewing DP
			{
				U2PC.ServerViewNextDeployPoint();
			}
		}
	}

	ScoreKeeper.NotifyHackedDeployPoint( PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

function Killed( Controller	Killer,	Controller Killed, Pawn	KilledPawn,	class<DamageType> damageType )
{
	local int i;

	ScoreKeeper.NotifyKilled( Killer, Killed, KilledPawn, damageType );

	Super.Killed( Killer, Killed, KilledPawn, damageType );

	for( i=SpawnList.Length-1; i >= 0; i-- )
		if( SpawnList[i].C == Killed )
			SpawnList.Remove( i, 1 );
}

//-----------------------------------------------------------------------------

function DiscardInventory( Pawn	Other )
{
	Other.TossArtifacts();
	Super.DiscardInventory( Other );
}

//-----------------------------------------------------------------------------

function NotifyBreakableDestroyed( Controller Killer, Actor Killed )
{
	ScoreKeeper.NotifyBreakableDestroyed( Killer, Killed );
}

//-----------------------------------------------------------------------------

function NotifyDeployableDestroyed( Controller Killer, Actor Killed )
{
	ScoreKeeper.NotifyDeployableDestroyed( Killer, Killed );
}

//-----------------------------------------------------------------------------

function NotifyHackedDeployPoint( Controller PrimaryHacker, Controller SecondaryHacker )
{
	ScoreKeeper.NotifyHackedDeployPoint( PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

function NotifyHackedOther( Controller PrimaryHacker, Controller SecondaryHacker )
{
	ScoreKeeper.NotifyHackedOther( PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

function NotifyRevived( Controller Reviver, Controller Wounded )
{
	ScoreKeeper.NotifyRevived( Reviver, Wounded );
}

//-----------------------------------------------------------------------------

function NotifyHealTeammateWithPack( Controller PackOwner, Controller PackReceiver )
{
	ScoreKeeper.NotifyHealTeammateWithPack( PackOwner, PackReceiver );
}

//-----------------------------------------------------------------------------

function NotifyRepairTeammateWithPack( Controller PackOwner, Controller PackReceiver )
{
	ScoreKeeper.NotifyRepairTeammateWithPack( PackOwner, PackReceiver );
}

//-----------------------------------------------------------------------------

function NotifyResupplyTeammateWithPack( Controller PackOwner, Controller PackReceiver )
{
	ScoreKeeper.NotifyResupplyTeammateWithPack( PackOwner, PackReceiver );
}

//-----------------------------------------------------------------------------

function NotifyTeammateHealTime( Controller User, Controller Usee, float UseTime )
{
	ScoreKeeper.NotifyTeammateHealTime( User, Usee, UseTime );
}

//-----------------------------------------------------------------------------

function NotifyTeammateRepairTime( Controller User, Controller Usee, float UseTime )
{
	ScoreKeeper.NotifyTeammateRepairTime( User, Usee, UseTime );
}

//-----------------------------------------------------------------------------

function NotifyTeammateResupplyTime( Controller User, Controller Usee, float UseTime )
{
	ScoreKeeper.NotifyTeammateResupplyTime( User, Usee, UseTime );
}

//-----------------------------------------------------------------------------

function ScoreKill(Controller Killer, Controller Other);

//-----------------------------------------------------------------------------

function bool ChangeTeam(Controller	Other, int N, bool bNewTeam)
{
	local int i;
	local TeamInfo NewTeam;
	local U2PlayerController U2PC;

	N = Clamp( N, 0, ArrayCount(GameReplicationInfo.Teams)-1 );
	NewTeam = GameReplicationInfo.Teams[N];

	if( Other.PlayerReplicationInfo.Team != None &&
		Other.PlayerReplicationInfo.Team == NewTeam )
	{
		return false;
	}

	if( Other.PlayerReplicationInfo.Team != None )
	{
		Other.PlayerReplicationInfo.Team.RemoveFromTeam( Other );
	}

	NewTeam.AddToTeam( Other );

	// clear out any respawning info 
	if( U2PlayerController(Other) != None )
	{
		U2PlayerController(Other).RespawnPoint = None;
		Other.PlayerReplicationInfo.RespawnPoint = None;
	}

	for( i=0; i < SpawnList.Length; i++ )
		if( SpawnList[i].C == Other )
			SpawnList[i].PS = None;

	// switch to a new deploy point that matches the new team
	U2PC = U2PlayerController(Other);
	if( U2PC? )
		U2PC.ServerViewNextDeployPoint();

	return true;
}

//-----------------------------------------------------------------------------

function SetPawnClass( PlayerController Player, class<Pawn> NewPawnClass )
{
	local string NewPawnClassString;

	if( Player? )
	{
		// Append any specific strings to base pawn class (team)
		NewPawnClassString = NewPawnClass.static.GetQualifiedClassName( Player.GetTeam() );	
		NewPawnClass = class<Pawn>(DynamicLoadObject( NewPawnClassString, class'Class' ));

		Player.PawnClass = NewPawnClass;
	}
}

//-----------------------------------------------------------------------------
// Inventory

function AddDefaultInventory( pawn PlayerPawn )
{
	Super.AddDefaultInventory( PlayerPawn );
	AddDefaultPowerSuit( PlayerPawn );
}

//-----------------------------------------------------------------------------

function AddDefaultPowerSuit( Pawn PlayerPawn )
{
	local Inventory Inv;
	local U2Pawn U2PP;

	U2PP = U2Pawn(PlayerPawn);
	if( U2PP? )
	{
		for( Inv = U2PP.Inventory; Inv != None; Inv = Inv.Inventory )
		{
			if( ClassIsChildOf( Inv.Class, class'PowerSuit' ) )
				Inv.Destroy(); //removes inventory as well
		}

		if( U2PP.DefaultPowerSuitName != "" )
		{
			class'UtilGame'.static.GiveInventoryString( U2PP, U2PP.DefaultPowerSuitName );
		}
	}
}

//-----------------------------------------------------------------------------
// Artifact support
//-----------------------------------------------------------------------------

// Artifact ID numbers are used to track the artifacts, instead of inventory or pickups
function int NextUniqueID()
{
	local int Result;
	Result = UniqueArtifactID;
	UniqueArtifactID++;
	return Result;
}

//-----------------------------------------------------------------------------

function int GetArtifactID( Actor A )
{
	local int NewID;
	local class<Artifact> ArtType;

	NewID = -1;

	if( ArtifactHolder(A)? )
		ArtType = class<Artifact>( ArtifactHolder(A).InventoryClass );

	if( ArtType? )
	{
		NewID = NextUniqueID();
		NotifyArtifactExists( NewID, A.GetTeam(), ArtType );
	}

	return NewID;
}

//-----------------------------------------------------------------------------

function int ArtifactsNeededToWin()
{
	return ArtInfo.Length;
}

//-----------------------------------------------------------------------------

function CheckForArtifactWin( int WinningTeam, Pawn Carrier )
{
	local IconNode IN;

	//Log("Checking for an artifact Win for team:" @ WinningTeam @ "Score:" @ GameReplicationInfo.Teams[WinningTeam].Score @ "NeededToWin:" @ ArtifactsNeededToWin() );

	if( GameReplicationInfo.Teams[WinningTeam].Score < ArtifactsNeededToWin() )
		return;

	if( WinningTeam == 0 )		GameEndUIEvent = 7;		//Red got all artifacts
	else						GameEndUIEvent = 8;		//Blue got all artifacts

	foreach AllActors( class'IconNode', IN )
		if( IN.GetTeam() == WinningTeam )
		{
			HandleTeamWon( Carrier, IN, "Artifacts" );
			return;
		}
}

//-----------------------------------------------------------------------------

function HandleTeamWon( Pawn Winner, Actor WinningViewTarget, string WinningReason )
{
	local PlayerReplicationInfo WinnerPRI;

	GameReplicationInfo.Winner = WinningViewTarget;

	if( Winner? )
		WinnerPRI = Winner.PlayerReplicationInfo;

	EndGame(WinnerPRI,WinningReason);
	EndTime = Level.TimeSeconds;
}

//-----------------------------------------------------------------------------

function int GetArtifactTeam( int ArtifactID )
{
	local int i;
	for( i=0; i < ArtInfo.Length; i++ )
		if( ArtInfo[i].ID == ArtifactID )
			return ArtInfo[i].Team;
	return 255;
}

//-----------------------------------------------------------------------------

function class<Artifact> GetArtifactType( int ArtifactID )
{
	local int i;
	for( i=0; i < ArtInfo.Length; i++ )
		if( ArtInfo[i].ID == ArtifactID )
			return ArtInfo[i].Type;
	return None;
}

//-----------------------------------------------------------------------------

function NotifyArtifactExists( int ArtifactID, int TeamIndex, class<Artifact> ArtifactType )
{
	//DM("Team" @ TeamIndex @ "is starting out with a" @ ArtifactType @ " (#" @ ArtifactID @ ")");
	UpdateArtifactInfo( ArtifactID, TeamIndex, None, AS_Stored, ArtifactType );
	GameReplicationInfo.Teams[TeamIndex].Score = GameReplicationInfo.Teams[TeamIndex].Score + 1;
}

//-----------------------------------------------------------------------------

function NotifyArtifactStolen( int ArtifactID, Pawn Carrier )
{
	local int TeamIndex;
	TeamIndex = GetArtifactTeam(ArtifactID);
	//Log( GetArtifactType(ArtifactID) @ "Artifact (#" $ ArtifactID $ " / Team:" $ TeamIndex $ ") has been stolen by" @ Carrier.GetHumanReadableName() );
	UpdateArtifactInfo( ArtifactID, TeamIndex, Carrier, AS_Carried );
	GameReplicationInfo.Teams[TeamIndex].Score = GameReplicationInfo.Teams[TeamIndex].Score - 1;
	ScoreKeeper.NotifyArtifactStolen( Carrier.Controller );
	StartArtifactCarryTimer();
}

//-----------------------------------------------------------------------------

function NotifyArtifactStored( int ArtifactID, Pawn Carrier )
{
	local int CarrierTeam;

	CarrierTeam = Carrier.GetTeam();

	//Log( GetArtifactType(ArtifactID) @ "Artifact (#" $ ArtifactID $ ") has been scored by" @ Carrier.GetHumanReadableName() );
	UpdateArtifactInfo( ArtifactID, CarrierTeam, Carrier, AS_Stored );

	GameReplicationInfo.Teams[CarrierTeam ].Score = GameReplicationInfo.Teams[CarrierTeam].Score + 1;
	ScoreKeeper.NotifyArtifactCapturedBy( Carrier.Controller );
	CheckForArtifactWin( CarrierTeam, Carrier );
	RemoveArtifactCarryTimer();
}

//-----------------------------------------------------------------------------

function NotifyArtifactDropped( int ArtifactID, Pawn Carrier )
{
	//Log( GetArtifactType(ArtifactID) @ "Artifact (#" $ ArtifactID $ ") has been dropped by" @ Carrier.GetHumanReadableName() );
	UpdateArtifactInfo( ArtifactID, GetArtifactTeam(ArtifactID), Carrier, AS_Dropped );
	ConsoleCommand( "DumpArtifacts" );		// tbr: for debugging the artifact scoring
	RemoveArtifactCarryTimer();
}

//-----------------------------------------------------------------------------

function NotifyArtifactPickedup( int ArtifactID, Pawn Carrier )
{
	//Log( GetArtifactType(ArtifactID) @ "Artifact (#" $ ArtifactID $ ") has been picked up by" @ Carrier.GetHumanReadableName() );
	UpdateArtifactInfo( ArtifactID, GetArtifactTeam(ArtifactID), Carrier, AS_Carried );
	ScoreKeeper.NotifyArtifactPickedUp( Carrier.Controller );
	ConsoleCommand( "DumpArtifacts" );		// tbr: for debugging the artifact scoring
	StartArtifactCarryTimer();
}

//-----------------------------------------------------------------------------

function NotifyArtifactSentHome( int ArtifactID, Pawn Carrier )
{
	local int TeamIndex;
	local IconNode IN;
	local int INCheckCount;

	TeamIndex = GetArtifactTeam( ArtifactID );
	//Log( GetArtifactType(ArtifactID) @ "Artifact (#" $ ArtifactID $ ") has been sent home to team" @ TeamIndex );
	if( Carrier? ) DM("Sent home carrier:" @ Carrier @ "team:" @ Carrier.GetTeam() );
	UpdateArtifactInfo( ArtifactID, TeamIndex, None, AS_Stored );

	foreach AllActors( class'IconNode', IN )
		if( IN.GetTeam() == TeamIndex )
		{
			INCheckCount++;
			if( IN.SentHome( ArtifactID ) )
			{
				GameReplicationInfo.Teams[TeamIndex].Score = GameReplicationInfo.Teams[TeamIndex].Score + 1;
				if( Carrier? )
				{
					CheckForArtifactWin( TeamIndex, Carrier );
					ScoreKeeper.NotifyArtifactRecoveredBy( Carrier.Controller );
				}
				else
				{
					CheckForArtifactWin( TeamIndex, None );		// artifact faded out and sent home
				}
				ConsoleCommand( "DumpArtifacts" );		// tbr: for debugging the artifact scoring
				return;
			}
		}

	ConsoleCommand( "DumpArtifacts" );		// tbr: for debugging the artifact scoring
	Warn("Unable to send home artifact #:" @ ArtifactID @ "IconNodes with Team:" @ TeamIndex @ "checked:" @ INCheckCount );
	RemoveArtifactCarryTimer();
}

//-----------------------------------------------------------------------------

function UpdateArtifactInfo( int ArtifactID, int NewTeam, Pawn NewCarrier, EArtifactState NewArtState, optional class<Artifact> ArtifactType )
{
	local int i, NewIndex;

	//Log("UpdateArtifactInfo -- ID:" @ ArtifactID @ "Team:" @ NewTeam @ "Carrier:" @ NewCarrier @ "State:" @ NewArtState @ "Type;" @ ArtifactType );

	NewIndex = -1;
	for( i=0; i < ArtInfo.Length; i++ )
		if( ArtInfo[i].ID == ArtifactID )
		{
			NewIndex = i;
			break;
		}

	if( NewIndex < 0 )
	{
		NewIndex = ArtInfo.Length;
		ArtInfo.Length = ArtInfo.Length + 1;
		ArtInfo[NewIndex].Type = ArtifactType;
	}

	ArtInfo[NewIndex].ID	  = ArtifactID;
	ArtInfo[NewIndex].Team	  = NewTeam;
	ArtInfo[NewIndex].Carrier = NewCarrier;
	ArtInfo[NewIndex].State	  = NewArtState;
	if( NewArtState == 	AS_Carried )
		ArtInfo[NewIndex].PickedUpTime = Level.TimeSeconds;

	UpdateArtifactScore( NewIndex );
}

//-----------------------------------------------------------------------------

function StartArtifactCarryTimer()
{
//	RemoveTimer( 'ArtifactCarryTimer' );
//	AddTimer( 'ArtifactCarryTimer', MaxArtifactCarryTime );
}

//-----------------------------------------------------------------------------

function RemoveArtifactCarryTimer()
{
//	RemoveTimer( 'ArtifactCarryTimer' );
}

//-----------------------------------------------------------------------------

function ArtifactCarryTimer()
{
/*	local int i;

	DMTN("ArtifactCarryTimer -- time:" @ Level.TimeSeconds );
	for( i=0; i < ArtInfo.Length; i++ )
	{
		DMTN("Artifact#:" @ i @ "picked up time:" @ ArtInfo[i].PickedUpTime );
		if( ArtInfo[i].PickedUpTime > 0.0 &&
			Level.TimeSeconds - ArtInfo[i].PickedUpTime > MaxArtifactCarryTime )
		{
			NotifyArtifactSentHome( ArtInfo[i].ID, None );
		}
	}
*/
}

//-----------------------------------------------------------------------------

function InitArtifactScoreboard()
{
	local int i;
	for( i=0; i < ArtInfo.Length; i++ )
		UpdateArtifactScore(i);
}

//-----------------------------------------------------------------------------

function UpdateArtifactScore( int ArtIndex )
{
	local EArtifactState AState;
	local class<Artifact> AType;
	local int ATeam;
	local GameReplicationInfo GRI;

	GRI = GameReplicationInfo;
	if( GRI == None )
		return;

	AState = ArtInfo[ArtIndex].State;
	AType  = ArtInfo[ArtIndex].Type;
	ATeam  = ArtInfo[ArtIndex].Team;

	switch( ArtIndex )
	{
	case 0:

		// clear flags
		GRI.bArtBG_Slot0_Red_Dropped  = false; GRI.bArtBG_Slot0_Red_Stored  = false; GRI.bArtBG_Slot0_Red_Carried  = false;
		GRI.bArtBG_Slot0_Blue_Dropped = false; GRI.bArtBG_Slot0_Blue_Stored = false; GRI.bArtBG_Slot0_Blue_Carried = false;
		GRI.bArt_Defense_Slot0		= false;
		GRI.bArt_Health_Slot0		= false;
		GRI.bArt_Shield_Slot0		= false;
		GRI.bArt_Skill_Slot0		= false;
		GRI.bArt_Stamina_Slot0		= false;
		GRI.bArt_WeaponDamage_Slot0 = false;

		// artifact background
		switch( ATeam )
		{
		case 0:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot0_Red_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot0_Red_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot0_Red_Dropped = true;
			break;

		case 1:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot0_Blue_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot0_Blue_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot0_Blue_Dropped = true;
			break;

		default:
			Warn("Artifact has unknown team:" @ ATeam );
		}

		// artifact icon
			 if( AType == class'ArtifactInvDefense' )		GRI.bArt_Defense_Slot0		= true;
		else if( AType == class'ArtifactInvHealth' )		GRI.bArt_Health_Slot0		= true;
		else if( AType == class'ArtifactInvShield' )		GRI.bArt_Shield_Slot0		= true;
		else if( AType == class'ArtifactInvSkill' )			GRI.bArt_Skill_Slot0		= true;
		else if( AType == class'ArtifactInvStamina' )		GRI.bArt_Stamina_Slot0		= true;
		else if( AType == class'ArtifactInvWeaponDamage' )	GRI.bArt_WeaponDamage_Slot0 = true;

		break;

	case 1:

		// clear flags
		GRI.bArtBG_Slot1_Red_Dropped  = false; GRI.bArtBG_Slot1_Red_Stored  = false; GRI.bArtBG_Slot1_Red_Carried  = false;
		GRI.bArtBG_Slot1_Blue_Dropped = false; GRI.bArtBG_Slot1_Blue_Stored = false; GRI.bArtBG_Slot1_Blue_Carried = false;
		GRI.bArt_Defense_Slot1		= false;
		GRI.bArt_Health_Slot1		= false;
		GRI.bArt_Shield_Slot1		= false;
		GRI.bArt_Skill_Slot1		= false;
		GRI.bArt_Stamina_Slot1		= false;
		GRI.bArt_WeaponDamage_Slot1 = false;

		// artifact background
		switch( ATeam )
		{
		case 0:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot1_Red_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot1_Red_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot1_Red_Dropped = true;
			break;

		case 1:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot1_Blue_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot1_Blue_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot1_Blue_Dropped = true;
			break;

		default:
			Warn("Artifact has unknown team:" @ ATeam );
		}

		// artifact icon
			 if( AType == class'ArtifactInvDefense' )		GRI.bArt_Defense_Slot1		= true;
		else if( AType == class'ArtifactInvHealth' )		GRI.bArt_Health_Slot1		= true;
		else if( AType == class'ArtifactInvShield' )		GRI.bArt_Shield_Slot1		= true;
		else if( AType == class'ArtifactInvSkill' )			GRI.bArt_Skill_Slot1		= true;
		else if( AType == class'ArtifactInvStamina' )		GRI.bArt_Stamina_Slot1		= true;
		else if( AType == class'ArtifactInvWeaponDamage' )	GRI.bArt_WeaponDamage_Slot1 = true;

		break;

	case 2:

		// clear flags
		GRI.bArtBG_Slot2_Red_Dropped  = false; GRI.bArtBG_Slot2_Red_Stored  = false; GRI.bArtBG_Slot2_Red_Carried  = false;
		GRI.bArtBG_Slot2_Blue_Dropped = false; GRI.bArtBG_Slot2_Blue_Stored = false; GRI.bArtBG_Slot2_Blue_Carried = false;
		GRI.bArt_Defense_Slot2		= false;
		GRI.bArt_Health_Slot2		= false;
		GRI.bArt_Shield_Slot2		= false;
		GRI.bArt_Skill_Slot2		= false;
		GRI.bArt_Stamina_Slot2		= false;
		GRI.bArt_WeaponDamage_Slot2 = false;

		// artifact background
		switch( ATeam )
		{
		case 0:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot2_Red_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot2_Red_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot2_Red_Dropped = true;
			break;

		case 1:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot2_Blue_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot2_Blue_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot2_Blue_Dropped = true;
			break;

		default:
			Warn("Artifact has unknown team:" @ ATeam );
		}

		// artifact icon
			 if( AType == class'ArtifactInvDefense' )		GRI.bArt_Defense_Slot2		= true;
		else if( AType == class'ArtifactInvHealth' )		GRI.bArt_Health_Slot2		= true;
		else if( AType == class'ArtifactInvShield' )		GRI.bArt_Shield_Slot2		= true;
		else if( AType == class'ArtifactInvSkill' )			GRI.bArt_Skill_Slot2		= true;
		else if( AType == class'ArtifactInvStamina' )		GRI.bArt_Stamina_Slot2		= true;
		else if( AType == class'ArtifactInvWeaponDamage' )	GRI.bArt_WeaponDamage_Slot2 = true;

		break;

	case 3:

		// clear flags
		GRI.bArtBG_Slot3_Red_Dropped  = false; GRI.bArtBG_Slot3_Red_Stored  = false; GRI.bArtBG_Slot3_Red_Carried  = false;
		GRI.bArtBG_Slot3_Blue_Dropped = false; GRI.bArtBG_Slot3_Blue_Stored = false; GRI.bArtBG_Slot3_Blue_Carried = false;
		GRI.bArt_Defense_Slot3		= false;
		GRI.bArt_Health_Slot3		= false;
		GRI.bArt_Shield_Slot3		= false;
		GRI.bArt_Skill_Slot3		= false;
		GRI.bArt_Stamina_Slot3		= false;
		GRI.bArt_WeaponDamage_Slot3 = false;

		// artifact background
		switch( ATeam )
		{
		case 0:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot3_Red_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot3_Red_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot3_Red_Dropped = true;
			break;

		case 1:
			     if( AState == AS_Stored ) 				GRI.bArtBG_Slot3_Blue_Stored  = true;
			else if( AState == AS_Carried )				GRI.bArtBG_Slot3_Blue_Carried = true;
			else if( AState == AS_Dropped )				GRI.bArtBG_Slot3_Blue_Dropped = true;
			break;

		default:
			Warn("Artifact has unknown team:" @ ATeam );
		}

		// artifact icon
			 if( AType == class'ArtifactInvDefense' )		GRI.bArt_Defense_Slot3		= true;
		else if( AType == class'ArtifactInvHealth' )		GRI.bArt_Health_Slot3		= true;
		else if( AType == class'ArtifactInvShield' )		GRI.bArt_Shield_Slot3		= true;
		else if( AType == class'ArtifactInvSkill' )			GRI.bArt_Skill_Slot3		= true;
		else if( AType == class'ArtifactInvStamina' )		GRI.bArt_Stamina_Slot3		= true;
		else if( AType == class'ArtifactInvWeaponDamage' )	GRI.bArt_WeaponDamage_Slot3 = true;

		break;

	default:
		Warn("Artifact index out of scoreboard range:" @ ArtIndex );
	}
}

//------------------------------------------------------------------------------
// Game States

event PostLogin( PlayerController NewPlayer	)
{
	Super.PostLogin( NewPlayer );
	InitArtifactScoreboard();
}

function StartMatch()
{
    RemainingTime = 60 * TimeLimit + 1;		// plus one so that whole number minutes shows on the hud
	Deploy();
	GotoState('MatchInProgress');
	Super.StartMatch();
}

function EndGame(PlayerReplicationInfo Winner, string Reason )
{
	Super.EndGame(Winner,Reason);
	GotoState('MatchOver');
}

auto state PendingMatch
{
Begin:
	if( Level.NetMode != NM_StandAlone )
		Sleep( PreMatchWait );
	StartMatch();
}

state MatchInProgress
{
    function Timer()
    {
        Global.Timer();
		Deploy();
	    if ( TimeLimit > 0 && Level.NetMode != NM_Standalone )
        {
            GameReplicationInfo.bStopCountDown = false;
            RemainingTime--;
            GameReplicationInfo.RemainingTime = RemainingTime;
            if ( RemainingTime % 60 == 0 )
                GameReplicationInfo.RemainingMinute = RemainingTime;
            if ( RemainingTime < 0 )
			{
				HandleTeamWon( None, None, "TimeLimit");
				GameEndUIEvent = 11;	// Game over - no winner?
			}
        }
        ElapsedTime++;
        GameReplicationInfo.ElapsedTime = ElapsedTime;
		UpdateTeamEnergy( 1.0 );
    }

    function BeginState()
    {
		local PlayerReplicationInfo PRI;
		foreach DynamicActors(class'PlayerReplicationInfo',PRI)
			PRI.StartTime = 0;
		ElapsedTime = 0;
	}
}

state MatchOver
{
	function RestartPlayer( Controller aPlayer ) {}
	function ScoreKill(Controller Killer, Controller Other) {}
	function float DrainTeamEnergy( int TeamNumber, float EnergyAmount, name Reason );
	function int ReduceDamage( int Damage, pawn injured, pawn instigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType )
	{
		return 0;
	}

	function bool ChangeTeam(Controller Other, int num, bool bNewTeam)
	{
		return false;
	}

    function Timer()
    {
		local Controller C;

        Global.Timer();

        if ( Level.TimeSeconds > EndTime + RestartWait )
            RestartGame();

		for (C=Level.ControllerList;C?;C=C.NextController)
			if (PlayerController(C)?)
				PlayerController(C).ClientSendEvent(GameEndUIEvent);
	}

	function ShowStats()
	{
		local Controller C;
		for (C=Level.ControllerList;C?;C=C.NextController)
			if (PlayerController(C)?)
				PlayerController(C).ClientSendEvent( 14 );	// 	SendEvent("LevelLoadStart")
	}

    function BeginState()
    {
		local Controller C;

		if ( GameReplicationInfo.Winner != None )
		{
			GameReplicationInfo.Winner.bAlwaysRelevant = true;
			for (C=Level.ControllerList;C?;C=C.NextController)
				if (PlayerController(C)?)
					PlayerController(C).ClientSetViewtarget(GameReplicationInfo.Winner);
		}

		AddTimer( 'ShowStats', RestartWait / 2 );

		GameReplicationInfo.bStopCountDown = true;

		ConsoleCommand( "DumpArtifacts" );		// tbr: for debugging the artifact scoring

		if( ScoreKeeper? )
			ScoreKeeper.SendScores();
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	PlayerControllerClassName="U2.U2PlayerController"
	MapPrefix="XMP"
	MapListType="U2.MapListXMP"
	bTeamGame=true
	bDelayedStart=true
	bRestartLevel=false
	RespawnDelaySeconds=25
	PreMatchWait=60
	RestartWait=20
	TimeLimit=20
	TeamEnergyMax=1.2000
	TeamEnergyInitial=0.4990
	TeamDamage=0.5
	MaxArtifactCarryTime=240
	LockedInSound=sound'U2A.Messages.DeployLocked'

	TeamFeatureWarningThreshold=0.0200
	EnergyThresholds(0)=(Feature="PackBase",Threshold=0.8000)
	EnergyThresholds(1)=(Feature="XMPStationaryTurret",Threshold=0.7500)
	EnergyThresholds(2)=(Feature="RocketTurret",Threshold=0.7000)
	EnergyThresholds(3)=(Feature="LandMines",Threshold=0.6500)
	EnergyThresholds(4)=(Feature="FieldGenerator",Threshold=0.6000)
	EnergyThresholds(5)=(Feature="LaserTripMines",Threshold=0.5500)
	EnergyThresholds(6)=(Feature="AutoTurret",Threshold=0.5000)
	EnergyThresholds(7)=(Feature="XMPVehicle",Threshold=0.3500)
	EnergyThresholds(8)=(Feature="PowerStation",Threshold=0.3000)
	EnergyThresholds(9)=(Feature="IconNode",Threshold=0.2000)
}