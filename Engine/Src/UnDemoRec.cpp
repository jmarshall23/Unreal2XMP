/*=============================================================================
	DemoRecDrv.cpp: Unreal demo recording network driver.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Jack Porter.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

#define PACKETSIZE 512

/*-----------------------------------------------------------------------------
	UDemoRecConnection.
-----------------------------------------------------------------------------*/

void UDemoRecConnection::StaticConstructor()
{
	guard(UDemoRecConnection::StaticConstructor);
	unguard;
}
UDemoRecConnection::UDemoRecConnection( UNetDriver* InDriver, const FURL& InURL )
: UNetConnection( InDriver, InURL )
{
	guard(UDemoRecConnection::UDemoRecConnection);
	MaxPacket   = PACKETSIZE;
	InternalAck = 1;
	unguard;
}
UDemoRecDriver* UDemoRecConnection::GetDriver()
{
	return (UDemoRecDriver *)Driver;
}
FString UDemoRecConnection::LowLevelGetRemoteAddress()
{
	guard(UDemoRecConnection::LowLevelGetRemoteAddress);
	return TEXT("");
	unguard;
}
void UDemoRecConnection::LowLevelSend( void* Data, INT Count )
{
	guard(UDemoRecConnection::LowLevelSend);
	if( !GetDriver()->ServerConnection )
	{
		*GetDriver()->FileAr << GetDriver()->LastDeltaTime << GetDriver()->FrameNum << Count;
		GetDriver()->FileAr->Serialize( Data, Count );
		//!!if GetDriver()->GetFileAr()->IsError(), print error, cancel demo recording
	}
	unguard;
}
FString UDemoRecConnection::LowLevelDescribe()
{
	guard(UDemoRecConnection::Describe);
	return TEXT("Demo recording driver connection");
	unguard;
}
INT UDemoRecConnection::IsNetReady( UBOOL Saturate )
{
	return 1;
}
void UDemoRecConnection::FlushNet()
{
	// in playback, there is no data to send except
	// channel closing if an error occurs.
	if( !GetDriver()->ServerConnection )
		Super::FlushNet();
}
void UDemoRecConnection::HandleClientPlayer( APlayerController* PC )
{
	guard(UDemoRecConnection::HandleClientPlayer);
	Super::HandleClientPlayer(PC);
	PC->bDemoOwner = 1;
	unguard;
}
IMPLEMENT_CLASS(UDemoRecConnection);

/*-----------------------------------------------------------------------------
	UDemoRecDriver.
-----------------------------------------------------------------------------*/

