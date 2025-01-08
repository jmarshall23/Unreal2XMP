/*=============================================================================
	UnVcWin32.cpp: Visual C++ Windows 32-bit core.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/
#if _MSC_VER

// Windows and runtime includes.
#pragma warning( disable : 4201 )

#include <windows.h>
#include <stdio.h>
#include <float.h>
#include <time.h>
#include <io.h>
#include <direct.h>
#include <errno.h>
#include <sys/stat.h>

// Core includes.
#include "CorePrivate.h"

#include <string.h>

/*-----------------------------------------------------------------------------
	Unicode helpers.
-----------------------------------------------------------------------------*/

#if UNICODE

// Windows versions ask Windows to convert UNICODE to ANSI using the current encoding.

CORE_API ANSICHAR* winToANSI( ANSICHAR* ACh, const TCHAR* InUCh, INT Count )
{
	guardSlow(winToANSI);
	WideCharToMultiByte( CP_ACP, 0, InUCh, -1, ACh, Count, NULL, NULL );
	return ACh;
	unguardSlow;
}
CORE_API ANSICHAR* winToOEM( ANSICHAR* ACh, const TCHAR* InUCh, INT Count )
{
	guardSlow(winToOEM);
	WideCharToMultiByte( CP_OEMCP, 0, InUCh, -1, ACh, Count, NULL, NULL );
	return ACh;
	unguardSlow;
}
CORE_API INT winGetSizeANSI( const TCHAR* InUCh )
{
	guardSlow(winGetSizeANSI);
	return WideCharToMultiByte( CP_ACP, 0, InUCh, -1, NULL, 0, NULL, NULL );
	unguardSlow;
}
CORE_API TCHAR* winToUNICODE( TCHAR* UCh, const ANSICHAR* InACh, INT Count )
{
	MultiByteToWideChar( CP_ACP, 0, InACh, -1, UCh, Count );
	return UCh;
}
CORE_API INT winGetSizeUNICODE( const ANSICHAR* InACh )
{
	return MultiByteToWideChar( CP_ACP, 0, InACh, -1, NULL, 0 );
}

CORE_API UNICHAR* winAnsiToTCHAR(char* str)
{
	INT iLength = winGetSizeUNICODE(str);
	UNICHAR* pBuffer = new TCHAR[iLength];
	appStrcpy(pBuffer,TEXT(""));
	return winToUNICODE(pBuffer,str,iLength);
}

#endif	// UNICODE

/*-----------------------------------------------------------------------------
	FOutputDeviceWindowsError.
-----------------------------------------------------------------------------*/

//
// Immediate exit.
//
CORE_API void appRequestExit( UBOOL Force )
{
	guard(appForceExit);
	debugf( TEXT("appRequestExit(%i)"), Force );
	if( Force )
	{
		// Force immediate exit. Dangerous because config code isn't flushed, etc.
		ExitProcess( 1 );
	}
	else
	{
		// Tell the platform specific code we want to exit cleanly from the main loop.
		PostQuitMessage( 0 );
		GIsRequestingExit = 1;
	}
	unguard;
}

//
// Get system error.
//
CORE_API const TCHAR* appGetSystemErrorMessage( INT Error )
{
	guard(appGetSystemErrorMessage);
	static TCHAR Msg[1024];
	*Msg = 0;
	if( Error==0 )
		Error = GetLastError();
#if UNICODE
	if( !GUnicodeOS )
	{
		ANSICHAR ACh[1024];
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ACh, 1024, NULL );
		appStrcpy( Msg, appFromAnsi(ACh) );
	}
	else
#endif
	{
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), Msg, 1024, NULL );
	}
	if( appStrchr(Msg,'\r') )
		*appStrchr(Msg,'\r')=0;
	if( appStrchr(Msg,'\n') )
		*appStrchr(Msg,'\n')=0;
	return Msg;
	unguard;
}

/*-----------------------------------------------------------------------------
	USystem.
-----------------------------------------------------------------------------*/

int CDECL StringCompare(const void *A, const void *B)
{
	return appStricmp( **(*(FString**)A), **(*(FString**)B) );
}

