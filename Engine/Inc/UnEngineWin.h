/*=============================================================================
	UnEngineWin.h: Unreal engine windows-specific code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
        * Secret Level's changes, integrated 07/03/02 - Erik de Neve
	  - Broken up Main() into ctor/ looping body /dtor - to enable (Maya-)
	    plugin's external activation of the editor+engine as a child process;
            code from Secret Level's Michael Arnold.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Splash screen.
-----------------------------------------------------------------------------*/

//
// Splash screen, implemented with old-style Windows code so that it
// can be opened super-fast before initialization.
//
BOOL CALLBACK SplashDialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return 0;
}
static HWND hWndSplash = NULL;
void InitSplash( const TCHAR* Filename )
{
	guard(InitSplash);
	HBITMAP hBitmap    = NULL;
	INT     BitmapX    = 0;
	INT     BitmapY    = 0;

	FWindowsBitmap Bitmap(1);
	if( Filename )
	{
		verify(Bitmap.LoadFile(Filename) );
		hBitmap = Bitmap.GetBitmapHandle();
		BitmapX = Bitmap.SizeX;
		BitmapY = Bitmap.SizeY;
	}

	hWndSplash = TCHAR_CALL_OS(CreateDialogW(hInstance,MAKEINTRESOURCEW(IDDIALOG_Splash), NULL, SplashDialogProc),CreateDialogA(hInstance, MAKEINTRESOURCEA(IDDIALOG_Splash), NULL, SplashDialogProc) );
	if( hWndSplash )
	{
		HWND hWndLogo = GetDlgItem(hWndSplash,IDC_Logo);
		if( hWndLogo )
		{
			SetWindowPos(hWndSplash,HWND_TOPMOST,(GetSystemMetrics(SM_CXSCREEN)-BitmapX)/2,(GetSystemMetrics(SM_CYSCREEN)-BitmapY)/2,BitmapX,BitmapY,SWP_SHOWWINDOW);
			SetWindowPos(hWndSplash,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			SendMessageX( hWndLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap );
			UpdateWindow( hWndSplash );
		}
	}
	unguard;
}
void ExitSplash()
{
	guard(ExitSplash);
	DestroyWindow( hWndSplash );
	unguard;
}

/*-----------------------------------------------------------------------------
	System Directories.
-----------------------------------------------------------------------------*/

TCHAR SysDir[256]=TEXT(""), WinDir[256]=TEXT(""), ThisFile[256]=TEXT("");
void InitSysDirs()
{
#if UNICODE
	if( !GUnicodeOS )
	{
		ANSICHAR ASysDir[256]="", AWinDir[256]="", AThisFile[256]="";
		GetSystemDirectoryA( ASysDir, ARRAY_COUNT(ASysDir) );
		GetWindowsDirectoryA( AWinDir, ARRAY_COUNT(AWinDir) );
		GetModuleFileNameA( NULL, AThisFile, ARRAY_COUNT(AThisFile) );
		appStrcpy( SysDir, appFromAnsi(ASysDir) );
		appStrcpy( WinDir, appFromAnsi(AWinDir) );
		appStrcpy( ThisFile, appFromAnsi(AThisFile) );
	}
	else
#endif
	{
		GetSystemDirectory( SysDir, ARRAY_COUNT(SysDir) );
		GetWindowsDirectory( WinDir, ARRAY_COUNT(WinDir) );
		GetModuleFileName( NULL, ThisFile, ARRAY_COUNT(ThisFile) );
	}
	if( !appStricmp( &ThisFile[appStrlen(ThisFile) - 4], TEXT(".ICD") ) )
		appStrcpy( &ThisFile[appStrlen(ThisFile) - 4], TEXT(".EXE") );
}

/*-----------------------------------------------------------------------------
	Config wizard.
-----------------------------------------------------------------------------*/

class WConfigWizard : public WWizardDialog
{
	DECLARE_WINDOWCLASS(WConfigWizard,WWizardDialog,Startup)
	WLabel LogoStatic;
	FWindowsBitmap LogoBitmap;
	UBOOL Cancel;
	FString Title;
	WConfigWizard()
	: LogoStatic(this,IDC_Logo)
	, Cancel(0)
	{
		InitSysDirs();
	}
	void OnInitDialog()
	{
		guard(WStartupWizard::OnInitDialog);
		WWizardDialog::OnInitDialog();
		SendMessageX( *this, WM_SETICON, ICON_BIG, (WPARAM)LoadIconIdX(hInstance,IDICON_Mainframe) );
		LogoBitmap.LoadFile( TEXT("..\\Help\\Logo.bmp") );
		SendMessageX( LogoStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LogoBitmap.GetBitmapHandle() );
		SetText( *Title );
		SetForegroundWindow( hWnd );
		unguard;
	}
};

class WConfigPageSafeOptions : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageSafeOptions,WWizardPage,Startup)
	WConfigWizard* Owner;
	WButton NoSoundButton, ResButton, ResetConfigButton, NoProcessorButton, NoJoyButton;
	WConfigPageSafeOptions( WConfigWizard* InOwner )
	: WWizardPage		( TEXT("ConfigPageSafeOptions"), IDDIALOG_ConfigPageSafeOptions, InOwner )
	, Owner				(InOwner)
	, NoSoundButton		(this,IDC_NoSound)
	, ResButton			(this,IDC_Res)
	, ResetConfigButton	(this,IDC_ResetConfig)
	, NoProcessorButton	(this,IDC_NoProcessor)
	, NoJoyButton		(this,IDC_NoJoy)
	{}
	void OnInitDialog()
	{
		WWizardPage::OnInitDialog();
		SendMessageX( NoSoundButton,     BM_SETCHECK, 1, 0 );
		SendMessageX( ResButton,         BM_SETCHECK, 1, 0 );
		SendMessageX( ResetConfigButton, BM_SETCHECK, 0, 0 );
		SendMessageX( NoProcessorButton, BM_SETCHECK, 1, 0 );
		SendMessageX( NoJoyButton,       BM_SETCHECK, 1, 0 );
	}
	const TCHAR* GetNextText()
	{
		return LocalizeGeneral(TEXT("Run"),TEXT("Startup"));
	}
	WWizardPage* GetNext()
	{
		FString CmdLine;
		if( SendMessageX(NoSoundButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nosound");
		if( SendMessageX(ResButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -defaultres");
		if( SendMessageX(ResetConfigButton,BM_GETCHECK,0,0)==BST_CHECKED )
			GFileManager->Delete( *(FString(appPackage())+TEXT(".ini")) );
		if( SendMessageX(NoProcessorButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nommx -nokni -nok6");
		if( SendMessageX(NoJoyButton,BM_GETCHECK,0,0)==BST_CHECKED )
			CmdLine+=TEXT(" -nojoy");
		ShellExecuteX( NULL, TEXT("open"), ThisFile, *CmdLine, appBaseDir(), SW_SHOWNORMAL );
		Owner->EndDialog(0);
		return NULL;
	}
};

class WConfigPageSafeMode : public WWizardPage
{
	DECLARE_WINDOWCLASS(WConfigPageSafeMode,WWizardPage,Startup)
	WConfigWizard* Owner;
	WCoolButton RunButton, SafeModeButton, WebButton;
	WConfigPageSafeMode( WConfigWizard* InOwner )
	: WWizardPage    ( TEXT("ConfigPageSafeMode"), IDDIALOG_ConfigPageSafeMode, InOwner )
	, RunButton      ( this, IDC_Run,      FDelegate(this,(TDelegate)&WConfigPageSafeMode::OnRun) )
	, SafeModeButton ( this, IDC_SafeMode, FDelegate(this,(TDelegate)&WConfigPageSafeMode::OnSafeMode) )
	, WebButton      ( this, IDC_Web,      FDelegate(this,(TDelegate)&WConfigPageSafeMode::OnWeb) )
	, Owner          (InOwner)
	{}
	void OnRun()
	{
		Owner->EndDialog(1);
	}
	void OnSafeMode()
	{
		Owner->Advance( new WConfigPageSafeOptions(Owner) );
	}
	void OnWeb()
	{
		ShellExecuteX( *this, TEXT("open"), LocalizeGeneral(TEXT("WebPage"),TEXT("Startup")), TEXT(""), appBaseDir(), SW_SHOWNORMAL );
		Owner->EndDialog(0);
	}
	const TCHAR* GetNextText()
	{
		return NULL;
	}
};


/*-----------------------------------------------------------------------------
	Exec hook.
-----------------------------------------------------------------------------*/

// FExecHook.
class FExecHook : public FExec, public FNotifyHook
{
private:
	WConfigProperties* Preferences;
	void NotifyDestroy( void* Src )
	{
		if( Src==Preferences )
			Preferences = NULL;
	}
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		guard(FExecHook::Exec);
		if( ParseCommand(&Cmd,TEXT("SHOWLOG")) )
		{
			TObjectIterator<UEngine> EngineIt;
			if(	
			   EngineIt 
			&& EngineIt->Client 
			&& EngineIt->Client->Viewports.Num() 
			&& GLogWindow )
			{
				if( !GIsEditor )
					EngineIt->Client->Viewports(0)->Exec(TEXT("ENDFULLSCREEN"));
				GLogWindow->Show(1);
				SetFocus( *GLogWindow );
				GLogWindow->Display.ScrollCaret();
			}
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("TakeFocus")) )
		{
			TObjectIterator<UEngine> EngineIt;
			if
			(	EngineIt
			&&	EngineIt->Client
			&&	EngineIt->Client->Viewports.Num() )
				SetForegroundWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("EditObject")) )
		{
			UClass* Class;
			UObject* Outer;
			FName ObjectName;
			TArray<UObject*> Objects;
			TObjectIterator<UEngine> EngineIt;

			static INT Count=0;

			if( EngineIt )
				if( ParseObject<UClass>( Cmd, TEXT("Class="), Class, ANY_PACKAGE ) )
				{
					for( TObjectIterator<UObject> It; It; ++It )
						if( It->IsA( Class ) )
							Objects.AddItem( *It );
				}
				else if( Parse( Cmd, TEXT("Name="), ObjectName ) )
				{
					for( TObjectIterator<UObject> It; It; ++It )
						if( It->GetName() == *ObjectName )
							Objects.AddItem( *It );
				}
				else if( ParseObject<UObject>( Cmd, TEXT("Outer="), Outer, ANY_PACKAGE ) )
				{
					for( TObjectIterator<UObject> It; It; ++It )
						if( It->IsIn( Outer ) )
							Objects.AddItem( *It );
				}

			UBOOL ShowAll=0;
			ParseUBOOL( Cmd, TEXT("ALL="), ShowAll );

			if( Objects.Num() )
			{
				if( ShowAll )
				{
					for( INT i=0; i<Objects.Num(); i++ )
					{
						WObjectProperties* P = new WObjectProperties( TEXT("EditObject"), 0, TEXT(""), NULL, 1 );
						P->OpenWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
						P->Root.SetObjects( (UObject**)&Objects(i), 1 );
						P->Show(1);
					}
				}
				else
				{
					WObjectProperties* P = new WObjectProperties( TEXT("EditObject"), 0, TEXT(""), NULL, 1 );
					P->OpenWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
					P->Root.SetObjects( (UObject**)&Objects( Count++ % Objects.Num() ), 1 );
					P->Show(1);
				}
			}
			else Ar.Logf( TEXT("Bad or missing name") );
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("EditDefault")) )
		{
			UClass* Class;
			TObjectIterator<UEngine> EngineIt;
			if( EngineIt && ParseObject<UClass>( Cmd, TEXT("Class="), Class, ANY_PACKAGE ) )
			{
				AActor* Found   = Class->GetDefaultActor();
				if( Found )
				{
					Found->bSelected = 1;
					WObjectProperties* P = new WObjectProperties( TEXT("EditActor"), 0, TEXT(""), NULL, 1 );
					P->OpenWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
					P->Root.SetObjects( (UObject**)&Found, 1 );
					P->Show(1);
				}
			}
			else Ar.Logf( TEXT("Missing class") );
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("CopyActor")) )
		{
			TObjectIterator<UEngine> EngineIt;
			if( EngineIt && EngineIt->Client )
			{
				UObject* Level = EngineIt->Client->Viewports(0)->Actor->GetLevel();
				UObject::StaticLoadObject( UClass::StaticClass(), NULL, TEXT("Editor.LevelExporterT3D"), NULL, LOAD_NoWarn, NULL );

				// Export the actors.
				FStringOutputDevice Ar;
				UExporter::ExportToOutputDevice( Level, NULL, Ar, TEXT("copy"), 0 );
				appClipboardCopy( *Ar );           
			}
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("EditActor")) )
		{
			UClass* Class;
			TObjectIterator<UEngine> EngineIt;
			UObject* Found   = NULL;
			if( EngineIt )
			{
				if( ParseObject<UClass>( Cmd, TEXT("Class="), Class, ANY_PACKAGE ) )
				{
					if( ParseCommand(&Cmd,TEXT("Defaults")) )
					{
						Found = Class->GetDefaultObject();
					}
					else
					{
						AActor* Player  = EngineIt->Client ? EngineIt->Client->Viewports(0)->Actor : NULL;
						FLOAT   MinDist = 999999.0;
						for( TObjectIterator<AActor> It; It; ++It )
						{
							FLOAT Dist = Player ? FDist(It->Location,Player->Location) : 0.0;
							if
							(	(!Player || It->GetLevel()==Player->GetLevel())
							&&	(!It->bDeleteMe)
							&&	(It->IsA( Class) )
							&&	(Dist<MinDist) )
							{
								MinDist = Dist;
								Found   = *It;
							}
						}
					}
				}
				else
				{
					FName ActorName;
					if( Parse( Cmd, TEXT("Name="), ActorName ) )
					{
						// look for actor by name
						for( TObjectIterator<AActor> It; It; ++It )
						{
							if( !It->bDeleteMe && It->GetName() == *ActorName )
							{
								Found = *It;
								break;
							}
						}
					}
				}

				if( Found )
				{
					WObjectProperties* P = new WObjectProperties( TEXT("EditActor"), 0, TEXT(""), NULL, 1 );
					P->OpenWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
					P->Root.SetObjects( (UObject**)&Found, 1 );
					P->Show(1);
					for( TObjectIterator<AActor> It; It; ++It )
						It->bSelected = 0;
					AActor* A = Cast<AActor>(Found);
                    if( A )
						A->bSelected = 1;
				}
				else 
					Ar.Logf( TEXT("Target not found") );
			}
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("CopyToClipboard")) )
		{
			UClass* Class;
			TObjectIterator<UEngine> EngineIt;
			if( EngineIt && ParseObject<UClass>( Cmd, TEXT("Class="), Class, ANY_PACKAGE ) )
			{
				UObject* Found   = NULL;
				if( ParseCommand(&Cmd,TEXT("Defaults")) )
				{
					Found = Class->GetDefaultObject();
				}
				else
				{
					AActor* Player  = EngineIt->Client ? EngineIt->Client->Viewports(0)->Actor : NULL;
					FLOAT   MinDist = 999999.0;
					for( TObjectIterator<AActor> It; It; ++It )
					{
						FLOAT Dist = Player ? FDist(It->Location,Player->Location) : 0.0;
						if
						(	(!Player || It->GetLevel()==Player->GetLevel())
						&&	(!It->bDeleteMe)
						&&	(It->IsA( Class) )
						&&	(Dist<MinDist) )
						{
							MinDist = Dist;
							Found   = *It;
						}
					}
				}
				if( Found )
				{
					FStringOutputDevice Ar;
					UExporter::ExportToOutputDevice( Found, NULL, Ar, TEXT("T3D"), 0 );
					appClipboardCopy( *Ar );
				}
				else Ar.Logf( TEXT("Actor not found") );
			}
			else Ar.Logf( TEXT("Missing class") );
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("HideLog")) )
		{
			if( GLogWindow )
				GLogWindow->Show(0);
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("Preferences")) )
		{
			TObjectIterator<UEngine> EngineIt;
			if(	
			   EngineIt 
			&& EngineIt->Client 
			&& EngineIt->Client->Viewports.Num() 
			&& !GIsEditor
			)
				EngineIt->Client->Viewports(0)->Exec(TEXT("ENDFULLSCREEN"));

			if( !Preferences )
			{
				Preferences = new WConfigProperties( TEXT("Preferences"), LocalizeGeneral("AdvancedOptionsTitle",TEXT("Window")) );
				Preferences->SetNotifyHook( this );
				Preferences->OpenWindow( GLogWindow ? GLogWindow->hWnd : NULL );
				Preferences->ForceRefresh();
			}
			Preferences->Show(1);
			SetFocus( *Preferences );
			return 1;
		}
		else return 0;
		unguard;
	}
