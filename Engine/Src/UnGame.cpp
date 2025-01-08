/*=============================================================================
	UnGame.cpp: Unreal game engine.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"
#include "UnLinker.h"
#include "FTimerManager.h"

#include "SafeBuild.h"	// IP_Checking support. (requires wsock32.lib)

/*-----------------------------------------------------------------------------
	Object class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UGameEngine);

/*-----------------------------------------------------------------------------
	cleanup!!
-----------------------------------------------------------------------------*/

void UGameEngine::PaintProgress()
{
	guard(PaintProgress);

    if( !Client || Client->Viewports.Num()==0 )
        return;

	UViewport* Viewport=Client->Viewports(0);

    if( !Viewport || !Viewport->Canvas || !Viewport->RI )
        return;

    Viewport->Canvas->Update();
	Viewport->RI->Clear(0,FColor(0,0,0),1,1.0f);

	Draw(Viewport);

    //if ( Viewport->Canvas->pCanvasUtil )
    //    Viewport->Canvas->pCanvasUtil->Flush();

	if( Viewport->PendingFrame )
	Viewport->Present();

	unguard;
}

INT UGameEngine::ChallengeResponse( INT Challenge )
{
	guard(UGameEngine::ChallengeResponse);
	return (Challenge*237) ^ (0x93fe92Ce) ^ (Challenge>>16) ^ (Challenge<<16);
	unguard;
}

void UGameEngine::UpdateConnectingMessage()
{
	guard(UGameEngine::UpdateConnectingMessage);
	if( GPendingLevel && Client && Client->Viewports.Num() )
	{
		if( Client->Viewports(0)->Actor->ProgressTimeOut<Client->Viewports(0)->Actor->Level->TimeSeconds )
		{
			TCHAR Msg1[256], Msg2[256];
			if( GPendingLevel->DemoRecDriver )
			{
				appSprintf( Msg1, TEXT("") );
				appSprintf( Msg2, *GPendingLevel->URL.Map );
			}
			else
			{
				appSprintf( Msg1, LocalizeProgress(TEXT("ConnectingText"),TEXT("Engine")) );
				appSprintf( Msg2, LocalizeProgress(TEXT("ConnectingURL"),TEXT("Engine")), *GPendingLevel->URL.Host, *GPendingLevel->URL.Map );
			}
			SetProgress( Msg1, Msg2, 60.f );
		}
	}
	unguard;
}
void UGameEngine::BuildServerMasterMap( UNetDriver* NetDriver, ULevel* InLevel )
{
	guard(UGameEngine::BuildServerMasterMap);
	check(NetDriver);
	check(InLevel);
	BeginLoad();
	{
		// Init LinkerMap.
		check(InLevel->GetLinker());
		NetDriver->MasterMap->AddLinker( InLevel->GetLinker() );

		// Load server-required packages.
		for( INT i=0; i<ServerPackages.Num(); i++ )
		{
			debugf( TEXT("Server Package: %s"), *ServerPackages(i) );
			ULinkerLoad* Linker = GetPackageLinker( NULL, *ServerPackages(i), LOAD_NoFail, NULL, NULL );
			if( NetDriver->MasterMap->AddLinker( Linker )==INDEX_NONE )
				debugf( TEXT("   (server-side only)") );
		}

		// Add GameInfo's package to map.
		check(InLevel->GetLevelInfo());
		check(InLevel->GetLevelInfo()->Game);
		check(InLevel->GetLevelInfo()->Game->GetClass()->GetLinker());
		NetDriver->MasterMap->AddLinker( InLevel->GetLevelInfo()->Game->GetClass()->GetLinker() );

		// Precompute linker info.
		NetDriver->MasterMap->Compute();
	}
	EndLoad();
	unguard;
}

/*-----------------------------------------------------------------------------
	Game init and exit.
-----------------------------------------------------------------------------*/

//
// Construct the game engine.
//
UGameEngine::UGameEngine()
: LastURL(TEXT(""))
{}

#if IP_CHECKING

static UBOOL s_bAuthorized=false;
static INT ValidIP [][4] = {
#if LEGEND_BUILD
	{ 66,95,39,-1 },	/* Legend subnet mask */
#endif
#if INFOGRAMES_BUILD
	{ 10,121,3,-1 },	/* Infogrames subnet mask */
#endif
#if TIM_HESS
	{ 10,121,3,38 },	/* Tim Hess */
#endif
#if ARON_DRAYER
	{ 10,121,3,x },		/* Aron Drayer */
#endif
#if MATT_POWERS
	{ 10,121,3,20 },	/* Matt Powers */
#endif
#if EPIC_BUILD
	{ 65,168,118,-1 },	/* Epic subnet mask */
#endif
#if NVIDIA_BUILD		/* nVidia developers */
	{ 172,16,194,16 },
	{ 172,16,193,173 },
	{ 172,16,194,52 },
#endif
#if ATARI_QA			/* Atari QA subnet masks */
	{ 10,21,50,-1 },
	{ 10,71,55,-1 },
#endif
};

static UBOOL GetHostIP( in_addr& HostAddr )
{
	guard(GetHostIP);
	UBOOL bSuccess=0;
	WSAData wsaData;
	char HostName[256];
	hostent* HostEnt;
	if(WSAStartup(MAKEWORD(1,1),&wsaData) != SOCKET_ERROR)
	{
		if(gethostname(HostName,sizeof(HostName)) != SOCKET_ERROR)
		{
			HostEnt = gethostbyname(HostName); 
			if(HostEnt && HostEnt->h_addrtype == PF_INET)
			{
				HostAddr = *(in_addr*)(*HostEnt->h_addr_list);
				bSuccess=1;
			}
		}
	}
	WSACleanup();
	return bSuccess;
	unguard;
}
#define IP(sin_addr,n) sin_addr.S_un.S_un_b.s_b##n
static void ValidateIP()
{
//	guard(ValidateIP);

	// s_bAuthorized=true; return; // DO NOT CHECK IN UNLESS COMMENTED OUT!

	struct in_addr Addr;
	if( GetHostIP(Addr) ){
		for(INT i=0;i<ARRAY_COUNT(ValidIP);i++)
			if
			(	(ValidIP[i][0]==-1 || ValidIP[i][0]==IP(Addr,1))
			&&	(ValidIP[i][1]==-1 || ValidIP[i][1]==IP(Addr,2))
			&&	(ValidIP[i][2]==-1 || ValidIP[i][2]==IP(Addr,3))
			&&	(ValidIP[i][3]==-1 || ValidIP[i][3]==IP(Addr,4))
			)	{ s_bAuthorized=true; return; } //valid
	}
	appErrorf(TEXT("D3D Driver: Requires newer version of DirectX."));	// diversionary tactics.  :)
//	unguard;
}
#undef IP
#endif

//
// Initialize the game engine.
//
void UGameEngine::Init()
{
	guard(UGameEngine::Init);
	if(sizeof(*this) != GetClass()->GetPropertiesSize())
		appErrorf(TEXT("GameEngine size mismatch: UScript=%i Native=%i"),GetClass()->GetPropertiesSize(),sizeof(*this));

	// Call base.
	UEngine::Init();

#if IP_CHECKING
	// Validate user's ip address.
	ValidateIP();
#endif

	// Init variables.
	GLevel = NULL;

	// Delete temporary files in cache.
	appCleanFileCache();

	// If not a dedicated server.
	if( GIsClient )
	{	
		// Init client.
		LoadClass<UClient>( NULL, TEXT("Engine.Client"), NULL, LOAD_NoFail, NULL );
		UClass* ClientClass = StaticLoadClass( UClient::StaticClass(), NULL, TEXT("ini:Engine.Engine.ViewportManager"), NULL, LOAD_NoFail, NULL );
		Client = ConstructObject<UClient>( ClientClass );
		VERIFY_CLASS_OFFSET(U,Client,LastCurrent);
		Client->Init( this );

		// Init Render Device
		UClass* RenDevClass = StaticLoadClass( URenderDevice::StaticClass(), NULL, TEXT("ini:Engine.Engine.RenderDevice"), NULL, LOAD_NoFail, NULL );
		GRenDev = ConstructObject<URenderDevice>( RenDevClass );
		GRenDev->Init();
	}
    
#ifdef WITH_KARMA
    KInitGameKarma(); // Init Karma physics.
#endif

	// Load the entry level.
	FString Error;
	
	// Add code to load Packages.MD5

	MD5Package = LoadPackage( NULL, TEXT("Packages.md5"), 0 );

	if (!MD5Package)
        appErrorf(LocalizeError(TEXT("FailedMD5Load"),TEXT("Engine")), TEXT("Packages.md5"));
    else
    {
	    // Build the PackageValidation Array for quick lookup
	    for( FObjectIterator It; It; ++It )
	    {
		    UPackageCheckInfo *Info = Cast<UPackageCheckInfo>(*It);
		    if(Info && Info->IsIn( MD5Package ) )
			    PackageValidation.AddItem(Info);
	    }
    }

	DefaultMD5();

	bCheatProtection = !(ParseParam( appCmdLine(), TEXT("nocheat") ) );	
	if (!bCheatProtection)
		GWarn->Logf(TEXT("Cheat protection disabled"));

	// For Later

	if( Client )
	{
		if( !LoadMap( FURL(TEXT("Entry")), NULL, NULL, Error ) )
			appErrorf( LocalizeError(TEXT("FailedBrowse"),TEXT("Engine")), TEXT("Entry"), *Error );
		Exchange( GLevel, GEntry );
	}

	// Create default URL.
	FURL DefaultURL;
	DefaultURL.LoadURLConfig( TEXT("DefaultPlayer"), TEXT("User") );

	// Enter initial world.
	TCHAR Parm[4096]=TEXT("");
	const TCHAR* Tmp = appCmdLine();
	if
	(	!ParseToken( Tmp, Parm, ARRAY_COUNT(Parm), 0 )
	||	(appStricmp(Parm,TEXT("SERVER"))==0 && !ParseToken( Tmp, Parm, ARRAY_COUNT(Parm), 0 ))
	||	Parm[0]=='-' )
		appStrcpy( Parm, *FURL::DefaultLocalMap );
	FURL URL( &DefaultURL, Parm, TRAVEL_Partial );
	if( !URL.Valid )
		appErrorf( LocalizeError(TEXT("InvalidUrl"),TEXT("Engine")), Parm );
	UBOOL Success = Browse( URL, NULL, Error );

	// If waiting for a network connection, go into the starting level.
	if( !Success && Error==TEXT("") && appStricmp( Parm, *FURL::DefaultLocalMap )!=0 )
		Success = Browse( FURL(&DefaultURL,*FURL::DefaultLocalMap,TRAVEL_Partial), NULL, Error );

	// Handle failure.
	if( !Success )
		appErrorf( LocalizeError(TEXT("FailedBrowse"),TEXT("Engine")), Parm, *Error );

	// Open initial Viewport.
	if( Client )
	{
		// Init input.!!Temporary
		UInput::StaticInitInput();

		// Create viewport.
		UViewport* Viewport = Client->NewViewport( NAME_None );

		// Create console.
		UClass* ConsoleClass = StaticLoadClass( UConsole::StaticClass(), NULL, TEXT("ini:Engine.Engine.Console"), NULL, LOAD_NoFail, NULL );
		Viewport->Console = ConstructObject<UConsole>( ConsoleClass );

		// Initialize Audio.
		InitAudio();
		if( Audio )
			Audio->SetViewport( Viewport );

		// Spawn play actor.
		FString Error;
		if( !GLevel->SpawnPlayActor( Viewport, ROLE_SimulatedProxy, URL, Error ) )
			appErrorf( TEXT("%s"), *Error );

		// Set Audio Viewport again so static ambientsounds can be cached.
		if( Audio )
			Audio->SetViewport( Viewport );

		Viewport->Console->_Init( Viewport );

		Viewport->Input->Init( Viewport );
		Viewport->OpenWindow( 0, 0, (INT) INDEX_NONE, (INT) INDEX_NONE, (INT) INDEX_NONE, (INT) INDEX_NONE );
		GLevel->DetailChange(
			Viewport->RenDev->SuperHighDetailActors ? DM_SuperHigh :
			Viewport->RenDev->HighDetailActors ? DM_High :
			DM_Low
			);

	}
	debugf( NAME_Init, TEXT("Game engine initialized") );

	unguard;
}

