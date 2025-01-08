//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/02 1:48p $
// $Revision: 85 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	NetMonkey.uc
// Author:	Aaron R Leiby
// Date:	10 March 2003
//------------------------------------------------------------------------------
// Description:	Used to store various needed UnrealScript functions for the UI.
//------------------------------------------------------------------------------
// Notes:
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class NetMonkey extends UIHelper;

var InternetInfo Receiver;

var array<string> ServerNames;
var array<string> ServerIPs;
var string SelectedServer;

var config bool bListen;

var array<string> MapList;
var string SelectedMap;
var bool bShowAllMaps;

var MapList ML;

struct TeamEntry
{
	var string Name;
	var int IconIndex;
	var float Score;
//	var float Deaths;
	var float Ping;
	var bool bHasArtifact;
	var int ArtifactIndex;
};

var array<TeamEntry> TeamRed,TeamBlue;
var float NextUpdateTime;

var array<TeamEntry> TeamRed_GameOver,TeamBlue_GameOver;
var string GameOver_MapName;

function Constructed()
{
	CreateReceiver();
}

function CreateReceiver()
{
	if(!Receiver)
	{
		Receiver = GetPlayerOwner().GetEntryLevel().Spawn( class<InternetInfo>(DynamicLoadObject("IpDrv.ClientBeaconReceiver", class'Class')) );
		log(Self@"created"@Receiver);
	}
}

function interface array<string> GetServerList()
{
	local array<string> TempNames;
	local array<string> TempIPs;
	local string ServerName;
	local int i;
	TempNames[0]="No servers found";
	TempIPs[0]="127.0.0.1";
	for (i=0;i<32;i++)
	{
		ServerName = Receiver.GetBeaconText(i);
		if (ServerName=="")
			break;
		TempNames[i] = ServerName;
		TempIPs[i] = Receiver.GetBeaconAddress(i);
	}
	ServerNames=TempNames;
	ServerIPs=TempIPs;
	return ServerNames;
}

function interface SetServer( string NewServer )
{
	SelectedServer=NewServer;
}

function interface string GetServer()
{
	if ( Len(SelectedServer) < 1 ) SelectedServer = GetServerList()[0];
	return SelectedServer;
}

function interface SetTeam0(){ SetTeam(0); }
function interface SetTeam1(){ SetTeam(1); }

function interface SetTeam( float NewTeam ) { GetPlayerOwner().ChangeTeam( NewTeam ); GetPlayerOwner().UpdateURL("Team", string(int(NewTeam)), true); SaveConfig(); }
function interface float GetTeam() { return GetPlayerOwner().GetTeam(); }

function interface SetPlayerName( string NewName ) { GetPlayerOwner().SetName( NewName ); }
function interface string GetPlayerName()	{ return GetPlayerOwner().PlayerReplicationInfo.PlayerName; }

function interface bool IsOnTeam0() { return( GetPlayerOwner()? && GetPlayerOwner().GetTeam() == 0 ); } //mdf: slow accessor?
function interface bool IsOnTeam1() { return( GetPlayerOwner()? && GetPlayerOwner().GetTeam() == 1 ); } //mdf: slow accessor?

// Scoreboard stuff

function interface string GetCurrentMapName()	{ return GetPlayerOwner().Level.Title; }
function interface ResetScoreboard(){ NextUpdateTime=0; TeamRed.Length=0; TeamBlue.Length=0; }

