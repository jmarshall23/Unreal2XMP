#include "UI.h"
#include "ImportUI.h"
#include "UnUIRender.h"

///////////////
// Main loop //
///////////////

EXECFUNC(ToggleUI){ GetGConsole()->bHideUI = !GetGConsole()->bHideUI; }

EXECFUNC(AddWatch)
{
	NOTE(debugf(TEXT("AddWatch: %s"), FString(inArgs[1])));

	// Validate parameters.
	if(inArgCount<=1 || !inArgs[1]){ GExecDisp->Printf( TEXT("Warning! AddWatch: No object specified.") ); return; }
	if(inArgCount<=2 || !inArgs[2]){ GExecDisp->Printf( TEXT("Warning! AddWatch: No variable specified.") ); return; }

	// Get optional array index.
	INT ArrayIndex=0;
	if(inArgCount>3 && inArgs[3])
		ArrayIndex=appAtoi(*FString(inArgs[3]));

	GetGConsole()->AddWatch( FName(inArgs[1],FNAME_Intrinsic), FName(inArgs[2],FNAME_Intrinsic), ArrayIndex );
}

EXECFUNC(SpitNums)
{
	for(INT i=0;i<10;i++)
	for(INT j=0;j<10;j++)
	for(INT k=0;k<10;k++)
	debugf(TEXT("%i%i%i"),i,j,k);
}

EXECFUNC(DumpUITree)
{
	UComponent* Root = GetGConsole()->Root;
	if(inArgCount==2 && inArgs[1])
		Root = Root->GetComponent(FString(inArgs[1]));

#if 1 //NEW

	struct Recursive
	{
		static void Dump(UComponent* A)
		{
			debugf(TEXT("%s"),*GetPathString(A->GetFullPath()));
			UContainer* B=DynamicCast<UContainer>(A);
			if(B)
			{
				for(INT i=0;i<B->Components.Num();i++)
					Dump(B->Components(i));
			}
		}
	};

	Recursive::Dump(Root);

#else

	TCHAR spacer[4096];
	spacer[0]=0;

	struct Recursive
	{
		static void Dump(UComponent* A,TCHAR* spacer,INT Len)
		{
			debugf(TEXT("%s%s"),spacer,*A->GetName());
			UContainer* B=DynamicCast<UContainer>(A);
			if(B)
			{
				const TCHAR* spacertext=TEXT("|---");
				for(INT i=0;i<4;i++)
					spacer[Len++]=spacertext[i];
				check(Len<4096);
				spacer[Len]=0;

				for(INT i=0;i<B->Components.Num();i++)
					Dump(B->Components(i),spacer,Len);

				spacer[Len-4]=0;
			}
		}
	};

	Recursive::Dump(Root,spacer,0);

#endif
}