//
// System manager.
//
#pragma warning (push)
#pragma warning (disable : 4717)
static void Recurse(UBOOL B)
{
	guard(Recurse);

	if(B)
		Recurse(B);

	unguard;
}
#pragma warning (pop)
USystem::USystem()
:	SavePath	( E_NoInit )
,	CachePath	( E_NoInit )
,	CacheExt	( E_NoInit )
,	Paths		( E_NoInit )
,	Suppress	( E_NoInit )
,   LicenseeMode (1)
{}
void USystem::StaticConstructor()
{
	guard(USystem::StaticConstructor);

	new(GetClass(),TEXT("PurgeCacheDays"),      RF_Public)UIntProperty   (CPP_PROPERTY(PurgeCacheDays    ), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("SavePath"),            RF_Public)UStrProperty   (CPP_PROPERTY(SavePath          ), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("CachePath"),           RF_Public)UStrProperty   (CPP_PROPERTY(CachePath         ), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("CacheExt"),            RF_Public)UStrProperty   (CPP_PROPERTY(CacheExt          ), TEXT("Options"), CPF_Config );

	UArrayProperty* A = new(GetClass(),TEXT("Paths"),RF_Public)UArrayProperty( CPP_PROPERTY(Paths), TEXT("Options"), CPF_Config );
	A->Inner = new(A,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* B = new(GetClass(),TEXT("Suppress"),RF_Public)UArrayProperty( CPP_PROPERTY(Suppress), TEXT("Options"), CPF_Config );
	B->Inner = new(B,TEXT("NameProperty0"),RF_Public)UNameProperty;

	unguard;
}
UBOOL USystem::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( ParseCommand(&Cmd,TEXT("MEMSTAT2")) )
	{
		MEMORYSTATUS B; B.dwLength = sizeof(B);
		GlobalMemoryStatus(&B);
		Ar.Logf( TEXT("Memory: %3.1f %3.1f %3.1f"), 
			B.dwAvailPhys/1024.f/1024.f, 
			(B.dwTotalPageFile-B.dwAvailPageFile)/1024.f/1024.f,
			(B.dwTotalVirtual-B.dwAvailVirtual)/1024.f/1024.f );
		return 1;
	}	
	else if( ParseCommand(&Cmd,TEXT("MEMSTAT")) )
	{
		MEMORYSTATUS B; B.dwLength = sizeof(B);
		GlobalMemoryStatus(&B);
		Ar.Logf( TEXT("Memory: PhysAvail=%iK/%iK PageFileUsed=%iK/%iK VirtUsed=%iK/%iK"), 
			B.dwAvailPhys/1024, B.dwTotalPhys/1024, 
			(B.dwTotalPageFile-B.dwAvailPageFile)/1024, B.dwTotalPageFile/1024, 
			(B.dwTotalVirtual-B.dwAvailVirtual)/1024, B.dwTotalVirtual/1024 );
		Ar.Logf( TEXT("Memory load = %i%%"), B.dwMemoryLoad );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("CONFIGHASH")) )
	{
		GConfig->Dump( Ar );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("EXIT")) || ParseCommand(&Cmd,TEXT("QUIT")))
	{
		// Ignore these commands when running the editor
		if( !GIsEditor )
		{
			Ar.Log( TEXT("Closing by request") );
			appRequestExit( 0 );
		}
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("RELAUNCH") ) )
	{
		debugf( TEXT("Relaunch: %s"), Cmd );
		GConfig->Flush( 0 );
#if UNICODE
		if( !GUnicodeOS )
		{
			ANSICHAR ThisFile[256];
			GetModuleFileNameA( NULL, ThisFile, ARRAY_COUNT(ThisFile) );
			debugf( TEXT("GetModuleFileName returned %s"), appFromAnsi(ThisFile) );
			ShellExecuteA( NULL, "open", ThisFile, TCHAR_TO_ANSI(Cmd), TCHAR_TO_ANSI(appBaseDir()), SW_SHOWNORMAL );
		}
		else
#endif
		{
			TCHAR ThisFile[256];
			GetModuleFileName( NULL, ThisFile, ARRAY_COUNT(ThisFile) );
			debugf( TEXT("GetModuleFileName returned %s"), ThisFile );
			ShellExecute( NULL, TEXT("open"), ThisFile, Cmd, appBaseDir(), SW_SHOWNORMAL );
		}
		appRequestExit( 0 );
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("DEBUG") ) )
	{
		if( ParseCommand(&Cmd,TEXT("CRASH")) )
		{
			appErrorf( TEXT("%s"), TEXT("Unreal crashed at your request") );
			return 1;
		}
		else if( ParseCommand( &Cmd, TEXT("GPF") ) )
		{
			Ar.Log( TEXT("Unreal crashing with voluntary GPF") );
			*(int *)NULL = 123;
			return 1;
		}
		else if( ParseCommand( &Cmd, TEXT("RECURSE") ) )
		{
			Ar.Logf( TEXT("Recursing") );
			Recurse(1);
			return 1;
		}
		else if( ParseCommand( &Cmd, TEXT("EATMEM") ) )
		{
			Ar.Log( TEXT("Eating up all available memory") );
			while( 1 )
			{
				void* Eat = appMalloc(65536,TEXT("EatMem"));
				memset( Eat, 0, 65536 );
			}
			return 1;
		}
		else return 0;
	}
	else if( ParseCommand(&Cmd,TEXT("DIR")) )		// DIR [path\pattern]
	{
		TArray<FString> Files = GFileManager->FindFiles( Cmd, 1, 0 );
		TArray<FString> Directories = GFileManager->FindFiles( Cmd, 0, 1 );

		FString* Buffer[512];
		INT Count;

		//
		// DIRECTORIES
		//

		// Sort
		Count = Directories.Num();
		INT x;
		for( x = 0 ; x < Directories.Num() ; ++x )
			Buffer[x] = &Directories(x);
		appQsort( &Buffer[0], Count, sizeof(FString*), StringCompare );
		// Display
		for( INT x = 0 ; x < Count ; x++ )
			Ar.Logf( TEXT("[%s]"), *Buffer[x] );

		//
		// FILES
		//

		// Sort
		Count = Files.Num();
		for( INT x = 0 ; x < Files.Num() ; ++x )
			Buffer[x] = &Files(x);
		appQsort( &Buffer[0], Count, sizeof(FString*), StringCompare );
		// Display
		for( INT x = 0 ; x < Count ; x++ )
			Ar.Logf( TEXT("%s"), *Buffer[x] );
		return 1;
	}
	else return 0;
}
IMPLEMENT_CLASS(USystem);

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

//
// Copy text to clipboard.
//
CORE_API void appClipboardCopy( const TCHAR* Str )
{
	guard(appClipboardCopy);
	if( OpenClipboard(GetActiveWindow()) )
	{
		verify(EmptyClipboard());
#if UNICODE
		HGLOBAL GlobalMem;
		if( GUnicode && !GUnicodeOS )
		{
			INT Count = WideCharToMultiByte(CP_ACP,0,Str,-1,NULL,0,NULL,NULL);
			GlobalMem = GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, Count+1 );
			check(GlobalMem);
			ANSICHAR* Data = (ANSICHAR*) GlobalLock( GlobalMem );
			WideCharToMultiByte(CP_ACP,0,Str,-1,Data,Count,NULL,NULL);
			Data[Count] = 0;
			GlobalUnlock( GlobalMem );
			if( SetClipboardData( CF_TEXT, GlobalMem ) == NULL )
				appErrorf(TEXT("SetClipboardData(A) failed with error code %i"), GetLastError() );
		}
		else
#endif
		{
			GlobalMem = GlobalAlloc( GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(TCHAR)*(appStrlen(Str)+1) );
			check(GlobalMem);
			TCHAR* Data = (TCHAR*) GlobalLock( GlobalMem );
			appStrcpy( Data, Str );
			GlobalUnlock( GlobalMem );
			if( SetClipboardData( GUnicode ? CF_UNICODETEXT : CF_TEXT, GlobalMem ) == NULL )
				appErrorf(TEXT("SetClipboardData(%s) failed with error code %i"), GUnicode ? TEXT("W") : TEXT("A"), GetLastError() );
		}
		verify(CloseClipboard());
	}
	unguard;
}

