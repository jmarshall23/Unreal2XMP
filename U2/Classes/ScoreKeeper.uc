//=============================================================================
// ScoreKeeper.uc
//=============================================================================

class ScoreKeeper extends Actor;

//-----------------------------------------------------------------------------

enum EScoreType
{
	ST_ArtifactCaptureCarrier,				
	ST_ArtifactCaptureTeam,
	ST_ArtifactPickedUp,
	ST_ArtifactRecover,
	ST_ArtifactSteal,
	ST_FragDeployable,
	ST_FragDestroyable,
	ST_FragPlayer,
	ST_FragPlayerByDeployable,
	ST_FragTurretGiant,
	ST_FragTurretMedium,
	ST_FragTurretOther,
	ST_FragTurretSmall,
	ST_FragVehicleHarbinger,
	ST_FragVehicleJuggernaut,
	ST_FragVehicleOther,
	ST_FragVehicleRaptor,
	ST_HackAssist,
	ST_HackDeployPoint,
	ST_HackEnergySource,
	ST_HackOther,
	ST_HealTeammatePerSec,
	ST_HealTeammateWithPack,
	ST_RepairTeammatePerSec,
	ST_RepairTeammateWithPack,
	ST_ResupplyTeammatePerSec,
	ST_ResupplyTeammateWithPack,
	ST_Revive,
};

//-----------------------------------------------------------------------------

enum EBonusType
{
	BT_ArtifactCarry,
	BT_ArtifactRegisterDenial,
	BT_DefendArtifactCarrier,
	BT_DefendDroppedArtifact,
	BT_DefendEnergySource,
	BT_DefendHacker,
	BT_DefendIconNode,
	BT_FragArtifactCarrier,
	BT_FragEnemyDefender,
	BT_FragEnemyHacker,
	BT_FragHeadshot,
	BT_FragPerfectAccuracy,
	BT_FragPerfectAccuracyChained,
	BT_FragSelf,
	BT_FragTeammate,
	BT_HealArtifactCarrier,
	BT_HealTeammatePerSec,
	BT_MonsterKill,
	BT_RepairArtifactCarrier,
	BT_RepairTeammatePerSec,
	BT_ResupplyArtifactCarrier,
	BT_ResupplyTeammatePerSec,
	BT_SupplyingEnemy,
	BT_VehicularManslaughter
};

//-----------------------------------------------------------------------------

struct ScoreDefT
{
	var EScoreType Type;
	var int Score;
};

//-----------------------------------------------------------------------------

struct BonusDefT
{
	var EBonusType Type;
	var float Multiplier;
};

//-----------------------------------------------------------------------------

struct ScoreProfileT
{
	var int PlayerID;
	var array<float> ScoreFreq;
	var array<float> BonusFreq;
};

//-----------------------------------------------------------------------------

// score Definitions and Labels
var array<ScoreDefT> ScoreDefs;					// point catagories
var array<BonusDefT> BonusDefs;					// bonus multipiers

var localized array<string> ScoreLabels;		// strings to display on hud for raw points
var localized array<string> BonusLabels;		// strings to display on hud for multipliers
var localized string TotalScoreMsg;

// the score keeping for all of the players
var array<ScoreProfileT> ScoreProfiles;			// Player Scores

// MVP stats
var array<string> MVPScoreNames;
var array<string> MVPBonusNames;
var array<float> MVPScoreStats;
var array<float> MVPBonusStats;

// the current multipliers to affect the current score
var array<EBonusType> CurrentBonuses;			// multipliers to be applied to the current score
var EScoreType CurrentScore;					// scoring category
var float SpecialScore;							// used for timed score events
var string DefaultMVPName;

// game-related score parameters
var float RecentlyOccupiedVehicleTime;
var float RecentlyOccupiedTurretTime;
var float AttackingIconNodeDistance;
var float DefendingIconNodeDistance;
var float AttackingEnergySourceDistance;
var float DefendingEnergySourceDistance;
var float MaxMonsterKillDuration;
var float DroppedArtifactDistance;
var float DefendHackerDistance;
var float DefendArtifactCarrierDistance;

//-----------------------------------------------------------------------------
// Interfaces

