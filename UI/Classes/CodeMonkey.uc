//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/02 1:48p $
// $Revision: 85 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	CodeMonkey.uc
// Author:	Aaron R Leiby
// Date:	3 January 2001
//------------------------------------------------------------------------------
// Description:	Used to store various needed UnrealScript function interfaces for the UI.
//------------------------------------------------------------------------------
// Notes:
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class CodeMonkey extends UIHelper;

var localized string ApplySettingsText;

var string EngineVersion;

var string PendingRes;
var string PendingBPP;
var bool bPendingDetail;

var bool bNoShadowUpdate;
var bool bPendingUpdateShadow;
var bool bPendingSuperUpdateShadow;

var bool bNoResetMenus;

var bool bLastHUDState;
var bool bLockHUDState;

const PauseKey = 314;
const Pause_withDrawKey = 278;
const PauseKey_Objectives = 777;


//
// Map support.
//

var() array<string> GameTypes;
var() string SelectedGameType;

var array<string> MapList;
var string SelectedMap;

var() bool bUseAllMaps;

function interface string GameRules()				{ return "Atari QA Build #1: INTERNAL USE ONLY - Do Not Distribute"; }

function interface string GetEngineVersion()		{ if (EngineVersion == "") EngineVersion="U2XMP v." $ GetPlayerOwner().Level.EngineVersion $ "["$GetPlayerOwner().GetEntryLevel().Game.Version$"]"; return EngineVersion; }

function interface NoDrawWorld()					{ GetConsole().bNoDrawWorld=true; }
function interface DrawWorld()						{ GetConsole().bNoDrawWorld=false; }

function interface Pause()							{ GetPlayerOwner().SetPause(true,PauseKey); GetConsole().bNoDrawWorld=true; }
function interface UnPause()						{ GetPlayerOwner().SetPause(false,PauseKey); GetConsole().bNoDrawWorld=false; }

function interface Pause_withDraw()					{ GetPlayerOwner().SetPause(true,Pause_withDrawKey); }
function interface UnPause_withDraw()				{ GetPlayerOwner().SetPause(false,Pause_withDrawKey); }

function interface Pause_Objectives()				{ GetPlayerOwner().SetPause(true,PauseKey_Objectives); }
function interface UnPause_Objectives()				{ GetPlayerOwner().SetPause(false,PauseKey_Objectives); }

function interface HideHUD()						{ if(bLockHUDState) return; bLockHUDState = true; bLastHUDState = GetPlayerOwner().Level.Game.bDisplayHud; GetPlayerOwner().Level.Game.bDisplayHud = false; }
function interface RestoreHUD()						{ bLockHUDState = false; GetPlayerOwner().Level.Game.bDisplayHud = bLastHUDState; }

/*!!MERGE
function interface EndGame_LoadMenu()				{ class'UIConsole'.static.SendEvent("ShowMenus"); class'UIConsole'.static.SendEvent("ShowMenuLoadGame"); }
function interface EndGame_QuickLoad()				{ if(GetPlayerOwner().IsInState(U2PlayerController.DeadState)) class'MusicScriptEvent'.static.CallRoutine(GetPlayerOwner(),"",""); QuickLoad(); }
function interface EndGame_RestartLevel()			{ GetPlayerOwner().ConsoleCommand("restartlevel"); }
*/
function interface SetAirControl(float F)			{ GetPlayerOwner().Pawn.AirControl = F; }
function interface float GetAirControl()			{ return GetPlayerOwner().Pawn.AirControl; }

function interface SetBehindView(bool B)			{ GetPlayerOwner().BehindView(B); }
function interface bool GetBehindView()				{ return GetPlayerOwner().bBehindView; }

//OPTIONS::PREFERENCES::GAME
/*!!MERGE
function interface SetViewBob(float F)				{ GetPlayerOwner().Bob=F; GetPlayerOwner().SaveConfig(); }
function interface float GetViewBob()				{ return GetPlayerOwner().Bob; }

function interface SetGameSpeed(float F)			{ GetPlayerOwner().Level.Game.SetGameSpeed(F); GetPlayerOwner().Level.Game.SaveConfig(); }
function interface float GetGameSpeed()				{ return GetPlayerOwner().Level.Game.GetGameSpeed(); }
*/
function interface SetDodging(float F)				{ GetPlayerOwner().PlayerInput.DoubleClickTime=F; GetPlayerOwner().PlayerInput.SaveConfig(); }
function interface float GetDodging()				{ return GetPlayerOwner().PlayerInput.DoubleClickTime; }

function interface SetDuckToggle(bool B)			{} //!!MERGE { local U2PlayerNetTestController C; C=U2PlayerNetTestController(GetPlayerOwner()); C.bUseDuckToggles=B; C.SaveConfig(); }
function interface bool GetDuckToggle()				{ return false; } //!!MERGE { local U2PlayerNetTestController C; C=U2PlayerNetTestController(GetPlayerOwner()); return C.bUseDuckToggles; }

function interface SetRunToggle(bool B)				{} //!!MERGE { local U2PlayerNetTestController C; C=U2PlayerNetTestController(GetPlayerOwner()); C.bUseRunToggles=B; C.SaveConfig(); }
function interface bool GetRunToggle()				{ return false; } //!!MERGE { local U2PlayerNetTestController C; C=U2PlayerNetTestController(GetPlayerOwner()); return C.bUseRunToggles; }

