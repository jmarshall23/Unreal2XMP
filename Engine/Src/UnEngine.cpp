/*=============================================================================
	UnEngine.cpp: Unreal engine main.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnLinker.h"
#include "FTimerManager.h"
/*-----------------------------------------------------------------------------
	Object class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UEngine);
IMPLEMENT_CLASS(URenderDevice);

/*-----------------------------------------------------------------------------
	Engine init and exit.
-----------------------------------------------------------------------------*/

//
// Construct the engine.
//
UEngine::UEngine()
{
	guard(UEngine::UEngine);
	unguard;
}

//
// Init class.
//
void UEngine::StaticConstructor()
{
	guard(UEngine::StaticConstructor);

	new(GetClass(),TEXT("CacheSizeMegs"),      RF_Public)UIntProperty (CPP_PROPERTY(CacheSizeMegs      ), TEXT("Settings"), CPF_Config );

	unguard;
}

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) ENGINE_API FName ENGINE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "EngineClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

// Register natives.
#define NATIVES_ONLY
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name)
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#include "EngineClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NATIVES_ONLY
#undef NAMES_ONLY

//
// Init audio.
//
void UEngine::InitAudio()
{
	guard(UEngine::InitAudio);
	if
	(	UseSound
	&&	GIsClient
	&&	!ParseParam(appCmdLine(),TEXT("NOSOUND")) )
	{
		UClass* AudioClass = StaticLoadClass( UAudioSubsystem::StaticClass(), NULL, TEXT("ini:Engine.Engine.AudioDevice"), NULL, LOAD_NoFail, NULL );
		Audio = ConstructObject<UAudioSubsystem>( AudioClass );
		if( !Audio->Init() )
		{
			debugf( NAME_Error, TEXT("Audio initialization failed.") );
			delete Audio;
			Audio = NULL;
		}
	}
	unguard;
}

//
// Initialize the engine.
//
void UEngine::Init()
{
	guard(UEngine::Init);

	// Add the intrinsic names.
	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name) ENGINE_##name = FName(TEXT(#name),FNAME_Intrinsic);
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#include "EngineClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY

	// Subsystems.
	FURL::StaticInit();
	GEngineMem.Init( 65536 );
	GCache.Init( 1024 * 1024 * CacheSizeMegs, 4096 );
	GEngineStats.Init();

	// Initialize random number generator.
	if( GIsBenchmarking )
		appRandInit( 0 );
	else
		appRandInit( appCycles() );

	if(!GStatGraph)
		GStatGraph = new FStatGraph();

	if(!GTempLineBatcher)
		GTempLineBatcher = new FTempLineBatcher();

	// Objects.
	Cylinder = new UPrimitive;

	// Add to root.
	AddToRoot();

	// Create GGlobalTempObjects
	GGlobalTempObjects = new UGlobalTempObjects;

	// Ensure all native classes are loaded.
	for( TObjectIterator<UClass> It ; It ; ++It )
		if( !It->GetLinker() )
			LoadObject<UClass>( It->GetOuter(), It->GetName(), NULL, LOAD_Quiet|LOAD_NoWarn, NULL );

	debugf( NAME_Init, TEXT("Unreal engine initialized") );
	unguard;
}

//
// Exit the engine.
//
void UEngine::Destroy()
{
	guard(UEngine::Destroy);

	// Remove from root.
	RemoveFromRoot();

	// Shut down all subsystems.
	Audio	 = NULL;
	Client	 = NULL;
	FURL::StaticExit();
	GEngineMem.Exit();
	GCache.Exit( 1 );
	if(GStatGraph)
	{
		delete GStatGraph;
		GStatGraph = NULL;
	}
	
	if(GTempLineBatcher)
	{
		delete GTempLineBatcher;
		GTempLineBatcher = NULL;
	}

	FTimerManager::FreeInstance();

	Super::Destroy();
	unguard;
}

//
// Flush all caches.
//
void UEngine::Flush( UBOOL AllowPrecache )
{
	guard(UEngine::Flush);

	GCache.Flush();
	if( Client )
		Client->Flush( AllowPrecache );

	unguard;
}

//
// Update Gamma/Brightness/Contrast settings.
//
void UEngine::UpdateGamma()
{
	guard(UEngine::UpdateGamma);

	if( Client )
		Client->UpdateGamma();

	unguard;
}

