//=============================================================================
// U2PlayerController.uc
//=============================================================================
class U2PlayerController extends PlayerController
	native;

//!! fix excessive use of U2Pawn(Pawn)

//-----------------------------------------------------------------------------
// WeaponPanel support.

struct native WeaponInfo
{
	var config string	WeaponName;	// User readable name of the weapon.
	var config byte		Priority;	// Order within the tray (lower numbers = higher priority) - essentially group offset.
	var /****/ Weapon	Weapon;		// Pointer to the associated weapon - if the player has it currently.
};

// NOTE[aleiby]: Stored as separate variables to make the ini cleaner and 
// to support weapon mods from clashing with U2 weapons with the same InventoryGroup/GroupOffsets
var /*config*/ array<WeaponInfo> WeaponOrder1;
var /*config*/ array<WeaponInfo> WeaponOrder2;
var /*config*/ array<WeaponInfo> WeaponOrder3;
var /*config*/ array<WeaponInfo> WeaponOrder4;
var /*config*/ array<WeaponInfo> WeaponOrder5;

var() globalconfig byte UIAlpha;

var float AmmoFlashTime;

var float WoundedDelaySeconds;			// time before player is given the option to return to deployment area
var float WoundedTime;					// time at which player died

var DeploymentPoint RespawnPoint;		// point from which player will respawn
var float NextDeployTime;				// time the new players will be redeployed
var float LastGestureTime;

var font MessageFont;
var sound SaySound,TeamSaySound;

var class<VoiceMessage>
				VM_MedicMsg,
				VM_RepairsMsg,
				VM_SuppliesMsg,
				VM_HackerMsg,
				VM_OffenseMsg,
				VM_BackupMsg,
				VM_DefenseMsg,
				VM_FollowMsg,
				VM_BaseMsg,
				VM_GeneratorMsg,
				VM_ArtifactMsg,
				VM_AllClearMsg,
				VM_ThanksMsg,
				VM_SorryMsg,
				VM_NoProblemMsg,
				VM_AcknowledgedMsg;

var Material UseProjectorMaterial,UseHighlightMaterial;

var float LastPlayerFragTime;
var int MonsterKills;

//-----------------------------------------------------------------------------

replication
{
	reliable if( Role<ROLE_Authority )
		ServerViewNextDeployPoint, ServerSetDeployPoint, ServerWaitToDeploy, ServerAltFire, ServerPlayVoice, ServerSendScores, ServerDumpArtifacts;

	reliable if( bNetDirty && bNetOwner && (Role==ROLE_Authority) )
		RespawnPoint;

	reliable if ( Role==ROLE_Authority )
		WoundedTime, ClientSetNextDeployTime, ClientDisplayScoreMsg, ClientSetPreMatchTime;
		
	unreliable if( Role<ROLE_Authority )
        Taunt, Gesture;
}

//-----------------------------------------------------------------------------

function ServerWaitToDeploy();
function ServerViewNextDeployPoint();
function ServerSetDeployPoint();
function ServerAltFire( float F );
function NotifyDeployPointHacked();

//-----------------------------------------------------------------------------

exec function Scores()
{
	ServerSendScores();
}

function ServerSendScores()
{
	local XMPGame Game;
	Game = XMPGame(Level.Game);
    if( Game? && Game.ScoreKeeper? )
		Game.ScoreKeeper.SendScores();
}

exec function PrintScores()
{
	local int iPRI, iEntry;
	local GameReplicationInfo GRI;

	GRI = GameReplicationInfo;
	for( iPRI=0; iPRI < GRI.PRIArray.Length; iPRI++ )
	{
		if( GameReplicationInfo.PRIArray[iPRI]? )
		{
			for( iEntry=0; iEntry < ArrayCount(GRI.PRIArray[iPRI].ScoreStat); iEntry++ )
				DM( class'ScoreKeeper'.default.ScoreLabels[iEntry]  $ ":" @ GRI.PRIArray[iPRI].ScoreStat[iEntry] );

			for( iEntry=0; iEntry < ArrayCount(GRI.PRIArray[iPRI].BonusStat); iEntry++ )
				DM( class'ScoreKeeper'.default.BonusLabels[iEntry]  $ ":" @ GRI.PRIArray[iPRI].BonusStat[iEntry] );
		}
	}
}

//-----------------------------------------------------------------------------
// Menu Accessors

function interface SetSprintRatio(float F)			{ U2Pawn(Pawn).SprintRatio = F; }
function interface float GetSprintRatio()			{ return U2Pawn(Pawn).SprintRatio; }

function interface SetStaminaMax(float F)			{ U2Pawn(Pawn).StaminaMax = F; }
function interface float GetStaminaMax()			{ return U2Pawn(Pawn).StaminaMax; }

function interface SetStaminaRechargeRate(float F)	{ U2Pawn(Pawn).StaminaRechargeRate = F; }
function interface float GetStaminaRechargeRate()	{ return U2Pawn(Pawn).StaminaRechargeRate; }

function interface SetSprintCost(float F)			{ U2Pawn(Pawn).SprintCost = F; }
function interface float GetSprintCost()			{ return U2Pawn(Pawn).SprintCost; }

