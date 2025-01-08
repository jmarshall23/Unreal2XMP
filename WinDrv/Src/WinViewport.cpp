/*=============================================================================
	WinViewport.cpp: UWindowsViewport code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "WinDrv.h"

#define WM_MOUSEWHEEL 0x020A

// Duplicated here for convenience.
enum EEditorMode
{
	EM_None 				= 0,	// Gameplay, editor disabled.
	EM_ViewportMove			= 1,	// Move viewport normally.
	EM_ViewportZoom			= 2,	// Move viewport with acceleration.
	EM_ActorRotate			= 5,	// Rotate actor.
	EM_ActorScale			= 8,	// Scale actor.
	EM_TexturePan			= 11,	// Pan textures.
	EM_TextureRotate		= 13,	// Rotate textures.
	EM_TextureScale			= 14,	// Scale textures.
	EM_ActorSnapScale		= 18,	// Actor snap-scale.
	EM_TexView				= 19,	// Viewing textures.
	EM_TexBrowser			= 20,	// Browsing textures.
	EM_StaticMeshBrowser	= 21,	// Browsing static meshes.
	EM_MeshView				= 22,	// Viewing mesh.
	EM_MeshBrowser			= 23,	// Browsing mesh.
	EM_BrushClip			= 24,	// Brush Clipping.
	EM_VertexEdit			= 25,	// Multiple Vertex Editing.
	EM_FaceDrag				= 26,	// Face Dragging.
	EM_Polygon				= 27,	// Free hand polygon drawing
	EM_TerrainEdit			= 28,	// Terrain editing.
	EM_PrefabBrowser		= 29,	// Browsing prefabs.
	EM_Matinee				= 30,	// Movie editing.
	EM_EyeDropper			= 31,	// Eyedropper
	EM_Animation			= 32,	// Viewing Animation
	EM_FindActor			= 33,	// Find Actor
	EM_MaterialEditor		= 34,	// Material editor
	EM_Geometry				= 35,	// Geometry editing mode
	EM_NewCameraMove		= 50,
};


/*-----------------------------------------------------------------------------
	Class implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UWindowsViewport);

/*-----------------------------------------------------------------------------
	UWindowsViewport Init/Exit.
-----------------------------------------------------------------------------*/


//
// Constructor.
//
UWindowsViewport::UWindowsViewport()
:	UViewport()
,	Status( WIN_ViewportOpening )
{
	guard(UWindowsViewport::UWindowsViewport);
	Window = new WWindowsViewportWindow( this );

	// Set color bytes based on screen resolution.
	HWND hwndDesktop = GetDesktopWindow();
	HDC  hdcDesktop  = GetDC(hwndDesktop);
	switch( GetDeviceCaps( hdcDesktop, BITSPIXEL ) )
	{
		case 32: 
			ColorBytes		= 4;
			FullscreenOnly	= 0;
			break;
		case 8:
		case 16:
		case 24:
		default: 
			ColorBytes		= 4;
			FullscreenOnly	= 0;
			break;
	}

	// Init other stuff.
	ReleaseDC( hwndDesktop, hdcDesktop );
	SavedCursor.x = -1;

	StandardCursors[0] = LoadCursorIdX(NULL, IDC_ARROW);
	StandardCursors[1] = LoadCursorIdX(NULL, IDC_SIZEALL);
	StandardCursors[2] = LoadCursorIdX(NULL, IDC_SIZENESW);
	StandardCursors[3] = LoadCursorIdX(NULL, IDC_SIZENS);
	StandardCursors[4] = LoadCursorIdX(NULL, IDC_SIZENWSE);
	StandardCursors[5] = LoadCursorIdX(NULL, IDC_SIZEWE);
	StandardCursors[6] = LoadCursorIdX(NULL, IDC_WAIT);

	// Get mouse. (shared between viewports)
	if ( !GIsEditor && (Mouse == NULL) )
	{
		HRESULT hr;
		if( FAILED( hr = DirectInput8->CreateDevice( GUID_SysMouse, &Mouse, NULL ) ) )
			DirectInputError( TEXT("Couldn't create mouse device"), hr, true ); 

		// Set data format.
		if( FAILED( hr = Mouse->SetDataFormat( &c_dfDIMouse2 ) ) )
			DirectInputError( TEXT("Couldn't set mouse data format"), hr, true );
	
		// Set mouse buffer.
		DIPROPDWORD Property;

		Property.diph.dwSize		= sizeof(DIPROPDWORD);
	    Property.diph.dwHeaderSize	= sizeof(DIPROPHEADER);
	    Property.diph.dwObj			= 0;
	    Property.diph.dwHow			= DIPH_DEVICE;
	    Property.dwData				= 1024;	// buffer size
 
		if ( FAILED( hr = Mouse->SetProperty(DIPROP_BUFFERSIZE, &Property.diph) ) )
			DirectInputError( TEXT("Couldn't set mouse buffer size"), hr, true );

		// Set mouse axis mode.
		Property.dwData				= DIPROPAXISMODE_REL;

		if ( FAILED( hr = Mouse->SetProperty(DIPROP_AXISMODE, &Property.diph) ) )
			DirectInputError( TEXT("Couldn't set relative axis mode for mouse"), hr, true );
	}

	// Joystick. (shared between viewports)
	if ( !GIsEditor && (Joystick == NULL) )
	{
		HRESULT hr;
		if( FAILED( hr = DirectInput8->EnumDevices( 
			DI8DEVCLASS_GAMECTRL, 
			EnumJoysticksCallback,
			NULL, 
			DIEDFL_ATTACHEDONLY 
			)))
			DirectInputError( TEXT("Couldn't enumerate devices"), hr, true );

		// Make sure we got a joystick
		if( Joystick != NULL)
		{
			// Set joystick data format.
			if( FAILED( hr = Joystick->SetDataFormat( &c_dfDIJoystick2 ) ) )
				DirectInputError( TEXT("Couldn't set joystick data format"), hr, true );

			// Get caps.
			JoystickCaps.dwSize = sizeof(DIDEVCAPS);
			if ( FAILED( hr = Joystick->GetCapabilities(&JoystickCaps) ) )
				DirectInputError( TEXT("Couldn't get joystick caps"), hr, true );

			// Set axis range.
		    if ( FAILED( hr = Joystick->EnumObjects( EnumAxesCallback, NULL, DIDFT_AXIS ) ) )
				DirectInputError( TEXT("Couldn't enumerate joystick axis"), hr, true );
		}
	}

	Captured = false;
	LastJoyPOV	= IK_None;
	CurrentIMESize	= 0;
	SupportsIME		= 0;

	unguard;
}

//
// Destroy.
//
void UWindowsViewport::Destroy()
{
	guard(UWindowsViewport::Destroy);
	Super::Destroy();

	// Release devices.
	if( Mouse )
	{
		Mouse->Release();
		Mouse = NULL;
	}
	if( Joystick )
	{
		Joystick->Release();
		Joystick = NULL;
	}

	if( BlitFlags & BLIT_Temporary )
		appFree( ScreenPointer );
	Window->MaybeDestroy();
	delete Window;
	Window = NULL;

	unguard;
}

//
// Error shutdown.
//
void UWindowsViewport::ShutdownAfterError()
{
	RenDev->RestoreGamma();
	if( Window->hWnd )
	{
		DestroyWindow( Window->hWnd );
	}
	Super::ShutdownAfterError();
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

//
// Command line.
//
UBOOL UWindowsViewport::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UWindowsViewport::Exec);
	if( UViewport::Exec( Cmd, Ar ) )
	{
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GETSYSTEMLANGUAGE")) )
	{
		LCID Locale = MAKELCID( GetSystemDefaultLangID(), SORT_DEFAULT );
		TCHAR LangAbrev[10];		
		if(	TCHAR_CALL_OS( 
				GetLocaleInfoW( Locale, LOCALE_SABBREVLANGNAME, LangAbrev, 10 ), 
				GetLocaleInfoA( Locale, LOCALE_SABBREVLANGNAME, (LPSTR)LangAbrev, 10 ) 
		))
			Ar.Log( *FString(LangAbrev).Left(2) );
		else
			Ar.Log( TEXT("EN") );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("ENDFULLSCREEN")) )
	{
		if( BlitFlags & BLIT_Fullscreen )
			EndFullscreen();
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEFULLSCREEN")) )
	{
		ToggleFullscreen();
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("TOGGLEIME")) )
	{
		UBOOL Enable = 0;

		if( appStrcmp(Cmd,TEXT("")) != 0 ) 
			Enable = appAtoi(Cmd);

		if( SupportsIME )
		{
			if( !Enable )
			{
				ImmAssociateContext( Window->hWnd, NULL );
				CurrentIMESize	= 0;
			}
			else
			{
				HIMC hImc = ImmGetContext( Window->hWnd );
				if( !hImc )
				{
					debugf(TEXT("Creating IME context."));
					hImc = ImmCreateContext();
					if( hImc )	
						ImmAssociateContext( Window->hWnd, hImc ); 
					else
						SupportsIME = 0;

					CurrentIMESize	= 0;
				}
				else
				{
					ImmAssociateContext( Window->hWnd, hImc );
					ImmReleaseContext( Window->hWnd, hImc );
				}
			}
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GETCURRENTRES")) )
	{
		Ar.Logf( TEXT("%ix%i"), SizeX, SizeY ); 
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GETCURRENTCOLORDEPTH")) )
	{
		Ar.Logf( TEXT("%i"), (ColorBytes?ColorBytes:2)*8 );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GETCOLORDEPTHS")) )
	{
		Ar.Log( TEXT("32") ); //16
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("GETCURRENTRENDERDEVICE")) )
	{
		Ar.Log( RenDev->GetClass()->GetPathName() );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("SETRES")) )
	{
		INT X=appAtoi(Cmd);
		TCHAR* CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT Y=appAtoi(CmdTemp);
		Cmd = CmdTemp;
		CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT BPP=appAtoi(CmdTemp);
		Cmd = CmdTemp;
		UBOOL	Fullscreen = IsFullscreen() || FullscreenOnly;
		if(appStrchr(Cmd,'w') || appStrchr(Cmd,'W'))
			Fullscreen = 0;
		else if(appStrchr(Cmd,'f') || appStrchr(Cmd,'F'))
			Fullscreen = 1;
		if( X && Y )
		{
			INT ColorBytes = 0;
			switch( BPP )
			{
			case 16:
				ColorBytes = 2;
				break;
			case 24:
			case 32:
				ColorBytes = 4;
				break;
			}
			HoldCount++;
			UBOOL Result = RenDev->SetRes( this, X, Y, Fullscreen || FullscreenOnly, ColorBytes );
			HoldCount--;
			if( !Result )
				EndFullscreen();
		}
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("TEMPSETRES")) )
	{
		INT X=appAtoi(Cmd);
		TCHAR* CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT Y=appAtoi(CmdTemp);
		Cmd = CmdTemp;
		CmdTemp = appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : TEXT("");
		INT BPP=appAtoi(CmdTemp);
		Cmd = CmdTemp;
		UBOOL	Fullscreen = IsFullscreen() || FullscreenOnly;
		if(appStrchr(Cmd,'w') || appStrchr(Cmd,'W'))
			Fullscreen = 0;
		else if(appStrchr(Cmd,'f') || appStrchr(Cmd,'F'))
			Fullscreen = 1;

		UWindowsClient* Client = GetOuterUWindowsClient();
		INT				SavedX,
						SavedY;
		if(Fullscreen)
		{
			SavedX = Client->FullscreenViewportX;
			SavedY = Client->FullscreenViewportY;
		}
		else
		{
			SavedX = Client->WindowedViewportX;
			SavedY = Client->WindowedViewportY;
		}

		if( X && Y )
		{
			INT ColorBytes = 0;
			switch( BPP )
		{
			case 16:
				ColorBytes = 2;
				break;
			case 24:
			case 32:
				ColorBytes = 4;
				break;
			}
			HoldCount++;
			UBOOL Result = RenDev->SetRes( this, X, Y, Fullscreen || FullscreenOnly, ColorBytes,0 );
			HoldCount--;
			if( !Result )
			EndFullscreen();
		}

		if(Fullscreen)
		{
			Client->FullscreenViewportX = SavedX;
			Client->FullscreenViewportY = SavedY;
		}
		else
		{
			Client->WindowedViewportX = SavedX;
			Client->WindowedViewportY = SavedY;
		}
		Client->SaveConfig();

		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("SETMOUSE")) )
	{
	    const TCHAR* CmdTemp = Cmd;
		INT X, Y;
		
		X = appAtoi(CmdTemp);
		
		while( appIsDigit( *CmdTemp) )
		    CmdTemp++;

		while( appIsSpace( *CmdTemp) )
		    CmdTemp++;
		    
		Y = appAtoi(CmdTemp);
		
		POINT p;
		p.x = X; p.y = Y;
        ::ClientToScreen( Window->hWnd, &p );
	    ::SetCursorPos( p.x, p.y );
	
        return 1;
	}
	else 
		return 0;
	unguard;
}

