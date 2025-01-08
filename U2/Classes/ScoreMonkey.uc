//=============================================================================
// $Author$
// $Date$
// $Revision$
//=============================================================================

class ScoreMonkey extends UIHelper;

//-----------------------------------------------------------------------------

const NumScoreCategories = 28;
const NumBonusCategories = 24;

// score labels
var localized array<string> ScoreLabels;		// strings to display on hud for raw points
var localized array<string> BonusLabels;		// strings to display on hud for multipliers

// cached scores
var float	ScoreStat[NumScoreCategories];
var float	BonusStat[NumBonusCategories];
var string	MVPScoreName[NumScoreCategories];
var string	MVPBonusName[NumBonusCategories];
var float	MVPScoreStat[NumScoreCategories];
var float	MVPBonusStat[NumBonusCategories];

//-----------------------------------------------------------------------------

//
// End game stats
//

function interface UpdateScoreData()
{
	local int i;

	for( i=0; i < ArrayCount(GetPlayerOwner().PlayerReplicationInfo.ScoreStat); i++ )
	{
		ScoreStat[i]	= GetPlayerOwner().PlayerReplicationInfo.ScoreStat[i];
		MVPScoreName[i] = GetPlayerOwner().PlayerReplicationInfo.MVPScoreName[i];
		MVPScoreStat[i] = GetPlayerOwner().PlayerReplicationInfo.MVPScoreStat[i];
	}

	for( i=0; i < ArrayCount(GetPlayerOwner().PlayerReplicationInfo.BonusStat); i++ )
	{
		BonusStat[i]	= GetPlayerOwner().PlayerReplicationInfo.BonusStat[i];
		MVPBonusName[i] = GetPlayerOwner().PlayerReplicationInfo.MVPBonusName[i];
		MVPBonusStat[i] = GetPlayerOwner().PlayerReplicationInfo.MVPBonusStat[i];
	}
}

function interface string GetScoreCategory00()	{ return ScoreLabels[00]; }
function interface string GetScoreCategory01()	{ return ScoreLabels[01]; }
function interface string GetScoreCategory02()	{ return ScoreLabels[02]; }
function interface string GetScoreCategory03()	{ return ScoreLabels[03]; }
function interface string GetScoreCategory04()	{ return ScoreLabels[04]; }
function interface string GetScoreCategory05()	{ return ScoreLabels[05]; }
function interface string GetScoreCategory06()	{ return ScoreLabels[06]; }
function interface string GetScoreCategory07()	{ return ScoreLabels[07]; }
function interface string GetScoreCategory08()	{ return ScoreLabels[08]; }
function interface string GetScoreCategory09()	{ return ScoreLabels[09]; }
function interface string GetScoreCategory10()	{ return ScoreLabels[10]; }
function interface string GetScoreCategory11()	{ return ScoreLabels[11]; }
function interface string GetScoreCategory12()	{ return ScoreLabels[12]; }
function interface string GetScoreCategory13()	{ return ScoreLabels[13]; }
function interface string GetScoreCategory14()	{ return ScoreLabels[14]; }
function interface string GetScoreCategory15()	{ return ScoreLabels[15]; }
function interface string GetScoreCategory16()	{ return ScoreLabels[16]; }
function interface string GetScoreCategory17()	{ return ScoreLabels[17]; }
function interface string GetScoreCategory18()	{ return ScoreLabels[18]; }
function interface string GetScoreCategory19()	{ return ScoreLabels[19]; }
function interface string GetScoreCategory20()	{ return ScoreLabels[20]; }
function interface string GetScoreCategory21()	{ return ScoreLabels[21]; }
function interface string GetScoreCategory22()	{ return ScoreLabels[22]; }
function interface string GetScoreCategory23()	{ return ScoreLabels[23]; }
function interface string GetScoreCategory24()	{ return ScoreLabels[24]; }
function interface string GetScoreCategory25()	{ return ScoreLabels[25]; }
function interface string GetScoreCategory26()	{ return ScoreLabels[26]; }
function interface string GetScoreCategory27()	{ return ScoreLabels[27]; }