function interface SetJumpJetZ(float F)				{ U2Pawn(Pawn).JumpJetZ = F; }
function interface float GetJumpJetZ()				{ return U2Pawn(Pawn).JumpJetZ; }

function interface SetJumpJetCost(float F)			{ U2Pawn(Pawn).JumpJetCost = F; }
function interface float GetJumpJetCost()			{ return U2Pawn(Pawn).JumpJetCost; }

function interface SetJumpJetDuration(float F)		{ U2Pawn(Pawn).JumpJetDuration = F; }
function interface float GetJumpJetDuration()		{ return U2Pawn(Pawn).JumpJetDuration; }

//-----------------------------------------------------------------------------
// VoiceMenu

interface function VM_Medic()			{ ServerPlayVoice(0); }
interface function VM_Repairs()			{ ServerPlayVoice(1); }
interface function VM_Supplies()		{ ServerPlayVoice(2); }
interface function VM_Hacker()			{ ServerPlayVoice(3); }
interface function VM_Offense()			{ ServerPlayVoice(4); }
interface function VM_Backup()			{ ServerPlayVoice(5); }
interface function VM_Defense()			{ ServerPlayVoice(6); }
interface function VM_Follow()			{ ServerPlayVoice(7); }
interface function VM_Base()			{ ServerPlayVoice(8); }
interface function VM_Generator()		{ ServerPlayVoice(9); }
interface function VM_Artifact()		{ ServerPlayVoice(10); }
interface function VM_AllClear()		{ ServerPlayVoice(11); }
interface function VM_Thanks()			{ ServerPlayVoice(12); }
interface function VM_Sorry()			{ ServerPlayVoice(13); }
interface function VM_NoProblem()		{ ServerPlayVoice(14); }
interface function VM_Acknowledged()	{ ServerPlayVoice(15); }

function ServerPlayVoice(byte i)
{
	if (!ViewTarget) return;
	switch(i)
	{
	case 0:		Level.Game.BroadcastLocalizedTeam(self,VM_MedicMsg,,PlayerReplicationInfo);			break;
	case 1:		Level.Game.BroadcastLocalizedTeam(self,VM_RepairsMsg,,PlayerReplicationInfo);		break;
	case 2:		Level.Game.BroadcastLocalizedTeam(self,VM_SuppliesMsg,,PlayerReplicationInfo);		break;
	case 3:		Level.Game.BroadcastLocalizedTeam(self,VM_HackerMsg,,PlayerReplicationInfo);		break;
	case 4:		Level.Game.BroadcastLocalizedTeam(self,VM_OffenseMsg,,PlayerReplicationInfo);		break;
	case 5:		Level.Game.BroadcastLocalizedTeam(self,VM_BackupMsg,,PlayerReplicationInfo);		break;
	case 6:		Level.Game.BroadcastLocalizedTeam(self,VM_DefenseMsg,,PlayerReplicationInfo);		break;
	case 7:		Level.Game.BroadcastLocalizedTeam(self,VM_FollowMsg,,PlayerReplicationInfo);		break;
	case 8:		Level.Game.BroadcastLocalizedTeam(self,VM_BaseMsg,,PlayerReplicationInfo);			break;
	case 9:		Level.Game.BroadcastLocalizedTeam(self,VM_GeneratorMsg,,PlayerReplicationInfo);		break;
	case 10:	Level.Game.BroadcastLocalizedTeam(self,VM_ArtifactMsg,,PlayerReplicationInfo);		break;
	case 11:	Level.Game.BroadcastLocalizedTeam(self,VM_AllClearMsg,,PlayerReplicationInfo);		break;
	case 12:	Level.Game.BroadcastLocalizedTeam(self,VM_ThanksMsg,,PlayerReplicationInfo);		break;
	case 13:	Level.Game.BroadcastLocalizedTeam(self,VM_SorryMsg,,PlayerReplicationInfo);			break;
	case 14:	Level.Game.BroadcastLocalizedTeam(self,VM_NoProblemMsg,,PlayerReplicationInfo);		break;
	case 15:	Level.Game.BroadcastLocalizedTeam(self,VM_AcknowledgedMsg,,PlayerReplicationInfo);	break;
	};
}

//-----------------------------------------------------------------------------
// Hud Accessors

function interface bool ShowHUD() { return true; }

function interface bool HasArtifact() { return PlayerReplicationInfo? && PlayerReplicationInfo.bHasArtifact; }

function interface int GetPlayerClassIndex() { return PawnClass.default.IconIndex; }

function interface float GetMaxSpeedPct()
{
	if( ValidPawn( Pawn ) )
		return FMin( 1.0, Pawn.GroundSpeed / 2000.0 );
	else
		return 0.0;
}

function interface float GetSpeedPct()
{
	if( ValidPawn( Pawn ) )
		return FMin( 1.0, VSize(Pawn.Velocity) / 2000.0 );
	else
		return 0.0;
}

function interface float GetSpeed()
{
	if( ValidPawn( Pawn ) )
		return VSize(Pawn.Velocity);
	else
		return 0.0;
}

//-----------------------------------------------------------------------------

