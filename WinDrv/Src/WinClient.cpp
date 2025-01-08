/*=============================================================================
	WinClient.cpp: UWindowsClient code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "WinDrv.h"

/*-----------------------------------------------------------------------------
	Class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UWindowsClient);

/*-----------------------------------------------------------------------------
	UWindowsClient implementation.
-----------------------------------------------------------------------------*/

//
// UWindowsClient constructor.
//
UWindowsClient::UWindowsClient()
{
	guard(UWindowsClient::UWindowsClient);

	// Init hotkey atoms.
	hkAltEsc	= GlobalAddAtom( TEXT("UnrealAltEsc")  );
	hkAltTab	= GlobalAddAtom( TEXT("UnrealAltTab")  );
	hkCtrlEsc	= GlobalAddAtom( TEXT("UnrealCtrlEsc") );
	hkCtrlTab	= GlobalAddAtom( TEXT("UnrealCtrlTab") );

	unguard;
}

//
// Static init.
//
void UWindowsClient::StaticConstructor()
{
	guard(UWindowsClient::StaticConstructor);

	new(GetClass(),TEXT("UseJoystick"),       RF_Public)UBoolProperty(CPP_PROPERTY(UseJoystick),       TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("StartupFullscreen"), RF_Public)UBoolProperty(CPP_PROPERTY(StartupFullscreen), TEXT("Display"),  CPF_Config );
#if 1 //NEW (mdf) make mouse event remapping optional
	new(GetClass(),TEXT("RemapMouseEvents"),  RF_Public)UBoolProperty(CPP_PROPERTY(RemapMouseEvents),  TEXT("Display"),  CPF_Config );
#endif	
	new(GetClass(),TEXT("MouseXMultiplier"),  RF_Public)UFloatProperty(CPP_PROPERTY(MouseXMultiplier),  TEXT("Display"),  CPF_Config );
	new(GetClass(),TEXT("MouseYMultiplier"),  RF_Public)UFloatProperty(CPP_PROPERTY(MouseYMultiplier),  TEXT("Display"),  CPF_Config );

	unguard;
}

//
// Initialize the platform-specific viewport manager subsystem.
// Must be called after the Unreal object manager has been initialized.
// Must be called before any viewports are created.
//
void UWindowsClient::Init( UEngine* InEngine )
{
	guard(UWindowsClient::UWindowsClient);

	// Init base.
	UClient::Init( InEngine );

	// Register window class.
	IMPLEMENT_WINDOWCLASS(WWindowsViewportWindow,GIsEditor ? (CS_DBLCLKS|CS_OWNDC) : (CS_OWNDC));

	// Initialize DirectInput.
	HRESULT hr;
	if( FAILED( hr = DirectInput8Create( hInstanceWindow, DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&UWindowsViewport::DirectInput8, NULL ) ) )
		DirectInputError( TEXT("Couldn't create input device"), hr, true );

	// Note configuration.
	PostEditChange();

	// Default res option.
	if( ParseParam(appCmdLine(),TEXT("defaultres")) )
	{
		WindowedViewportX  = FullscreenViewportX  = 640;
		WindowedViewportY  = FullscreenViewportY  = 480;
	}

	// Save sticky key information before disabling it.
	STICKYKEYS StickyKeys;
	StickyKeys.cbSize		= sizeof(STICKYKEYS);
	StickyKeys.dwFlags		= 0;
	SavedStickyKeys.cbSize	= sizeof(STICKYKEYS);
	
	SystemParametersInfoX( SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &SavedStickyKeys, 0 );
	SystemParametersInfoX( SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &StickyKeys, 0 ); 

	// Get mouse info.
//	SystemParametersInfoX( SPI_GETMOUSE, 0, NormalMouseInfo, 0 );
//	debugf( NAME_Init, TEXT("Mouse info: %i %i %i"), NormalMouseInfo[0], NormalMouseInfo[1], NormalMouseInfo[2] );
//	CaptureMouseInfo[0] = 0;     // First threshold.
//	CaptureMouseInfo[1] = 0;     // Second threshold.
//	CaptureMouseInfo[2] = 65536; // Speed.

	// Success.
	debugf( NAME_Init, TEXT("Client initialized") );
	unguard;
}


//
// Shut down the platform-specific viewport manager subsystem.
//
void UWindowsClient::Destroy()
{
	guard(UWindowsClient::Destroy);

	// Make sure to shut down Viewports first.
	for( INT i=0; i<Viewports.Num(); i++ )
		Viewports(i)->ConditionalDestroy();

	// Shut down GRenDev.
	Engine->GRenDev->Exit(NULL);

	// Stop capture.
	SetCapture( NULL );
	ClipCursor( NULL );

	// Restore sticky keys setting.
	SystemParametersInfoX( SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &SavedStickyKeys, 0 ); 
//	SystemParametersInfoX( SPI_SETMOUSE, 0, NormalMouseInfo, 0 );
	
	UWindowsViewport::DirectInput8->Release();
	UWindowsViewport::DirectInput8 = NULL;
	
	GlobalDeleteAtom( hkAltEsc );
	GlobalDeleteAtom( hkAltTab );
	GlobalDeleteAtom( hkCtrlEsc );
	GlobalDeleteAtom( hkCtrlTab );

	debugf( NAME_Exit, TEXT("Windows client shut down") );
	Super::Destroy();
	unguard;
}