public:
	FExecHook()
	: Preferences( NULL )
	{}
};

/*-----------------------------------------------------------------------------
	Startup and shutdown.
-----------------------------------------------------------------------------*/

#ifndef _EDITOR_
//
// Initialize.
//
static UEngine* InitEngine()
{
	guard(InitEngine);
	DOUBLE LoadTime = appSeconds();

	// Set exec hook.
	static FExecHook GLocalHook;
	GExec = &GLocalHook;

	// Create mutex so installer knows we're running.
	CreateMutexX( NULL, 0, TEXT("UnrealIsRunning"));
	UBOOL AlreadyRunning;
	AlreadyRunning = (GetLastError()==ERROR_ALREADY_EXISTS);

	// First-run menu.
	INT FirstRun=0;
	GConfig->GetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );
	if( ParseParam(appCmdLine(),TEXT("FirstRun")) )
		FirstRun=0;
	if( FirstRun<220 )
	{
		// Migrate savegames.
		TArray<FString> Saves = GFileManager->FindFiles( TEXT("..\\Save\\*.usa"), 1, 0 );
		for( TArray<FString>::TIterator It(Saves); It; ++It )
		{
			INT Pos = appAtoi(**It+4);
			FString Section = TEXT("UnrealShare.UnrealSlotMenu");
			FString Key     = FString::Printf(TEXT("SlotNames[%i]"),Pos);
			if( appStricmp(GConfig->GetStr(*Section,*Key,TEXT("user")),TEXT(""))==0 )
				GConfig->SetString(*Section,*Key,TEXT("Saved game"),TEXT("user"));
		}
	}

	// EXEC from command-line
	FString Command;
	if( Parse(appCmdLine(),TEXT("consolecommand="), Command) )
	{
		debugf(TEXT("Executing console command %s"),*Command);
		GExec->Exec( *Command, *GLog );
		return NULL;
	}

	// Test render device.
	FString Device;
	if( Parse(appCmdLine(),TEXT("testrendev="),Device) )
	{
		debugf(TEXT("Detecting %s"),*Device);
		try
		{
			UClass* Cls = LoadClass<URenderDevice>( NULL, *Device, NULL, 0, NULL );
			GConfig->SetInt(*Device,TEXT("DescFlags"),RDDESCF_Incompatible);
			GConfig->Flush(0);
			if( Cls )
			{
				URenderDevice* RenDev = ConstructObject<URenderDevice>(Cls);
				if( RenDev )
				{
					if( RenDev->Init() )
					{
						debugf(TEXT("Successfully detected %s"),*Device);
					}
					else
					{
						delete RenDev;
						RenDev = NULL;
					}
				}
			}
		} catch( ... ) {}
		FArchive* Ar = GFileManager->CreateFileWriter(TEXT("Detected.ini"),0);
		if( Ar )
			delete Ar;
		return NULL;
	}

	// Config UI.
	guard(ConfigUI);
	if( !GIsEditor && GIsClient )
	{
		WConfigWizard D;
		WWizardPage* Page = NULL;
		if( ParseParam(appCmdLine(),TEXT("safe")) || appStrfind(appCmdLine(),TEXT("readini")) )
		{
			Page = new WConfigPageSafeMode(&D);
			D.Title=LocalizeGeneral(TEXT("SafeMode"),TEXT("Startup"));
		}
// vogel: unwanted behaviour.
#if 0
		else if( FirstRun<ENGINE_VERSION )
		{
			GConfig->SetString(TEXT("Engine.Engine"),TEXT("RenderDevice"),TEXT("D3DDrv.D3DRenderDevice"));
		}
#endif
		//else if( !AlreadyRunning && GFileManager->FileSize(TEXT("Running.ini"))>=0 )
		//	{Page = new WConfigPageSafeMode(&D); D.Title=LocalizeGeneral(TEXT("RecoveryMode"),TEXT("Startup"));}
		if( Page )
		{
			ExitSplash();
			D.Advance( Page );
			if( !D.DoModal() )
				return NULL;
			InitSplash(NULL);
		}
	}
	unguard;

	// Create is-running semaphore file.
	FArchive* Ar = GFileManager->CreateFileWriter(TEXT("Running.ini"),0);
	if( Ar )
		delete Ar;

	// Update first-run.
	if( FirstRun<ENGINE_VERSION )
		FirstRun = ENGINE_VERSION;
	GConfig->SetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );


	// Create the global engine object.
	UClass* EngineClass;
	if( !GIsEditor )
	{
		// Create game engine.
		EngineClass = UObject::StaticLoadClass( UGameEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.GameEngine"), NULL, LOAD_NoFail, NULL );
	}
	else
	{
		// Editor.
		EngineClass = UObject::StaticLoadClass( UEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail, NULL );
	}
	UEngine* Engine = ConstructObject<UEngine>( EngineClass );
	Engine->Init();
	debugf( TEXT("Startup time: %f seconds"), appSeconds()-LoadTime );

	return Engine;
	unguard;
}
#endif