function interface float GetStaminaPct()
{
	if( ValidPawn( U2Pawn(Pawn) )  )
		return U2Pawn(Pawn).Stamina / U2Pawn(Pawn).StaminaMax;
	else if( ValidPawn( KVehicle(Pawn) )  )
		return KVehicle(Pawn).Stamina / KVehicle(Pawn).StaminaMax;
	else
		return 1.0;
}

//-----------------------------------------------------------------------------

function interface float GetSkillPct()
{
	if( ValidPawn( U2Pawn(Pawn) )  )
		return U2Pawn(Pawn).Skill / U2Pawn(Pawn).SkillMax;
	else
		return 1.0;
}

//-----------------------------------------------------------------------------

function interface string GetTimeRemaining()
{
	if( GameReplicationInfo? )
		return class'UIConsole'.static.SecondsToTime( GameReplicationInfo.RemainingTime );
	else
		return "NA";
}

//-----------------------------------------------------------------------------

function interface float GetUIAlpha(){ return UIAlpha; }
function interface SetUIAlpha( float A ){ UIAlpha=A; SaveConfig(); }

//-----------------------------------------------------------------------------

function SetPowerSuitPower( float NewPower )
{
	if( U2Pawn(Pawn)? )
		U2Pawn(Pawn).SetPowerSuitPower( NewPower );
}

//-----------------------------------------------------------------------------

function interface float GetPowerSuitPct()
{
	local U2Pawn P;

	P = U2Pawn( Pawn );
	if( P? && P.PowerSuitMax > 0 )
		return P.PowerSuitPower / P.PowerSuitMax;
	else
		return -1.0;
}

//-----------------------------------------------------------------------------

function interface string GetPowerSuitPowerString()
{
	local U2Pawn P;
	local int Value;

	P = U2Pawn(Pawn);
	if( P? ) Value = P.PowerSuitPower;

	if		( Value < 1    ) return "000";
	else if	( Value < 10   ) return "00"$Value;
	else if	( Value < 100  ) return "0"$Value;
	else if	( Value < 1000 ) return string(Value);
	else				     return "MAX";
}

//-----------------------------------------------------------------------------
function interface float GetAmmoClipPct()
{
	if ( Pawn != None && Pawn.Weapon != None )
		return float(Pawn.Weapon.GetClipAmount()) / float(Pawn.Weapon.GetClipSize());
	else
		return 0;
}

//-----------------------------------------------------------------------------
function interface float GetAmmoTotalPct()
{
	if( Pawn != None && Pawn.Weapon != None )
		return float(Pawn.Weapon.GetAmmoAmount() - Pawn.Weapon.GetClipAmount()) / float(Pawn.Weapon.GetMaxAmmo());
	else
		return 0;
}

//-----------------------------------------------------------------------------
function interface string GetTotalAmmoString()
{
	local int Value;
	if( Pawn != None && Pawn.Weapon != None )
		Value = Pawn.Weapon.GetAmmoAmount() - Pawn.Weapon.GetClipAmount();
	if		( Value < 1    ) return "000";
	else if	( Value < 10   ) return "00"$Value;
	else if	( Value < 100  ) return "0"$Value;
	else if	( Value < 1000 ) return string(Value);
	else				     return "MAX";
}

//-----------------------------------------------------------------------------
function interface string GetClipRoundsRemaining()
{
	local int Value;
	if ( Pawn != None && Pawn.Weapon != None )
		Value = Pawn.Weapon.GetClipAmount();
	if		( Value < 1    ) return "000";
	else if	( Value < 10   ) return "00"$Value;
	else if	( Value < 100  ) return "0"$Value;
	else if	( Value < 1000 ) return string(Value);
	else				     return "MAX";
}

//-----------------------------------------------------------------------------

function interface float GetTeamEnergyPct()
{
	if( PlayerReplicationInfo? && PlayerReplicationInfo.Team? )
		return PlayerReplicationInfo.Team.EnergyLevel / class'XMPGame'.default.TeamEnergyMax;
	return 1.0;
}

//-----------------------------------------------------------------------------

function interface float GetEnergyRed()			{ local TeamInfo T; T=GameReplicationInfo.Teams[0]; return T.EnergyLevel / class'XMPGame'.default.TeamEnergyMax; }
function interface float GetEnergyBlue()		{ local TeamInfo T; T=GameReplicationInfo.Teams[1]; return T.EnergyLevel / class'XMPGame'.default.TeamEnergyMax; }

function interface float GetEnergyAmtRed()		{ local TeamInfo T; T=GameReplicationInfo.Teams[0]; return T.EnergyLevel * 1000.0; }
function interface float GetEnergyAmtBlue()		{ local TeamInfo T; T=GameReplicationInfo.Teams[1]; return T.EnergyLevel * 1000.0; }

//-----------------------------------------------------------------------------

function interface float GetTeamEnergy()
{
	if( PlayerReplicationInfo? && PlayerReplicationInfo.Team? )
		return PlayerReplicationInfo.Team.EnergyLevel * 1000.0;
	return -1.0;
}

//-----------------------------------------------------------------------------

function interface color GetProgressColorA()	{ return ProgressColor[0]; }