//
// Failsafe routine to shut down viewport manager subsystem
// after an error has occured. Not guarded.
//
void UWindowsClient::ShutdownAfterError()
{
	debugf( NAME_Exit, TEXT("Executing UWindowsClient::ShutdownAfterError") );
	SetCapture( NULL );
	ClipCursor( NULL );

	// Restore sticky keys setting.
	SystemParametersInfoX( SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &SavedStickyKeys, 0 ); 
//	SystemParametersInfoX( SPI_SETMOUSE, 0, NormalMouseInfo, 0 );
	
	while( ShowCursor(TRUE)<0 );
	if( Engine && Engine->Audio )
	{
		Engine->Audio->ConditionalShutdownAfterError();
	}
	for( INT i=Viewports.Num()-1; i>=0; i-- )
	{
		UWindowsViewport* Viewport = (UWindowsViewport*)Viewports( i );
		Viewport->ConditionalShutdownAfterError();
	}
	Super::ShutdownAfterError();
}

void UWindowsClient::NotifyDestroy( void* Src )
{
	guard(UWindowsClient::NotifyDestroy);
	if( Src==ConfigProperties )
	{
		ConfigProperties = NULL;
		if( ConfigReturnFullscreen && Viewports.Num() )
			Viewports(0)->Exec( TEXT("ToggleFullscreen") );
	}
	unguard;
}

//
// Command line.
//
UBOOL UWindowsClient::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UWindowsClient::Exec);
	if( UClient::Exec( Cmd, Ar ) )
	{
		return 1;
	}
	return 0;
	unguard;
}

//
// Perform timer-tick processing on all visible viewports.  This causes
// all realtime viewports, and all non-realtime viewports which have been
// updated, to be blitted.
//
void UWindowsClient::Tick()
{
	guard(UWindowsClient::Tick);

	// Blit any viewports that need blitting.
  	for( INT i=0; i<Viewports.Num(); i++ )
	{
		UWindowsViewport* Viewport = CastChecked<UWindowsViewport>(Viewports(i));
		check(!Viewport->HoldCount);
		if( !IsWindow(Viewport->Window->hWnd) )
		{
			// Window was closed via close button.
			delete Viewport;
			return;
		}
  		else if( (Viewport->IsRealtime() || (Viewport->DirtyViewport != 0)) && Viewport->SizeX && Viewport->SizeY )
			Viewport->Repaint( (Viewport->DirtyViewport == -1) ? 0 : 1 );
	}
	
	unguard;
}

//
// Create a new viewport.
//
UViewport* UWindowsClient::NewViewport( const FName Name )
{
	guard(UWindowsClient::NewViewport);
	return new( this, Name )UWindowsViewport();
	unguard;
}

//
// Configuration change.
//
void UWindowsClient::PostEditChange()
{
	guard(UWindowsClient::PostEditChange);
	Super::PostEditChange();
	// TODO: detect Joystick configuration.
	unguard;
}

//
// Enable or disable all viewport windows that have ShowFlags set (or all if ShowFlags=0).
//
void UWindowsClient::EnableViewportWindows( DWORD ShowFlags, int DoEnable )
{
	guard(UWindowsClient::EnableViewportWindows);
  	for( int i=0; i<Viewports.Num(); i++ )
	{
		UWindowsViewport* Viewport = (UWindowsViewport*)Viewports(i);
		if( (Viewport->Actor->ShowFlags & ShowFlags)==ShowFlags )
			EnableWindow( Viewport->Window->hWnd, DoEnable );
	}
	unguard;
}

//
// Show or hide all viewport windows that have ShowFlags set (or all if ShowFlags=0).
//
void UWindowsClient::ShowViewportWindows( DWORD ShowFlags, int DoShow )
{
	guard(UWindowsClient::ShowViewportWindows); 	
	for( int i=0; i<Viewports.Num(); i++ )
	{
		UWindowsViewport* Viewport = (UWindowsViewport*)Viewports(i);
		if( (Viewport->Actor->ShowFlags & ShowFlags)==ShowFlags )
			Viewport->Window->Show(DoShow);
	}
	unguard;
}

//
// Make this viewport the current one.
// If Viewport=0, makes no viewport the current one.
//
void UWindowsClient::MakeCurrent( UViewport* InViewport )
{
	guard(UWindowsViewport::MakeCurrent);
	for( INT i=0; i<Viewports.Num(); i++ )
	{
		UViewport* OldViewport = Viewports(i);
		if( OldViewport->Current && OldViewport!=InViewport )
		{
			OldViewport->Current = 0;
			OldViewport->UpdateWindowFrame();
		}
	}
	if( InViewport )
	{
		LastCurrent = InViewport;
		InViewport->Current = 1;
		InViewport->UpdateWindowFrame();
	}
	unguard;
}

// Returns a pointer to the viewport that was last current.
UViewport* UWindowsClient::GetLastCurrent()
{
	guard(UWindowsViewport::GetLastCurrent);
	return LastCurrent;
	unguard;
}

/*-----------------------------------------------------------------------------
	Getting error messages.
-----------------------------------------------------------------------------*/