// Looks at all currently loaded packages and prompts the user to save them
// if their "bDirty" flag is set.
//
// Returns 0 if the user doesn't want to save one of the packages.
//
static UBOOL SaveDirtyPackages( UEngine* Engine )
{
	guard(SaveDirtyPackages);

	for( TObjectIterator<UPackage> It; It; ++It )
	{
		if( !It->GetOuter() && It->bDirty && appStricmp( It->GetName(), TEXT("mylevel") ) )
		{
			if( appMsgf( 1, TEXT("The package '%s' has been changed and needs to be saved.  Save it now?"), It->GetName() ) )
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = NULL;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Packages (*.*)\0*.*\0\0";
				ofn.lpstrDefExt = "";
				ofn.lpstrTitle = "Save Dirty Package";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

				if( GetSaveFileNameA(&ofn) )
					Engine->Exec( *FString::Printf( TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\""), It->GetName(), appFromAnsi( File ) ) );
			}
		}
	}

	return 1;

	unguard;
}


//
// Unreal's main message loop.  All windows in Unreal receive messages
// somewhere below this function on the stack.
//

class CMainLoop
{
public:
	CMainLoop( UEngine* InEngine, UBOOL InIncludeMessaging = true );
	~CMainLoop(void);

	void	RunLoop(void);
	bool	Finished(void) const	{return !GIsRunning || GIsRequestingExit;}

private:
	UEngine *		Engine;
	UBOOL			IncludeMessaging;