function UpdateTeamData()
{
	local PlayerController Player;
	local GameReplicationInfo GRI;
	local PlayerReplicationInfo PRI;
	local int i,j,iTeam;

	Player = GetPlayerOwner();
	if( Player.Level.TimeSeconds > NextUpdateTime )
	{
		NextUpdateTime = Player.Level.TimeSeconds + 1.0;
		TeamRed.Length=0;
		TeamBlue.Length=0;
		GRI = Player.GameReplicationInfo;
		for( i=0; i<GRI.PRIArray.Length; i++ )
		{
			PRI = GRI.PRIArray[i];
			if( PRI.Team.TeamIndex == 0 )
			{
				iTeam = TeamRed.Length;
				TeamRed.Length = iTeam+1;
				TeamRed[iTeam].Name				= PRI.PlayerName;
				TeamRed[iTeam].IconIndex		= PRI.PawnClass.default.IconIndex;
				TeamRed[iTeam].Score			= PRI.Score;
//				TeamRed[iTeam].Deaths			= PRI.Deaths;
				TeamRed[iTeam].Ping				= PRI.Ping;
				TeamRed[iTeam].bHasArtifact		= PRI.bHasArtifact;
				if( PRI.HasArtifact? )
				TeamRed[iTeam].ArtifactIndex	= PRI.HasArtifact.IconIndex;
			}
			else if( PRI.Team.TeamIndex == 1 )
			{
				iTeam = TeamBlue.Length;
				TeamBlue.Length = iTeam+1;
				TeamBlue[iTeam].Name			= PRI.PlayerName;
				TeamBlue[iTeam].IconIndex		= PRI.PawnClass.default.IconIndex;
				TeamBlue[iTeam].Score			= PRI.Score;
//				TeamBlue[iTeam].Deaths			= PRI.Deaths;
				TeamBlue[iTeam].Ping			= PRI.Ping;
				TeamBlue[iTeam].bHasArtifact	= PRI.bHasArtifact;
				if( PRI.HasArtifact? )
				TeamBlue[iTeam].ArtifactIndex	= PRI.HasArtifact.IconIndex;
			}
		}

		// sort list by score.
		for( j=0; j<TeamRed.Length-1; j++ )
			for( i=j+1; i<TeamRed.Length; i++ )
				if( IsGreaterRed(i,j) )
					ExchangeTeamDataRed(i,j);
		for( j=0; j<TeamBlue.Length-1; j++ )
			for( i=j+1; i<TeamBlue.Length; i++ )
				if( IsGreaterBlue(i,j) )
					ExchangeTeamDataBlue(i,j);
	}
}
function bool IsGreaterRed(int i, int j)
{
	// favor higher score
	if( TeamRed[i].Score > TeamRed[j].Score )
		return true;
	if( TeamRed[i].Score < TeamRed[j].Score )
		return false;
/*	// favor lower deaths if tied
	if( TeamRed[i].Deaths < TeamRed[j].Deaths )
		return true;
	if( TeamRed[i].Deaths > TeamRed[j].Deaths )
		return false;
*/	// favor higher ping if still tied
	if( TeamRed[i].Ping > TeamRed[j].Ping )
		return true;
	if( TeamRed[i].Ping < TeamRed[j].Ping )
		return false;
	// leave alone if still tied
	return false;
}
function bool IsGreaterBlue(int i, int j)
{
	// favor higher score
	if( TeamBlue[i].Score > TeamBlue[j].Score )
		return true;
	if( TeamBlue[i].Score < TeamBlue[j].Score )
		return false;
/*	// favor lower deaths if tied
	if( TeamBlue[i].Deaths < TeamBlue[j].Deaths )
		return true;
	if( TeamBlue[i].Deaths > TeamBlue[j].Deaths )
		return false;
*/	// favor higher ping if still tied
	if( TeamBlue[i].Ping > TeamBlue[j].Ping )
		return true;
	if( TeamBlue[i].Ping < TeamBlue[j].Ping )
		return false;
	// leave alone if still tied
	return false;
}
function ExchangeTeamDataRed(int i, int j)
{
	local TeamEntry temp;
	temp = TeamRed[i];
	TeamRed[i] = TeamRed[j];
	TeamRed[j] = temp;
}
function ExchangeTeamDataBlue(int i, int j)
{
	local TeamEntry temp;
	temp = TeamBlue[i];
	TeamBlue[i] = TeamBlue[j];
	TeamBlue[j] = temp;
}

function interface bool IsEnabled_Red01()			{ UpdateTeamData(); return TeamRed.Length >= 1; }
function interface bool IsEnabled_Red02()			{ return TeamRed.Length >= 2; }
function interface bool IsEnabled_Red03()			{ return TeamRed.Length >= 3; }
function interface bool IsEnabled_Red04()			{ return TeamRed.Length >= 4; }
function interface bool IsEnabled_Red05()			{ return TeamRed.Length >= 5; }
function interface bool IsEnabled_Red06()			{ return TeamRed.Length >= 6; }
function interface bool IsEnabled_Red07()			{ return TeamRed.Length >= 7; }
function interface bool IsEnabled_Red08()			{ return TeamRed.Length >= 8; }
function interface bool IsEnabled_Red09()			{ return TeamRed.Length >= 9; }
function interface bool IsEnabled_Red10()			{ return TeamRed.Length >= 10; }
function interface bool IsEnabled_Red11()			{ return TeamRed.Length >= 11; }
function interface bool IsEnabled_Red12()			{ return TeamRed.Length >= 12; }

function interface bool IsEnabled_Blue01()			{ return TeamBlue.Length >= 1; }
function interface bool IsEnabled_Blue02()			{ return TeamBlue.Length >= 2; }
function interface bool IsEnabled_Blue03()			{ return TeamBlue.Length >= 3; }
function interface bool IsEnabled_Blue04()			{ return TeamBlue.Length >= 4; }
function interface bool IsEnabled_Blue05()			{ return TeamBlue.Length >= 5; }
function interface bool IsEnabled_Blue06()			{ return TeamBlue.Length >= 6; }
function interface bool IsEnabled_Blue07()			{ return TeamBlue.Length >= 7; }
function interface bool IsEnabled_Blue08()			{ return TeamBlue.Length >= 8; }
function interface bool IsEnabled_Blue09()			{ return TeamBlue.Length >= 9; }
function interface bool IsEnabled_Blue10()			{ return TeamBlue.Length >= 10; }
function interface bool IsEnabled_Blue11()			{ return TeamBlue.Length >= 11; }
function interface bool IsEnabled_Blue12()			{ return TeamBlue.Length >= 12; }