/*-----------------------------------------------------------------------------
	Window openining and closing.
-----------------------------------------------------------------------------*/

#if 1 //NEW (mdf) windowed mode offset support
void UWindowsViewport::SetWindowOffsets( UWindowsClient* Client, INT ClientWidth, INT ClientHeight, INT& OffsetX, INT& OffsetY )
{
	FRect rcWork;
    SystemParametersInfoA( SPI_GETWORKAREA, 0, &rcWork, 0 );

	if( OffsetX == -1 )
	{
		// not yet set
		OffsetX = Client->WindowedOffsetX;
		INT DeltaX = rcWork.Width() - ClientWidth;
		if( OffsetX == -1 )
		{
			// -1 means center window horizontally
			OffsetX = DeltaX / 2;
		}
		else
		{
			// make sure window is entirely visible in X
			OffsetX = Max( 0, Min( DeltaX, OffsetX ) );
		}
	}

	if( OffsetY == -1 )
	{
		// not yet set
		OffsetY = Client->WindowedOffsetY;
		INT DeltaY = rcWork.Height() - ClientHeight;
		if( OffsetY == -1 )
		{
			// -1 means center window vertically
			OffsetY = DeltaY / 2;
		}
		else
		{
			// make sure window is entirely visible in Y
			OffsetY = Max( 0, Min( DeltaY, OffsetY ) );
		}
	}
}
#endif

//
// Open this viewport's window.
//
void UWindowsViewport::OpenWindow( DWORD InParentWindow, UBOOL IsTemporary, INT NewX, INT NewY, INT OpenX, INT OpenY )
{
	guard(UWindowsViewport::OpenWindow);

	// Check resolution.
	HWND hwndDesktop = GetDesktopWindow();
	HDC  hdcDesktop  = GetDC(hwndDesktop);
	if ( GetDeviceCaps( hdcDesktop, BITSPIXEL ) != 32 && GIsEditor )
		appErrorf(NAME_FriendlyError, TEXT("Editor requires desktop set to 32 bit resolution"));
	
	check(Actor);
	check(!HoldCount);
	UBOOL DoRepaint=0, DoSetActive=0;
	UWindowsClient* C = GetOuterUWindowsClient();
	if( NewX!=INDEX_NONE )
		NewX = Align( NewX, 2 );

	// User window of launcher if no parent window was specified.
	if( !InParentWindow )
		Parse( appCmdLine(), TEXT("HWND="), InParentWindow );

	// Create frame buffer.
	if( IsTemporary )
	{
		// Create in-memory data.
		BlitFlags     = BLIT_Temporary;
		ColorBytes    = 2;
		SizeX         = NewX;
		SizeY         = NewY;
		ScreenPointer = (BYTE*)appMalloc( 2 * NewX * NewY, TEXT("TemporaryViewportData") );	
		Window->hWnd  = NULL;
		debugf( NAME_Log, TEXT("Opened temporary viewport") );
   	}
	else
	{
		// Clear BlitFlags.
		BlitFlags = 0;

		// Figure out physical window size we must specify to get appropriate client area.
		FRect rTemp( 100, 100, (NewX!=INDEX_NONE?NewX:C->WindowedViewportX) + 100, (NewY!=INDEX_NONE?NewY:C->WindowedViewportY) + 100 );

		// Get style and proper rectangle.
		DWORD Style;
		if( InParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
		{
			Style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS;
   			AdjustWindowRect( rTemp, Style, 0 );
		}
		else
		{
			Style = WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
   			AdjustWindowRect( rTemp, Style, FALSE );
		}

		// Set position and size.
#if 1 //NEW (mdf) windowed mode offset support
		SetWindowOffsets( C, rTemp.Width(), rTemp.Height(), OpenX, OpenY );
		if( SavedWindowRect.Width() == 0 || SavedWindowRect.Height() == 0 )
		{
			// started full screen so didn't save off window rect yet?
			SavedWindowRect = FRect( OpenX, OpenY, OpenX + rTemp.Width(), OpenY + rTemp.Height() );
		}		
#else
		if( OpenX==-1 )
			OpenX = CW_USEDEFAULT;
		if( OpenY==-1 )
			OpenY = CW_USEDEFAULT;
#endif
		INT OpenXL = rTemp.Width();
		INT OpenYL = rTemp.Height();

		// Create or update the window.
		if( !Window->hWnd )
		{
			// Creating new viewport.
			ParentWindow = (HWND)InParentWindow;

			// Open the physical window.
			Window->PerformCreateWindowEx
			(
				WS_EX_APPWINDOW,
				TEXT(""),
				Style,
				OpenX, OpenY,
				OpenXL, OpenYL,
				ParentWindow,
				NULL,
				hInstance
			);

			// Set parent window.
			if( ParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
			{
				// Force this to be a child.
				SetWindowLongX( Window->hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP );
			}
			debugf( NAME_Log, TEXT("Opened viewport") );
			DoSetActive = DoRepaint = 1;
		}
		else
		{
			// Resizing existing viewport.
			//!!only needed for old vb code
			SetWindowPos( Window->hWnd, NULL, OpenX, OpenY, OpenXL, OpenYL, SWP_NOACTIVATE );
		}
		
		ShowWindow( Window->hWnd, SW_SHOWNOACTIVATE );
		if( DoRepaint )
			UpdateWindow( Window->hWnd );
	}

	// Create rendering device.
	if( !RenDev && !appStrcmp( GetName(), TEXT("VisibiltyViewport") ) )
		TryRenderDevice( TEXT("Editor.VisibilityRenderDevice"), NewX, NewY, 0 );
	if( !RenDev && (BlitFlags & BLIT_Temporary) )
		TryRenderDevice( TEXT("D3DDrv.D3DRenderDevice"), NewX, NewY, 0 );
	if( !RenDev && !GIsEditor && ParseParam(appCmdLine(),TEXT("windowed")) )
		TryRenderDevice( TEXT("ini:Engine.Engine.RenderDevice"), NewX, NewY, 0 );
	if( !RenDev )
		TryRenderDevice( TEXT("ini:Engine.Engine.RenderDevice"), NewX, NewY, GIsEditor ? 0 : C->StartupFullscreen );

	// Set cooperative level.
	HRESULT hr;
	DWORD Flags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

	if( Mouse )
	{
		Mouse->Unacquire();
		if( FAILED( hr = Mouse->SetCooperativeLevel( Window->hWnd, Flags ) ) )
			DirectInputError( TEXT("Couldn't set cooperative level"), hr, true );
		Mouse->Acquire();
	}

	if( Joystick )
	{
		Joystick->Unacquire();
		if( FAILED( hr = Joystick->SetCooperativeLevel( Window->hWnd, Flags ) ) )
			DirectInputError( TEXT("Couldn't set cooperative level"), hr, true );
		Joystick->Acquire();
	}

	check(RenDev);
	RenDev->UpdateGamma( this );
	UpdateWindowFrame();
	//if( DoRepaint )
		//Repaint( 1 );
	if( DoSetActive )
		SetActiveWindow( Window->hWnd );

	HIMC hImc = ImmGetContext( Window->hWnd );
	if( !hImc )
	{
		debugf(TEXT("Creating IME context."));
		hImc = ImmCreateContext();
		if( hImc )	
			ImmAssociateContext( Window->hWnd, hImc ); 
		else
			debugf(TEXT("OS doesn't support IME."));
	}
	else
		ImmReleaseContext( Window->hWnd, hImc );

	SupportsIME = hImc != NULL;

	unguard;
}

//
// Close a viewport window.  Assumes that the viewport has been opened with
// OpenViewportWindow.  Does not affect the viewport's object, only the
// platform-specific information associated with it.
//
void UWindowsViewport::CloseWindow()
{
	guard(UWindowsViewport::CloseWindow);
	if( Window->hWnd && Status==WIN_ViewportNormal )
	{
		// IME stuff.
		HIMC hImc = ImmGetContext( Window->hWnd );
		if( hImc )
		{
			ImmAssociateContext( Window->hWnd, NULL );
			ImmDestroyContext( hImc );
		}

		Status = WIN_ViewportClosing;
		DestroyWindow( Window->hWnd );
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	UWindowsViewport operations.
-----------------------------------------------------------------------------*/

//
// Set window position according to menu's on-top setting:
//
void UWindowsViewport::SetTopness()
{
	guard(UWindowsViewport::SetTopness);
	HWND Topness = HWND_NOTOPMOST;
	if( GetMenu(Window->hWnd) && GetMenuState(GetMenu(Window->hWnd),ID_ViewTop,MF_BYCOMMAND)&MF_CHECKED )
		Topness = HWND_TOPMOST;
	SetWindowPos( Window->hWnd, Topness, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE );
	unguard;
}

//
// Repaint the viewport.
//
void UWindowsViewport::Repaint( UBOOL Blit )
{
	guard(UWindowsViewport::Repaint);
	GetOuterUWindowsClient()->Engine->Draw( this, Blit );
	unguard;
}

//
// Return whether fullscreen.
//
UBOOL UWindowsViewport::IsFullscreen()
{
	guard(UWindowsViewport::IsFullscreen);
	return (BlitFlags & BLIT_Fullscreen)!=0;
	unguard;
}

//
// Set the mouse cursor according to Unreal or UnrealEd's mode, or to
// an hourglass if a slow task is active.
//
void UWindowsViewport::SetModeCursor()
{
	guard(UWindowsViewport::SetModeCursor);
	if( GIsSlowTask )
	{
		SetCursor(LoadCursorIdX(NULL,IDC_WAIT));
		return;
	}
	HCURSOR hCursor = NULL;
	switch( GetOuterUWindowsClient()->Engine->edcamMode(this) )
	{
		case EM_ViewportZoom:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_CameraZoom); break;
		case EM_ActorRotate:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushRot); break;
		case EM_ActorScale:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushScale); break;
		case EM_ActorSnapScale:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushSnap); break;
		case EM_TexturePan:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexPan); break;
		case EM_TextureRotate:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexRot); break;
		case EM_TextureScale:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexScale); break;
		case EM_None: 				hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_ViewportMove:		hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_TexView:			hCursor = LoadCursorIdX(NULL,IDC_ARROW); break;
		case EM_TexBrowser:			hCursor = LoadCursorIdX(NULL,IDC_ARROW); break;
		case EM_StaticMeshBrowser:	hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_MeshView:			hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_Animation:			hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_VertexEdit:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_VertexEdit); break;
		case EM_BrushClip:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushClip); break;
		case EM_FaceDrag:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_FaceDrag); break;
		case EM_Polygon:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_BrushWarp); break;
		case EM_TerrainEdit:
		{
			switch( GetOuterUWindowsClient()->Engine->edcamTerrainBrush() )
			{
				case TB_VertexEdit:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_VertexEdit); break;
				case TB_Paint:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Paint); break;
				case TB_Smooth:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Smooth); break;
				case TB_Noise:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Noise); break;
				case TB_Flatten:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Flatten); break;
				case TB_TexturePan:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexPan); break;
				case TB_TextureRotate:	hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexRot); break;
				case TB_TextureScale:	hCursor = LoadCursorIdX(hInstance,IDCURSOR_TexScale); break;
				case TB_Select:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Selection); break;
				case TB_Visibility:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Visibility); break;
				case TB_Color:			hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_Color); break;
				case TB_EdgeTurn:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_TB_EdgeTurn); break;
				default:				hCursor = LoadCursorIdX(hInstance,IDCURSOR_TerrainEdit); break;
			}
		}
		break;
		case EM_PrefabBrowser:	hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		case EM_Matinee:	 	hCursor = LoadCursorIdX(hInstance,IDCURSOR_Matinee); break;
		case EM_EyeDropper:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_EyeDropper); break;
		case EM_FindActor:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_FindActor); break;
		case EM_Geometry:		hCursor = LoadCursorIdX(hInstance,IDCURSOR_Geometry); break;
		
		case EM_NewCameraMove:	hCursor = LoadCursorIdX(NULL,IDC_CROSS); break;
		default: 				hCursor = LoadCursorIdX(NULL,IDC_ARROW); break;
	}
	check(hCursor);
	SetCursor( hCursor );
	unguard;
}