	DWORD			ThreadId;
	HANDLE			hThread;
	DOUBLE			OldTime;
	DOUBLE			SecondStartTime;
	INT				TickCount;
	DWORD			LastFrameCycles;

	// Benchmarking.
	INT				BMFrames;
	INT				BMDiscardedFrames;
	INT				BMMaxFrames;
	FLOAT			BMSeconds;
	TArray<FString> BMStrings;
	TArray<FLOAT>	BMFrameTimes;

	// Movie recording.
	UBOOL			RecordingMovie;

};

inline CMainLoop::CMainLoop( UEngine* InEngine, UBOOL InIncludeMessaging )
:	Engine(InEngine), IncludeMessaging(InIncludeMessaging)
{
	guard(MainLoopCtor);
	check(Engine);

	// Enter main loop.
	guard(EnterMainLoop);
	if( GLogWindow )
		GLogWindow->SetExec( Engine );
	unguard;

	// Loop while running.
	GIsRunning = 1;
	ThreadId			= GetCurrentThreadId();
	hThread				= GetCurrentThread();
	OldTime				= appSeconds();
	SecondStartTime		= OldTime;
	TickCount			= 0;

	// Benchmarking.
	BMFrames			= 0;
	BMDiscardedFrames	= 10;
	BMSeconds			= 0.0f;
	
	BMMaxFrames			= 0; 
	Parse(appCmdLine(),TEXT("SECONDS="),BMMaxFrames);
	BMMaxFrames			*= 30;
	
	for( INT i=0; i<(BMMaxFrames-BMDiscardedFrames+1); i++ )
		new(BMStrings) FString(TEXT(""));
	if( BMMaxFrames )
		BMFrameTimes.Add(BMMaxFrames-BMDiscardedFrames+1);

	LastFrameCycles		= appCycles();

	// Seed random number generator with constant seed for benchmarking.
	if( GIsBenchmarking )
		appRandInit( 0 );

	// Movie recording.
	RecordingMovie	= ParseParam(appCmdLine(),TEXT("RECORDMOVIE"));
		
	unguard;
		}