function interface string GetBonusCategory00()	{ return BonusLabels[00]; }
function interface string GetBonusCategory01()	{ return BonusLabels[01]; }
function interface string GetBonusCategory02()	{ return BonusLabels[02]; }
function interface string GetBonusCategory03()	{ return BonusLabels[03]; }
function interface string GetBonusCategory04()	{ return BonusLabels[04]; }
function interface string GetBonusCategory05()	{ return BonusLabels[05]; }
function interface string GetBonusCategory06()	{ return BonusLabels[06]; }
function interface string GetBonusCategory07()	{ return BonusLabels[07]; }
function interface string GetBonusCategory08()	{ return BonusLabels[08]; }
function interface string GetBonusCategory09()	{ return BonusLabels[09]; }
function interface string GetBonusCategory10()	{ return BonusLabels[10]; }
function interface string GetBonusCategory11()	{ return BonusLabels[11]; }
function interface string GetBonusCategory12()	{ return BonusLabels[12]; }
function interface string GetBonusCategory13()	{ return BonusLabels[13]; }
function interface string GetBonusCategory14()	{ return BonusLabels[14]; }
function interface string GetBonusCategory15()	{ return BonusLabels[15]; }
function interface string GetBonusCategory16()	{ return BonusLabels[16]; }
function interface string GetBonusCategory17()	{ return BonusLabels[17]; }
function interface string GetBonusCategory18()	{ return BonusLabels[18]; }
function interface string GetBonusCategory19()	{ return BonusLabels[19]; }
function interface string GetBonusCategory20()	{ return BonusLabels[20]; }
function interface string GetBonusCategory21()	{ return BonusLabels[21]; }
function interface string GetBonusCategory22()	{ return BonusLabels[22]; }

function interface float GetScorePersonal00()	{ return ScoreStat[0];  }
function interface float GetScorePersonal01()	{ return ScoreStat[1];  }
function interface float GetScorePersonal02()	{ return ScoreStat[2];  }
function interface float GetScorePersonal03()	{ return ScoreStat[3];  }
function interface float GetScorePersonal04()	{ return ScoreStat[4];  }
function interface float GetScorePersonal05()	{ return ScoreStat[5];  }
function interface float GetScorePersonal06()	{ return ScoreStat[6];  }
function interface float GetScorePersonal07()	{ return ScoreStat[7];  }
function interface float GetScorePersonal08()	{ return ScoreStat[8];  }
function interface float GetScorePersonal09()	{ return ScoreStat[9];  }
function interface float GetScorePersonal10()	{ return ScoreStat[10]; }
function interface float GetScorePersonal11()	{ return ScoreStat[11]; }
function interface float GetScorePersonal12()	{ return ScoreStat[12]; }
function interface float GetScorePersonal13()	{ return ScoreStat[13]; }
function interface float GetScorePersonal14()	{ return ScoreStat[14]; }
function interface float GetScorePersonal15()	{ return ScoreStat[15]; }
function interface float GetScorePersonal16()	{ return ScoreStat[16]; }
function interface float GetScorePersonal17()	{ return ScoreStat[17]; }
function interface float GetScorePersonal18()	{ return ScoreStat[18]; }
function interface float GetScorePersonal19()	{ return ScoreStat[19]; }
function interface float GetScorePersonal20()	{ return ScoreStat[20]; }
function interface float GetScorePersonal21()	{ return ScoreStat[21]; }
function interface float GetScorePersonal22()	{ return ScoreStat[22]; }
function interface float GetScorePersonal23()	{ return ScoreStat[23]; }
function interface float GetScorePersonal24()	{ return ScoreStat[24]; }
function interface float GetScorePersonal25()	{ return ScoreStat[25]; }
function interface float GetScorePersonal26()	{ return ScoreStat[26]; }
function interface float GetScorePersonal27()	{ return ScoreStat[27]; }

function interface float GetBonusPersonal00()	{ return BonusStat[0];  }
function interface float GetBonusPersonal01()	{ return BonusStat[1];  }
function interface float GetBonusPersonal02()	{ return BonusStat[2];  }
function interface float GetBonusPersonal03()	{ return BonusStat[3];  }
function interface float GetBonusPersonal04()	{ return BonusStat[4];  }
function interface float GetBonusPersonal05()	{ return BonusStat[5];  }
function interface float GetBonusPersonal06()	{ return BonusStat[6];  }
function interface float GetBonusPersonal07()	{ return BonusStat[7];  }
function interface float GetBonusPersonal08()	{ return BonusStat[8];  }
function interface float GetBonusPersonal09()	{ return BonusStat[9];  }
function interface float GetBonusPersonal10()	{ return BonusStat[10]; }
function interface float GetBonusPersonal11()	{ return BonusStat[11]; }
function interface float GetBonusPersonal12()	{ return BonusStat[12]; }
function interface float GetBonusPersonal13()	{ return BonusStat[13]; }
function interface float GetBonusPersonal14()	{ return BonusStat[14]; }
function interface float GetBonusPersonal15()	{ return BonusStat[15]; }
function interface float GetBonusPersonal16()	{ return BonusStat[16]; }
function interface float GetBonusPersonal17()	{ return BonusStat[17]; }
function interface float GetBonusPersonal18()	{ return BonusStat[18]; }
function interface float GetBonusPersonal19()	{ return BonusStat[19]; }
function interface float GetBonusPersonal20()	{ return BonusStat[20]; }
function interface float GetBonusPersonal21()	{ return BonusStat[21]; }
function interface float GetBonusPersonal22()	{ return BonusStat[22]; }

