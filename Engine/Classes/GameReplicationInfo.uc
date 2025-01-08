//=============================================================================
// GameReplicationInfo.
//=============================================================================
class GameReplicationInfo extends ReplicationInfo
	native nativereplication;

var string GameName;						// Assigned by GameInfo.
var string GameClass;						// Assigned by GameInfo.
var bool bTeamGame;							// Assigned by GameInfo.
var bool bStopCountDown;
var bool bMatchHasBegun;
var bool bTeamSymbolsUpdated;
var int  RemainingTime, ElapsedTime, RemainingMinute;
var float SecondCount;
var int GoalScore;
var int TimeLimit;
var int MaxLives;

var TeamInfo Teams[2];

var() globalconfig string ServerName;		// Name of the server, i.e.: Bob's Server.
var() globalconfig string ShortName;		// Abbreviated name of server, i.e.: B's Serv (stupid example)
var() globalconfig string AdminName;		// Name of the server admin.
var() globalconfig string AdminEmail;		// Email address of the server admin.
var() globalconfig int	  ServerRegion;		// Region of the game server.

var() globalconfig string MOTDLine1;		// Message
var() globalconfig string MOTDLine2;		// Of
var() globalconfig string MOTDLine3;		// The
var() globalconfig string MOTDLine4;		// Day

var Actor Winner;			// set by gameinfo when game ends

var() array<PlayerReplicationInfo> PRIArray;

var vector FlagPos;	// replicated 2D position of one object

enum ECarriedObjectState
{
    COS_Home,
    COS_HeldFriendly,
    COS_HeldEnemy,
    COS_Down,
};
var ECarriedObjectState CarriedObjectState[2];

// stats
var int MatchID;

// artifact score board - 4 out of 5 dentists surveyed recommend bools for their patients who chew bools.

var bool bArtBG_Slot0_Red_Dropped, bArtBG_Slot0_Red_Stored, bArtBG_Slot0_Red_Carried;
var bool bArtBG_Slot1_Red_Dropped, bArtBG_Slot1_Red_Stored, bArtBG_Slot1_Red_Carried;
var bool bArtBG_Slot2_Red_Dropped, bArtBG_Slot2_Red_Stored, bArtBG_Slot2_Red_Carried;
var bool bArtBG_Slot3_Red_Dropped, bArtBG_Slot3_Red_Stored, bArtBG_Slot3_Red_Carried;

var bool bArtBG_Slot0_Blue_Dropped, bArtBG_Slot0_Blue_Stored, bArtBG_Slot0_Blue_Carried;
var bool bArtBG_Slot1_Blue_Dropped, bArtBG_Slot1_Blue_Stored, bArtBG_Slot1_Blue_Carried;
var bool bArtBG_Slot2_Blue_Dropped, bArtBG_Slot2_Blue_Stored, bArtBG_Slot2_Blue_Carried;
var bool bArtBG_Slot3_Blue_Dropped, bArtBG_Slot3_Blue_Stored, bArtBG_Slot3_Blue_Carried;

var bool bArt_Defense_Slot0, bArt_Health_Slot0, bArt_Shield_Slot0, bArt_Skill_Slot0, bArt_Stamina_Slot0, bArt_WeaponDamage_Slot0;
var bool bArt_Defense_Slot1, bArt_Health_Slot1, bArt_Shield_Slot1, bArt_Skill_Slot1, bArt_Stamina_Slot1, bArt_WeaponDamage_Slot1;
var bool bArt_Defense_Slot2, bArt_Health_Slot2, bArt_Shield_Slot2, bArt_Skill_Slot2, bArt_Stamina_Slot2, bArt_WeaponDamage_Slot2;
var bool bArt_Defense_Slot3, bArt_Health_Slot3, bArt_Shield_Slot3, bArt_Skill_Slot3, bArt_Stamina_Slot3, bArt_WeaponDamage_Slot3;

// energy source score board

var bool bEnergySource0,  bEnergySource1,  bEnergySource2,  bEnergySource3,  bEnergySource4,  bEnergySource5,  bEnergySource6,  bEnergySource7;
var bool bEnergySource8,  bEnergySource9,  bEnergySource10, bEnergySource11, bEnergySource12, bEnergySource13, bEnergySource14, bEnergySource15;

var bool bEnergySourceRed0,   bEnergySourceRed1,   bEnergySourceRed2,   bEnergySourceRed3,   bEnergySourceRed4,   bEnergySourceRed5,   bEnergySourceRed6,   bEnergySourceRed7;
var bool bEnergySourceBlue0,  bEnergySourceBlue1,  bEnergySourceBlue2,  bEnergySourceBlue3,  bEnergySourceBlue4,  bEnergySourceBlue5,  bEnergySourceBlue6,  bEnergySourceBlue7;

var bool bEnergySourceRed8,   bEnergySourceRed9,   bEnergySourceRed10,  bEnergySourceRed11,  bEnergySourceRed12,  bEnergySourceRed13,  bEnergySourceRed14,  bEnergySourceRed15;
var bool bEnergySourceBlue8,  bEnergySourceBlue9,  bEnergySourceBlue10, bEnergySourceBlue11, bEnergySourceBlue12, bEnergySourceBlue13, bEnergySourceBlue14, bEnergySourceBlue15;