function NotifyKilled( Controller Killer, Controller Killed, Pawn KilledPawn, class<DamageType> damageType )
{
	local U2Pawn KilledP;
	local U2PlayerController KillerC;
	local KVehicle V;
	local StationaryTurret ST;

	if( !Killer || !Killed )
		return;

	KilledP = U2Pawn( KilledPawn );
	if( KilledP? )				// player was killed
	{
		//
		// multipliers
		//
		if( HasArtifact( KilledPawn ) )
			RegisterMultiplier( Killer, BT_FragArtifactCarrier );

		if( HasArtifactC( Killer ) )
			RegisterMultiplier( Killer, BT_ArtifactCarry );

		if( CloseToActorTeam( KilledPawn, DefendingIconNodeDistance, 'IconNode', true ) )
		{
			if( HasArtifact( KilledPawn ) )
				RegisterMultiplier( Killer, BT_ArtifactRegisterDenial );
			else
				RegisterMultiplier( Killer, BT_FragEnemyDefender );
		}

		if( CloseToActorTeam( KilledPawn, DefendingEnergySourceDistance, 'EnergySource', true ) )
		{
			RegisterMultiplier( Killer, BT_FragEnemyDefender );
		}

		if( IsHacking( KilledPawn ) )
		{
			RegisterMultiplier( Killer, BT_FragEnemyHacker );
		}

		if( KilledP.bLastDamageWasHeadshot )
		{
			RegisterMultiplier( Killer, BT_FragHeadshot );
		}
		
		KillerC = U2PlayerController(Killer);
		if( KillerC? && Killer != Killed && KillerC.Level.TimeSeconds - KillerC.LastPlayerFragTime < MaxMonsterKillDuration )
		{
			RegisterMultiplier( Killer, BT_MonsterKill );
			KillerC.MonsterKills++;
		}
		else
		{
			KillerC.MonsterKills = 0;
		}

		if( CloseToActorTeam( Killer, AttackingEnergySourceDistance, 'EnergySource', true ) )
		{
			RegisterMultiplier( Killer, BT_DefendEnergySource );
		}

		if( CloseToActorTeam( Killer, AttackingIconNodeDistance, 'IconNode', true ) )
		{
			RegisterMultiplier( Killer, BT_DefendIconNode );
		}

		if( CloseToActor( KilledPawn, DroppedArtifactDistance, 'ArtifactPickup' ) )
		{
			RegisterMultiplier( Killer, BT_DefendDroppedArtifact );
		}

		if( CloseToTeammateHacker( KilledPawn, DefendHackerDistance ) )
		{
			RegisterMultiplier( Killer, BT_DefendHacker );
		}

		if( CloseToTeammateArtifactCarrier( KilledPawn, DefendArtifactCarrierDistance ) )
		{
			RegisterMultiplier( Killer, BT_DefendArtifactCarrier );
		}

		if( Killer.Pawn.IsA('KVehicle') )
		{
			RegisterMultiplier( Killer, BT_VehicularManslaughter );
		}

		if( Killer == Killed )
		{
			RegisterMultiplier( Killer, BT_FragSelf );
		}
		else if( Killer.GetTeam() == Killed.GetTeam() )
		{
			RegisterMultiplier( Killer, BT_FragTeammate );
		}

		//
		// points
		//
		if( PlayerController(Killer)? )
			RegisterScore( Killer, ST_FragPlayer );
		else
			RegisterScore( Killer, ST_FragPlayerByDeployable );

		// keep track of last kill time for monster kills
		if( KillerC? )
			KillerC.LastPlayerFragTime = KillerC.Level.TimeSeconds;
	}
	else if( KilledPawn.IsA( 'DeployedUnit' ) )	// deployable was killed
	{
		RegisterScore( Killer, ST_FragDeployable );
	}
	else if( KilledPawn.IsA( 'KVehicle' ) )
	{
		V = KVehicle(KilledPawn);

		// enemy vehicle or recently-used vehicle by enemy
		if( V.GetTeam() != Killer.GetTeam() ||
			(V.LastOccupiedTeam != Killer.GetTeam() &&
			 V.LastOccupiedTime - Killer.Level.TimeSeconds < RecentlyOccupiedVehicleTime) )
		{
			if( KilledPawn.IsA( 'Raptor' ) )
			{
				RegisterScore( Killer, ST_FragVehicleRaptor );
			}
			else if( KilledPawn.IsA( 'Harbinger' ) )
			{
				RegisterScore( Killer, ST_FragVehicleHarbinger );
			}
			else if( KilledPawn.IsA( 'Juggernaut' ) )
			{
				RegisterScore( Killer, ST_FragVehicleJuggernaut );
			}
			else
			{
				RegisterScore( Killer, ST_FragVehicleOther );
			}
		}
	}
	else if( KilledPawn.IsA( 'StationaryTurret' ) )
	{
		ST = StationaryTurret( KilledPawn );

		// enemy turret or recently-used turret by enemy
		if( ST.GetTeam() != Killer.GetTeam() ||
			(ST.LastOccupiedTeam != Killer.GetTeam() &&
			 ST.LastOccupiedTime - Killer.Level.TimeSeconds < RecentlyOccupiedTurretTime) )
		{
			if( KilledPawn.IsA( 'BaseGiantTurret' ) ||
				KilledPawn.IsA( 'LowlandsGiantTurret' ) )
			{
				RegisterScore( Killer, ST_FragTurretGiant );
			}
			else if( KilledPawn.IsA( 'GardenTurret' ) ||
					 KilledPawn.IsA( 'LowlandsMediumWingedTurret' ) )
			{
				RegisterScore( Killer, ST_FragTurretMedium );
			}
			else if( KilledPawn.IsA( 'CarnivalTurret' ) ||
					 KilledPawn.IsA( 'LowlandsRoundhouseTurret' ) )
			{
				RegisterScore( Killer, ST_FragTurretSmall );
			}
			else
			{
				RegisterScore( Killer, ST_FragTurretOther );
			}
		}
	}

	ComputeScore( Killer );
}