function interface string GetScoreMVPName00()	{ return MVPScoreName[0];  }
function interface string GetScoreMVPName01()	{ return MVPScoreName[1];  }
function interface string GetScoreMVPName02()	{ return MVPScoreName[2];  }
function interface string GetScoreMVPName03()	{ return MVPScoreName[3];  }
function interface string GetScoreMVPName04()	{ return MVPScoreName[4];  }
function interface string GetScoreMVPName05()	{ return MVPScoreName[5];  }
function interface string GetScoreMVPName06()	{ return MVPScoreName[6];  }
function interface string GetScoreMVPName07()	{ return MVPScoreName[7];  }
function interface string GetScoreMVPName08()	{ return MVPScoreName[8];  }
function interface string GetScoreMVPName09()	{ return MVPScoreName[9];  }
function interface string GetScoreMVPName10()	{ return MVPScoreName[10]; }
function interface string GetScoreMVPName11()	{ return MVPScoreName[11]; }
function interface string GetScoreMVPName12()	{ return MVPScoreName[12]; }
function interface string GetScoreMVPName13()	{ return MVPScoreName[13]; }
function interface string GetScoreMVPName14()	{ return MVPScoreName[14]; }
function interface string GetScoreMVPName15()	{ return MVPScoreName[15]; }
function interface string GetScoreMVPName16()	{ return MVPScoreName[16]; }
function interface string GetScoreMVPName17()	{ return MVPScoreName[17]; }
function interface string GetScoreMVPName18()	{ return MVPScoreName[18]; }
function interface string GetScoreMVPName19()	{ return MVPScoreName[19]; }
function interface string GetScoreMVPName20()	{ return MVPScoreName[20]; }
function interface string GetScoreMVPName21()	{ return MVPScoreName[21]; }
function interface string GetScoreMVPName22()	{ return MVPScoreName[22]; }
function interface string GetScoreMVPName23()	{ return MVPScoreName[23]; }
function interface string GetScoreMVPName24()	{ return MVPScoreName[24]; }
function interface string GetScoreMVPName25()	{ return MVPScoreName[25]; }
function interface string GetScoreMVPName26()	{ return MVPScoreName[26]; }
function interface string GetScoreMVPName27()	{ return MVPScoreName[27]; }

function interface string GetBonusMVPName00()	{ return MVPBonusName[0];  }
function interface string GetBonusMVPName01()	{ return MVPBonusName[1];  }
function interface string GetBonusMVPName02()	{ return MVPBonusName[2];  }
function interface string GetBonusMVPName03()	{ return MVPBonusName[3];  }
function interface string GetBonusMVPName04()	{ return MVPBonusName[4];  }
function interface string GetBonusMVPName05()	{ return MVPBonusName[5];  }
function interface string GetBonusMVPName06()	{ return MVPBonusName[6];  }
function interface string GetBonusMVPName07()	{ return MVPBonusName[7];  }
function interface string GetBonusMVPName08()	{ return MVPBonusName[8];  }
function interface string GetBonusMVPName09()	{ return MVPBonusName[9];  }
function interface string GetBonusMVPName10()	{ return MVPBonusName[10]; }
function interface string GetBonusMVPName11()	{ return MVPBonusName[11]; }
function interface string GetBonusMVPName12()	{ return MVPBonusName[12]; }
function interface string GetBonusMVPName13()	{ return MVPBonusName[13]; }
function interface string GetBonusMVPName14()	{ return MVPBonusName[14]; }
function interface string GetBonusMVPName15()	{ return MVPBonusName[15]; }
function interface string GetBonusMVPName16()	{ return MVPBonusName[16]; }
function interface string GetBonusMVPName17()	{ return MVPBonusName[17]; }
function interface string GetBonusMVPName18()	{ return MVPBonusName[18]; }
function interface string GetBonusMVPName19()	{ return MVPBonusName[19]; }
function interface string GetBonusMVPName20()	{ return MVPBonusName[20]; }
function interface string GetBonusMVPName21()	{ return MVPBonusName[21]; }
function interface string GetBonusMVPName22()	{ return MVPBonusName[22]; }