//
// Restore Gamma/Brightness/Contrast settings.
//
void UEngine::RestoreGamma()
{
	guard(UEngine::RestoreGamma);

	if( Client )
		Client->RestoreGamma();

	unguard;
}

//
// Tick rate.
//
FLOAT UEngine::GetMaxTickRate()
{
	guard(UEngine::GetMaxTickRate);
	return ( GIsEditor ? 30 : 0);
	unguard;
}

//
// Progress indicator.
//
void UEngine::SetProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds )
{
	guard(UEngine::SetProgress);
	unguard;
}

//
// Serialize.
//
void UEngine::Serialize( FArchive& Ar )
{
	guard(UGameEngine::Serialize);

	Super::Serialize( Ar );
	Ar << Cylinder << Client << Audio << GRenDev;

	unguardobj;
}

/*-----------------------------------------------------------------------------
	Input.
-----------------------------------------------------------------------------*/

//
// This always going to be the last exec handler in the chain. It
// handles passing the command to all other global handlers.
//
UBOOL UEngine::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UEngine::Exec);

	// See if any other subsystems claim the command.
	if( GSys    && GSys->Exec		(Cmd,Ar) ) return 1;
	if( UObject::StaticExec			(Cmd,Ar) ) return 1;
	if( GCache.Exec					(Cmd,Ar) ) return 1;
	if( GExec   && GExec->Exec      (Cmd,Ar) ) return 1;
	if( Client  && Client->Exec		(Cmd,Ar) ) return 1;
	if( Audio   && Audio->Exec		(Cmd,Ar) ) return 1;
	if( GStatGraph && GStatGraph->Exec(Cmd,Ar) ) return 1;
	if( GExecDisp )
	{
		GExecDisp->UserData = this;
		if( GExecDisp->Exec(Cmd,Ar) )
			return 1;
	}

	// Handle engine command line.
	if( ParseCommand(&Cmd,TEXT("FLUSH")) )
	{
		Flush(1);
		Ar.Log( TEXT("Flushed engine caches") );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("STAT")) )
	{
		if( ParseCommand(&Cmd,TEXT("ANIM")) )
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowAnimStats;
#else
			bShowAnimStats = !bShowAnimStats;
#endif
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("DEFAULT")) || ParseCommand(&Cmd,TEXT("RESET")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = 0;
#else
			bShowAnimStats		= 0;
			bShowRenderStats	= 0;
			bShowHardwareStats	= 0;
			bShowMatineeStats	= 0;
			bShowGameStats		= 0;
			bShowAudioStats		= 0;
			bShowNetStats		= 0;
			bShowHistograph		= 0;
#endif
			GStats.bShowAssetMemStats = 0;
			GStats.bShowMemStats = 0;
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("FPS")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowFPS;
#else
			bShowFrameRate = !bShowFrameRate;
#endif
			return 1;
		}
#if 1 //NEW (mdf) Legend stats
		else if(ParseCommand(&Cmd,TEXT("FPSEX")))
		{
			StatFlags = StatFlags ^ STAT_ShowFPSEx;
			return 1;
		}