//
// Paste text from clipboard.
//
CORE_API FString appClipboardPaste()
{
	guard(appClipboardPaste);
	FString Result;
	if( OpenClipboard(GetActiveWindow()) )
	{
		HGLOBAL GlobalMem = NULL;
		UBOOL Unicode = 0;
		if( GUnicode && GUnicodeOS )
		{
			GlobalMem = GetClipboardData( CF_UNICODETEXT );
			Unicode = 1;
		}
		if( !GlobalMem )
		{
			GlobalMem = GetClipboardData( CF_TEXT );
			Unicode = 0;
		}
		if( !GlobalMem )
			Result = TEXT("");
		else
		{
			void* Data = GlobalLock( GlobalMem );
			check( Data );	
			if( Unicode )
				Result = (TCHAR*) Data;
			else
			{
				ANSICHAR* ACh = (ANSICHAR*) Data;
				INT i;
				for( i=0; ACh[i]; i++ );
				TArray<TCHAR> Ch(i+1);
				for( i=0; i<Ch.Num(); i++ )
					Ch(i)=FromAnsi(ACh[i]);
				Result = &Ch(0);
			}
			GlobalUnlock( GlobalMem );
		}
		verify(CloseClipboard());
	}
	else Result=TEXT("");
	return Result;
	unguard;
}

/*-----------------------------------------------------------------------------
	DLLs.
-----------------------------------------------------------------------------*/

//
// Load a library.
//warning: Intended only to be called by UPackage::BindPackage. Don't call directly.
//
static void* GetDllHandleHelper( const TCHAR* Filename )
{

	return TCHAR_CALL_OS(LoadLibraryW(Filename),LoadLibraryA(TCHAR_TO_ANSI(Filename)));
}

#pragma warning(disable:4700)
CORE_API void* appGetDllHandle( const TCHAR* Filename )
{
	guard(appGetDllHandle);
	check(Filename);

	void* Result = GetDllHandleHelper( Filename );
	if( !Result )
		Result = GetDllHandleHelper( *(FString(Filename) + DLLEXT) );
	if( !Result )
		Result = GetDllHandleHelper( *(US + TEXT("_") + Filename + DLLEXT) );

	return Result;
	unguard;
}
#pragma warning(default:4700)

//
// Free a DLL.
//
CORE_API void appFreeDllHandle( void* DllHandle )
{
	guard(appFreeDllHandle);
	check(DllHandle);

	FreeLibrary( (HMODULE)DllHandle );

	unguard;
}

//
// Lookup the address of a DLL function.
//
CORE_API void* appGetDllExport( void* DllHandle, const TCHAR* ProcName )
{
	guard(appGetDllExport);
	check(DllHandle);
	check(ProcName);

	return (void*)GetProcAddress( (HMODULE)DllHandle, TCHAR_TO_ANSI(ProcName) );

	unguard;
}

CORE_API const void appDebugMessagef( const TCHAR* Fmt, ... )
{
	TCHAR TempStr[4096]=TEXT("");
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt, Fmt );
	guard(appDebugMessagef);

	MessageBox(NULL,TempStr,TEXT("appDebugMessagef"),MB_OK);

	unguard;
}

// Type - dicrates the type of dialog we're displaying
CORE_API const UBOOL appMsgf( INT Type, const TCHAR* Fmt, ... )
{
	guard(appMsgf);
	TCHAR TempStr[4096]=TEXT("");
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt, Fmt );
	switch( Type )
	{
		case 1:
			return MessageBox( NULL, TempStr, TEXT("Message"), MB_YESNO ) == IDYES;
			break;
		case 2:
			return MessageBox( NULL, TempStr, TEXT("Message"), MB_OKCANCEL ) == IDOK;
			break;
		default:
			MessageBox( NULL, TempStr, TEXT("Message"), MB_OK );
			break;
	}
	return 1;
	unguard;
}

CORE_API const void appGetLastError( void )
{
	guard(appGetLastError);
	TCHAR TempStr[4096]=TEXT("");
	appSprintf( TempStr, TEXT("GetLastError : %d\n\n%s"),
		GetLastError(), appGetSystemErrorMessage() );

	MessageBox( NULL, TempStr, TEXT("System Error"), MB_OK );

	unguard;
}

// Interface for recording loading errors in the editor
CORE_API const void EdClearLoadErrors()
{
	GEdLoadErrors.Empty();
}

CORE_API const void EdLoadErrorf( INT Type, const TCHAR* Fmt, ... )
{
	TCHAR TempStr[4096]=TEXT("");
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt, Fmt );

	// Check to see if this error already exists ... if so, don't add it.
	// NOTE : for some reason, I can't use AddUniqueItem here or it crashes
	for( INT x = 0 ; x < GEdLoadErrors.Num() ; ++x )
		if( GEdLoadErrors(x) == FEdLoadError( Type, TempStr ) )
			return;

	new( GEdLoadErrors )FEdLoadError( Type, TempStr );
}

#if 1 //NEW (mdf) sound
CORE_API const void appSound()
{
	guard(appSound);
	MessageBeep( MB_OK );
	unguard;
}
#endif

//
// Break the debugger.
//
void appDebugBreak()
{
	guard(appDebugBreak);
	::DebugBreak();
	unguard;
}