//
// Update user viewport interface.
//
void UWindowsViewport::UpdateWindowFrame()
{
	guard(UWindowsViewport::UpdateWindowFrame);

	// If not a window, exit.
	if( HoldCount || !Window->hWnd || (BlitFlags&BLIT_Fullscreen) || (BlitFlags&BLIT_Temporary) || !Actor)
		return;

	// Set viewport window's name to show resolution.
	TCHAR WindowName[80];
	if( !GIsEditor || (Actor->ShowFlags&SHOW_PlayerCtrl) )
	{
		appSprintf( WindowName, LocalizeGeneral("Product",appPackage()) );
	}
	else switch( Actor->RendMap )
	{
		case REN_Wire:		appStrcpy(WindowName,LocalizeGeneral(TEXT("ViewPersp"),TEXT("WinDrv"))); break;
		case REN_OrthXY:	appStrcpy(WindowName,LocalizeGeneral(TEXT("ViewXY")   ,TEXT("WinDrv"))); break;
		case REN_OrthXZ:	appStrcpy(WindowName,LocalizeGeneral(TEXT("ViewXZ")   ,TEXT("WinDrv"))); break;
		case REN_OrthYZ:	appStrcpy(WindowName,LocalizeGeneral(TEXT("ViewYZ")   ,TEXT("WinDrv"))); break;
		default:			appStrcpy(WindowName,LocalizeGeneral(TEXT("ViewOther"),TEXT("WinDrv"))); break;
	}
	Window->SetText( WindowName );

	// Update parent window.
	if( ParentWindow )
	{
		SendMessageX( ParentWindow, WM_CHAR, 0, 0 );
		PostMessageX( ParentWindow, WM_COMMAND, WM_VIEWPORT_UPDATEWINDOWFRAME, 0 );
	}

	unguard;
}

//
// Return the viewport's window.
//
void* UWindowsViewport::GetWindow()
{
	guard(UWindowsViewport::GetWindow);
	return Window->hWnd;
	unguard;
}

/*-----------------------------------------------------------------------------
	Input.
-----------------------------------------------------------------------------*/

//
// Get localized key name.
//
TCHAR* UWindowsViewport::GetLocalizedKeyName( EInputKey Key )
{
	guard(UWindowsViewport::GetLocalizedKeyName);

	TCHAR* KeyName = appStaticString1024();
	
	if( !GetKeyNameText( (DWORD( MapVirtualKey( Key, 0 ) ) ) << 16, KeyName, 63 ) )
		appStrncpy( KeyName, TEXT("UNKNOWN"), 1024 );

	return KeyName;

	unguard;
}


//
// Input event router.
//
UBOOL UWindowsViewport::CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta )
{
	guard(UWindowsViewport::CauseInputEvent);
	return GetOuterUWindowsClient()->Engine->InputEvent( this, (EInputKey)iKey, Action, Delta );
	unguard;
}

//
// If the cursor is currently being captured, stop capturing, clipping, and 
// hiding it, and move its position back to where it was when it was initially
// captured.
//
void UWindowsViewport::SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL OnlyFocus )
{
	guard(UWindowsViewport::SetMouseCapture);

	// If only handling focus windows, exit out.
	if( OnlyFocus )
		if( Window->hWnd != GetFocus() )
			return;

	// If capturing, windows requires clipping in order to keep focus.
	Clip |= Capture;

	// Get window rectangle.
	RECT TempRect;
	::GetClientRect( Window->hWnd, &TempRect );
	MapWindowPoints( Window->hWnd, NULL, (POINT*)&TempRect, 2 );

	// Handle capturing.
	if( Capture )
	{
		if( SavedCursor.x == -1 )
		{
			// Bring window to the top (including all Win32 mojo).
			HWND hWndForeground = ::GetForegroundWindow();
			UBOOL Attach		= (hWndForeground == Window->hWnd);

			if( Attach )
				AttachThreadInput(GetWindowThreadProcessId(hWndForeground, NULL), GetCurrentThreadId(), true);

			SetForegroundWindow( Window->hWnd );
			SetActiveWindow( Window->hWnd );
			
			if( Attach )
				AttachThreadInput(GetWindowThreadProcessId(hWndForeground, NULL), GetCurrentThreadId(), false);

			// Confine cursor to window.
			::GetCursorPos( &SavedCursor );
			OrigCursor = FVector( SavedCursor.x, SavedCursor.y, 0 );

			if( GetOuterUWindowsClient()->Engine->edcamMouseControl( this ) == MOUSEC_Locked )
			{
				SetCursorPos( (TempRect.left+TempRect.right)/2, (TempRect.top+TempRect.bottom)/2 );
			}

			// Start capturing cursor.
			SetCapture( Window->hWnd );
//			SystemParametersInfoX( SPI_SETMOUSE, 0, GetOuterUWindowsClient()->CaptureMouseInfo, 0 );
			while( ShowCursor(FALSE)>=0 );
		}
	}
	else
	{
		// Release captured cursor.
		if( !(BlitFlags & BLIT_Fullscreen) )
		{
			SetCapture( NULL );
//			SystemParametersInfoX( SPI_SETMOUSE, 0, GetOuterUWindowsClient()->NormalMouseInfo, 0 );
		}

		// Restore position.
		if( SavedCursor.x != -1 )
		{
			if( GetOuterUWindowsClient()->Engine->edcamMouseControl( this ) == MOUSEC_Locked )
			{
				SetCursorPos( SavedCursor.x, SavedCursor.y );
			}

			SavedCursor.x = -1;
			while( ShowCursor(TRUE)<0 );
		}
	}

	// Handle clipping.
	ClipCursor( Clip ? &TempRect : NULL );

	// Memorize state.
	Captured = Capture;

	unguard;
}


#if 1 //NEW (mdf) extended input
void UWindowsViewport::MaybeRemapFunctionKey( EInputKey& Key )
{
	if( Input->KeyDown(IK_Shift) )
	{
		// map shift-F1..shift-F12 to F13..F24
		if( Key >= VK_F1 && Key <= VK_F12 )
		{
			Key = EInputKey( (int)Key + (VK_F13-VK_F1) );
		}
	}
	else if( Input->KeyDown(IK_Ctrl) )
	{
		// map control-F1..control-F12 to IK_Unknown92..IK_Unknown9D 
		if( Key >= VK_F1 && Key <= VK_F12 )
		{
			Key = EInputKey( ((int)Key-VK_F1) + IK_Unknown92 );
		}
	}
	else if( Input->KeyDown(IK_Alt) )
	{
		// map alt-F1..alt-F12 to IK_UnknownA4..IK_UnknownAF
		if( Key >= VK_F1 && Key <= VK_F12 )
		{
			Key = EInputKey( ((int)Key-VK_F1) + IK_UnknownA4 );
		}
	}
}

UBOOL UWindowsViewport::RemappedMouseEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
{
	EInputKey OldKey = Key;

	if( !GIsEditor && GetOuterUWindowsClient()->RemapMouseEvents )
	{
		if( Input->KeyDown(IK_Shift) )
		{
			if( Key == IK_LeftMouse )
				Key = IK_UnknownB0; // shift-left mouse
			else if( Key == IK_MiddleMouse )
				Key = IK_UnknownB1; // shift-middle mouse
			else if( Key == IK_RightMouse )
				Key = IK_UnknownB2; // shift-right mouse
		}
		else if( Input->KeyDown(IK_Ctrl) )
		{
			if( Key == IK_LeftMouse )
				Key = IK_UnknownB3; // ctrl-left mouse
			else if( Key == IK_MiddleMouse )
				Key = IK_UnknownB4; // ctrl-middle mouse
			else if( Key == IK_RightMouse )
				Key = IK_UnknownB5; // ctrl-right mouse
		}
		else if( Input->KeyDown(IK_Alt) )
		{
			if( Key == IK_LeftMouse )
				Key = IK_UnknownB6; // alt-left mouse
			else if( Key == IK_MiddleMouse )
				Key = IK_UnknownB7; // alt-middle mouse
			else if( Key == IK_RightMouse )
				Key = IK_UnknownB8; // alt-right mouse
		}
	
		if( Key != OldKey )
			CauseInputEvent( Key, Action, Delta );
	}

	return (Key != OldKey);
}

void UWindowsViewport::CauseMaybeRemappedMouseEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
{
	if( !RemappedMouseEvent( Key, Action ) )
		CauseInputEvent( Key, Action, Delta );
}
#endif

