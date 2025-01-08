/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#include "LaunchPrivate.h"
#include "UnEngineWin.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
extern "C" {HINSTANCE hInstance;}
extern "C" {TCHAR GPackage[64]=TEXT("Launch");}
static void SetInitialConfiguration();

// Memory allocator.
#ifdef _DEBUG
	#include "FMallocDebug.h"
	FMallocDebug Malloc;
#else
	#include "FMallocWindows.h"
	FMallocWindows Malloc;
#endif

// Log file.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error handler.
#include "FOutputDeviceWindowsError.h"
FOutputDeviceWindowsError Error;

// Feedback.
#include "FFeedbackContextWindows.h"
FFeedbackContextWindows Warn;

// File manager.
#include "FFileManagerWindows.h"
FFileManagerWindows FileManager;

// Config.
#include "FConfigCacheIni.h"

/*-----------------------------------------------------------------------------
	Video Driver Upgrade dialog.
-----------------------------------------------------------------------------*/

enum EVideoDriverSelection
{
	VD_UPGRADE_NONE			= 0,
	VD_UPGRADE_FROM_CD		= 1,
	VD_UPGRADE_FROM_WEB		= 2
};

class WDlgVideoDrivers : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgVideoDrivers,WDialog,Launch)

	EVideoDriverSelection Selection;

	// Variables.
	WButton UpgradeFromCDButton, UpgradeFromWebButton, CancelButton;
	WLabel TextLabel;

	// Constructor.
	WDlgVideoDrivers( WWindow* InOwnerWindow )
	:	WDialog				( TEXT("VideoDrivers"), IDDIALOG_VideoDrivers, InOwnerWindow )
	,	TextLabel			( this, IDSC_TEXT )
	,	UpgradeFromCDButton	( this, IDPB_UPGRADE_CD, FDelegate(this,(TDelegate)&WDlgVideoDrivers::OnUpgradeFromCD) )
	,	UpgradeFromWebButton( this, IDPB_UPGRADE_WEB, FDelegate(this,(TDelegate)&WDlgVideoDrivers::OnUpgradeFromWeb) )
	,	CancelButton		( this, IDCANCEL, FDelegate(this,(TDelegate)&WDialog::EndDialogFalse) )
	{
		Selection = VD_UPGRADE_NONE;
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgVideoDrivers::OnInitDialog);
		WDialog::OnInitDialog();

		FRect rc = GetWindowRect();

		SetWindowPos(hWnd,HWND_TOPMOST,(GetSystemMetrics(SM_CXSCREEN)-rc.Width())/2,(GetSystemMetrics(SM_CYSCREEN)-rc.Height())/2,rc.Width(),rc.Height(),SWP_SHOWWINDOW);
		SetWindowPos(hWnd,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);

		TextLabel.SetText( Localize(TEXT("UpgradeDrivers"), TEXT("OutdatedDrivers"), TEXT("Engine"), NULL ) );
		UpgradeFromCDButton.SetText( Localize(TEXT("UpgradeDrivers"), TEXT("CDButton"), TEXT("Engine"), NULL ) );
		UpgradeFromWebButton.SetText( Localize(TEXT("UpgradeDrivers"), TEXT("WebButton"), TEXT("Engine"), NULL ) );
		CancelButton.SetText( Localize(TEXT("UpgradeDrivers"), TEXT("cancelButton"), TEXT("Engine"), NULL ) );

		unguard;
	}
	void OnUpgradeFromCD()
	{
		Selection = VD_UPGRADE_FROM_CD;
		EndDialogTrue();
	}
	void OnUpgradeFromWeb()
	{
		Selection = VD_UPGRADE_FROM_WEB;
		EndDialogTrue();
	}
};

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main entry point.
// This is an example of how to initialize and launch the engine.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, INT nCmdShow )
{
	// Remember instance.
	INT ErrorLevel = 0;
	GIsStarted     = 1;
	hInstance      = hInInstance;
	const TCHAR* CmdLine = GetCommandLine();
	appStrcpy( GPackage, appPackage() );

    //_controlfp( 0, _EM_INVALID |_EM_DENORMAL |_EM_ZERODIVIDE |_EM_OVERFLOW |_EM_UNDERFLOW |_EM_INEXACT ); //!!mdf-tbr:

	// See if this should be passed to another instances.
	if
	(	!appStrfind(CmdLine,TEXT("NewWindow"))
	&&	!appStrfind(CmdLine,TEXT("changevideo"))
	&&	!appStrfind(CmdLine,TEXT("TestRenDev")) )
	{
		TCHAR ClassName[256];
		MakeWindowClassName(ClassName,TEXT("WLog"));
		for( HWND hWnd=NULL; ; )
		{
			hWnd = TCHAR_CALL_OS(FindWindowExW(hWnd,NULL,ClassName,NULL),FindWindowExA(hWnd,NULL,TCHAR_TO_ANSI(ClassName),NULL));
			if( !hWnd )
				break;
			if( GetPropX(hWnd,TEXT("IsBrowser")) )
			{
				while( *CmdLine && *CmdLine!=' ' )
					CmdLine++;
				if( *CmdLine==' ' )
					CmdLine++;
				COPYDATASTRUCT CD;
				DWORD Result;
				CD.dwData = WindowMessageOpen;
				CD.cbData = (appStrlen(CmdLine)+1)*sizeof(TCHAR*);
				CD.lpData = const_cast<TCHAR*>( CmdLine );
				SendMessageTimeout( hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&CD, SMTO_ABORTIFHUNG|SMTO_BLOCK, 30000, &Result );
				GIsStarted = 0;
				return 0;
			}
		}
	}

	// Begin guarded code.
#ifndef _DEBUG
	try
	{
#endif
		// Init core.
		GIsClient = GIsGuarded = 1;
		appInit( GPackage, CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );

		// Init mode.
		GIsServer     = 1;
		GIsClient     = !ParseParam(appCmdLine(),TEXT("SERVER"));
		GIsEditor     = 0;
		GIsScriptable = 1;
		GLazyLoad     = !GIsClient || ParseParam(appCmdLine(),TEXT("LAZY"));

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(appCmdLine(),TEXT("LOG"));
		FString Filename = FString(TEXT("..\\Help")) * GPackage + TEXT("Logo.bmp");
		if( GFileManager->FileSize(*Filename)<0 )
			Filename = TEXT("..\\Help\\Logo.bmp");
		appStrcpy( GPackage, appPackage() );
		if( !ShowLog && !ParseParam(appCmdLine(),TEXT("server")) && !appStrfind(appCmdLine(),TEXT("TestRenDev")) )
			InitSplash( *Filename );

		// Set sane default values on initial run.
		INT FirstRun = 0;
		GConfig->GetInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );
		if( GIsClient && (FirstRun==0) )
			SetInitialConfiguration();

		// Exit if wanted.
		if( GIsRequestingExit )
		{
			ExitSplash();
			appPreExit();
			appExit();
			return 0;
		}

#if 1 // DEB - TODO - Remove for release
		{
			INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
			appSystemTime( Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec );
			appSprintf( GCrashLogFilename,
				TEXT("\\\\MegaMonkey\\Vol1\\Projects\\U2XMP\\Logs\\Adhoc\\%s_%s_%04d%02d%02d%02d%02d%02d.log"),
				appComputerName(), appUserName(),
				Year, Month, Day, Hour, Min, Sec
			);
		}
#endif

		// Init windowing.
		InitWindowing();

		// Create log window, but only show it if ShowLog.
		GLogWindow = new WLog( Log.Filename, Log.LogAr, TEXT("GameLog") );
		GLogWindow->OpenWindow( ShowLog, 0 );
		GLogWindow->Log( NAME_Title, LocalizeGeneral(TEXT("Start"),GPackage) );
		if( GIsClient )
			SetPropX( *GLogWindow, TEXT("IsBrowser"), (HANDLE)1 );

		// Benchmarking.
		GIsBenchmarking	= ParseParam(appCmdLine(),TEXT("BENCHMARK"));
		
		// Don't update ini files if benchmarking.
		if( GIsBenchmarking )
		{
			GConfig->Detach( GIni );
			GConfig->Detach( GUserIni );
		}

		// Ugly resolution overriding code.
		FString ScreenWidth;
		FString ScreenHeight;
		UBOOL	OverrideResolution = false;

		//!!vogel: TODO: clean up this code :)
		if( ParseParam(appCmdLine(),TEXT("320x240")) )
		{
			ScreenWidth			= TEXT("320");
			ScreenHeight		= TEXT("240");
			OverrideResolution	= 1;
		}
		if( ParseParam(appCmdLine(),TEXT("512x384")) )
		{
			ScreenWidth			= TEXT("512");
			ScreenHeight		= TEXT("384");
			OverrideResolution	= 1;
		}
		if( ParseParam(appCmdLine(),TEXT("640x480")) )
		{
			ScreenWidth			= TEXT("640");
			ScreenHeight		= TEXT("480");
			OverrideResolution	= 1;
		}
		if( ParseParam(appCmdLine(),TEXT("800x600")) )
		{
			ScreenWidth			= TEXT("800");
			ScreenHeight		= TEXT("600");
			OverrideResolution	= 1;
		}
		if( ParseParam(appCmdLine(),TEXT("1024x768")) )
		{
			ScreenWidth			= TEXT("1024");
			ScreenHeight		= TEXT("768");
			OverrideResolution	= 1;
		}
		if( ParseParam(appCmdLine(),TEXT("1280x960")) )
		{
			ScreenWidth			= TEXT("1280");
			ScreenHeight		= TEXT("960");
			OverrideResolution	= 1;
		}
		if( ParseParam(appCmdLine(),TEXT("1280x1024")) )
		{
			ScreenWidth			= TEXT("1280");
			ScreenHeight		= TEXT("1024");
			OverrideResolution	= 1;
		}
		if( ParseParam(appCmdLine(),TEXT("1600x1200")) )
		{
			ScreenWidth			= TEXT("1600");
			ScreenHeight		= TEXT("1200");
			OverrideResolution	= 1;
		}

		if( OverrideResolution )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportX"), *ScreenWidth  );
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullscreenViewportY"), *ScreenHeight );
		}

		// Init engine.
		UEngine* Engine = InitEngine();
		if( Engine )
		{
#if DEMOVERSION
			Engine->DummyFunctionToBreakCompatibility(1);
#endif
			GLogWindow->Log( NAME_Title, LocalizeGeneral(TEXT("Run"),GPackage) );

			// Hide splash screen.
			ExitSplash();

			// Optionally Exec an exec file
			FString Temp;
			if( Parse(appCmdLine(), TEXT("EXEC="), Temp) )
			{
				Temp = FString(TEXT("exec ")) + Temp;
				if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
					Engine->Client->Viewports(0)->Exec( *Temp, *GLogWindow );
			}

			// Start main engine loop, including the Windows message pump.
			if( !GIsRequestingExit )
				MainLoop( Engine );
		}

		// Clean shutdown.
		GFileManager->Delete(TEXT("Running.ini"),0,0);
		RemovePropX( *GLogWindow, TEXT("IsBrowser") );
		GLogWindow->Log( NAME_Title, LocalizeGeneral(TEXT("Exit"),GPackage) );
		delete GLogWindow;
		appPreExit();
		GIsGuarded = 0;