UDemoRecDriver::UDemoRecDriver()
{}
UBOOL UDemoRecDriver::InitBase( UBOOL Connect, FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error )
{
	guard(UDemoRecDriver::Init);

	DemoFilename   = ConnectURL.Map;
	Time           = 0;
	FrameNum       = 0;
	DemoEnded      = 0;

	return 1;
	unguard;
}
UBOOL UDemoRecDriver::InitConnect( FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error )
{
	guard(UDemoRecDriver::InitConnect);
	if( !Super::InitConnect( InNotify, ConnectURL, Error ) )
		return 0;
	if( !InitBase( 1, InNotify, ConnectURL, Error ) )
		return 0;

	// Playback, local machine is a client, and the demo stream acts "as if" it's the server.
	ServerConnection = new UDemoRecConnection( this, ConnectURL );
	ServerConnection->CurrentNetSpeed = 1000000;
	ServerConnection->State        = USOCK_Pending;
	FileAr                         = GFileManager->CreateFileReader( *DemoFilename );
	if( !FileAr )
	{
		Error = FString::Printf( TEXT("Couldn't open demo file %s for reading"), *DemoFilename );//!!localize!!
		return 0;
	}
	LoopURL = ConnectURL;
	NoFrameCap          = ConnectURL.HasOption(TEXT("timedemo"));
	Loop				= ConnectURL.HasOption(TEXT("loop"));

	LastFrameTime = appSeconds();
	return 1;
	unguard;
}
UBOOL UDemoRecDriver::InitListen( FNetworkNotify* InNotify, FURL& ConnectURL, FString& Error )
{
	guard(UDemoRecDriver::InitListen);
	if( !Super::InitListen( InNotify, ConnectURL, Error ) )
		return 0;
	if( !InitBase( 0, InNotify, ConnectURL, Error ) )
		return 0;

	// Recording, local machine is server, demo stream acts "as if" it's a client.
	UDemoRecConnection* Connection = new UDemoRecConnection( this, ConnectURL );
	Connection->CurrentNetSpeed   = 1000000;
	Connection->State             = USOCK_Open;
	Connection->InitOut();

	FileAr = GFileManager->CreateFileWriter( *DemoFilename );
	ClientConnections.AddItem( Connection );
	if( !FileAr )
	{
		Error = FString::Printf( TEXT("Couldn't open demo file %s for writing"), *DemoFilename );//localize!!
		return 0;
	}

	// Build package map.
	UGameEngine* GameEngine = CastChecked<UGameEngine>( GetLevel()->Engine );
	UBOOL ClientDemo = GetLevel()->GetLevelInfo()->NetMode == NM_Client;

	if( ClientDemo)
		MasterMap->CopyLinkers( GetLevel()->NetDriver->ServerConnection->PackageMap );
	else
		GameEngine->BuildServerMasterMap( this, GetLevel() );

	// Create the control channel.
	Connection->CreateChannel( CHTYPE_Control, 1, 0 );

	// Welcome the player to the level.
	FString WelcomeExtra;

	if( GetLevel()->GetLevelInfo()->NetMode == NM_Client )
		WelcomeExtra = TEXT("NETCLIENTDEMO");
	else
	if( GetLevel()->GetLevelInfo()->NetMode == NM_Standalone )
		WelcomeExtra = TEXT("CLIENTDEMO");
	else
		WelcomeExtra = TEXT("SERVERDEMO");

	WelcomeExtra = WelcomeExtra + FString::Printf(TEXT(" TICKRATE=%d"), GetLevel()->GetLevelInfo()->NetMode == NM_DedicatedServer ? appRound(GameEngine->GetMaxTickRate()) : appRound(NetServerMaxTickRate) );

	GetLevel()->WelcomePlayer( Connection, (TCHAR*)(*WelcomeExtra)  );

	// Spawn the demo recording spectator.
	if( !ClientDemo )
		SpawnDemoRecSpectator(Connection);
	else
		GetLevel()->NetDriver->ServerConnection->Actor->eventStartClientDemoRec();

	return 1;
	unguard;
}
void UDemoRecDriver::StaticConstructor()
{
	guard(UDemoRecDriver::StaticConstructor);
	new(GetClass(),TEXT("DemoSpectatorClass"), RF_Public)UStrProperty(CPP_PROPERTY(DemoSpectatorClass), TEXT("Client"), CPF_Config);
	unguard;
}
void UDemoRecDriver::LowLevelDestroy()
{
	guard(UDemoRecDriver::LowLevelDestroy);

	debugf( TEXT("Closing down demo driver.") );

	// Shut down file.
	guard(CloseFile);
	if( FileAr )
	{	
		delete FileAr;
		FileAr = NULL;
	}
	unguard;

	unguard;
}
UBOOL UDemoRecDriver::UpdateDemoTime( FLOAT* DeltaTime )
{
	guard(UDemoRecDriver::UpdateDemoTime);

	UBOOL Result = 0;

	if( ServerConnection )
	{
		// Ensure LastFrameTime is inside a valid range, so we don't lock up if things get very out of sync.
		LastFrameTime = Clamp<DOUBLE>( LastFrameTime, appSeconds() - 1.0, appSeconds() );

		// Playback
		FrameNum++;
		if( Notify->NotifyGetLevel() && Notify->NotifyGetLevel()->FinishedPrecaching && InitialFrameStart == 0 )
		{
			PlaybackStartTime = appSeconds();
			InitialFrameStart = FrameNum;
		}

		if( ServerConnection->State==USOCK_Open ) 
		{
			if( !FileAr->AtEnd() && !FileAr->IsError() )
			{
				// peek at next delta time.
				FLOAT NewDeltaTime;
				INT NewFrameNum;

				*FileAr << NewDeltaTime << NewFrameNum;
				FileAr->Seek(FileAr->Tell() - sizeof(NewDeltaTime) - sizeof(NewFrameNum));

				// If the real delta time is too small, sleep for the appropriate amount.
				if( !NoFrameCap )
					while(appSeconds() < LastFrameTime+(DOUBLE)NewDeltaTime);

				// Lie to the game about the amount of time which has passed.
				*DeltaTime = NewDeltaTime;
			}
		}
		LastDeltaTime = *DeltaTime;
		LastFrameTime = appSeconds();
	}
	else
	{
		// Recording
		BYTE NetMode = Notify->NotifyGetLevel()->GetLevelInfo()->NetMode;

		// Accumulate the current DeltaTime for the real frames this demo frame will represent.
		DemoRecMultiFrameDeltaTime += *DeltaTime;

		// Cap client demo recording rate (but not framerate).
		if( NetMode==NM_DedicatedServer || ( (appSeconds()-LastClientRecordTime) >= (DOUBLE)(1.f/NetServerMaxTickRate) ) )
		{
			// record another frame.
			FrameNum++;
			LastClientRecordTime = appSeconds();
			LastDeltaTime = DemoRecMultiFrameDeltaTime;
			DemoRecMultiFrameDeltaTime = 0.f;
			Result = 1;

			// Save the new delta-time and frame number, with no data, in case there is nothing to replicate.
			INT Count = 0;
			*FileAr << LastDeltaTime << FrameNum << Count;
		}
	}

	return Result;

	unguard;
}
void UDemoRecDriver::TickDispatch( FLOAT DeltaTime )
{
	guard(UDemoRecDriver::TickDispatch);
	Super::TickDispatch( DeltaTime );

	if( ServerConnection && (ServerConnection->State==USOCK_Pending || ServerConnection->State==USOCK_Open) )
	{	
		BYTE Data[PACKETSIZE + 8];
		// Read data from the demo file
		DWORD PacketBytes;
		INT PlayedThisTick = 0;
		for( ; ; )
		{
			// At end of file?
			if( FileAr->AtEnd() || FileAr->IsError() )
			{
			AtEnd:
				ServerConnection->State = USOCK_Closed;
				DemoEnded = 1;

				DOUBLE Seconds = appSeconds()-PlaybackStartTime;
				if( NoFrameCap )
					ServerConnection->Actor->eventClientMessage( *FString::Printf(TEXT("Demo %s ended: %d frames in %lf seconds (%.3f fps)"), *DemoFilename, FrameNum-InitialFrameStart, Seconds, (FLOAT)(FrameNum-InitialFrameStart)/Seconds ), NAME_None );//!!localize!!
				else
					ServerConnection->Actor->eventClientMessage( *FString::Printf(TEXT("Demo %s ended: %d frames in %lf seconds"), *DemoFilename, FrameNum-InitialFrameStart, Seconds ), NAME_None );//!!localize!!

				if( Loop )
					GetLevel()->Exec( *(FString(TEXT("DEMOPLAY "))+(*LoopURL.String())), *GLog );
				return;
			}
	
			INT ServerFrameNum;
			FLOAT ServerDeltaTime;

			*FileAr << ServerDeltaTime;
			*FileAr << ServerFrameNum;
			if( ServerFrameNum > FrameNum )
			{
				FileAr->Seek(FileAr->Tell() - sizeof(ServerFrameNum) - sizeof(ServerDeltaTime));
				break;
			}
			*FileAr << PacketBytes;

			if( PacketBytes )
			{
				// Read data from file.
				FileAr->Serialize( Data, PacketBytes );
				if( FileAr->IsError() )
				{
					debugf( NAME_DevNet, TEXT("Failed to read demo file packet") );
					goto AtEnd;
				}

				// Update stats.
				PlayedThisTick++;

				// Process incoming packet.
				ServerConnection->ReceivedRawPacket( Data, PacketBytes );
			}

			// Only play one packet per tick on demo playback, until we're 
			// fully connected.  This is like the handshake for net play.
			if(ServerConnection->State == USOCK_Pending)
			{
				FrameNum = ServerFrameNum;
				break;
			}
		}
	}
	unguard;
}
FString UDemoRecDriver::LowLevelGetNetworkNumber()
{
	guard(UDemoRecDriver::LowLevelGetNetworkNumber);
	return TEXT("");
	unguard;
}
INT UDemoRecDriver::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UDemoRecDriver::Exec);
	if( DemoEnded )
		return 0;
	if( ParseCommand(&Cmd,TEXT("DEMOREC")) || ParseCommand(&Cmd,TEXT("DEMOPLAY")) )
	{
		if( ServerConnection )
			Ar.Logf( TEXT("Demo playback currently active: %s"), *DemoFilename );//!!localize!!
		else
			Ar.Logf( TEXT("Demo recording currently active: %s"), *DemoFilename );//!!localize!!
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("STOPDEMO")) )
	{
		Loop = 0;
		Ar.Logf( TEXT("Demo %s stopped at frame %d"), *DemoFilename, FrameNum );//!!localize!!
		if( !ServerConnection )
		{
			GetLevel()->DemoRecDriver=NULL;
			delete this;
		}
		else
			ServerConnection->State = USOCK_Closed;
		return 1;
	}
	else return 0;
	unguard;
}
ULevel* UDemoRecDriver::GetLevel()
{
	guard(UDemoRecDriver::GetLevel);
	check(Notify->NotifyGetLevel());
	return Notify->NotifyGetLevel();
	unguard;

}
void UDemoRecDriver::SpawnDemoRecSpectator( UNetConnection* Connection )
{
	guard(UDemoRecDriver::SpawnDemoRecSpectator);
	UClass* C = StaticLoadClass( AActor::StaticClass(), NULL, *DemoSpectatorClass, NULL, LOAD_NoFail, NULL );
	APlayerController* Controller = CastChecked<APlayerController>(GetLevel()->SpawnActor( C ));

    guard(FindPlayerStart);
	for( INT i=0; i<GetLevel()->Actors.Num(); i++ )
	{
		if( GetLevel()->Actors(i) && GetLevel()->Actors(i)->IsA(APlayerStart::StaticClass()) )
		{
			Controller->Location = GetLevel()->Actors(i)->Location;
			Controller->Rotation = GetLevel()->Actors(i)->Rotation;
			break;
		}
	}
	unguard;

	unguard;
}
IMPLEMENT_CLASS(UDemoRecDriver);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