//
// Game exit.
//
void UGameEngine::Destroy()
{
	guard(UGameEngine::Destroy);

	// Game exit.
	if( GPendingLevel )
		CancelPending();
	GLevel = NULL;
	debugf( NAME_Exit, TEXT("Game engine shut down") );

#ifdef WITH_KARMA
    KGData->bShutdownPending = 1;
    KTermGameKarma();
#endif

	Super::Destroy();
	unguard;
}

//
// Progress text.
//
void UGameEngine::SetProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds )
{
	guard(UGameEngine::SetProgress);
	if( Client && Client->Viewports.Num() )
	{
		APlayerController* Actor = Client->Viewports(0)->Actor;
#if 0 //!!MERGE
		if( Seconds==-1.f )
		{
			// Upgrade message.
			if ( Actor->myHUD )
				Actor->myHUD->eventShowUpgradeMenu();
		}
#endif
		Actor->eventSetProgressMessage(0, Str1, FColor(255,255,255));
		Actor->eventSetProgressMessage(1, Str2, FColor(255,255,255));
		Actor->eventSetProgressTime(Seconds);
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	Command line executor.
-----------------------------------------------------------------------------*/

//
// This always going to be the last exec handler in the chain. It
// handles passing the command to all other global handlers.
//
UBOOL UGameEngine::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UGameEngine::Exec);
	const TCHAR* Str=Cmd;
	if( ParseCommand( &Str, TEXT("OPEN") ) )
	{
		FString Error;
		if( Client && Client->Viewports.Num() )
			SetClientTravel( Client->Viewports(0), Str, 0, TRAVEL_Partial );
		else
		if( !Browse( FURL(&LastURL,Str,TRAVEL_Partial), NULL, Error ) && Error!=TEXT("") )
			Ar.Logf( TEXT("Open failed: %s"), *Error );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("START") ) )
	{
		FString Error;
		if( Client && Client->Viewports.Num() )
			SetClientTravel( Client->Viewports(0), Str, 0, TRAVEL_Absolute );
		else
		if( !Browse( FURL(&LastURL,Str,TRAVEL_Absolute), NULL, Error ) && Error!=TEXT("") )
			Ar.Logf( TEXT("Start failed: %s"), *Error );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("SERVERTRAVEL") ) && (GIsServer && !GIsClient) )
	{
		GLevel->GetLevelInfo()->eventServerTravel(Str,0);
		return 1;
	}
	else if( (GIsServer && !GIsClient) && ParseCommand( &Str, TEXT("SAY") ) )
	{
		GLevel->GetLevelInfo()->Game->eventBroadcast(NULL, Str, NAME_None, NULL);
		return 1;
	}
	else if( ParseCommand(&Str, TEXT("DISCONNECT")) )
	{
		FString Error;
		if( Client && Client->Viewports.Num() )
		{
			if( GLevel && GLevel->NetDriver && GLevel->NetDriver->ServerConnection && GLevel->NetDriver->ServerConnection->Channels[0] )
			{
				GLevel->NetDriver->ServerConnection->Channels[0]->Close();
				GLevel->NetDriver->ServerConnection->FlushNet();
			}
			if( GPendingLevel && GPendingLevel->NetDriver && GPendingLevel->NetDriver->ServerConnection && GPendingLevel->NetDriver->ServerConnection->Channels[0] )
			{
				GPendingLevel->NetDriver->ServerConnection->Channels[0]->Close();
				GPendingLevel->NetDriver->ServerConnection->FlushNet();
			}
			SetClientTravel( Client->Viewports(0), TEXT("?failed"), 0, TRAVEL_Absolute );
		}
		else
		if( !Browse( FURL(&LastURL,TEXT("?failed"),TRAVEL_Absolute), NULL, Error ) && Error!=TEXT("") )
			Ar.Logf( TEXT("Disconnect failed: %s"), *Error );
		return 1;
	}
	else if( ParseCommand(&Str, TEXT("RECONNECT")) )
	{
		FString Error;
		if( Client && Client->Viewports.Num() )
			SetClientTravel( Client->Viewports(0), *LastURL.String(), 0, TRAVEL_Absolute );
		else
		if( !Browse( FURL(LastURL), NULL, Error ) && Error!=TEXT("") )
			Ar.Logf( TEXT("Reconnect failed: %s"), *Error );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("EXIT")) || ParseCommand(&Cmd,TEXT("QUIT")))
	{
		if( GLevel && GLevel->NetDriver && GLevel->NetDriver->ServerConnection && GLevel->NetDriver->ServerConnection->Channels[0] )
		{
			GLevel->NetDriver->ServerConnection->Channels[0]->Close();
			GLevel->NetDriver->ServerConnection->FlushNet();
		}
		if( GPendingLevel && GPendingLevel->NetDriver && GPendingLevel->NetDriver->ServerConnection && GPendingLevel->NetDriver->ServerConnection->Channels[0] )
		{
			GPendingLevel->NetDriver->ServerConnection->Channels[0]->Close();
			GPendingLevel->NetDriver->ServerConnection->FlushNet();
		}
		Ar.Log( TEXT("Closing by request") );
		appRequestExit( 0 );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("GETCURRENTTICKRATE") ) )
	{
		Ar.Logf( TEXT("%f"), CurrentTickRate );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("GETMAXTICKRATE") ) )
	{
		Ar.Logf( TEXT("%f"), GetMaxTickRate() );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("GSPYLITE") ) )
	{
		FString Error;
		appLaunchURL( TEXT("GSpyLite.exe"), TEXT(""), &Error );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("SAVEGAME")) )
	{
		if( appIsDigit(Str[0]) )
			SaveGame( appAtoi(Str) );
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("CANCEL") ) )
	{
		static UBOOL InCancel = 0;
		if( !InCancel )	
		{
			//!!Hack for broken Input subsystem.  JP.
			//!!Inside LoadMap(), ResetInput() is called,
			//!!which can retrigger an Exec call.
			InCancel = 1;
			if( GPendingLevel )
			{
				if( GPendingLevel->TrySkipFile() )
				{
					InCancel = 0;
					return 1;
				}
				SetProgress( LocalizeProgress(TEXT("CancelledConnect"),TEXT("Engine")), TEXT(""), 2.f );
			}
			else
				SetProgress( TEXT(""), TEXT(""), 0.f );
			CancelPending();
			InCancel = 0;
		}
		return 1;
	}
    else if( ParseCommand(&Cmd,TEXT("SOUND_REBOOT"))  )
    {
        if( Audio )
        {
            UViewport* Viewport = Audio->GetViewport();
            delete Audio;
            InitAudio();
            if( Audio )
                Audio->SetViewport( Viewport );
        }
		return 1;
    }
	else if( GLevel && GLevel->Exec( Cmd, Ar ) )
	{
		return 1;
	}
	else if( GLevel && GLevel->GetLevelInfo()->Game && GLevel->GetLevelInfo()->Game->ScriptConsoleExec(Cmd,Ar,NULL) )
	{
		return 1;
	}
	else
	{
		// disallow set of pawn property if network game
		INT AllowSet = 1;
		if ( GLevel && (GLevel->GetLevelInfo()->NetMode == NM_Client) )
		{
			const TCHAR *Str = Cmd;
			if ( ParseCommand(&Str,TEXT("SET")) )
			{
				TCHAR ClassName[256];
				UClass* Class;
				if
				(	ParseToken( Str, ClassName, ARRAY_COUNT(ClassName), 1 )
				&&	(Class=FindObject<UClass>( ANY_PACKAGE, ClassName))!=NULL )
				{
					TCHAR PropertyName[256];
					UProperty* Property;
					if
					(	ParseToken( Str, PropertyName, ARRAY_COUNT(PropertyName), 1 )
					&&	(Property=FindField<UProperty>( Class, PropertyName))!=NULL )
					{
					if ( Class->IsChildOf(AActor::StaticClass()) 
							&& !Class->IsChildOf(AGameInfo::StaticClass()) 
							&& !(Property->PropertyFlags & (CPF_Config|CPF_GlobalConfig)) )
						{
						AllowSet = 0;
							debugf(
								TEXT("Not allowing set command: %u %u %u"),
								Class->IsChildOf(AActor::StaticClass()),
								!Class->IsChildOf(AGameInfo::StaticClass()),
								!(Property->PropertyFlags & (CPF_Config|CPF_GlobalConfig))
								);
						}
					}
				}
			}
		}
		if( AllowSet && UEngine::Exec( Cmd, Ar ) )
			return 1;
		else
			return 0;
	}

	unguard;
}

/*-----------------------------------------------------------------------------
	Serialization.
-----------------------------------------------------------------------------*/

//
// Serializer.
//
void UGameEngine::Serialize( FArchive& Ar )
{
	guard(UGameEngine::Serialize);
	Super::Serialize( Ar );

	Ar << GLevel << GEntry << GPendingLevel;

	unguardobj;
}

/*-----------------------------------------------------------------------------
	Game entering.
-----------------------------------------------------------------------------*/

//
// Cancel pending level.
//
void UGameEngine::CancelPending()
{
	guard(UGameEngine::CancelPending);
	if( GPendingLevel )
	{
		if( GPendingLevel->NetDriver && GPendingLevel->NetDriver->ServerConnection && GPendingLevel->NetDriver->ServerConnection->Channels[0] )
		{
			GPendingLevel->NetDriver->ServerConnection->Channels[0]->Close();
			GPendingLevel->NetDriver->ServerConnection->FlushNet();
		}
		delete GPendingLevel;
		GPendingLevel = NULL;
	}
	unguard;
}

//
// Match Viewports to actors.
//
static void MatchViewportsToActors( UClient* Client, ULevel* Level, const FURL& URL )
{
	guard(MatchViewportsToActors);

	for( INT i=0; i<Client->Viewports.Num(); i++ )
	{
		FString Error;
		UViewport* Viewport = Client->Viewports(i);
		debugf( NAME_Log, TEXT("Spawning new actor for Viewport %s"), Viewport->GetName() );
		if( !Level->SpawnPlayActor( Viewport, ROLE_SimulatedProxy, URL, Error ) )
			appErrorf( TEXT("%s"), *Error );
	}
	unguardf(( TEXT("(%s)"), *Level->URL.Map ));
}

void UGameEngine::DefaultMD5()
{
	guard(GameEngine::DefaultMD5);

	TArray<FString> EditPackages;
	TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate(TEXT("Editor.EditorEngine"),0,1);
	Sec->MultiFind( FString(TEXT("EditPackages")), EditPackages );

	UMasterMD5Commandlet* MasterMD5 = new UMasterMD5Commandlet;

	FString MD5;
	FString GUID;

	for (INT i=0; i<EditPackages.Num(); i++)
	{
		if (MasterMD5->DoQuickMD5(*FString::Printf(TEXT("%s.u"),*EditPackages(i)), MD5, GUID) )
			AddMD5(*GUID,*MD5,0);
	}

	unguard;
}