#ifndef _DEBUG
	}
	catch( ... )
	{
		// Crashed.
		ErrorLevel = 1;
		Error.HandleError();
	}
#endif

	// Final shut down.
	appExit();
	GIsStarted = 0;
	return ErrorLevel;
}


// Query the HW for caps.
#include "../../DirectX8/Inc/ddraw.h"
#include "../../DirectX8/Inc/d3d8.h"
static void SetInitialConfiguration()
{
	guard(SetInitialConfiguration);
	//!!vogel: currently only queries primary device though it will be used by default anyways.

	// Variables.
	HRESULT Result;
	INT		OnboardVideoMemory	= MAXINT;
	UBOOL	HardwareTL			= true,
			DeviceCapsValid		= false;
	FLOAT	CPUSpeed			= 0.000001 / GSecondsPerCycle;
	D3DADAPTER_IDENTIFIER8		DeviceIdentifier;
	D3DCAPS8					DeviceCaps8;

	// Get memory status.
	MEMORYSTATUS MemoryStatus; 
	MemoryStatus.dwLength = sizeof(MemoryStatus);
	GlobalMemoryStatus(&MemoryStatus);

	INT TotalMemory = MemoryStatus.dwTotalPhys / 1024 / 1024;
	debugf(NAME_Init, TEXT("Physical Memory: %i MByte"), TotalMemory);

	// Query D3D7 DirectDraw for available local video memory.
	guard(GetAvailableVidMem);
	LPDIRECTDRAW DirectDraw;

	appMemzero( &DeviceCaps8, sizeof(DeviceCaps8) );

	if( FAILED(Result=DirectDrawCreate( NULL, &DirectDraw, NULL ) ) )
		debugf(NAME_Init, TEXT("D3D Driver: Couldn't query amount of local video memory"));
	else 
	{
		LPDIRECTDRAW7 DirectDraw7;
		if( FAILED(Result=DirectDraw->QueryInterface(IID_IDirectDraw7, (LPVOID*) &DirectDraw7) ) ) 
			debugf(NAME_Init, TEXT("D3D Device: Couldn't query amount of local video memory"));
		else
		{			
			DDSCAPS2	ddsCaps2; 
			DWORD		dwTotal; 
			DWORD		dwFree;

			// See Tom Forsyth's post to DXDev
			// http://discuss.microsoft.com/SCRIPTS/WA-MSD.EXE?A2=ind0203a&L=directxdev&D=1&F=&S=&P=12849

			appMemzero( &ddsCaps2, sizeof(ddsCaps2) );
			ddsCaps2.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_LOCALVIDMEM | DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
			DirectDraw7->GetAvailableVidMem(&ddsCaps2, &dwTotal, &dwFree);

			if( dwTotal )
				OnboardVideoMemory = Min<DWORD>( OnboardVideoMemory, dwTotal );

			appMemzero( &ddsCaps2, sizeof(ddsCaps2) );
			ddsCaps2.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
			DirectDraw7->GetAvailableVidMem(&ddsCaps2, &dwTotal, &dwFree);

			if( dwTotal )
				OnboardVideoMemory = Min<DWORD>( OnboardVideoMemory, dwTotal );

			appMemzero( &ddsCaps2, sizeof(ddsCaps2) );
			ddsCaps2.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
			DirectDraw7->GetAvailableVidMem(&ddsCaps2, &dwTotal, &dwFree);

			if( dwTotal )
				OnboardVideoMemory = Min<DWORD>( OnboardVideoMemory, dwTotal );

			DirectDraw7->Release();
		}
		DirectDraw->Release();
	}
	unguard;

	// As GetAvailableVidMem is called before entering fullscreen the numbers are slightly less
	// as it reports "local video memory" - "memory used for desktop". Though this shouldn't matter 
	// that much as the user can always override it and as the default values should be conservative.
	if( OnboardVideoMemory != MAXINT )
	{
		OnboardVideoMemory = OnboardVideoMemory / 1024 / 1024;
		debugf( NAME_Init, TEXT("D3D Device: Video memory on board: %i"), OnboardVideoMemory );
	}

	// Query D3D8 device for SW vs HW T&L and DXT1.
	// Card specific defaults.
	guard(GetD3DDeviceCaps);
	IDirect3D8* Direct3D8 = Direct3DCreate8(D3D_SDK_VERSION);
	if( Direct3D8 )
	{
		if( SUCCEEDED(Result = Direct3D8->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &DeviceCaps8) ) )
		{
			DeviceCapsValid = true;
			if( (DeviceCaps8.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 )
				HardwareTL = 0;
			// We require at least 8 vertex streams.
			if( DeviceCaps8.MaxStreams < 8 )
				HardwareTL = 0;
		}
		else
			debugf(NAME_Init, TEXT("D3D Device: Couldn't query SW vs HW T&L"));

		// Lower detail settings if DXT3 isn't supported
		if(FAILED(Direct3D8->CheckDeviceFormat(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,0,D3DRTYPE_TEXTURE,D3DFMT_DXT3)))
			OnboardVideoMemory /= 2;

		if( FAILED( Direct3D8->GetAdapterIdentifier(D3DADAPTER_DEFAULT,D3DENUM_NO_WHQL_LEVEL,&DeviceIdentifier) ) )
			appMemzero( &DeviceIdentifier, sizeof(DeviceIdentifier) );

		Direct3D8->Release();
	}
	else
	{
		appErrorf(NAME_FriendlyError, TEXT("Please install DirectX 8.1b or later (see Release Notes for instructions on how to obtain it)"));
		debugf(NAME_Init, TEXT("D3D Device: Couldn't query SW vs HW T&L"));
		appMemzero( &DeviceIdentifier, sizeof(DeviceIdentifier) );
	}
	unguard;

	// Caps specific settings.
	if( DeviceCapsValid )
	{
		if( DeviceCaps8.MaxSimultaneousTextures < 4 )
		{
			GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("DetailTextures"		),	TEXT("False"		));		
		}
	}

	// Rough generic card defaults.
	if( OnboardVideoMemory < 8 )
	{
		// 8 MByte or less
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullScreenViewportX"	),	TEXT("320"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullScreenViewportY"	),	TEXT("240"			));

		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailInterface"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailTerrain"	),	TEXT("VeryLow"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWeaponSkin"),	TEXT("VeryLow"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("VeryLow"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWorld"		),	TEXT("Low"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailRenderMap"	),	TEXT("VeryLow"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailLightmap"	),	TEXT("VeryLow"		));

		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Decals"					),	TEXT("False"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Coronas"				),	TEXT("False"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Projectors"				),	TEXT("False"		));

		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("UseTrilinear"			),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("UseCubemaps"			),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("HighDetailActors"		),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("SuperHighDetailActors"),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("Use16bit"				),	TEXT("True"			));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("Use16bitTextures"		),	TEXT("True"			));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("DetailTextures"		),	TEXT("False"		));		

		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoFractalAnim"			),	TEXT("True"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoDynamicLights"		),	TEXT("True"			));
		GConfig->SetString( TEXT("UnrealGame.UnrealPawn"),TEXT("bPlayerShadows"			),	TEXT("False"		), GUserIni );
	}
	else if( OnboardVideoMemory <= 16 )
	{
		// 16 MByte
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullScreenViewportX"	),	TEXT("512"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullScreenViewportY"	),	TEXT("384"			));

		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailInterface"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailTerrain"	),	TEXT("Low"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWeaponSkin"),	TEXT("Low"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("Low"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWorld"		),	TEXT("VeryLow"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailRenderMap"	),	TEXT("Low"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailLightmap"	),	TEXT("Low"			));

		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Decals"					),	TEXT("False"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Coronas"				),	TEXT("False"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Projectors"				),	TEXT("False"		));

		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("UseTrilinear"			),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("UseCubemaps"			),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("HighDetailActors"		),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("SuperHighDetailActors"),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("Use16bit"				),	TEXT("True"			));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("Use16bitTextures"		),	TEXT("True"			));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("DetailTextures"		),	TEXT("False"		));		

		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoFractalAnim"			),	TEXT("True"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoDynamicLights"		),	TEXT("True"			));
		GConfig->SetString( TEXT("UnrealGame.UnrealPawn"),TEXT("bPlayerShadows"			),	TEXT("False"		), GUserIni );
	}
	else if( OnboardVideoMemory <= 32 )
	{
		// 32 MByte
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailInterface"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailTerrain"	),	TEXT("Lower"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWeaponSkin"),	TEXT("Lower"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("Low"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWorld"		),	TEXT("Low"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailRenderMap"	),	TEXT("Lower"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailLightmap"	),	TEXT("Lower"		));

		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));

		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("UseTrilinear"			),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("HighDetailActors"		),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("SuperHighDetailActors"),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("Use16bit"				),	TEXT("True"			));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("DetailTextures"		),	TEXT("False"		));
	}
	else if( OnboardVideoMemory <= 64 )
	{
		// 64 MByte
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailInterface"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailTerrain"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWeaponSkin"),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("Lower"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWorld"		),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailRenderMap"	),	TEXT("Lower"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailLightmap"	),	TEXT("Normal"		));

		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"),	TEXT("UseTrilinear"			),	TEXT("False"		));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("SuperHighDetailActors"),	TEXT("False"		));
	}
	else