//-----------------------------------------------------------------------------
// cheats

function ServerSetTeamEnergy( int TeamNumber, float NewEnergyLevel )
{
	if( TeamNumber < ArrayCount(GameReplicationInfo.Teams) )
		GameReplicationInfo.Teams[TeamNumber].EnergyLevel = NewEnergyLevel;
}

exec function DumpArtifacts()
{
	ServerDumpArtifacts();
}

function ServerDumpArtifacts()
{
	local IconNode IN;
	local Artifact Art;
	local ArtifactPickup AP;

	foreach AllActors( class'IconNode', IN )
		IN.DumpArtifacts();
	foreach AllActors( class'ArtifactPickup', AP )
		AP.DumpArtifacts();
	foreach AllActors( class'Artifact', Art )
		Art.DumpArtifacts();
}

//-----------------------------------------------------------------------------
// artifact score board

function interface bool HasArtifact0()	 { local Artifact A; if( PlayerReplicationInfo? ) A = Artifact( PlayerReplicationInfo.HasArtifact ); return( A? && A.ID == 0 ); }
function interface bool HasArtifact1()	 { local Artifact A; if( PlayerReplicationInfo? ) A = Artifact( PlayerReplicationInfo.HasArtifact ); return( A? && A.ID == 1 ); }
function interface bool HasArtifact2()	 { local Artifact A; if( PlayerReplicationInfo? ) A = Artifact( PlayerReplicationInfo.HasArtifact ); return( A? && A.ID == 2 ); }
function interface bool HasArtifact3()	 { local Artifact A; if( PlayerReplicationInfo? ) A = Artifact( PlayerReplicationInfo.HasArtifact ); return( A? && A.ID == 3 ); }

//-----------------------------------------------------------------------------
function interface array<string> GetObjectives()
{
	local array<string> Objectives;
	Objectives.length=1;
	Objectives[0]="NOT IMPLEMENTED";
	return Objectives;
//!!MERGE	return U2GameInfo(Level.Game).GetObjectives();
}

//-----------------------------------------------------------------------------
function interface float GetTimeToStart()
{
	return FMax( 0, WaitDelay - Level.TimeSeconds );
}

//-----------------------------------------------------------------------------
function interface float GetTimeToDeploy()
{
	return FMax( 0, NextDeployTime - Level.TimeSeconds );
}

//-----------------------------------------------------------------------------
function interface bool WeaponInfoEnabled() //!!tbd
{
	return (Pawn!=None && Pawn.Weapon!=None);
}

//-----------------------------------------------------------------------------
function interface string GetWeaponSeq() //!!tbd
{
	local name Anim;
	local float frame,rate;
	
	if( Pawn? && Pawn.Weapon? )
	{
		Pawn.Weapon.GetAnimParams(0,Anim,frame,rate);
		return "["$Anim$"|" $ Pawn.Weapon.GetStateName()$"]";
	}
	else
	{
		return "["$Anim$"|???]";
	}
}

//-----------------------------------------------------------------------------
function interface float GetWeaponSeqPct() //!!tbd
{
	if( Pawn? && Pawn.Weapon? )
		return Pawn.Weapon.LatentFloat / Pawn.Weapon.TempTimer;
	else
		return 0.0;
}

//-----------------------------------------------------------------------------

exec function PrevWeapon()
{
	Super.PrevWeapon();
	HighlightPendingWeapon( Pawn );
}

//-----------------------------------------------------------------------------

exec function NextWeapon()
{
	Super.NextWeapon();
	HighlightPendingWeapon( Pawn );
}

//-----------------------------------------------------------------------------

exec function SwitchWeapon (byte F )
{
	Super.SwitchWeapon(F);
	HighlightPendingWeapon( Pawn );
}

//-----------------------------------------------------------------------------

exec function SwitchToBestWeapon()
{
	Super.SwitchToBestWeapon();
	if( Pawn? && Pawn.PendingWeapon!=None )
		HighlightPendingWeapon( Pawn );
}

//-----------------------------------------------------------------------------
// WeaponPanel convenience functions

function WeaponPanel GetWeaponPanel()
{
	local UIConsole Console;
	local WeaponPanel WP;
	local int i;

	if( Viewport(Player) != None )
	{
		Console = UIConsole(Player.Console);
		if( Console != None )
		{
			// Fix ARL: This code may break if we don't force Root to be loaded first.  (make changes in Console then delete this note)
			for(i=0;i<Console.Helpers.length;i++)
			{
				WP=WeaponPanel(Console.Helpers[i]);
				if(WP!=None) break;
			}
		}
	}

	return WP;
}

//-----------------------------------------------------------------------------

function HighlightPendingWeapon( Pawn P )
{
	local WeaponPanel Panel;
	if( P != None && P.PendingWeapon != None )
		Panel = GetWeaponPanel();
	if( Panel != None )
		Panel.HighlightWeapon( P.PendingWeapon );
}

//-----------------------------------------------------------------------------
// Inventory notifications

function NotifyAddInventory(inventory AddedItem)
{
	local WeaponPanel WP;
	WP = GetWeaponPanel();
	if( WP != None && Weapon(AddedItem) != None )
		WP.DoAddWeapon( Weapon(AddedItem), true );
	Super.NotifyAddInventory( AddedItem );
}