//------------------------------------------------------------------------------
void UUIConsole::PostRender( FSceneNode* Frame )
{
	guard(UUIConsole::PostRender);
	NOTE(debugf(TEXT("UUIConsole::PostRender")));

	clock( GStats.DWORDStats(GEngineStats.STATS_UI_RenderCycles) );

	Super::PostRender(Frame);

	if(bHideUI) 
	{
		unclock( GStats.DWORDStats(GEngineStats.STATS_UI_RenderCycles) );
		return;
	}

//	Frame->Viewport->Canvas->eventReset();
//	Frame->Viewport->Canvas->SetClip(Frame->Viewport->SizeX,Frame->Viewport->SizeY);

	// Send script events (must be outside scope of UIRenderer).
	UContainer* RootC=DynamicCast<UContainer>(Root);
	if( RootC )
	{
		for( INT i=0; i < RootC->Components.Num(); i++ )
		{
			UScriptComponent* ScriptC=DynamicCast<UScriptComponent>(RootC->Components(i));
			if( ScriptC )
				ScriptC->SendScriptEvent(Frame->Viewport->Canvas);
		}
	}

	// Update LevelAction.
	UBOOL bAnimate=0;
	FString LevelAction=TEXT("");
	ALevelInfo* Info = Viewport->Actor->GetLevel()->GetLevelInfo();
	switch( Info->LevelAction )
	{
	case LEVACT_None:
		if( Info->Pauser && (Info->TimeSeconds > Info->PauseDelay) )
		{
			LevelAction = Localize(TEXT("StatusMessages"),TEXT("Paused"),TEXT("UI"));
			bAnimate=1;
		}
		break;
/*!!MERGE
	case LEVACT_GameOver:
		LevelAction = Localize(TEXT("StatusMessages"),TEXT("GameOver"),TEXT("UI"));
		Info->Game->bDisplayHud=0;
		bAnimate=1;
		break;
	case LEVACT_MissionFailed:
		LevelAction = Localize(TEXT("StatusMessages"),TEXT("MissionFailed"),TEXT("UI"));
		Info->Game->bDisplayHud=0;
		bAnimate=1;
		break;
*/
	case LEVACT_Loading:
		LevelAction = Localize(TEXT("StatusMessages"),TEXT("LoadingMessage"),TEXT("UI"));
/*!!ARL no GameInfo on client - use another method for hiding hud.
		Info->Game->bDisplayHud=0;	*/
		break;
	case LEVACT_Saving:
		LevelAction = Localize(TEXT("StatusMessages"),TEXT("SavingMessage"),TEXT("UI"));
		break;
	case LEVACT_Connecting:
		LevelAction = Localize(TEXT("StatusMessages"),TEXT("ConnectingMessage"),TEXT("UI"));
		break;
	case LEVACT_Precaching:
		LevelAction = Localize(TEXT("StatusMessages"),TEXT("PrecachingMessage"),TEXT("UI"));
		break;
/*!!MERGE
	case LEVACT_Initializing:
		LevelAction = Localize(TEXT("StatusMessages"),TEXT("InitializingMessage"),TEXT("UI"));
		break;
*/
	};

	if( LevelAction != LastLevelAction )
	{
		LastLevelAction = LevelAction;
		UBOOL bOn = LevelAction!=TEXT("");
		SendEvent( bOn ? bAnimate ? TEXT("LevelActionShow") : TEXT("LevelActionOn") : TEXT("LevelActionOff") );
		if( Info->LevelAction==LEVACT_Loading )
			SendEvent( TEXT("ShowLoadingBar") );
//!!MERGE		LevelLoadingPct=0.f;
	}

	// Draw remaining components.
	if( Root )
	{
		bRendering=1;
		FrameIndex++;
		FDefaultUIRenderer Ren(Viewport);
		Root->SetLocationC(Frame->Viewport->SizeX*OffsetX,Frame->Viewport->SizeY*OffsetY);
		Root->SetSizeC(Frame->Viewport->SizeX*ScaleX,Frame->Viewport->SizeY*ScaleY);
		if( Frame->Viewport->Precaching ){}
			//Root->Precache(Frame,&Ren);
		else
			Root->Update(Frame,&Ren);
		Ren.Release();
		bRendering=0;
	}

	// Delete incinerated components.
	MaybeFlushIncinerator();

	unclock( GStats.DWORDStats(GEngineStats.STATS_UI_RenderCycles) );
	
	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::Incinerate( UComponent* C )
{
	guard(UIConsole::Incinerate);
	Incinerator.AddUniqueItem(C);
	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::MaybeFlushIncinerator()
{
	guard(UIConsole::MaybeFlushIncinerator);

	if( bRendering )	// wait till we're done with the update loop, otherwise deleting components will screw it up.
		return;

	if( Incinerator.Num() )
	{
		for( INT i=0; i<Incinerator.Num(); i++ )
		{
			NOTE(debugf(TEXT("Incinerator deleting: %s"), Incinerator.Last() ? *Incinerator.Last()->GetFullName() : TEXT("None") ));
			Incinerator(i)->DeleteUObject();
		}
		Incinerator.Empty();
	}

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::CollectAsset( UObject* A )
{
	guard(UIConsole::CollectAsset);
	if( !GetGTransient() )
		Assets.AddUniqueItem(A);
	unguard;
}


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UUIConsole::_Init( UViewport* Viewport )
{
	guard(UIConsole::_Init);

	Super::_Init(Viewport);

	debugf(NAME_Init,TEXT("Initializing UI subsystem..."));
	
	if( !GConfig->GetString(TEXT("Core.System"),TEXT("UIPath"),GetGUIPath()) )
		appErrorf(TEXT("UIPath not specified!"));

#if 0 //NEW
	FString PackageName = Filename+TEXT(".uix");
	if( GFileManager->FileSize(*PackageName)>=0 )
	{
		// Load textures.
		LoadPackage( NULL, *(Filename+TEXT("T")), LOAD_NoWarn );

		// Load UI Tree.
		LoadUI Ar(*PackageName);
		Ar << Root;
		Ar.Fixup();
	}
	else
	{
		// Create UI Tree from UIScripts.
		GetGTransient()=0;
		Root = LoadComponent(Filename);
		GetGTransient()=1;

		// Save textures.
		UPackage* Pkg = Cast<UPackage>(StaticFindObject( UPackage::StaticClass(), NULL, *(Filename+TEXT("T")) ));
		if(Pkg) SavePackage( Pkg, NULL, RF_Standalone, *FString::Printf(TEXT("..\\Textures\\%sT.utx"),*Filename), NULL );

		// Save UI Tree.
		SaveUI Ar(*PackageName);
		Ar << Root;
		Ar.Fixup();
	}
#else
	GetGTransient()=0;
	Root = LoadComponent(Filename);
	if(Root) Root->Shrink();
	GetGTransient()=1;
#endif

	// Send any events that have queued up waiting for the UI to be loaded.
	for(INT i=0;i<EventQueue.Num();i++)
		SendEvent(EventQueue(i));
	EventQueue.Empty();

	// Not sure why this doesn't get called for the first level loaded.
	LevelBegin();

	unguard;
}

//------------------------------------------------------------------------------
UComponent* UUIConsole::LoadComponent( FString Filename, FString ComponentName, FString* Overrides )
{
	guard(UUIConsole::LoadComponent);
	NOTE(debugf(TEXT("UUIConsole::LoadComponent( %s::%s )"), *Filename, *ComponentName ));

	UComponent* C=NULL;

	if( Filename!=TEXT("") )
	{
		FString PrevFile = GetGFilename();
		GetGFilename() = Filename;
		C = ImportComponent( ComponentName, this, Overrides, true );
		GetGFilename() = PrevFile;
	}

	return C;

	unguard;
}

//------------------------------------------------------------------------------
UComponent* UUIConsole::AddNewComponent( FString Name, FString Parent )
{
	guard(UUIConsole::AddNewComponent);
	NOTE(debugf(TEXT("UUIConsole::AddNewComponent( %s::%s )"), *Name, *Parent ));

	if( !Root )
	{
		debugf(NAME_Warning,TEXT("AddNewComponent: Root is NULL! Name='%s' Parent='%s'"),*Name,*Parent);
		return NULL;
	}

	UContainer* P = DynamicCast<UContainer>(Root->FindLooseComponent( uiName(Parent) ));
	if( !P )
	{
		debugf(NAME_Warning,TEXT("AddNewComponent: Parent container not found! Name='%s' Parent='%s'"),*Name,*Parent);
		return NULL;
	}

	UComponent* C = ImportComponent( Name, this, NULL );
	if( !C )
	{
		debugf(NAME_Warning,TEXT("AddNewComponent: Failed to load component! Name='%s' Parent='%s'"),*Name,*Parent);
		return NULL;
	}

	P->AddComponent( C );

	return C;

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::SendEvent( FString EventName )
{
	guard(UUIConsole::SendEvent);
	NOTE(debugf(TEXT("static UUIConsole::SendEvent( %s )"), (EventName!= FString()) ? *EventName : TEXT("None") ));

	if( GIsEditor )
		return;

	// Queue up events sent before the UI is loaded in _Init.
	if( Root==NULL )
	{
		new(EventQueue)FString(EventName);
		return;
	}

	NOTE(debugf(TEXT("Sending Event: %s"), *EventName));
	FString ObjStr = FString(TEXT("Root"));
	FString EventStr = EventName;
	EventName.Split( TEXT("."), &ObjStr, &EventStr );
	UComponent* C = GetEventScopeTable().FindRef(ObjStr);
	if(!C){ debugf(TEXT("Warning! Send event failed.  '%s' is not a registered component.  Event='%s'"), *ObjStr, *EventName ); return; }
	C->SendEvent( GetEvent(EventStr) );

	unguard;
}


///////////////////////
// UObject overrides //
///////////////////////

//------------------------------------------------------------------------------
UUIConsole::UUIConsole()
{
	guard(UIConsole::UUIConsole);
	NOTE(debugf(TEXT("UUIConsole::UUIConsole")));

	GetGConsole() = this;
	GetObjMap().Set( TEXT("Console"), this );

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::Destroy()
{
	guard(UUIConsole::Destroy);
	NOTE(debugf(TEXT("UUIConsole::Destroy")));

	if( Root )
	{
		Root->DeleteUObject();
		Root = NULL;
	}

	if( InputActionListenerTable )
	{
		delete InputActionListenerTable;
		InputActionListenerTable=NULL;
	}

	while( Helpers.Num() )
	{
		UUIHelper* Helper = Helpers.Pop();
		Helper->DeleteUObject();
	}

	KeyListeners.Empty();
	KeyEventListeners.Empty();
	Assets.Empty();

	GetGConsole() = NULL;

	Super::Destroy();

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::Serialize( FArchive &Ar )
{
	guard(UUIConsole::Serialize);
	NOTE(debugf(TEXT("(%s)UUIConsole::Serialize"), GetFullName() ));

	UObject::Serialize( Ar );	// Can't use Super because of UConsole's function redefinition.

#if 0 // Fix ARL: We need to figure out how to serialize this.
	Ar << *(DWORD*)&InputActionListenerTable;
	if( InputActionListenerTable )
	{
		if( Ar.IsLoading() )
			InputActionListenerTable = new TMap<EInputAction,NotifyList>();
		Ar << *InputActionListenerTable;
	}
#endif

	unguard;
}


///////////////////////
// Console overrides //
///////////////////////

//------------------------------------------------------------------------------
void UUIConsole::Tick( float DeltaTime )
{
	guard(UUIConsole::Tick);
	NOTE(debugf(TEXT("UUIConsole::Tick( %f )"), DeltaTime ));

	Super::Tick( DeltaTime );

	if(bHideUI) return;

	for( INT i=0; i<TickListeners.Num(); i++ )
		TickListeners(i)->Tick( DeltaTime );

	for( INT i=0; i<HelperTickListeners.Num(); i++ )
		HelperTickListeners(i)->Tick( DeltaTime );

	for( INT i=0; i<ActionTickListeners.Num(); i++ )
		ActionTickListeners(i)->Tick( DeltaTime );

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::LevelBegin()
{
	guard(UUIConsole::LevelBegin);
	NOTE(debugf(TEXT("UUIConsole::LevelBegin")));

	Root->Reset();

	// Remove any previous level-specific UIComponents.
	for( INT i=0; i<LevelComponents.Num(); i++ )
		Incinerate( LevelComponents(i) );

	// Destroy all render listeners since they may be pointing to Actors with will be garbage collected.
	UContainer* RootC=DynamicCast<UContainer>(Root);
	if( RootC )
	{
		for( INT i=0; i < RootC->Components.Num(); i++ )
		{
			UScriptComponent* ScriptC=DynamicCast<UScriptComponent>(RootC->Components(i));
			if( ScriptC )
				Incinerate( ScriptC );
		}
	}

	MaybeFlushIncinerator();

/* -- !!arl: disabled for now, not sure if this is safe.
	GetEventScopeTable().Empty();
	GetObjMap().Empty();
//	GetGMouse() = NULL;
	GetGFilename() = TEXT("");
	GetGVarList().Empty();
	GetGVarMap().Empty();
	GetGComponent() = NULL;

	GetEventScopeTable().Set( TEXT("Root"), Root );
*/

	// Create level-specific UIComponents.
	ALevelInfo* Level = Viewport->Actor->Level;
	INT NumComponents = Level->UIComponents.Num();
	LevelComponents.Empty(NumComponents);
	if( NumComponents > 0 )
	{
		LevelComponents.Add(NumComponents);
		for( INT i=0; i<NumComponents; i++ )
			LevelComponents(i) = LoadComponent( TEXT("UI"), Level->UIComponents(i).Component );
		for( INT i=0; i<NumComponents; i++ )
		{
			UContainer* Parent = DynamicCast<UContainer>(Root->FindLooseComponent( Level->UIComponents(i).Parent ));
			Parent->AddComponent( LevelComponents(i) );
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UUIConsole::ConsoleCommand( const TCHAR* Command, FString* Result )
{
	guard(UUIConsole::ConsoleCommand);
	NOTE(debugf(TEXT("UUIConsole::ConsoleCommand( %s )"), Command ));

	UBOOL bResult = Super::ConsoleCommand( Command, Result );

	if( !bResult )
	{
		FString ErrorString = Localize(TEXT("Errors"),TEXT("Exec"),TEXT("Core"));
		ErrorString += TEXT(": ");
		ErrorString += Command;
		eventMessage( ErrorString, 6.0 );

		// send to log also
		debugf( NAME_Log, TEXT("%s"), ErrorString );
	}
	return bResult;

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UUIConsole::KeyType( EInputKey Key )
{
	guard(UUIConsole::KeyType);
	NOTE(debugf(TEXT("UUIConsole::KeyType")));

	if( !bHideUI )
	{
		KeyTypeParms Parms;
		Parms.Key = Key;
		Parms.bHandled = false;

		for( INT i=0; i<KeyListeners.Num(); i++ )
		{
			KeyListeners(i)->OnEvent(&Parms);
			if( Parms.bHandled )
				return true;
		}
	}

	return Super::KeyType( Key );

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UUIConsole::KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
{
	guard(UUIConsole::KeyEvent);
	NOTE(debugf(TEXT("UUIConsole::KeyEvent( %d, %d, %f)"), Key, Action, Delta ));

	if( Key==IK_Shift && Action==IST_Press )	bShift = true;
	if( Key==IK_Shift && Action==IST_Release )	bShift = false;
	if( Key==IK_Ctrl  && Action==IST_Press )	bCtrl  = true;
	if( Key==IK_Ctrl  && Action==IST_Release )	bCtrl  = false;
	if( Key==IK_Alt   && Action==IST_Press )	bAlt   = true;
	if( Key==IK_Alt   && Action==IST_Release )	bAlt   = false;

	if( !bHideUI )
	{
		if( KeyEventListeners.Num() )
		{
			struct KeyEventParms
			{
				BYTE Key;
				BYTE Action;
				FLOAT Delta;
				UBOOL bHandled;
			};
			KeyEventParms Parms;
			Parms.Key = (BYTE)Key;
			Parms.Action = (BYTE)Action;
			Parms.Delta = Delta;
			Parms.bHandled = false;

			for( INT i=0; i<KeyEventListeners.Num(); i++ )
			{
				KeyEventListeners(i)->OnEvent(&Parms);
				if( Parms.bHandled )
					return true;
			}
		}

		if( InputActionListenerTable )
		{
			NotifyList ActionListeners = InputActionListenerTable->FindRef( Action );
			if( ActionListeners )
			{
				KeyEventParms Parms;
				Parms.Key = Key;
				Parms.Action = Action;
				Parms.Delta = Delta;
				Parms.bHandled = false;

				for( INT i=0; i<ActionListeners->Num(); i++ )
				{
					(*ActionListeners)(i)->SendNotification( &Parms );
					if( Parms.bHandled )
						return true;
				}
			}
		}
	}

	return Super::KeyEvent( Key, Action, Delta );

	unguard;
}


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UUIConsole::AddInputActionListener( EInputAction Action, UComponent* C, ActionEvent* NotifyObj )
{
	guard(UUIConsole::AddInputActionListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::AddInputActionListener"), GetFullName() ));

	if( !InputActionListenerTable )
		InputActionListenerTable = new TMap<EInputAction,NotifyList>();

	NotifyList ActionListeners = InputActionListenerTable->FindRef( Action );

	if( !ActionListeners )
	{
		ActionListeners = new NotifyListType();
		InputActionListenerTable->Set( Action, ActionListeners );
	}

	// Insert at beginning so new entries have higher priority.
	// Fix ARL: We may want to change this to take an optional priority rather than explicitly adding new ones to the head of the list.
#if 1 //NEW
	ActionListeners->AddItem( new NotifyTypeClass(C,NotifyObj) );			// Fix ARL: Memory leak.
#else
	new(*ActionListeners,0)NotifyType(new NotifyTypeClass(C,NotifyObj));	// Fix ARL: Memory leak.
#endif

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::RemoveInputActionListener( EInputAction Action, UComponent* C, ActionEvent* NotifyObj )
{
	guard(UUIConsole::RemoveInputActionListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::RemoveInputActionListener"), GetFullName() ));

	NotifyTypeClass NotifyInfo = NotifyTypeClass(C,NotifyObj);

	if( InputActionListenerTable )
	{
		NotifyList ActionListeners = InputActionListenerTable->FindRef( Action );
		if( ActionListeners )
		{
			for( INT i=0; i<ActionListeners->Num(); /*below*/ )
			{
				if( *(*ActionListeners)(i) == NotifyInfo )
					ActionListeners->Remove(i);
				else i++;
			}
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::AddKeyListener( ActionEvent* NotifyObj )
{
	guard(UUIConsole::AddKeyListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::AddKeyListener"), GetFullName() ));

	new(KeyListeners,0)(ActionEvent*)(NotifyObj);	// inserted at beginning so new entries have higher priority.

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::RemoveKeyListener( ActionEvent* NotifyObj )
{
	guard(UUIConsole::RemoveKeyListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::RemoveKeyListener"), GetFullName() ));

	for( INT i=0; i<KeyListeners.Num(); /*below*/ )
	{
		if( *KeyListeners(i) == *NotifyObj )
			KeyListeners.Remove(i);
		else i++;
	}

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::AddKeyEventListener( ActionEvent* NotifyObj )
{
	guard(UUIConsole::AddKeyEventListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::AddKeyEventListener"), GetFullName() ));

	new(KeyEventListeners,0)(ActionEvent*)(NotifyObj);	// inserted at beginning so new entries have higher priority.

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::RemoveKeyEventListener( ActionEvent* NotifyObj )
{
	guard(UUIConsole::RemoveKeyEventListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::RemoveKeyEventListener"), GetFullName() ));

	for( INT i=0; i<KeyEventListeners.Num(); /*below*/ )
	{
		if( *KeyEventListeners(i) == *NotifyObj )
			KeyEventListeners.Remove(i);
		else i++;
	}

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::AddTickListener( UComponent* Listener )
{
	guard(UUIConsole::AddTickListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::AddTickListener( %s )"), GetFullName(), Listener ? *Listener->GetFullName() : TEXT("None") ));

	TickListeners.AddItem( Listener );

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::RemoveTickListener( UComponent* Listener )
{
	guard(UUIConsole::RemoveTickListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::RemoveTickListener"), GetFullName() ));

	TickListeners.RemoveItem( Listener );

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::AddActionTickListener( ActionEvent* Listener )
{
	guard(UUIConsole::AddActionTickListener);

	ActionTickListeners.AddItem( Listener );

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::RemoveActionTickListener( ActionEvent* Listener )
{
	guard(UUIConsole::RemoveActionTickListener);

	ActionTickListeners.RemoveItem( Listener );

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::RegisterRenderListener( UObject* TargetObj, FString TargetFunc )
{
	guard(UUIConsole::RegisterRenderListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::RegisterRenderListener"), GetFullName() ));

	UScriptComponent* C=NEW_TRANSCOMPONENT(UScriptComponent);
	C->AddAccessor( new ScriptEvent(ACCESSOR_ScriptComponentTarget,TargetObj,TargetFunc) );
	DynamicCastChecked<UContainer>(Root)->AddComponent(C);

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::UnRegisterRenderListener( UObject* TargetObj, FString TargetFunc )
{
	guard(UUIConsole::UnRegisterRenderListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::UnRegisterRenderListener"), GetFullName() ));

	ScriptEvent Event(ACCESSOR_None,TargetObj,TargetFunc);

	UContainer* RootC=DynamicCast<UContainer>(Root);
	if(RootC) for( INT i=0; i<RootC->Components.Num(); /*below*/ )
	{
		UScriptComponent* C=DynamicCast<UScriptComponent>(RootC->Components(i));
		ActionEvent* E = C ? C->GetTarget() : NULL;
		if( E && *E==Event )
			C->DeleteUObject();
		else i++;
	}

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::AddWatch( FName ObjName, FName VarName, INT ArrayIndex )
{
	guard(UUIConsole::AddWatch);
	NOTE(debugf(TEXT("(%s)UUIConsole::AddWatch"), GetFullName() ));

	UContainer* RootC = DynamicCast<UContainer>(GetEventScopeTable().FindRef(FString(TEXT("Root"))));

	if(!RootC)
		return;

	// Create WatchContainer as needed.
	static UTileContainer* WatchContainer=NULL;
	if(!WatchContainer)
	{
		WatchContainer=NEW_TRANSCOMPONENT(UTileContainer);
		RootC->AddComponent(WatchContainer);
	}

	// Find specified watch target.
	UObject* WatchObj=NULL;
	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->GetFName() == ObjName )
		{
			WatchObj = *It;

			// Add UI Component.
			UVarWatcher* Watch=NEW_TRANSCOMPONENT(UVarWatcher);
			Watch->bFullDesc=true;
			Watch->SetSizeC(1000,18);
			Watch->SetWatch( WatchObj, VarName, ArrayIndex );
			WatchContainer->AddComponent(Watch);
		}
	}

	unguard;
}


//////////////////////////
// UnrealScript natives //
//////////////////////////

//------------------------------------------------------------------------------
void UUIConsole::execLoadComponent( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execLoadComponent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execLoadComponent"), GetFullName() ));

	P_GET_STR(Filename);
	P_GET_STR_OPTX(Component,TEXT("Root"));
	P_FINISH;

	*(FComponentHandle*)Result = GetGConsole()->LoadComponent( Filename, Component );

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execAddNewComponent( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execAddNewComponent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execAddNewComponent"), GetFullName() ));

	P_GET_STR(Name);
	P_GET_STR_OPTX(Parent,TEXT("Root"));
	P_FINISH;

	*(FComponentHandle*)Result = GetGConsole()->AddNewComponent( Name, Parent );

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execDestroyComponent( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execDestroyComponent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execDestroyComponent"), GetFullName() ));

	P_GET_STRUCT(FComponentHandle,Component);
	P_FINISH;

	// Check for attempt to destroy component after entire ui tree has been destroyed (ComponentHandle will be bogus and crash if we try to delete it).
	if(!GetGConsole())
	{
		debugf(NAME_Warning,TEXT("DestroyComponent: Console==NULL"));
		return;
	}
	if(!GetGConsole()->Root)
	{
		debugf(NAME_Warning,TEXT("DestroyComponent: Root==NULL"));
		return;
	}

	// Other error checks.
	if(!Component)
	{
		debugf(NAME_Warning,TEXT("Attempt to destroy NULL component!"));
		*(FComponentHandle*)Result = NULL;
		return;
	}
	if(!Component->IsTransient())
	{
		debugf(NAME_Warning,TEXT("Cannot delete non-transient component %s!"),Component->GetFullName());
		*(FComponentHandle*)Result = Component;
		return;
	}

	GetGConsole()->MaybeFlushIncinerator();		// just in case this component is in the incinerator.
	Component->DeleteUObject();
	*(FComponentHandle*)Result = NULL;

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execAddComponent( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execAddComponent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execAddComponent"), GetFullName() ));

	P_GET_STRUCT(FComponentHandle,Component);
	P_GET_STRUCT_OPTX(FComponentHandle,Container,GetGConsole()->Root);
	P_FINISH;

	UContainer* C=DynamicCast<UContainer>(Container);
	if(C) C->AddComponent( Component );
	else debugf(NAME_Warning,TEXT("UIConsole::AddComponent: Invalid container specified!"));

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetComponent( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetComponent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetComponent"), GetFullName() ));

	P_GET_STR(Name);
	P_GET_STRUCT_OPTX(FComponentHandle,C,GetGConsole()->Root);
	P_FINISH;

	*(FComponentHandle*)Result = C->GetComponent( uiName(Name) );

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execFindLooseComponent( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execFindLooseComponent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execFindLooseComponent"), GetFullName() ));

	P_GET_STR(Name);
	P_GET_STRUCT_OPTX(FComponentHandle,C,GetGConsole()->Root);
	P_FINISH;

	*(FComponentHandle*)Result = C->FindLooseComponent( uiName(Name) );

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetLooseComponents( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetLooseComponent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetLooseComponent"), GetFullName() ));

	P_GET_STR(Name);
	P_GET_DYNARRAY_REF(UComponent*,Components);
	P_GET_STRUCT_OPTX(FComponentHandle,C,GetGConsole()->Root);
	P_FINISH;

	C->GetLooseComponents( uiName(Name), *Components );

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execDispatchMessage( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execDispatchMessage);
	NOTE(debugf(TEXT("(%s)UUIConsole::execDispatchMessage"), GetFullName() ));

	P_GET_STR(Msg);
	P_FINISH;

	for( INT i=0; i<MessageAreas.Num(); i++ )
		MessageAreas(i)->AddText(Msg);

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execRegisterRenderListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execRegisterRenderListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execRegisterRenderListener"), GetFullName() ));

	P_GET_OBJECT(UObject,TargetObj);
	P_GET_STR(TargetFunc);
	P_FINISH;

	GetGConsole()->RegisterRenderListener( TargetObj, TargetFunc );

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execUnRegisterRenderListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execUnRegisterRenderListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execUnRegisterRenderListener"), GetFullName() ));

	P_GET_OBJECT(UObject,TargetObj);
	P_GET_STR(TargetFunc);
	P_FINISH;

	GetGConsole()->UnRegisterRenderListener( TargetObj, TargetFunc );

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execSendEvent( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execSendEvent);
	NOTE(debugf(TEXT("(%s)UUIConsole::execSendEvent"), GetFullName() ));

	P_GET_STR(EventName);
	P_GET_STRUCT_OPTX(FComponentHandle,C,NULL);
	P_FINISH;

	if( C )
		C->SendEvent( GetEvent(EventName) );
	else if( GetGConsole() )
		GetGConsole()->SendEvent(EventName);
	//NOTE (mdf): this is happening all the time on a dedicated server?
	//else
	//	debugf( NAME_Warning, TEXT( "Event '%s' not sent! (Console==NULL)"), *EventName );

	unguardexec;
}


//------------------------------------------------------------------------------
void UUIConsole::execAddTickListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsoler::execAddTickListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execAddTickListener"), GetFullName() ));

	P_GET_OBJECT(UUIHelper,Helper);
	P_FINISH;

	HelperTickListeners.AddItem(Helper);

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execRemoveTickListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsoler::execRemoveTickListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execRemoveTickListener"), GetFullName() ));

	P_GET_OBJECT(UUIHelper,Helper);
	P_FINISH;

	HelperTickListeners.RemoveItem(Helper);

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execAddKeyListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsoler::execAddKeyListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execAddKeyListener"), GetFullName() ));

	P_GET_OBJECT(UObject,Target);
	P_GET_STR(TargetFunc);
	P_FINISH;

	ActionEvent* Event = new ScriptEvent(ACCESSOR_None,Target,TargetFunc);
	GetGConsole()->AddKeyListener(Event);

	*(INT*)Result = (INT)Event;

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execRemoveKeyListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsoler::execRemoveKeyListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execRemoveKeyListener"), GetFullName() ));

	P_GET_INT(Event);
	P_FINISH;

	GetGConsole()->RemoveKeyListener((ActionEvent*)Event);
	delete (ActionEvent*)Event;

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execAddKeyEventListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsoler::execAddKeyEventListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execAddKeyEventListener"), GetFullName() ));

	P_GET_OBJECT(UObject,Target);
	P_GET_STR(TargetFunc);
	P_FINISH;

	ActionEvent* Event = new ScriptEvent(ACCESSOR_None,Target,TargetFunc);
	GetGConsole()->AddKeyEventListener(Event);

	*(INT*)Result = (INT)Event;

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execRemoveKeyEventListener( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsoler::execRemoveKeyEventListener);
	NOTE(debugf(TEXT("(%s)UUIConsole::execRemoveKeyEventListener"), GetFullName() ));

	P_GET_INT(Event);
	P_FINISH;

	GetGConsole()->RemoveKeyEventListener((ActionEvent*)Event);
	delete (ActionEvent*)Event;

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetInstance( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetInstance);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetInstance"), GetFullName() ));
	P_FINISH;

	*(UUIConsole**)Result = GetGConsole();

	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execAddWatch( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execAddWatch);
	NOTE(debugf(TEXT("(%s)UUIConsole::execAddWatch"), GetFullName() ));

	P_GET_NAME(ObjName);
	P_GET_NAME(VarName);
	P_GET_INT_OPTX(ArrayIndex,0);
	P_FINISH;

	AddWatch(ObjName,VarName,ArrayIndex);

	unguard;
}


//------------------------------------------------------------------------------
void UUIConsole::execTimedMessage( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execTimedMessage);
	NOTE(debugf(TEXT("(%s)UUIConsole::execTimedMessage"), GetFullName() ));

	P_GET_STR(Msg);
	P_GET_FLOAT_OPTX(Duration,0.0f);
	P_GET_OBJECT_OPTX(UFont,Font,NULL);
	P_GET_STRUCT_OPTX(FColor,Color,FColor(0,0,0,0));
	P_GET_STR_OPTX(HolderStr,TEXT(""));
	P_GET_STR_OPTX(LabelStr,TEXT(""));
	P_GET_UBOOL_OPTX(bClearExisting,false);
	P_GET_FLOAT_OPTX(WrapX,0.0f);
	P_GET_UBOOL_OPTX(bReverseOrder,false);
	P_FINISH;

	if(HolderStr==TEXT(""))
		HolderStr=TEXT("ConsoleTimedLabelHolder");
	if(LabelStr==TEXT(""))
		LabelStr=TEXT("ConsoleTimedLabel");

	UContainer* TimedLabelHolder=NULL;
	UTimedLabel* TimedLabel=NULL;
	UComponent* TopComponent=NULL;
	UContainer* TimedLabelContainer=NULL;
	uiName HolderName=0;

	if(!GetGConsole() || !GetGConsole()->Root)	// Fix ARL: Remove once _Init is called before player is created.
		goto Fail;

	HolderName=uiName(HolderStr);
	if( HolderName==uiName(0,0) )
		goto Fail;

	// Fix ARL: Cache found holders for faster lookup since we are most likely to look for the same ones over and over.
	TimedLabelHolder = DynamicCast<UContainer>(GetGConsole()->Root->FindLooseComponent(HolderName));
	if(!TimedLabelHolder)
		goto Fail;

	TopComponent = GetGConsole()->LoadComponent(TEXT("Console"),LabelStr);
	TimedLabel = DynamicCast<UTimedLabel>(TopComponent);
	if(!TimedLabel)
	{
		TimedLabelContainer = DynamicCast<UContainer>(TopComponent);
		if(!TimedLabelContainer)
			goto Fail;
		static uiName LabelName=uiName(TEXT("ConsoleTimedLabel"));
		TimedLabel = DynamicCast<UTimedLabel>(TimedLabelContainer->FindLooseComponent(LabelName));	// Don't use cached since this is a newly loaded component.
		if(!TimedLabel)
			goto Fail;
	}

	if( bClearExisting )
	{
		for( INT i=0; i<TimedLabelHolder->Components.Num(); i++ )
			GetGConsole()->Incinerate(TimedLabelHolder->Components(i));
		MaybeFlushIncinerator();
	}

	if( bReverseOrder )
		TopComponent->DrawOrder = TimedLabelHolder->Components.Num() ? TimedLabelHolder->Components(0)->DrawOrder-1 : 0.0f;

	TimedLabelHolder->AddComponent(TopComponent);
	if( Font )
		TimedLabel->Font = Font;
	TimedLabel->SetText( Msg );
	if( Duration )
		TimedLabel->SetLifespan( Duration );
	if( Color.A )
		TimedLabel->DrawColor = Color;
	if( WrapX )
	{
		TimedLabel->bWrap = true;
		TimedLabel->SetWidth( WrapX );
	}

	return;

Fail:
	debugf(NAME_Warning,TEXT("Failed to display timed message: '%s' (Holder='%s' Label='%s')"),*Msg,*HolderStr,*LabelStr);
	unguard;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetConsole( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetConsole);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetConsole"), GetFullName() ));

	P_FINISH;

	*(UConsole**)Result = GetGConsole();

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetUIConsole( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetUIConsole);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetUIConsole"), GetFullName() ));

	P_FINISH;

	*(UUIConsole**)Result = GetGConsole();

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetPlayerOwner( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetPlayerOwner);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetPlayerOwner"), GetFullName() ));

	P_FINISH;

	*(APlayerController**)Result = GetGConsole()->Viewport->Actor;

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetTimeSeconds( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetTimeSeconds);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetTimeSeconds"), GetFullName() ));

	P_FINISH;

	*(FLOAT*)Result = appSeconds();

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetClient( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetClient);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetClient"), GetFullName() ));

	P_FINISH;

	*(UClient**)Result = GetGConsole()->Viewport->Actor->GetLevel()->Engine->Client;

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execSecondsToTime( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execSecondsToTime);

	P_GET_FLOAT(TimeSeconds);
	P_FINISH;

	UBOOL bNegative = (TimeSeconds < 0.f);
	if( bNegative )
		TimeSeconds = -TimeSeconds;

	INT	Seconds	= TimeSeconds;
	INT	Minutes	= Seconds / 60;
	INT	Hours	= Minutes / 60;
		Seconds	= Seconds - (Minutes * 60);
		Minutes	= Minutes - (Hours * 60);

	if( bNegative )
		*(FString*)Result = FString::Printf(TEXT("[-%02d:%02d:%02d]"),Hours,Minutes,Seconds);
	else
		*(FString*)Result = FString::Printf(TEXT("[%02d:%02d:%02d]"),Hours,Minutes,Seconds);

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIConsole::execGetComponentSize( FFrame& Stack, RESULT_DECL )
{
	guard(UUIConsole::execGetComponentSize);
	NOTE(debugf(TEXT("(%s)UUIConsole::execGetComponentSize"), GetFullName() ));

	P_GET_STRUCT(FComponentHandle,Component);
	P_FINISH;

	if(!Component)
	{
		debugf(NAME_Warning,TEXT("Attempt to get the size of NULL component!"));
		*(FDimension*)Result = NULL;
		return;
	}

	*(FDimension*)Result = Component->GetSize();

	unguardexec;
}


IMPLEMENT_CLASS(UUIConsole);