//-----------------------------------------------------------------------------

function NotifyDeployableDestroyed( Controller Killer, Actor Killed )
{
	if( Killer? && Killed? && Killer.GetTeam() != Killed.GetTeam() )
	{
		RegisterScore( Killer, ST_FragDeployable );
		ComputeScore( Killer );
	}
}

//-----------------------------------------------------------------------------

function NotifyBreakableDestroyed( Controller Killer, Actor Killed )
{
	if( Killer? && Killed? && Killer.GetTeam() != Killed.GetTeam() )
	{
		RegisterScore( Killer, ST_FragDestroyable );
		ComputeScore( Killer );
	}
}

//-----------------------------------------------------------------------------

function NotifyArtifactCapturedBy( Controller Scorer )
{
	local int ScoringTeam;
	local Controller C;

	if( Scorer? )
	{
		RegisterScore( Scorer, ST_ArtifactCaptureCarrier );
		ComputeScore( Scorer );

		ScoringTeam = Scorer.GetTeam();
		for( C=Level.ControllerList; C!=None; C=C.nextController )
		{
			if( PlayerController(C)? && C.GetTeam() == ScoringTeam && C != Scorer )
			{
				RegisterScore( C, ST_ArtifactCaptureTeam );
				ComputeScore( C );
			}
		}
	}
}

//-----------------------------------------------------------------------------

function NotifyArtifactStolen( Controller Stealer )
{
	if( Stealer? )
	{
		RegisterScore( Stealer, ST_ArtifactSteal );
		ComputeScore( Stealer );
	}
}

//-----------------------------------------------------------------------------

function NotifyArtifactRecoveredBy( Controller Recoverer )
{
	if( Recoverer? )
	{
		RegisterScore( Recoverer, ST_ArtifactRecover );
		ComputeScore( Recoverer );
	}
}

//-----------------------------------------------------------------------------

function NotifyArtifactPickedUp( Controller Carrier )
{
	if( Carrier? )
	{
		RegisterScore( Carrier, ST_ArtifactPickedUp );
		ComputeScore( Carrier );
	}
}

//-----------------------------------------------------------------------------

function ScoreHack( EScoreType HackType, Controller PrimaryHacker, Controller SecondaryHacker )
{
	if( PrimaryHacker? )
	{
		if( HasArtifactC( PrimaryHacker ) )
			RegisterMultiplier( PrimaryHacker, BT_ArtifactCarry );

		RegisterScore( PrimaryHacker, HackType );
		ComputeScore( PrimaryHacker );
	}
	if( SecondaryHacker? )
	{
		if( HasArtifactC( SecondaryHacker ) )
			RegisterMultiplier( SecondaryHacker, BT_ArtifactCarry );

		RegisterScore( SecondaryHacker, ST_HackAssist );
		ComputeScore( SecondaryHacker );
	}
}

//-----------------------------------------------------------------------------