#if !DEMOVERSION
	if( TotalMemory <= 128 )
#endif
	{
		// 128 MByte of RAM
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailInterface"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailTerrain"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWeaponSkin"),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWorld"		),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailRenderMap"	),	TEXT("Normal"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailLightmap"	),	TEXT("Normal"		));

		// Sorry - messy due to #if's
		if( TotalMemory <= 128 )
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("Lower"));
		else
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("Normal"));
	}
#if !DEMOVERSION
	else if( OnboardVideoMemory <= 128 )
	{
		// 128 MByte
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailInterface"	),	TEXT("Higher"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailTerrain"	),	TEXT("Higher"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWeaponSkin"),	TEXT("Higher"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("Higher"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWorld"		),	TEXT("Higher"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailRenderMap"	),	TEXT("Higher"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailLightmap"	),	TEXT("Higher"		));
	}
	else
	{
		// 256 MByte and more
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailInterface"	),	TEXT("UltraHigh"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailTerrain"	),	TEXT("UltraHigh"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWeaponSkin"),	TEXT("UltraHigh"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailPlayerSkin"),	TEXT("UltraHigh"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailWorld"		),	TEXT("UltraHigh"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailRenderMap"	),	TEXT("UltraHigh"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("TextureDetailLightmap"	),	TEXT("UltraHigh"	));

		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("UseCompressedLightmaps"	),	TEXT("False"	));
	}