#endif
		else if(ParseCommand(&Cmd,TEXT("RENDER")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowRenderStats;
#else
			bShowRenderStats = !bShowRenderStats;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("HARDWARE")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowHardwareStats;
#else
			bShowHardwareStats = !bShowHardwareStats;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("GAME")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowGameStats;
#else
			bShowGameStats = !bShowGameStats;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("HISTOGRAPH")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowHistograph;
#else
			bShowHistograph = !bShowHistograph;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("XBOXMEM")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowXboxMemStats;
#else
			bShowXboxMemStats = !bShowXboxMemStats;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("MATINEE")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowMatineeStats;
#else
			bShowMatineeStats = !bShowMatineeStats;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("AUDIO")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowAudioStats;
#else
			bShowAudioStats = !bShowAudioStats;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("NET")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags ^ STAT_ShowNetStats;
#else
			bShowNetStats		= !bShowNetStats;
#endif
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("ALL")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = StatFlags & ( STAT_ShowFPS | 
									  STAT_ShowRenderStats | 
									  STAT_ShowHardwareStats | 
									  STAT_ShowMatineeStats |
									  STAT_ShowGameStats |
									  STAT_ShowAudioStats |
									  STAT_ShowNetStats );
#else
			//bShowAnimStats	= 1;
			bShowFrameRate		= 1;
			bShowRenderStats	= 1;
			bShowHardwareStats	= 1;
			bShowMatineeStats	= 1;
			bShowGameStats		= 1;
			bShowAudioStats		= 1;
			bShowNetStats		= 1;
#endif
			GStats.bShowAssetMemStats = 1;
			GStats.bShowMemStats = 1;
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("NONE")))
		{
#if 1 //NEW (mdf) Legend stats
			StatFlags = 0;
#else
			bShowAnimStats		= 0;
			bShowFrameRate		= 0;
			bShowRenderStats	= 0;
			bShowHardwareStats	= 0;
			bShowMatineeStats	= 0;
			bShowGameStats		= 0;
			bShowAudioStats		= 0;
			bShowNetStats		= 0;
#endif
			GStats.bShowAssetMemStats = 0;
			GStats.bShowMemStats = 0;
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("MEM")))
		{
			GStats.bShowMemStats = !GStats.bShowMemStats;
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("ASSET")))
		{
			GStats.bShowAssetMemStats = !GStats.bShowAssetMemStats;
			return 1;
		}
		else if( ParseCommand( &Cmd, TEXT("SETASSET") ) )
		{
			UObject*  CheckObject   = NULL;
			UClass*   CheckType     = NULL;
			ParseObject<UClass>  ( Cmd, TEXT("CLASS="  ),
				CheckType,     ANY_PACKAGE );
			ParseObject          ( Cmd, TEXT("NAME="   ),
				CheckType, CheckObject, ANY_PACKAGE );

			if (!CheckObject && GIsEditor)
			{
				for (TObjectIterator<AActor> It; It; ++It)
				{
					if( *It && It->bSelected )
					{
						CheckObject = *It;
						break;
					}
				}
			}


			if (CheckObject)
			{
				GStats.CheckObject = CheckObject;
			}
			return 1;
		}
#if 1 //NEW (mdf) Legend stats
		else if(ParseCommand(&Cmd,TEXT("QUICK")))
		{
			StatFlags = StatFlags ^ STAT_ShowQuick;
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("AUDIO")))
		{
			StatFlags = StatFlags ^ STAT_ShowAudioStats;
			return 1;
		}
		else if(ParseCommand(&Cmd,TEXT("LOCK")))
		{
			StatFlags = StatFlags ^ STAT_Lock;
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("CLEAR")) )
		{
			if( !(StatFlags & STAT_Lock ) )
				GStats.ClearStats();
			GStats.ResetFrameTimeWindow();
			return 1;
		}
		else
			return 0;
#endif		
	}
	else if( ParseCommand(&Cmd,TEXT("CRACKURL")) )
	{
		FURL URL(NULL,Cmd,TRAVEL_Absolute);
		if( URL.Valid )
		{
			Ar.Logf( TEXT("     Protocol: %s"), *URL.Protocol );
			Ar.Logf( TEXT("         Host: %s"), *URL.Host );
			Ar.Logf( TEXT("         Port: %i"), URL.Port );
			Ar.Logf( TEXT("          Map: %s"), *URL.Map );
			Ar.Logf( TEXT("   NumOptions: %i"), URL.Op.Num() );
			for( INT i=0; i<URL.Op.Num(); i++ )
				Ar.Logf( TEXT("     Option %i: %s"), i, *URL.Op(i) );
			Ar.Logf( TEXT("       Portal: %s"), *URL.Portal );
			Ar.Logf( TEXT("       String: '%s'"), *URL.String() );
		}
		else Ar.Logf( TEXT("BAD URL") );
		return 1;
	}
#if 1 //NEW (mdf)	
	else if( ParseCommand(&Cmd,TEXT("GETCOMMANDLINE")) )
	{
		Ar.Logf( TEXT("%s"), appCmdLine() );
		return 1;
	}
#endif	
	else return 0;
	unguard;
}

//
// Key handler.
//
UBOOL UEngine::Key( UViewport* Viewport, EInputKey Key, TCHAR Unicode )
{
	guard(UEngine::Key);

	if( !GIsRunning )
		return false;

	return Viewport->Console && Viewport->Console->KeyType( Key );

	unguard;
}

