/*=============================================================================
	UnCon.h: UConsole game-specific definition
	Copyright 1997-2003 Epic Games, Inc. All Rights Reserved.

	Contains routines for: Messages, menus, status bar
=============================================================================*/

/*------------------------------------------------------------------------------
	UConsole definition.
------------------------------------------------------------------------------*/

//
// Viewport console.
//
struct UConsole_eventMessage_Parms
{
	FString S;
	FLOAT MsgLife;
};
struct UConsole_eventConnectFailure_Parms
{
    FString FailCode;
    FString URL;
};
class ENGINE_API UConsole : public UObject, public FOutputDevice
{
	DECLARE_CLASS(UConsole,UObject,CLASS_Transient,Engine)

	// Constructor.
	UConsole();

	// UConsole interface.
	virtual void _Init( UViewport* Viewport );
	virtual void PreRender( FSceneNode* Frame );
	virtual void PostRender( FSceneNode* Frame );
	virtual void Serialize( const TCHAR* Data, EName MsgType );
	virtual UBOOL GetDrawWorld();
	virtual void SendEvent( FString EventName ) {}

	virtual void Tick( float DeltaTime )
	{
		guard(UConsole::Tick);
//!!MERGE		clock(Viewport->Actor->GetLevel()->ScriptTickCycles);
		eventTick( DeltaTime );
//!!MERGE		unclock(Viewport->Actor->GetLevel()->ScriptTickCycles);
		unguard;
	}
	virtual void LevelBegin(){}		// notification on start of new level.
	void Destroy();
	UBOOL HandleExec( const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor );

	// Natives.
	DECLARE_FUNCTION(execConsoleCommand);
	DECLARE_FUNCTION(execConsoleCommandEx);	//NEW (arl)
	DECLARE_FUNCTION(execTextSize);
	DECLARE_FUNCTION(execAddExecListener);
	DECLARE_FUNCTION(execRemoveExecListener);

	// Script events.
    void eventMessage(const FString& S, FLOAT MsgLife)
    {
		UConsole_eventMessage_Parms Parms;

        Parms.S=S;
		Parms.MsgLife = MsgLife;

        ProcessEvent(FindFunctionChecked(NAME_Message),&Parms);
    }
    void eventTick(FLOAT DeltaTime)
    {
        struct {FLOAT DeltaTime; } Parms;
        Parms.DeltaTime=DeltaTime;
        ProcessEvent(FindFunctionChecked(ENGINE_Tick),&Parms);
    }
    void eventVideoChange()
    {
        ProcessEvent(FindFunctionChecked(NAME_VideoChange),NULL);
    }
    void eventPostRender(class UCanvas* C)
    {
        struct {class UCanvas* C; } Parms;
        Parms.C=C;
        ProcessEvent(FindFunctionChecked(ENGINE_PostRender),&Parms);
    }
    DWORD eventKeyType(BYTE Key)
    {
        struct {BYTE Key; DWORD ReturnValue; } Parms;
        Parms.Key=Key;
        Parms.ReturnValue=0;
        ProcessEvent(FindFunctionChecked(NAME_KeyType),&Parms);
        return Parms.ReturnValue;
    }
    DWORD eventKeyEvent(BYTE Key, BYTE Action, FLOAT Delta)
    {
        struct {BYTE Key; BYTE Action; FLOAT Delta; DWORD ReturnValue; } Parms;
        Parms.Key=Key;
        Parms.Action=Action;
        Parms.Delta=Delta;
        Parms.ReturnValue=0;
        ProcessEvent(FindFunctionChecked(NAME_KeyEvent),&Parms);
        return Parms.ReturnValue;
    }
    void eventNotifyLevelChange()
    {
        ProcessEvent(FindFunctionChecked(NAME_NotifyLevelChange),NULL);
    }
    void eventConnectFailure(const FString& FailCode, const FString& URL)
    {
        UConsole_eventConnectFailure_Parms Parms;
        Parms.FailCode=FailCode;
        Parms.URL=URL;
        ProcessEvent(FindFunctionChecked(NAME_ConnectFailure),&Parms);
    }
	virtual UBOOL ConsoleCommand( const TCHAR* Command, FString* Result=NULL )
	{
		UBOOL Success=0;

		if( Result )
		{
			FStringOutputDevice StrOut;
			Success = Viewport->Exec( Command, StrOut );
			*Result = *StrOut;
		}
		else
		{
			Success = Viewport->Exec( Command, *this );
		}

		return Success;
	}
	virtual UBOOL KeyType( EInputKey Key )
	{
		return eventKeyType( Key );
	}
	virtual UBOOL KeyEvent( EInputKey iKey, EInputAction State, FLOAT Delta )
	{
		return eventKeyEvent( iKey, State, Delta );
	}
	virtual void AddExecListener( UObject* Listener );
	virtual void RemoveExecListener( UObject* Listener );
	void TextSize( UFont* Font, const TCHAR* Text, FLOAT &XL, FLOAT &YL );

	public:

	// Variables.
    class UViewport* Viewport;
    FLOAT FrameX;
    FLOAT FrameY;
    BITFIELD bNoDrawWorld:1;
	TArray<UObject*> PlugIn;
	TArray<UObject*> ExecListeners;
};

/*------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------*/