function NotifyHackedDeployPoint( Controller PrimaryHacker, Controller SecondaryHacker )
{
	ScoreHack( ST_HackDeployPoint, PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

function NotifyHackedEnergySource( Controller PrimaryHacker, Controller SecondaryHacker )
{
	ScoreHack( ST_HackEnergySource, PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

function NotifyHackedOther( Controller PrimaryHacker, Controller SecondaryHacker )
{
	ScoreHack( ST_HackOther, PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

function NotifyHealTeammateWithPack( Controller PackOwner, Controller PackReceiver )
{
	if( PackOwner? && PackOwner != PackReceiver )
	{
		if( PackOwner.GetTeam() != PackReceiver.GetTeam() )
			RegisterMultiplier( PackOwner, BT_SupplyingEnemy );
		RegisterScore( PackOwner, ST_HealTeammateWithPack );
		ComputeScore( PackOwner );
	}
}

//-----------------------------------------------------------------------------

function NotifyRepairTeammateWithPack( Controller PackOwner, Controller PackReceiver )
{
	if( PackOwner? && PackOwner != PackReceiver )
	{
		if( PackOwner.GetTeam() != PackReceiver.GetTeam() )
			RegisterMultiplier( PackOwner, BT_SupplyingEnemy );
		RegisterScore( PackOwner, ST_RepairTeammateWithPack );
		ComputeScore( PackOwner );
	}
}

//-----------------------------------------------------------------------------

function NotifyResupplyTeammateWithPack( Controller PackOwner, Controller PackReceiver )
{
	if( PackOwner? && PackOwner != PackReceiver )
	{
		if( PackOwner.GetTeam() != PackReceiver.GetTeam() )
			RegisterMultiplier( PackOwner, BT_SupplyingEnemy );
		RegisterScore( PackOwner, ST_ResupplyTeammateWithPack );
		ComputeScore( PackOwner );
	}
}

//-----------------------------------------------------------------------------

function NotifyTeammateHealTime( Controller User, Controller Healed, float UseTime )
{
	if( User? )
	{
		if( HasArtifactC( Healed ) )
			RegisterMultiplier( User, BT_HealArtifactCarrier );
		RegisterMultiplier( User, BT_HealTeammatePerSec );
		RegisterScore( User, ST_HealTeammatePerSec, UseTime );
		ComputeScore( User );
	}
}

//-----------------------------------------------------------------------------

function NotifyTeammateRepairTime( Controller User, Controller Repaired, float UseTime )
{
	if( User? )
	{
		if( HasArtifactC( Repaired ) )
			RegisterMultiplier( User, BT_RepairArtifactCarrier );
		RegisterMultiplier( User, BT_RepairTeammatePerSec );
		RegisterScore( User, ST_RepairTeammatePerSec, UseTime );
		ComputeScore( User );
	}
}

//-----------------------------------------------------------------------------

function NotifyTeammateResupplyTime( Controller User, Controller Resupplied, float UseTime )
{
	if( User? )
	{
		if( HasArtifactC( User ) )
			RegisterMultiplier( User, BT_ResupplyArtifactCarrier );
		RegisterMultiplier( User, BT_ResupplyTeammatePerSec );
		RegisterScore( User, ST_ResupplyTeammatePerSec, UseTime );
		ComputeScore( User );
	}
}

//-----------------------------------------------------------------------------

function NotifyRevived( Controller Reviver, Controller Wounded )
{
	if( Reviver? )
	{
		RegisterScore( Reviver, ST_Revive );
		ComputeScore( Reviver );
	}
}

//-----------------------------------------------------------------------------
// Helpers

function RegisterScore( Controller InPlayer, EScoreType InScoreType, optional float CustomScore )
{
	local int i, ProfileIndex;
	local float RegisteredScore;

	// store score in the player's score profile
	ProfileIndex = -1;
	for( i=0; i < ScoreProfiles.Length; i++ )
		if( ScoreProfiles[i].PlayerID == InPlayer.PlayerReplicationInfo.PlayerID )
			ProfileIndex = i;

	if( ProfileIndex < 0 )
	{
		ScoreProfiles.Insert(0,1);
		ScoreProfiles[0].PlayerID = InPlayer.PlayerReplicationInfo.PlayerID;
		ScoreProfiles[0].BonusFreq.Length = BonusDefs.Length;
		ScoreProfiles[0].ScoreFreq.Length = ScoreDefs.Length;
		ProfileIndex = 0;
	}

	if( CustomScore > 0 )
	{
		ScoreProfiles[ProfileIndex].ScoreFreq[InScoreType] += CustomScore;
		SpecialScore = CustomScore;
	}
	else
	{
		ScoreProfiles[ProfileIndex].ScoreFreq[InScoreType] += 1;
	}

	RegisteredScore = CustomScore;
	CurrentScore = InScoreType;

	// update mvp as needed
	if( MVPScoreStats[InScoreType] < ScoreProfiles[ProfileIndex].ScoreFreq[InScoreType] )
	{
		MVPScoreNames[InScoreType] = Level.Game.FindPlayerByID( ScoreProfiles[ProfileIndex].PlayerID );
		MVPScoreStats[InScoreType] = ScoreProfiles[ProfileIndex].ScoreFreq[InScoreType];
	}
}

//-----------------------------------------------------------------------------

function RegisterMultiplier( Controller InPlayer, EBonusType InBonusType )
{
	local int i, ProfileIndex;

	// store multiplier in the player's score profile
	ProfileIndex = -1;
	for( i=0; i < ScoreProfiles.Length; i++ )
		if( ScoreProfiles[i].PlayerID == InPlayer.PlayerReplicationInfo.PlayerID )
			ProfileIndex = i;

	if( ProfileIndex < 0 )
	{
		ScoreProfiles.Insert(0,1);
		ScoreProfiles[0].PlayerID = InPlayer.PlayerReplicationInfo.PlayerID;
		ScoreProfiles[0].BonusFreq.Length = BonusDefs.Length;
		ScoreProfiles[0].ScoreFreq.Length = ScoreDefs.Length;
		ProfileIndex = 0;
	}

	ScoreProfiles[ProfileIndex].BonusFreq[InBonusType] += 1;

	CurrentBonuses.Insert(0,1);
	CurrentBonuses[0] = InBonusType;

	// update mvp as needed
	if( MVPBonusStats[InBonusType] < ScoreProfiles[ProfileIndex].BonusFreq[InBonusType] )
	{
		MVPBonusNames[InBonusType] = Level.Game.FindPlayerByID( ScoreProfiles[ProfileIndex].PlayerID );
		MVPBonusStats[InBonusType] = ScoreProfiles[ProfileIndex].BonusFreq[InBonusType];
	}
}

//-----------------------------------------------------------------------------

function ComputeScore( Controller Player )
{
	local int i;
	local string Msg;
	local float CurrentMultiplier, TotalMultiplier, RawScore, GrandTotal;
	local U2PlayerController PC;

	PC = U2PlayerController(Player);
	if( !PC )
		return;

	if( SpecialScore > 0 )
	{
		RawScore = SpecialScore;
		SpecialScore = 0;
	}
	else
	{
		RawScore = ScoreDefs[CurrentScore].Score;
	}

	Msg = ScoreLabels[CurrentScore] @ "=>" @ RawScore;
	PC.ClientDisplayScoreMsg( Msg );

	TotalMultiplier = 1.0;

	// display multipliers if any
	for( i=0; i < CurrentBonuses.Length; i++ )
	{
		// monster kill handing
		if( CurrentBonuses[i] == BT_MonsterKill )
		{
			CurrentMultiplier = 1.0 + PC.MonsterKills * BonusDefs[CurrentBonuses[i]].Multiplier;
			Msg = BonusLabels[CurrentBonuses[i]] @ "(" @ PC.MonsterKills $ "X ) =>" @ CurrentMultiplier;
		}
		else
		{
			CurrentMultiplier = BonusDefs[CurrentBonuses[i]].Multiplier;
			Msg = BonusLabels[CurrentBonuses[i]] @ "=>" @ CurrentMultiplier;
		}

		TotalMultiplier *= CurrentMultiplier;
		PC.ClientDisplayScoreMsg( Msg );
	}

	GrandTotal = TotalMultiplier * RawScore;

	if( CurrentBonuses.Length > 0 )
	{
		// display the total
		Msg = TotalScoreMsg @ GrandTotal;
		PC.ClientDisplayScoreMsg( Msg );
		CurrentBonuses.Length = 0;
	}

	// give player credit
	PC.PlayerReplicationInfo.Score += GrandTotal;
}

//-----------------------------------------------------------------------------

function bool CheckTeam( bool bSameTeam, actor A, actor B )
{
	if( bSameTeam )
		return A.GetTeam() == B.GetTeam();
	else
		return A.GetTeam() != B.GetTeam();
}

//-----------------------------------------------------------------------------

function bool CloseToActor( actor AnchorActor, float MaxDistance, name ActorClassName )
{
	local Actor A;

	if( !AnchorActor )
		return false;

	foreach AnchorActor.AllActors( class'Actor', A )
		if( A.IsA( ActorClassName ) &&
			VSize( A.Location - AnchorActor.Location ) < MaxDistance )
		{
			return true;
		}

	return false;
}

//-----------------------------------------------------------------------------

function bool CloseToActorTeam( actor AnchorActor, float MaxDistance, name ActorClassName, bool bSameTeam )
{
	local Actor A;

	if( !AnchorActor )
		return false;

	foreach AnchorActor.AllActors( class'Actor', A )
		if( A.IsA( ActorClassName ) &&
			CheckTeam( bSameTeam, A, AnchorActor ) &&
			VSize( A.Location - AnchorActor.Location ) < MaxDistance )
		{
			return true;
		}

	return false;
}

//-----------------------------------------------------------------------------

function bool CloseToTeammateHacker( actor AnchorActor, float MaxDistance )
{
	local Actor A;

	if( !AnchorActor )
		return false;

	foreach AnchorActor.AllActors( class'Actor', A )
		if( IsHacking( Pawn(A) ) &&
			CheckTeam( true, A, AnchorActor ) &&
			VSize( A.Location - AnchorActor.Location ) < MaxDistance )
		{
			return true;
		}

	return false;
}

//-----------------------------------------------------------------------------

function bool CloseToTeammateArtifactCarrier( actor AnchorActor, float MaxDistance )
{
	local Actor A;

	if( !AnchorActor )
		return false;

	foreach AnchorActor.AllActors( class'Actor', A )
		if( HasArtifact( Pawn(A) ) &&
			CheckTeam( true, A, AnchorActor ) &&
			VSize( A.Location - AnchorActor.Location ) < MaxDistance )
		{
			return true;
		}

	return false;
}

//-----------------------------------------------------------------------------

function bool HasArtifact( Pawn P )
{
	return( P? && P.PlayerReplicationInfo? && P.PlayerReplicationInfo.bHasArtifact );
}

//-----------------------------------------------------------------------------

function bool HasArtifactC( Controller C )
{
	return( C? && C.PlayerReplicationInfo? && C.PlayerReplicationInfo.bHasArtifact );
}

//-----------------------------------------------------------------------------

function bool IsHacking( Pawn P )
{
	local HackTrigger HT;

	if( P? )
	{
		foreach P.AllActors( class'HackTrigger', HT )
			if( HT.IsHacking( P.Controller ) )
				return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

event PreBeginPlay()
{
	local int i;
	Super.PreBeginPlay();
	for( i=0; i < ScoreLabels.Length; i++ )
		MVPScoreNames[i] = DefaultMVPName;
	for( i=0; i < BonusLabels.Length; i++ )
		MVPBonusNames[i] = DefaultMVPName;
	MVPScoreStats.Length = ScoreLabels.Length;
	MVPBonusStats.Length = BonusLabels.Length;
}

//-----------------------------------------------------------------------------

function SendScores()
{
	local int iProfile, iEntry;
	local Controller C;

	for( C=Level.ControllerList; C!=None; C=C.nextController )
	{
		if( C? && C.PlayerReplicationInfo? )
		{
			for( iProfile=0; iProfile < ScoreProfiles.Length; iProfile++ )
			{
				if( ScoreProfiles[iProfile].PlayerID == C.PlayerReplicationInfo.PlayerID )
				{
					// default names
					for( iEntry=0; iEntry < ScoreLabels.Length; iEntry++ )
						C.PlayerReplicationInfo.MVPScoreName[iEntry] = DefaultMVPName;
					for( iEntry=0; iEntry < BonusLabels.Length; iEntry++ )
						C.PlayerReplicationInfo.MVPBonusName[iEntry] = DefaultMVPName;

					for( iEntry=0; iEntry < ScoreProfiles[iProfile].ScoreFreq.Length; iEntry++ )
					{
						C.PlayerReplicationInfo.ScoreStat[iEntry] = ScoreProfiles[iProfile].ScoreFreq[iEntry];
						C.PlayerReplicationInfo.MVPScoreName[iEntry] = MVPScoreNames[iEntry];
						C.PlayerReplicationInfo.MVPScoreStat[iEntry] = MVPScoreStats[iEntry];
					}

					for( iEntry=0; iEntry < ScoreProfiles[iProfile].BonusFreq.Length; iEntry++ )
					{
						C.PlayerReplicationInfo.BonusStat[iEntry] = ScoreProfiles[iProfile].BonusFreq[iEntry];
						C.PlayerReplicationInfo.MVPBonusName[iEntry] = MVPBonusNames[iEntry];
						C.PlayerReplicationInfo.MVPBonusStat[iEntry] = MVPBonusStats[iEntry];
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_None;
	TotalScoreMsg="Total score:"
	DefaultMVPName="Nobody"
	RecentlyOccupiedVehicleTime=3.0
	RecentlyOccupiedTurretTime=3.0
	DefendingIconNodeDistance=3000
	DefendingEnergySourceDistance=2000
	AttackingEnergySourceDistance=2000
	AttackingIconNodeDistance=3000
	MaxMonsterKillDuration=4.0
	DroppedArtifactDistance=1000
	DefendHackerDistance=1000
	DefendArtifactCarrierDistance=1000
	ScoreDefs(0)=(Type=ST_ArtifactCaptureCarrier,Score=400)
	ScoreDefs(1)=(Type=ST_ArtifactCaptureTeam,Score=200)
	ScoreDefs(2)=(Type=ST_ArtifactPickedUp,Score=100)
	ScoreDefs(3)=(Type=ST_ArtifactRecover,Score=50)
	ScoreDefs(4)=(Type=ST_ArtifactSteal,Score=75)
	ScoreDefs(5)=(Type=ST_FragDeployable,Score=75)
	ScoreDefs(6)=(Type=ST_FragDestroyable,Score=50)
	ScoreDefs(7)=(Type=ST_FragPlayer,Score=100)
	ScoreDefs(8)=(Type=ST_FragPlayerByDeployable,Score=100)
	ScoreDefs(9)=(Type=ST_FragTurretGiant,Score=600)
	ScoreDefs(10)=(Type=ST_FragTurretMedium,Score=400)
	ScoreDefs(11)=(Type=ST_FragTurretOther,Score=300)
	ScoreDefs(12)=(Type=ST_FragTurretSmall,Score=250)
	ScoreDefs(13)=(Type=ST_FragVehicleHarbinger,Score=600)
	ScoreDefs(14)=(Type=ST_FragVehicleJuggernaut,Score=800)
	ScoreDefs(15)=(Type=ST_FragVehicleOther,Score=500)
	ScoreDefs(16)=(Type=ST_FragVehicleRaptor,Score=400)
	ScoreDefs(17)=(Type=ST_HackAssist,Score=75)
	ScoreDefs(18)=(Type=ST_HackDeployPoint,Score=125)
	ScoreDefs(19)=(Type=ST_HackEnergySource,Score=125)
	ScoreDefs(20)=(Type=ST_HackOther,Score=75)
	ScoreDefs(21)=(Type=ST_HealTeammatePerSec,Score=0)
	ScoreDefs(22)=(Type=ST_HealTeammateWithPack,Score=30)
	ScoreDefs(23)=(Type=ST_RepairTeammatePerSec,Score=0)
	ScoreDefs(24)=(Type=ST_RepairTeammateWithPack,Score=15)
	ScoreDefs(25)=(Type=ST_ResupplyTeammatePerSec,Score=0)
	ScoreDefs(26)=(Type=ST_ResupplyTeammateWithPack,Score=15)
	ScoreDefs(27)=(Type=ST_Revive,Score=100)

	ScoreLabels(0)="Captured artifact"
	ScoreLabels(1)="Teammate captured artifact"
	ScoreLabels(2)="Picked up dropped artifact"
	ScoreLabels(3)="Recovered artifact"
	ScoreLabels(4)="Stole artifact"
	ScoreLabels(5)="Fragged deployable"
	ScoreLabels(6)="Fragged breakable object"
	ScoreLabels(7)="Fragged player"
	ScoreLabels(8)="Deployable fragged player"
	ScoreLabels(9)="Fragged giant turret"
	ScoreLabels(10)="Fragged medium turret"
	ScoreLabels(11)="Fragged other turret"
	ScoreLabels(12)="Fragged small turret"
	ScoreLabels(13)="Fragged Harbinger"
	ScoreLabels(14)="Fragged Juggernaut"
	ScoreLabels(15)="Fragged other vehicle"
	ScoreLabels(16)="Fragged Raptor"
	ScoreLabels(17)="Hack assist"
	ScoreLabels(18)="Hacked deploy point"
	ScoreLabels(19)="Hacked energy source"
	ScoreLabels(20)="Hacked other"
	ScoreLabels(21)="Heal time:"
	ScoreLabels(22)="Healed with pack"
	ScoreLabels(23)="Repair time"
	ScoreLabels(24)="Repaired with pack"
	ScoreLabels(25)="Resupply time"
	ScoreLabels(26)="Resupplied with pack"
	ScoreLabels(27)="Revived"

	BonusDefs(0)=(Type=BT_ArtifactCarry,Multiplier=1.5)
	BonusDefs(1)=(Type=BT_ArtifactRegisterDenial,Multiplier=2.0)
	BonusDefs(2)=(Type=BT_DefendArtifactCarrier,Multiplier=1.25)
	BonusDefs(3)=(Type=BT_DefendDroppedArtifact,Multiplier=1.5)
	BonusDefs(4)=(Type=BT_DefendEnergySource,Multiplier=1.25)
	BonusDefs(5)=(Type=BT_DefendHacker,Multiplier=1.25)
	BonusDefs(6)=(Type=BT_DefendIconNode,Multiplier=1.5)
	BonusDefs(7)=(Type=BT_FragArtifactCarrier,Multiplier=2.5)
	BonusDefs(8)=(Type=BT_FragEnemyDefender,Multiplier=1.5)
	BonusDefs(9)=(Type=BT_FragEnemyHacker,Multiplier=1.25)
	BonusDefs(10)=(Type=BT_FragHeadshot,Multiplier=1.5)
	BonusDefs(11)=(Type=BT_FragPerfectAccuracy,Multiplier=1.5)
	BonusDefs(12)=(Type=BT_FragPerfectAccuracyChained,Multiplier=2.0)
	BonusDefs(13)=(Type=BT_FragSelf,Multiplier=-0.5)
	BonusDefs(14)=(Type=BT_FragTeammate,Multiplier=-1.5)
	BonusDefs(15)=(Type=BT_HealArtifactCarrier,Multiplier=2.0)
	BonusDefs(16)=(Type=BT_HealTeammatePerSec,Multiplier=20.0)
	BonusDefs(17)=(Type=BT_MonsterKill,Multiplier=0.25)				// additive per MonsterKill 1.25, 1.5, 1.75, etc.
	BonusDefs(18)=(Type=BT_RepairArtifactCarrier,Multiplier=2.0)
	BonusDefs(19)=(Type=BT_RepairTeammatePerSec,Multiplier=10.0)
	BonusDefs(20)=(Type=BT_ResupplyArtifactCarrier,Multiplier=2.0)
	BonusDefs(21)=(Type=BT_ResupplyTeammatePerSec,Multiplier=10.0)
	BonusDefs(22)=(Type=BT_SupplyingEnemy,Multiplier=-0.5)
	BonusDefs(23)=(Type=BT_VehicularManslaughter,Multiplier=2.5)

	BonusLabels(0)="Carrying artifact"
	BonusLabels(1)="Denied artifact capture"
	BonusLabels(2)="Defended artifact carrier"
	BonusLabels(3)="Defended dropped artifact"
	BonusLabels(4)="Defended energy source"
	BonusLabels(5)="Defended hacker"
	BonusLabels(6)="Defended icon node"
	BonusLabels(7)="Fragged artifact carrier"
	BonusLabels(8)="Fragged enemy defender"
	BonusLabels(9)="Fragged enemy hacker"
	BonusLabels(10)="Headshot"
	BonusLabels(11)="Perfect accuracy"
	BonusLabels(12)="Perfect accuracy chain"
	BonusLabels(13)="Fragged Self"
	BonusLabels(14)="Fragged Teammate"
	BonusLabels(15)="Healed artifact carrier"
	BonusLabels(16)="Healed teammate (per sec)"
	BonusLabels(17)="Monster kill"
	BonusLabels(18)="Repaired artifact carrier"
	BonusLabels(19)="Repaired teammate (per sec)"
	BonusLabels(20)="Resupplied artifact carrier"
	BonusLabels(21)="Resupplied teammate (per sec)"
	BonusLabels(22)="Supplying the enemy"
	BonusLabels(23)="Vehicular manslaughter"
}