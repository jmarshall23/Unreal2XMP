/*=============================================================================
	UnFile.h: General-purpose file utilities.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// Global variables.
CORE_API extern DWORD GCRCTable[];

/*----------------------------------------------------------------------------
	Byte order conversion.
----------------------------------------------------------------------------*/

// Bitfields.
#ifndef NEXT_BITFIELD
	#if __INTEL_BYTE_ORDER__
		#define NEXT_BITFIELD(b) ((b)<<1)
		#define FIRST_BITFIELD   (1)
		#define INTEL_ORDER(x)   (x)
	#else
		#define NEXT_BITFIELD(b) ((b)>>1)
		#define FIRST_BITFIELD   (0x80000000)
		#define INTEL_ORDER(x)   (((x)>>24) + (((x)>>8)&0xff00) + (((x)<<8)&0xff0000) + ((x)<<24))
	#endif
#endif

/*-----------------------------------------------------------------------------
	Stats.
-----------------------------------------------------------------------------*/

#if STATS
	#define STAT(x) x
#else
	#define STAT(x)
#endif

/*-----------------------------------------------------------------------------
	Global init and exit.
-----------------------------------------------------------------------------*/

CORE_API void appInit( const TCHAR* InPackage, const TCHAR* InCmdLine, FMalloc* InMalloc, FOutputDevice* InLog, FOutputDeviceError* InError, FFeedbackContext* InWarn, FFileManager* InFileManager, FConfigCache*(*ConfigFactory)(), UBOOL RequireConfig );
CORE_API void appPreExit();
CORE_API void appExit();

/*-----------------------------------------------------------------------------
	Logging and critical errors.
-----------------------------------------------------------------------------*/

CORE_API void appRequestExit( UBOOL Force );

CORE_API void VARARGS appFailAssert( const ANSICHAR* Expr, const ANSICHAR* File, INT Line );
CORE_API void VARARGS appUnwindf( const TCHAR* Fmt, ... );
CORE_API const TCHAR* appGetSystemErrorMessage( INT Error=0 );
CORE_API const void appDebugMessagef( const TCHAR* Fmt, ... );
CORE_API const UBOOL appMsgf( INT Type, const TCHAR* Fmt, ... );
CORE_API const void appGetLastError( void );
CORE_API const void EdClearLoadErrors();
CORE_API const void EdLoadErrorf( INT Type, const TCHAR* Fmt, ... );
CORE_API const void appSound(); //NEW (mdf) sound
CORE_API void appDebugBreak();
CORE_API UBOOL appIsDebuggerPresent();

#define debugf				GLog->Logf
#define appErrorf			GError->Logf

#if DO_GUARD_SLOW
	#define debugfSlow		GLog->Logf
	#define appErrorfSlow	GError->Logf
#else
	#define debugfSlow		GNull->Logf
	#define appErrorfSlow	GNull->Logf
#endif

#define GetNameSafe( A ) ((A) ? (A)->GetName() : TEXT("NA") )
#define GetFullNameSafe( A ) ((A) ? (A)->GetFullName() : TEXT("NA") )

#define __TFILE__ winAnsiToTCHAR( __FILE__ )
#define HERE		debugf( TEXT("got here (%s %d)"), __TFILE__, __LINE__ );
#define HERES(s)	debugf( TEXT("%s (%s %d)"), TEXT(s), __TFILE__, __LINE__ );

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

CORE_API void* appGetDllHandle( const TCHAR* DllName );
CORE_API void appFreeDllHandle( void* DllHandle );
CORE_API void* appGetDllExport( void* DllHandle, const TCHAR* ExportName );
CORE_API void appLaunchURL( const TCHAR* URL, const TCHAR* Parms=NULL, FString* Error=NULL );
CORE_API void* appCreateProc( const TCHAR* URL, const TCHAR* Parms );
CORE_API UBOOL appGetProcReturnCode( void* ProcHandle, INT* ReturnCode );
CORE_API class FGuid appCreateGuid();
CORE_API void appCreateTempFilename( const TCHAR* Path, TCHAR* Result256 );
CORE_API void appCleanFileCache();
CORE_API UBOOL appFindPackageFile( const TCHAR* In, const FGuid* Guid, TCHAR* Out );
CORE_API UBOOL appCreateBitmap( const TCHAR* Pattern, INT Width, INT Height, DWORD* Data, FFileManager* FileManager = GFileManager );

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

