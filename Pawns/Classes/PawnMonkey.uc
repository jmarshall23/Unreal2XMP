//=============================================================================
// PawnMonkey.uc
// $Author$
// $Date$
// $Revision$
//=============================================================================
class PawnMonkey extends UIHelper;

struct DeploySlot
{
	var string Name;
	var color Color;
	var int IconIndex;
};

var array<DeploySlot> DeploySlots;
var float NextUpdateTime;
var Actor LastDeployPoint;

var float NextEnergyUpdateTime;

function interface ResetScripting() { local ScriptedController C; foreach GetPlayerOwner().AllActors( class'ScriptedController', C ) C.ResetScripting(2); }
function interface AbortScenes() { local SceneManager M; foreach GetPlayerOwner().AllActors( class'SceneManager', M ) M.AbortScene(); }

//-----------------------------------------------------------------------------
//  HUD Accessors
//-----------------------------------------------------------------------------
// Menu Accessors

// MOVEMENT

function interface bool IsXMPClass0()	{ return GetPlayerOwner()? && ClassIsChildOf( GetPlayerOwner().PawnClass, class'Ranger' ); }	//mdf: slow accessor?
function interface bool IsXMPClass1()	{ return GetPlayerOwner()? && ClassIsChildOf( GetPlayerOwner().PawnClass, class'Tech' ); }	//mdf: slow accessor?
function interface bool IsXMPClass2()	{ return GetPlayerOwner()? && ClassIsChildOf( GetPlayerOwner().PawnClass, class'Gunner' ); }	//mdf: slow accessor?

function interface string GetXMPClassName()
{
	if( ClassIsChildOf( GetPlayerOwner().PawnClass, class'Ranger' ) ) return "[Ranger]";	//!!localize
	if( ClassIsChildOf( GetPlayerOwner().PawnClass, class'Tech'   ) ) return "[Tech]";		//!!localize
	if( ClassIsChildOf( GetPlayerOwner().PawnClass, class'Gunner' ) ) return "[Gunner]";	//!!localize
	return "[invalid class]";
}

function interface SetXMPClass0()	{ GetPlayerOwner().ServerChangeClass( class'Ranger' ); GetPlayerOwner().UpdateURL("Class", "Pawns.Ranger", true); SaveConfig(); }
function interface SetXMPClass1()	{ GetPlayerOwner().ServerChangeClass( class'Tech'   ); GetPlayerOwner().UpdateURL("Class", "Pawns.Tech",   true); SaveConfig(); }
function interface SetXMPClass2()	{ GetPlayerOwner().ServerChangeClass( class'Gunner' ); GetPlayerOwner().UpdateURL("Class", "Pawns.Gunner", true); SaveConfig(); }

function interface SetHealth(float F)					{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) P.default.Health = F; }
function interface float GetHealth()					{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) return P.default.Health; else return 0.0; }

function interface SetGroundSpeed(float F)				{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) { P.default.GroundSpeed = F; P.GroundSpeed = F; } }
function interface float GetGroundSpeed()				{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) return P.GroundSpeed; else return 0.0; }

function interface SetAcceleration(float F)				{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) P.AccelRate = F; }
function interface float GetAcceleration()				{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) return P.AccelRate; else return 0.0; }

function interface SetJumpZ(float F)					{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) P.JumpZ = F; }
function interface float GetJumpZ()						{ local Pawn P; P = GetPlayerOwner().Pawn; if( P? ) return P.JumpZ; else return 0.0; }

function interface SetSprintRatio(float F)				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) C.SetSprintRatio(F); }
function interface float GetSprintRatio()				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) return C.GetSprintRatio(); else return 0.0; }

function interface SetStaminaMax(float F)				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) C.SetStaminaMax(F); }
function interface float GetStaminaMax()				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) return C.GetStaminaMax(); else return 0.0; }

function interface SetStaminaRechargeRate(float F)		{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) C.SetStaminaRechargeRate(F); }
function interface float GetStaminaRechargeRate()		{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) return C.GetStaminaRechargeRate(); else return 0.0; }