//
// Input event handler.
//
UBOOL UEngine::InputEvent( UViewport* Viewport, EInputKey iKey, EInputAction State, FLOAT Delta )
{
	guard(UEngine::InputEvent);

	if( !GIsRunning )
		return false;

	// Process it.
	if( Viewport->Console && Viewport->Console->KeyEvent( iKey, State, Delta ) )
	{
		if( State == IST_Release )
			Viewport->Input->PreProcess( iKey, State, Delta );

		// Player console handled it.
		return 1;
	}
	else if
	(	Viewport->Input->PreProcess( iKey, State, Delta )
	&&	Viewport->Input->Process( Viewport->Console ? (FOutputDevice&)*Viewport->Console : *GLog, iKey, State, Delta ) )
	{
		// Input system handled it.
		return 1;
	}
	else
	{
		// Nobody handled it.
		return 0;
	}

	unguard;
}

INT UEngine::ChallengeResponse( INT Challenge )
{
	guard(UEngine::ChallengeResponse);
	return 0;
	unguard;
}

#if DEMOVERSION
INT UEngine::DummyFunctionToBreakCompatibility( INT i )
{
	return i++;
}
#endif

/*-----------------------------------------------------------------------------
	UServerCommandlet.
-----------------------------------------------------------------------------*/

void UServerCommandlet::StaticConstructor()
{
	guard(UServerCommandlet::StaticConstructor);

	LogToStdout = 1;
	IsClient    = 0;
	IsEditor    = 0;
	IsServer    = 1;
	LazyLoad    = 1;

	unguard;
}