inline void	CMainLoop::RunLoop(void)
{
#if 0 //NEW (mdf) not ever needed afaict
	// Ever really needed ? - Erik
	if( Finished() ) 
		return;
#endif

	// Clear stats (will also update old stats).
	GStats.Clear();

    if( GIsBenchmarking )
	{
		if( BMFrames == BMDiscardedFrames )
			BMSeconds		= 0.0f;
		else if( BMFrames > BMMaxFrames )
			appRequestExit(0);
	}

	// Update the world.
	DOUBLE NewTime   = appSeconds();
	FLOAT  DeltaTime = NewTime - OldTime;

	guard(UpdateWorld);
	if( GIsBenchmarking || RecordingMovie )
	{
		//Update.
		Engine->Tick( 1.0f / 30.0f );
		if( GWindowManager )
			GWindowManager->Tick( 1.0f / 30.0f );

		if( BMFrames < BMMaxFrames )
			BMSeconds += DeltaTime;
		BMFrames++;
	}
	else
	{
		//Update.
		Engine->Tick( DeltaTime );
		if( GWindowManager )
			GWindowManager->Tick( DeltaTime );
	}

	TickCount++;
	OldTime = NewTime;

	if( OldTime > SecondStartTime + 1 )
	{
		Engine->CurrentTickRate = (FLOAT)TickCount / (OldTime - SecondStartTime);
		SecondStartTime = OldTime;
		TickCount = 0;
	}
	unguard;

	// Enforce optional maximum tick rate.
	guard(EnforceTickRate);
	if( !GIsBenchmarking && IncludeMessaging )
	{		
		FLOAT MaxTickRate = Engine->GetMaxTickRate();
		if( MaxTickRate>0.0 )
		{
			FLOAT Delta = (1.0/MaxTickRate) - (appSeconds()-OldTime);
			appSleep( Max(0.f,Delta) );
		}
	}
	unguard;

	if( IncludeMessaging ) 
	{
		// Handle all incoming messages.
		guard(MessagePump);
		MSG Msg;
		while( PeekMessageX(&Msg,NULL,0,0,PM_REMOVE) )
		{
			if( Msg.message == WM_QUIT )
			{
				// When closing down the editor, check to see if there are any unsaved dirty packages.
				if( GIsEditor )
					SaveDirtyPackages( Engine );

				GIsRequestingExit = 1;
			}

			guard(TranslateMessage);
			TranslateMessage( &Msg );
			unguardf(( TEXT("%08X %i"), (INT)Msg.hwnd, Msg.message ));

			guard(DispatchMessage);
			DispatchMessageX( &Msg );
			unguardf(( TEXT("%08X %i"), (INT)Msg.hwnd, Msg.message ));
		}
		unguard;
	}

	// If editor thread doesn't have the focus, don't suck up too much CPU time.
	if( GIsEditor && IncludeMessaging )
	{
		guard(ThrottleEditor);
		static UBOOL HadFocus=1;
		UBOOL HasFocus = (GetWindowThreadProcessId(GetForegroundWindow(),NULL) == ThreadId );
		if( HadFocus && !HasFocus )
		{
			// Drop our priority to speed up whatever is in the foreground.
			SetThreadPriority( hThread, THREAD_PRIORITY_BELOW_NORMAL );
		}
		else if( HasFocus && !HadFocus )
		{
			// Boost our priority back to normal.
			SetThreadPriority( hThread, THREAD_PRIORITY_NORMAL );
		}
		if( !HasFocus )
		{
			// Surrender the rest of this timeslice.
			Sleep(0);
		}
		HadFocus = HasFocus;
		unguard;
	}

	if( RecordingMovie ) //&& BMFrames > 120)
			Engine->Client->Viewports(0)->Exec(TEXT("shot"));

	GStats.DWORDStats( GEngineStats.STATS_Frame_TotalCycles ) = appCycles() - LastFrameCycles;
	GStats.DWORDStats( GEngineStats.STATS_Game_ScriptCycles ) = GScriptCycles;
	GScriptCycles = 0;
	LastFrameCycles = appCycles();
	
	if( GIsBenchmarking )
	{
		if( BMFrames == 1 )
		{
			// Get descriptions.
			GStats.UpdateString( BMStrings(0), 1 );
			BMFrameTimes(0) = 0.f;
		}
		else
		{
			// Get frame stats. (Index already implies the +1 for descriptions)
			INT BMStringsIndex = Clamp( BMFrames - BMDiscardedFrames, 2, BMMaxFrames - BMDiscardedFrames );
			GStats.UpdateString( BMStrings(BMStringsIndex - 1), 0 );
			BMFrameTimes( BMStringsIndex - 1 ) = GStats.DWORDStats( GEngineStats.STATS_Frame_TotalCycles ) * GSecondsPerCycle * 1000.f;
		}
	}
}