//
// appIsDebuggerPresent - are we running under a debugger?
//
UBOOL appIsDebuggerPresent()
{
	UBOOL Result = 0;
    OSVERSIONINFO WinVersion;
    WinVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    // IsDebuggerPresent doesn't exist under 95, so we will only test for it under NT+
    if( GetVersionEx(&WinVersion) && WinVersion.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        HINSTANCE KernLib =  LoadLibraryEx( TEXT("kernel32.dll"), NULL, 0);
        if( KernLib )
        {
            FARPROC lIsDebuggerPresent = GetProcAddress( KernLib, "IsDebuggerPresent" );
            if( lIsDebuggerPresent && lIsDebuggerPresent() )
                Result = 1;

            FreeLibrary( KernLib );
        }
    }
	return Result;
}



/*-----------------------------------------------------------------------------
	Timing.
-----------------------------------------------------------------------------*/

//
// Get time in seconds. Origin is arbitrary.
//
#if !DEFINED_appSeconds
CORE_API DOUBLE appSeconds()
{
	static DWORD  InitialTime = timeGetTime();
	static DOUBLE TimeCounter = 0.0;

	// Accumulate difference to prevent wraparound.
	DWORD NewTime = timeGetTime();
	TimeCounter += (NewTime - InitialTime) * (1./1000.);
	InitialTime = NewTime;

	return TimeCounter;
}
#endif

//
// Return number of CPU cycles passed. Origin is arbitrary.
//
#if !DEFINED_appCycles
CORE_API DWORD appCycles()
{
	return appSeconds();
}
#endif

//
// Sleep this thread for Seconds, 0.0 means release the current
// timeslice to let other threads get some attention.
//
CORE_API void appSleep( FLOAT Seconds )
{
	guard(appSleep);
	Sleep( (DWORD)(Seconds * 1000.0) );
	unguard;
}

//
// Return the system time.
//
CORE_API void appSystemTime( INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec )
{
	guard(appSystemTime);

	SYSTEMTIME st;
	GetLocalTime( &st );

	Year		= st.wYear;
	Month		= st.wMonth;
	DayOfWeek	= st.wDayOfWeek;
	Day			= st.wDay;
	Hour		= st.wHour;
	Min			= st.wMinute;
	Sec			= st.wSecond;
	MSec		= st.wMilliseconds;

	unguard;
}

//
//	Return the system time as a 64-bit number.
//

CORE_API SQWORD appSystemTime64()
{
	guard(appSystemTime64);

	SYSTEMTIME	SystemTime;
	FILETIME	FileTime;

	GetLocalTime(&SystemTime);
	verify(SystemTimeToFileTime(&SystemTime,&FileTime));

	return *(SQWORD*)&FileTime;

	unguard;
}

//
// Return seconds counter.
//
CORE_API DOUBLE appSecondsSlow()
{
	static INT LastTickCount=0;
	static DOUBLE TickCount=0.0;
	INT ThisTickCount = GetTickCount();
	TickCount += (ThisTickCount-LastTickCount) / 1000.0;
	LastTickCount = ThisTickCount;
	return TickCount;
}

/*-----------------------------------------------------------------------------
	Link functions.
-----------------------------------------------------------------------------*/

//
// Launch a uniform resource locator (i.e. http://www.epicgames.com/unreal).
// This is expected to return immediately as the URL is launched by another
// task.
//
void appLaunchURL( const TCHAR* URL, const TCHAR* Parms, FString* Error )
{
	guard(appLaunchURL);
	 debugf( NAME_Log, appStrlen( URL ) > 256 ? TEXT("LaunchURL %253.253s...%s") : TEXT("LaunchURL %s %s"), URL, Parms?Parms:TEXT("") );
	INT Code = (INT)TCHAR_CALL_OS(ShellExecuteW(NULL,TEXT("open"),URL,Parms?Parms:TEXT(""),TEXT(""),SW_SHOWNORMAL),ShellExecuteA(NULL,"open",TCHAR_TO_ANSI(URL),Parms?TCHAR_TO_ANSI(Parms):"","",SW_SHOWNORMAL));
	if( Error )
		*Error = Code<=32 ? LocalizeError(TEXT("UrlFailed"),TEXT("Core")) : TEXT("");
	unguard;
}

void *appCreateProc( const TCHAR* URL, const TCHAR* Parms )
{
	guard(appCreateProc);
	debugf( NAME_Log, TEXT("CreateProc %s %s"), URL, Parms );

	TCHAR CommandLine[1024];
	appSprintf( CommandLine, TEXT("%s %s"), URL, Parms );

	PROCESS_INFORMATION ProcInfo;
	SECURITY_ATTRIBUTES Attr;
	Attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	Attr.lpSecurityDescriptor = NULL;
	Attr.bInheritHandle = TRUE;

#if UNICODE
	if( GUnicode && !GUnicodeOS )
	{
		STARTUPINFOA StartupInfoA = { sizeof(STARTUPINFO), NULL, NULL, NULL,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, NULL, NULL, SW_HIDE, NULL, NULL,
			NULL, NULL, NULL };
		if( !CreateProcessA( NULL, TCHAR_TO_ANSI(CommandLine), &Attr, &Attr, TRUE, DETACHED_PROCESS | REALTIME_PRIORITY_CLASS,
			NULL, NULL, &StartupInfoA, &ProcInfo ) )
			return NULL;
	}
	else
#endif
	{
		STARTUPINFO StartupInfo = { sizeof(STARTUPINFO), NULL, NULL, NULL,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, NULL, NULL, SW_HIDE, NULL, NULL,
			NULL, NULL, NULL };
		if( !CreateProcess( NULL, CommandLine, &Attr, &Attr, TRUE, DETACHED_PROCESS | REALTIME_PRIORITY_CLASS,
			NULL, NULL, &StartupInfo, &ProcInfo ) )
			return NULL;
	}
	return (void*)ProcInfo.hProcess;
	unguard;
}

UBOOL appGetProcReturnCode( void* ProcHandle, INT* ReturnCode )
{
	guard(appGetProcReturnCode);
	return GetExitCodeProcess( (HANDLE)ProcHandle, (DWORD*)ReturnCode ) && *((DWORD*)ReturnCode) != STILL_ACTIVE;
	unguard;
}

/*-----------------------------------------------------------------------------
	File finding.
-----------------------------------------------------------------------------*/