var bool bGetEnergy_Offline_RocketTurrets,
		 bGetEnergy_Offline_FieldGenerators,
		 bGetEnergy_Offline_LandMines,
		 bGetEnergy_Offline_LaserTripMines,
		 bGetEnergy_Offline_AutoTurrets,
		 bGetEnergy_Offline_SupplyPacks,
		 bGetEnergy_Offline_IconNode,
		 bGetEnergy_Offline_StationaryTurrets,
		 bGetEnergy_Offline_SupplyStations,
		 bGetEnergy_Offline_Vehicles,
		 
		 bGetEnergy_Warning_RocketTurrets,
		 bGetEnergy_Warning_FieldGenerators,
		 bGetEnergy_Warning_LandMines,
		 bGetEnergy_Warning_LaserTripMines,
		 bGetEnergy_Warning_AutoTurrets,
		 bGetEnergy_Warning_SupplyPacks,
		 bGetEnergy_Warning_IconNode,
		 bGetEnergy_Warning_StationaryTurrets,
		 bGetEnergy_Warning_SupplyStations,
		 bGetEnergy_Warning_Vehicles,
		 
		 bGetEnergy_Online_RocketTurrets,
		 bGetEnergy_Online_FieldGenerators,
		 bGetEnergy_Online_LandMines,
		 bGetEnergy_Online_LaserTripMines,
		 bGetEnergy_Online_AutoTurrets,
		 bGetEnergy_Online_SupplyPacks,
		 bGetEnergy_Online_IconNode,
		 bGetEnergy_Online_StationaryTurrets,
		 bGetEnergy_Online_SupplyStations,
		 bGetEnergy_Online_Vehicles;

replication
{
	reliable if ( bNetDirty && (Role == ROLE_Authority) )
		RemainingTime, RemainingMinute, bStopCountDown, Winner, Teams, FlagPos, CarriedObjectState, bMatchHasBegun, MatchID; 

	reliable if ( bNetInitial && (Role==ROLE_Authority) )
		GameName, GameClass, bTeamGame, 
		ElapsedTime,MOTDLine1, MOTDLine2, 
		MOTDLine3, MOTDLine4, ServerName, ShortName, AdminName,
		AdminEmail, ServerRegion, GoalScore, MaxLives, TimeLimit;

	reliable if ( bNetDirty && (Role == ROLE_Authority) )
		bArtBG_Slot0_Red_Dropped, bArtBG_Slot0_Red_Stored, bArtBG_Slot0_Red_Carried,
		bArtBG_Slot1_Red_Dropped, bArtBG_Slot1_Red_Stored, bArtBG_Slot1_Red_Carried,
		bArtBG_Slot2_Red_Dropped, bArtBG_Slot2_Red_Stored, bArtBG_Slot2_Red_Carried,
		bArtBG_Slot3_Red_Dropped, bArtBG_Slot3_Red_Stored, bArtBG_Slot3_Red_Carried,
		bArtBG_Slot0_Blue_Dropped, bArtBG_Slot0_Blue_Stored, bArtBG_Slot0_Blue_Carried,
		bArtBG_Slot1_Blue_Dropped, bArtBG_Slot1_Blue_Stored, bArtBG_Slot1_Blue_Carried,
		bArtBG_Slot2_Blue_Dropped, bArtBG_Slot2_Blue_Stored, bArtBG_Slot2_Blue_Carried,
		bArtBG_Slot3_Blue_Dropped, bArtBG_Slot3_Blue_Stored, bArtBG_Slot3_Blue_Carried,
		bArt_Defense_Slot0, bArt_Health_Slot0, bArt_Shield_Slot0, bArt_Skill_Slot0, bArt_Stamina_Slot0, bArt_WeaponDamage_Slot0,
		bArt_Defense_Slot1, bArt_Health_Slot1, bArt_Shield_Slot1, bArt_Skill_Slot1, bArt_Stamina_Slot1, bArt_WeaponDamage_Slot1,
		bArt_Defense_Slot2, bArt_Health_Slot2, bArt_Shield_Slot2, bArt_Skill_Slot2, bArt_Stamina_Slot2, bArt_WeaponDamage_Slot2,
		bArt_Defense_Slot3, bArt_Health_Slot3, bArt_Shield_Slot3, bArt_Skill_Slot3, bArt_Stamina_Slot3, bArt_WeaponDamage_Slot3;

	reliable if ( bNetInitial && (Role==ROLE_Authority) )
		bEnergySource0, bEnergySource1, bEnergySource2,  bEnergySource3,  bEnergySource4,  bEnergySource5,  bEnergySource6,  bEnergySource7,
		bEnergySource8, bEnergySource9, bEnergySource10, bEnergySource11, bEnergySource12, bEnergySource13, bEnergySource14, bEnergySource15;

	reliable if ( bNetDirty && (Role == ROLE_Authority) )
		bEnergySourceRed0,   bEnergySourceRed1,   bEnergySourceRed2,   bEnergySourceRed3,   bEnergySourceRed4,   bEnergySourceRed5,   bEnergySourceRed6,   bEnergySourceRed7,
		bEnergySourceBlue0,  bEnergySourceBlue1,  bEnergySourceBlue2,  bEnergySourceBlue3,  bEnergySourceBlue4,  bEnergySourceBlue5,  bEnergySourceBlue6,  bEnergySourceBlue7,
		bEnergySourceRed8,   bEnergySourceRed9,   bEnergySourceRed10,  bEnergySourceRed11,  bEnergySourceRed12,  bEnergySourceRed13,  bEnergySourceRed14,  bEnergySourceRed15,
		bEnergySourceBlue8,  bEnergySourceBlue9,  bEnergySourceBlue10, bEnergySourceBlue11, bEnergySourceBlue12, bEnergySourceBlue13, bEnergySourceBlue14, bEnergySourceBlue15;
}