//
// Update input for this viewport.
//
void UWindowsViewport::UpdateInput( UBOOL Reset, FLOAT DeltaSeconds )
{
	guard(UWindowsViewport::UpdateInput);

	// IME stuff.
	if( SupportsIME )
	{
		HIMC hImc = ImmGetContext( Window->hWnd );
		if( !hImc )
	{
			debugf(TEXT("Creating IME context."));
			hImc = ImmCreateContext();
			if( hImc )	
				ImmAssociateContext( Window->hWnd, hImc ); 
		else
				SupportsIME = 0;

			CurrentIMESize = 0;
	}
		else
			ImmReleaseContext( Window->hWnd, hImc );
}

	BYTE Processed[256];
	appMemset( Processed, 0, 256 );
	//debugf(TEXT("%i"),(INT)GTempDouble);

	// From Legend - fix for key strokes left in input buffer
	// e.g. bind a key to exit, press and hold it down while exiting,
	// next time engine starts, appExit will be called because of
	// remaining key strokes left in input buffer without this fix.
	if( Reset )
	{
		BYTE KeyStates[256];
		appMemset( KeyStates, 0, 256 );

		// don't wipe out toggleable key settings:
		KeyStates[IK_NumLock]		= GetKeyState(IK_NumLock);
		KeyStates[IK_CapsLock]		= GetKeyState(IK_CapsLock);
		KeyStates[IK_ScrollLock]	= GetKeyState(IK_ScrollLock);
        KeyStates[IK_Ctrl]			= GetKeyState(IK_Ctrl);
        KeyStates[IK_Shift]			= GetKeyState(IK_Shift);
        KeyStates[IK_Alt]			= GetKeyState(IK_Alt);

		SetKeyboardState(KeyStates);
	}

	HRESULT hr;

	// DirectInput Joystick.
	if( Joystick && GetOuterUWindowsClient()->UseJoystick )
	{
		DIJOYSTATE2 State;
 
		// Poll the device and read the current state.
		if( FAILED( hr = Joystick->Poll() ) || FAILED( hr = Joystick->GetDeviceState( sizeof(DIJOYSTATE2), &State ) ))  
		{
			Joystick->Acquire();
		}
		else
		{		
			// Pass buttons to the input system.
			for( INT i=0; i<12; i++ )
			{
				INT Button = IK_Joy1 + i;
				if( !Input->KeyDown( Button ) && (State.rgbButtons[i] & 0x80) )
					CauseInputEvent( Button, IST_Press );
				else if( Input->KeyDown( Button ) && !(State.rgbButtons[i] & 0x80) )
					CauseInputEvent( Button, IST_Release );
				Processed[IK_Joy1+i] = 1;				
			}
		
			// Joystick POV support.
			DWORD		POV			= State.rgdwPOV[0];
			UBOOL		POVCentered = (LOWORD(POV) == 0xFFFF);
			EInputKey	ThisJoyPOV	= IK_None;

			if( POVCentered )
				ThisJoyPOV = IK_None;
			else if( POV <  4500 )
				ThisJoyPOV = IK_Joy13;
			else if( POV < 13500 )
				ThisJoyPOV = IK_Joy14;
			else if( POV < 22500 )
				ThisJoyPOV = IK_Joy15;
			else if( POV < 31500 )
				ThisJoyPOV = IK_Joy16;
			else
				ThisJoyPOV = IK_Joy13;

			if( LastJoyPOV != ThisJoyPOV )
			{
				if( LastJoyPOV != IK_None )
					CauseInputEvent( LastJoyPOV, IST_Release );
				if( ThisJoyPOV != IK_None )	
					CauseInputEvent( ThisJoyPOV, IST_Press );		
				LastJoyPOV = ThisJoyPOV;
			} 

			Processed[IK_Joy13] = 1;
			Processed[IK_Joy14] = 1;				
			Processed[IK_Joy15] = 1;				
			Processed[IK_Joy16] = 1;				

			// Pass axis to the input system.
			Input->DirectAxis( IK_JoyX, (State.lX - 32768.f) / 65535.f, DeltaSeconds );
			Input->DirectAxis( IK_JoyY, (State.lY - 32768.f) / 65535.f, DeltaSeconds );
			Input->DirectAxis( IK_JoyZ, (State.lZ - 32768.f) / 65535.f, DeltaSeconds );

			Input->DirectAxis( IK_JoyR, (State.lRx - 32768.f) / 65535.f, DeltaSeconds );
			Input->DirectAxis( IK_JoyU, (State.lRy - 32768.f) / 65535.f, DeltaSeconds );
			Input->DirectAxis( IK_JoyV, (State.lRz - 32768.f) / 65535.f, DeltaSeconds );

			Input->DirectAxis( IK_JoySlider1, (State.rglSlider[0] - 32768.f) / 65535.f, DeltaSeconds );
			Input->DirectAxis( IK_JoySlider2, (State.rglSlider[1] - 32768.f) / 65535.f, DeltaSeconds );

			Processed[IK_JoyX] = 1;
			Processed[IK_JoyY] = 1;
			Processed[IK_JoyZ] = 1;
			Processed[IK_JoyR] = 1;
			Processed[IK_JoyU] = 1;
			Processed[IK_JoyV] = 1;
			Processed[IK_JoySlider1]	= 1;
			Processed[IK_JoySlider2]	= 1;
		}
	}

	// DirectInput mouse.
	DWORD Elements = 1;
	FLOAT	MouseXMultiplier	= GetOuterUWindowsClient()->MouseXMultiplier,
			MouseYMultiplier	= GetOuterUWindowsClient()->MouseYMultiplier;

	// Backward compatibility with old inis.
	if( MouseXMultiplier == 0.f )
		MouseXMultiplier = 1.f;
	if( MouseYMultiplier == 0.f )
		MouseYMultiplier = 1.f;

	while( !GIsEditor && Elements && Mouse )
	{
        DIDEVICEOBJECTDATA Event;
 
		if (FAILED(hr = Mouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &Event, &Elements, 0) ) )
		{	
			Mouse->Acquire();
			Elements = 0;
		}
		else if ( Elements )
		{
			// Look at the element to see what occurred
			switch (Event.dwOfs) 
			{
			case DIMOFS_X: 
#if 1 //NEW (mdf) extended input
				CauseMaybeRemappedMouseEvent( IK_MouseX, IST_Axis, +MouseXMultiplier * ((INT) Event.dwData) );
#else
				CauseInputEvent( IK_MouseX, IST_Axis, +MouseXMultiplier * ((INT) Event.dwData) );
#endif
				break;
			case DIMOFS_Y: 
#if 1 //NEW (mdf) extended input
				CauseMaybeRemappedMouseEvent( IK_MouseY, IST_Axis, -MouseYMultiplier * ((INT) Event.dwData) );
#else
				CauseInputEvent( IK_MouseY, IST_Axis, -MouseYMultiplier * ((INT) Event.dwData) );
#endif
                break; 
			case DIMOFS_Z:
#if 1 //NEW (mdf) extended input
				CauseMaybeRemappedMouseEvent( IK_MouseW, IST_Axis, (INT) Event.dwData );
				if( ((INT)Event.dwData) < 0)
				{
					CauseMaybeRemappedMouseEvent( IK_MouseWheelDown, IST_Press );
					CauseMaybeRemappedMouseEvent( IK_MouseWheelDown, IST_Release );
				}
				else if( ((INT)Event.dwData) > 0)
				{
					CauseMaybeRemappedMouseEvent( IK_MouseWheelUp, IST_Press );
					CauseMaybeRemappedMouseEvent( IK_MouseWheelUp, IST_Release );
				}
#else
				CauseInputEvent( IK_MouseW, IST_Axis, (INT) Event.dwData );
				if( ((INT)Event.dwData) < 0)
				{
					CauseInputEvent( IK_MouseWheelDown, IST_Press );
					CauseInputEvent( IK_MouseWheelDown, IST_Release );
				}
				else if( ((INT)Event.dwData) > 0)
				{
					CauseInputEvent( IK_MouseWheelUp, IST_Press );
					CauseInputEvent( IK_MouseWheelUp, IST_Release );
				}
#endif
				break;
            case DIMOFS_BUTTON0: // left button
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_LeftMouse, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_LeftMouse, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_LeftMouse, IST_Press );
				else
					CauseInputEvent( IK_LeftMouse, IST_Release );
#endif
				break;
            case DIMOFS_BUTTON1: // right button
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_RightMouse, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_RightMouse, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_RightMouse, IST_Press );
				else
					CauseInputEvent( IK_RightMouse, IST_Release );
#endif
				break;
			case DIMOFS_BUTTON2: // middle button
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_MiddleMouse, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_MiddleMouse, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_MiddleMouse, IST_Press );
				else
					CauseInputEvent( IK_MiddleMouse, IST_Release );
#endif
				break;

			case DIMOFS_BUTTON3:
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_Mouse4, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_Mouse4, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_Mouse4, IST_Press );
				else
					CauseInputEvent( IK_Mouse4, IST_Release );
#endif
				break;
			case DIMOFS_BUTTON4:
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_Mouse5, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_Mouse5, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_Mouse5, IST_Press );
				else
					CauseInputEvent( IK_Mouse5, IST_Release );
#endif
				break;
			case DIMOFS_BUTTON5:
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_Mouse6, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_Mouse6, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_Mouse6, IST_Press );
				else
					CauseInputEvent( IK_Mouse6, IST_Release );
#endif
				break;
			case DIMOFS_BUTTON6:
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_Mouse7, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_Mouse7, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_Mouse7, IST_Press );
				else
					CauseInputEvent( IK_Mouse7, IST_Release );
#endif
				break;
			case DIMOFS_BUTTON7:
#if 1 //NEW (mdf) extended input
				if (Event.dwData & 0x80) 
					CauseMaybeRemappedMouseEvent( IK_Mouse8, IST_Press );
				else
					CauseMaybeRemappedMouseEvent( IK_Mouse8, IST_Release );
#else
				if (Event.dwData & 0x80) 
					CauseInputEvent( IK_Mouse8, IST_Press );
				else
					CauseInputEvent( IK_Mouse8, IST_Release );
#endif
				break;
			default:
                break;        
			}
		}
	}

	Processed[IK_LeftMouse]		= 1;
	Processed[IK_RightMouse]	= 1;
	Processed[IK_MiddleMouse]	= 1;
	Processed[IK_Mouse4]		= 1;
	Processed[IK_Mouse5]		= 1;
	Processed[IK_Mouse6]		= 1;
	Processed[IK_Mouse7]		= 1;
	Processed[IK_Mouse8]		= 1;

#if 1 //NEW (mdf) extended input
	INT idx;
	for( idx=IK_F13; idx<=IK_F24; idx++ ) 
		Processed[idx] = Input->KeyDown(IK_Shift);		// shift-Fxx
	for( idx=IK_Unknown92; idx<=IK_Unknown9D; idx++ )
		Processed[idx] = Input->KeyDown(IK_Ctrl);		// ctrl-Fxx
	for( idx=IK_UnknownA4; idx<=IK_UnknownAF; idx++ )
		Processed[idx] = Input->KeyDown(IK_Alt);		// alt-Fxx

	for( idx=IK_UnknownB0; idx<=IK_UnknownB2; idx++ )	
		Processed[idx] = Input->KeyDown(IK_Shift);		// shift-mouse button
	for( idx=IK_UnknownB3; idx<=IK_UnknownB5; idx++ )
		Processed[idx] = Input->KeyDown(IK_Ctrl);		// ctrl-mouse button
	for( idx=IK_UnknownB6; idx<=IK_UnknownB8; idx++ )
		Processed[idx] = Input->KeyDown(IK_Alt);		// alt-mouse button
