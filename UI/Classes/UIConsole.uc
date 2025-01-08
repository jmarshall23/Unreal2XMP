//=============================================================================
// $Author: Aleiby $
// $Date: 11/03/02 9:34p $
// $Revision: 20 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	UIConsole.uc
// Author:	Aaron R Leiby
// Date:	21 September 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// Notes:
// + Our Root component will automatically get deleted when we are destroyed.
//   (This is performed in UUIConsole.cpp)
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class UIConsole extends Console
	native;

var() public config string Filename;
var() public config float ScaleX,ScaleY;
var() public config float OffsetX,OffsetY;

var() public bool bShift, bCtrl, bAlt;

var() public bool bHideUI;

var bool bRendering;

var string LastLevelAction;

var int FrameIndex;	// useful for caching things on a per-tick basis (like keyframe calculations).  updated once per frame regardless of pausing.  be wary of wrapping.

var ComponentHandle								Root;
var array<UIHelper>								Helpers;
var array<Object>								Assets;						// Assets we are using -- keep a pointer around so they don't get garbage collected.

var array<ComponentHandle>						LevelComponents;			// Level specific ui components.

var private const native noexport array<int>	MessageAreas;				//TArray<UTextArea*>

// throw components here if you want them to be destroyed at the end of the current Update.  (useful for destroying components during an Update without screwing up the parent's iteration)
var private const native noexport array<int>	Incinerator;				//TArray<UComponent*>

var private const native noexport array<int>	TickListeners;				//TArray<UComponent*>
var private const native noexport array<int>	HelperTickListeners;		//TArray<UUIHelper*>
var private const native noexport array<int>	ActionTickListeners;		//TArray<ActionEvent*>

var private const native noexport array<int>	KeyListeners;				//TArray<ActionEvent*>
var private const native noexport array<int>	KeyEventListeners;			//TArray<ActionEvent*>

var private const native noexport int			InputActionListenerTable;	//TMap< EInputAction, TArray<NotifyInfo>* >*

var private const native noexport array<int>	EventQueue;					//TArray<FString>

native static function UIConsole GetInstance();

//------------------------------------------------------------------------------
interface function string GetLevelAction(){ return LastLevelAction; }

//------------------------------------------------------------------------------
static native function AddWatch( name ObjName, name VarName, optional int ArrayIndex );

//-----------------------------------------------------------------------------
static function ClearProgressMessages( PlayerController P )
{
	// Fix ARL: Implement me!!
}

//------------------------------------------------------------------------------
// Sends notifications to specified object.function once per frame.
// Specified function must take one parameter: Canvas.
//------------------------------------------------------------------------------
static native function RegisterRenderListener( object TargetObj, string TargetFunc );
static native function UnRegisterRenderListener( object TargetObj, string TargetFunc );

//------------------------------------------------------------------------------
static native function SendEvent( string EventName, optional ComponentHandle Target );

//------------------------------------------------------------------------------
static native exec function ComponentHandle LoadComponent( string Filename, optional string Component );
static native function ComponentHandle AddNewComponent( string Name, optional string Parent );
static native function ComponentHandle DestroyComponent( ComponentHandle Component );

//------------------------------------------------------------------------------
static native function AddComponent( ComponentHandle Component, optional ComponentHandle Container );

//------------------------------------------------------------------------------
static native function ComponentHandle GetComponent( string Name, optional ComponentHandle Root );
static native function ComponentHandle FindLooseComponent( string Name, optional ComponentHandle Root );
static native function GetLooseComponents( string Name, out array<ComponentHandle> Components, optional ComponentHandle Root );

//------------------------------------------------------------------------------
public native function DispatchMessage( coerce string Msg );

//------------------------------------------------------------------------------
public native function AddTickListener( Object A );
public native function RemoveTickListener( Object A );

//------------------------------------------------------------------------------
static native function int AddKeyListener( object Target, string TargetFunc );
static native function RemoveKeyListener( int MagicKey );	// MagicKey is returned from AddKeyListener.

//------------------------------------------------------------------------------
static native function int AddKeyEventListener( object Target, string TargetFunc );
static native function RemoveKeyEventListener( int MagicKey );	// MagicKey is returned from AddKeyEventListener.

//------------------------------------------------------------------------------
static native function Console				GetConsole();
static native function UIConsole			GetUIConsole();
static native function PlayerController		GetPlayerOwner();
static native function float				GetTimeSeconds();
static native function Client				GetClient();

//------------------------------------------------------------------------------
static native function string				SecondsToTime( float TimeSeconds );

//------------------------------------------------------------------------------
static native function Dimension			GetComponentSize( ComponentHandle Root );

//------------------------------------------------------------------------------
event Message( coerce string Msg, float MsgLife)
{
	DispatchMessage(Msg);
}

//------------------------------------------------------------------------------
native function TimedMessage( coerce string Msg, optional float Duration, optional font Font, optional color Color, optional string Holder, optional string Label, optional bool bClearExisting, optional float WrapX, optional bool bReverseOrder );

//------------------------------------------------------------------------------
event NotifyLevelChange()
{
	Super.NotifyLevelChange();
	SendEvent("LevelChange");
}


defaultproperties
{
	Filename="UI"
	ScaleX=1.0
	ScaleY=1.0
}
