
friend FArchive& operator<<( FArchive& Ar, EInputAction &A )
{
	return Ar << *(BYTE*)A;
}

// noexport vars.
TArray<class UTextArea*>			MessageAreas;
TArray<class UComponent*>			Incinerator;
TArray<class UComponent*>			TickListeners;
TArray<class UUIHelper*>			HelperTickListeners;
TArray<class ActionEvent*>			ActionTickListeners;
TArray<ActionEvent*>				KeyListeners;
TArray<ActionEvent*>				KeyEventListeners;
TMap<EInputAction,NotifyList>*		InputActionListenerTable;
TArray<FString>						EventQueue;

// Main loop.
void PostRender( FSceneNode* Frame );

// Interface.
UComponent* LoadComponent( FString Filename, FString ComponentName=TEXT("Root"), FString* Overrides=NULL );
UComponent* AddNewComponent( FString Name, FString Parent=TEXT("Root") );
void SendEvent( FString EventName );
void Incinerate( UComponent* C );
void MaybeFlushIncinerator();
void CollectAsset( UObject* A );

// UObject overrides.
UUIConsole();
void Destroy();
void Serialize( FArchive &Ar );

// Console overrides.
void _Init( UViewport* Viewport );
UBOOL KeyType( EInputKey Key );
UBOOL KeyEvent( EInputKey iKey, EInputAction Action, FLOAT Delta );
void Tick( float DeltaTime );
void LevelBegin();
UBOOL ConsoleCommand( const TCHAR* Command, FString* Result=NULL );

// Interfaces.
// Fix ARL: We should probably change all Listener-type functions to use a similar interface using NotifyInfos.
void AddInputActionListener( EInputAction Action, UComponent* C, ActionEvent* NotifyObj );
void RemoveInputActionListener( EInputAction Action, UComponent* C, ActionEvent* NotifyObj );
void AddKeyListener( ActionEvent* NotifyObj );
void RemoveKeyListener( ActionEvent* NotifyObj );
void AddKeyEventListener( ActionEvent* NotifyObj );
void RemoveKeyEventListener( ActionEvent* NotifyObj );
void AddTickListener( UComponent* Listener );
void RemoveTickListener( UComponent* Listener );
void AddActionTickListener( ActionEvent* Listener );
void RemoveActionTickListener( ActionEvent* Listener );
void RegisterRenderListener( UObject* TargetObj, FString TargetStr );
void UnRegisterRenderListener( UObject* TargetObj, FString TargetStr );
void AddWatch( FName ObjName, FName VarName, INT ArrayIndex=0 );


APlayerController*	GetPlayerOwner()	{ return Viewport->Actor; }
ALevelInfo*			GetLevel()			{ return Viewport->Actor->GetLevel()->GetLevelInfo(); }
ALevelInfo*			GetEntryLevel()		{ return ((UGameEngine*)(Viewport->Actor->GetLevel()->Engine))->GEntry->GetLevelInfo(); }