//
// Clean out the file cache.
//
static INT GetFileAgeDays( const TCHAR* Filename )
{
	guard(GetFileAgeDays);
	struct _stat Buf;
	INT Result = 0;
#if UNICODE
	if( GUnicodeOS )
	{
		Result = _wstat(Filename,&Buf);
	}
	else
#endif
	{
		Result = _stat(TCHAR_TO_ANSI(Filename),&Buf);
	}
	if( Result==0 )
	{
		time_t CurrentTime, FileTime;
		FileTime = Buf.st_mtime;
		time( &CurrentTime );
		DOUBLE DiffSeconds = difftime( CurrentTime, FileTime );
		return DiffSeconds / 60.0 / 60.0 / 24.0;
	}
	return 0;
	unguard;
}

CORE_API void appCleanFileCache()
{
	guard(appCleanFileCache);

	// Delete all temporary files.
	guard(DeleteTemps);
	FString Temp = FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("*.tmp"), *GSys->CachePath );
	TArray<FString> Found = GFileManager->FindFiles( *Temp, 1, 0 );
	for( INT i=0; i<Found.Num(); i++ )
	{
		Temp = FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s"), *GSys->CachePath, *Found(i) );
		debugf( TEXT("Deleting temporary file: %s"), *Temp );
		GFileManager->Delete( *Temp );
	}
	unguard;

	// Delete cache files that are no longer wanted.
	guard(DeleteExpired);
	TArray<FString> Found = GFileManager->FindFiles( *(GSys->CachePath * TEXT("*") + GSys->CacheExt), 1, 0 );
	if( GSys->PurgeCacheDays )
	{
		for( INT i=0; i<Found.Num(); i++ )
		{
			FString Temp = FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s"), *GSys->CachePath, *Found(i) );
			INT DiffDays = GetFileAgeDays( *Temp );
			if( DiffDays > GSys->PurgeCacheDays )
			{
				debugf( TEXT("Purging outdated file from cache: %s (%i days old)"), *Temp, DiffDays );
				GFileManager->Delete( *Temp );
			}
		}
	}
	unguard;

	unguard;
}

/*-----------------------------------------------------------------------------
	File Streaming.
-----------------------------------------------------------------------------*/
#pragma pack (push,8)
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#pragma pack (pop)


FCriticalSection::FCriticalSection()
{
	guard(FCriticalSection::FCriticalSection);
	Handle = new CRITICAL_SECTION;
	InitializeCriticalSection( (CRITICAL_SECTION*) Handle );
	unguard;
}

FCriticalSection::~FCriticalSection()
{
	guard(FCriticalSection::~FCriticalSection);
	DeleteCriticalSection( (CRITICAL_SECTION*) Handle );
	delete Handle;
	unguard;
}

void FCriticalSection::Lock()
{
	EnterCriticalSection( (CRITICAL_SECTION*) Handle );
}

void FCriticalSection::Unlock()
{
	LeaveCriticalSection( (CRITICAL_SECTION*) Handle );
}

FScopeCriticalSection::FScopeCriticalSection( FCriticalSection* InCriticalSection )
{
	CriticalSection = InCriticalSection;
	CriticalSection->Lock();
}

FScopeCriticalSection::~FScopeCriticalSection()
{
	CriticalSection->Unlock();
}


static DWORD FileStreamingThreadId = 0;

static DWORD STDCALL FileStreamingThread( void* Arg )
{
	while (GFileStream->Destroyed == 0)
	{
		for (INT i=0; i<GFileStream->MaxStreams; i++)
		{	
			FScopeCriticalSection Lock( GFileStream->CriticalSection );

			if (GFileStream->Streams[i].Used && GFileStream->Streams[i].ChunksRequested )
			{
				GFileStream->Read( i, GFileStream->Streams[i].ChunkSize );
				GFileStream->Streams[i].ChunksRequested--;
				GFileStream->Streams[i].Data = ((BYTE*) GFileStream->Streams[i].Data) + GFileStream->Streams[i].ChunkSize;
			}
		}
		Sleep(1);
	}
	for (INT i=0; i<GFileStream->MaxStreams; i++)
	{
		FScopeCriticalSection Lock( GFileStream->CriticalSection );

		if (GFileStream->Streams[i].Handle)
			GFileStream->Destroy(i);
	}
	GFileStream->Destroyed = 2;
	ExitThread( 0 );
	return 0;
}

UBOOL FFileStream::Read( INT StreamId, INT Bytes )
{
	if ( Streams[StreamId].Handle && Streams[StreamId].Data )
	{
		switch ( Streams[StreamId].Type )
		{
			case ST_Regular:	
			{
				INT Count=0;
				UBOOL RetVal = ReadFile( Streams[StreamId].Handle, Streams[StreamId].Data, Bytes, (DWORD*) &Count, NULL );
				if (RetVal)
					Streams[StreamId].Data = (BYTE*)(Streams[StreamId].Data) + Count;
				if (Count != Bytes)
					Streams[StreamId].EndOfFile = 1;
				return RetVal;
			}
			case ST_Ogg:
			case ST_OggLooping:
			{
				long Count = 0;
				while ( Count < Bytes )
				{
					long Read = ov_read( 
						(OggVorbis_File*) Streams[StreamId].TDD, 
						((char*) Streams[StreamId].Data) + Count, 
						Bytes - Count, 
						0,	//!! vogel: TODO: should be 1 for big endian machines
						2,
						1, 
						&Streams[StreamId].FileSeek
					);
					if ( Read == 0 )
					{
						if( Streams[StreamId].Type == ST_OggLooping )
						{
							//Rewind our stream
							ov_time_seek( (OggVorbis_File*) Streams[StreamId].TDD, 0.0 );
						}
						else
						{
							// Filling the rest of the buffer with silence. Note that memset is used for reentrance reasons.
							memset( ((char*) Streams[StreamId].Data) + Count, 0 , Bytes - Count );
							Streams[StreamId].EndOfFile = 1;
							return false;
						}
					}
					else if ( Read < 0 )
					{
						// Filling the rest of the buffer with silence. Note that memset is used for reentrance reasons.
						memset( ((char*) Streams[StreamId].Data) + Count, 0 , Bytes - Count );
						return false;
					}
					else
					{
						Count += Read;
					}
				}
				return true;
			}
			default:
					return false;
		}
	}
	else
	{
		return false;
	}
}
	