inline CMainLoop::~CMainLoop(void)
{
	guard(CMainLoopDtor);

	GIsRunning = 0;
	if( GIsBenchmarking )
	{
		INT Frames		= BMMaxFrames - BMDiscardedFrames; 
		UBOOL LogLowFPS = ParseParam(appCmdLine(),TEXT("ONLYLOGLOWFPS"));
		FLOAT LowFPS	= 100.f; // in ms

		// Level.
		FString LevelName = Engine->Client->Viewports(0)->Actor->GetViewTarget()->XLevel->GetPathName();
		LevelName = LevelName.LeftChop( 8 );

		// Get time & date.
		INT Year, Month, DayOfWeek, Day, Hour, Minutes, Sec, MSec;
		appSystemTime( Year, Month, DayOfWeek, Day, Hour, Minutes, Sec, MSec );
		FString DateTime = FString::Printf(TEXT("%i-%02i-%02i-%02i-%02i-%02i"),Year,Month,Day,Hour,Minutes,Sec);

		// Machine Details.
		FString MachineString = FString::Printf(TEXT("%s\r\n%s\r\n%s\r\n%s\r\n\r\n%s\r\n\r\n"),GBuildLabel,GMachineOS,GMachineCPU,GMachineVideo,appCmdLine());
		FString OutputString = MachineString;
		FString LowFPSString = TEXT("");

		// Count how many frames take more than LowFPS ms.
		if( LogLowFPS )
		{
			INT Count = 0;
			for( INT i=1; i<Frames; i++ )	
				if( BMFrameTimes(i) > LowFPS )
					Count++;
			
			LowFPSString = FString::Printf(TEXT("High frametimes: %i / %i == %f percent \r\n\r\n"), Count, Frames, 100.f * FLOAT(Count) / Frames ); 
			OutputString += LowFPSString;
		}

		// Determine min/ max framerate and score.
		FLOAT	MinFPS = 1000.f,
				MaxFPS = 0.f,
				AvgFPS = Frames / BMSeconds,
				Score  = 0.f;

		// Check for MAXFPS command line option.
		FLOAT FPSCap = 0;
		Parse(appCmdLine(),TEXT("MAXFPS="),FPSCap);
		if( FPSCap <= 0 )
			FPSCap = 10000;
		FLOAT FrameTimeCap = 1000.f / FPSCap;

		for( INT i=1; i<Frames; i++ )
		{
			// Calculate min/max.
			FLOAT FPS = 1000.f / BMFrameTimes(i);
			MinFPS = Min( MinFPS, FPS );
			MaxFPS = Max( MaxFPS, FPS );

			// Calculate score.
			Score += Max( BMFrameTimes(i), FrameTimeCap ) ;
		}

		Score = Frames / Score * 1000.f;
		
		// Output detailed results to a file.
		INT LastRand = appRand();
		for( INT i=0; i<Frames; i++ )
		{
			if( LogLowFPS && i!=0 )
			{
				if( BMFrameTimes(i) > LowFPS )
					OutputString += BMStrings(i);
			}
			else
			OutputString += BMStrings(i);
		}
		TCHAR File[1024];
		appSprintf( File, TEXT("..\\Benchmark\\CSVs\\stats-%s.csv"), *DateTime );
		appSaveStringToFile( OutputString, File );
			
		// Output average framerate.
		OutputString = TEXT("");
		appLoadFileToString( OutputString, TEXT("..\\Benchmark\\benchmark.log") );
		OutputString += FString::Printf(TEXT("%f / %f / %f fps -- Score = %f        rand[%i]\r\n"), MinFPS, AvgFPS, MaxFPS, Score, LastRand );
		appSaveStringToFile( OutputString, TEXT("..\\Benchmark\\benchmark.log") );
		OutputString = MachineString;
		if( LogLowFPS )
			OutputString += LowFPSString;
		OutputString += FString::Printf(TEXT("%f / %f / %f fps         rand[%i]\r\nScore = %f\r\n"), MinFPS, AvgFPS, MaxFPS, LastRand, Score );
		appSaveStringToFile( OutputString, *FString::Printf(TEXT("..\\Benchmark\\Results\\avgfps-%s.log"), *DateTime ) );
	
		// Output low framerate stats.
		OutputString = TEXT("");
		appLoadFileToString( OutputString, TEXT("..\\Benchmark\\lowframerate.log") );
		OutputString += FString::Printf(TEXT("%s\r\n%f / %f / %f fps\r\nScore = %f\r\n%s\r\n"), *LevelName, MinFPS, AvgFPS, MaxFPS, Score, *LowFPSString );
		appSaveStringToFile( OutputString, TEXT("..\\Benchmark\\lowframerate.log") );

		// Output average for benchmark launcher.
		if( ParseParam(appCmdLine(),TEXT("UPT") ) )
		{
			OutputString = FString::Printf(TEXT("%f"), AvgFPS );
			appSaveStringToFile( OutputString, TEXT("dummy.ben") );
		}

		GLog->Flush();
		GFileManager->Copy( *FString::Printf(TEXT("..\\Benchmark\\Logs\\u2xmp-%s.log"),*DateTime), TEXT("u2xmp.log") );
	}
	// Exit main loop.
	guard(ExitMainLoop);
	if( GLogWindow )
		GLogWindow->SetExec( NULL );
	GExec = NULL;
	unguard;

	unguard;
}

static void MainLoop( UEngine* Engine )
{
	guard(MainLoop);
	check(Engine);

	CMainLoop theLoop(Engine);
	while( !theLoop.Finished()) 
	{
		theLoop.RunLoop();
	}
	
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