INT UServerCommandlet::Main( const TCHAR* Parms )
{
	guard(UServerCommandlet::Main);

	// Language.
	TCHAR Temp[256];
	if( GConfig->GetString( TEXT("Engine.Engine"), TEXT("Language"), Temp, ARRAY_COUNT(Temp) ) )
	UObject::SetLanguage( Temp );

	// Create the editor class.
	UClass* EngineClass = UObject::StaticLoadClass( UEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.GameEngine"), NULL, LOAD_NoFail, NULL );
	UEngine* Engine = ConstructObject<UEngine>( EngineClass );
	Engine->Init();

	// Main loop.
	GIsRunning = 1;
	DOUBLE OldTime = appSeconds();
	DOUBLE SecondStartTime = OldTime;
	INT TickCount = 0;
	while( GIsRunning && !GIsRequestingExit )
	{
		// Clear stats (will also update old stats).
		GStats.Clear();

		// Update the world.
		guard(UpdateWorld);
		DOUBLE NewTime = appSeconds();
		Engine->Tick( NewTime - OldTime );
		OldTime = NewTime;
		TickCount++;
		if( OldTime > SecondStartTime + 1 )
		{
			Engine->CurrentTickRate = (FLOAT)TickCount / (OldTime - SecondStartTime);
			SecondStartTime = OldTime;
			TickCount = 0;
		}
		unguard;

		// Enforce optional maximum tick rate.
		guard(EnforceTickRate);
		FLOAT MaxTickRate = Engine->GetMaxTickRate();
		if( MaxTickRate>0.f )
		{
			FLOAT Delta = (1.f/MaxTickRate) - (appSeconds()-OldTime);
			appSleep( Max(0.f,Delta) );
		}
		unguard;
	}
	GIsRunning = 0;
	return 0;
	unguard;
}

IMPLEMENT_CLASS(UServerCommandlet)


/*-----------------------------------------------------------------------------
	UMasterMD5Commandlet.
-----------------------------------------------------------------------------*/

void UMasterMD5Commandlet::StaticConstructor()
{
	guard(UMasterMD5Commandlet::StaticConstructor);

	LogToStdout     = 1;
	IsClient        = 1;
	IsEditor        = 1;
	IsServer        = 1;
	LazyLoad        = 1;
	ShowErrorCount  = 1;

	unguard;
}

// Process a directory for requested files

INT UMasterMD5Commandlet::ProcessDirectory(FString Directory, const TCHAR* Parms)
{
	guard(UMasterMD5Commandlet::ProcessDirectory);

	FString Wildcard;		
	FString SearchPath;
	if( !ParseToken(Parms,Wildcard,0) )
		appErrorf(TEXT("Source file(s) not specified"));
	do
	{
		INT	NewRevision=0;

		if ( appStrlen(Parms)>0 )
		{
			Parms++; // Skip the space
			NewRevision = appAtoi(Parms);
		}

		SearchPath.Empty();
		SearchPath = FString::Printf(TEXT("%s%s"),*Directory,*Wildcard);

		TArray<FString> FilesFound = GFileManager->FindFiles( *SearchPath, 1, 0 );
		for (INT i=0; i<FilesFound.Num(); i++)
		{
			FString Pkg = FString::Printf(TEXT("%s%s"),*Directory,*FilesFound(i));

			FString MD5Str;
			FString GUID;

			if ( DoQuickMD5(*Pkg,MD5Str,GUID) )
			{
				INT Index=-1;
				for (INT j=0;j<PackageValidation.Num();j++)
				{
					if ( !appStricmp(*GUID, *PackageValidation(j)->PackageID) )
					{
						Index = j;	// Reset the Index to 
						break;
					}
				}

				if (Index==-1)
				{
					debugf(TEXT("Adding New GUID %s for %s"),*GUID, *Pkg);
					debugf(TEXT("   Allowed MD5 0 [%s]"),*MD5Str);

					Index = PackageValidation.Num();
					PackageValidation.AddItem(ConstructObject<UPackageCheckInfo>(UPackageCheckInfo::StaticClass(),OutputPackage,NAME_None,RF_Public));
					PackageValidation(Index)->PackageID = GUID;
					new(PackageValidation(Index)->AllowedIDs)FString(MD5Str);

					PackageValidation(Index)->Native = true;
					PackageValidation(Index)->RevisionLevel = NewRevision;
				}
				else	// GUID Already exists
				{

					bool bFound=false;
					debugf(TEXT("GUID %s already exists.. "), *GUID);
					
					for (INT k=0;k<PackageValidation(Index)->AllowedIDs.Num();k++)
					{
						if ( !appStrcmp(*PackageValidation(Index)->AllowedIDs(k), *MD5Str ) ) 
						{
							bFound=true;
							break;
						}
					}

					if (!bFound)
					{
						debugf(TEXT("   Allowed MD5 %i [%s]"),PackageValidation(Index)->AllowedIDs.Num(),*MD5Str);
						new(PackageValidation(Index)->AllowedIDs)FString(MD5Str);
					}
					else
						debugf(TEXT("   MD5 %s is already allowed"),*MD5Str);
				}
			}
		}
	}
	while( ParseToken(Parms,Wildcard,0) );

	return 0;

	unguard;
}

INT UMasterMD5Commandlet::AddPackagesToDatabase(UBOOL Fresh, const TCHAR* Parms)
{
	guard(UMasterMD5Comandlet::AddPackagesToDatabase);
	PackageValidation.Empty();

	if (!Fresh)
		OutputPackage = LoadPackage( NULL, TEXT("Packages.md5"), 0 );
	
	if (!Fresh && OutputPackage)
    {
		GWarn->Logf( TEXT("Loading existing MD5 information..."));
		
		// Build the PackageValidation Array for quick lookup
	    for( FObjectIterator It; It; ++It )
	    {
		    UPackageCheckInfo *Info = (UPackageCheckInfo *) *It;
		    if(Info && Info->IsIn( OutputPackage ) )
			    PackageValidation.AddItem(Info);
	    }
    }
	else
	{
		GWarn->Logf( TEXT("Creating a new MD5 Database..."));
		OutputPackage = CreatePackage(NULL,TEXT("Packages.md5"));

		if (OutputPackage==NULL)
		{
			GWarn->Logf(TEXT("Failed!"));
			GIsRequestingExit=1;
			return 0;
		}
	}

	TArray<FString> DirsFound = GFileManager->FindFiles( TEXT("..\\*.*"), 0, 1 );
	for (INT i=0; i<DirsFound.Num(); i++)
	{
		FString ThisDir=FString::Printf(TEXT("..\\%s\\"),*DirsFound(i));
		ProcessDirectory(ThisDir,Parms);
	}
	
	DirsFound = GFileManager->FindFiles( TEXT("..\\MD5\\*.*"), 0, 1 );
	for (INT i=0; i<DirsFound.Num(); i++)
	{
		FString ThisDir=FString::Printf(TEXT("..\\MD5\\%s\\"),*DirsFound(i));
		ProcessDirectory(ThisDir,Parms);
	}

	GWarn->Logf( TEXT("=================================================="));
	GWarn->Logf( TEXT(" No of Packages in Array: %i"),PackageValidation.Num());
	GWarn->Logf( TEXT("=================================================="));

	FString Text=TEXT("");
	INT BestRevision = -1;
	for (INT i=0; i< PackageValidation.Num(); i++)
	{
		UPackageCheckInfo* P = PackageValidation(i);
	
		Text += P->PackageID;
		Text += FString::Printf(TEXT("MD5=%s\n\n"),*P->AllowedIDs(0));
		
		if (P->RevisionLevel > BestRevision)
			BestRevision = P->RevisionLevel;

		GWarn->Logf(TEXT("  Package GUID: %s Revision: %i Native %i"),*P->PackageID,P->RevisionLevel,P->Native);
		for (INT j=0;j<P->AllowedIDs.Num();j++)
			GWarn->Logf(TEXT("    MD5 #%i [%s]"),j,*P->AllowedIDs(j));
	}

	GWarn->Logf( TEXT("=================================================="));
	GWarn->Logf( TEXT("This MD5 Database is at revision level %i"),BestRevision);

	appSaveStringToFile(Text,TEXT("Packages.txt"), GFileManager);

	SavePackage(OutputPackage,NULL,RF_Public,TEXT("Packages.md5"),GWarn,NULL);
	GIsRequestingExit=1;
	return 0;

	unguard;
}

INT UMasterMD5Commandlet::ShowDatabase(const TCHAR* Parms)	
{
	guard(UMasterMD5Commandlet::ShowDatabase);

	UBOOL bBareList=false;
	Parms++;
	if (!appStricmp(Parms,TEXT("-b")))
			bBareList=true;

	OutputPackage = LoadPackage( NULL, TEXT("Packages.md5"), 0 );
	
	if (OutputPackage)
    {
		if (!bBareList)
		{
			GWarn->Logf( TEXT("Loading existing MD5 information..."));
			GWarn->Logf( TEXT(" "));
		}

		INT Count = 0;
		INT MaxRevision=-1;

	    for( FObjectIterator It; It; ++It )
	    {
		    UPackageCheckInfo *Info = (UPackageCheckInfo *) *It;
		    if(Info && Info->IsIn( OutputPackage ) )
			{
				// Display information about this PackageCheckInfo

				if (!bBareList)
					GWarn->Logf(TEXT("GUID: %s [rl=%i]"),*Info->PackageID,Info->RevisionLevel);

				for (INT i=0;i<Info->AllowedIDs.Num();i++)
				{
					if (!bBareList)
						GWarn->Logf(TEXT("    id %i: %s"),i,*Info->AllowedIDs(i));
					else
						GWarn->Logf(TEXT("%s\t%s\t%i"),*Info->PackageID,*Info->AllowedIDs(i),Info->RevisionLevel);
				}

				Count++;


				if (Info->RevisionLevel > MaxRevision)
					MaxRevision = Info->RevisionLevel;

			}
	    }

		if (!bBareList)
		{
			GWarn->Logf( TEXT("=================================================="));
			GWarn->Logf( TEXT(" No of Packages in database: %i"),Count);
			GWarn->Logf( TEXT(" Highest Revision Level    : %i"),MaxRevision);
			GWarn->Logf( TEXT("=================================================="));
		}

    }
	else
		GWarn->Logf( TEXT("Master MD5 Database does not exist!"));

	GIsRequestingExit=1;
	return 0;

	unguard;
}

INT UMasterMD5Commandlet::Revision(const TCHAR* Parms)		
{
	guard(UMasterMD5Commandlet::Revision);

	Parms++; // Skip the space

	INT	NewRevision = appAtoi(Parms);
	debugf(TEXT("Revision::Parms [%s][%i]"),Parms,NewRevision);

	if ( NewRevision>=0 )
	{
		OutputPackage = LoadPackage( NULL, TEXT("Packages.md5"), 0 );
		if (OutputPackage)
		{
			GWarn->Logf( TEXT("Loading existing MD5 information..."));
			GWarn->Logf( TEXT(" "));

			for( FObjectIterator It; It; ++It )
			{
				UPackageCheckInfo *Info = (UPackageCheckInfo *) *It;
				if(Info && Info->IsIn( OutputPackage ) )
				{
					Info->RevisionLevel = NewRevision;					
					GWarn->Logf(TEXT("Setting [%s] to revision level %i"),*Info->PackageID,Info->RevisionLevel);
				}
			}
			SavePackage(OutputPackage,NULL,RF_Public,TEXT("Packages.md5"),GWarn,NULL);
			GWarn->Logf( TEXT("=================================================="));
			GWarn->Logf( TEXT(" Database Updated"));
			GWarn->Logf( TEXT("=================================================="));

		}
		else
			GWarn->Logf( TEXT("Master MD5 Database does not exist!"));
	}
	else
		GWarn->Logf(TEXT("Illegal revision number"));

	GIsRequestingExit=1;
	return 0;

	unguard;
}

INT UMasterMD5Commandlet::FullMD5(const TCHAR* Parms)
{

	guard(UMasterMD5Commandlet::FullMD5);

	Parms++;

	GWarn->Logf( TEXT("Performing full MD5 on %s"),Parms);
	GWarn->Logf( TEXT(" "));

	
	FArchive* MD5Ar = GFileManager->CreateFileReader( Parms );
	int BytesToRead;
	if( !MD5Ar )
	{
		GWarn->Logf( TEXT("  ERROR: Could not open %s for reading!"),Parms);
		GWarn->Logf( TEXT(" "));
		return -1;
	}

	BYTE* MD5Buffer = (BYTE*)appMalloc(65535, TEXT(""));

	FMD5Context PMD5Context;
	appMD5Init( &PMD5Context );

	while ( MD5Ar->Tell() < MD5Ar->TotalSize() )
	{
		BytesToRead = MD5Ar->TotalSize() - MD5Ar->Tell();
		if (BytesToRead>65535)
			BytesToRead=65535;

		MD5Ar->Serialize(MD5Buffer, BytesToRead);
		appMD5Update( &PMD5Context, MD5Buffer, BytesToRead);
	}
	BYTE Digest[16];
	appMD5Final( Digest, &PMD5Context );

	// Convert to a string

	FString MD5Str;
	for (int i=0; i<16; i++)
		MD5Str += FString::Printf(TEXT("%02x"), Digest[i]);	

	GWarn->Logf( TEXT("  MD5 = [%s]"),*MD5Str);
	GWarn->Logf( TEXT(" "));

	// Free the buffer

	appFree(MD5Buffer);

	delete MD5Ar;

	return 0;
	unguard;
}

INT UMasterMD5Commandlet::QuickMD5(const TCHAR* Parms)
{
	guard(UMasterMD5Commandlet::QuickMD5);

	Parms++;

	GWarn->Logf( TEXT("Performing Quick MD5 on %s"),Parms);
	GWarn->Logf( TEXT(" "));
	
	FString MD5Str;
	FString Guid;
	
	if (!DoQuickMD5(Parms, MD5Str,Guid) ) // FString::Printf(TEXT("%s"),DoQuickMD5(Parms));
		return 0;

	GWarn->Logf( TEXT("  MD5 = [%s]"),*MD5Str);
	GWarn->Logf( TEXT(" "));

	return 0;
	unguard;

}

//FString UMasterMD5Commandlet::DoQuickMD5(const TCHAR *Parms)
INT UMasterMD5Commandlet::DoQuickMD5(const TCHAR *Parms, FString& MD5, FString& Guid)
{
	guard(UMasterMD5Commandlet::DoQuickMD5);

	FArchive* MD5Ar = GFileManager->CreateFileReader( Parms );
	if( !MD5Ar )
	{
		GWarn->Logf( TEXT("  ERROR: Could not open %s for reading!"),Parms);
		GWarn->Logf( TEXT(" "));
		return 0;
	}

	FPackageFileSummary Summary;
	*MD5Ar << Summary; //Serialize(&Summary, sizeof(Summary));

	if( Summary.NameCount > 0 )
	{
		MD5Ar->Seek( Summary.NameOffset );
		for( INT i=0; i<Summary.NameCount; i++ )
		{
			// Read the name entry from the file.
			FNameEntry NameEntry;
			*MD5Ar << NameEntry; //Serialize(&NameEntry,sizeof(NameEntry));
		}
	}

	INT SizeOfPart1 = MD5Ar->Tell();	// Store the end of NameTable for Part 1 of the QuickMD5
	INT SizeOfPart2 = MD5Ar->TotalSize() - Summary.ImportOffset;
	INT BufSize;

	if (SizeOfPart2 >= SizeOfPart1)
		BufSize = SizeOfPart2;
	else
		BufSize = SizeOfPart1;

	BYTE* MD5Buffer = (BYTE*)appMalloc(BufSize, TEXT(""));

	BYTE QuickMD5Digest[16];	
	FMD5Context PMD5Context;

	appMD5Init( &PMD5Context );

	// MD5 Summary, Generations and Names

	MD5Ar->Seek(0);
	MD5Ar->Serialize(MD5Buffer,SizeOfPart1);	
	appMD5Update( &PMD5Context, MD5Buffer, SizeOfPart1);

	// MD5 Import and Exports

	MD5Ar->Seek(Summary.ImportOffset);
	MD5Ar->Serialize(MD5Buffer,SizeOfPart2);	
	appMD5Update( &PMD5Context, MD5Buffer, SizeOfPart2);
	appMD5Final( QuickMD5Digest, &PMD5Context );

	// Free the buffer

	appFree(MD5Buffer);

	// Diplay the MD5

	INT i;
	MD5 = TEXT("");
	for (i=0; i<16; i++)
		MD5 += FString::Printf(TEXT("%02x"), QuickMD5Digest[i]);	

	Guid = FString::Printf(TEXT("%s"),Summary.Guid.String());
	return 1;
	unguard;
}

INT UMasterMD5Commandlet::Jack(const TCHAR* Parms)
{
	guard(UMasterMD5Comandlet::Jack);


	GWarn->Logf( TEXT("Dumping MD5 for Master Server"),Parms);
	GWarn->Logf( TEXT(" "));

	FString SearchPath;
	if( !ParseToken(Parms,SearchPath,0) )
		appErrorf(TEXT("Source file(s) not specified"));

	FString Revision;
	if (!ParseToken(Parms,Revision,0) )
		appErrorf(TEXT("Revision not specified"));

	Parms++;
	GWarn->Logf( TEXT("Parms: %s %s %s"), *SearchPath, *Revision,Parms);

	TArray<FString> DirsFound = GFileManager->FindFiles( *SearchPath , 1, 0 );

	for (INT i=0; i<DirsFound.Num(); i++)
	{

		FString MD5Str;
		FString GUID;

		if ( DoQuickMD5(*DirsFound(i),MD5Str,GUID) )
		{
			FString FileName = DirsFound(i);
			INT j = FileName.InStr( TEXT("\\"), 1 );
			if( j != -1 )
				FileName = FileName.Mid(j+1);
			GWarn->Logf( TEXT("INSERT INTO packagemd5 (guid, md5, revision, description) VALUES ('%s','%s',%s,'%s %s');"),*GUID, *MD5Str,*Revision, Parms, *FileName );
		}
		else
			GWarn->Logf( TEXT("  Error loaded %s"),*DirsFound(i));
	}
	
	return 0;

	unguard;
}


INT UMasterMD5Commandlet::Main( const TCHAR* Parms )
{
	guard(UMasterMD5Commandlet::Main);

	GWarn->Logf( TEXT("=================================================="));
	GWarn->Logf( TEXT(" MD5 Database Management"));
	GWarn->Logf( TEXT("=================================================="));
	GWarn->Logf( TEXT(" "));

	FString Cmd;
	ParseToken(Parms,Cmd,0);
	
	if ( !appStricmp(*Cmd, TEXT("-a")) )
		AddPackagesToDatabase(false, Parms);	

	else if ( !appStricmp(*Cmd,TEXT("-c")) )
		AddPackagesToDatabase(true, Parms);	

	else if ( !appStricmp(*Cmd, TEXT("-s")) )
		ShowDatabase(Parms);

	else if ( !appStricmp(*Cmd, TEXT("-r")) )
		Revision(Parms);

	else if ( !appStricmp(*Cmd, TEXT("-f")) )
		FullMD5(Parms);

	else if ( !appStricmp(*Cmd, TEXT("-q")) )
		QuickMD5(Parms);

	else if ( !appStricmp(*Cmd, TEXT("-j")) )
		Jack(Parms);

	else
		GWarn->Logf( TEXT("Unknown Command [%s]"),Parms);
	
	return 0;

	unguard;
}

IMPLEMENT_CLASS(UMasterMD5Commandlet);

/*-----------------------------------------------------------------------------
	UGlobalTempObjects.
-----------------------------------------------------------------------------*/

ENGINE_API UGlobalTempObjects* GGlobalTempObjects = NULL;
IMPLEMENT_CLASS(UGlobalTempObjects)

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