CORE_API void appClipboardCopy( const TCHAR* Str );
CORE_API FString appClipboardPaste();

/*-----------------------------------------------------------------------------
	Guard macros for call stack display.
-----------------------------------------------------------------------------*/

//
// guard/unguardf/unguard macros.
// For showing calling stack when errors occur in major functions.
// Meant to be enabled in release builds.
//

#if _DEBUG || !DO_GUARD || __UNIX__
#if /* FIXME: */ __LINUX__
	#define guard(func)			{
#else
	#define guard(func)			{static const TCHAR __FUNC_NAME__[]=TEXT(#func);
#endif
	#define unguard				}
	#define unguardf(msg)		}
#else
	#define guard(func)			{static const TCHAR __FUNC_NAME__[]=TEXT(#func); try{
	#define unguard				}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),__FUNC_NAME__); throw;}}
	#define unguardf(msg)		}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),__FUNC_NAME__); appUnwindf msg; throw;}}
#endif

//
// guardSlow/unguardfSlow/unguardSlow macros.
// For showing calling stack when errors occur in performance-critical functions.
// Meant to be disabled in release builds.
//
#if defined(_DEBUG) || !DO_GUARD || !DO_GUARD_SLOW
	#define guardSlow(func)		{
	#define unguardfSlow(msg)	}
	#define unguardSlow			}
	#define unguardfSlow(msg)	}
#else
	#define guardSlow(func)		guard(func)
	#define unguardSlow			unguard
	#define unguardfSlow(msg)	unguardf(msg)
#endif

//
// For throwing string-exceptions which safely propagate through guard/unguard.
//
CORE_API void VARARGS appThrowf( const TCHAR* Fmt, ... );

/*-----------------------------------------------------------------------------
	Check macros for assertions.
-----------------------------------------------------------------------------*/

//
// "check" expressions are only evaluated if enabled.
// "verify" expressions are always evaluated, but only cause an error if enabled.
//
#if DO_CHECK
	#define check(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
	#define verify(expr) {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
#else
	#define check(expr) {}
	#define verify(expr) if(expr){}
#endif

//
// Check for development only.
//
#if DO_GUARD_SLOW
	#define checkSlow(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
	#define verifySlow(expr) {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
#else
	#define checkSlow(expr) {}
	#define verifySlow(expr) if(expr){}
#endif

/*-----------------------------------------------------------------------------
	Timing macros.
-----------------------------------------------------------------------------*/

//
// Normal timing.
//
#if 1 //NEW (mdf)
/*
Note that the overhead for the clock/unclock calls themselves can vary across
different systems and depending on the circumstances due to out of order
execution, cache effects etc. The lowest overhead that I've seen is 11 (this
was seen consistently on a 1.67GHz Athlon system for example) while on some
machines, this is as high as 80 (2.8 GHz hyperthreaded Intel system). Also,
appCycles currently tracks only the low RDSTC count (EAX) which means that
the counter will wrap after about 1.33 secs on a 3GHz CPU. As long as this
is used to time code which takes < about 1 second to execute and as long
as very precise timings aren't required (where the variation in the overhead
could make a difference) this shouldn't be a problem.

See http://cedar.intel.com/software/idap/media/pdf/rdtscpm1.pdf for details.
*/