function interface SetLeanToggle(bool B)			{} //!!MERGE { local U2PlayerNetTestController C; C=U2PlayerNetTestController(GetPlayerOwner()); C.bUseLeanToggles=B; C.SaveConfig(); }
function interface bool GetLeanToggle()				{ return false; } //!!MERGE { local U2PlayerNetTestController C; C=U2PlayerNetTestController(GetPlayerOwner()); return C.bUseLeanToggles; }

function interface SetSwitchOnPickup(bool B)		{ GetPlayerOwner().bNeverSwitchOnPickup=!B; GetPlayerOwner().SaveConfig(); }
function interface bool GetSwitchOnPickup()			{ return !GetPlayerOwner().bNeverSwitchOnPickup; }

function interface SetAutoSwitch(bool B)			{ GetPlayerOwner().bAutoSwitch=B; GetPlayerOwner().SaveConfig(); }
function interface bool GetAutoSwitch()				{ return GetPlayerOwner().bAutoSwitch; }

function interface SetBroadcastObjectives(bool B)	{} //!!MERGE { U2GameInfo(GetPlayerOwner().Level.Game).bBroadcastObjectives=B; GetPlayerOwner().Level.Game.SaveConfig(); }
function interface bool GetBroadcastObjectives()	{ return false; } //!!MERGE { return U2GameInfo(GetPlayerOwner().Level.Game).bBroadcastObjectives; }

// Dialog stuff
function interface SetShowSubtitles(bool B)			{} //!!MERGE { GetPlayerOwner().ShowSubtitles = B; GetPlayerOwner().SaveConfig(); }
function interface bool GetShowSubtitles()			{ return false; } //!!MERGE { return GetPlayerOwner().ShowSubtitles; }

//OPTIONS::PREFERENCES::VIDEO
function interface array<string> GetDisplayModes()			{ local string S; local array<string> DisplayModes; S=GetConsole().ConsoleCommandEx("DISPLAYMODES"); StrArray(S," ",DisplayModes,true); return DisplayModes; }
function interface SetRes(string Res)						{ PendingRes=Res; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }	//{ GetPlayerOwner().ConsoleCommand("SETRES"@Res); }
function interface string GetRes()							{ local Dimension D; if(PendingRes?) return PendingRes; D=class'UIConsole'.static.GetComponentSize(GetUIConsole().Root); return int(D.Width)$"x"$int(D.Height); }

function interface SetBPP32()								{ PendingBPP="32"; class'UIConsole'.static.SendEvent("FlashVideoSettings"); } //{ GetPlayerOwner().ConsoleCommand("SETRES"@GetRes()$"x32"); }
function interface SetBPP16()								{ PendingBPP="16"; class'UIConsole'.static.SendEvent("FlashVideoSettings"); } //{ GetPlayerOwner().ConsoleCommand("SETRES"@GetRes()$"x16"); }
function interface int GetBPPIndex()						{ local string BPP; if(PendingBPP?) BPP=PendingBPP; else BPP=GetPlayerOwner().ConsoleCommand("GETCURRENTCOLORDEPTH"); switch(BPP){ case "32": return 0; case "16": return 1; }; }