#endif

	// Keyboard.
	Reset = Reset && GetFocus()==Window->hWnd;
	for( INT i=0; i<256; i++ )
	{
		if( !Processed[i] )
		{
			if( !Input->KeyDown(i) )
			{
				//old: if( Reset && (GetAsyncKeyState(i) & 0x8000) )
				if( Reset && (GetKeyState(i) & 0x8000) )
					CauseInputEvent( i, IST_Press );
			}
			else
			{
				//old: if( !(GetAsyncKeyState(i) & 0x8000) )
				if( !(GetKeyState(i) & 0x8000) )
					CauseInputEvent( i, IST_Release );
			}
		}
	}

	unguard;
}


/*-----------------------------------------------------------------------------
	Viewport WndProc.
-----------------------------------------------------------------------------*/

// Figures out the current combination of mouse/system keys.
DWORD UWindowsViewport::GetViewportButtonFlags( DWORD wParam )
{
	WORD Buttons = wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON);
	DWORD ViewportButtonFlags = 0;
	if( Buttons & MK_LBUTTON     ) ViewportButtonFlags |= MOUSE_Left;
	if( Buttons & MK_RBUTTON     ) ViewportButtonFlags |= MOUSE_Right;
	if( Buttons & MK_MBUTTON     ) ViewportButtonFlags |= MOUSE_Middle;
	if( Input->KeyDown(IK_Shift) ) ViewportButtonFlags |= MOUSE_Shift;
	if( Input->KeyDown(IK_Ctrl ) ) ViewportButtonFlags |= MOUSE_Ctrl;
	if( Input->KeyDown(IK_Alt  ) ) ViewportButtonFlags |= MOUSE_Alt;

	return ViewportButtonFlags;
}