function interface SetSprintCost(float F)				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) C.SetSprintCost(F); }
function interface float GetSprintCost()				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) return C.GetSprintCost(); else return 0.0; }

function interface SetJumpJetZ(float F)					{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) C.SetJumpJetZ(F); }
function interface float GetJumpJetZ()					{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) return C.GetJumpJetZ(); else return 0.0; }

function interface SetJumpJetCost(float F)				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) C.SetJumpJetCost(F); }
function interface float GetJumpJetCost()				{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) return C.GetJumpJetCost(); else return 0.0; }

function interface SetJumpJetDuration(float F)			{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) C.SetJumpJetDuration(F); }
function interface float GetJumpJetDuration()			{ local U2PlayerController C; C=U2PlayerController(GetPlayerOwner()); if( C? ) return C.GetJumpJetDuration(); else return 0.0; }

// INVENTORY

/*
These classes are in a sub-package now.

function interface SetAutoTurretCost(float F)			{ class'AutoTurretDeployable'.default.DeploySkillCost = F; }
function interface float GetAutoTurretCost()			{ return class'AutoTurretDeployable'.default.DeploySkillCost; }

function interface SetRocketTurretCost(float F)			{ class'RocketTurretDeployable'.default.DeploySkillCost = F; }
function interface float GetRocketTurretCost()			{ return class'RocketTurretDeployable'.default.DeploySkillCost; }

function interface SetFieldGeneratorCost(float F)		{ class'ammoInvFieldGenerator'.default.DeploySkillCost = F; }
function interface float GetFieldGeneratorCost()		{ return class'ammoInvFieldGenerator'.default.DeploySkillCost; }

function interface SetAutoTurretCost(float F)			{}
function interface float GetAutoTurretCost()			{ return 0.0; }

function interface SetRocketTurretCost(float F)			{}
function interface float GetRocketTurretCost()			{ return 0.0; }

*/

//------------------------------------------------------------------------------

function interface ClearDeploySlots() { LastDeployPoint = None; NextUpdateTime = 0; DeploySlots.Length = 0; }

function UpdateDeploySlots()
{
	local U2PlayerController Player;
	local GameReplicationInfo GRI;
	local PlayerReplicationInfo PRI;
	local int i,j;

	Player = U2PlayerController(GetPlayerOwner());
	if( (Player.Level.TimeSeconds > NextUpdateTime) || (Player.RespawnPoint != LastDeployPoint) )
	{
		NextUpdateTime = Player.Level.TimeSeconds + 1.0;
		LastDeployPoint = Player.RespawnPoint;
		DeploySlots.Length = 0;
		GRI = Player.GameReplicationInfo;
		for( i=0; i<GRI.PRIArray.Length; i++ )
		{
			PRI = GRI.PRIArray[i];
			if( (PRI.RespawnPoint == LastDeployPoint) && (PRI != Player.PlayerReplicationInfo) )
			{
				j = DeploySlots.Length;
				DeploySlots.Length = j+1;
				DeploySlots[j].Name			= PRI.PlayerName;
				DeploySlots[j].IconIndex	= PRI.PawnClass.default.IconIndex;
				if( PRI.bWaitingToRespawn )
					DeploySlots[j].Color	= class'Actor'.static.ColorWhite();
				else
					DeploySlots[j].Color	= class'Actor'.static.ColorGrey();
			}
		}
	}
}

function interface bool GetDeploySlotEnabled0()			{ UpdateDeploySlots(); return DeploySlots.Length > 0; }
function interface bool GetDeploySlotEnabled1()			{ return DeploySlots.Length > 1; }
function interface bool GetDeploySlotEnabled2()			{ return DeploySlots.Length > 2; }
function interface bool GetDeploySlotEnabled3()			{ return DeploySlots.Length > 3; }
function interface bool GetDeploySlotEnabled4()			{ return DeploySlots.Length > 4; }
function interface bool GetDeploySlotEnabled5()			{ return DeploySlots.Length > 5; }
function interface bool GetDeploySlotEnabled6()			{ return DeploySlots.Length > 6; }
function interface bool GetDeploySlotEnabled7()			{ return DeploySlots.Length > 7; }