UBOOL FFileStream::Create( INT StreamId, const TCHAR* Filename )
{
	switch ( Streams[StreamId].Type )
	{
		case ST_Regular:
		{
			DWORD  Access    = GENERIC_READ;
			DWORD  WinFlags  = FILE_SHARE_READ;
			DWORD  Create    = OPEN_EXISTING;

			HANDLE Handle    = TCHAR_CALL_OS(
				CreateFileW( Filename, Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL ), 
				CreateFileA( TCHAR_TO_ANSI(Filename), Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL ) 
			);

			if( Handle == INVALID_HANDLE_VALUE )
			{
				Streams[StreamId].Handle = NULL;
				return false;
			}
			else
			{
				Streams[StreamId].Handle = Handle;
					return true;
			}
		}
		case ST_Ogg:
		case ST_OggLooping:
		{
			void* Handle = TCHAR_CALL_OS( _wfopen(Filename,TEXT("rb")), fopen(TCHAR_TO_ANSI(Filename),"rb") );
			if ( Handle == NULL ) 
				return false;
			INT Error = ov_open( (FILE*) Handle, (OggVorbis_File*) Streams[StreamId].TDD, NULL, 0 );
			if (Error < 0)
				return false;
			else
			{
				Streams[StreamId].Handle = Handle;
				return true;
			}
		}
		default:
			return false;
	}
}
	
UBOOL FFileStream::Destroy( INT StreamId )
{
	switch ( Streams[StreamId].Type )
	{
		case ST_Regular:
		{	
			if ( Streams[StreamId].Handle )
				CloseHandle( Streams[StreamId].Handle );
			Streams[StreamId].TDD		= NULL;
			Streams[StreamId].Handle	= NULL;
			Streams[StreamId].Used		= 0;
			return true;
		}
		case ST_Ogg:
		case ST_OggLooping:
		{
			if ( Streams[StreamId].TDD )
				ov_clear( (OggVorbis_File*) Streams[StreamId].TDD );
			Streams[StreamId].TDD		= NULL;
			Streams[StreamId].Handle	= NULL;
			Streams[StreamId].Used		= 0;
			return true;
		}
		default:
			return false;
	}
}

/*-----------------------------------------------------------------------------
	Guids.
-----------------------------------------------------------------------------*/

//
// Create a new globally unique identifier.
//
CORE_API FGuid appCreateGuid()
{
	guard(appCreateGuid);

	FGuid Result(0,0,0,0);
	verify( CoCreateGuid( (GUID*)&Result )==S_OK );
	return Result;

	unguard;
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

// Get startup directory.
CORE_API const TCHAR* appBaseDir()
{
	guard(appBaseDir);
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		// Get directory this executable was launched from.
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			ANSICHAR ACh[256];
			GetModuleFileNameA( hInstance, ACh, ARRAY_COUNT(ACh) );
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, Result, ARRAY_COUNT(Result) );
		}
		else
#endif
		{
			GetModuleFileName( hInstance, Result, ARRAY_COUNT(Result) );
		}
		INT i;
		for( i=appStrlen(Result)-1; i>0; i-- )
			if( Result[i-1]==PATH_SEPARATOR[0] || Result[i-1]=='/' )
				break;
		Result[i]=0;
	}
	return Result;
	unguard;
}

// Get computer name.
CORE_API const TCHAR* appComputerName()
{
	guard(appComputerName);
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		DWORD Size=ARRAY_COUNT(Result);
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			ANSICHAR ACh[ARRAY_COUNT(Result)];
			GetComputerNameA( ACh, &Size );
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, Result, ARRAY_COUNT(Result) );
		}
		else
#endif
		{
			GetComputerName( Result, &Size );
		}
		TCHAR *c, *d;
		for( c=Result, d=Result; *c!=0; c++ )
			if( appIsAlnum(*c) )
				*d++ = *c;
		*d++ = 0;
	}
	return Result;
	unguard;
}

// Get user name.
CORE_API const TCHAR* appUserName()
{
	guard(appUserName);
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		DWORD Size=ARRAY_COUNT(Result);
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			ANSICHAR ACh[ARRAY_COUNT(Result)];
			GetUserNameA( ACh, &Size );
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, Result, ARRAY_COUNT(Result) );
		}
		else
#endif
		{
			GetUserName( Result, &Size );
		}
		TCHAR *c, *d;
		for( c=Result, d=Result; *c!=0; c++ )
			if( appIsAlnum(*c) )
				*d++ = *c;
		*d++ = 0;
	}
	return Result;
	unguard;
}

// Get launch package base name.
CORE_API const TCHAR* appPackage()
{
	guard(appPackage);
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		TCHAR Tmp[256], *End=Tmp;
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			ANSICHAR ACh[ARRAY_COUNT(Result)];
			GetModuleFileNameA( NULL, ACh, ARRAY_COUNT(ACh) );
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, Tmp, ARRAY_COUNT(Tmp) );
		}
		else
#endif
		{
			GetModuleFileName( NULL, Tmp, ARRAY_COUNT(Tmp) );
		}
		while( appStrchr(End,PATH_SEPARATOR[0]) )
			End = appStrchr(End,PATH_SEPARATOR[0])+1;
		while( appStrchr(End,'/') )
			End = appStrchr(End,'/')+1;
		if( appStrchr(End,'.') )
			*appStrchr(End,'.') = 0;
		appStrcpy( Result, End );
	}
	return Result;
	unguard;
}

/*-----------------------------------------------------------------------------
	App init/exit.
-----------------------------------------------------------------------------*/

