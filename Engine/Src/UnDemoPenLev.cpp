/*=============================================================================
	DemoPlayPenLev.cpp: Unreal demo playback pending level class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Jack Porter
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

/*-----------------------------------------------------------------------------
	UDemoPlayPendingLevel implementation.
-----------------------------------------------------------------------------*/

//
// Constructor.
//
UDemoPlayPendingLevel::UDemoPlayPendingLevel( UEngine* InEngine, const FURL& InURL )
:	UPendingLevel( InEngine, InURL )
{
	guard(UDemoPlayPendingLevel::UDemoPlayPendingLevel);

	// Try to create demo playback driver.
	UClass* DemoDriverClass = StaticLoadClass( UDemoRecDriver::StaticClass(), NULL, TEXT("ini:Engine.Engine.DemoRecordingDevice"), NULL, LOAD_NoFail, NULL );
	DemoRecDriver = ConstructObject<UDemoRecDriver>( DemoDriverClass );
	if( !DemoRecDriver->InitConnect( this, URL, Error ) )
	{
		delete DemoRecDriver;
		DemoRecDriver = NULL;
	}

	unguard;
}
//
// FNetworkNotify interface.
//
ULevel* UDemoPlayPendingLevel::NotifyGetLevel()
{
	guard(UDemoPlayPendingLevel::NotifyGetLevel);
	return NULL;
	unguard;
}
void UDemoPlayPendingLevel::NotifyReceivedText( UNetConnection* Connection, const TCHAR* Text )
{
	guard(UDemoPlayPendingLevel::NotifyReceivedText);
	debugf( TEXT("DemoPlayPendingLevel received: %s"), Text );
	if( ParseCommand( &Text, TEXT("USES") ) )
	{
		// Dependency information.
		FPackageInfo& Info = *new(Connection->PackageMap->List)FPackageInfo(NULL);
		TCHAR PackageName[NAME_SIZE]=TEXT("");
		Parse( Text, TEXT("GUID="), Info.Guid );
		Parse( Text, TEXT("GEN=" ), Info.RemoteGeneration );
		Parse( Text, TEXT("SIZE="), Info.FileSize );
		Info.DownloadSize = Info.FileSize;
		Parse( Text, TEXT("FLAGS="), Info.PackageFlags );
		Parse( Text, TEXT("PKG="), PackageName, ARRAY_COUNT(PackageName) );
		Info.Parent = CreatePackage(NULL,PackageName);
	}
	else if( ParseCommand( &Text, TEXT("WELCOME") ) )
	{
		FURL URL;
	
		// Parse welcome message.
		Parse( Text, TEXT("LEVEL="), URL.Map );

		UBOOL HadMissing = 0;
        
		// Make sure all packages we need available
		for( INT i=0; i<Connection->PackageMap->List.Num(); i++ )
		{
			TCHAR Filename[256];
			FPackageInfo& Info = Connection->PackageMap->List(i);
			if( !appFindPackageFile( Info.Parent->GetName(), &Info.Guid, Filename ) )
			{
				if( Engine->Client->Viewports.Num() )
					Engine->Client->Viewports(0)->Actor->eventClientMessage( *FString::Printf(TEXT("Warning: missing package '%s' for demo playback"), Info.Parent->GetName()), NAME_None ); //!!localize

				if( !HadMissing )
				{
					Error = FString::Printf( TEXT("Missing package '%s' for demo playback"), Info.Parent->GetName() ); //!!localize
					Connection->State = USOCK_Closed;
					HadMissing = 1;
				}
			}
		}

		if( HadMissing )
			return;

		DemoRecDriver->Time = 0;
		Success = 1;
	}
	unguard;
}
//
// UPendingLevel interface.
//
void UDemoPlayPendingLevel::Tick( FLOAT DeltaTime )
{
	guard(UDemoPlayPendingLevel::Tick);
	check(DemoRecDriver);
	check(DemoRecDriver->ServerConnection);

	// Update demo recording driver.
	DemoRecDriver->UpdateDemoTime( &DeltaTime );
	DemoRecDriver->TickDispatch( DeltaTime );
	DemoRecDriver->TickFlush();

	unguard;
}

UNetDriver* UDemoPlayPendingLevel::GetDriver()
{
	return DemoRecDriver;
}

IMPLEMENT_CLASS(UDemoPlayPendingLevel);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