function interface string GetPlayerName_Red01()		{ return TeamRed[0].Name; }
function interface string GetPlayerName_Red02()		{ return TeamRed[1].Name; }
function interface string GetPlayerName_Red03()		{ return TeamRed[2].Name; }
function interface string GetPlayerName_Red04()		{ return TeamRed[3].Name; }
function interface string GetPlayerName_Red05()		{ return TeamRed[4].Name; }
function interface string GetPlayerName_Red06()		{ return TeamRed[5].Name; }
function interface string GetPlayerName_Red07()		{ return TeamRed[6].Name; }
function interface string GetPlayerName_Red08()		{ return TeamRed[7].Name; }
function interface string GetPlayerName_Red09()		{ return TeamRed[8].Name; }
function interface string GetPlayerName_Red10()		{ return TeamRed[9].Name; }
function interface string GetPlayerName_Red11()		{ return TeamRed[10].Name; }
function interface string GetPlayerName_Red12()		{ return TeamRed[11].Name; }

function interface string GetPlayerName_Blue01()	{ return TeamBlue[0].Name; }
function interface string GetPlayerName_Blue02()	{ return TeamBlue[1].Name; }
function interface string GetPlayerName_Blue03()	{ return TeamBlue[2].Name; }
function interface string GetPlayerName_Blue04()	{ return TeamBlue[3].Name; }
function interface string GetPlayerName_Blue05()	{ return TeamBlue[4].Name; }
function interface string GetPlayerName_Blue06()	{ return TeamBlue[5].Name; }
function interface string GetPlayerName_Blue07()	{ return TeamBlue[6].Name; }
function interface string GetPlayerName_Blue08()	{ return TeamBlue[7].Name; }
function interface string GetPlayerName_Blue09()	{ return TeamBlue[8].Name; }
function interface string GetPlayerName_Blue10()	{ return TeamBlue[9].Name; }
function interface string GetPlayerName_Blue11()	{ return TeamBlue[10].Name; }
function interface string GetPlayerName_Blue12()	{ return TeamBlue[11].Name; }

function interface int GetPlayerIconIndex_Red01()	{ return TeamRed[0].IconIndex; }
function interface int GetPlayerIconIndex_Red02()	{ return TeamRed[1].IconIndex; }
function interface int GetPlayerIconIndex_Red03()	{ return TeamRed[2].IconIndex; }
function interface int GetPlayerIconIndex_Red04()	{ return TeamRed[3].IconIndex; }
function interface int GetPlayerIconIndex_Red05()	{ return TeamRed[4].IconIndex; }
function interface int GetPlayerIconIndex_Red06()	{ return TeamRed[5].IconIndex; }
function interface int GetPlayerIconIndex_Red07()	{ return TeamRed[6].IconIndex; }
function interface int GetPlayerIconIndex_Red08()	{ return TeamRed[7].IconIndex; }
function interface int GetPlayerIconIndex_Red09()	{ return TeamRed[8].IconIndex; }
function interface int GetPlayerIconIndex_Red10()	{ return TeamRed[9].IconIndex; }
function interface int GetPlayerIconIndex_Red11()	{ return TeamRed[10].IconIndex; }
function interface int GetPlayerIconIndex_Red12()	{ return TeamRed[11].IconIndex; }

function interface int GetPlayerIconIndex_Blue01()	{ return TeamBlue[0].IconIndex; }
function interface int GetPlayerIconIndex_Blue02()	{ return TeamBlue[1].IconIndex; }
function interface int GetPlayerIconIndex_Blue03()	{ return TeamBlue[2].IconIndex; }
function interface int GetPlayerIconIndex_Blue04()	{ return TeamBlue[3].IconIndex; }
function interface int GetPlayerIconIndex_Blue05()	{ return TeamBlue[4].IconIndex; }
function interface int GetPlayerIconIndex_Blue06()	{ return TeamBlue[5].IconIndex; }
function interface int GetPlayerIconIndex_Blue07()	{ return TeamBlue[6].IconIndex; }
function interface int GetPlayerIconIndex_Blue08()	{ return TeamBlue[7].IconIndex; }
function interface int GetPlayerIconIndex_Blue09()	{ return TeamBlue[8].IconIndex; }
function interface int GetPlayerIconIndex_Blue10()	{ return TeamBlue[9].IconIndex; }
function interface int GetPlayerIconIndex_Blue11()	{ return TeamBlue[10].IconIndex; }
function interface int GetPlayerIconIndex_Blue12()	{ return TeamBlue[11].IconIndex; }

