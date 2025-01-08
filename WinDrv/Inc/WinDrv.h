/*=============================================================================
	WinDrvPrivate.cpp: Unreal Windows viewport and platform driver.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#ifndef _INC_WINDRV
#define _INC_WINDRV

#define POINTER_64 

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef WINDRV_API
	#define WINDRV_API DLL_IMPORT
#endif

#define DIRECTINPUT_VERSION 0x0800 // NEW (mdf) get rid of dinput.h warning?

/*----------------------------------------------------------------------------
	Dependencies.
----------------------------------------------------------------------------*/

// Windows includes.
#pragma warning( disable : 4201 )
#define STRICT
#include <windows.h>
#include <shlobj.h>
#include <dinput.h>
#include "Res\WinDrvRes.h"

// Unreal includes.
#include "Engine.h"
#include "Window.h"
#include "UnRender.h"

/*-----------------------------------------------------------------------------
	Declarations.
-----------------------------------------------------------------------------*/

// Classes.
class UWindowsViewport;
class UWindowsClient;

// Global functions.
WINDRV_API void DirectInputError( const FString Error, HRESULT hr, UBOOL Fatal );

/*-----------------------------------------------------------------------------
	UWindowsClient.
-----------------------------------------------------------------------------*/

//
// Windows implementation of the client.
//
class DLL_EXPORT UWindowsClient : public UClient, public FNotifyHook
{
	DECLARE_CLASS(UWindowsClient,UClient,CLASS_Transient|CLASS_Config,WinDrv)

	// Configuration.
	BITFIELD			UseJoystick;
	BITFIELD			StartupFullscreen;
#if 1 //NEW (mdf) make mouse event remapping optional
	BITFIELD			RemapMouseEvents;
#endif
	FLOAT				MouseXMultiplier,
						MouseYMultiplier;

	// Variables.
	UBOOL				InMenuLoop;
	UBOOL				ConfigReturnFullscreen;
	INT					NormalMouseInfo[3];
	INT					CaptureMouseInfo[3];
	STICKYKEYS			SavedStickyKeys;

	WConfigProperties*	ConfigProperties;
	ATOM				hkAltEsc, hkAltTab, hkCtrlEsc, hkCtrlTab;

	// Constructors.
	UWindowsClient();
	void StaticConstructor();

	// FNotifyHook interface.
	void NotifyDestroy( void* Src );

	// UObject interface.
	void Destroy();
	void PostEditChange();
	void ShutdownAfterError();

	// UClient interface.
	void Init( UEngine* InEngine );
	void ShowViewportWindows( DWORD ShowFlags, INT DoShow );
	void EnableViewportWindows( DWORD ShowFlags, INT DoEnable );
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void Tick();
	void MakeCurrent( UViewport* InViewport );
	UViewport* GetLastCurrent();
	class UViewport* NewViewport( const FName Name );
};

/*-----------------------------------------------------------------------------
	UWindowsViewport.
-----------------------------------------------------------------------------*/

//
// Viewport window status.
//
enum EWinViewportStatus
{
	WIN_ViewportOpening	= 0, // Viewport is opening and hWnd is still unknown.
	WIN_ViewportNormal	= 1, // Viewport is operating normally, hWnd is known.
	WIN_ViewportClosing	= 2, // Viewport is closing and CloseViewport has been called.
};

//
// A Windows viewport.
//
class DLL_EXPORT UWindowsViewport : public UViewport
{
	DECLARE_CLASS(UWindowsViewport,UViewport,CLASS_Transient,WinDrv)
	DECLARE_WITHIN(UWindowsClient)

	// Variables.
	class WWindowsViewportWindow* Window;
	EWinViewportStatus  Status;
	HWND				ParentWindow;
	HHOOK				hHook;
	INT					HoldCount;
	DWORD				BlitFlags;
	UBOOL				Borderless,
						Captured,
						SupportsIME;
	EInputKey			LastJoyPOV;
	INT					CurrentIMESize;

	// DirectInput variables.
	static LPDIRECTINPUT8		DirectInput8;
	static LPDIRECTINPUTDEVICE8	Mouse;
	static LPDIRECTINPUTDEVICE8	Joystick;
	static DIDEVCAPS            JoystickCaps;

	// Info saved during captures and fullscreen sessions.
	POINT				SavedCursor;
	FRect				SavedWindowRect;
	INT					SavedCaps;
	HCURSOR				StandardCursors[10];

	// Constructor.
	UWindowsViewport();

	// UObject interface.
	void Destroy();
	void ShutdownAfterError();

	// UViewport interface.
	UBOOL Lock( BYTE* HitData=NULL, INT* HitSize=0 );
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar );
	UBOOL ResizeViewport( DWORD BlitFlags, INT NewX=INDEX_NONE, INT NewY=INDEX_NONE, UBOOL bSaveSize=true );
	UBOOL IsFullscreen();
	void Unlock();
	void Repaint( UBOOL Blit );
	void SetModeCursor();
	void UpdateWindowFrame();
	void OpenWindow( DWORD ParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY );
	void CloseWindow();
	void UpdateInput( UBOOL Reset, FLOAT DeltaSeconds );
	void* GetWindow();
	void SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL FocusOnly );
	void SetWindowOffsets( UWindowsClient* Client, INT ClientWidth, INT ClientHeight, INT& OffsetX, INT& OffsetY ); //NEW (mdf) windowed mode offset support
	TCHAR* GetLocalizedKeyName( EInputKey Key );

	// UWindowsViewport interface.
#if 1 //NEW (mdf) extended input
	void  MaybeRemapFunctionKey( EInputKey& Key );
	UBOOL RemappedMouseEvent( EInputKey Key, EInputAction Action, FLOAT Delta=0.0 );
	void CauseMaybeRemappedMouseEvent( EInputKey Key, EInputAction Action, FLOAT Delta=0.0 );
#endif
	LONG ViewportWndProc( UINT Message, UINT wParam, LONG lParam );
	void ToggleFullscreen();
	void EndFullscreen();
	void SetTopness();
	DWORD GetViewportButtonFlags( DWORD wParam );

	UBOOL CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta=0.0 );

	virtual void TryRenderDevice( const TCHAR* ClassName, INT NewX, INT NewY, UBOOL Fullscreen );

	// Static functions.
	static LRESULT CALLBACK KeyboardProc( INT Code, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK SysMsgProc( INT nCode, WPARAM wParam, LPARAM lParam );
	static BOOL    CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );
	static BOOL    CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );

};

//
// A windows viewport window.
//
class DLL_EXPORT WWindowsViewportWindow : public WWindow
{
	W_DECLARE_CLASS(WWindowsViewportWindow,WWindow,CLASS_Transient)
	DECLARE_WINDOWCLASS(WWindowsViewportWindow,WWindow,Window)
	class UWindowsViewport* Viewport;
	WWindowsViewportWindow()
	{}
	WWindowsViewportWindow( class UWindowsViewport* InViewport )
	: Viewport( InViewport )
	{}
	LONG WndProc( UINT Message, UINT wParam, LONG lParam )
	{
		return Viewport->ViewportWndProc( Message, wParam, lParam );
	}
};

#define AUTO_INITIALIZE_REGISTRANTS_WINDRV \
	UWindowsViewport::StaticClass(); \
	UWindowsClient::StaticClass();

#endif //_INC_WINDRV
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