function interface SetTextureDetailWorldUltraHigh()			{ GetClient().TextureDetailWorld=UltraHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldVeryHigh()			{ GetClient().TextureDetailWorld=VeryHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldHigh()				{ GetClient().TextureDetailWorld=High;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldHigher()			{ GetClient().TextureDetailWorld=Higher;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldNormal()			{ GetClient().TextureDetailWorld=Normal;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldLower()				{ GetClient().TextureDetailWorld=Lower;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldLow()				{ GetClient().TextureDetailWorld=Low;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldVeryLow()			{ GetClient().TextureDetailWorld=VeryLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWorldUltraLow()			{ GetClient().TextureDetailWorld=UltraLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface int GetTextureDetailWorldIndex()			{ return GetClient().TextureDetailWorld; }

function interface SetTextureDetailPlayerSkinUltraHigh()	{ GetClient().TextureDetailPlayerSkin=UltraHigh;	GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinVeryHigh()		{ GetClient().TextureDetailPlayerSkin=VeryHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinHigh()			{ GetClient().TextureDetailPlayerSkin=High;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinHigher()		{ GetClient().TextureDetailPlayerSkin=Higher;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinNormal()		{ GetClient().TextureDetailPlayerSkin=Normal;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinLower()		{ GetClient().TextureDetailPlayerSkin=Lower;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinLow()			{ GetClient().TextureDetailPlayerSkin=Low;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinVeryLow()		{ GetClient().TextureDetailPlayerSkin=VeryLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailPlayerSkinUltraLow()		{ GetClient().TextureDetailPlayerSkin=UltraLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface int GetTextureDetailPlayerSkinIndex()	{ return GetClient().TextureDetailPlayerSkin; }

function interface SetTextureDetailWeaponSkinUltraHigh()	{ GetClient().TextureDetailWeaponSkin=UltraHigh;	GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinVeryHigh()		{ GetClient().TextureDetailWeaponSkin=VeryHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinHigh()			{ GetClient().TextureDetailWeaponSkin=High;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinHigher()		{ GetClient().TextureDetailWeaponSkin=Higher;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinNormal()		{ GetClient().TextureDetailWeaponSkin=Normal;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinLower()		{ GetClient().TextureDetailWeaponSkin=Lower;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinLow()			{ GetClient().TextureDetailWeaponSkin=Low;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinVeryLow()		{ GetClient().TextureDetailWeaponSkin=VeryLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailWeaponSkinUltraLow()		{ GetClient().TextureDetailWeaponSkin=UltraLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface int GetTextureDetailWeaponSkinIndex()	{ return GetClient().TextureDetailWeaponSkin; }

function interface SetTextureDetailTerrainUltraHigh()		{ GetClient().TextureDetailTerrain=UltraHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainVeryHigh()		{ GetClient().TextureDetailTerrain=VeryHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainHigh()			{ GetClient().TextureDetailTerrain=High;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainHigher()			{ GetClient().TextureDetailTerrain=Higher;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainNormal()			{ GetClient().TextureDetailTerrain=Normal;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainLower()			{ GetClient().TextureDetailTerrain=Lower;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainLow()				{ GetClient().TextureDetailTerrain=Low;				GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainVeryLow()			{ GetClient().TextureDetailTerrain=VeryLow;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailTerrainUltraLow()		{ GetClient().TextureDetailTerrain=UltraLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface int GetTextureDetailTerrainIndex()		{ return GetClient().TextureDetailTerrain; }

function interface SetTextureDetailLightmapUltraHigh()		{ GetClient().TextureDetailLightmap=UltraHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapVeryHigh()		{ GetClient().TextureDetailLightmap=VeryHigh;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapHigh()			{ GetClient().TextureDetailLightmap=High;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapHigher()			{ GetClient().TextureDetailLightmap=Higher;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapNormal()			{ GetClient().TextureDetailLightmap=Normal;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapLower()			{ GetClient().TextureDetailLightmap=Lower;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapLow()			{ GetClient().TextureDetailLightmap=Low;			GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapVeryLow()		{ GetClient().TextureDetailLightmap=VeryLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface SetTextureDetailLightmapUltraLow()		{ GetClient().TextureDetailLightmap=UltraLow;		GetClient().SaveConfig(); bPendingDetail=true; class'UIConsole'.static.SendEvent("FlashVideoSettings"); }
function interface int GetTextureDetailLightmapIndex()		{ return GetClient().TextureDetailLightmap; }
	
function interface color GetApplyColor()					{ if( PendingRes? || PendingBPP? || bPendingDetail ) return GetPlayerOwner().ColorRed(); else return GetPlayerOwner().ColorWhite(); }

function interface ApplyVideoSettings()
{
	if( PendingRes? || PendingBPP? || bPendingDetail )
	{
		GetPlayerOwner().ConsoleCommand("FLUSH");
//		class'UIConsole'.static.SendEvent("ResetMenus");
		class'Console'.static.ShowStatusMessage(GetPlayerOwner(),ApplySettingsText,0.5,,,,,,"U2MenusTimedHolder");
		if(PendingRes? && PendingBPP?) GetPlayerOwner().ConsoleCommand("SETRES"@PendingRes$"x"$PendingBPP);
		if(PendingRes? && !PendingBPP) GetPlayerOwner().ConsoleCommand("SETRES"@PendingRes);
		if(!PendingRes && PendingBPP?) GetPlayerOwner().ConsoleCommand("SETRES"@GetRes()$"x"$PendingBPP);
//		if(!PendingRes && !PendingBPP) GetPlayerOwner().ConsoleCommand("SETRES"@GetRes());
		PendingRes="";
		PendingBPP="";
		bPendingDetail=false;
	}
}

function interface SetBrightness(float F)				{ GetPlayerOwner().ConsoleCommand("brightness"@F); }
function interface float GetBrightness()				{ return GetClient().Brightness; }

function interface SetContrast(float F)					{ GetPlayerOwner().ConsoleCommand("contrast"@F); }
function interface float GetContrast()					{ return GetClient().Contrast; }

function interface SetGamma(float F)					{ GetPlayerOwner().ConsoleCommand("gamma"@F); }
function interface float GetGamma()						{ return GetClient().Gamma; }

function interface SetFullscreen(bool B)				{ class'UIConsole'.static.SendEvent("ResetMenus"); GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.ViewportManager StartupFullscreen"@B); GetPlayerOwner().ConsoleCommand("TOGGLEFULLSCREEN"); }
function interface bool GetFullscreen()					{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.ViewportManager StartupFullscreen")); }

function interface SetDecals(bool B)					{ GetClient().Decals=B; GetClient().SaveConfig(); }
function interface bool GetDecals()						{ return GetClient().Decals; }

function interface SetCrosshairGlows(bool B)			{} //!!MERGE { GetPlayerOwner().ConsoleCommand("set U2Weapon UseGlowingCrosshairs"@B); class'U2Weapon'.static.StaticSaveConfig(); }
function interface bool GetCrosshairGlows()				{ return false; } //!!MERGE { return bool(GetPlayerOwner().ConsoleCommand("get U2Weapon UseGlowingCrosshairs")); }

// Is this the same as sensitivity?
function interface SetMouseSpeed(float F)				{ GetClient().MouseSpeed=F; GetClient().SaveConfig(); }
function interface float GetMouseSpeed()				{ return GetClient().MouseSpeed; }

function interface SetMinFramerate(string S)			{ GetClient().MinDesiredFrameRate=int(S); GetClient().SaveConfig(); }
function interface string GetMinFramerate()				{ return string(int(GetClient().MinDesiredFrameRate))$"fps"; }

function interface SetParticleDensity(float F)			{ GetClient().ParticleDensity=F; GetClient().SaveConfig(); }
function interface float GetParticleDensity()			{ return GetClient().ParticleDensity; }

function interface SetDecoLayerDensity(float F)			{ GetPlayerOwner().ConsoleCommand("set TerrainInfo DecoLayerDensity"@F); class'TerrainInfo'.static.StaticSaveConfig(); }
function interface float GetDecoLayerDensity()			{ return float(GetPlayerOwner().ConsoleCommand("get TerrainInfo DecoLayerDensity")); }

//OPTIONS::PREFERENCES::D3D

function interface SetUsePrecaching(bool B)				{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice UsePrecaching"@B); }
function interface bool GetUsePrecaching()				{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice UsePrecaching")); }

function interface SetUseTrilinear(bool B)				{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice UseTrilinear"@B); }
function interface bool GetUseTrilinear()				{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice UseTrilinear")); }

function interface SetUseVSync(bool B)					{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice UseVSync"@B); }
function interface bool GetUseVSync()					{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice UseVSync")); }

function interface SetUseTripleBuffering(bool B)		{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice UseTripleBuffering"@B); }
function interface bool GetUseTripleBuffering()			{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice UseTripleBuffering")); }

function interface SetUseCubemaps(bool B)				{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice UseCubemaps"@B); }
function interface bool GetUseCubemaps()				{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice UseCubemaps")); }

function interface SetReduceMouseLag(bool B)			{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice ReduceMouseLag"@B); }
function interface bool GetReduceMouseLag()				{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice ReduceMouseLag")); }

function interface SetRefreshRate85()					{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice DesiredRefreshRate"@85); }
function interface SetRefreshRate75()					{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice DesiredRefreshRate"@75); }
function interface SetRefreshRate72()					{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice DesiredRefreshRate"@72); }
function interface SetRefreshRate70()					{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice DesiredRefreshRate"@70); }
function interface SetRefreshRate60()					{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice DesiredRefreshRate"@60); }
function interface int GetRefreshRate()					{ switch(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice DesiredRefreshRate")){ case "85": return 0; case "75": return 1; case "72": return 2; case "70": return 3; case "60": return 4; default: return 4; } }

//OPTIONS::PREFERENCES::AUDIO
function interface SetMusicVolume(float F)				{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.AudioDevice MusicVolume"@F); }
/*!!MERGE
	if(F?)	class'MusicScriptEvent'.static.SetMasterVolume(Blend(-1500,100,F));
	else	class'MusicScriptEvent'.static.SetMasterVolume(-10000); }
*/
function interface float GetMusicVolume()				{ return float(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.AudioDevice MusicVolume")); }

function interface SoundReboot()
{
	GetPlayerOwner().ConsoleCommand("sound_reboot");

	// Restart music if necessary.
	if( GetPlayerOwner().Level.Song? )
		GetPlayerOwner().ClientSetMusic( GetPlayerOwner().Level.Song, MTRAN_Fade );
}

function interface int GetAudioChannelsUsed()			{ return int(GetConsole().ConsoleCommandEx("AUDIOCHANNELSUSED")); }
function interface int GetAudioChannelsTotal()			{ return int(GetConsole().ConsoleCommandEx("AUDIOCHANNELSTOTAL")); }
function interface float GetAudioChannelsPct()			{ return float(GetAudioChannelsUsed()) / float(GetAudioChannelsTotal()); }
function interface float GetAudioChannelsUnPct()		{ return 1.0 - GetAudioChannelsPct(); }
function interface string GetAudioStat()				{ return "[" $ GetAudioChannelsUsed() $ "/" $ GetAudioChannelsTotal() $ "]"; }

function interface SetSoundVolume(float F)				{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.AudioDevice SoundVolume"@F); }
function interface float GetSoundVolume()				{ return float(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.AudioDevice SoundVolume")); }

function interface SetReverseStereo(bool B)				{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.AudioDevice ReverseStereo"@B); }
function interface bool GetReverseStereo()				{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.AudioDevice ReverseStereo")); }

function interface SetAudioChannels(float F)			{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.AudioDevice Channels"@byte(F)); SoundReboot(); }
function interface float GetAudioChannels()				{ return float(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.AudioDevice Channels")); }

function interface SetUseEAX(bool B)					{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.AudioDevice UseEAX"@B); SoundReboot(); }
function interface bool GetUseEAX()						{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.AudioDevice UseEAX")); }

function interface SetUse3DSound(bool B)				{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.AudioDevice Use3DSound"@B); SoundReboot(); }
function interface bool GetUse3DSound()					{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.AudioDevice Use3DSound")); }

function interface SetDetailTextures(bool B)			{ GetPlayerOwner().ConsoleCommand("set ini:Engine.Engine.RenderDevice DetailTextures"@B); }
function interface bool GetDetailTextures()				{ return bool(GetPlayerOwner().ConsoleCommand("get ini:Engine.Engine.RenderDevice DetailTextures")); }
/*!!MERGE
function interface int GetDifficulty()					{ return GetPlayerOwner().Level.Game.GetDifficulty(); }
function interface string GetDifficultyText()			{ switch(GetDifficulty()){ case 0: return "Easy"; case 1: return "Normal"; case 2: return "Hard"; } }

function interface SetDifficulty0()						{ GetPlayerOwner().Level.Game.SetDifficulty( 0 ); GetPlayerOwner().Level.Game.SaveConfig(); }
function interface SetDifficulty1()						{ GetPlayerOwner().Level.Game.SetDifficulty( 1 ); GetPlayerOwner().Level.Game.SaveConfig(); }
function interface SetDifficulty2()						{ GetPlayerOwner().Level.Game.SetDifficulty( 2 ); GetPlayerOwner().Level.Game.SaveConfig(); }
function interface int GetDifficultyIndex()				{ return (2 - GetPlayerOwner().Level.Game.GetDifficulty()); }

function interface SetActorDetail0()					{ GetClient().MaxDetailLevel=0; GetClient().SaveConfig(); }
function interface SetActorDetail1()					{ GetClient().MaxDetailLevel=1; GetClient().SaveConfig(); }
function interface SetActorDetail2()					{ GetClient().MaxDetailLevel=2; GetClient().SaveConfig(); }
function interface int GetActorDetailIndex()			{ return (2 - GetClient().MaxDetailLevel); }

function interface SetRagdoll0()						{ U2GameInfo(GetPlayerOwner().Level.Game).RagdollDetailLevel=0; GetPlayerOwner().Level.Game.SaveConfig(); }
function interface SetRagdoll1()						{ U2GameInfo(GetPlayerOwner().Level.Game).RagdollDetailLevel=1; GetPlayerOwner().Level.Game.SaveConfig(); }
function interface SetRagdoll2()						{ U2GameInfo(GetPlayerOwner().Level.Game).RagdollDetailLevel=2; GetPlayerOwner().Level.Game.SaveConfig(); }
function interface SetRagdoll3()						{ U2GameInfo(GetPlayerOwner().Level.Game).RagdollDetailLevel=3; GetPlayerOwner().Level.Game.SaveConfig(); }
function interface int GetRagdollIndex()				{ return (3 - U2GameInfo(GetPlayerOwner().Level.Game).RagdollDetailLevel); }

function interface SetFluidSurfaceHigh()				{ GetClient().FluidSurfaceDetail=1; GetClient().SaveConfig(); }
function interface SetFluidSurfaceLow()					{ GetClient().FluidSurfaceDetail=0; GetClient().SaveConfig(); }
function interface int GetFluidSurfaceDetail()			{ return (1 - GetClient().FluidSurfaceDetail); }

function interface SetGoreLevel0()						{ GetClient().GoreDetailLevel=0; GetClient().SaveConfig(); AdjustGore(); }
function interface SetGoreLevel1()						{ GetClient().GoreDetailLevel=1; GetClient().SaveConfig(); AdjustGore(); }
function interface SetGoreLevel2()						{ GetClient().GoreDetailLevel=2; GetClient().SaveConfig(); AdjustGore(); }
function interface SetGoreLevel3()						{ GetClient().GoreDetailLevel=3; GetClient().SaveConfig(); AdjustGore(); }
function interface int GetGoreLevelIndex()				{ return (3 - GetClient().GoreDetailLevel); }
function interface bool GetGoreLock()					{ return !GetPlayerOwner().Level.Game.bAllowGore; }
function interface  AdjustGore()
{
	local Actor A;
	local byte GoreDetaillevel;
	GoreDetailLevel = GetClient().GoreDetailLevel;
	foreach GetPlayerOwner().DynamicActors( class'Actor', A )
		A.NotifyGoreDetailChanged( GoreDetailLevel );
}
*/

/*!!MERGE
function interface SetMasterShadowDetail0()			{ SetMasterShadowDetail(0); }
function interface SetMasterShadowDetail1()			{ SetMasterShadowDetail(1); }
function interface SetMasterShadowDetail2()			{ SetMasterShadowDetail(2); }
function interface SetMasterShadowDetail3()			{ SetMasterShadowDetail(3); }
function interface SetMasterShadowDetailX()			{ CustomShadows(); }
function interface SetMasterShadowDetail(int Level)
{
	ShadowUpdateDisable();
	switch(Level)
	{
	case 3:		//HIGH
		if(GetEnableShadows()==false)				// Shadows: enabled
			SetEnableShadows(true);
		if(GetBlobShadows()==true)					// BlobShadows: false
			SetBlobShadows(false);					
		if(GetClient().ShadowResolution!=256)		// ShadowResolution: 256 [High]
			SetShadowResHigh();
		if(GetLightSourceShadows()==false)			// LightSourced: true
			SetLightSourceShadows(true);
//		if(GetWeaponShadows()==false)				// Weapon shadows: enabled
//			SetWeaponShadows(true);
		if(GetMaxVisibleShadows()!=3)				// MaxVisibleShadows: 3
			SetMaxVisibleShadows(3);
		if(GetShadowCullDistance()!=1024)			// ShadowCullDistance: 1024
			SetShadowCullDistance(1024);
		if(GetShadowTraceDistance()!=512)			// ShadowTraceDistance: 512
			SetShadowTraceDistance(512);
		break;
	case 2:		//MEDIUM
		if(GetEnableShadows()==false)				// Shadows: enabled
			SetEnableShadows(true);					
		if(GetBlobShadows()==true)					// BlobShadows: false
			SetBlobShadows(false);					
		if(GetClient().ShadowResolution!=128)		// ShadowResolution: 128 [Medium]
			SetShadowResMedium();					
		if(GetLightSourceShadows()==true)			// LightSourced: false
			SetLightSourceShadows(false);			
//		if(GetWeaponShadows()==true)				// Weapon shadows: disabled
//			SetWeaponShadows(false);
		if(GetMaxVisibleShadows()!=4)				// MaxVisibleShadows: 4
			SetMaxVisibleShadows(4);
		if(GetShadowCullDistance()!=1024)			// ShadowCullDistance: 1024
			SetShadowCullDistance(1024);
		if(GetShadowTraceDistance()!=512)			// ShadowTraceDistance: 512
			SetShadowTraceDistance(512);
		break;
	case 1:		//LOW
		if(GetEnableShadows()==false)				// Shadows: enabled
			SetEnableShadows(true);					
		if(GetBlobShadows()==false)					// BlobShadows: true
			SetBlobShadows(true);					
		if(GetClient().ShadowResolution!=64)		// ShadowResolution: 64 [Low]
			SetShadowResLow();					
		if(GetLightSourceShadows()==false)			// LightSourced: true
			SetLightSourceShadows(true);			
//		if(GetWeaponShadows()==true)				// Weapon shadows: disabled
//			SetWeaponShadows(false);
		if(GetMaxVisibleShadows()!=0)				// MaxVisibleShadows: 0
			SetMaxVisibleShadows(0);
		if(GetShadowCullDistance()!=1024)			// ShadowCullDistance: 1024
			SetShadowCullDistance(1024);
		if(GetShadowTraceDistance()!=512)			// ShadowTraceDistance: 512
			SetShadowTraceDistance(512);
		break;
	case 0:		//OFF
		if(GetEnableShadows()==true)				// Shadows: disabled
			SetEnableShadows(false);					
//		if(GetBlobShadows()==false)					// BlobShadows: true
//			SetBlobShadows(true);					
//		if(GetClient().ShadowResolution!=64)		// ShadowResolution: 64 [Low]
//			SetShadowResLow();					
//		if(GetLightSourceShadows()==true)			// LightSourced: false
//			SetLightSourceShadows(false);			
//		if(GetWeaponShadows==true)					// Weapon shadows: disabled
//			SetWeaponShadows(false);
		if(GetMaxVisibleShadows()!=0)				// MaxVisibleShadows: 0
			SetMaxVisibleShadows(0);
//		if(GetShadowCullDistance()!=1024)			// ShadowCullDistance: 1024
//			SetShadowCullDistance(1024);
//		if(GetShadowTraceDistance()!=512)			// ShadowTraceDistance: 512
//			SetShadowTraceDistance(512);
		break;
	};
	ShadowUpdateEnable();
	GetClient().MasterShadowDetail=Level;
	GetClient().SaveConfig();
}
function interface int GetMasterShadowDetailIndex()
{
	switch(GetClient().MasterShadowDetail)
	{
	case 3: return 0;
	case 2: return 1;
	case 1: return 2;
	case 0: return 3;
	default: return 4;
	};
}

function interface CustomShadows()						{ GetClient().MasterShadowDetail=255; GetClient().SaveConfig(); }

function interface SetShadowResUltraHigh()				{ GetClient().ShadowResolution=512; SuperUpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface SetShadowResHigh()					{ GetClient().ShadowResolution=256; SuperUpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface SetShadowResMedium()					{ GetClient().ShadowResolution=128; SuperUpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface SetShadowResLow()					{ GetClient().ShadowResolution=64; SuperUpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface int GetShadowResIndex()				{ switch(GetClient().ShadowResolution){ case 512: return 0; case 256: return 1; case 128: return 2; case 64: return 3; }; }

function interface SetEnableShadows(bool B)				{ GetClient().MaxShadowsStatic=byte(B); UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface bool GetEnableShadows()				{ return bool(GetClient().MaxShadowsStatic); }
*/
/*
function interface SetWeaponShadows(bool B)				{ GetClient().MaxShadowsDynamic=byte(B); UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface bool GetWeaponShadows()				{ return bool(GetClient().MaxShadowsDynamic); }
*/
/*!!MERGE
function interface SetLightSourceShadows(bool B)		{ GetClient().LightSourceShadows=B; UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface bool GetLightSourceShadows()			{ return GetClient().LightSourceShadows; }

function interface SetBlobShadows(bool B)				{ GetClient().BlobShadows=B; UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface bool GetBlobShadows()				{ return GetClient().BlobShadows; }

function interface SetBlurShadows(bool B)				{ GetClient().BlurShadows=B; UpdateShadows(); GetClient().SaveConfig(); } //CustomShadows(); }
function interface bool GetBlurShadows()				{ return GetClient().BlurShadows; }

function interface SetMaxVisibleShadows(float F)		{ GetPlayerOwner().Level.ObjectPoolPrecacheList[0].NumObjects=F; SuperUpdateShadows(); GetPlayerOwner().Level.SaveConfig(); CustomShadows(); }
function interface float GetMaxVisibleShadows()			{ return GetPlayerOwner().Level.ObjectPoolPrecacheList[0].NumObjects; }
*/
/*
function interface SetMaxShadowsStatic(float F)			{ GetClient().MaxShadowsStatic=F; UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface float GetMaxShadowsStatic()			{ return GetClient().MaxShadowsStatic; }

function interface SetMaxShadowsDynamic(float F)		{ GetClient().MaxShadowsDynamic=F; UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface float GetMaxShadowsDynamic()			{ return GetClient().MaxShadowsDynamic; }
*/
/*!!MERGE
function interface SetShadowCullDistance(float F)		{ GetClient().ShadowCullDistance=F; UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface float GetShadowCullDistance()		{ return GetClient().ShadowCullDistance; }

function interface SetShadowTraceDistance(float F)		{ GetClient().ShadowTraceDistance=F; UpdateShadows(); GetClient().SaveConfig(); CustomShadows(); }
function interface float GetShadowTraceDistance()		{ return GetClient().ShadowTraceDistance; }

function interface ShadowUpdateDisable()				{ bNoShadowUpdate=true; bPendingUpdateShadow=false; bPendingSuperUpdateShadow=false; }
function interface ShadowUpdateEnable()					{ bNoShadowUpdate=false; if(bPendingUpdateShadow) UpdateShadows(); if(bPendingSuperUpdateShadow) SuperUpdateShadows(); }
*/
/*!!MERGE
function interface UpdateShadows()
{
//	local Actor A;
//	foreach GetPlayerOwner().AllActors( class'Actor', A )
//		A.UpdateShadows();
	local Pawn P;
	if(bNoShadowUpdate){ bPendingUpdateShadow=true; return; }
	foreach GetPlayerOwner().AllActors( class'Pawn', P )
		P.ResetShadows();
}

function interface SuperUpdateShadows()
{
	local PlayerController Player;
	local Pawn P;
	local ShadowBitmapMaterial M;
	local LevelInfo Level;

	if(bNoShadowUpdate){ bPendingSuperUpdateShadow=true; return; }

	Player = GetPlayerOwner();
	Level = Player.Level;
	foreach Player.AllActors( class'Pawn', P )
	{
		if( P.ShadowA != None )
		{
			P.ShadowA.Destroy();
			P.ShadowA=None;
		}
		if( P.ShadowB != None )
		{
			P.ShadowB.Destroy();
			P.ShadowB=None;
		}
	}

	for	(	M=ShadowBitmapMaterial(Level.ObjectPool.GetObject(class'ShadowBitmapMaterial'));M?;
			M=ShadowBitmapMaterial(Level.ObjectPool.GetObject(class'ShadowBitmapMaterial')))
	{
		M.Delete();
	}

	Level.PrecacheObjects();

	foreach Player.AllActors( class'Pawn', P )
		P.ResetShadows();

	Player.ConsoleCommand("FLUSH");
}
*/

function interface SetPlayerShadows(bool B)				{ GetClient().PlayerShadows=B; if(B) EnableShadows(); else DisableShadows(); GetClient().SaveConfig(); }
function interface bool GetPlayerShadows()				{ return GetClient().PlayerShadows; }

function interface EnableShadows()
{
	local Pawn P;
	local PlayerController Player;
	Player = GetPlayerOwner();
	foreach Player.AllActors( class'Pawn', P )
		P.CreateShadow();
}

function interface DisableShadows()
{
	local Pawn P;
	local PlayerController Player;
	Player = GetPlayerOwner();
	foreach Player.AllActors( class'Pawn', P )
		P.DestroyShadow();
}

//OPTIONS::CONTROLS::INPUT

function interface SetMouseLook(bool B)					{ GetPlayerOwner().bAlwaysMouseLook=B; GetPlayerOwner().SaveConfig(); }
function interface bool GetMouseLook()					{ return GetPlayerOwner().bAlwaysMouseLook; }

function interface SetMouseSmoothing(bool B)			{ GetPlayerOwner().PlayerInput.MouseSmoothingMode=byte(B); GetPlayerOwner().PlayerInput.SaveConfig(); }
function interface bool GetMouseSmoothing()				{ return bool(GetPlayerOwner().PlayerInput.MouseSmoothingMode); }

function interface SetInvertMouse(bool B)				{ GetPlayerOwner().PlayerInput.bInvertMouse=B; GetPlayerOwner().PlayerInput.SaveConfig(); }
function interface bool GetInvertMouse()				{ return GetPlayerOwner().PlayerInput.bInvertMouse; }

function interface SetLookSpring(bool B)				{ GetPlayerOwner().bSnapToLevel=B; GetPlayerOwner().SaveConfig(); }
function interface bool GetLookSpring()					{ return GetPlayerOwner().bSnapToLevel; }

function interface SetMouseSensitivity(float F)			{ GetPlayerOwner().PlayerInput.MouseSensitivity=F; GetPlayerOwner().PlayerInput.SaveConfig(); }
function interface float GetMouseSensitivity()			{ return GetPlayerOwner().PlayerInput.MouseSensitivity; }

function interface ResetKeyboard()						{ GetPlayerOwner().ResetKeyboard(); }
/*!!MERGE
function interface float GetLevelLoadPct()				{ return GetConsole().LevelLoadingPct; }
function interface float GetLevelLoadAlpha()			{ return Blend( 255, 32, GetConsole().LevelLoadingPct ); }
*/
function interface TriggerEvent( name Event )					{ GetPlayerOwner().TriggerEvent(Event,None,None); }
function interface TriggerSpecialCreditsParticles_yellow()		{ TriggerEvent('SpecialCreditsParticles_yellow'); }
function interface TriggerSpecialCreditsParticles_blue()		{ TriggerEvent('SpecialCreditsParticles_blue'); }
function interface TriggerSpecialCreditsParticles_purple()		{ TriggerEvent('SpecialCreditsParticles_purple'); }
function interface TriggerSpecialCreditsParticles_gold()		{ TriggerEvent('SpecialCreditsParticles_gold'); }
function interface TriggerLevelChange()							{ TriggerEvent('LevelChange'); }

function interface StopMusic()
{
//!!MERGE	class'MusicScriptEvent'.static.CallRoutine(GetPlayerOwner(),"dmFadeout");
	GetPlayerOwner().StopAllMusic( 1.0 );
}

function interface PlayMusic( string Song )
{
	StopMusic();
	GetPlayerOwner().PlayMusic( Song, 0.0 );
}

function interface PlayMusic_Credits()
{
	PlayMusic("CreditsMusic.ogg");
}

function interface ResetMenus(){ if(!bNoResetMenus) class'UIConsole'.static.SendEvent("ResetMenus"); }

/*
var InternetInfo Receiver;

function interface JoinFirstServer()					{ blah(); GetPlayerOwner().ConsoleCommand("open "$Receiver.GetBeaconAddress(0)); }
function interface string GetServerAddress( int i )		{ blah(); return Receiver.GetBeaconAddress(i); }
function interface string GetServerName( int i )		{ blah(); return Receiver.GetBeaconText(i); }

function interface array<string> GetBeaconAddressList() { blah(); return Receiver.GetBeaconAddressList(); }
function interface JoinServer( string S )				{ GetPlayerOwner().ConsoleCommand("open "$S); }

function interface blah()
{
	if( Receiver==None )
		Receiver = GetPlayerOwner().Spawn( class<InternetInfo>(DynamicLoadObject("IpDrv.ClientBeaconReceiver", class'Class')) );
}
*/

function interface array<string> GetObjectives()
{
	local array<string> Objectives;
	Objectives.length=1;
	Objectives[0]="Not implemented";
	return Objectives;
/*!!MERGE
	return U2GameInfo(GetPlayerOwner().Level.Game).GetObjectives();
*/
}

//
// Map support.
//

function interface class<GameInfo> CurrentGameType()
{
	return class<GameInfo>(DynamicLoadObject(GetGameType(), class'Class'));
}

function interface array<string> GetGameTypeList()
{
	GameTypes[0]="U2.XMPGame";
/*
	local array<string> TempGames;
	local string NextGame, NextDesc;
	local int i;

	// Compile a list of all gametypes.
	GetPlayerOwner().GetNextIntDesc("GameInfo", 0, NextGame, NextDesc);
	while (NextGame != "")
	{
		TempGames[i++] = NextGame;
		GetPlayerOwner().GetNextIntDesc("GameInfo", i, NextGame, NextDesc);
	}
	GameTypes = TempGames;
*/
	return GameTypes;
}

function interface SetGameType( string GameType )
{
	SelectedGameType=GameType;
}

function interface string GetGameType()
{
	if ( Len(SelectedGameType) < 1 ) SelectedGameType = GetGameTypeList()[0];
	return SelectedGameType;
}

function interface array<string> GetMapList()
{
	local string FirstMap, NextMap, TestMap;
	local array<string> TempMaps;
	local String MapPrefix;
	local int i;

    if ( bUseAllMaps )
    	MapPrefix = "";
    else
    	MapPrefix = CurrentGameType().default.MapPrefix;

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

function interface SetUseAllMaps(bool B){ bUseAllMaps=B; }
function interface bool GetUseAllMaps(){ return bUseAllMaps; }

function interface PlayMap()
{
	GetPlayerOwner().ConsoleCommand("OPEN "$GetMap()$
		"?game="$GetGameType() );
}


defaultproperties
{
	ApplySettingsText="Applying settings..."
}