function interface float GetScoreMVPStat00()	{ return MVPScoreStat[0];  }
function interface float GetScoreMVPStat01()	{ return MVPScoreStat[1];  }
function interface float GetScoreMVPStat02()	{ return MVPScoreStat[2];  }
function interface float GetScoreMVPStat03()	{ return MVPScoreStat[3];  }
function interface float GetScoreMVPStat04()	{ return MVPScoreStat[4];  }
function interface float GetScoreMVPStat05()	{ return MVPScoreStat[5];  }
function interface float GetScoreMVPStat06()	{ return MVPScoreStat[6];  }
function interface float GetScoreMVPStat07()	{ return MVPScoreStat[7];  }
function interface float GetScoreMVPStat08()	{ return MVPScoreStat[8];  }
function interface float GetScoreMVPStat09()	{ return MVPScoreStat[9];  }
function interface float GetScoreMVPStat10()	{ return MVPScoreStat[10]; }
function interface float GetScoreMVPStat11()	{ return MVPScoreStat[11]; }
function interface float GetScoreMVPStat12()	{ return MVPScoreStat[12]; }
function interface float GetScoreMVPStat13()	{ return MVPScoreStat[13]; }
function interface float GetScoreMVPStat14()	{ return MVPScoreStat[14]; }
function interface float GetScoreMVPStat15()	{ return MVPScoreStat[15]; }
function interface float GetScoreMVPStat16()	{ return MVPScoreStat[16]; }
function interface float GetScoreMVPStat17()	{ return MVPScoreStat[17]; }
function interface float GetScoreMVPStat18()	{ return MVPScoreStat[18]; }
function interface float GetScoreMVPStat19()	{ return MVPScoreStat[19]; }
function interface float GetScoreMVPStat20()	{ return MVPScoreStat[20]; }
function interface float GetScoreMVPStat21()	{ return MVPScoreStat[21]; }
function interface float GetScoreMVPStat22()	{ return MVPScoreStat[22]; }
function interface float GetScoreMVPStat23()	{ return MVPScoreStat[23]; }
function interface float GetScoreMVPStat24()	{ return MVPScoreStat[24]; }
function interface float GetScoreMVPStat25()	{ return MVPScoreStat[25]; }
function interface float GetScoreMVPStat26()	{ return MVPScoreStat[26]; }
function interface float GetScoreMVPStat27()	{ return MVPScoreStat[27]; }

function interface float GetBonusMVPStat00()	{ return MVPBonusStat[0];  }
function interface float GetBonusMVPStat01()	{ return MVPBonusStat[1];  }
function interface float GetBonusMVPStat02()	{ return MVPBonusStat[2];  }
function interface float GetBonusMVPStat03()	{ return MVPBonusStat[3];  }
function interface float GetBonusMVPStat04()	{ return MVPBonusStat[4];  }
function interface float GetBonusMVPStat05()	{ return MVPBonusStat[5];  }
function interface float GetBonusMVPStat06()	{ return MVPBonusStat[6];  }
function interface float GetBonusMVPStat07()	{ return MVPBonusStat[7];  }
function interface float GetBonusMVPStat08()	{ return MVPBonusStat[8];  }
function interface float GetBonusMVPStat09()	{ return MVPBonusStat[9];  }
function interface float GetBonusMVPStat10()	{ return MVPBonusStat[10]; }
function interface float GetBonusMVPStat11()	{ return MVPBonusStat[11]; }
function interface float GetBonusMVPStat12()	{ return MVPBonusStat[12]; }
function interface float GetBonusMVPStat13()	{ return MVPBonusStat[13]; }
function interface float GetBonusMVPStat14()	{ return MVPBonusStat[14]; }
function interface float GetBonusMVPStat15()	{ return MVPBonusStat[15]; }
function interface float GetBonusMVPStat16()	{ return MVPBonusStat[16]; }
function interface float GetBonusMVPStat17()	{ return MVPBonusStat[17]; }
function interface float GetBonusMVPStat18()	{ return MVPBonusStat[18]; }
function interface float GetBonusMVPStat19()	{ return MVPBonusStat[19]; }
function interface float GetBonusMVPStat20()	{ return MVPBonusStat[20]; }
function interface float GetBonusMVPStat21()	{ return MVPBonusStat[21]; }
function interface float GetBonusMVPStat22()	{ return MVPBonusStat[22]; }

defaultproperties
{
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
	ScoreLabels(22)="Healed teammate with pack"
	ScoreLabels(23)="Repair time"
	ScoreLabels(24)="Repaired teammate with pack"
	ScoreLabels(25)="Resupply time"
	ScoreLabels(26)="Resupplied teammate with pack"
	ScoreLabels(27)="Revived"

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