function interface float GetPlayerScore_Red01()		{ return TeamRed[0].Score; }
function interface float GetPlayerScore_Red02()		{ return TeamRed[1].Score; }
function interface float GetPlayerScore_Red03()		{ return TeamRed[2].Score; }
function interface float GetPlayerScore_Red04()		{ return TeamRed[3].Score; }
function interface float GetPlayerScore_Red05()		{ return TeamRed[4].Score; }
function interface float GetPlayerScore_Red06()		{ return TeamRed[5].Score; }
function interface float GetPlayerScore_Red07()		{ return TeamRed[6].Score; }
function interface float GetPlayerScore_Red08()		{ return TeamRed[7].Score; }
function interface float GetPlayerScore_Red09()		{ return TeamRed[8].Score; }
function interface float GetPlayerScore_Red10()		{ return TeamRed[9].Score; }
function interface float GetPlayerScore_Red11()		{ return TeamRed[10].Score; }
function interface float GetPlayerScore_Red12()		{ return TeamRed[11].Score; }

function interface float GetPlayerScore_Blue01()	{ return TeamBlue[0].Score; }
function interface float GetPlayerScore_Blue02()	{ return TeamBlue[1].Score; }
function interface float GetPlayerScore_Blue03()	{ return TeamBlue[2].Score; }
function interface float GetPlayerScore_Blue04()	{ return TeamBlue[3].Score; }
function interface float GetPlayerScore_Blue05()	{ return TeamBlue[4].Score; }
function interface float GetPlayerScore_Blue06()	{ return TeamBlue[5].Score; }
function interface float GetPlayerScore_Blue07()	{ return TeamBlue[6].Score; }
function interface float GetPlayerScore_Blue08()	{ return TeamBlue[7].Score; }
function interface float GetPlayerScore_Blue09()	{ return TeamBlue[8].Score; }
function interface float GetPlayerScore_Blue10()	{ return TeamBlue[9].Score; }
function interface float GetPlayerScore_Blue11()	{ return TeamBlue[10].Score; }
function interface float GetPlayerScore_Blue12()	{ return TeamBlue[11].Score; }
/*
function interface float GetPlayerDeaths_Red01()	{ return TeamRed[0].Deaths; }
function interface float GetPlayerDeaths_Red02()	{ return TeamRed[1].Deaths; }
function interface float GetPlayerDeaths_Red03()	{ return TeamRed[2].Deaths; }
function interface float GetPlayerDeaths_Red04()	{ return TeamRed[3].Deaths; }
function interface float GetPlayerDeaths_Red05()	{ return TeamRed[4].Deaths; }
function interface float GetPlayerDeaths_Red06()	{ return TeamRed[5].Deaths; }
function interface float GetPlayerDeaths_Red07()	{ return TeamRed[6].Deaths; }
function interface float GetPlayerDeaths_Red08()	{ return TeamRed[7].Deaths; }
function interface float GetPlayerDeaths_Red09()	{ return TeamRed[8].Deaths; }
function interface float GetPlayerDeaths_Red10()	{ return TeamRed[9].Deaths; }
function interface float GetPlayerDeaths_Red11()	{ return TeamRed[10].Deaths; }
function interface float GetPlayerDeaths_Red12()	{ return TeamRed[11].Deaths; }

function interface float GetPlayerDeaths_Blue01()	{ return TeamBlue[0].Deaths; }
function interface float GetPlayerDeaths_Blue02()	{ return TeamBlue[1].Deaths; }
function interface float GetPlayerDeaths_Blue03()	{ return TeamBlue[2].Deaths; }
function interface float GetPlayerDeaths_Blue04()	{ return TeamBlue[3].Deaths; }
function interface float GetPlayerDeaths_Blue05()	{ return TeamBlue[4].Deaths; }
function interface float GetPlayerDeaths_Blue06()	{ return TeamBlue[5].Deaths; }
function interface float GetPlayerDeaths_Blue07()	{ return TeamBlue[6].Deaths; }
function interface float GetPlayerDeaths_Blue08()	{ return TeamBlue[7].Deaths; }
function interface float GetPlayerDeaths_Blue09()	{ return TeamBlue[8].Deaths; }
function interface float GetPlayerDeaths_Blue10()	{ return TeamBlue[9].Deaths; }
function interface float GetPlayerDeaths_Blue11()	{ return TeamBlue[10].Deaths; }
function interface float GetPlayerDeaths_Blue12()	{ return TeamBlue[11].Deaths; }
*/
function interface float GetPlayerPing_Red01()		{ return TeamRed[0].Ping; }
function interface float GetPlayerPing_Red02()		{ return TeamRed[1].Ping; }
function interface float GetPlayerPing_Red03()		{ return TeamRed[2].Ping; }
function interface float GetPlayerPing_Red04()		{ return TeamRed[3].Ping; }
function interface float GetPlayerPing_Red05()		{ return TeamRed[4].Ping; }
function interface float GetPlayerPing_Red06()		{ return TeamRed[5].Ping; }
function interface float GetPlayerPing_Red07()		{ return TeamRed[6].Ping; }
function interface float GetPlayerPing_Red08()		{ return TeamRed[7].Ping; }
function interface float GetPlayerPing_Red09()		{ return TeamRed[8].Ping; }
function interface float GetPlayerPing_Red10()		{ return TeamRed[9].Ping; }
function interface float GetPlayerPing_Red11()		{ return TeamRed[10].Ping; }
function interface float GetPlayerPing_Red12()		{ return TeamRed[11].Ping; }