simulated function SetCarriedObjectState(int Team, name NewState)
{
	switch( NewState )
	{
		case 'Down':
			CarriedObjectState[Team] = COS_Down;
			break;
		case 'HeldEnemy ':
			CarriedObjectState[Team] = COS_HeldEnemy;
			break;
		case 'Home ':
			CarriedObjectState[Team] = COS_Home;
			break;
		case 'HeldFriendly ':
			CarriedObjectState[Team] = COS_HeldFriendly;
			break;
	}
}

simulated function name GetCarriedObjectState(int Team)
{
	switch( CarriedObjectState[Team] )
	{
		case COS_Down:
			return 'Down';
		case COS_HeldEnemy:
			return 'HeldEnemy';
		case COS_Home:
			return 'Home';
		case COS_HeldFriendly:
			return 'HeldFriendly';
	}
	return '';
}			

simulated function PostNetBeginPlay()
{
	local PlayerReplicationInfo PRI;
	
	ForEach DynamicActors(class'PlayerReplicationInfo',PRI)
		AddPRI(PRI);
	if ( Level.NetMode == NM_Client )
		TeamSymbolNotify();
}

simulated function TeamSymbolNotify()
{
	local Actor A;

	if ( (Teams[0] == None) || (Teams[1] == None)
		|| (Teams[0].TeamIcon == None) || (Teams[1].TeamIcon == None) )
		return;
	bTeamSymbolsUpdated = true;
	ForEach AllActors(class'Actor', A)
		A.SetGRI(self);
}

simulated function PostBeginPlay()
{
	if( Level.NetMode == NM_Client )
	{
		// clear variables so we don't display our own values if the server has them left blank 
		ServerName = "";
		AdminName = "";
		AdminEmail = "";
		MOTDLine1 = "";
		MOTDLine2 = "";
		MOTDLine3 = "";
		MOTDLine4 = "";
	}

	SecondCount = Level.TimeSeconds;
	SetTimer(1, true);
}

/* Reset() 
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	Super.Reset();
	Winner = None;
}

simulated function Timer()
{
	if ( Level.NetMode == NM_Client )
	{
		if (Level.TimeSeconds - SecondCount >= Level.TimeDilation)
		{
			ElapsedTime++;
			if ( RemainingMinute != 0 )
			{
				RemainingTime = RemainingMinute;
				RemainingMinute = 0;
			}
			if ( (RemainingTime > 0) && !bStopCountDown )
				RemainingTime--;
			SecondCount += Level.TimeDilation;
		}
		if ( !bTeamSymbolsUpdated )
			TeamSymbolNotify();
	}
}

simulated function AddPRI(PlayerReplicationInfo PRI)
{
    PRIArray[PRIArray.Length] = PRI;
}

simulated function RemovePRI(PlayerReplicationInfo PRI)
{
    local int i;

    for (i=0; i<PRIArray.Length; i++)
    {
        if (PRIArray[i] == PRI)
            break;
    }

    if (i == PRIArray.Length)
    {
        log("GameReplicationInfo::RemovePRI() pri="$PRI$" not found.", 'Error');
        return;
    }

    PRIArray.Remove(i,1);
	}

simulated function GetPRIArray(out array<PlayerReplicationInfo> pris)
{
    local int i;
    local int num;

    pris.Remove(0, pris.Length);
    for (i=0; i<PRIArray.Length; i++)
    {
        if (PRIArray[i] != None)
            pris[num++] = PRIArray[i];
    }
}

simulated function TeamInfo GetTeamInfo( int Index )
{
	if( Index >= 0 && Index < ArrayCount( Teams ) )
		return Teams[Index];
	else
		return None;
}


defaultproperties
{
	CarriedObjectState[0]=FLAG_Home
	CarriedObjectState[1]=FLAG_Home
	bStopCountDown=true
	RemoteRole=ROLE_SimulatedProxy
	bAlwaysRelevant=True
	ServerName="Another Server"
	ShortName="Server"
	MOTDLine1=""
	MOTDLine2=""
	MOTDLine3=""
	MOTDLine4=""
    bNetNotify=true
}