//
// Main viewport window function.
//
LONG UWindowsViewport::ViewportWndProc( UINT iMessage, WPARAM wParam, LPARAM lParam )
{
	guard(UWindowsViewport::ViewportWndProc);
	UWindowsClient* Client = GetOuterUWindowsClient();
	if( HoldCount || Client->Viewports.FindItemIndex(this)==INDEX_NONE || !Actor )
		return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );

	// Statics.
	static UBOOL MovedSinceLeftClick=0;
	static UBOOL MovedSinceRightClick=0;
	static UBOOL MovedSinceMiddleClick=0;
	static DWORD StartTimeLeftClick=0;
	static DWORD StartTimeRightClick=0;
	static DWORD StartTimeMiddleClick=0;

	// Updates.
	if( iMessage==WindowMessageMouseWheel )
	{
		iMessage = WM_MOUSEWHEEL;
		wParam   = MAKEWPARAM(0,wParam);
	}

	// Message handler.
	switch( iMessage )
	{
		case WM_CREATE:
		{
			guard(WM_CREATE);

			// Set status.
			Status = WIN_ViewportNormal; 

			// Make this viewport current and update its title bar.
			GetOuterUClient()->MakeCurrent( this );

			// Disable IME (input method editor) for Japanese Windows
			ImmAssociateContext( Window->hWnd, NULL );

			// Fix for the "losing focus when starting up fullscreen" bug.
			if( !GIsEditor )
				SetFocus(Window->hWnd);

			return 0;
			unguard;
		}
		case WM_HOTKEY:
		{
			return 0;
		}
		case WM_DESTROY:
		{
			guard(WM_DESTROY);

			// If there's an existing Viewport structure corresponding to
			// this window, deactivate it.
			if( BlitFlags & BLIT_Fullscreen )
				EndFullscreen();

			// Restore focus to caller if desired.
			DWORD ParentWindow=0;
			Parse( appCmdLine(), TEXT("HWND="), ParentWindow );
			if( ParentWindow )
			{
				::SetParent( Window->hWnd, NULL );
				SetFocus( (HWND)ParentWindow );
			}

			// Stop clipping.
			SetDrag( 0 );
			if( Status==WIN_ViewportNormal )
			{
				// Closed by user clicking on window's close button, so delete the viewport.
				Status = WIN_ViewportClosing; // Prevent recursion.
				delete this;
			}
			debugf( NAME_Log, TEXT("Closed viewport") );
			return 0;
			unguard;
		}
		case WM_PAINT:
		{
			guard(WM_PAINT);
			if( BlitFlags & (BLIT_Fullscreen|BLIT_Direct3D|BLIT_OpenGL|BLIT_HardwarePaint) )
			{
				DirtyViewport = 1;
				ValidateRect( Window->hWnd, NULL );
				return 0;
			}
			else return 1;
			unguard;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			guard(WM_KEYDOWN);

			// Capture mouse if focus was lost.
			if( !GIsEditor && Captured )
				SetMouseCapture(1,1,0);

			// Get key code.
			EInputKey Key = (EInputKey)wParam;

			// Send key to input system.
			if( Key==IK_Enter && (GetKeyState(VK_MENU)&0x8000) )
			{
				ToggleFullscreen();
			}
#if 1 //NEW (mdf) extended input
			else
			{
				MaybeRemapFunctionKey( Key );
				if( (Key>=0) && (Key<IK_MAX) && CauseInputEvent( Key, IST_Press ) && GIsEditor && !IsRealtime() )
					Repaint( 1 ); 	// Redraw if the viewport won't be redrawn on timer.
			}
#else
			else if( (Key>=0) && (Key<IK_MAX) && CauseInputEvent( Key, IST_Press ) && GIsEditor && !IsRealtime() )
					Repaint( 1 ); 	// Redraw if the viewport won't be redrawn on timer.
#endif

			// Send to UnrealEd.
			if( ParentWindow && GIsEditor )
			{
				if( Key==IK_F1 )
					PostMessageX( ParentWindow, iMessage, IK_F2, lParam );
				else if( Key!=IK_Tab && Key!=IK_Enter && Key!=IK_Alt )
					PostMessageX( ParentWindow, iMessage, wParam, lParam );
			}

			// Set the cursor.
			if( GIsEditor )
				SetModeCursor();

			return 0;
			unguard;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			guard(WM_KEYUP);

			// Send to the input system.
			EInputKey Key = (EInputKey)wParam;
#if 1 //NEW (mdf) extended input
			MaybeRemapFunctionKey( Key );
#endif
			if( (Key>=0) && (Key<IK_MAX) && CauseInputEvent( Key, IST_Release ) && GIsEditor && !IsRealtime() )
					Repaint( 1 );	// Redraw if the viewport won't be redrawn on timer.

			// Pass keystroke on to UnrealEd.
			if( ParentWindow && GIsEditor )
			{				
				if( Key == IK_F1 )
					PostMessageX( ParentWindow, iMessage, IK_F2, lParam );
				else if( Key!=IK_Tab && Key!=IK_Enter && Key!=IK_Alt )
					PostMessageX( ParentWindow, iMessage, wParam, lParam );
			}
			if( GIsEditor )
				SetModeCursor();
			return 0;
			unguard;
		}
		case WM_SYSCHAR:
		case WM_CHAR:
		{
			guard(WM_CHAR);
			EInputKey Key = (EInputKey)wParam;
			TCHAR Unicode = GUnicodeOS ? wParam : TCHAR(Key);
			if( (Key!=IK_Enter && Client->Engine->Key( this, Key, Unicode )) )
			{
				if( GIsEditor && !IsRealtime() )
				{
					// Redraw if needed.
					Repaint( 1 );
					SetModeCursor();
				}
			}
			else if( iMessage == WM_SYSCHAR )
			{
				// Perform default processing.
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			}
			return 0;
			unguard;
		}
		case WM_IME_COMPOSITION:
		{
			guard(WM_IME_COMPOSITION);
			if( lParam & GCS_RESULTSTR )
			{
				HIMC hImc = ImmGetContext(Window->hWnd);

				if( !hImc )
					appErrorf( TEXT("No IME context") );

				// Get the size of the result string.
				INT Size = ImmGetCompositionString( hImc, GCS_RESULTSTR, NULL, 0 );

				TCHAR* String = new TCHAR[Size+1];
				appMemzero( String, sizeof(TCHAR) * (Size+1) );

				// Get the result strings that is generated by IME.
				Size = ImmGetCompositionString( hImc, GCS_RESULTSTR, String, Size );
				Size /= sizeof( TCHAR );

				for( INT i=0; i<CurrentIMESize; i++ )
				{
					CauseInputEvent( IK_Backspace, IST_Press );
					CauseInputEvent( IK_Backspace, IST_Release );
				}

				for( INT i=0; i<Size; i++ )
				{
					INT Key = String[i];
					if( Key )
						Client->Engine->Key( this, IK_Unicode, String[i] );
				}

				delete [] String;

				ImmReleaseContext(Window->hWnd, hImc);

				CurrentIMESize = 0;
			}
			else if( lParam & GCS_COMPSTR ) 
			{
				HIMC hImc = ImmGetContext(Window->hWnd);

				if( !hImc )
					appErrorf( TEXT("No IME context") );

				// Get the size of the result string.
				INT Size = ImmGetCompositionString( hImc, GCS_COMPSTR, NULL, 0 );

				TCHAR* String = new TCHAR[Size+1];
				appMemzero( String, sizeof(TCHAR) * (Size+1) );

				// Get the result strings that is generated by IME.
				Size = ImmGetCompositionString( hImc, GCS_COMPSTR, String, Size );
				Size /= sizeof( TCHAR );

				for( INT i=0; i<CurrentIMESize; i++ )
				{
					CauseInputEvent( IK_Backspace, IST_Press );
					CauseInputEvent( IK_Backspace, IST_Release );
				}
				
				for( INT i=0; i<Size; i++ )
				{
					INT Key = String[i];
					if( Key )
						Client->Engine->Key( this, IK_Unicode, String[i] );
				}

				delete [] String;

				ImmReleaseContext(Window->hWnd, hImc);

				CurrentIMESize = Size;
			}
			else
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			
			return 0;
			unguard;
		}
		case WM_ERASEBKGND:
		{
			// Prevent Windows from repainting client background in white.
			return 0;
		}
		case WM_SETCURSOR:
		{
			guard(WM_SETCURSOR);
			if( (LOWORD(lParam)==1) || GIsSlowTask )
			{
				// In client area or processing slow task.
				if( GIsEditor )
					SetModeCursor();
				return 0;
			}
			else
			{
				// Out of client area.
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			}
			unguard;
		}
		case WM_LBUTTONDBLCLK:
		{
			if( GIsEditor && SizeX && SizeY && !(BlitFlags&BLIT_Fullscreen) ) //!!
			{
				Client->Engine->Click( this, MOUSE_LeftDouble, LOWORD(lParam), HIWORD(lParam) );
				//if( !IsRealtime() )
				//	Repaint( 1 );
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			guard(WM_BUTTONDOWN);

			if( !GIsEditor || Client->InMenuLoop )
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );

			// Doing this allows the editor to know where the mouse was last clicked (in
			// world coordinates) without having to do hit tests and such.  Useful for
			// a few things ...
			if( GIsEditor )
			{
				// Figure out where the mouse was clicked, in world coordinates.

				FCameraSceneNode	SceneNode(this,&RenderTarget,Actor,Actor->Location,Actor->Rotation,Actor->FovAngle);
				FCanvasUtil			CanvasUtil(&RenderTarget,NULL);

				FVector	V = SceneNode.Deproject(
					CanvasUtil.CanvasToScreen.TransformFPlane(
						FPlane(
							LOWORD(lParam),
							HIWORD(lParam),
							0.0f,
							1.0f
							)
						)
					);

				Client->Engine->edSetClickLocation(V);
			}

			// Send a notification message to the editor frame.  
			//!!This relies on the window hierarchy not changing ... if it does, update this!
			HWND hwndEditorFrame = GetParent(GetParent(GetParent(ParentWindow)));
			SendMessageX( hwndEditorFrame, WM_COMMAND, WM_EDC_VIEWPORTUPDATEWINDOWFRAME, 0 );
			Client->Engine->Click( this, GetViewportButtonFlags(wParam), LOWORD(lParam), HIWORD(lParam) );

			if( iMessage == WM_LBUTTONDOWN )
			{
#if 1 //NEW (mdf) extended input
				if( RemappedMouseEvent( IK_LeftMouse, IST_Press) )
					return 0;
#endif
				MovedSinceLeftClick = 0;
				StartTimeLeftClick = GetMessageTime();
				CauseInputEvent( IK_LeftMouse, IST_Press );
			}
			else if( iMessage == WM_RBUTTONDOWN )
			{
#if 1 //NEW (mdf) extended input
				if( RemappedMouseEvent( IK_RightMouse, IST_Press) )
					return 0;
#endif
				MovedSinceRightClick = 0;
				StartTimeRightClick = GetMessageTime();
				CauseInputEvent( IK_RightMouse, IST_Press );
			}
			else if( iMessage == WM_MBUTTONDOWN )
			{
#if 1 //NEW (mdf) extended input
				if( RemappedMouseEvent( IK_MiddleMouse, IST_Press) )
					return 0;
#endif
				MovedSinceMiddleClick = 0;
				StartTimeMiddleClick = GetMessageTime();
				CauseInputEvent( IK_MiddleMouse, IST_Press );
			}
			SetDrag( 1 );
			return 0;
			unguard;
		}
		case WM_MOUSEACTIVATE:
		{
			// Activate this window and send the mouse-down message.
			return MA_ACTIVATE;
		}
		case WM_ACTIVATE:
		{
			guard(WM_ACTIVATE);
			if ( wParam == WA_INACTIVE )
				SetDrag(0);
			return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			unguard;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			guard(WM_BUTTONUP);

			// Exit if in menu loop.
			if( !GIsEditor || Client->InMenuLoop )
				return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );

#if 1 //NEW (mdf) extended input
			if( (iMessage == WM_LBUTTONUP) && RemappedMouseEvent( IK_LeftMouse, IST_Release) )
				return 0;
			if( (iMessage == WM_RBUTTONUP) && RemappedMouseEvent( IK_RightMouse, IST_Release) )
				return 0;
			if( (iMessage == WM_MBUTTONUP) && RemappedMouseEvent( IK_MiddleMouse, IST_Release) )
				return 0;
#endif
			// Get mouse cursor position.
			POINT TempPoint={0,0};
			::ClientToScreen( Window->hWnd, &TempPoint );
			INT MouseX = SavedCursor.x!=-1 ? SavedCursor.x-TempPoint.x : LOWORD(lParam);
			INT MouseY = SavedCursor.x!=-1 ? SavedCursor.y-TempPoint.y : HIWORD(lParam);

			// Get time interval to determine if a click occured.
			INT DeltaTime, Button;
			EInputKey iKey;
			if( iMessage == WM_LBUTTONUP )
			{
				DeltaTime = GetMessageTime() - StartTimeLeftClick;
				iKey      = IK_LeftMouse;
				Button    = MOUSE_Left;
			}
			else if( iMessage == WM_MBUTTONUP )
			{
				DeltaTime = GetMessageTime() - StartTimeMiddleClick;
				iKey      = IK_MiddleMouse;
				Button    = MOUSE_Middle;
			}
			else
			{
				DeltaTime = GetMessageTime() - StartTimeRightClick;
				iKey      = IK_RightMouse;
				Button    = MOUSE_Right;
			}

			// Send to the input system.
			CauseInputEvent( iKey, IST_Release );

			// Release the cursor.
			if
			(	!Input->KeyDown(IK_LeftMouse)
			&&	!Input->KeyDown(IK_MiddleMouse)
			&&	!Input->KeyDown(IK_RightMouse) 
			&&	!(BlitFlags & BLIT_Fullscreen) )
                SetDrag( 0 );

			// Handle viewport clicking.
			if
			(	!(BlitFlags & BLIT_Fullscreen)
			&&	SizeX && SizeY 
			&&	!(MovedSinceLeftClick || MovedSinceRightClick || MovedSinceMiddleClick)
			)
			{
				Client->Engine->UnClick( this, Button, MouseX, MouseY );
				if( !IsRealtime() )
					Repaint( 1 );
			}

			// Update times.
			if		( iMessage == WM_LBUTTONUP ) MovedSinceLeftClick	= 0;
			else if	( iMessage == WM_RBUTTONUP ) MovedSinceRightClick	= 0;
			else if	( iMessage == WM_MBUTTONUP ) MovedSinceMiddleClick	= 0;

			return 0;
			unguard;
		}
		case WM_ENTERMENULOOP:
		{
			guard(WM_ENTERMENULOOP);
			Client->InMenuLoop = 1;
			SetDrag( 0 );
			UpdateWindowFrame();
			if( Mouse )
				Mouse->Unacquire();
			if( Joystick )
				Joystick->Unacquire();
			return 0;
			unguard;
		}
		case WM_EXITMENULOOP:
		{
			guard(WM_EXITMENULOOP);
			Client->InMenuLoop = 0;
			if( Mouse )
				Mouse->Acquire();
			if( Joystick )
				Joystick->Acquire();
			return 0;
			unguard;
		}
		case WM_CANCELMODE:
		{
			guard(WM_CANCELMODE);
			SetDrag( 0 );
			return 0;
			unguard;
		}
		case WM_MOUSEWHEEL:
		{		
			guard(WM_MOUSEWHEEL);
			SWORD zDelta = HIWORD(wParam);
			if( GIsEditor && zDelta )
			{
				CauseInputEvent( IK_MouseW, IST_Axis, zDelta );
				if( zDelta < 0 )
				{
					CauseInputEvent( IK_MouseWheelDown, IST_Press );
					CauseInputEvent( IK_MouseWheelDown, IST_Release );
				}
				else if( zDelta > 0 )
				{
					CauseInputEvent( IK_MouseWheelUp, IST_Press );
					CauseInputEvent( IK_MouseWheelUp, IST_Release );
				}

				if( GIsEditor )
				{
					Client->Engine->MouseWheel( this, GetViewportButtonFlags(wParam), zDelta );
				}
			}
			return 0;
			unguard;
		}
		case WM_MOUSEMOVE:
		{
			guard(WM_MOUSEMOVE);
			if ( GIsEditor )
			{
				// If in a window, see if cursor has been captured; if not, ignore mouse movement.
				if( Client->InMenuLoop )
					break;

				// Get window rectangle.
				RECT TempRect;
				::GetClientRect( Window->hWnd, &TempRect );
				WORD Buttons = wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON);

				// Update the reference mouse positions
				if( GIsEditor )
				{
					POINT pt;
					::GetCursorPos( &pt );
					MouseScreenPos = FVector( pt.x, pt.y, 0 );

					::ScreenToClient( (HWND)GetWindow(), &pt );
					MouseClientPos = FVector( pt.x, pt.y, 0 );
				}

				// If cursor isn't captured, just do MousePosition.
				if( (!(BlitFlags & BLIT_Fullscreen) && SavedCursor.x==-1) || GetOuterUWindowsClient()->Engine->edcamMouseControl( this ) == MOUSEC_Free )
				{
					// Do mouse messaging.
					POINTS Point = MAKEPOINTS(lParam);
					DWORD ViewportButtonFlags = 0;
					if( Buttons & MK_LBUTTON     ) ViewportButtonFlags |= MOUSE_Left;
					if( Buttons & MK_RBUTTON     ) ViewportButtonFlags |= MOUSE_Right;
					if( Buttons & MK_MBUTTON     ) ViewportButtonFlags |= MOUSE_Middle;
					if( Input->KeyDown(IK_Shift) ) ViewportButtonFlags |= MOUSE_Shift;
					if( Input->KeyDown(IK_Ctrl ) ) ViewportButtonFlags |= MOUSE_Ctrl;
					if( Input->KeyDown(IK_Alt  ) ) ViewportButtonFlags |= MOUSE_Alt;

					Client->Engine->MousePosition( this, Buttons, Point.x-TempRect.left, Point.y-TempRect.top );

					if( GetOuterUWindowsClient()->Engine->edcamMouseControl( this ) == MOUSEC_Free )
						Client->Engine->MouseDelta( this, ViewportButtonFlags, 0, 0 );

					if( bShowWindowsMouse && SelectedCursor >= 0 && SelectedCursor <= 6 )
						SetCursor( StandardCursors[SelectedCursor] );
					break;
				}

				// Get center of window.			
				POINT TempPoint, Base;
				TempPoint.x = (TempRect.left + TempRect.right )/2;
				TempPoint.y = (TempRect.top  + TempRect.bottom)/2;
				Base = TempPoint;

				// Movement accumulators.
				UBOOL Moved=0;
				INT Cumulative=0;

				// Grab all pending mouse movement.
				INT DX=0, DY=0;
				Loop:
				Buttons		  = wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON);
				POINTS Points = MAKEPOINTS(lParam);
				INT X         = Points.x - Base.x;
				INT Y         = Points.y - Base.y;
				Cumulative += Abs(X) + Abs(Y);
				DX += X;
				DY += Y;

				// Process valid movement.
				DWORD ViewportButtonFlags = 0;
				if( Buttons & MK_LBUTTON     ) ViewportButtonFlags |= MOUSE_Left;
				if( Buttons & MK_RBUTTON     ) ViewportButtonFlags |= MOUSE_Right;
				if( Buttons & MK_MBUTTON     ) ViewportButtonFlags |= MOUSE_Middle;
				if( Input->KeyDown(IK_Shift) ) ViewportButtonFlags |= MOUSE_Shift;
				if( Input->KeyDown(IK_Ctrl ) ) ViewportButtonFlags |= MOUSE_Ctrl;
				if( Input->KeyDown(IK_Alt  ) ) ViewportButtonFlags |= MOUSE_Alt;

				// Move viewport with buttons.
				if( X || Y )
				{
					Moved = 1;
					Client->Engine->MouseDelta( this, ViewportButtonFlags, X, Y );
				}

				// Handle any more mouse moves.
				MSG Msg;
				if( PeekMessageX( &Msg, Window->hWnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE ) )
				{
					lParam = Msg.lParam;
					wParam = Msg.wParam;
					Base.x = Points.x;
					Base.y = Points.y;
					goto Loop;
				}

				// Set moved-flags.
				if( Cumulative>4 )
				{
					if( wParam & MK_LBUTTON ) MovedSinceLeftClick   = 1;
					if( wParam & MK_RBUTTON ) MovedSinceRightClick  = 1;
					if( wParam & MK_MBUTTON ) MovedSinceMiddleClick = 1;
				}

				// Send to input subsystem.
				if( DX ) CauseInputEvent( IK_MouseX, IST_Axis, +DX );
				if( DY ) CauseInputEvent( IK_MouseY, IST_Axis, -DY );

				// Put cursor back in middle of window.
				if( ( DX || DY ) && GetOuterUWindowsClient()->Engine->edcamMouseControl( this ) == MOUSEC_Locked )
				{
					::ClientToScreen( Window->hWnd, &TempPoint );
					SetCursorPos( TempPoint.x, TempPoint.y );
				}

				// Viewport isn't realtime, so we must update the frame here and now.
				if( Moved && !IsRealtime() )
				{
					if( Input->KeyDown(IK_Space) )
						for( INT i=0; i<Client->Viewports.Num(); i++ )
							Client->Viewports(i)->Repaint( 1 );
					else
						Repaint( 1 );
				}

				// Dispatch keyboard events.
				while( PeekMessageX( &Msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE ) )
				{
					TranslateMessage( &Msg );
					DispatchMessageX( &Msg );
				}
			}

			else
			{
				//FIXMEJOE

				// This code had to be added in order to provide the proper Mouse X and Y coords
				// to the viewport.  It must have been removed at some point.

				POINT pt;
				::GetCursorPos( &pt );
				::ScreenToClient( (HWND)GetWindow(), &pt );

				Client->Viewports(0)->WindowsMouseX = pt.x;
				Client->Viewports(0)->WindowsMouseY = pt.y;

				if( bShowWindowsMouse && SelectedCursor >= 0 && SelectedCursor <= 6 )
					SetCursor( StandardCursors[SelectedCursor] );
			}
			//	return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			return 0;
			unguard;
		}
		case WM_SIZE:
		{
			guard(WM_SIZE);
			INT NewX = LOWORD(lParam);
			INT NewY = HIWORD(lParam);

			UBOOL ForceFullscreen = 0;

			if( wParam == SIZE_MAXIMIZED )
			{
				SetMouseCapture(1,1,0);
				ForceFullscreen = 1;
			}
			if( wParam == SIZE_MINIMIZED )
				SetMouseCapture(0,0,0);
#if 1
			if( BlitFlags & BLIT_Fullscreen )
			{
				// Window forced out of fullscreen.
				if( wParam==SIZE_RESTORED )
				{
					HoldCount++;
					Window->MoveWindow( SavedWindowRect, 1 );
					HoldCount--;
				}
				return 0;
			}
			else
#endif
			{
				// Use resized window.
				if( RenDev && (BlitFlags & (BLIT_OpenGL|BLIT_Direct3D)) )
				{
					RenDev->SetRes( this, NewX, NewY, (BlitFlags & BLIT_Fullscreen) || FullscreenOnly || ForceFullscreen );
				}
				else
				{
					ResizeViewport( BlitFlags|BLIT_NoWindowChange, NewX, NewY );
				}
				if( GIsEditor )
				{
#if 0
					Repaint( 0 );
#else
					DirtyViewport = -1;
#endif
				}
      			return 0;
        	}
			unguard;
		}
		case WM_KILLFOCUS:
		{
			guard(WM_KILLFOCUS);

			DWORD ProcID;
			HWND hWnd = wParam ? (HWND) wParam : ::GetForegroundWindow();

			GetWindowThreadProcessId(hWnd, &ProcID);
			if( ProcID != GetCurrentProcessId() )
				RenDev->RestoreGamma();

            UBOOL OldCaptured = Captured; // Remember if we want to re-capture.
			SetMouseCapture( 0, 0, 0 );
			Captured = OldCaptured;
			
			SetDrag( 0 );
			if( !GIsEditor )
				Input->ResetInput();

			return 0;
			unguard;
		}
		case WM_SETFOCUS:
		{
			guard(WM_SETFOCUS);

			// Grab the mouse.
			if( !GIsEditor && Captured )
				SetMouseCapture( 1, 1, 0 );

			// Reset viewport's input.
			if( Input )
				Input->ResetInput();

			// Acquire devices.
			if( Mouse )
				Mouse->Acquire();
			if( Joystick )
				Joystick->Acquire();

			// Make this viewport current.
			GetOuterUClient()->MakeCurrent( this );
			SetModeCursor();

			// Disable IME (input method editor) for Japanese Windows
			ImmAssociateContext( Window->hWnd, NULL );

			if( RenDev )
				RenDev->UpdateGamma(this);
 
			return 0;
			unguard;
		}
		case WM_SYSCOMMAND:
		{
			guard(WM_SYSCOMMAND);
			DWORD nID = wParam & 0xFFF0;
			if( nID==SC_SCREENSAVE || nID==SC_MONITORPOWER )
			{
				// Return 1 to prevent screen saver.
				if( nID==SC_SCREENSAVE )
					debugf( NAME_Log, TEXT("Received SC_SCREENSAVE") );
				else
					debugf( NAME_Log, TEXT("Received SC_MONITORPOWER") );
				return 0;
			}
			else if( nID==SC_MAXIMIZE )
			{
				// Maximize.
				ToggleFullscreen();
				return 0;
			}
			else if
			(	(BlitFlags & BLIT_Fullscreen)
			&&	(nID==SC_NEXTWINDOW || nID==SC_PREVWINDOW || nID==SC_TASKLIST || nID==SC_HOTKEY) )
			{
				// Don't allow window changes here.
				return 0;
			}
			else
				return DefWindowProcX(Window->hWnd,iMessage,wParam,lParam);
			unguard;
		}
		case WM_POWER:
		{
			guard(WM_POWER);
			if( wParam )
			{
				if( wParam == PWR_SUSPENDREQUEST )
				{
					debugf( NAME_Log, TEXT("Received WM_POWER suspend") );

					// Prevent powerdown if dedicated server or using joystick.
					if( 1 )
						return PWR_FAIL;
					else
						return PWR_OK;
				}
				else
				{
					debugf( NAME_Log, TEXT("Received WM_POWER") );
					return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
				}
			}
			return 0;
			unguard;
		}
		case WM_DISPLAYCHANGE:
		{
			guard(WM_DISPLAYCHANGE);
			debugf( NAME_Log, TEXT("Viewport %s: WM_DisplayChange"), GetName() );
			unguard;
			return 0;
		}
		default:
		{
			guard(WM_UNKNOWN);
			return DefWindowProcX( Window->hWnd, iMessage, wParam, lParam );
			unguard;
		}
	}
	unguard;
	return 0;
}
W_IMPLEMENT_CLASS(WWindowsViewportWindow)