function interface float GetPlayerPing_Blue01()		{ return TeamBlue[0].Ping; }
function interface float GetPlayerPing_Blue02()		{ return TeamBlue[1].Ping; }
function interface float GetPlayerPing_Blue03()		{ return TeamBlue[2].Ping; }
function interface float GetPlayerPing_Blue04()		{ return TeamBlue[3].Ping; }
function interface float GetPlayerPing_Blue05()		{ return TeamBlue[4].Ping; }
function interface float GetPlayerPing_Blue06()		{ return TeamBlue[5].Ping; }
function interface float GetPlayerPing_Blue07()		{ return TeamBlue[6].Ping; }
function interface float GetPlayerPing_Blue08()		{ return TeamBlue[7].Ping; }
function interface float GetPlayerPing_Blue09()		{ return TeamBlue[8].Ping; }
function interface float GetPlayerPing_Blue10()		{ return TeamBlue[9].Ping; }
function interface float GetPlayerPing_Blue11()		{ return TeamBlue[10].Ping; }
function interface float GetPlayerPing_Blue12()		{ return TeamBlue[11].Ping; }

function interface bool GetArtifactEnabled_Red01()	{ return TeamRed.Length>0 && TeamRed[0].bHasArtifact; }
function interface bool GetArtifactEnabled_Red02()	{ return TeamRed.Length>1 && TeamRed[1].bHasArtifact; }
function interface bool GetArtifactEnabled_Red03()	{ return TeamRed.Length>2 && TeamRed[2].bHasArtifact; }
function interface bool GetArtifactEnabled_Red04()	{ return TeamRed.Length>3 && TeamRed[3].bHasArtifact; }
function interface bool GetArtifactEnabled_Red05()	{ return TeamRed.Length>4 && TeamRed[4].bHasArtifact; }
function interface bool GetArtifactEnabled_Red06()	{ return TeamRed.Length>5 && TeamRed[5].bHasArtifact; }
function interface bool GetArtifactEnabled_Red07()	{ return TeamRed.Length>6 && TeamRed[6].bHasArtifact; }
function interface bool GetArtifactEnabled_Red08()	{ return TeamRed.Length>7 && TeamRed[7].bHasArtifact; }
function interface bool GetArtifactEnabled_Red09()	{ return TeamRed.Length>8 && TeamRed[8].bHasArtifact; }
function interface bool GetArtifactEnabled_Red10()	{ return TeamRed.Length>9 && TeamRed[9].bHasArtifact; }
function interface bool GetArtifactEnabled_Red11()	{ return TeamRed.Length>10 && TeamRed[10].bHasArtifact; }
function interface bool GetArtifactEnabled_Red12()	{ return TeamRed.Length>11 && TeamRed[11].bHasArtifact; }