#endif

	// Generic CPU defaults.
	if( CPUSpeed < 700 )
	{
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Coronas"		),	TEXT("False"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Decals"			),	TEXT("False"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"		),	TEXT("False"	));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoFractalAnim"	),	TEXT("True"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoDynamicLights"),	TEXT("True"			));

		GConfig->SetString( TEXT("UnrealGame.UnrealPawn"),TEXT("bPlayerShadows"	),	TEXT("False"	), GUserIni );

		GConfig->SetString( TEXT("Engine.LevelInfo")	, TEXT("PhysicsDetailLevel"		),	TEXT("PDL_Low"	));

		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("HighDetailActors"		),	TEXT("False"	));
		GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("SuperHighDetailActors"),	TEXT("False"	));
	}
	else if( CPUSpeed < 1000 )
	{
		if( !HardwareTL )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Coronas"		),	TEXT("False"	));
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoFractalAnim"	),	TEXT("True"		));

			GConfig->SetString( TEXT("UnrealGame.UnrealPawn"), TEXT("bPlayerShadows"	),	TEXT("False"	), GUserIni );

			GConfig->SetString( TEXT("Engine.LevelInfo")	, TEXT("PhysicsDetailLevel"		),	TEXT("PDL_Low"	));

			GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("HighDetailActors"		),	TEXT("False"	));
			GConfig->SetString( TEXT("D3DDrv.D3DRenderDevice"), TEXT("SuperHighDetailActors"),	TEXT("False"	));
		}
		else
			GConfig->SetString( TEXT("Engine.LevelInfo"), TEXT("PhysicsDetailLevel"		),	TEXT("PDL_Medium"	));
	}
	else if( CPUSpeed < 1500 )
	{
		if( !HardwareTL )
		{
			GConfig->SetString( TEXT("UnrealGame.UnrealPawn"), TEXT("bPlayerShadows"		),	TEXT("False"	), GUserIni );
			GConfig->SetString( TEXT("Engine.LevelInfo"), TEXT("PhysicsDetailLevel"		),	TEXT("PDL_Medium"	));
		}
	}

	UBOOL	LaunchNVIDIAInstaller	= 0,
			LaunchATIInstaller		= 0;

	// ATI
	if( DeviceIdentifier.VendorId==0x1002 )
	{
		switch( GRunningOS )
		{
		case OS_WIN98:
		case OS_WINME:
			if( (LOWORD(DeviceIdentifier.DriverVersion.LowPart) < 9062) && (LOWORD(DeviceIdentifier.DriverVersion.LowPart) > 3000) )
				LaunchATIInstaller |= 1;
			break;
		case OS_WINXP:
			if( LOWORD(DeviceIdentifier.DriverVersion.LowPart) < 6166 )
				LaunchATIInstaller |= 1;
			break;
		case OS_WIN2K:
			if( LOWORD(DeviceIdentifier.DriverVersion.LowPart) < 6166 )
				LaunchATIInstaller |= 1;
			break;
		}
	}
	// 3dfx
	else if( DeviceIdentifier.VendorId==0x121A )
	{
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullScreenViewportX"	),	TEXT("640"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("FullScreenViewportY"	),	TEXT("480"			));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Projectors"				),	TEXT("False"		));
		GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("NoDynamicLights"		),	TEXT("True"			));
	}
	// Intel
	else if( DeviceIdentifier.VendorId==0x8086 )
	{
	}