/*-----------------------------------------------------------------------------
	Lock and Unlock.
-----------------------------------------------------------------------------*/

//
// Lock the viewport window and set the approprite Screen and RealScreen fields
// of Viewport.  Returns 1 if locked successfully, 0 if failed.  Note that a
// lock failing is not a critical error; it's a sign that a DirectDraw mode
// has ended or the user has closed a viewport window.
//
UBOOL UWindowsViewport::Lock( BYTE* HitData, INT* HitSize )
{
	guard(UWindowsViewport::Lock);

	// Make sure window is lockable.
	if( (Window->hWnd && !IsWindow(Window->hWnd)) || HoldCount || !SizeX || !SizeY || !RenDev )
      	return 0;

	// Get info.
	Stride = SizeX;

	// Success here, so pass to superclass.
	return UViewport::Lock(HitData,HitSize);

	unguard;
}

//
// Unlock the viewport window.  If Blit=1, blits the viewport's frame buffer.
//
void UWindowsViewport::Unlock()
{
	guard(UWindowsViewport::Unlock);

	check(!HoldCount);
	UViewport::Unlock();

	unguard;
}

/*-----------------------------------------------------------------------------
	Viewport modes.
-----------------------------------------------------------------------------*/

//
// Try switching to a new rendering device.
//
void UWindowsViewport::TryRenderDevice( const TCHAR* ClassName, INT NewX, INT NewY, UBOOL Fullscreen )
{
	guard(UWindowsViewport::TryRenderDevice);

	// Shut down current rendering device.
	guard(ShutdownOldRenDev);
	if( RenDev && RenDev != GetOuterUWindowsClient()->Engine->GRenDev ) 
	{
		RenDev->Exit(this);
		delete RenDev;
		RenDev = NULL;
	}
	unguard;

	// Only allow fullscreen if window can be brought on top.
	HWND hWndForeground = ::GetForegroundWindow();
	UBOOL Focus			= 1;
	UBOOL Attach		= (hWndForeground == Window->hWnd);
	if( Attach )
		AttachThreadInput(GetWindowThreadProcessId(hWndForeground, NULL), GetCurrentThreadId(), true);
	if( !SetForegroundWindow( Window->hWnd ) && !Attach)
	{
		debugf( TEXT("Couldn't bring window to foreground.") );
		Focus = 0;
	}
	if( !SetActiveWindow( Window->hWnd ) )
	{
		debugf( TEXT("Couldn't set window as active one.") );
		Focus = 0;
	}	
	if( Attach )
		AttachThreadInput(GetWindowThreadProcessId(hWndForeground, NULL), GetCurrentThreadId(), false);

	// Use appropriate defaults.
	UWindowsClient* C = GetOuterUWindowsClient();
	if( NewX==INDEX_NONE )
		NewX = ((Fullscreen && Focus) || FullscreenOnly) ? C->FullscreenViewportX : C->WindowedViewportX;
	if( NewY==INDEX_NONE )
		NewY = ((Fullscreen && Focus) || FullscreenOnly) ? C->FullscreenViewportY : C->WindowedViewportY;

	// Find device driver.
	UClass* RenderClass = UObject::StaticLoadClass( URenderDevice::StaticClass(), NULL, ClassName, NULL, 0, NULL );
	if( RenderClass  )
	{
		HoldCount++;
//!!FIXME Clean up when we remove Softdrv.
		if( RenderClass == GetOuterUWindowsClient()->Engine->GRenDev->GetClass() )
		{
			RenDev = GetOuterUWindowsClient()->Engine->GRenDev;
		}
		else
		{
			RenDev = ConstructObject<URenderDevice>( RenderClass, this );
			RenDev->Init();
		}
		
		FullscreenOnly = RenDev->Is3dfx;

		if( !Focus && !FullscreenOnly )
			Fullscreen = 0;
		
		if( RenDev->SetRes( this, NewX, NewY, Fullscreen | FullscreenOnly ) )
		{
			if( GIsRunning )
				Actor->GetLevel()->DetailChange(
					RenDev->SuperHighDetailActors ? DM_SuperHigh :
					RenDev->HighDetailActors ? DM_High :
					DM_Low
					);
		}
		else
		{
			debugf( NAME_Log, LocalizeError(TEXT("Failed3D"),TEXT("WinDrv")) );
			if( RenDev != GetOuterUWindowsClient()->Engine->GRenDev ) 
				delete RenDev;
			RenDev = NULL;
		}

		if( !Focus && FullscreenOnly )			
		{
			SetMouseCapture( 0, 0, 0 );
			ShowWindow( Window->hWnd, SW_MINIMIZE );
		}

		HoldCount--;
	}
	unguard;
}