void UGameEngine::AddMD5(const TCHAR* GUID, const TCHAR* MD5, int Revision)
{
	guard(GameEngine::AddMD5);

	for (INT i=0;i<PackageValidation.Num();i++)
	{
		if (!appStricmp(GUID, *PackageValidation(i)->PackageID) )
		{
			if (PackageValidation(i)->RevisionLevel < Revision)
			{
				debugf(TEXT("Updating Revision level for %s to %i"),GUID, Revision);
				PackageValidation(i)->RevisionLevel = Revision;
			}

			for (INT j=0;j<PackageValidation(i)->AllowedIDs.Num();j++)
			{
				if (!appStricmp(MD5, *PackageValidation(i)->AllowedIDs(j)) )
				{
					debugf(TEXT("Updating Revision level for %s to %i"),GUID, Revision);
					return;		// Already in DB
				}
			}

			debugf(TEXT("Updating MD5 for %s [%s]"),GUID,MD5);
			new(PackageValidation(i)->AllowedIDs)FString( FString::Printf(TEXT("%s"),MD5));
			return;
		}
	}

	// Add everything

	debugf(TEXT("Adding MD5 Entry for %s [%s]"),GUID,MD5);

	INT Index = PackageValidation.Num();
	PackageValidation.AddItem(ConstructObject<UPackageCheckInfo>(UPackageCheckInfo::StaticClass(),MD5Package,NAME_None,RF_Public));
	PackageValidation(Index)->PackageID = FString::Printf(TEXT("%s"),GUID);
	new(PackageValidation(Index)->AllowedIDs)FString(FString::Printf(TEXT("%s"),MD5));

	PackageValidation(Index)->Native = true;
	PackageValidation(Index)->RevisionLevel = Revision;

	unguard;
}

// 
// Save the MD5 database to disk

void UGameEngine::SaveMD5Database()
{
	guard(GameEngine::SaveMD5Database);

	if (MD5Package)
	{
		debugf(TEXT("Saving Packages.md5..."));
		SavePackage(MD5Package,NULL,RF_Public,TEXT("Packages.md5"),GWarn,NULL);
	}
	else
		debugf(TEXT("Could not save Packages.MD5"));

	unguard;
}

//
// Updates the MD5 database with any MD5's loaded in memory.

void UGameEngine::ServerUpdateMD5()
{

	guard(UGameEngine::ServerUpdateMD5);

	// Check to see if any "Non-Approved" packages are laying around

	TArray<UObject*> ObjLoaders = UObject::GetLoaderList(); 

	int Found;
	for( INT i=0; i<ObjLoaders.Num(); i++ )
	{
		ULinker* Linker = CastChecked<ULinker>( ObjLoaders(i) );
		if ( Linker->LinksToCode() )
		{
			Found=-1;
			for (INT k=0;k<PackageValidation.Num();k++)
			{
				if ( !appStricmp(Linker->Summary.Guid.String(),*PackageValidation(k)->PackageID) )
				{
					Found = k;
					break;
				}
			}

			if (Found>=0)	// Check the MD5
			{
				INT l;
				for (l=0;l<PackageValidation(Found)->AllowedIDs.Num();l++)
				{
					if ( !appStricmp(*PackageValidation(Found)->AllowedIDs(l),*Linker->QuickMD5()) )
					{
						Found=-1;
						break;
					}
				}

				if ( Found>=0 )	// We need to add this id
				{
					new(PackageValidation(Found)->AllowedIDs)FString(Linker->QuickMD5());
					debugf(TEXT("Adding temporary MD5 entry for %s"),Linker->LinkerRoot->GetFullName());
				}
			}
			else	// Create a whole new entry
			{

				Found = PackageValidation.Num();
				PackageValidation.AddItem(ConstructObject<UPackageCheckInfo>(UPackageCheckInfo::StaticClass(),MD5Package,NAME_None,RF_Public));
				PackageValidation(Found)->PackageID = FString::Printf(TEXT("%s"),Linker->Summary.Guid.String());
				new(PackageValidation(Found)->AllowedIDs)FString(Linker->QuickMD5());
				PackageValidation(Found)->Native = false;
				PackageValidation(Found)->RevisionLevel = 0;

				debugf(TEXT("Adding temporary MD5 Database record for %s [%s]"),Linker->LinkerRoot->GetFullName(),*Linker->QuickMD5());
			}
				
		}
	}

	unguard;


}

//
// Browse to a specified URL, relative to the current one.
//
UBOOL UGameEngine::Browse( FURL URL, const TMap<FString,FString>* TravelInfo, FString& Error )
{
	guard(UGameEngine::Browse);
	Error = TEXT("");
	const TCHAR* Option;

	// Tear down voice chat.
	if( Audio )
		Audio->LeaveVoiceChat();

	// Convert .unreal link files.
	const TCHAR* LinkStr = TEXT(".unreal");//!!
	if( appStrstr(*URL.Map,LinkStr)-*URL.Map==appStrlen(*URL.Map)-appStrlen(LinkStr) )
	{
		debugf( TEXT("Link: %s"), *URL.Map );
		FString NewUrlString;
		if( GConfig->GetString( TEXT("Link")/*!!*/, TEXT("Server"), NewUrlString, *URL.Map ) )
		{
			// Go to link.
			URL = FURL( NULL, *NewUrlString, TRAVEL_Absolute );//!!
		}
		else
		{
			// Invalid link.
			guard(InvalidLink);
			Error = FString::Printf( LocalizeError(TEXT("InvalidLink"),TEXT("Engine")), *URL.Map );
			unguard;
			return 0;
		}
	}

	// Crack the URL.
	debugf( TEXT("Browse: %s"), *URL.String() );

	// Handle it.
	if( !URL.Valid )
	{
		// Unknown URL.
		guard(UnknownURL);
		Error = FString::Printf( LocalizeError(TEXT("InvalidUrl"),TEXT("Engine")), *URL.String() );
		unguard;
		return 0;
	}
	else if( URL.HasOption(TEXT("failed")) || URL.HasOption(TEXT("entry")) )
	{
		// Handle failure URL.
		guard(FailedURL);
		debugf( NAME_Log, LocalizeError(TEXT("AbortToEntry"),TEXT("Engine")) );
		if( GLevel && GLevel!=GEntry )
			ResetLoaders( GLevel->GetOuter(), 1, 0 );
		NotifyLevelChange();
		GLevel = GEntry;
		GLevel->GetLevelInfo()->LevelAction = LEVACT_None;
		check(Client && Client->Viewports.Num());
		MatchViewportsToActors( Client, GLevel, URL );
		if( Audio )
			Audio->SetViewport( Audio->GetViewport() );
		//CollectGarbage( RF_Native ); // Causes texture corruption unless you flush.
		if( URL.HasOption(TEXT("failed")) )
		{
			if( !GPendingLevel )
				SetProgress( LocalizeError(TEXT("ConnectionFailed"),TEXT("Engine")), TEXT(""), 6.f );
		}
		unguard;
		return 1;
	}
	else if( URL.HasOption(TEXT("pop")) )
	{
		// Pop the hub.
		guard(PopURL);
		if( GLevel && GLevel->GetLevelInfo()->HubStackLevel>0 )
		{
			TCHAR Filename[256], SavedPortal[256];
			appSprintf( Filename, TEXT("%s") PATH_SEPARATOR TEXT("Game%i.usa"), *GSys->SavePath, GLevel->GetLevelInfo()->HubStackLevel-1 );
			appStrcpy( SavedPortal, *URL.Portal );
			URL = FURL( &URL, Filename, TRAVEL_Partial );
			URL.Portal = SavedPortal;
		}
		else return 0;
		unguard;
	}
	else if( URL.HasOption(TEXT("restart")) )
	{
		// Handle restarting.
		guard(RestartURL);
		URL = LastURL;
		unguard;
	}
	else if( (Option=URL.GetOption(TEXT("load="),NULL))!=NULL )
	{
		// Handle loadgame.
		guard(LoadURL);
		FString Error, Temp=FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("Save%i.usa?load"), *GSys->SavePath, appAtoi(Option) );
		if( LoadMap(FURL(&LastURL,*Temp,TRAVEL_Partial),NULL,NULL,Error) )
		{
			// Copy the hub stack.
			INT i;
			for( i=0; i<GLevel->GetLevelInfo()->HubStackLevel; i++ )
			{
				TCHAR Src[256], Dest[256];//!!
				appSprintf( Src, TEXT("%s") PATH_SEPARATOR TEXT("Save%i%i.usa"), *GSys->SavePath, appAtoi(Option), i );
				appSprintf( Dest, TEXT("%s") PATH_SEPARATOR TEXT("Game%i.usa"), *GSys->SavePath, i );
				GFileManager->Copy( Src, Dest );
			}
			while( 1 )
			{
				Temp = FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("Game%i.usa"), *GSys->SavePath, i++ );
				if( GFileManager->FileSize(*Temp)<=0 )
					break;
				GFileManager->Delete( *Temp );
			}
			LastURL = GLevel->URL;
			return 1;
		}
		else return 0;
		unguard;
	}

	// Handle normal URL's.
	if( URL.IsLocalInternal() )
	{
		// Local map file.
		guard(LocalMapURL);
		UBOOL MapRet = LoadMap( URL, NULL, TravelInfo, Error )!=NULL;

		//if (MapRet && GIsServer)
		//	ServerUpdateMD5();

		return MapRet;
		unguard;
	}
	else if( URL.IsInternal() && GIsClient )
	{
		// Network URL.
		guard(NetworkURL);
		if( GPendingLevel )
			CancelPending();
		GPendingLevel = new UNetPendingLevel( this, URL );
		if( !GPendingLevel->NetDriver )
		{
			SetProgress( TEXT("Networking Failed"), *GPendingLevel->Error, 6.f );
			delete GPendingLevel;
			GPendingLevel = NULL;
		}
		return 0;
		unguard;
	}
	else if( URL.IsInternal() )
	{
		// Invalid.
		guard(InvalidURL);
		Error = LocalizeError(TEXT("ServerOpen"),TEXT("Engine"));
		unguard;
		return 0;
	}
	else
	{
		// External URL.
		guard(ExternalURL);
		//Client->Viewports(0)->Exec(TEXT("ENDFULLSCREEN"));
		appLaunchURL( *URL.String(), TEXT(""), &Error );
		unguard;
		return 0;
	}
	unguard;
}

//
// Notify that level is changing
//
void UGameEngine::NotifyLevelChange()
{
	guard(UGameEngine::NotifyLevelChange);

	// Make sure cinematic view is turned off when the level changes
	if( Client && Client->Viewports.Num() )
		for( INT x = 0 ; x < Client->Viewports.Num() ; x++ )
			Client->Viewports(x)->bRenderCinematics = 0;

	if( Client && Client->Viewports.Num() && Client->Viewports(0)->Console )
		Client->Viewports(0)->Console->eventNotifyLevelChange();

	if ( Audio )
		Audio->Exec(TEXT("StopMusic"));

	unguard;	
}