#if 0
	// Kyro
	else if( DeviceIdentifier.DeviceId==0x010 )
	{
	}
#endif
	// NVIDIA
	else if( DeviceIdentifier.VendorId==0x10DE )
	{
		if( LOWORD(DeviceIdentifier.DriverVersion.LowPart) < 3082 )
			LaunchNVIDIAInstaller |= 1;

		// TNT/ GF2/ GF4MX can't handle required stage setup for most projectors.
		// if( DeviceCaps8.MaxSimultaneousTextures == 2 )
		//	GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("Projectors"				),	TEXT("False"		));

		// TNT
		if( DeviceIdentifier.DeviceId == 0x0020 )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		}
		// TNT 2
		if( DeviceIdentifier.DeviceId >= 0x0028 && DeviceIdentifier.DeviceId <= 0x002F )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		}
		// GeForce
		if( DeviceIdentifier.DeviceId >= 0x0100 && DeviceIdentifier.DeviceId <= 0x0103 )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		}
		// GeForce 2 MX
		if( DeviceIdentifier.DeviceId >= 0x0110 && DeviceIdentifier.DeviceId <= 0x0113 )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		}
		// GeForce 2
		if( DeviceIdentifier.DeviceId >= 0x0150 && DeviceIdentifier.DeviceId <= 0x0153 )
		{
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		}
		// GeForce 4 MX
		if( DeviceIdentifier.DeviceId >= 0x0170 && DeviceIdentifier.DeviceId <= 0x0179 )
		{		
			GConfig->SetString( TEXT("WinDrv.WindowsClient"), TEXT("DecoLayers"				),	TEXT("False"		));
		}
		// GeForce 3
		if( DeviceIdentifier.DeviceId >= 0x0200 && DeviceIdentifier.DeviceId <= 0x0203 )
		{
		}
		// GeForce 4 Ti
		if( DeviceIdentifier.DeviceId >= 0x0250 && DeviceIdentifier.DeviceId <= 0x0253 )
		{
		}
	}
	// Matrox
	else if( DeviceIdentifier.VendorId==0x102B )
	{
	}
	// Trident
	else if( DeviceIdentifier.VendorId==0x1023 )
	{
	}
	// SiS
	else if( DeviceIdentifier.VendorId==0x1039 )
	{
	}
	// Generic
	else
	{
	}

	
	if( TotalMemory < 64 )
	{
		// 64 MByte
		GConfig->SetString( TEXT("Engine.GameEngine"		), TEXT("CacheSizeMegs"		),	TEXT("16"		));
		GConfig->SetString( TEXT("ALAudio.ALAudioSubsystem"	), TEXT("LowQualitySound"	),	TEXT("True"		));
		GConfig->SetString( TEXT("Engine.LevelInfo"			), TEXT("bLowSoundDetail"	),	TEXT("True"		));
	}
	else if( TotalMemory < 128 )
	{
		// 128 MByte
		GConfig->SetString( TEXT("Engine.GameEngine"		), TEXT("CacheSizeMegs"		),	TEXT("24"		));
		GConfig->SetString( TEXT("ALAudio.ALAudioSubsystem"	), TEXT("LowQualitySound"	),	TEXT("True"		));
		GConfig->SetString( TEXT("Engine.LevelInfo"			), TEXT("bLowSoundDetail"	),	TEXT("True"		));
	}
	else if( TotalMemory < 192 )
	{
		// 192 MByte
	}
	else if( TotalMemory < 256 )
	{
		// 256 MByte
	}