//
// If in fullscreen mode, end it and return to Windows.
//
void UWindowsViewport::EndFullscreen()
{
	guard(UWindowsViewport::EndFullscreen);
	UWindowsClient* Client = GetOuterUWindowsClient();
	debugf(TEXT("EndFullscreen"));
	if( RenDev && (BlitFlags & (BLIT_Direct3D|BLIT_OpenGL)) && !FullscreenOnly && !RenDev->Is3dfx )
	{
		RenDev->SetRes( this, Client->WindowedViewportX, Client->WindowedViewportY, 0 );
	}
	else if( RenDev && (BlitFlags & BLIT_OpenGL) )
	{
		RenDev->SetRes( this, INDEX_NONE, INDEX_NONE, FullscreenOnly );
	}
	UpdateWindowFrame();
	unguard;
}

//
// Toggle fullscreen.
//
static UBOOL ToggleFullscreenMutex = false;

void UWindowsViewport::ToggleFullscreen()
{
	guard(UWindowsViewport::ToggleFullscreen);
	if( !ToggleFullscreenMutex )
	{
		ToggleFullscreenMutex = 1;
	if( BlitFlags & BLIT_Fullscreen )
	{
		EndFullscreen();
		SetMouseCapture(1,1,0);
	}
	else if( !(Actor->ShowFlags & SHOW_ChildWindow) )
	{
		debugf(TEXT("AttemptFullscreen"));
		TryRenderDevice( TEXT("ini:Engine.Engine.RenderDevice"), INDEX_NONE, INDEX_NONE, 1 );
	}
		ToggleFullscreenMutex = 0;
	}
	unguard;
}

//
// Resize the viewport.
//
UBOOL UWindowsViewport::ResizeViewport( DWORD NewBlitFlags, INT InNewX, INT InNewY, UBOOL bSaveSize )
{
	guard(UWindowsViewport::ResizeViewport);
	UWindowsClient* Client = GetOuterUWindowsClient();

	// Andrew says: I made this keep the BLIT_Temporary flag so the temporary screen buffer doesn't get leaked
	// during light rebuilds.
	
	// Remember viewport.
	UViewport* SavedViewport = NULL;
	guard(SaveViewport);
	if( Client->Engine->Audio && !GIsEditor && !(GetFlags() & RF_Destroyed) )
		SavedViewport = Client->Engine->Audio->GetViewport();
	unguard;

	// Accept default parameters.
	INT NewX          = InNewX         ==INDEX_NONE ? SizeX      : InNewX;
	INT NewY          = InNewY         ==INDEX_NONE ? SizeY      : InNewY;

	// Get this window rect.
	FRect WindowRect = SavedWindowRect;
	guard(GetWindowRect);
	if( Window->hWnd && !(BlitFlags & BLIT_Fullscreen) && !(NewBlitFlags&BLIT_NoWindowChange) )
		WindowRect = Window->GetWindowRect();
	unguard;

	// Default resolution handling.
	NewX = InNewX!=INDEX_NONE ? InNewX : (NewBlitFlags&BLIT_Fullscreen) ? Client->FullscreenViewportX : Client->WindowedViewportX;
	NewY = InNewX!=INDEX_NONE ? InNewY : (NewBlitFlags&BLIT_Fullscreen) ? Client->FullscreenViewportY : Client->WindowedViewportY;

	// Align NewX.
	check(NewX>=0);
	check(NewY>=0);
	NewX = Align(NewX,2);

	// If currently fullscreen, end it.
	guard(EndFullscreen);
	if( BlitFlags & BLIT_Fullscreen )
	{
		// Saved parameters.
		SetFocus( Window->hWnd );

		// Remember saved info.
		WindowRect          = SavedWindowRect;
		Caps                = SavedCaps;

		// Restore window topness.
		SetTopness();
		SetDrag( 0 );

		// Stop inhibiting windows keys.
		UnregisterHotKey( Window->hWnd, Client->hkAltEsc  );
		UnregisterHotKey( Window->hWnd, Client->hkAltTab  );
		UnregisterHotKey( Window->hWnd, Client->hkCtrlEsc );
		UnregisterHotKey( Window->hWnd, Client->hkCtrlTab );
//		DWORD Old=0;
//		SystemParametersInfoX( SPI_SCREENSAVERRUNNING, 0, &Old, 0 );
	}
	unguard;

	// If transitioning into fullscreen.
	guard(StartFullscreen);
	if( (NewBlitFlags & BLIT_Fullscreen) && !(BlitFlags & BLIT_Fullscreen) )
	{
		// Save window parameters.
		SavedWindowRect = WindowRect;
		SavedCaps       = Caps;

		// Make "Advanced Options" not return fullscreen after this.
		if( Client->ConfigProperties )
		{
			Client->ConfigReturnFullscreen = 0;
			DestroyWindow( *Client->ConfigProperties );
		}

		// Turn off window border and menu.
		HoldCount++;
		SendMessageX( Window->hWnd, WM_SETREDRAW, 0, 0 );
		SetMenu( Window->hWnd, NULL );
		if( !GIsEditor )
		{
			SetWindowLongX( Window->hWnd, GWL_STYLE, GetWindowLongX(Window->hWnd,GWL_STYLE) & ~(WS_CAPTION|WS_THICKFRAME) );
			Borderless = 1;
		}
		SendMessageX( Window->hWnd, WM_SETREDRAW, 1, 0 );
		HoldCount--;
	}
	unguard;

	if( !(NewBlitFlags & BLIT_Temporary) )
	{
		ScreenPointer = NULL;
	}

	// OpenGL and Pixomatic handling.
	if( (NewBlitFlags & BLIT_Fullscreen) 
	&&	!GIsEditor 
	&&	RenDev 
	&&	(appStricmp(RenDev->GetClass()->GetName(),TEXT("PixoRenderDevice"))==0 || appStricmp(RenDev->GetClass()->GetName(),TEXT("OpenGLRenderDevice"))==0) 
	)
	{
		// Turn off window border and menu.
		HoldCount++;
		SendMessageX( Window->hWnd, WM_SETREDRAW, 0, 0 );
		Window->MoveWindow( FRect(0,0,NewX,NewY), 0 );
		SendMessageX( Window->hWnd, WM_SETREDRAW, 1, 0 );
		HoldCount--;
	}

	// Set new info.
	DWORD OldBlitFlags = BlitFlags;
	BlitFlags          = NewBlitFlags & ~BLIT_ParameterFlags;
	SizeX              = NewX;
	SizeY              = NewY;

	// If transitioning out of fullscreen.
	if( !(NewBlitFlags & BLIT_Fullscreen) && (OldBlitFlags & BLIT_Fullscreen) )
	{
		SetMouseCapture( 0, 0, 0 );
	}

	// Handle type.
	guard(HandleType);
	if( NewBlitFlags & BLIT_Fullscreen )
	{	
		// Handle fullscreen input.
		SetDrag( 1 );
		SetMouseCapture( 1, 1, 0 );
		RegisterHotKey( Window->hWnd, Client->hkAltEsc,  MOD_ALT,     VK_ESCAPE );
		RegisterHotKey( Window->hWnd, Client->hkAltTab,  MOD_ALT,     VK_TAB    );
		RegisterHotKey( Window->hWnd, Client->hkCtrlEsc, MOD_CONTROL, VK_ESCAPE );
		RegisterHotKey( Window->hWnd, Client->hkCtrlTab, MOD_CONTROL, VK_TAB    );
//		DWORD Old=0;
//		SystemParametersInfoX( SPI_SCREENSAVERRUNNING, 1, &Old, 0 );
	}
	else if( !(NewBlitFlags & BLIT_Temporary) && !(NewBlitFlags & BLIT_NoWindowChange) )
	{
		// Turn on window border and menu.
		if( Borderless )
		{
			HoldCount++;
			SetWindowLongX( Window->hWnd, GWL_STYLE, GetWindowLongX(Window->hWnd,GWL_STYLE) | (WS_CAPTION|WS_THICKFRAME) );
			HoldCount--;
		}

		// Going to a window.
		FRect ClientRect(0,0,NewX,NewY);
		AdjustWindowRect( ClientRect, GetWindowLongX(Window->hWnd,GWL_STYLE), FALSE );

		// Resize the window and repaint it.
		if( !(Actor->ShowFlags & SHOW_ChildWindow) )
		{
			HoldCount++;
			Window->MoveWindow( FRect(WindowRect.Min,WindowRect.Min+ClientRect.Size()), 1 );
			HoldCount--;
		}
		SetDrag( 0 );
	}
	unguard;

	// Update audio.
	guard(UpdateAudio);
	if( SavedViewport && SavedViewport!=Client->Engine->Audio->GetViewport() )
		Client->Engine->Audio->SetViewport( SavedViewport );
	unguard;

	// Update the window.
	UpdateWindowFrame();

	bWindowsMouseAvailable = !(NewBlitFlags & BLIT_Fullscreen);

	// Save info.
	guard(SaveInfo);
	if( RenDev && !GIsEditor && bSaveSize )
	{
		if( NewBlitFlags & BLIT_Fullscreen )
		{
			if( NewX && NewY )
			{
				Client->FullscreenViewportX  = NewX;
				Client->FullscreenViewportY  = NewY;
			}
		}
		else
		{
			if( NewX && NewY )
			{
				Client->WindowedViewportX  = NewX;
				Client->WindowedViewportY  = NewY;
			}
		}
		Client->SaveConfig();
	}
	unguard;

	return 1;
	unguard;
}

//
// DirectInput joystick callback.
//
BOOL CALLBACK UWindowsViewport::EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext )
{
    HRESULT hr;
	
	if( FAILED( hr = DirectInput8->CreateDevice( pdidInstance->guidInstance, &Joystick, NULL ) ) ) 
	{
		Joystick = NULL;
        return DIENUM_CONTINUE;
	}

    return DIENUM_STOP;
}

BOOL CALLBACK UWindowsViewport::EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext )
{
    DIPROPRANGE diprg; 
    diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    diprg.diph.dwHow        = DIPH_BYOFFSET; 
    diprg.diph.dwObj        = pdidoi->dwOfs; // Specify the enumerated axis
    diprg.lMin              = 0;//-32768; 
    diprg.lMax              = 65535;//+32767; 
    
	// Set the range for the axis
	Joystick->SetProperty( DIPROP_RANGE, &diprg.diph );
//	if( FAILED( Joystick->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
//		return DIENUM_STOP;

    return DIENUM_CONTINUE;
}


LPDIRECTINPUTDEVICE8	UWindowsViewport::Mouse			= NULL;
LPDIRECTINPUTDEVICE8	UWindowsViewport::Joystick		= NULL;
LPDIRECTINPUT8			UWindowsViewport::DirectInput8	= NULL;
DIDEVCAPS				UWindowsViewport::JoystickCaps;


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