// Fixup a map
// hack to post release fix map actor problems without breaking compatibility
void UGameEngine::FixUpLevel( )
{
	debugf(TEXT("Level is %s"), GLevel->GetFullName());
}
//
// Load a map.
//
ULevel* UGameEngine::LoadMap( const FURL& URL, UPendingLevel* Pending, const TMap<FString,FString>* TravelInfo, FString& Error )
{
	guard(UGameEngine::LoadMap);
	Error = TEXT("");
	debugf( NAME_Log, TEXT("LoadMap: %s"), *URL.String() );
	GInitRunaway();

	// Remember current level's stack level.
	INT SavedHubStackLevel = GLevel ? GLevel->GetLevelInfo()->HubStackLevel : 0;

	// Get network package map.
	UPackageMap* PackageMap = NULL;
	if( Pending )
		PackageMap = Pending->GetDriver()->ServerConnection->PackageMap;

	// Verify that we can load all packages we need.
	UObject* MapParent = NULL;
	guard(VerifyPackages);
	try
	{
		BeginLoad();
		if( Pending )
		{
			// Verify that we can load everything needed for client in this network level.
			for( INT i=0; i<PackageMap->List.Num(); i++ )
				PackageMap->List(i).Linker = GetPackageLinker
				(
					PackageMap->List(i).Parent,
					NULL,
					LOAD_Verify | LOAD_Throw | LOAD_NoWarn | LOAD_NoVerify,
					NULL,
					&PackageMap->List(i).Guid
				);
			for( INT i=0; i<PackageMap->List.Num(); i++ )
				VerifyLinker( PackageMap->List(i).Linker );
			if( PackageMap->List.Num() )
				MapParent = PackageMap->List(0).Parent;
		}
		LoadObject<ULevel>( MapParent, TEXT("MyLevel"), *URL.Map, LOAD_Verify | LOAD_Throw | LOAD_NoWarn, NULL );
		EndLoad();
	}
	#if UNICODE
	catch( TCHAR* CatchError )
	#else
	catch( char* CatchError )
	#endif
	{
		// Safely failed loading.
		EndLoad();
		Error = CatchError;
        #if UNICODE
        TCHAR *e = CatchError;
        #else
        TCHAR *e = ANSI_TO_TCHAR(CatchError);
        #endif

#if 1 //NEW (mdf) until SetProgress works -- show reason for failure in log file        
		debugf( L"LoadMap failed: %s", e );
#endif		
		SetProgress( LocalizeError(TEXT("UrlFailed"),TEXT("Core")), e, 6.f );
		return NULL;
	}
	unguard;

	// Display loading screen.
	guard(LoadingScreen);
	if( Client && Client->Viewports.Num() && GLevel && !URL.HasOption(TEXT("quiet")) )
	{
		GLevel->GetLevelInfo()->LevelAction = LEVACT_Loading;
		GLevel->GetLevelInfo()->Pauser = NULL;
		PaintProgress();
		if( Audio )
			Audio->SetViewport( Audio->GetViewport() );
		GLevel->GetLevelInfo()->LevelAction = LEVACT_None;
	}
	unguard;

	// Notify of the level change, before we dissociate Viewport actors
	guard(NotifyLevelChange);
	if( GLevel )
		NotifyLevelChange();
	unguard;

	// Dissociate Viewport actors.
	guard(DissociateViewports);
	if( Client )
	{
		for( INT i=0; i<Client->Viewports.Num(); i++ )
		{
			APlayerController* Actor    = Client->Viewports(i)->Actor;
			ULevel*      Level          = Actor->GetLevel();
			Actor->Player               = NULL;
			Client->Viewports(i)->Actor = NULL;
			if ( Actor->Pawn )
				Level->DestroyActor(Actor->Pawn);
			Level->DestroyActor( Actor );
		}
	}
	unguard;

#if 1 //NEW (mdf)
	//debugf( L"--> LoadMap calling CleanupDeleted #2" );
	UObject::CleanupDeleted();
#endif

	// Clean up game state.
	guard(ExitLevel);
	if( GLevel )
	{
		// Shut down.
		ResetLoaders( GLevel->GetOuter(), 1, 0 );
		if( GLevel->NetDriver )
		{
			delete GLevel->NetDriver;
			GLevel->NetDriver = NULL;
		}
		if( GLevel->DemoRecDriver )
		{
			delete GLevel->DemoRecDriver;
			GLevel->DemoRecDriver = NULL;
		}
		if( URL.HasOption(TEXT("push")) )
		{
			// Save the current level minus players actors.
			GLevel->CleanupDestroyed( 1 );
			TCHAR Filename[256];
			appSprintf( Filename, TEXT("%s") PATH_SEPARATOR TEXT("Game%i.usa"), *GSys->SavePath, SavedHubStackLevel );
			SavePackage( GLevel->GetOuter(), GLevel, 0, Filename, GLog );
		}

#ifdef WITH_KARMA
		if(!GIsEditor) // dont need to do this in editor - no Karma runs.
		{
			// To save memory, we remove Karma from all actors before loading the new level.
			for( INT iActor=0; iActor<GLevel->Actors.Num(); iActor++ )
			{
				AActor* actor = GLevel->Actors(iActor);
				if(actor)
				{
					KTermActorKarma(actor);
				}
			}
		}
#endif

		GLevel = NULL;
	}
	unguard;

	// Load the level and all objects under it, using the proper Guid.
	guard(LoadLevel);
	GLevel = LoadObject<ULevel>( MapParent, TEXT("MyLevel"), *URL.Map, LOAD_NoFail, NULL );
	unguard;

	// If pending network level.
	if( Pending )
	{
		// If playing this network level alone, ditch the pending level.
		if( Pending && Pending->LonePlayer )
			Pending = NULL;

		// Setup network package info.
		PackageMap->Compute();
		for( INT i=0; i<PackageMap->List.Num(); i++ )
			if( PackageMap->List(i).LocalGeneration!=PackageMap->List(i).RemoteGeneration )
				Pending->GetDriver()->ServerConnection->Logf( TEXT("HAVE GUID=%s GEN=%i"), PackageMap->List(i).Guid.String(), PackageMap->List(i).LocalGeneration );
	}

	// Verify classes.
	guard(VerifyClasses);
	UBOOL Mismatch = false;
	#define VERIFY_CLASS_SIZES
	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name)
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#include "EngineClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
	#undef VERIFY_CLASS_SIZES
	if( Mismatch )
		appErrorf(TEXT("C++/ Script class size mismatch"));

	VERIFY_CLASS_OFFSET( A, Actor,       Owner         );
	VERIFY_CLASS_OFFSET( A, Actor,       TimerCounter  );
	VERIFY_CLASS_OFFSET( A, PlayerController,  Player  );
	VERIFY_CLASS_OFFSET( A, Pawn,  Health );
  
    VERIFY_CLASS_SIZE( UCanvas );
    VERIFY_CLASS_SIZE( UCubemap );
    VERIFY_CLASS_SIZE( UEngine );
    VERIFY_CLASS_SIZE( UGameEngine );
    VERIFY_CLASS_SIZE( UPalette );
    VERIFY_CLASS_SIZE( UPlayer );
    VERIFY_CLASS_SIZE( UTexture );
	unguard;

	// Get LevelInfo.
	check(GLevel);
	ALevelInfo* Info = GLevel->GetLevelInfo();
	Info->ComputerName = appComputerName();

	// Handle pushing.
	guard(ProcessHubStack);
	Info->HubStackLevel
	=	URL.HasOption(TEXT("load")) ? Info->HubStackLevel
	:	URL.HasOption(TEXT("push")) ? SavedHubStackLevel+1
	:	URL.HasOption(TEXT("pop" )) ? Max(SavedHubStackLevel-1,0)
	:	URL.HasOption(TEXT("peer")) ? SavedHubStackLevel
	:	                              0;
	unguard;

	// Handle pending level.
	guard(ActivatePending);
	if( Pending )
	{
		check(Pending==GPendingLevel);

		// Hook network driver up to level.
		GLevel->NetDriver = Pending->NetDriver;
		if( GLevel->NetDriver )
			GLevel->NetDriver->Notify = GLevel;

		// Hook demo playback driver to level
		GLevel->DemoRecDriver = Pending->DemoRecDriver;
		if( GLevel->DemoRecDriver )
			GLevel->DemoRecDriver->Notify = GLevel;

		// Setup level.
		GLevel->GetLevelInfo()->NetMode = NM_Client;
	}
	else check(!GLevel->NetDriver);
	unguard;

	// Set level info.
	guard(InitLevel);
	if( !URL.GetOption(TEXT("load"),NULL) )
		GLevel->URL = URL;
	Info->EngineVersion = FString::Printf( TEXT("%i"), ENGINE_VERSION );
	Info->MinNetVersion = FString::Printf( TEXT("%i"), ENGINE_MIN_NET_VERSION );
	GLevel->Engine = this;
	if( TravelInfo )
		GLevel->TravelInfo = *TravelInfo;
	unguard;

	// Remove cubemaps.
#if 1
	if( GLevel->Engine->GRenDev && GLevel->Engine->GRenDev->Is3dfx )
	{
		for( TObjectIterator<UModifier> It; It; ++It )
		{
			if( It->Material && It->Material->IsA(UCubemap::StaticClass()) )
				It->Material = NULL;
		}

		for( TObjectIterator<UCombiner> It; It; ++It )
		{
			if( It->Material1 && It->Material1->IsA(UCubemap::StaticClass()) )
			{
				It->Material1 = It->Material2;
				It->Material2 = NULL;
			}
			if( It->Material2 && It->Material2->IsA(UCubemap::StaticClass()) )
				It->Material2 = NULL;
		}

		for( TObjectIterator<UShader> It; It; ++It )
		{
			if( It->Diffuse && It->Diffuse->IsA(UCubemap::StaticClass()) )
				It->Diffuse = NULL;

			if( It->Opacity && It->Opacity->IsA(UCubemap::StaticClass()) )
				It->Opacity = NULL;

			if( It->Specular && It->Specular->IsA(UCubemap::StaticClass()) )
				It->Specular = NULL;

			if( It->SpecularityMask && It->SpecularityMask->IsA(UCubemap::StaticClass()) )
				It->SpecularityMask = NULL;

			if( It->SelfIllumination && It->SelfIllumination->IsA(UCubemap::StaticClass()) )
				It->SelfIllumination = NULL;

			if( It->SelfIlluminationMask && It->SelfIlluminationMask->IsA(UCubemap::StaticClass()) )
				It->SelfIlluminationMask = NULL;

			if( It->Detail && It->Detail->IsA(UCubemap::StaticClass()) )
				It->Detail = NULL;
		}
	}