//
// Platform specific initialization.
//
static void DoCPUID( int i, DWORD *A, DWORD *B, DWORD *C, DWORD *D )
{
#if ASM
 	__asm
	{			
		mov eax,[i]
		_emit 0x0f
		_emit 0xa2

		mov edi,[A]
		mov [edi],eax

		mov edi,[B]
		mov [edi],ebx

		mov edi,[C]
		mov [edi],ecx

		mov edi,[D]
		mov [edi],edx

		mov eax,0
		mov ebx,0
		mov ecx,0
		mov edx,0
		mov esi,0
		mov edi,0
	}
#else
	*A=*B=*C=*D=0;
#endif
}
void appPlatformPreInit()
{
	guard(appPlatformPreInit);

	// Check Windows version.
	guard(GetWindowsVersion);
	DWORD dwPlatformId, dwMajorVersion, dwMinorVersion, dwBuildNumber;
#if UNICODE
	if( GUnicode && !GUnicodeOS )
	{
		OSVERSIONINFOA Version;
		Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		GetVersionExA(&Version);
		dwPlatformId   = Version.dwPlatformId;
		dwMajorVersion = Version.dwMajorVersion;
		dwMinorVersion = Version.dwMinorVersion;
		dwBuildNumber  = Version.dwBuildNumber;
	}
	else
#endif
	{
		OSVERSIONINFO Version;
		Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&Version);
		dwPlatformId   = Version.dwPlatformId;
		dwMajorVersion = Version.dwMajorVersion;
		dwMinorVersion = Version.dwMinorVersion;
		dwBuildNumber  = Version.dwBuildNumber;
	}
	if( dwPlatformId==VER_PLATFORM_WIN32_NT )
	{
	    if( dwMajorVersion == 5 )
	    {
            if( dwMinorVersion == 0 )
            {
    		    debugf( NAME_Init, TEXT("Detected: Microsoft Windows 2000 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
        	    appSprintf( GMachineOS, TEXT("Windows 2000 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
				GRunningOS = OS_WIN2K;
        	}
            else
            {
    		    debugf( NAME_Init, TEXT("Detected: Microsoft Windows XP %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
        	    appSprintf( GMachineOS, TEXT("Windows XP %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
				GRunningOS = OS_WINXP;
        	}
    	}
    	else
    	{
		debugf( NAME_Init, TEXT("Detected: Microsoft Windows NT %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
        	appSprintf( GMachineOS, TEXT("Windows NT %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
			GRunningOS = OS_WINNT;
        }

		GUnicodeOS = 1;
	}
	else if( dwPlatformId==VER_PLATFORM_WIN32_WINDOWS && (dwMajorVersion>4 || dwMinorVersion>=10) )
	{
		debugf( NAME_Init, TEXT("Detected: Microsoft Windows 98 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
    	appSprintf( GMachineOS, TEXT("Windows 98 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
		GRunningOS = OS_WIN98;
	}
	else if( dwPlatformId==VER_PLATFORM_WIN32_WINDOWS )
	{
		debugf( NAME_Init, TEXT("Detected: Microsoft Windows 95 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
    	appSprintf( GMachineOS, TEXT("Windows 95 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
		GRunningOS = OS_WIN95;
	}
	else
	{
		debugf( NAME_Init, TEXT("Detected: Windows %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
    	appSprintf( GMachineOS, TEXT("Windows %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
		GRunningOS = OS_UNKNOWN;
	}
	unguard;

	unguard;
}
BOOL WINAPI CtrlCHandler( DWORD dwCtrlType )
{
	if( !GIsRequestingExit )
	{
		PostQuitMessage( 0 );
		GIsRequestingExit = 1;
	}
	else
	{
		appErrorf( TEXT("Aborted!") );
	}
	return 1;
}
#ifndef _DEBUG
#pragma optimize( "g", off ) // Fix CPU brand string mangling in release mode
#endif
void appPlatformInit()
{
	guard(appPlatformInit);

	// System initialization.
	GSys = new USystem;
	GSys->AddToRoot();
	for( INT i=0; i<GSys->Suppress.Num(); i++ )
		GSys->Suppress(i).SetFlags( RF_Suppress );

	// Ctrl+C handling.
	SetConsoleCtrlHandler( CtrlCHandler, 1 );

	// Randomize.
    if( GIsBenchmarking )
        srand( 0 );
    else
		srand( (unsigned)time( NULL ) );

	// Identity.
	debugf( NAME_Init, TEXT("Computer: %s"), appComputerName() );
	debugf( NAME_Init, TEXT("User: %s"), appUserName() );

	// Get CPU info.
	guard(GetCpuInfo);
	SYSTEM_INFO SI;
	GetSystemInfo(&SI);
	GPageSize = SI.dwPageSize;
	check(!(GPageSize&(GPageSize-1)));
	GProcessorCount=SI.dwNumberOfProcessors;
	debugf( NAME_Init, TEXT("CPU Page size=%i, Processors=%i"), SI.dwPageSize, SI.dwNumberOfProcessors );
    appSprintf( GMachineCPU, TEXT("%sXBox CPU"), SI.dwNumberOfProcessors > 1 ? TEXT("SMP ") : TEXT("") );
	unguard;

	// Check processor version with CPUID.
	try
	{
		DWORD A=0, B=0, C=0, D=0;
		DoCPUID(0,&A,&B,&C,&D);
		TCHAR Brand[13], *Model, FeatStr[256]=TEXT("");
		Brand[ 0] = (ANSICHAR)(B);
		Brand[ 1] = (ANSICHAR)(B>>8);
		Brand[ 2] = (ANSICHAR)(B>>16);
		Brand[ 3] = (ANSICHAR)(B>>24);
		Brand[ 4] = (ANSICHAR)(D);
		Brand[ 5] = (ANSICHAR)(D>>8);
		Brand[ 6] = (ANSICHAR)(D>>16);
		Brand[ 7] = (ANSICHAR)(D>>24);
		Brand[ 8] = (ANSICHAR)(C);
		Brand[ 9] = (ANSICHAR)(C>>8);
		Brand[10] = (ANSICHAR)(C>>16);
		Brand[11] = (ANSICHAR)(C>>24);
		Brand[12] = (ANSICHAR)(0);
		DoCPUID( 1, &A, &B, &C, &D );
		switch( (A>>8) & 0x000f )
		{
			case 4:  Model=TEXT("486-class processor");        break;
			case 5:  Model=TEXT("Pentium-class processor");    break;
			case 6:  Model=TEXT("PentiumPro-class processor"); break;
			case 7:  Model=TEXT("P7-class processor");         break;
			default: Model=TEXT("Unknown processor");          break;
		}
		if( (D & 0x00008000) ) {appStrcat( FeatStr, TEXT(" CMov") );}
		if( (D & 0x00000001) ) {appStrcat( FeatStr, TEXT(" FPU") );}
		if( (D & 0x00000010) ) {appStrcat( FeatStr, TEXT(" RDTSC") );}
		if( (D & 0x00000040) ) {appStrcat( FeatStr, TEXT(" PAE") );}
		if( (D & 0x00800000) && !ParseParam(appCmdLine(),TEXT("NOMMX")) ) {appStrcat( FeatStr, TEXT(" MMX") );}
		if( (D & 0x02000000) && !ParseParam(appCmdLine(),TEXT("NOSSE")) ) 
		{
			appStrcat( FeatStr, TEXT(" SSE") ); 
			GIsSSE=1;
		}

		// Print features.
		debugf( NAME_Init, TEXT("CPU Detected: %s (%s)"), Model, Brand );
		debugf( NAME_Init, TEXT("CPU Features:%s"), FeatStr );
		
        appSprintf( GMachineCPU, TEXT("%s %s"), Brand, Model );
	}
	catch( ... )
	{
		debugf( NAME_Init, TEXT("Couldn't detect CPU: Probably 486 or non-Intel processor") );
        appSprintf( GMachineCPU, TEXT("Unknown CPU") );
	}
	
	GStartCycles = FCycles();
 
	// CPU speed.
	guard(CheckCpuSpeed);
	FLOAT CpuSpeed;
	try
	{
		timeBeginPeriod( 1 );
		GSecondsPerCycle = 1.f;

		for( INT i=0; i<3; i++ )
		{
			LARGE_INTEGER	StartCycles,
							EndCycles;
			volatile DOUBLE	DoubleDummy = 0.0;
			volatile INT	IntDummy	= 0;
			__asm
			{
				rdtsc
				mov StartCycles.HighPart, edx
				mov StartCycles.LowPart, eax
	}

			DWORD	StartMsec	= timeGetTime(),
					EndMsec		= StartMsec;
			while( EndMsec-StartMsec < 200 )
			{
				DoubleDummy += appSqrt(DoubleDummy) + 3.14;
				IntDummy	*= ((INT) (DoubleDummy) + 555) / 13;
				EndMsec = timeGetTime();
			}
			
			__asm
			{
				rdtsc
				mov EndCycles.HighPart, edx
				mov EndCycles.LowPart, eax
			}
			DOUBLE	C1	= (DOUBLE)(SQWORD)(((QWORD)StartCycles.LowPart) + ((QWORD)StartCycles.HighPart<<32)),
					C2	= (DOUBLE)(SQWORD)(((QWORD)EndCycles.LowPart) + ((QWORD)EndCycles.HighPart<<32));

			GSecondsPerCycle = Min( GSecondsPerCycle, 1.0 / (1000.f * ( C2 - C1 ) / (EndMsec - StartMsec)) );
		}
		timeEndPeriod( 1 );
		debugf( NAME_Init, TEXT("CPU Speed=%f MHz"), 0.000001 / GSecondsPerCycle );
        appStrcat( GMachineCPU, *FString::Printf(TEXT(" @ %d MHz"), appRound(0.000001 / GSecondsPerCycle) ) );
	}
	catch( ... )
	{
		debugf( NAME_Init, TEXT("Timestamp not supported (Possibly 486 or Cyrix processor)") );
        appStrcat( GMachineCPU, TEXT(" Unknown clock") );
		GSecondsPerCycle = 1;
	}
	if( Parse(appCmdLine(),TEXT("CPUSPEED="),CpuSpeed) )
	{
		GSecondsPerCycle = 0.000001/CpuSpeed;
		debugf( NAME_Init, TEXT("CPU Speed Overridden=%f MHz"), 0.000001 / GSecondsPerCycle );
        appStrcat( GMachineCPU, *FString::Printf(TEXT(" @ %d MHz"), appRound(0.000001 / GSecondsPerCycle) ) );
	}
	unguard;

	// Get memory.
	guard(GetMemory);
	MEMORYSTATUS M;
	GlobalMemoryStatus(&M);
	GPhysicalMemory=M.dwTotalPhys;
	debugf( NAME_Init, TEXT("Memory total: Phys=%iK Pagef=%iK Virt=%iK"), M.dwTotalPhys/1024, M.dwTotalPageFile/1024, M.dwTotalVirtual/1024 );
    appStrcat( GMachineCPU, *FString::Printf(TEXT(" with %dMB RAM"), M.dwTotalPhys/(1024 * 1024) ) );
	unguard;

	// Working set.
	guard(GetWorkingSet);
	DWORD WsMin=0, WsMax=0;
	GetProcessWorkingSetSize( GetCurrentProcess(), &WsMin, &WsMax );
	debugf( NAME_Init, TEXT("Working set: %X / %X"), WsMin, WsMax );
	unguard;

	// Create file streaming thread.
	HANDLE hThread = CreateThread( NULL, 0, FileStreamingThread, 0, 0, &FileStreamingThreadId );
	check(hThread);
	CloseHandle( hThread );

	unguard;
}
#ifndef _DEBUG
#pragma optimize("", on)
#endif

void appPlatformPreExit()
{
	guard(appPlatformPreExit);
	unguard;
}
void appPlatformExit()
{
}

#endif
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