function interface string GetDeploySlotName0()			{ return DeploySlots[0].Name; }
function interface string GetDeploySlotName1()			{ return DeploySlots[1].Name; }
function interface string GetDeploySlotName2()			{ return DeploySlots[2].Name; }
function interface string GetDeploySlotName3()			{ return DeploySlots[3].Name; }
function interface string GetDeploySlotName4()			{ return DeploySlots[4].Name; }
function interface string GetDeploySlotName5()			{ return DeploySlots[5].Name; }
function interface string GetDeploySlotName6()			{ return DeploySlots[6].Name; }
function interface string GetDeploySlotName7()			{ return DeploySlots[7].Name; }

function interface color GetDeploySlotColor0()			{ return DeploySlots[0].Color; }
function interface color GetDeploySlotColor1()			{ return DeploySlots[1].Color; }
function interface color GetDeploySlotColor2()			{ return DeploySlots[2].Color; }
function interface color GetDeploySlotColor3()			{ return DeploySlots[3].Color; }
function interface color GetDeploySlotColor4()			{ return DeploySlots[4].Color; }
function interface color GetDeploySlotColor5()			{ return DeploySlots[5].Color; }
function interface color GetDeploySlotColor6()			{ return DeploySlots[6].Color; }
function interface color GetDeploySlotColor7()			{ return DeploySlots[7].Color; }

function interface int GetDeploySlotIconIndex0()		{ return DeploySlots[0].IconIndex; }
function interface int GetDeploySlotIconIndex1()		{ return DeploySlots[1].IconIndex; }
function interface int GetDeploySlotIconIndex2()		{ return DeploySlots[2].IconIndex; }
function interface int GetDeploySlotIconIndex3()		{ return DeploySlots[3].IconIndex; }
function interface int GetDeploySlotIconIndex4()		{ return DeploySlots[4].IconIndex; }
function interface int GetDeploySlotIconIndex5()		{ return DeploySlots[5].IconIndex; }
function interface int GetDeploySlotIconIndex6()		{ return DeploySlots[6].IconIndex; }
function interface int GetDeploySlotIconIndex7()		{ return DeploySlots[7].IconIndex; }

//-----------------------------------------------------------------------------
// team energy icons

interface function ResetEnergyStatus() { NextEnergyUpdateTime = 0; }
interface function bool UpdateEnergyStatus()
{
	local PlayerController PC;
	local GameReplicationInfo GRI;
	local PlayerReplicationInfo PRI;
	local TeamInfo Team;
	local float EnergyLevel;
	local float TeamFeatureWarningThreshold;
	local float Threshold[10];
	local int i;

	PC = GetPlayerOwner();
	if (!PC) return false;
	if (PC.Level.TimeSeconds < NextEnergyUpdateTime) return true;
	NextEnergyUpdateTime = PC.Level.TimeSeconds + 0.5;
	PRI = PC.PlayerReplicationInfo;
	if (!PRI) return false;
	Team = PRI.Team;
	if (!Team) return false;
	GRI = PC.GameReplicationInfo;
	if (!GRI) return false;
	EnergyLevel = Team.EnergyLevel;
	TeamFeatureWarningThreshold = class'XMPGame'.default.TeamFeatureWarningThreshold;
	for (i=0;i<10;i++)
		Threshold[i]=class'XMPGame'.default.EnergyThresholds[i].Threshold;