#if 0
	// Driver upgrade.
	if( GRunningOS == OS_WIN98 
	||	GRunningOS == OS_WINME
	||	GRunningOS == OS_WIN2K
	||	GRunningOS == OS_WINXP
	)
	{
		guard(DriverUpgrade);
		const TCHAR* Localized;
		if( LaunchATIInstaller || LaunchNVIDIAInstaller )
		{
			WDlgVideoDrivers dlg( NULL );
			if( dlg.DoModal() )
			{
				if( dlg.Selection == VD_UPGRADE_FROM_CD )
				{
					TCHAR InstallerPath[1024];
					FString CDPath;

					GConfig->GetString( TEXT("Engine.Engine"), TEXT("CDPath"), CDPath );
					appStrncpy( InstallerPath, *CDPath, 1024 );
					
					if( LaunchNVIDIAInstaller )
					{
						// NVIDIA
						switch( GRunningOS )
						{
						case OS_WIN98:
						case OS_WINME:
							appStrncat( InstallerPath, TEXT("\\Extras\\Drivers\\Win9x\\NVIDIA\\30.82_win9x.exe"), 1024 );
							break;
						case OS_WIN2K:
						case OS_WINXP:
							appStrncat( InstallerPath, TEXT("\\Extras\\Drivers\\Win2k\\NVIDIA\\30.82_winxp.exe"), 1024 );
							break;
						}
					}
					else
		{
						// ATI
						switch( GRunningOS )
						{
						case OS_WIN98:
						case OS_WINME:
							appStrncat( InstallerPath, TEXT("\\Extras\\Drivers\\Win9x\\ATI\\wme-radeon4-13-01-9062.exe"), 1024 );
							break;
						case OS_WIN2K:
							appStrncat( InstallerPath, TEXT("\\Extras\\Drivers\\Win2k\\ATI\\w2k-radeon-5-13-01-6166-efg.exe"), 1024 );
							break;
						case OS_WINXP:
							appStrncat( InstallerPath, TEXT("\\Extras\\Drivers\\Win2k\\ATI\\wxp-radeon-6-13-01-6166-efg.exe"), 1024 );
							break;
		}
	}

					UBOOL Installed = 0;
					Localized = Localize(TEXT("UpgradeDrivers"), TEXT("InsertCD"), TEXT("Engine"), NULL );
					do
	{
						if( GFileManager->FileSize(InstallerPath) > 0 )
		{
							// Launch installer and exit.
							Installed = 1;
							appLaunchURL( InstallerPath, NULL, NULL );
							GIsRequestingExit = 1;
						}
						else
							Sleep( 1000 );
		}
					while( !Installed && appMsgf( 2, TEXT("%s"), Localized ) );
	}
				else if( dlg.Selection == VD_UPGRADE_FROM_WEB )
				{
					// Launch browser and exit.
					if( LaunchNVIDIAInstaller )
	{
						// NVIDIA
						Localized = Localize(TEXT("UpgradeDrivers"), TEXT("NvidiaURL"), TEXT("Engine"), NULL );
						appLaunchURL( Localized, NULL, NULL );
						GIsRequestingExit = 1;
					}
					else
		{
						// ATI
						Localized = Localize(TEXT("UpgradeDrivers"), TEXT("AtiURL"), TEXT("Engine"), NULL );
						appLaunchURL( Localized, NULL, NULL );
						GIsRequestingExit = 1;
					}
		}
	}
		}
		unguard;
	}
#endif

	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