function interface bool GetArtifactEnabled_Blue01()	{ return TeamBlue.Length>0 && TeamBlue[0].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue02()	{ return TeamBlue.Length>1 && TeamBlue[1].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue03()	{ return TeamBlue.Length>2 && TeamBlue[2].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue04()	{ return TeamBlue.Length>3 && TeamBlue[3].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue05()	{ return TeamBlue.Length>4 && TeamBlue[4].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue06()	{ return TeamBlue.Length>5 && TeamBlue[5].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue07()	{ return TeamBlue.Length>6 && TeamBlue[6].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue08()	{ return TeamBlue.Length>7 && TeamBlue[7].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue09()	{ return TeamBlue.Length>8 && TeamBlue[8].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue10()	{ return TeamBlue.Length>9 && TeamBlue[9].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue11()	{ return TeamBlue.Length>10 && TeamBlue[10].bHasArtifact; }
function interface bool GetArtifactEnabled_Blue12()	{ return TeamBlue.Length>11 && TeamBlue[11].bHasArtifact; }

function interface int GetArtifactIndex_Red01()		{ return TeamRed[0].ArtifactIndex; }
function interface int GetArtifactIndex_Red02()		{ return TeamRed[1].ArtifactIndex; }
function interface int GetArtifactIndex_Red03()		{ return TeamRed[2].ArtifactIndex; }
function interface int GetArtifactIndex_Red04()		{ return TeamRed[3].ArtifactIndex; }
function interface int GetArtifactIndex_Red05()		{ return TeamRed[4].ArtifactIndex; }
function interface int GetArtifactIndex_Red06()		{ return TeamRed[5].ArtifactIndex; }
function interface int GetArtifactIndex_Red07()		{ return TeamRed[6].ArtifactIndex; }
function interface int GetArtifactIndex_Red08()		{ return TeamRed[7].ArtifactIndex; }
function interface int GetArtifactIndex_Red09()		{ return TeamRed[8].ArtifactIndex; }
function interface int GetArtifactIndex_Red10()		{ return TeamRed[9].ArtifactIndex; }
function interface int GetArtifactIndex_Red11()		{ return TeamRed[10].ArtifactIndex; }
function interface int GetArtifactIndex_Red12()		{ return TeamRed[11].ArtifactIndex; }

function interface int GetArtifactIndex_Blue01()	{ return TeamBlue[0].ArtifactIndex; }
function interface int GetArtifactIndex_Blue02()	{ return TeamBlue[1].ArtifactIndex; }
function interface int GetArtifactIndex_Blue03()	{ return TeamBlue[2].ArtifactIndex; }
function interface int GetArtifactIndex_Blue04()	{ return TeamBlue[3].ArtifactIndex; }
function interface int GetArtifactIndex_Blue05()	{ return TeamBlue[4].ArtifactIndex; }
function interface int GetArtifactIndex_Blue06()	{ return TeamBlue[5].ArtifactIndex; }
function interface int GetArtifactIndex_Blue07()	{ return TeamBlue[6].ArtifactIndex; }
function interface int GetArtifactIndex_Blue08()	{ return TeamBlue[7].ArtifactIndex; }
function interface int GetArtifactIndex_Blue09()	{ return TeamBlue[8].ArtifactIndex; }
function interface int GetArtifactIndex_Blue10()	{ return TeamBlue[9].ArtifactIndex; }
function interface int GetArtifactIndex_Blue11()	{ return TeamBlue[10].ArtifactIndex; }
function interface int GetArtifactIndex_Blue12()	{ return TeamBlue[11].ArtifactIndex; }

// GameOver Scoreboard stuff -- start

function interface string GetCurrentMapName_GameOver()	{ return GameOver_MapName; }
function interface SaveScoreboard()
{
	UpdateTeamData();
	GameOver_MapName = GetPlayerOwner().Level.Title;
	TeamRed_GameOver = TeamRed;
	TeamBlue_GameOver = TeamBlue;
}

function interface bool IsEnabled_Red01_GameOver()			{ return TeamRed_GameOver.Length >= 1; }
function interface bool IsEnabled_Red02_GameOver()			{ return TeamRed_GameOver.Length >= 2; }
function interface bool IsEnabled_Red03_GameOver()			{ return TeamRed_GameOver.Length >= 3; }
function interface bool IsEnabled_Red04_GameOver()			{ return TeamRed_GameOver.Length >= 4; }
function interface bool IsEnabled_Red05_GameOver()			{ return TeamRed_GameOver.Length >= 5; }
function interface bool IsEnabled_Red06_GameOver()			{ return TeamRed_GameOver.Length >= 6; }
function interface bool IsEnabled_Red07_GameOver()			{ return TeamRed_GameOver.Length >= 7; }
function interface bool IsEnabled_Red08_GameOver()			{ return TeamRed_GameOver.Length >= 8; }
function interface bool IsEnabled_Red09_GameOver()			{ return TeamRed_GameOver.Length >= 9; }
function interface bool IsEnabled_Red10_GameOver()			{ return TeamRed_GameOver.Length >= 10; }
function interface bool IsEnabled_Red11_GameOver()			{ return TeamRed_GameOver.Length >= 11; }
function interface bool IsEnabled_Red12_GameOver()			{ return TeamRed_GameOver.Length >= 12; }

function interface bool IsEnabled_Blue01_GameOver()			{ return TeamBlue_GameOver.Length >= 1; }
function interface bool IsEnabled_Blue02_GameOver()			{ return TeamBlue_GameOver.Length >= 2; }
function interface bool IsEnabled_Blue03_GameOver()			{ return TeamBlue_GameOver.Length >= 3; }
function interface bool IsEnabled_Blue04_GameOver()			{ return TeamBlue_GameOver.Length >= 4; }
function interface bool IsEnabled_Blue05_GameOver()			{ return TeamBlue_GameOver.Length >= 5; }
function interface bool IsEnabled_Blue06_GameOver()			{ return TeamBlue_GameOver.Length >= 6; }
function interface bool IsEnabled_Blue07_GameOver()			{ return TeamBlue_GameOver.Length >= 7; }
function interface bool IsEnabled_Blue08_GameOver()			{ return TeamBlue_GameOver.Length >= 8; }
function interface bool IsEnabled_Blue09_GameOver()			{ return TeamBlue_GameOver.Length >= 9; }
function interface bool IsEnabled_Blue10_GameOver()			{ return TeamBlue_GameOver.Length >= 10; }
function interface bool IsEnabled_Blue11_GameOver()			{ return TeamBlue_GameOver.Length >= 11; }
function interface bool IsEnabled_Blue12_GameOver()			{ return TeamBlue_GameOver.Length >= 12; }

function interface string GetPlayerName_Red01_GameOver()	{ return TeamRed_GameOver[0].Name; }
function interface string GetPlayerName_Red02_GameOver()	{ return TeamRed_GameOver[1].Name; }
function interface string GetPlayerName_Red03_GameOver()	{ return TeamRed_GameOver[2].Name; }
function interface string GetPlayerName_Red04_GameOver()	{ return TeamRed_GameOver[3].Name; }
function interface string GetPlayerName_Red05_GameOver()	{ return TeamRed_GameOver[4].Name; }
function interface string GetPlayerName_Red06_GameOver()	{ return TeamRed_GameOver[5].Name; }
function interface string GetPlayerName_Red07_GameOver()	{ return TeamRed_GameOver[6].Name; }
function interface string GetPlayerName_Red08_GameOver()	{ return TeamRed_GameOver[7].Name; }
function interface string GetPlayerName_Red09_GameOver()	{ return TeamRed_GameOver[8].Name; }
function interface string GetPlayerName_Red10_GameOver()	{ return TeamRed_GameOver[9].Name; }
function interface string GetPlayerName_Red11_GameOver()	{ return TeamRed_GameOver[10].Name; }
function interface string GetPlayerName_Red12_GameOver()	{ return TeamRed_GameOver[11].Name; }

function interface string GetPlayerName_Blue01_GameOver()	{ return TeamBlue_GameOver[0].Name; }
function interface string GetPlayerName_Blue02_GameOver()	{ return TeamBlue_GameOver[1].Name; }
function interface string GetPlayerName_Blue03_GameOver()	{ return TeamBlue_GameOver[2].Name; }
function interface string GetPlayerName_Blue04_GameOver()	{ return TeamBlue_GameOver[3].Name; }
function interface string GetPlayerName_Blue05_GameOver()	{ return TeamBlue_GameOver[4].Name; }
function interface string GetPlayerName_Blue06_GameOver()	{ return TeamBlue_GameOver[5].Name; }
function interface string GetPlayerName_Blue07_GameOver()	{ return TeamBlue_GameOver[6].Name; }
function interface string GetPlayerName_Blue08_GameOver()	{ return TeamBlue_GameOver[7].Name; }
function interface string GetPlayerName_Blue09_GameOver()	{ return TeamBlue_GameOver[8].Name; }
function interface string GetPlayerName_Blue10_GameOver()	{ return TeamBlue_GameOver[9].Name; }
function interface string GetPlayerName_Blue11_GameOver()	{ return TeamBlue_GameOver[10].Name; }
function interface string GetPlayerName_Blue12_GameOver()	{ return TeamBlue_GameOver[11].Name; }

function interface float GetPlayerScore_Red01_GameOver()	{ return TeamRed_GameOver[0].Score; }
function interface float GetPlayerScore_Red02_GameOver()	{ return TeamRed_GameOver[1].Score; }
function interface float GetPlayerScore_Red03_GameOver()	{ return TeamRed_GameOver[2].Score; }
function interface float GetPlayerScore_Red04_GameOver()	{ return TeamRed_GameOver[3].Score; }
function interface float GetPlayerScore_Red05_GameOver()	{ return TeamRed_GameOver[4].Score; }
function interface float GetPlayerScore_Red06_GameOver()	{ return TeamRed_GameOver[5].Score; }
function interface float GetPlayerScore_Red07_GameOver()	{ return TeamRed_GameOver[6].Score; }
function interface float GetPlayerScore_Red08_GameOver()	{ return TeamRed_GameOver[7].Score; }
function interface float GetPlayerScore_Red09_GameOver()	{ return TeamRed_GameOver[8].Score; }
function interface float GetPlayerScore_Red10_GameOver()	{ return TeamRed_GameOver[9].Score; }
function interface float GetPlayerScore_Red11_GameOver()	{ return TeamRed_GameOver[10].Score; }
function interface float GetPlayerScore_Red12_GameOver()	{ return TeamRed_GameOver[11].Score; }

function interface float GetPlayerScore_Blue01_GameOver()	{ return TeamBlue_GameOver[0].Score; }
function interface float GetPlayerScore_Blue02_GameOver()	{ return TeamBlue_GameOver[1].Score; }
function interface float GetPlayerScore_Blue03_GameOver()	{ return TeamBlue_GameOver[2].Score; }
function interface float GetPlayerScore_Blue04_GameOver()	{ return TeamBlue_GameOver[3].Score; }
function interface float GetPlayerScore_Blue05_GameOver()	{ return TeamBlue_GameOver[4].Score; }
function interface float GetPlayerScore_Blue06_GameOver()	{ return TeamBlue_GameOver[5].Score; }
function interface float GetPlayerScore_Blue07_GameOver()	{ return TeamBlue_GameOver[6].Score; }
function interface float GetPlayerScore_Blue08_GameOver()	{ return TeamBlue_GameOver[7].Score; }
function interface float GetPlayerScore_Blue09_GameOver()	{ return TeamBlue_GameOver[8].Score; }
function interface float GetPlayerScore_Blue10_GameOver()	{ return TeamBlue_GameOver[9].Score; }
function interface float GetPlayerScore_Blue11_GameOver()	{ return TeamBlue_GameOver[10].Score; }
function interface float GetPlayerScore_Blue12_GameOver()	{ return TeamBlue_GameOver[11].Score; }

// GameOver Scoreboard stuff -- end

function interface string GetServerIP()
{
	local int i;
	local string ServerName;
	ServerName = GetServer();
	for (i=0;i<ServerNames.length;i++)
		if (ServerNames[i]==ServerName)
			return ServerIPs[i];
	return "127.0.0.1";
}

function interface JoinGame()
{
	GetPlayerOwner().ConsoleCommand("OPEN"@GetServerIP()$"?Team="$GetTeam()$"?Player="$GetPlayerName());
}

function interface SetListen(bool B)		{ bListen=B; SaveConfig(); }
function interface bool GetListen()			{ return bListen; }

function interface SetShowAllMaps(bool B)	{ bShowAllMaps=B; }
function interface bool GetShowAllMaps()	{ return bShowAllMaps; }

function interface array<string> GetMapList()
{
	local string FirstMap, NextMap, TestMap;
	local array<string> TempMaps;
	local String MapPrefix;
	local int i;

    if ( bShowAllMaps )
    	MapPrefix = "";
    else
    	MapPrefix = GetGameClass().default.MapPrefix;

	FirstMap = GetPlayerOwner().GetMapName(MapPrefix, "", 0);

	NextMap = FirstMap;
	i=0;

	while (!(FirstMap ~= TestMap))
	{
		// Add the map.
		TempMaps[i] = Left(NextMap, Len(NextMap) - 4);

		// Get the map.
		NextMap = GetPlayerOwner().GetMapName(MapPrefix, NextMap, 1);

		// Test to see if this is the last.
		TestMap = NextMap;
		i++;
	}
	MapList = TempMaps;
	return MapList;
}

function interface SetMap( string NewMap )
{
	SelectedMap=NewMap;
}

function interface string GetMap()
{
	if ( Len(SelectedMap) < 1 ) SelectedMap = GetMapList()[0];
	return SelectedMap;
}

function interface array<string> GetServerMapList()
{
	local class<MapList> MLClass;
	if (!ML)
	{
		MLClass = class<MapList>(DynamicLoadObject(GetMapListClass(), class'class'));
		if (MLClass?)
			ML = GetPlayerOwner().GetEntryLevel().Spawn(MLClass);
	}
	return ML.Maps;
}

function interface AddMap()
{
	ML.MapNum = Clamp( ML.MapNum, 0, Max(ML.Maps.Length-1,0) );
	if(ML.Maps.Length > 0)
		ML.MapNum++;
	ML.Maps.Insert(ML.MapNum,1);
	ML.Maps[ML.MapNum]=GetMap();
	ML.SaveConfig();
}

function interface RemoveMap()
{
	ML.MapNum = Clamp( ML.MapNum, 0, Max(ML.Maps.Length-1,0) );
	ML.Maps.Remove(ML.MapNum,1);
	ML.MapNum = Clamp( ML.MapNum, 0, Max(ML.Maps.Length-1,0) );
	ML.SaveConfig();
}

function interface SetServerMapIndex(int i)	{ ML.MapNum=i; ML.SaveConfig(); }
function interface int GetServerMapIndex()	{ return ML.MapNum; }

function string GetMapListClass()
{
	return GetGameClass().default.MapListType;
}

function class<GameInfo> GetGameClass()
{
	return class<GameInfo>(DynamicLoadObject(GetGameType(), class'class'));
}

function string GetGameType()
{
	return "U2.XMPGame";
}

function interface SetTimeLimit(float F)	{ GetGameClass().default.TimeLimit=F; GetGameClass().static.StaticSaveConfig(); }
function interface float GetTimeLimit()		{ return GetGameClass().default.TimeLimit; }

function interface SetTeamDamage(float F)	{ GetGameClass().default.TeamDamage=F; GetGameClass().static.StaticSaveConfig(); }
function interface float GetTeamDamage()	{ return GetGameClass().default.TeamDamage; }

function interface SetPreMatchWait(float F)	{ GetGameClass().default.PreMatchWait=F; GetGameClass().static.StaticSaveConfig(); }
function interface float GetPreMatchWait()	{ return GetGameClass().default.PreMatchWait; }

function interface StartServer()
{
	local string FullURL;

	FullURL = ML.Maps[ML.MapNum];	//!! additional options

    if( bListen )
        GetPlayerOwner().ClientTravel( FullURL $"?Listen", TRAVEL_Absolute, false );
    else
        GetPlayerOwner().ConsoleCommand( "RELAUNCH " $ FullURL $ " -server -log=Server.log" );
}

defaultproperties
{
}