#endif

	// Purge unused objects and flush caches.
	guard(Cleanup);
	if( appStricmp(GLevel->GetOuter()->GetName(),TEXT("Entry"))!=0 )
	{
		Flush(0);
		UViewport* Viewport = NULL;
		if ( Audio )
		{
			Viewport = Audio->GetViewport();
			Audio->SetViewport( NULL );
		}
		{for( TObjectIterator<AActor> It; It; ++It )
			if( It->IsIn(GLevel->GetOuter()) )
				It->SetFlags( RF_EliminateObject );}
		{for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) )
				GLevel->Actors(i)->ClearFlags( RF_EliminateObject );}
		CollectGarbage( RF_Native );
		if( Audio )
		{
			for( INT i=0; i<Client->Viewports.Num(); i++ )
			{
				if( Client->Viewports(i) == Viewport )
				{
					Audio->SetViewport( Viewport );
					break;
				}
			}
		}
	}
	unguard;

	// Tell the audio driver to clean up.
	if( Audio )
		Audio->CleanUp();

	// Init collision.
	GLevel->SetActorCollision( 1 );

	// Setup zone distance table for sound damping. Fast enough: Approx 3 msec.
	guard(SetupZoneTable);
	QWORD OldConvConn[64];
	QWORD ConvConn[64];
	for( INT i=0; i<64; i++ )
	{
		for( INT j=0; j<64; j++ )
		{
			OldConvConn[i] = GLevel->Model->Zones[i].Connectivity;
			if( i == j )
				GLevel->ZoneDist[i][j] = 0;
			else
				GLevel->ZoneDist[i][j] = 255;
		}
	}
	for( INT i=1; i<64; i++ )
	{
		for( INT j=0; j<64; j++ )
			for( INT k=0; k<64; k++ )
				if( (GLevel->ZoneDist[j][k] > i) && ((OldConvConn[j] & ((QWORD)1 << k)) != 0) )
					GLevel->ZoneDist[j][k] = i;
		for( INT j=0; j<64; j++ )
			ConvConn[j] = 0;
		for( INT j=0; j<64; j++ )
			for( INT k=0; k<64; k++ )
				if( (OldConvConn[j] & ((QWORD)1 << k)) != 0 )
					ConvConn[j] = ConvConn[j] | OldConvConn[k];
		for( INT j=0; j<64; j++ )
			OldConvConn[j] = ConvConn[j];
	}
	unguard;

	// Update the LevelInfo's time.
	GLevel->UpdateTime(Info);

	// Init the game info.
	TCHAR Options[1024]=TEXT("");
	TCHAR GameClassName[256]=TEXT("");
	FString Error=TEXT("");
	guard(InitGameInfo);
	for( INT i=0; i<URL.Op.Num(); i++ )
	{
		appStrcat( Options, TEXT("?") );
		appStrcat( Options, *URL.Op(i) );
		Parse( *URL.Op(i), TEXT("GAME="), GameClassName, ARRAY_COUNT(GameClassName) );
	}
	if( GLevel->IsServer() && !Info->Game )
	{
		// Get the GameInfo class.
		UClass* GameClass=NULL;
		if ( GameClassName[0] )
			GameClass = StaticLoadClass( AGameInfo::StaticClass(), NULL, GameClassName, NULL, LOAD_NoFail, PackageMap );
		if( !GameClass && Info->DefaultGameType.Len() > 0 )
			GameClass = StaticLoadClass( AGameInfo::StaticClass(), NULL, *(Info->DefaultGameType), NULL, LOAD_NoFail, PackageMap );
		if( !GameClass && appStricmp(GLevel->GetOuter()->GetName(),TEXT("Entry"))==0 )
			GameClass = AGameInfo::StaticClass();
		if( !GameClass ) 
			GameClass = StaticLoadClass( AGameInfo::StaticClass(), NULL, Client ? TEXT("ini:Engine.Engine.DefaultGame") : TEXT("ini:Engine.Engine.DefaultServerGame"), NULL, LOAD_NoFail, PackageMap );
        if ( !GameClass ) 
			GameClass = AGameInfo::StaticClass();

		// Spawn the GameInfo.
		debugf( NAME_Log, TEXT("Game class is '%s'"), GameClass->GetName() );
		Info->Game = (AGameInfo*)GLevel->SpawnActor( GameClass );
		check(Info->Game!=NULL);
	}
	unguard;

	// Listen for clients.
	guard(Listen);
	if( !Client || URL.HasOption(TEXT("Listen")) )
	{
		if( GPendingLevel )
		{
			guard(CancelPendingForListen);
			check(!Pending);
			delete GPendingLevel;
			GPendingLevel = NULL;
			unguard;
		}
		FString Error;
		if( !GLevel->Listen( Error ) )
			appErrorf( LocalizeError(TEXT("ServerListen"),TEXT("Engine")), *Error );
	}
	unguard;

	// Init detail.
	Info->DetailMode = DM_SuperHigh;
	if(Client && Client->Viewports.Num() && Client->Viewports(0)->RenDev)
	{
		if(Client->Viewports(0)->RenDev->SuperHighDetailActors)
			Info->DetailMode = DM_SuperHigh;
		else if(Client->Viewports(0)->RenDev->HighDetailActors)
			Info->DetailMode = DM_High;
		else
			Info->DetailMode = DM_Low;
	}

	// Clear any existing stat graphs.
	if(GStatGraph)
		GStatGraph->Reset();

	// Init level gameplay info.
	guard(BeginPlay);
	GLevel->iFirstDynamicActor = 0;
	if( !Info->bBegunPlay )
	{

		// fix up level problems
		FixUpLevel();

		// Check that paths are valid
		#if 0 //NEW (mdf) disabled for now (not using paths/AI currently)
		if ( !GLevel->GetLevelInfo()->bPathsRebuilt )
			debugf(TEXT("*** WARNING - PATHS MAY NOT BE VALID ***"));
		#endif
		// Lock the level.
		debugf( NAME_Log, TEXT("Bringing %s up for play (%i)..."), GLevel->GetFullName(), appRound(GetMaxTickRate()) );
		GLevel->FinishedPrecaching = 0;
		GLevel->TimeSeconds = 0;
		GLevel->GetLevelInfo()->TimeSeconds = 0;
		GLevel->GetLevelInfo()->GetDefaultPhysicsVolume()->bNoDelete = true;

		// Kill off actors that aren't interesting to the client.
		if( !GLevel->IsServer() )
		{
			for( INT i=0; i<GLevel->Actors.Num(); i++ )
			{
				AActor* Actor = GLevel->Actors(i);
				if( Actor )
				{
					if( Actor->bStatic || Actor->bNoDelete )
						Exchange( Actor->Role, Actor->RemoteRole );
					else
						GLevel->DestroyActor( Actor );
				}
			}
		}

		// Init touching actors & clear LastRenderTime
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) )
			{
				GLevel->Actors(i)->LastRenderTime = 0.f;
				GLevel->Actors(i)->Touching.Empty();
				GLevel->Actors(i)->PhysicsVolume = GLevel->GetLevelInfo()->GetDefaultPhysicsVolume();
			}


		// Init scripting.
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) )
				GLevel->Actors(i)->InitExecution();

		// Enable actor script calls.
		Info->bBegunPlay = 1;
		Info->bStartup = 1;

		// Init the game.
		if( Info->Game )
			Info->Game->eventInitGame( Options, Error );

		// Send PreBeginPlay.
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) && !GLevel->Actors(i)->bScriptInitialized )
				GLevel->Actors(i)->eventPreBeginPlay();

		// Set BeginPlay.
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) && !GLevel->Actors(i)->bScriptInitialized )
				GLevel->Actors(i)->eventBeginPlay();

		// Set zones.
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) && !GLevel->Actors(i)->bScriptInitialized )
				GLevel->Actors(i)->SetZone( 1, 1 );

		// set appropriate volumes for each actor
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) && !GLevel->Actors(i)->bScriptInitialized )
				GLevel->Actors(i)->SetVolumes();

		// Post begin play.
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) && !GLevel->Actors(i)->bScriptInitialized )
			{
				GLevel->Actors(i)->eventPostBeginPlay();

				if(GLevel->Actors(i))
					GLevel->Actors(i)->PostBeginPlay();
			}

		// Post net begin play.
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) && !GLevel->Actors(i)->bScriptInitialized )
				GLevel->Actors(i)->eventPostNetBeginPlay();

		// Begin scripting.
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
			if( GLevel->Actors(i) && !GLevel->Actors(i)->bScriptInitialized )
				GLevel->Actors(i)->eventSetInitialState();

		// Find bases
		for( INT i=0; i<GLevel->Actors.Num(); i++ )
		{
			if( GLevel->Actors(i) ) 
			{
				if ( GLevel->Actors(i)->AttachTag != NAME_None )
				{
					//find actor to attach self onto
					for( INT j=0; j<GLevel->Actors.Num(); j++ )
					{
						if( GLevel->Actors(j) 
							&& ((GLevel->Actors(j)->Tag == GLevel->Actors(i)->AttachTag) || (GLevel->Actors(j)->GetFName() == GLevel->Actors(i)->AttachTag))  )
						{
							GLevel->Actors(i)->SetBase(GLevel->Actors(j), FVector(0,0,1), 0);
							break;
						}
					}
				}
				else if( GLevel->Actors(i)->bCollideWorld && GLevel->Actors(i)->bShouldBaseAtStartup
				 &&	((GLevel->Actors(i)->Physics == PHYS_None) || (GLevel->Actors(i)->Physics == PHYS_Rotating)) )
				{
					 GLevel->Actors(i)->FindBase();
				}
			}
		}

		for( INT i=0; i<GLevel->Actors.Num(); i++ ) 
		{
			if(GLevel->Actors(i))
			{
#ifdef WITH_KARMA
				AActor* actor = GLevel->Actors(i);

				if(actor->Physics != PHYS_Karma || !actor->KParams || !actor->KParams->IsA(UKarmaParams::StaticClass()))
					continue;

				UKarmaParams* kparams = Cast<UKarmaParams>(actor->KParams);

				// If running below HighDetailPhysics, turn off karma dynamics for actors with bHighDetailOnly set true.
				if(	GLevel->GetLevelInfo()->PhysicsDetailLevel < PDL_High && kparams->bHighDetailOnly )
					KTermActorDynamics(actor);

				// If dedicated server, turn off karma for actors with bHighDetailOnly or bClientsOnly
				if(	GLevel->GetLevelInfo()->NetMode == NM_DedicatedServer && (kparams->bHighDetailOnly || kparams->bClientOnly) )					
					KTermActorDynamics(actor);
#endif
			}
		}


		// Preprocess Index/Vertex buffers for skeletal actors currently in memory.
		// #DEBUG - currently disabled.
#if (0)
		for( TObjectIterator<USkeletalMesh> It; It; ++It )
		{
			USkeletalMesh* SkelMesh = *It;				
			debugf(TEXT("D3D-predigesting skeletal mesh: %s"),SkelMesh->GetFullName());
			SkelMesh->RenderPreProcess( Client->HardwareSkinning );
		}
#endif


		Info->bStartup = 0;
	}
	else GLevel->TimeSeconds = GLevel->GetLevelInfo()->TimeSeconds;
	unguard;

	// Rearrange actors: static first, then others.
	guard(Rearrange);
	TArray<AActor*> Actors;
	Actors.AddItem(GLevel->Actors(0));
	Actors.AddItem(GLevel->Actors(1));
	for( INT i=2; i<GLevel->Actors.Num(); i++ )
		if( GLevel->Actors(i) && GLevel->Actors(i)->bStatic && !GLevel->Actors(i)->bAlwaysRelevant )
			Actors.AddItem( GLevel->Actors(i) );
	GLevel->iFirstNetRelevantActor=Actors.Num();
	for( INT i=2; i<GLevel->Actors.Num(); i++ )
		if( GLevel->Actors(i) && GLevel->Actors(i)->bStatic && GLevel->Actors(i)->bAlwaysRelevant )
			Actors.AddItem( GLevel->Actors(i) );
	GLevel->iFirstDynamicActor=Actors.Num();
	for( INT i=2; i<GLevel->Actors.Num(); i++ )
		if( GLevel->Actors(i) && !GLevel->Actors(i)->bStatic )
			Actors.AddItem( GLevel->Actors(i) );
	GLevel->Actors.Empty();
	GLevel->Actors.Add( Actors.Num() );
	for( INT i=0; i<Actors.Num(); i++ )
		GLevel->Actors(i) = Actors(i);
	unguard;

	// Cleanup profiling.
#if USCRIPT_PROFILE
	guard(CleanupProfiling);
	for( TObjectIterator<UFunction> It; It; ++It )
		It->Calls = It->Cycles=0;
	GTicks=1;
	unguard;