//-----------------------------------------------------------------------------

function NotifyDeleteInventory(inventory RemovedItem)
{
	local WeaponPanel WP;
	WP = GetWeaponPanel();
	if( WP != None && Weapon(RemovedItem) != None )
	{
		WP.DoRemoveWeapon( Weapon(RemovedItem), true);
		WP.HighlightWeapon( Pawn.Weapon );
	}
	Super.NotifyDeleteInventory( RemovedItem );
}

//-----------------------------------------------------------------------------

function NotifyAmmoChanged()
{
	local int InventoryCount;
	local float AccumSupplyPct;
	local Inventory Inv;
	local Weapon W;

	for( Inv=Pawn.Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		W = Weapon(Inv);
		if( W? && W.AmmoType? && W.AmmoType.AmmoUsed > 0 )
		{
			AccumSupplyPct += W.ResupplyStatus();
			InventoryCount++;
		}
	}
	if( InventoryCount > 0 )
		PlayerReplicationInfo.AmmoEquipedPct = AccumSupplyPct / InventoryCount;
}

//-----------------------------------------------------------------------------

simulated event InitPlayer()
{
	Super.InitPlayer();
	if (UseProjector?)
	{
		UseProjector.ProjTexture = UseProjectorMaterial;
		UseProjector.Highlight = UseHighlightMaterial;
	}
	if (Viewport(Player)?)
		SendEvent("LevelLoadEnd");
}

//-----------------------------------------------------------------------------

event TeamMessage( PlayerReplicationInfo PRI, coerce string Msg, name Type, optional sound MessageSound )
{
	if(Type == 'Say')
	{
		if(PRI != None)
			Msg = PRI.PlayerName$": "$Msg;

		if(!MessageSound)
			MessageSound=SaySound;

		class'Console'.static.ShowStatusMessage( Self, Msg, 6.0, MessageFont, ColorRed(), MessageSound,,, "MessageHolder",,, 400 );
	}

	if(Type == 'TeamSay')
	{
		if(PRI != None)
			Msg = PRI.PlayerName$": "$Msg;

		if(!MessageSound)
			MessageSound=TeamSaySound;

		class'Console'.static.ShowStatusMessage( Self, Msg, 6.0, MessageFont, ColorGreen(), MessageSound,,, "MessageHolder",,, 400 );
	}

	if( Player != None )
		Player.Console.Message( Msg, 6.0 );

	PRI.LastTalkTime = Level.TimeSeconds;
}

//-----------------------------------------------------------------------------

function ClientDisplayScoreMsg( string Msg )
{
	class'Console'.static.ShowStatusMessage( Self, Msg, 12.0, MessageFont, ColorWhite(),,,, "ScoreBoxHolder" );
	DM(Msg);
}

//-----------------------------------------------------------------------------

event MantleBegin();
event MantleEnd();