#define CLOCK_OVERHEAD 11
#define clock(Timer)	{ Timer -= appCycles(); }
#define unclock(Timer)	{ Timer += appCycles()-CLOCK_OVERHEAD; }
#define dclock(Timer)   { debugf( L"clock   %-16s", TEXT(#Timer) ); clock(Timer); }
#define dunclock(Timer) { debugf( L"unclock %-16s", TEXT(#Timer) ); unclock(Timer); }
#else
#define clock(Timer)   {Timer -= appCycles();}
#define unclock(Timer) {Timer += appCycles()-12;}
#endif

//
// Performance critical timing.
//
#if DO_CLOCK_SLOW
	#define clockSlow(Timer)   {Timer-=appCycles();}
#if 1 //NEW (mdf)
	#define unclockSlow(Timer) {Timer+=appCycles()-CLOCK_OVERHEAD;}
#else	
	#define unclockSlow(Timer) {Timer+=appCycles()-12;}
#endif
#else
	#define clockSlow(Timer) {}
	#define unclockSlow(Timer) {}
#endif

/*-----------------------------------------------------------------------------
	FClock.
  Safe for timing functions with multiple return points.
-----------------------------------------------------------------------------*/

class FClock
{
public:
	FClock(DWORD &InTimer)
		: Timer(InTimer)
		{ clock(Timer); }
	~FClock()
		{ unclock(Timer); }
private:
	DWORD &Timer;
};

/*-----------------------------------------------------------------------------
	Text format.
-----------------------------------------------------------------------------*/

CORE_API FString appFormat( FString Src, const TMultiMap<FString,FString>& Map );

/*-----------------------------------------------------------------------------
	Localization.
-----------------------------------------------------------------------------*/