#endif

	// Client init.
	guard(ClientInit);
	if( Client )
	{
		// Match Viewports to actors.
		MatchViewportsToActors( Client, GLevel->IsServer() ? GLevel : GEntry, URL );

		// Set up audio.
		if( Audio )
			Audio->SetViewport( Audio->GetViewport() );

		// Reset viewports.
		for( INT i=0; i<Client->Viewports.Num(); i++ )
		{
			UViewport* Viewport = Client->Viewports(i);
			Viewport->Input->ResetInput();
			if( Viewport->RenDev )
				Viewport->RenDev->Flush(Viewport);
		}
	}
	unguard;

	// Reset console.
	if( Client && Client->Viewports.Num() && Client->Viewports(0)->Console )
		Client->Viewports(0)->Console->LevelBegin();

	// Init detail.
	GLevel->DetailChange( (EDetailMode)Info->DetailMode );

	// Remember the URL.
	guard(RememberURL);
	LastURL = URL;
	unguard;

	// Remember DefaultPlayer options.
	if( GIsClient )
	{
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Name" ), TEXT("User") );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Team" ), TEXT("User") );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Class"), TEXT("User") );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Skin" ), TEXT("User") );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Face" ), TEXT("User") );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("Voice" ), TEXT("User") );
		URL.SaveURLConfig( TEXT("DefaultPlayer"), TEXT("OverrideClass" ), TEXT("User") );
	}

#ifdef WITH_KARMA
	// Pre-allocate pool of KarmaTriListData structs
	// Don't bother if bKNoInit is false
	ALevelInfo* lInfo = GLevel->GetLevelInfo();
	if(!lInfo->bKNoInit)
	{
		for(INT i=0; i<lInfo->MaxRagdolls; i++)
		{
			KarmaTriListData* list = (KarmaTriListData*)appMalloc(sizeof(KarmaTriListData), TEXT("RAGDOLL TRILIST"));
			KarmaTriListDataInit(list);
			GLevel->TriListPool.AddItem(list);
		}
	}
#endif

	// Successfully started local level.
	return GLevel;
	unguard;
}

/*-----------------------------------------------------------------------------
	Game Viewport functions.
-----------------------------------------------------------------------------*/

//
// Draw a global view.
//
void UGameEngine::Draw( UViewport* Viewport, UBOOL Blit, BYTE* HitData, INT* HitSize )
{
	guard(UGameEngine::Draw);

	// If not up and running yet, don't draw.
	if(!GIsRunning)
		return;

	clock(GStats.DWORDStats( GEngineStats.STATS_Frame_RenderCycles ));

	// Determine the camera actor, location and rotation.
	AActor*		CameraActor		= Viewport->Actor;
	FVector		CameraLocation	= CameraActor->Location;
	FRotator	CameraRotation	= CameraActor->Rotation;

	Viewport->Actor->eventPlayerCalcView(CameraActor,CameraLocation,CameraRotation);

	if(!CameraActor)
	{
		debugf(TEXT("Warning: NULL CameraActor returned from PlayerCalcView for %s"),Viewport->Actor->GetPathName());
		CameraActor = Viewport->Actor;
	}

	if(Viewport->Actor->XLevel != GLevel)
		return;

	// Render the level.
	UpdateConnectingMessage();

	BYTE SavedAction = Viewport->Actor->Level->LevelAction;
	
	if( Viewport->RenDev->PrecacheOnFlip ) // && !Viewport->Actor->Level->bNeverPrecache )
		Viewport->Actor->Level->LevelAction = LEVACT_Precaching;

	// Present the last frame.
	if( Viewport->PendingFrame && (!(Viewport->RenDev->PrecacheOnFlip) )  )  // && (!Viewport->Actor->Level->bNeverPrecache) )
			Viewport->Present();

	// Precache now if desired.
	if( Viewport->RenDev->PrecacheOnFlip )//&& !Viewport->Actor->Level->bNeverPrecache )
	{
		debugf(TEXT("Precaching: %s"), Viewport->Actor->Level->GetPathName() );
		Viewport->RenDev->PrecacheOnFlip = 0;

		// Request script to fill in dynamic stuff like player skins.
		Viewport->Actor->Level->eventFillPrecacheMaterialsArray();
		Viewport->Actor->Level->eventFillPrecacheStaticMeshesArray();

		Viewport->Precaching = 1;

		DOUBLE StartTime = appSeconds();

		if(Viewport->Lock(HitData,HitSize))
		{
			FPlayerSceneNode	SceneNode(Viewport,&Viewport->RenderTarget,CameraActor,CameraLocation,CameraRotation,Viewport->Actor->FovAngle);

			Viewport->LodSceneNode = &SceneNode;
			Viewport->RI->SetPrecacheMode( PRECACHE_VertexBuffers );
			SceneNode.Render(Viewport->RI);

			// Precache "dynamic" static meshes (e.g. weapon effects).
			for( INT i=0; i<Viewport->Actor->Level->PrecacheStaticMeshes.Num(); i++ )
			{
				// Set vertex and index buffers.
				UStaticMesh*	StaticMesh = Viewport->Actor->Level->PrecacheStaticMeshes(i);
				if( !StaticMesh )
					continue;

				FVertexStream*	VertexStreams[9] = { &StaticMesh->VertexStream };
				INT				NumVertexStreams = 1;

				if( !StaticMesh->VertexStream.Vertices.Num() )
					continue;

				if( StaticMesh->UseVertexColor )
				{
					if( StaticMesh->ColorStream.Colors.Num() )
						VertexStreams[NumVertexStreams++] = &StaticMesh->ColorStream;
				}
				else
				{
					if( StaticMesh->AlphaStream.Colors.Num() )
						VertexStreams[NumVertexStreams++] = &StaticMesh->AlphaStream;
				}

				for(INT UVIndex=0; UVIndex<StaticMesh->UVStreams.Num(); UVIndex++ )
					VertexStreams[NumVertexStreams++] = &StaticMesh->UVStreams(UVIndex);

				if(StaticMesh->TBStream.TBs.Num() > 0)
					VertexStreams[NumVertexStreams++] = &StaticMesh->TBStream;

				Viewport->RI->SetVertexStreams(VS_FixedFunction,VertexStreams,NumVertexStreams);
				Viewport->RI->SetIndexBuffer(&StaticMesh->IndexBuffer,0);

				for( INT MatIndex=0; MatIndex<StaticMesh->Materials.Num(); MatIndex++ )		
					Viewport->Actor->Level->PrecacheMaterials.AddItem( StaticMesh->Materials(MatIndex).Material );
			}
			Viewport->Actor->Level->PrecacheStaticMeshes.Empty();

			Viewport->LodSceneNode = NULL;

			Viewport->Unlock();
		}

		debugf(TEXT("Finished precaching geometry in %5.3f seconds"), (FLOAT) (appSeconds() - StartTime));
		StartTime = appSeconds();

		if(Viewport->Lock(HitData,HitSize))
		{
			FPlayerSceneNode SceneNode(Viewport,&Viewport->RenderTarget,CameraActor,CameraLocation,CameraRotation,Viewport->Actor->FovAngle);

			Viewport->LodSceneNode = &SceneNode;
			Viewport->RI->SetPrecacheMode( PRECACHE_All );
			SceneNode.Render(Viewport->RI);

			// Precache dynamic materials (e.g. player skins).
			for( INT i=0; i<Viewport->Actor->Level->PrecacheMaterials.Num(); i++ )
				Viewport->RI->SetMaterial( Viewport->Actor->Level->PrecacheMaterials(i) );
			Viewport->Actor->Level->PrecacheMaterials.Empty();

			Viewport->LodSceneNode = NULL;

			Viewport->Unlock();
		}

		debugf(TEXT("Finished precaching textures in %5.3f seconds"), (FLOAT) (appSeconds() - StartTime));
		Viewport->Precaching	= 0;
		Viewport->PendingFrame	= 0;

		Viewport->Actor->GetLevel()->FinishedPrecaching = 1;
	}
	else if(Viewport->Lock(HitData,HitSize))
	{
		if( Viewport->Actor->UseFixedVisibility )
		{
			FMatrix& WorldToCamera = Viewport->Actor->RenderWorldToCamera;
			WorldToCamera = FTranslationMatrix(-CameraLocation);

			if(!Viewport->IsOrtho())
				WorldToCamera = WorldToCamera * FInverseRotationMatrix(CameraRotation);

			if(Viewport->Actor->RendMap == REN_OrthXY)
				WorldToCamera = WorldToCamera * FMatrix(
											FPlane(Viewport->ScaleX,	0,					0,					0),
											FPlane(0,					-Viewport->ScaleY,	0,					0),
											FPlane(0,					0,					-1,					0),
											FPlane(0,					0,					-CameraLocation.Z,	1));
			else if(Viewport->Actor->RendMap == REN_OrthXZ)
				WorldToCamera = WorldToCamera * FMatrix(
											FPlane(Viewport->ScaleX,	0,					0,					0),
											FPlane(0,					0,					-1,					0),
											FPlane(0,					Viewport->ScaleY,	0,					0),
											FPlane(0,					0,					-CameraLocation.Y,	1));
			else if(Viewport->Actor->RendMap == REN_OrthYZ)
				WorldToCamera = WorldToCamera * FMatrix(
											FPlane(0,					0,					1,					0),
											FPlane(Viewport->ScaleX,	0,					0,					0),
											FPlane(0,					Viewport->ScaleY,	0,					0),
											FPlane(0,					0,					CameraLocation.X,	1));
			else
				WorldToCamera = WorldToCamera * FMatrix(
											FPlane(0,					0,					1,	0),
											FPlane(Viewport->ScaleX,	0,					0,	0),
											FPlane(0,					Viewport->ScaleY,	0,	0),
											FPlane(0,					0,					0,	1));

			CameraLocation = Viewport->Actor->FixedLocation;
			CameraRotation = Viewport->Actor->FixedRotation;

			Viewport->RI->Clear();
		}

		FPlayerSceneNode	SceneNode(Viewport,&Viewport->RenderTarget,CameraActor,CameraLocation,CameraRotation,Viewport->Actor->FovAngle);
	
		Viewport->LodSceneNode = &SceneNode;

		Viewport->RI->Clear(0,FColor(0,0,0),1,1.0f,1,~DEPTH_COMPLEXITY_MASK(Viewport));

		// Update level audio.
		if(Audio)
		{
			clock(GStats.DWORDStats(GEngineStats.STATS_Game_AudioTickCycles));
			Audio->Update(&SceneNode);
			unclock(GStats.DWORDStats(GEngineStats.STATS_Game_AudioTickCycles));
		}

		SceneNode.Render(Viewport->RI);

		if ( Viewport->Canvas->pCanvasUtil )
            Viewport->Canvas->pCanvasUtil->Flush();

		Viewport->Precaching = 1;

		// Precache "dynamic" static meshes (e.g. weapon effects).
		for( INT i=0; i<Viewport->Actor->Level->PrecacheStaticMeshes.Num(); i++ )
		{
			// Set vertex and index buffers.
			UStaticMesh*	StaticMesh = Viewport->Actor->Level->PrecacheStaticMeshes(i);
			if( !StaticMesh )
				continue;

			FVertexStream*	VertexStreams[9] = { &StaticMesh->VertexStream };
			INT				NumVertexStreams = 1;

			if( !StaticMesh->VertexStream.Vertices.Num() )
				continue;

			if( StaticMesh->UseVertexColor )
			{
				if( StaticMesh->ColorStream.Colors.Num() )
					VertexStreams[NumVertexStreams++] = &StaticMesh->ColorStream;
			}
			else
			{
				if( StaticMesh->AlphaStream.Colors.Num() )
					VertexStreams[NumVertexStreams++] = &StaticMesh->AlphaStream;
			}

			for(INT UVIndex = 0;UVIndex < StaticMesh->UVStreams.Num();UVIndex++)
				VertexStreams[NumVertexStreams++] = &StaticMesh->UVStreams(UVIndex);

			if(StaticMesh->TBStream.TBs.Num())
				VertexStreams[NumVertexStreams++] = &StaticMesh->TBStream;

			Viewport->RI->SetVertexStreams(VS_FixedFunction,VertexStreams,NumVertexStreams);
			Viewport->RI->SetIndexBuffer(&StaticMesh->IndexBuffer,0);

			for( INT MatIndex=0; MatIndex<StaticMesh->Materials.Num(); MatIndex++ )
				Viewport->Actor->Level->PrecacheMaterials.AddItem( StaticMesh->Materials(MatIndex).Material );
		}
		Viewport->Actor->Level->PrecacheStaticMeshes.Empty();

		// Precache dynamic materials (e.g. player skins).
		for( INT i=0; i<Viewport->Actor->Level->PrecacheMaterials.Num(); i++ )
			Viewport->RI->SetMaterial( Viewport->Actor->Level->PrecacheMaterials(i) );
		Viewport->Actor->Level->PrecacheMaterials.Empty();		

		Viewport->Precaching = 0;

		Viewport->LodSceneNode = NULL;
		Viewport->Unlock();
		Viewport->PendingFrame = Blit ? 1 : 0;
	}

	Viewport->Actor->Level->LevelAction = SavedAction;

	Viewport->RenDev->RenderMovie(Viewport); // Added by Demiurge Studios (Movie)

	unclock(GStats.DWORDStats( GEngineStats.STATS_Frame_RenderCycles ));
	unguard;
}