event PlayerTick( float DeltaTime )
{
	local float ClipPct;

	// same as super except //NEW below
	if ( bForcePrecache && (Level.TimeSeconds > ForcePrecacheTime) )
	{
		bForcePrecache = false;
		Level.FillPrecacheMaterialsArray();
		Level.FillPrecacheStaticMeshesArray();
	}
	if (!KVehicle(Pawn))										//NEW PWC (Even though KVehicle is not a U2Pawn we still want to sprint)
	{
		if ( !U2Pawn(Pawn) || (U2Pawn(Pawn).Stamina <= 0.0) )	//NEW
			bSprint = 0;										//NEW
	} else if (KVehicle(Pawn).Stamina <= 0.0)
		bSprint = 0;
	PlayerInput.PlayerInput(DeltaTime);
	if ( bUpdatePosition )
		ClientUpdatePosition();
	PlayerMove(DeltaTime);

	// Flashing ammo support.
	if( Pawn? && Pawn.Weapon? )
	{
		ClipPct = GetAmmoClipPct();
		if( ClipPct<0.5 )
		{
			AmmoFlashTime -= DeltaTime;
			if( AmmoFlashTime <= 0 )
			{
				SendEvent("ClipFlash");
				AmmoFlashTime = Blend( 0.25, 4.0, ClipPct );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// STATE CODE
//-----------------------------------------------------------------------------

// Player movement.
// Player Standing, walking, running, falling.
exec function Taunt( optional int Index );
exec function Gesture( optional int Index );

state PlayerWalking
{
ignores SeeEnemy, HearNoise, Bump;

	//-------------------------------------------------------------------------

	event MantleBegin()
	{
		// MantleBegin still called if player almost mantled but was moving too fast
		if( U2Pawn(Pawn).bMantling )
			GotoState( 'Mantling' );
		else if( !bUpdating )
			Pawn.PlayOwnedSound(U2Pawn(Pawn).MantleHurtSound, SLOT_Pain, 0.18,,80);
	}

	//-------------------------------------------------------------------------
	
	function bool CanGesture()
	{
		return( Pawn != None &&
				Pawn.Health > 0 && 
				(Pawn.Weapon == None || !Pawn.Weapon.bPointing) && 
				//tbd: Pawn.Physics == PHYS_Walking &&
				Level.TimeSeconds - LastGestureTime > 3.0 );
	}
	
	//-------------------------------------------------------------------------

	exec function Taunt( optional int Index )
	{
		if( CanGesture() )
		{
			if( U2Pawn(Pawn).SelectTaunt( Index ) )
				LastGestureTime = Level.TimeSeconds;
		}
	}

	//-------------------------------------------------------------------------

	exec function Gesture( optional int Index )
	{
		if( CanGesture() )
		{
			if( U2Pawn(Pawn).SelectGesture( Index ) )
				LastGestureTime = Level.TimeSeconds;
		}
	}
} //PlayerWalking

//-----------------------------------------------------------------------------

state Mantling
{
	ignores Fire, AltFire;

	//-------------------------------------------------------------------------

	function ProcessMove( float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot )
	{
		local vector MantleDir;
		if( U2Pawn(Pawn) != None )
		{
			if( U2Pawn(Pawn).Location.Z < U2Pawn(Pawn).MantleDestination.Z )
				U2Pawn(Pawn).Velocity = vect(0,0,1) * U2Pawn(Pawn).MantleSpeed; // move up to clear ledge
			else
	{
				MantleDir = U2Pawn(Pawn).MantleDestination - U2Pawn(Pawn).Location;
				U2Pawn(Pawn).Velocity = normal(MantleDir) * U2Pawn(Pawn).MantleSpeed; // move toward mantle destination
			}

			Global.ProcessMove(DeltaTime, vect(0,0,0), DoubleClickMove, DeltaRot);
	}
	}
	
	//-------------------------------------------------------------------------

	function PlayerMove( float DeltaTime )
	{
		local vector X, Y, Z, NewAccel;
		
		GetAxes( Pawn.Rotation, X, Y, Z );
		NewAccel = GetAccel( X, Y, Z);

		aLookup  *= 0.24;
		aTurn    *= 0.24;
		aUp		 *= 0.1;

		HandleMove( DeltaTime, NewAccel, DCLICK_None, Rot(0,0,0) );
	}

	//-------------------------------------------------------------------------
		
	function HandleMove( float DeltaTime, 
						 vector NewAccel, 
						 eDoubleClickDir DoubleClickMove, 
						 rotator DeltaRot )
	{
		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, NewAccel, DoubleClickMove, DeltaRot);
		else
			ProcessMove(DeltaTime, NewAccel, DoubleClickMove, DeltaRot);
	}

	//-------------------------------------------------------------------------

	function vector GetAccel( vector X, vector Y, vector Z )
	{
		local vector ReturnedAcceleration;
		ReturnedAcceleration = aForward*X + aStrafe*Y;
		ReturnedAcceleration.Z = 0;
		return ReturnedAcceleration;
	}

	//-------------------------------------------------------------------------

	event MantleEnd()
	{
		GotoState( 'PlayerWalking' );
	}

	//-------------------------------------------------------------------------

	event BeginState()
	{
		if( !bUpdating )
			Pawn.PlayOwnedSound(U2Pawn(Pawn).MantleSound, SLOT_Pain, 0.18,,80);

		Pawn.AirSpeed = U2Pawn(Pawn).MantleSpeed;			//!! use cached U2P if possible
		Pawn.SetPhysics( PHYS_Flying );

		U2Pawn(Pawn).MantlingBegin();			//!! use cached U2P if possible
	}

	//-------------------------------------------------------------------------

	event EndState()
	{
		Pawn.AirSpeed = Pawn.default.AirSpeed;
		U2Pawn(Pawn).bMantling = false;			//!! use cached U2P if possible
	}

} //Mantling

//-----------------------------------------------------------------------------

state Wounded
{
	ignores SeeEnemy, HearNoise, KilledBy, SwitchWeapon, NextWeapon, PrevWeapon;

	//-------------------------------------------------------------------------

	exec function Use();
	exec function UnUse();
	exec function Reload();
	function PawnDied( Pawn P );

	//-------------------------------------------------------------------------

	exec function Fire( optional float F )
	{
		// stop waiting for medic
		ServerWaitToDeploy();
	}

	//-------------------------------------------------------------------------

	exec function AltFire( optional float F )
	{
        Fire(F);
	}

	//-------------------------------------------------------------------------

	function HandleMercyKilled()
	{
		DestroyPawn();
		GotoState( 'WaitingToDeploy' );
	}

	//-------------------------------------------------------------------------

	function ServerWaitToDeploy()
	{
		if ( !bFrozen )
		{
			DestroyPawn();
			GotoState( 'WaitingToDeploy' );
		}
	}

	//-------------------------------------------------------------------------

	function DestroyPawn()
	{
		if ( Pawn != None && !Pawn.bDeleteMe )
		{
			SetLocation(Pawn.Location);
			Pawn.UnPossessed();
			Pawn.HandleDying();
			Pawn = None;
		}
		PendingMover = None;
		StartSpot = None;
	}

	//-------------------------------------------------------------------------

	event BeginState()
	{
		if( Role==ROLE_Authority )
		{
			bFrozen = true;
			bBehindView = true;
			ClientSetBehindView(bBehindView);
			bFreeCamera = true;
			FindGoodView();
			FixFOV();
			WoundedTime = Level.TimeSeconds;
		}
		bFire = 0;
		bAltFire = 0;
		bFreeCamera = true;
	}

	//-------------------------------------------------------------------------

	event EndState()
	{
		if( Role==ROLE_Authority )
		{
			bFrozen = false;
			bBehindView = false;
			ClientSetBehindView(bBehindView);
			if( !ValidPawn( Pawn ) && Level.NetMode != NM_Client )
				DestroyPawn();
		}
		bFreeCamera = false;
		class'UIConsole'.static.SendEvent( "HideDeploymentText" );
}

	//-------------------------------------------------------------------------

Begin:

	Sleep( WoundedDelaySeconds );
	bFrozen = false;
	ClientSendEvent( 2 );					// case 2:  SendEvent( "CanEnterLimboOn" )

} // Wounded

//-----------------------------------------------------------------------------

function SetDeployPointView();
function ClientSetPreMatchTime( float NewTime );

function ClientSetNextDeployTime( float NewTime )
{
	NextDeployTime = Level.TimeSeconds + NewTime;
}

//-----------------------------------------------------------------------------

state WaitingToDeploy extends CantMove
{
	ignores SeeEnemy, HearNoise, KilledBy, SwitchWeapon, NextWeapon, PrevWeapon;

	//-------------------------------------------------------------------------

	exec function Use();
	exec function UnUse();

	//-------------------------------------------------------------------------

	exec function Fire( optional float F )
	{
		ServerSetDeployPoint();
	}

	//-------------------------------------------------------------------------

	function ServerSetDeployPoint()
	{
		XMPGame( Level.Game ).LockInCurrentDeployPoint( Self );
		ClientSendEvent( 5 );			// case 5:  SendEvent( "ShowDeployPointOn" ) SendEvent( "SetDeployPointOff" )
	}

	//-------------------------------------------------------------------------
	
	function ServerAltFire( float F )
	{
		if( ServerDeployCheat() )
			XMPGame(Level.Game).ForceDeploy( Self );
		else
			Fire( F );
	}
	
	//-------------------------------------------------------------------------
	
	exec function AltFire(optional float F)
	{
		ServerAltFire( F );
	}

	//-------------------------------------------------------------------------
	// cycles through the deployment points

	exec function Jump( optional float F )
	{
		ServerViewNextDeployPoint();
	}

	//-------------------------------------------------------------------------
	// allows player a behind view of the current deployment point

	function ServerViewNextDeployPoint()
	{
		RespawnPoint = XMPGame( Level.Game ).GetNextDeploymentPoint( RespawnPoint, Self );
		SetDeployPointView();
	}

	//-------------------------------------------------------------------------

	function SetDeployPointView()
	{
		if( RespawnPoint != None )
		{
			if( RespawnPoint == PlayerReplicationInfo.RespawnPoint && PlayerReplicationInfo.bLockedIn )
			{
				SetViewTarget( RespawnPoint );
				bBehindView = true;
				ClientSendEvent( 4 );			// case 4:  SendEvent( "ShowDeployPointOn" )
			}
			else
			{
				SetViewTarget( RespawnPoint.Camera );
				bBehindView = (RespawnPoint.Camera == RespawnPoint);
				ClientSendEvent( 3 );			// case 3:  SendEvent( "ShowDeployPointOff" )
			}

			ClientSetViewTarget(ViewTarget);
			ClientSetBehindView(bBehindView);
			if( bBehindView )
				FindGoodView();
			FixFOV();

			if( Level.NetMode != NM_Client )
				ClientSetNextDeployTime( RespawnPoint.NextRespawnTime - Level.TimeSeconds );
		}

	}

	//-------------------------------------------------------------------------

	function NotifyDeployPointHacked()
	{
		ClientSendEvent( 6 );		// SendEvents: "SetDeployPointOn" "ShowDeployPointOff"
		ServerViewNextDeployPoint();
	}

	//-------------------------------------------------------------------------

	simulated event BeginState()
	{
		// let the server put us back at a spawn point
		if( Level.NetMode != NM_Client )
		{
			bFreeCamera = true;
			PlayerReplicationInfo.bWaitingToRespawn = true;
			RespawnPoint = DeploymentPoint(PlayerReplicationInfo.RespawnPoint); // where player last spawned
			if( RespawnPoint? && RespawnPoint.GetTeam() == GetTeam() )
				SetDeployPointView();
			else
				ServerViewNextDeployPoint();
		}

		// Hud stuff that does not need to run on dedicated servers
		if( Level.NetMode != NM_DedicatedServer )
		{
			if( Viewport(Player)? )
			{
				class'UIConsole'.static.SendEvent( "ShowXMPMenu" );
				class'UIConsole'.static.SendEvent( "ShowDeployList" );
				class'UIConsole'.static.SendEvent( "SetDeployPointOn" );
				class'UIConsole'.static.SendEvent( "ShowDeployPointOff" );
				class'UIConsole'.static.SendEvent( "SwitchDeployPointOn" );
				class'UIConsole'.static.SendEvent( "TimeToDeployOn" );
				if( ServerDeployCheat() )
					class'UIConsole'.static.SendEvent( "SpawnNowCheatOn" );
			}
		}
	}

	//-------------------------------------------------------------------------

	simulated event EndState()
	{
		bBehindView = false;
		bFreeCamera = false;
		PlayerReplicationInfo.bWaitingToRespawn = false;

		if( Level.NetMode != NM_DedicatedServer )
		{
			if( Viewport(Player)? )
			{
				class'UIConsole'.static.SendEvent( "HideXMPMenu" );
				class'UIConsole'.static.SendEvent( "HideDeployList" );
				class'UIConsole'.static.SendEvent( "HideDeploymentText" );
				class'UIConsole'.static.SendEvent( "HideDeploymentTimer" );
			}
		}
	}

	//-------------------------------------------------------------------------

} // WaitingToDeploy

//-----------------------------------------------------------------------------

auto state PlayerWaiting
{
ignores SeeEnemy, HearNoise, NotifyBump, TakeDamage, PhysicsVolumeChange, NextWeapon, PrevWeapon, SwitchToBestWeapon;

	exec function Use();
	exec function UnUse();

	function bool CanRestartPlayer()
	{
		return false;
	}

	exec function Fire( optional float F )
	{
		DoRestartPlayer();
	}

	exec function AltFire( optional float F )
	{
		Fire(F);
	}

	simulated function ServerRestartPlayer()
	{
		DoRestartPlayer();
	}

	function bool DoRestartPlayer()
	{
		if( Level.NetMode == NM_Client )
		{
			ServerRestartPlayer();
			return true;
		}

		if ( Level.TimeSeconds < WaitDelay )
			return false;
		if ( Level.NetMode == NM_Client )
			return false;

		GotoState( 'WaitingToDeploy' );
		return true;
	}

	function ClientSetPreMatchTime( float NewTime )
	{
		WaitDelay = Level.TimeSeconds + NewTime;
		if( NewTime > 0 )		// player must wait until others have joined too
		{
			class'UIConsole'.static.SendEvent( "StartMatchCountdownOn" );
			AddTimer( 'PreMatchTimeExpired', NewTime );
		}
		else	// enough pre-game time has passed, player can join immediately
		{
			class'UIConsole'.static.SendEvent( "FireToJoinOn" );
		}
	}

	function PreMatchTimeExpired()
	{
		// hide the pre match time
		class'UIConsole'.static.SendEvent( "StartMatchCountdownOff" );
		class'UIConsole'.static.SendEvent( "FireToJoinOn" );
	}

	function Init()
	{
		if( Level.NetMode == NM_StandAlone )
		{
			GotoState( 'WaitingToDeploy' );
			return;
		}

		if( Role == ROLE_Authority )
		{
			WaitDelay = XMPGame(Level.Game).PreMatchWait;
			ClientSetPreMatchTime( XMPGame(Level.Game).PreMatchWait - Level.TimeSeconds );
		}
	}

	simulated function BeginState()	// simulated because role isn't set yet.
	{
		class'UIConsole'.static.SendEvent( "ShowXMPMenu" );
		Super.BeginState();
	}

	simulated function EndState()
	{
		class'UIConsole'.static.SendEvent( "HideXMPMenu" );
		class'UIConsole'.static.SendEvent( "StartMatchCountdownOff" );
		class'UIConsole'.static.SendEvent( "FireToJoinOff" );
		RemoveTimer('SendPlayerToReDeploy');
		Super.EndState();
	}

Begin:
	Init();

} // PlayerWaiting

//-----------------------------------------------------------------------------

defaultproperties
{
	UIAlpha=255
	bNetNotify=true;						// get PostNetReceive messages
	WoundedDelaySeconds=2.0
	CheatClass=class'U2.U2CheatManager'
	MessageFont=font'HUD_Fonts.EuroExt14'
	SaySound=sound'U2A.Messages.Say'
	TeamSaySound=sound'U2A.Messages.TeamSay'
	RocketLockSound=sound'WeaponsA.RocketLauncher.RL_Alert'
	UseProjectorMaterial=Material'SpecialFX.Use.use_reticle_001'
	UseHighlightMaterial=Material'SpecialFX.Use.use_reticle_002'
	VM_MedicMsg=class'VM_Medic'
	VM_RepairsMsg=class'VM_Repairs'
	VM_SuppliesMsg=class'VM_Supplies'
	VM_HackerMsg=class'VM_Hacker'
	VM_OffenseMsg=class'VM_Offense'
	VM_BackupMsg=class'VM_Backup'
	VM_DefenseMsg=class'VM_Defense'
	VM_FollowMsg=class'VM_Follow'
	VM_BaseMsg=class'VM_Base'
	VM_GeneratorMsg=class'VM_Generator'
	VM_ArtifactMsg=class'VM_Artifact'
	VM_AllClearMsg=class'VM_AllClear'
	VM_ThanksMsg=class'VM_Thanks'
	VM_SorryMsg=class'VM_Sorry'
	VM_NoProblemMsg=class'VM_NoProblem'
	VM_AcknowledgedMsg=class'VM_Acknowledged'
}