CORE_API const TCHAR* Localize( const TCHAR* Section, const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
CORE_API const TCHAR* LocalizeError( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeProgress( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeQuery( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeGeneral( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );

#if UNICODE
	CORE_API const TCHAR* Localize( const ANSICHAR* Section, const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
	CORE_API const TCHAR* LocalizeError( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
	CORE_API const TCHAR* LocalizeProgress( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
	CORE_API const TCHAR* LocalizeQuery( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
	CORE_API const TCHAR* LocalizeGeneral( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
#endif

/*-----------------------------------------------------------------------------
	File functions.
-----------------------------------------------------------------------------*/

// File utilities.
CORE_API const TCHAR* appFExt( const TCHAR* Filename );
CORE_API UBOOL appUpdateFileModTime( TCHAR* Filename );
CORE_API FString appGetGMTRef();

/*-----------------------------------------------------------------------------
	OS functions.
-----------------------------------------------------------------------------*/

CORE_API const TCHAR* appCmdLine();
CORE_API const TCHAR* appBaseDir();
CORE_API const TCHAR* appPackage();
CORE_API const TCHAR* appComputerName();
CORE_API const TCHAR* appUserName();

/*-----------------------------------------------------------------------------
	Timing functions.
-----------------------------------------------------------------------------*/

#if !DEFINED_appCycles
CORE_API DWORD appCycles();
#endif

#if !DEFINED_appSeconds
CORE_API DOUBLE appSeconds();
#endif

CORE_API void appSystemTime( INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec );
CORE_API SQWORD appSystemTime64();
CORE_API const TCHAR* appTimestamp();
CORE_API DOUBLE appSecondsSlow();
CORE_API void appSleep( FLOAT Seconds );

/*-----------------------------------------------------------------------------
	Character type functions.
-----------------------------------------------------------------------------*/

inline TCHAR appToUpper( TCHAR c )
{
	return (c<'a' || c>'z') ? (c) : (c+'A'-'a');
}
inline TCHAR appToLower( TCHAR c )
{
	return (c<'A' || c>'Z') ? (c) : (c+'a'-'A');
}
inline UBOOL appIsUpper( TCHAR c )
{
	return (c>='A' && c<='Z');
}
inline UBOOL appIsLower( TCHAR c )
{
	return (c>='a' && c<='z');
}
inline UBOOL appIsAlpha( TCHAR c )
{
	return (c>='a' && c<='z') || (c>='A' && c<='Z');
}
inline UBOOL appIsDigit( TCHAR c )
{
	return c>='0' && c<='9';
}
inline UBOOL appIsAlnum( TCHAR c )
{
	return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9');
}

#if 1 //NEW (mdf)
inline UBOOL appIsNameChar( TCHAR c )
{
	return ( (c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') || (c=='_') || (c==' ') );
}

inline UBOOL appIsFloatChar( TCHAR c )
{
	return ( appIsDigit( c ) || c == '.' || c == '-' );
}

inline UBOOL appIsIntChar( TCHAR c )
{
	return ( appIsDigit( c ) || c == '-' );
}
#endif

#include <ctype.h>
inline UBOOL appIsSpace( TCHAR c )
{
#if UNICODE
    return( iswspace(c) );
#else
    return( isspace(c) );
#endif
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

CORE_API const ANSICHAR* appToAnsi( const TCHAR* Str, ANSICHAR* ACh=NULL  );
CORE_API const UNICHAR* appToUnicode( const TCHAR* Str );
CORE_API const TCHAR* appFromAnsi( const ANSICHAR* Str, TCHAR* TCh=NULL );
CORE_API const TCHAR* appFromUnicode( const UNICHAR* Str );
CORE_API UBOOL appIsPureAnsi( const TCHAR* Str );

CORE_API TCHAR* appStrcpy( TCHAR* Dest, const TCHAR* Src );
CORE_API INT appStrcpy( const TCHAR* String );
CORE_API INT appStrlen( const TCHAR* String );
CORE_API TCHAR* appStrstr( const TCHAR* String, const TCHAR* Find );
CORE_API TCHAR* appStrchr( const TCHAR* String, INT c );
CORE_API TCHAR* appStrcat( TCHAR* Dest, const TCHAR* Src );
CORE_API INT appStrcmp( const TCHAR* String1, const TCHAR* String2 );
CORE_API INT appStricmp( const TCHAR* String1, const TCHAR* String2 );
CORE_API INT appStrncmp( const TCHAR* String1, const TCHAR* String2, INT Count );
CORE_API TCHAR* appStaticString1024();
CORE_API ANSICHAR* appAnsiStaticString1024();

CORE_API const TCHAR* appSpc( int Num );
CORE_API TCHAR* appStrncpy( TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrncat( TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrupr( TCHAR* String );
CORE_API const TCHAR* appStrfind(const TCHAR* Str, const TCHAR* Find);
CORE_API DWORD appStrCrc( const TCHAR* Data );
CORE_API DWORD appStrCrcCaps( const TCHAR* Data );
CORE_API INT appAtoi( const TCHAR* Str );
CORE_API TCHAR* appItoa( const INT Num );
CORE_API FLOAT appAtof( const TCHAR* Str );
CORE_API INT appStrtoi( const TCHAR* Start, TCHAR** End, INT Base );
CORE_API INT appStrnicmp( const TCHAR* A, const TCHAR* B, INT Count );
CORE_API INT appSprintf( TCHAR* Dest, const TCHAR* Fmt, ... );
CORE_API void appTrimSpaces( ANSICHAR* String);

#if UNICODE
	#define appSSCANF	swscanf
#else
	#define appSSCANF	sscanf
#endif

#if _MSC_VER
	CORE_API INT appGetVarArgs( TCHAR* Dest, INT Count, const TCHAR*& Fmt );
	CORE_API INT appGetVarArgsAnsi( ANSICHAR* Dest, INT Count, const ANSICHAR*& Fmt );
#else
	#include <stdio.h>
	#include <stdarg.h>
#endif

typedef int QSORT_RETURN;
typedef QSORT_RETURN(CDECL* QSORT_COMPARE)( const void* A, const void* B );
CORE_API void appQsort( void* Base, INT Num, INT Width, QSORT_COMPARE Compare );

//
// Case insensitive string hash function.
//
inline DWORD appStrihash( const TCHAR* Data )
{
	DWORD Hash=0;
	while( *Data )
	{
		TCHAR Ch = appToUpper(*Data++);
		BYTE  B  = Ch;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
#if UNICODE
		B        = Ch>>8;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
#endif
	}
	return Hash;
}

/*-----------------------------------------------------------------------------
	Parsing functions.
-----------------------------------------------------------------------------*/

CORE_API UBOOL ParseCommand( const TCHAR** Stream, const TCHAR* Match );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FName& Name );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, DWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FGuid& Guid );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, TCHAR* Value, INT MaxLen );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, BYTE& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SBYTE& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, _WORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FLOAT& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, INT& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FString& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, QWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SQWORD& Value );
CORE_API UBOOL ParseUBOOL( const TCHAR* Stream, const TCHAR* Match, UBOOL& OnOff );
CORE_API UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, class UClass* Type, class UObject*& DestRes, class UObject* InParent );
CORE_API UBOOL ParseLine( const TCHAR** Stream, TCHAR* Result, INT MaxLen, UBOOL Exact=0 );
CORE_API UBOOL ParseLine( const TCHAR** Stream, FString& Resultd, UBOOL Exact=0 );
CORE_API UBOOL ParseToken( const TCHAR*& Str, TCHAR* Result, INT MaxLen, UBOOL UseEscape );
CORE_API UBOOL ParseToken( const TCHAR*& Str, FString& Arg, UBOOL UseEscape );
CORE_API FString ParseToken( const TCHAR*& Str, UBOOL UseEscape );
CORE_API void ParseNext( const TCHAR** Stream );
CORE_API UBOOL ParseParam( const TCHAR* Stream, const TCHAR* Param );

/*-----------------------------------------------------------------------------
	Math functions.
-----------------------------------------------------------------------------*/

CORE_API DOUBLE appExp( DOUBLE Value );
CORE_API DOUBLE appLoge( DOUBLE Value );
CORE_API DOUBLE appFmod( DOUBLE A, DOUBLE B );
CORE_API DOUBLE appSin( DOUBLE Value );
CORE_API DOUBLE appAsin( DOUBLE Value );
CORE_API DOUBLE appCos( DOUBLE Value );
CORE_API DOUBLE appAcos( DOUBLE Value );
CORE_API DOUBLE appTan( DOUBLE Value );
CORE_API DOUBLE appAtan( DOUBLE Value );
CORE_API DOUBLE appAtan2( DOUBLE Y, DOUBLE X );
CORE_API DOUBLE appSqrt( DOUBLE Value );
CORE_API DOUBLE appPow( DOUBLE A, DOUBLE B );
CORE_API UBOOL appIsNan( DOUBLE Value );
CORE_API void appRandInit( INT Seed );
CORE_API INT appRand();
CORE_API FLOAT appFrand();
CORE_API UBOOL appFDecision( float Odds );
CORE_API FLOAT appRandRange( FLOAT Min, FLOAT Max );
CORE_API INT appRandRange( INT Min, INT Max );
CORE_API FLOAT appBlend( FLOAT A, FLOAT B, FLOAT Pct );
CORE_API DOUBLE appBlend( DOUBLE A, DOUBLE B, DOUBLE Pct );
CORE_API FLOAT appAblend( FLOAT A, FLOAT B, FLOAT Result );
CORE_API DOUBLE appAblend( DOUBLE A, DOUBLE B, DOUBLE Result );

#if !DEFINED_appSRandInit
CORE_API void appSRandInit( INT );
#endif

#if !DEFINED_appSRand
CORE_API FLOAT appSRand();
#endif

#if !DEFINED_appRound
CORE_API INT appRound( FLOAT Value );
#endif

#if !DEFINED_appFloor
CORE_API INT appFloor( FLOAT Value );
#endif

#if !DEFINED_appCeil
CORE_API INT appCeil( FLOAT Value );
#endif

#if !DEFINED_appFractional
CORE_API FLOAT appFractional( FLOAT Value );
#endif

/*-----------------------------------------------------------------------------
	Array functions.
-----------------------------------------------------------------------------*/

// Core functions depending on TArray and FString.
CORE_API UBOOL appLoadFileToArray( TArray<BYTE>& Result, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
CORE_API UBOOL appLoadFileToString( FString& Result, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
CORE_API UBOOL appSaveArrayToFile( const TArray<BYTE>& Array, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
CORE_API UBOOL appSaveStringToFile( const FString& String, const TCHAR* Filename, FFileManager* FileManager=GFileManager );

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

CORE_API void* appMemmove( void* Dest, const void* Src, INT Count );
CORE_API INT appMemcmp( const void* Buf1, const void* Buf2, INT Count );
CORE_API UBOOL appMemIsZero( const void* V, int Count );
CORE_API DWORD appMemCrc( const void* Data, INT Length, DWORD CRC=0 );
CORE_API void appMemswap( void* Ptr1, void* Ptr2, DWORD Size );
CORE_API void appMemset( void* Dest, INT C, INT Count );

#ifndef DEFINED_appMemcpy
CORE_API void appMemcpy( void* Dest, const void* Src, INT Count );
#endif

#ifndef DEFINED_appMemzero
CORE_API void appMemzero( void* Dest, INT Count );
#endif

//
// C style memory allocation stubs.
//
#define appMalloc     GMalloc->Malloc
#define appFree       GMalloc->Free
#define appRealloc    GMalloc->Realloc

//
// C++ style memory allocation.
//
inline void* operator new( size_t Size, const TCHAR* Tag )
{
	guardSlow(new);
	return appMalloc( Size, Tag );
	unguardSlow;
}
inline void* operator new( size_t Size )
{
	guardSlow(new);
	return appMalloc( Size, TEXT("new") );
	unguardSlow;
}
inline void operator delete( void* Ptr )
{
	guardSlow(delete);
	appFree( Ptr );
	unguardSlow;
}

#if PLATFORM_NEEDS_ARRAY_NEW
inline void* operator new[]( size_t Size, const TCHAR* Tag )
{
	guardSlow(new);
	return appMalloc( Size, Tag );
	unguardSlow;
}
inline void* operator new[]( size_t Size )
{
	guardSlow(new);
	return appMalloc( Size, TEXT("new") );
	unguardSlow;
}
inline void operator delete[]( void* Ptr )
{
	guardSlow(delete);
	appFree( Ptr );
	unguardSlow;
}
#endif

/*-----------------------------------------------------------------------------
	Math.
-----------------------------------------------------------------------------*/

CORE_API BYTE appCeilLogTwo( DWORD Arg );

/*-----------------------------------------------------------------------------
	MD5 functions.
-----------------------------------------------------------------------------*/

//
// MD5 Context.
//
struct FMD5Context
{
	DWORD state[4];
	DWORD count[2];
	BYTE buffer[64];
};

//
// MD5 functions.
//!!it would be cool if these were implemented as subclasses of
// FArchive.
//
CORE_API void appMD5Init( FMD5Context* context );
CORE_API void appMD5Update( FMD5Context* context, BYTE* input, INT inputLen );
CORE_API void appMD5Final( BYTE* digest, FMD5Context* context );
CORE_API void appMD5Transform( DWORD* state, BYTE* block );
CORE_API void appMD5Encode( BYTE* output, DWORD* input, INT len );
CORE_API void appMD5Decode( DWORD* output, BYTE* input, INT len );

/*-----------------------------------------------------------------------------
	Misc functions.
-----------------------------------------------------------------------------*/
CORE_API QSORT_RETURN CDECL CompareObjectNames( const UObject** A, const UObject** B );
CORE_API UBOOL MakeWriteable( const TCHAR* Filename );

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