static void ExportTravel( FOutputDevice& Out, AActor* Actor )
{
	guard(ExportTravel);
	debugf( TEXT("Exporting travelling actor of class %s"), Actor->GetClass()->GetPathName() );//!!xyzzy
	check(Actor);
	if( !Actor->bTravel )
		return;
	Out.Logf( TEXT("Class=%s Name=%s\r\n{\r\n"), Actor->GetClass()->GetPathName(), Actor->GetName() );
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Actor->GetClass()); It; ++It )
	{
		for( INT Index=0; Index<It->ArrayDim; Index++ )
		{
			TCHAR Value[1024];
			if
			(	(It->PropertyFlags & CPF_Travel)
			&&	It->ExportText( Index, Value, (BYTE*)Actor, &Actor->GetClass()->Defaults(0), 0 ) )
			{
				Out.Log( It->GetName() );
				if( It->ArrayDim!=1 )
					Out.Logf( TEXT("[%i]"), Index );
				Out.Log( TEXT("=") );
				UObjectProperty* Ref = Cast<UObjectProperty>( *It );
				if( Ref && Ref->PropertyClass->IsChildOf(AActor::StaticClass()) )
				{
					UObject* Obj = *(UObject**)( (BYTE*)Actor + It->Offset + Index*It->ElementSize );
					Out.Logf( TEXT("%s\r\n"), Obj ? Obj->GetName() : TEXT("None") );
				}
				Out.Logf( TEXT("%s\r\n"), Value );
			}
		}
	}
	Out.Logf( TEXT("}\r\n") );
	unguard;
}

//
// Jumping viewport.
//
void UGameEngine::SetClientTravel( UPlayer* Player, const TCHAR* NextURL, UBOOL bItems, ETravelType TravelType )
{
	guard(UGameEngine::SetClientTravel);
	check(Player);

	UViewport* Viewport    = CastChecked<UViewport>( Player );
	Viewport->TravelURL    = NextURL;
	Viewport->TravelType   = TravelType;
	Viewport->bTravelItems = bItems;

	unguard;
}

/*-----------------------------------------------------------------------------
	Tick.
-----------------------------------------------------------------------------*/

//
// Get tick rate limitor.
//
FLOAT UGameEngine::GetMaxTickRate()
{
	guard(UGameEngine::GetMaxTickRate);
	static UBOOL LanPlay = ParseParam(appCmdLine(),TEXT("lanplay"));

	if( GLevel && GLevel->DemoRecDriver && !GLevel->DemoRecDriver->ServerConnection && GLevel->NetDriver && !GIsClient )
	{
		// We're a dedicated server recording a demo, use the high framerate demo tick.
		return Clamp( LanPlay ? GLevel->DemoRecDriver->LanServerMaxTickRate : GLevel->DemoRecDriver->NetServerMaxTickRate, 20, 60 );
	}
	else
	if( GLevel && GLevel->NetDriver && !GIsClient )
	{
		// We're a dedicated server, use the LAN or Net tick rate.
		return Clamp( LanPlay ? GLevel->NetDriver->LanServerMaxTickRate : GLevel->NetDriver->NetServerMaxTickRate, 10, 120 );
	}
	else
	if( GLevel && GLevel->NetDriver && GLevel->NetDriver->ServerConnection && GLevel->GetLevelInfo()->bCapFramerate )
	{
		// We're a network client with bCapFramerate - so cap it.
		return ::Min(90.f,GLevel->NetDriver->ServerConnection->CurrentNetSpeed/GLevel->GetLevelInfo()->MoveRepSize);
	}
	else
		return 0;
	unguard;
}

//
// @@Cheat Protection - Check a given GUID/MD5 against the PackageValidation database and return if it's ok
//

UBOOL UGameEngine::ValidatePackage(const TCHAR* GUID, const TCHAR* MD5)
{
	guard(UGameEngine::ValidatePackage);

	UPackageCheckInfo *Info;
	
	TCHAR Decrypt[33];
	INT O,I=0;

	for (O=16;O<32;O++)
		Decrypt[O] = GUID[I++];

	for (O=8;O<16;O++)
		Decrypt[O] = GUID[I++];

	for (O=0;O<8;O++)
		Decrypt[O] = GUID[I++];

	Decrypt[32]=0x00;
	
	for (INT i=0;i<PackageValidation.Num();i++)
	{
		Info = PackageValidation(i);

		if ( !appStricmp(Decrypt,*Info->PackageID) )
		{
			for (INT j=0;j<Info->AllowedIDs.Num();j++)
			{
				if ( !appStricmp(MD5,*Info->AllowedIDs(j)) )
				{
					return 0;
				}
			}
			break;
			return 1;
		}
	}
	
	return 2;

	unguard;
}

//
// @@Cheat Protection - check all loaded packages against the package map in GPendingLevel.  If they are not allowed, try to clean
// them up
//

UBOOL UGameEngine::CheckForRogues()
{

	guard(UGameEngine::CheckForRogues);

	// Check to see if any "Non-Approved" packages are laying around

	TArray<UObject*> ObjLoaders = UObject::GetLoaderList(); 

	UPackageMap* ServerPackageMap = NULL;

	if( GLevel )
		ServerPackageMap = GLevel->NetDriver->ServerConnection->PackageMap;

	UBOOL bNeedsGC=false;	// By default, we don't need collection
	UBOOL bRemovePackage;
	for( INT i=0; i<ObjLoaders.Num(); i++ )
	{

		ULinker* Linker = CastChecked<ULinker>( ObjLoaders(i) );

		if ( Linker->LinksToCode() )
		{
			bRemovePackage = true;

			for( TArray<FPackageInfo>::TIterator It(ServerPackageMap->List); It; ++It )
			{
				if (Linker->Summary.Guid == It->Guid)
				{
					bRemovePackage = false;
					break;
				}
			}
		}
		else
			bRemovePackage=false;

		if (bRemovePackage)
		{
			debugf(TEXT("There is a need to remove %s"),Linker->LinkerRoot->GetName());
			bNeedsGC = true;
		}
	}

	return bNeedsGC;
	unguard;

}

// 
// @@Cheat Protection - Grab all of the client's MD5's and add them to the list.
//

void UGameEngine::AuthorizeClient(ULevel* Level)
{

	guard(UGameEngine::AuthorizeClient);

	if( !Level->NetDriver )
		return;

	// Check to see if any "Non-Approved" packages are laying around

	TArray<UObject*> ObjLoaders = UObject::GetLoaderList(); 

	FGuid Tester;
	INT Cnt = 0;

	for( INT i=0; i<ObjLoaders.Num(); i++ )
	{
		ULinker* Linker = CastChecked<ULinker>( ObjLoaders(i) );
		if ( Linker->LinksToCode() )
			Cnt++;
	}

	Level->NetDriver->ServerConnection->Logf( TEXT("CRITOBJCNT %i"),Cnt);

	for( INT i=0; i<ObjLoaders.Num(); i++ )
	{
		ULinker* Linker = CastChecked<ULinker>( ObjLoaders(i) );
		if ( Linker->LinksToCode() )
		{
			Tester = Linker->Summary.Guid;
			Tester.A = Linker->Summary.Guid.C;
			Tester.B = Linker->Summary.Guid.D;
			Tester.C = Linker->Summary.Guid.B;
			Tester.D = Linker->Summary.Guid.A;

			Level->NetDriver->ServerConnection->Logf( TEXT("GAMESTATE %s%s"),Tester.String(),*Linker->QuickMD5());
		}
	}
	unguard;
}

//
//@@Cheat Protection - Figure out the highest revision level of the packages in the database

int UGameEngine::PackageRevisionLevel()
{
	guard(UGameEngine::PackageRevisionLevel);

	INT Best=-1;
	for (INT i=0;i<PackageValidation.Num();i++)
	{
		if (PackageValidation(i)->RevisionLevel > Best)
			Best = PackageValidation(i)->RevisionLevel;
	}
		
	return Best;

	unguard;
}