	GRI.bGetEnergy_Offline_SupplyPacks			= EnergyLevel < Threshold[0];
	GRI.bGetEnergy_Offline_StationaryTurrets	= EnergyLevel < Threshold[1];
	GRI.bGetEnergy_Offline_RocketTurrets		= EnergyLevel < Threshold[2];
	GRI.bGetEnergy_Offline_LandMines			= EnergyLevel < Threshold[3];
	GRI.bGetEnergy_Offline_FieldGenerators		= EnergyLevel < Threshold[4];
	GRI.bGetEnergy_Offline_LaserTripMines		= EnergyLevel < Threshold[5];
	GRI.bGetEnergy_Offline_AutoTurrets			= EnergyLevel < Threshold[6];
	GRI.bGetEnergy_Offline_Vehicles				= EnergyLevel < Threshold[7];
	GRI.bGetEnergy_Offline_SupplyStations		= EnergyLevel < Threshold[8];
	GRI.bGetEnergy_Offline_IconNode				= EnergyLevel < Threshold[9];

	GRI.bGetEnergy_Warning_SupplyPacks			= !GRI.bGetEnergy_Offline_SupplyPacks		&& EnergyLevel < Threshold[0] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_StationaryTurrets	= !GRI.bGetEnergy_Offline_StationaryTurrets	&& EnergyLevel < Threshold[1] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_RocketTurrets		= !GRI.bGetEnergy_Offline_RocketTurrets		&& EnergyLevel < Threshold[2] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_LandMines			= !GRI.bGetEnergy_Offline_LandMines			&& EnergyLevel < Threshold[3] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_FieldGenerators		= !GRI.bGetEnergy_Offline_FieldGenerators	&& EnergyLevel < Threshold[4] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_LaserTripMines		= !GRI.bGetEnergy_Offline_LaserTripMines	&& EnergyLevel < Threshold[5] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_AutoTurrets			= !GRI.bGetEnergy_Offline_AutoTurrets		&& EnergyLevel < Threshold[6] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_Vehicles				= !GRI.bGetEnergy_Offline_Vehicles			&& EnergyLevel < Threshold[7] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_SupplyStations		= !GRI.bGetEnergy_Offline_SupplyStations	&& EnergyLevel < Threshold[8] + TeamFeatureWarningThreshold;
	GRI.bGetEnergy_Warning_IconNode				= !GRI.bGetEnergy_Offline_IconNode			&& EnergyLevel < Threshold[9] + TeamFeatureWarningThreshold;
												  
	GRI.bGetEnergy_Online_SupplyPacks			= !GRI.bGetEnergy_Offline_SupplyPacks		&& !GRI.bGetEnergy_Warning_SupplyPacks;
	GRI.bGetEnergy_Online_StationaryTurrets		= !GRI.bGetEnergy_Offline_StationaryTurrets	&& !GRI.bGetEnergy_Warning_StationaryTurrets;
	GRI.bGetEnergy_Online_RocketTurrets			= !GRI.bGetEnergy_Offline_RocketTurrets		&& !GRI.bGetEnergy_Warning_RocketTurrets;
	GRI.bGetEnergy_Online_LandMines				= !GRI.bGetEnergy_Offline_LandMines			&& !GRI.bGetEnergy_Warning_LandMines;
	GRI.bGetEnergy_Online_FieldGenerators		= !GRI.bGetEnergy_Offline_FieldGenerators	&& !GRI.bGetEnergy_Warning_FieldGenerators;
	GRI.bGetEnergy_Online_LaserTripMines		= !GRI.bGetEnergy_Offline_LaserTripMines	&& !GRI.bGetEnergy_Warning_LaserTripMines;
	GRI.bGetEnergy_Online_AutoTurrets			= !GRI.bGetEnergy_Offline_AutoTurrets		&& !GRI.bGetEnergy_Warning_AutoTurrets;
	GRI.bGetEnergy_Online_Vehicles				= !GRI.bGetEnergy_Offline_Vehicles			&& !GRI.bGetEnergy_Warning_Vehicles;
	GRI.bGetEnergy_Online_SupplyStations		= !GRI.bGetEnergy_Offline_SupplyStations	&& !GRI.bGetEnergy_Warning_SupplyStations;
	GRI.bGetEnergy_Online_IconNode				= !GRI.bGetEnergy_Offline_IconNode			&& !GRI.bGetEnergy_Warning_IconNode;

	return true;
}

//------------------------------------------------------------------------------

defaultproperties
{
}
