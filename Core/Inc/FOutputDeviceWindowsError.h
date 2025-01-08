/*=============================================================================
	FOutputDeviceWindowsError.h: Windows error message outputter.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// Handle a critical error.
//
class FOutputDeviceWindowsError : public FOutputDeviceError
{
	INT ErrorPos;
	EName ErrorType;
public:
	FOutputDeviceWindowsError()
	: ErrorPos(0)
	, ErrorType(NAME_None)
	{}
	void Serialize( const TCHAR* Msg, enum EName Event )
	{

#if _DEBUG
		// Just display info and break the debugger.
  		debugf( NAME_Critical, TEXT("appError called while debugging:") );
		debugf( NAME_Critical, Msg );
		UObject::StaticShutdownAfterError();

		if( appIsDebuggerPresent() )
		{	
  			debugf( NAME_Critical, TEXT("Breaking debugger") );
			appDebugBreak();
		}
		else
		{
			MessageBox( NULL, Msg, GConfig ? LocalizeError(TEXT("Critical"),TEXT("Window")) : TEXT("Critical Error At Startup"), MB_OK|MB_ICONERROR|MB_TASKMODAL );
			if( !GIsCriticalError )
			{
				GIsCriticalError = 1;
				appExit();
			}
			appRequestExit(1);
		}
#else
		INT Error = GetLastError();
		if( !GIsCriticalError )
		{
			// First appError.
			GIsCriticalError = 1;
			ErrorType        = Event;
			debugf( NAME_Critical, TEXT("appError called:") );
			debugf( NAME_Critical, TEXT("%s"), Msg );

			// Windows error.
			debugf( NAME_Critical, TEXT("Windows GetLastError: %s (%i)"), appGetSystemErrorMessage(Error), Error );

			// Shut down.
			UObject::StaticShutdownAfterError();
			appStrncpy( GErrorHist, Msg, ARRAY_COUNT(GErrorHist) );
			appStrncat( GErrorHist, TEXT("\r\n\r\n"), ARRAY_COUNT(GErrorHist) );
			ErrorPos = appStrlen(GErrorHist);
			if( GIsGuarded )
			{
				appStrncat( GErrorHist, GIsGuarded ? LocalizeError("History",TEXT("Core")) : TEXT("History: "), ARRAY_COUNT(GErrorHist) );
				appStrncat( GErrorHist, TEXT(": "), ARRAY_COUNT(GErrorHist) );
			}
			else		
				HandleError();
		}
		else debugf( NAME_Critical, TEXT("Error reentered: %s"), Msg );

		// Propagate the error or exit.
		if( GIsGuarded )
			throw( 1 );
		else
			appRequestExit( 1 );
#endif
	}
	void HandleError()
	{
		try
		{
			GIsGuarded       = 0;
			GIsRunning       = 0;
			GIsCriticalError = 1;
			GLogHook         = NULL;
			UObject::StaticShutdownAfterError();
			GErrorHist[ErrorType==NAME_FriendlyError ? ErrorPos : ARRAY_COUNT(GErrorHist)-1]=0;
			if( GIsClient || GIsEditor || !GIsStarted )
			{
				try
				{
#if 0 //RELEASE
					FString Message = FString::Printf( TEXT("%s\r\n\r\nBuild: %s %s\r\n"),
						GErrorHist, appFromAnsi(__DATE__), appFromAnsi(__TIME__) );
#elif 1
					INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
					appSystemTime( Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec );
					appSprintf( GCrashLogFilename,
						TEXT("\\\\MegaMonkey\\Vol1\\Projects\\U2XMP\\Logs\\%s_%s_%04d%02d%02d%02d%02d%02d.log"),
						appComputerName(), appUserName(),
						Year, Month, Day, Hour, Min, Sec
					);
					FString Message = FString::Printf( TEXT("%s\r\n\r\nBuild: %s %s\r\nLog file:%s"),
						GErrorHist, appFromAnsi(__DATE__), appFromAnsi(__TIME__), GCrashLogFilename );
#endif
					WCrashBoxDialog CrashBox( GConfig ? LocalizeError(TEXT("Critical"),TEXT("Window")) : TEXT("Critical Error At Startup"), *Message );
					// work around WndProc error handling
					GIsCriticalError = 0;
					CrashBox.DoModal();
					GIsCriticalError = 1;
				}
				catch ( ... )
				{
					MessageBox( NULL, GErrorHist, GConfig ? LocalizeError(TEXT("Critical"),TEXT("Window")) : TEXT("Critical Error At Startup"), MB_OK|MB_ICONERROR|MB_TASKMODAL );
				}				
			}
		}
		catch( ... )
		{}
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