//
// Update everything.
//
void UGameEngine::Tick( FLOAT DeltaSeconds )
{
	guard(UGameEngine::Tick);
	INT LocalTickCycles=0;
	clock(LocalTickCycles);

    if( DeltaSeconds < 0.0f )
        appErrorf(TEXT("Negative delta time!"));

	GRealDeltaTime = DeltaSeconds;
	GGameDeltaTime = GRealDeltaTime*GLevel->GetLevelInfo()->TimeDilation;

	// If all viewports closed, time to exit.
	if( Client && Client->Viewports.Num()==0 )
	{
		debugf( TEXT("All Windows Closed") );
		appRequestExit( 0 );
		return;
	}

	// If game is paused, release the cursor.
	static UBOOL WasPaused=1;
	if
	(	Client
	&&	Client->Viewports.Num()==1
	&&	GLevel
	&&	!Client->Viewports(0)->IsFullscreen() )
	{
		UBOOL IsPaused
		=	GLevel->IsPaused()
		||	Client->Viewports(0)->bShowWindowsMouse;
		if( IsPaused && !WasPaused )
			Client->Viewports(0)->SetMouseCapture( 0, 0, 0 );
		else if( WasPaused && !IsPaused && Client->CaptureMouse )
			Client->Viewports(0)->SetMouseCapture( 1, 1, 1 );
		WasPaused = IsPaused;
	}
	else WasPaused=0;

	// Update subsystems.
	UObject::StaticTick();				
	GCache.Tick();

	// Update the level.
	guard(TickLevel);
	GameCycles=0;
	clock(GameCycles);
	if( GLevel )
	{
		// Decide whether to drop high detail because of frame rate
		if ( Client )
		{
			GLevel->GetLevelInfo()->bDropDetail = (DeltaSeconds > 1.f/Clamp(Client->MinDesiredFrameRate,1.f,100.f)) && !GIsBenchmarking;
			GLevel->GetLevelInfo()->bAggressiveLOD = (DeltaSeconds > 1.f/Clamp(Client->MinDesiredFrameRate - 5.f,1.f,100.f)) && !GIsBenchmarking;
		}
		// tick the level
		GLevel->Tick( LEVELTICK_All, DeltaSeconds );

		// tick timers using current slomo setting unless playersonly is in effect or paused
		if( !GLevel->GetLevelInfo()->bPlayersOnly && !GLevel->IsPaused() )
			FTimerManager::GetInstance()->Tick( DeltaSeconds*GLevel->GetLevelInfo()->TimeDilation );
	}
	if( GEntry && GEntry!=GLevel )
		GEntry->Tick( LEVELTICK_All, DeltaSeconds );
	if( Client && Client->Viewports.Num() && Client->Viewports(0)->Actor->GetLevel()!=GLevel )
		Client->Viewports(0)->Actor->GetLevel()->Tick( LEVELTICK_All, DeltaSeconds );
	unclock(GameCycles);
	unguard;

	// Handle server travelling.
	guard(ServerTravel);
	if( GLevel && GLevel->GetLevelInfo()->NextURL!=TEXT("") )
	{
		if( (GLevel->GetLevelInfo()->NextSwitchCountdown-=DeltaSeconds) <= 0.f )
		{
			// Travel to new level, and exit.
			TMap<FString,FString> TravelInfo;
			if( GLevel->GetLevelInfo()->NextURL==TEXT("?RESTART") )
			{
				TravelInfo = GLevel->TravelInfo;
			}
			else if( GLevel->GetLevelInfo()->bNextItems )
			{
				TravelInfo = GLevel->TravelInfo;
				for( INT i=0; i<GLevel->Actors.Num(); i++ )
				{
					APlayerController* P = Cast<APlayerController>( GLevel->Actors(i) );
					if( P && P->Player && P->Pawn )
					{
						// Export items and self.
						FStringOutputDevice PlayerTravelInfo;
						ExportTravel( PlayerTravelInfo, P->Pawn );
						for( AActor* Inv=P->Pawn->Inventory; Inv; Inv=Inv->Inventory )
							ExportTravel( PlayerTravelInfo, Inv );
						TravelInfo.Set( *P->PlayerReplicationInfo->PlayerName, *PlayerTravelInfo );

						// Prevent local ClientTravel from taking place, since it will happen automatically.
						if( Cast<UViewport>( P->Player ) )
							Cast<UViewport>( P->Player )->TravelURL = TEXT("");
					}
				}
			}
			debugf( TEXT("Server switch level: %s"), *GLevel->GetLevelInfo()->NextURL );
			FString Error;
			Browse( FURL(&LastURL,*GLevel->GetLevelInfo()->NextURL,TRAVEL_Relative), &TravelInfo, Error );
			GLevel->GetLevelInfo()->NextURL = TEXT("");
			return;
		}
	}
	unguard;

	// Handle client travelling.
	guard(ClientTravel);
	if( Client && Client->Viewports.Num() && Client->Viewports(0)->TravelURL!=TEXT("") )
	{
		// Travel to new level, and exit.
		UViewport* Viewport = Client->Viewports( 0 );
		TMap<FString,FString> TravelInfo;

		// Export items.
		if( appStricmp(*Viewport->TravelURL,TEXT("?RESTART"))==0 )
		{
			TravelInfo = GLevel->TravelInfo;
		}
		else if( Viewport->bTravelItems )
		{
			debugf( TEXT("Export travel for: %s"), *Viewport->Actor->PlayerReplicationInfo->PlayerName );
			FStringOutputDevice PlayerTravelInfo;
			ExportTravel( PlayerTravelInfo, Viewport->Actor->Pawn );
			for( AActor* Inv=Viewport->Actor->Pawn->Inventory; Inv; Inv=Inv->Inventory )
				ExportTravel( PlayerTravelInfo, Inv );
			TravelInfo.Set( *Viewport->Actor->PlayerReplicationInfo->PlayerName, *PlayerTravelInfo );
		}
		FString Error;
		Browse( FURL(&LastURL,*Viewport->TravelURL,Viewport->TravelType), &TravelInfo, Error );
		Viewport->TravelURL=TEXT("");

		return;
	}
	unguard;

	// Update the pending level.
	guard(TickPending);
	if( GPendingLevel )
	{
		GPendingLevel->Tick( DeltaSeconds );
		if( GPendingLevel->Error!=TEXT("") )
		{
			// Pending connect failed.
			guard(PendingFailed);
			SetProgress( LocalizeError(TEXT("ConnectionFailed"),TEXT("Engine")), *GPendingLevel->Error, 4.f );
			debugf( NAME_Log, LocalizeError(TEXT("Pending"),TEXT("Engine")), *GPendingLevel->URL.String(), *GPendingLevel->Error );
			delete GPendingLevel;
			GPendingLevel = NULL;
			unguard;
		}
		else if( GPendingLevel->Success && !GPendingLevel->FilesNeeded && !GPendingLevel->SentJoin )
		{
			// Attempt to load the map.
			FString Error;
			guard(AttemptLoadPending);
			LoadMap( GPendingLevel->URL, GPendingLevel, NULL, Error );
			if( Error!=TEXT("") )
			{
				SetProgress( LocalizeError(TEXT("ConnectionFailed"),TEXT("Engine")), *Error, 4.f );
			}
			else if( !GPendingLevel->LonePlayer )
			{

				//@@Cheat Protection

				if (bCheatProtection)					
				AuthorizeClient(GLevel);

				// Show connecting message, cause precaching to occur.
				GLevel->GetLevelInfo()->LevelAction = LEVACT_Connecting;
				GEntry->GetLevelInfo()->LevelAction = LEVACT_Connecting;
				if( Client )
					Client->Tick();

				// Send join.
				GPendingLevel->SendJoin();
				GPendingLevel->NetDriver = NULL;
				GPendingLevel->DemoRecDriver = NULL;
			}
			unguard;

			// Kill the pending level.
			guard(KillPending);
			delete GPendingLevel;
			GPendingLevel = NULL;
			unguard;
		}
	}
	unguard;

	// Render everything.
	guard(ClientTick);
	INT LocalClientCycles=0;
	if( Client )
	{
		clock(LocalClientCycles);
		Client->Tick();
		unclock(LocalClientCycles);
	}
	ClientCycles=LocalClientCycles;
	unguard;

	unclock(LocalTickCycles);
	TickCycles=LocalTickCycles;
	GTicks++;
	unguard;
}

/*-----------------------------------------------------------------------------
	Saving the game.
-----------------------------------------------------------------------------*/

//
// Save the current game state to a file.
//
void UGameEngine::SaveGame( INT Position )
{
	guard(UGameEngine::SaveGame);

	TCHAR Filename[256];
	GFileManager->MakeDirectory( *GSys->SavePath, 0 );
	appSprintf( Filename, TEXT("%s") PATH_SEPARATOR TEXT("Save%i.usa"), *GSys->SavePath, Position );
	GLevel->GetLevelInfo()->LevelAction=LEVACT_Saving;
	PaintProgress();
	GWarn->BeginSlowTask( LocalizeProgress(TEXT("Saving"),TEXT("Engine")), 1);
	GLevel->CleanupDestroyed( 1 );
	if( SavePackage( GLevel->GetOuter(), GLevel, 0, Filename, GLog ) )
	{
		// Copy the hub stack.
		INT i;
		for( i=0; i<GLevel->GetLevelInfo()->HubStackLevel; i++ )
		{
			TCHAR Src[256], Dest[256];
			appSprintf( Src, TEXT("%s") PATH_SEPARATOR TEXT("Game%i.usa"), *GSys->SavePath, i );
			appSprintf( Dest, TEXT("%s") PATH_SEPARATOR TEXT("Save%i%i.usa"), *GSys->SavePath, Position, i );
			GFileManager->Copy( Src, Dest );
		}
		while( 1 )
		{
			appSprintf( Filename, TEXT("%s") PATH_SEPARATOR TEXT("Save%i%i.usa"), *GSys->SavePath, Position, i++ );
			if( GFileManager->FileSize(Filename)<=0 )
				break;
			GFileManager->Delete( Filename );
		}
	}
	for( INT i=0; i<GLevel->Actors.Num(); i++ )
		if( Cast<AMover>(GLevel->Actors(i)) )
			Cast<AMover>(GLevel->Actors(i))->SavedPos = FVector(-1,-1,-1);
	GWarn->EndSlowTask();
	GLevel->GetLevelInfo()->LevelAction=LEVACT_None;
	GCache.Flush();

	unguard;
}

/*-----------------------------------------------------------------------------
	Mouse feedback.
-----------------------------------------------------------------------------*/

void UGameEngine::MouseWheel( UViewport* Viewport, DWORD Buttons, INT Delta )
{
}

//
// Mouse delta while dragging.
//
void UGameEngine::MouseDelta( UViewport* Viewport, DWORD ClickFlags, FLOAT DX, FLOAT DY )
{
	guard(UGameEngine::MouseDelta);
	if
	(	(ClickFlags & MOUSE_FirstHit)
	&&	Client
	&&	Client->Viewports.Num()==1
	&&	GLevel
	&&	!Client->Viewports(0)->IsFullscreen()
	&&	!GLevel->IsPaused()
	&&  !Viewport->bShowWindowsMouse )
	{
		Viewport->SetMouseCapture( 1, 1, 1 );
	}
	else if( (ClickFlags & MOUSE_LastRelease) && !Client->CaptureMouse )
	{
		Viewport->SetMouseCapture( 0, 0, 0 );
	}
	unguard;
}

//
// Absolute mouse position.
//
void UGameEngine::MousePosition( UViewport* Viewport, DWORD ClickFlags, FLOAT X, FLOAT Y )
{
	guard(UGameEngine::MousePosition);

	if( Viewport )
	{
		Viewport->WindowsMouseX = X;
		Viewport->WindowsMouseY = Y;
	}

	unguard;
}

//
// Mouse clicking.
//
void UGameEngine::Click( UViewport* Viewport, DWORD ClickFlags, FLOAT X, FLOAT Y )
{
	guard(UGameEngine::Click);
	unguard;
}

void UGameEngine::UnClick( UViewport* Viewport, DWORD ClickFlags, INT MouseX, INT MouseY )
{
	guard(UGameEngine::UnClick);
	unguard;
}

//
// InitSpecial - Performs a full MD5 on a given filename
//
FString UGameEngine::InitSpecial(const TCHAR* Special)
{

	guard(UGameEngine::InitSpecial);

	FArchive* Sp = GFileManager->CreateFileReader( Special );
	int BytesToRead;
	if( !Sp )
	{
		return TEXT("");
	}

	BYTE* SpBuffer = (BYTE*)appMalloc(65535, TEXT(""));

	FMD5Context SpC;
	appMD5Init( &SpC );

	while ( Sp->Tell() < Sp->TotalSize() )
	{
		BytesToRead = Sp->TotalSize() - Sp->Tell();
		if (BytesToRead>65535)
			BytesToRead=65535;

		Sp->Serialize(SpBuffer, BytesToRead);
		appMD5Update( &SpC, SpBuffer, BytesToRead);
	}
	BYTE S[16];
	appMD5Final( S, &SpC );

	// Convert to a string

	FString InitResult;
	for (int i=0; i<16; i++)
		InitResult += FString::Printf(TEXT("%02x"), S[i]);	

	// Free the buffer

	appFree(SpBuffer);

	delete Sp;

	return InitResult;
	unguard;
}


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

