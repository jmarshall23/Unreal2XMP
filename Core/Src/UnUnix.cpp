/*=============================================================================
	UnIX: Unix port of UnVcWin32.cpp.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Cloned by Mike Danylchuk
		* Severely amputated and mutilated by Brandon Reinhart
		* Surgically altered by Jack Porter
		* Mangled and rehabilitated by Brandon Reinhart
		* Obfuscated by Daniel Vogel
		* Peed on by Ryan C. Gordon
=============================================================================*/
#if __GNUG__ || (__INTEL_COMPILER && linux)

// Standard includes.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <time.h>
#include <errno.h>

// System includes.
#include <sys/param.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/stat.h>

#ifndef MACOSX
#include <dlfcn.h>
#endif

#include <netdb.h>
//#include <fpu_control.h>

#if __FILE_NOCASE
#include <sys/types.h>
#include <dirent.h>
#endif

#define USE_FILESTREAM_THREAD

#ifdef USE_FILESTREAM_THREAD
#include <pthread.h>
#endif

#include <sys/wait.h>

#if ((!defined MACOSX) && (!defined __FreeBSD__))
#include <execinfo.h>
#endif

// Core includes.
#include "CorePrivate.h"

// rcg04232002 Just to make sure this is sane...
#if defined(NO_UNICODE_OS_SUPPORT)
#if defined(UNICODE)
#undef UNICODE
#define UNICODE 0
#endif
#endif

#if UNICODE
#include <locale.h>

#ifndef MACOSX
#  include <langinfo.h>
#endif

#endif

#ifdef MACOSX  // Carbon includes.  --ryan.
   // Carbon's headers define "check" and "verify", which we do, too,
   //  so just predeclare functions we need here.
extern "C"
{
   signed int GetCPUSpeed(void);
}
#endif

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

/*-----------------------------------------------------------------------------
	Globals
-----------------------------------------------------------------------------*/

// Module
ANSICHAR GModule[32];

// Environment
extern char **environ;

TCHAR *GUnixSpawnOnExit = NULL;


#if UNICODE
CORE_API void unicode_str_to_stdout(const UNICHAR *str)
{
    if (str == NULL)
        str = TEXT("(null)");

    printf("UNICODE str: ");
    for (int i = 0; str[i]; i++)
        printf("%c", (ANSICHAR) str[i]);
    printf("\n");
}


CORE_API ANSICHAR* unixToANSI( ANSICHAR* ACh, const UNICHAR* InUCh )
{
	guardSlow(unixToANSI);
	if(InUCh)
	{
        INT Index;
		for(Index = 0;InUCh[Index] != 0;Index++)
		{
        	#if 0
				if (InUCh[Index] > 255)
					debugf("unixToANSI: can't convert character %d!", (int) InUCh[Index]);
            #endif

			ACh[Index] = (ANSICHAR) InUCh[Index];
		}
        ACh[Index] = 0;
		return ACh;
	}
	else
	{
		ACh[0] = 0;
		return ACh;
	}
	unguardSlow;
}


CORE_API INT unixGetSizeANSI( const UNICHAR* InUCh )
{
	guardSlow(unixGetSizeANSI);
	if(InUCh)
		return (wcslen(InUCh) + 1) * sizeof(ANSICHAR);
	else
		return sizeof(ANSICHAR);
	unguardSlow;
}


CORE_API UNICHAR* unixToUNICODE( UNICHAR* UCh, const ANSICHAR* InACh )
{
	guardSlow(unixToUNICODE);
	if(InACh)
	{
        INT Index;
		for(Index = 0;InACh[Index] != 0;Index++)
        {
			UCh[Index] = (UNICHAR) InACh[Index];
        }
        UCh[Index] = 0;
		return UCh;
	}
	else
	{
		UCh[0] = 0;
		return UCh;
	}
    unguardSlow;
}


CORE_API INT unixGetSizeUNICODE( const ANSICHAR* InACh )
{
	guardSlow(unixGetSizeUNICODE);
	if(InACh)
		return (strlen(InACh) + 1) * sizeof(UNICHAR);
	else
		return sizeof(UNICHAR);
    unguard;
}


CORE_API UNICHAR* unixANSIToUNICODE(char* str)
{
	guardSlow(unixANSIToUnicode);
	INT iLength = unixGetSizeUNICODE(str);
	UNICHAR* pBuffer = new UNICHAR[iLength];
	appStrcpy(pBuffer,TEXT(""));  // !!! FIXME: Is this really necessary?
	return unixToUNICODE(pBuffer,str);
    unguardSlow;
}


CORE_API INT unixDetectUNICODE( void )
{
    guard(unixDetectUNICODE);
    errno = 0;
    if (!setlocale(LC_CTYPE, ""))
    {
        debugf(TEXT("setlocale() failed: %s"), strerror(errno));
        debugf(TEXT("Must assume ANSI (8-bit) character encoding."));
    }
    else
    {
        char *encoding = nl_langinfo(_NL_CTYPE_CODESET_NAME);
        if (getenv("UNREAL_UNICODE") != NULL)
            GUnicode = GUnicodeOS = 1;
        else
        {
            char *unicode_encodings[] = { "UTF-8", "UTF8", NULL };
            for (INT i = 0; unicode_encodings[i]; i++)
            {
                if (strcmp(encoding, unicode_encodings[i]) == 0)
                {
                    GUnicode = GUnicodeOS = 1;
                }
            }
        }

        debugf(TEXT("Your locale: [%s].\n"), ANSI_TO_TCHAR(encoding));
        if (GUnicodeOS)
            debugf(TEXT("Enabled UNICODE support."));
    }
    unguard;
}
#endif


#if UNICODE_BY_HAND
CORE_API UNICHAR* wcscpy( UNICHAR* _Dest, const UNICHAR* Src)
{
    guardSlow(wcscpy)

    UNICHAR *Dest = _Dest;

    while (*Src)
    {
        *Dest = *Src;
        Dest++;
        Src++;
    }

    *Dest = 0;

    return(_Dest);

    unguardSlow;
}


CORE_API UNICHAR* wcsncpy( UNICHAR* _Dest, const UNICHAR* Src, INT MaxLen )
{
    guardSlow(wcsncpy)

    UNICHAR *Dest = _Dest;

    MaxLen--;  // always leave room for null char.
    for (INT i = 0; ((i < MaxLen) && (*Src)); i++)
    {
        *Dest = *Src;
        Dest++;
        Src++;
    }

    *Dest = 0;

    return(Dest);

    unguardSlow;
}


CORE_API INT wcslen( const UNICHAR* String )
{
    guardSlow(wcslen);

    INT retval = 0;
    for (retval = 0; *String; String++)
        retval++;

    return(retval);

    unguardSlow;
}


CORE_API INT wcscmp( const UNICHAR* Str1, const UNICHAR *Str2 )
{
    guardSlow(wcscmp);

    while (true)
    {
        UNICHAR ch1 = *Str1;
        UNICHAR ch2 = *Str2;

        if ((ch1 == 0) && (ch1 == ch2))
            return(0);

        if (ch1 < ch2)
            return(-1);
        else if (ch1 > ch2)
            return(1);

        Str1++;
        Str2++;
    }

    return(1);  // shouldn't ever hit this.

    unguardSlow;
}


CORE_API INT wcsncmp( const UNICHAR* Str1, const UNICHAR *Str2, INT max )
{
    guardSlow(wcscmp);

    INT i = 0;
    while (true)
    {
        UNICHAR ch1 = *Str1;
        UNICHAR ch2 = *Str2;

        if (i == max)
            return(0);

        if ((ch1 == 0) && (ch1 == ch2))
            return(0);

        if (ch1 < ch2)
            return(-1);
        else if (ch1 > ch2)
            return(1);

        i++;
        Str1++;
        Str2++;
    }

    return(1);  // shouldn't ever hit this.

    unguardSlow;
}


CORE_API INT _wcsicmp( const UNICHAR* String1, const UNICHAR* String2 )
{
    guardSlow(_wcsicmp);

    /*
    printf("_wcsicmp:\n");
    printf("  "); unicode_str_to_stdout(String1);
    printf("  "); unicode_str_to_stdout(String2);
    */

    while (true)
    {
        int ch1 = toupper((int) *String1);
        int ch2 = toupper((int) *String2);

        if ((ch1 == 0) && (ch1 == ch2))
            return(0);

        if (ch1 < ch2)
            return(-1);
        else if (ch1 > ch2)
            return(1);

        String1++;
        String2++;
    }

    return(1);  // shouldn't ever hit this.

    unguardSlow;
}



CORE_API INT _wcsnicmp( const UNICHAR* Str1, const UNICHAR *Str2, INT max )
{
    guardSlow(_wcsnicmp);

    /*
    printf("_wcsnicmp:\n");
    printf("  "); unicode_str_to_stdout(Str1);
    printf("  "); unicode_str_to_stdout(Str2);
    */

    INT i = 0;
    while (true)
    {
        UNICHAR ch1 = toupper((int) *Str1);
        UNICHAR ch2 = toupper((int) *Str2);

        if (i == max)
            return(0);

        if ((ch1 == 0) && (ch1 == ch2))
            return(0);

        if (ch1 < ch2)
            return(-1);
        else if (ch1 > ch2)
            return(1);

        i++;
        Str1++;
        Str2++;
    }

    return(1);  // shouldn't ever hit this.

    unguardSlow;
}


CORE_API UNICHAR* wcschr( const UNICHAR* String, const UNICHAR Find )
{
    guardSlow(wcschr);

    while (*String)
    {
        if (*String == Find)
            return((UNICHAR *) String);

        String++;
    }

    return(NULL);

    unguardSlow;
}


CORE_API UNICHAR* wcsstr( const UNICHAR* String, const UNICHAR* Find )
{
    guardSlow(wcsstr);
    INT findlen = wcslen(Find);

    while (true)
    {
        String = wcschr(String, *Find);
        if ( String == NULL)
            return(NULL);

        if (wcsncmp(String, Find, findlen) == 0)
            return((UNICHAR *) String);

        String++;
    }

    return(NULL);  // shouldn't ever hit this point.
    unguardSlow;
}


CORE_API UNICHAR* wcscat( UNICHAR* String, const UNICHAR *Add )
{
    guardSlow(wcscat);
    wcscpy(String + wcslen(String), Add);
    return(String);
    unguardSlow;
}


CORE_API UNICHAR* _wcsupr( UNICHAR* String )
{
    guardSlow(_wcsupr);
    for (UNICHAR *ptr = String; *ptr; ptr++)
        *ptr = (UNICHAR) toupper((int) *ptr);

    return(String);
    unguardSlow;
}


CORE_API INT _wtoi( const UNICHAR* Str )
{
    // !!! FIXME: This might be completely broken.
    char *ansistr = TCHAR_TO_ANSI(Str);
    return(atoi(ansistr));
}


CORE_API INT wcstoul( const TCHAR* Start, TCHAR** End, INT Base )
{
    // !!! FIXME: This might be completely broken.
    return(strtoul(TCHAR_TO_ANSI(Start), (char **) End, Base));
}

CORE_API INT swscanf( const UNICHAR* fmt, ... )
{
    debugf(TEXT("This is currently unimplemented on Unix."));
    appDebugBreak();
}

// !!! FIXME: This can always be more robust.
CORE_API INT wvsnprintf( TCHAR *buf, INT max, const TCHAR *fmt, va_list args )
{
    if (fmt == NULL)
    {
        if ((max > 0) && (buf != NULL))
            *buf = 0;
        return(0);
    }

    TCHAR *src = (TCHAR *) appAlloca((wcslen(fmt) + 1) * sizeof (TCHAR));
    wcscpy(src, fmt);

    TCHAR *dst = buf;
    TCHAR *enddst = dst + (max - 1);

    //printf("printf by-hand formatting "); unicode_str_to_stdout(src);

    while ((*src) && (dst < enddst))
    {
        if (*src != '%')
        {
            *dst = *src;
            dst++;
            src++;
            continue;
        }

        TCHAR *percent_ptr = src;
        INT fieldlen = -1;
        INT precisionlen = -1;

        src++; // skip the '%' char...

        while (*src == ' ')
        {
            *dst = ' ';
            dst++;
            src++;
        }

            // check for field width requests...
        if ((*src == '-') || ((*src >= '0') && (*src <= '9')))
        {
            TCHAR *ptr = src + 1;
            while ((*ptr >= '0') && (*ptr <= '9'))
                ptr++;

            TCHAR ch = *ptr;
            *ptr = '\0';
            fieldlen = atoi(TCHAR_TO_ANSI(src));
            *ptr = ch;

            src = ptr;
        }

        if (*src == '.')
        {
            TCHAR *ptr = src + 1;
            while ((*ptr >= '0') && (*ptr <= '9'))
                ptr++;

            TCHAR ch = *ptr;
            *ptr = '\0';
            precisionlen = atoi(TCHAR_TO_ANSI(src + 1));
            *ptr = ch;
            src = ptr;
        }

        switch (*src)
        {
            case '%':
            {
                src++;
                *dst = '%';
                dst++;
            }
            break;

            case 'c':
            {
                TCHAR val = va_arg(args, TCHAR);
                src++;
                *dst = val;
                dst++;
            }
            break;

            case 'd':
            case 'i':
            case 'X':
            case 'x':
            case 'u':
            {
                src++;
                int val = va_arg(args, int);
                ANSICHAR ansinum[30];
                ANSICHAR fmtbuf[30];

                // Yes, this is lame.
                INT cpyidx = 0;
                while (percent_ptr < src)
                {
                    fmtbuf[cpyidx] = (ANSICHAR) *percent_ptr;
                    percent_ptr++;
                    cpyidx++;
                }
                fmtbuf[cpyidx] = 0;

                int rc = snprintf(ansinum, sizeof (ansinum), fmtbuf, val);
                if ((dst + rc) > enddst)
                    rc = enddst - dst;
                for (int i = 0; i < rc; i++)
                {
                    *dst = (TCHAR) ansinum[i];
                    dst++;
                }
            }
            break;

            case 'I':
            {
                if ((src[1] != '6') || (src[2] != '4'))
                {
                    debugf(TEXT("Unknown percent in UnUnix.cpp::wvsnprintf()."));
                    unicode_str_to_stdout(fmt);
                    appDebugBreak();
                    src++;  // skip it, I guess.
                    break;
                }

                src += 4;
                QWORD val = va_arg(args, QWORD);
                ANSICHAR ansinum[60];
                ANSICHAR fmtbuf[30];
                strcpy(fmtbuf, "%L");
                percent_ptr += 4;

                // Yes, this is lame.
                INT cpyidx = 2;
                while (percent_ptr < src)
                {
                    fmtbuf[cpyidx] = (ANSICHAR) *percent_ptr;
                    percent_ptr++;
                    cpyidx++;
                }
                fmtbuf[cpyidx] = 0;

                int rc = snprintf(ansinum, sizeof (ansinum), fmtbuf, val);
                if ((dst + rc) > enddst)
                    rc = enddst - dst;
                for (int i = 0; i < rc; i++)
                {
                    *dst = (TCHAR) ansinum[i];
                    dst++;
                }
            }
            break;

            case 'f':
            {
                src++;
                double val = va_arg(args, double);
                ANSICHAR ansinum[30];
                ANSICHAR fmtbuf[30];

                // Yes, this is lame.
                INT cpyidx = 0;
                while (percent_ptr < src)
                {
                    fmtbuf[cpyidx] = (ANSICHAR) *percent_ptr;
                    percent_ptr++;
                    cpyidx++;
                }
                fmtbuf[cpyidx] = 0;

                int rc = snprintf(ansinum, sizeof (ansinum), fmtbuf, val);
                if ((dst + rc) > enddst)
                    rc = enddst - dst;
                for (int i = 0; i < rc; i++)
                {
                    *dst = (TCHAR) ansinum[i];
                    dst++;
                }
            }
            break;

            case 's':
            {
                src++;
                TCHAR *val = va_arg(args, TCHAR *);
                if (val == NULL)
                    val = TEXT("(null)");

                int rc = wcslen(val);
                if ((dst + rc) > enddst)
                    rc = enddst - dst;
                for (int i = 0; i < rc; i++)
                {
                    *dst = *val;
                    dst++;
                    val++;
                }
            }
            break;

            default:
                src++;  // skip char, I guess.
                debugf(TEXT("Unknown percent in UnUnix.cpp::wvsnprintf()."));
                unicode_str_to_stdout(fmt);
                appDebugBreak();
                break;
        }
    }

    *dst = 0;  // null terminate the new string.
    return(dst - buf);
}


CORE_API INT wprintf( const TCHAR* fmt, ... )
{
    va_list args;
    TCHAR buf[1024];  // (*shrug*)

    va_start(args, fmt);
    INT rc = wvsnprintf(buf, sizeof (buf) / sizeof (TCHAR), fmt, args);
    va_end(args);

    if (GUnicodeOS)
        fwrite(buf, sizeof (TCHAR), rc, stdout);
    else
        fwrite(appToAnsi(buf), sizeof (ANSICHAR), rc, stdout);

    return(rc);
}

CORE_API INT iswspace( UNICHAR ch  )
{
    return((INT) isspace((int) ch));
}

CORE_API TCHAR *_itow( const INT Num, TCHAR *Buffer, const INT BufSize )
{
    if ((Buffer == NULL) || (BufSize <= 0))
        return(Buffer);

    char *ansistr = (char *) appAlloca(BufSize);
    if (ansistr == NULL)
        return(NULL);  // !!! FIXME: Is this right?

    snprintf(ansistr, BufSize, "%d", Num);
    return(unixToUNICODE(Buffer, ansistr));
}

CORE_API QWORD _wcstoui64( const TCHAR* Start, TCHAR** End, INT Base )
{
    check((Base >= 2) && (Base <= 36));

    while (*Start == ' ')
        Start++;

    const TCHAR *ptr = Start;
    while (1)
    {
        char ch = toupper((char) *ptr);

        if ((ch >= 'A') && (ch <= 'Z'))
        {
            if ((Base <= 10) || (ch >= ('A' + Base)))
                break;
        }

        else if ((ch >= '0') && (ch <= '9'))
        {
            if (ch >= ('0' + Base))
                break;
        }

        else
            break;

        ptr++;
    }

    if (End != NULL)
        *End = const_cast<TCHAR *>(ptr);

    QWORD retval = 0;
    QWORD accumulator = 1;
    while (--ptr >= Start)
    {
        char ch = toupper((char) *ptr);
        QWORD val = (((ch >= 'A') && (ch <= 'Z')) ? (ch - 'A') + 10 : ch - '0');
        retval += val * accumulator;
        accumulator *= (QWORD) Base;
    }

    return(retval);
}

CORE_API TCHAR* _ui64tow( QWORD Num, TCHAR *Buffer, INT Base )
{
    check((Base >= 2) && (Base <= 36));

    int bufidx = 1;
    for (QWORD x = Num; x >= Base; x /= Base)
        bufidx++;

    Buffer[bufidx] = 0;  /* null terminate where the end will be. */

    while (--bufidx >= 0)
    {
        int val = (int) (Num % Base);
        Buffer[bufidx] = ((val > 9) ? ('a' + (val - 10)) : ('0' + val));
        Num /= Base;
    }

    return(Buffer);
}


CORE_API FLOAT _wtof( const TCHAR* Str )
{
    return((FLOAT) atof(appToAnsi(Str)));
}

#endif


/* visual C++ compatibility. --ryan. */
char *strlwr(char *str)
{
    for (char *p = str; *p; p++)
        *p = (char) tolower((int) *p);

    return(str);
}


CORE_API const UBOOL appMsgf( INT Type, const TCHAR* Fmt, ... )
{
	TCHAR TempStr[4096]=TEXT("");
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt, Fmt );
    printf("appMsgf(): %s\n", TCHAR_TO_ANSI(TempStr));
    appDebugBreak();
    return 1;
}



// Interface for recording loading errors in the editor
CORE_API const void EdClearLoadErrors()
{
	GEdLoadErrors.Empty();
}


/*-----------------------------------------------------------------------------
	USystem.
-----------------------------------------------------------------------------*/

//
// System manager.
//
static void Recurse()
{
	guard(Recurse);
	Recurse();
	unguard;
}
USystem::USystem()
:	SavePath	( E_NoInit )
,	CachePath	( E_NoInit )
,	CacheExt	( E_NoInit )
,	Paths		( E_NoInit )
,	Suppress	( E_NoInit )
{}
void USystem::StaticConstructor()
{
	guard(USystem::StaticConstructor);

	new(GetClass(),TEXT("PurgeCacheDays"),      RF_Public)UIntProperty   (CPP_PROPERTY(PurgeCacheDays     ), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("SavePath"),            RF_Public)UStrProperty   (CPP_PROPERTY(SavePath           ), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("CachePath"),           RF_Public)UStrProperty   (CPP_PROPERTY(CachePath          ), TEXT("Options"), CPF_Config );
	new(GetClass(),TEXT("CacheExt"),            RF_Public)UStrProperty   (CPP_PROPERTY(CacheExt           ), TEXT("Options"), CPF_Config );

	UArrayProperty* A = new(GetClass(),TEXT("Paths"),RF_Public)UArrayProperty( CPP_PROPERTY(Paths), TEXT("Options"), CPF_Config );
	A->Inner = new(A,TEXT("StrProperty0"),RF_Public)UStrProperty;

	UArrayProperty* B = new(GetClass(),TEXT("Suppress"),RF_Public)UArrayProperty( CPP_PROPERTY(Suppress), TEXT("Options"), CPF_Config );
	B->Inner = new(B,TEXT("NameProperty0"),RF_Public)UNameProperty;

	unguard;
}

static bool FindBinaryInPath(FString &cmd);

UBOOL USystem::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	if( ParseCommand(&Cmd,TEXT("MEMSTAT")) )
	{
		//!UNIX No MEMSTAT command.
		Ar.Logf( TEXT("MEMSTAT command not available.") );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("EXIT")) )
	{
		Ar.Log( TEXT("Closing by request") );
		appRequestExit( 0 );
		return 1;
	}
	else if( ParseCommand(&Cmd,TEXT("APP")) )
	{
		//!UNIX No APP command.
		Ar.Logf( TEXT("APP command not available.") );
		return 1;
	}
	else if( ParseCommand( &Cmd, TEXT("RELAUNCH") ) )
	{
		debugf( TEXT("Relaunch: %s"), Cmd );
		GConfig->Flush( 0 );

		// Fork out a new process using Cmd as arguments.
		TCHAR* EndArg0 = appStrstr( Cmd, TEXT(" ") );
		INT CmdLen = appStrlen( Cmd );
		INT RestLen = ((EndArg0 != NULL) ? appStrlen( EndArg0 ) : 0);
        #define RELAUNCH_ARRAY 1024
		TCHAR Arg0[RELAUNCH_ARRAY];
        INT max = (CmdLen - RestLen) + 1;
        if (max > RELAUNCH_ARRAY)
            max = RELAUNCH_ARRAY;
		appStrncpy( Arg0, Cmd, max );
        Arg0[RELAUNCH_ARRAY - 1] = 0;  // just in case.

#if 1
        INT arraylen = RELAUNCH_ARRAY + 32;
        FString xterm(TEXT("xterm"));
        if (FindBinaryInPath(xterm))
        {
            arraylen += xterm.Len();
            xterm += TEXT(" -e ");
        }
        else
        {
            xterm = TEXT("");
        }

        GUnixSpawnOnExit = new TCHAR[arraylen];
        appStrcpy(GUnixSpawnOnExit, *xterm);
        appStrcat(GUnixSpawnOnExit, TEXT("./ucc-bin server "));
        appStrcat(GUnixSpawnOnExit, Arg0);
        appRequestExit( 0 );
#else

        char ansistr[RELAUNCH_ARRAY];
        appToAnsi(Arg0, ansistr);

		if (fork() == 0)
		{
			appSleep(3);
			INT error = execl("./ucc-bin", "./ucc-bin", "server", ansistr, NULL );
			if (error == -1)
				appErrorf( TEXT("Failed to launch process.") );
		} else {
			appRequestExit( 0 );
		}
#endif
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
			Recurse();
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
	else return 0;
}
IMPLEMENT_CLASS(USystem);

/*-----------------------------------------------------------------------------
	Exit.
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
		exit( 1 );
	}
	else
	{
		// Tell the platform specific code we want to exit cleanly from the main loop.
		//!UNIX No quit message in UNIX.
		GIsRequestingExit = 1;
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

CORE_API void ClipboardCopy( const TCHAR* Str )
{
	guard(ClipboardCopy);
	//!UNIX Not supported in UNIX.
	unguard;
}

CORE_API void ClipboardPaste( FString& Result )
{
	guard(ClipboardPasteString);
	//!UNIX Not supported in UNIX.
	unguard;
}

/*-----------------------------------------------------------------------------
	Shared libraries.
-----------------------------------------------------------------------------*/

//
// Load a library.
//
void* appGetDllHandle( const TCHAR* Filename )
{
	guard(appGetDllHandle);

	#if __STATIC_LINK
		INT* FalseHandle;
		return (void*) FalseHandle;
	#endif
	check(Filename);
	const TCHAR* PackageName = Filename;
	TCHAR* Cur;
	TCHAR* Test = appStaticString1024();
	void* Result;
	const char* Error;

	// Get GLoadedPackage symbol name from full path.
 	while( (Cur = appStrchr( PackageName, '/' )) != NULL )
		PackageName = Cur + 1;
 	while( (Cur = appStrchr( PackageName, '\\' )) != NULL )
		PackageName = Cur + 1;
	appSprintf( Test, TEXT("GLoaded%s"), PackageName );
	if( (Cur = appStrchr( Test, '.' )) != NULL )
		*Cur = '\0';

	dlerror();	// Clear any error condition.

	// Check if the library was linked to the executable.
	Result = (void*)dlopen( NULL, RTLD_NOW | RTLD_GLOBAL );
	Error = dlerror();
	if( Error != NULL )
		debugf( "dlerror(): %s", Error );
	else
	{
		(void*)dlsym( Result, appToAnsi(Test) );
		Error = dlerror();
		if( Error == NULL )
			return Result;
	}

	// Load the new library.
	Result = (void*)dlopen( appToAnsi(Filename), RTLD_NOW | RTLD_GLOBAL );
	if( Result == NULL )
	{
		TCHAR Temp[256];
		appStrcpy( Temp, Filename );
		appStrcat( Temp, DLLEXT );
		Result = (void*)dlopen( appToAnsi(Temp), RTLD_NOW | RTLD_GLOBAL );
#ifdef LOOKING_FOR_UNDEFINED_SYMBOLS
		Error = dlerror();
		if( Error != NULL )
		debugf( "dlerror(): %s", Error );
#endif
	}

#if 1
    //printf("FIXME: Disable UNREAL_PAUSE_ON_DLOPEN before shipping!\n");
    const char *envr = getenv("UNREAL_PAUSE_ON_DLOPEN");
    if (envr)
    {
        const TCHAR *libname = ANSI_TO_TCHAR(envr);
        if (appStrfind(Filename, libname))
        {
            printf("%s loaded. Sleeping ten seconds...\n", envr);
            sleep(10);
        } /* if */
    } /* if */
#endif

	return Result;
	unguard;
}

//
// Free a library.
//
void appFreeDllHandle( void* DllHandle )
{
	guard(appFreeDllHandle);
	check(DllHandle);

	dlclose( DllHandle );

	unguard;
}

//
// Lookup the address of a shared library function.
//
void* appGetDllExport( void* DllHandle, const TCHAR* ProcName )
{
	guard(appGetDllExport);
	check(DllHandle);
	check(ProcName);

	void* Result;
	const char *Error;

	dlerror();	// Clear any error condition.

	Result = (void*)dlsym( DllHandle, appToAnsi(ProcName) );
	Error = dlerror();
	if( Error != NULL )
		debugf( "dlerror: %s", ANSI_TO_TCHAR(Error) );
	return Result;

	unguard;
}

//
// Break the debugger.
//
void appDebugBreak()
{
	guard(appDebugBreak);
#ifdef ASMLINUX
	asm("int $03");
#endif
	unguard;
}

//
// IsDebuggerPresent()
//
UBOOL appIsDebuggerPresent()
{
	// Does not need to be implemented.
	return 0;
}

/*-----------------------------------------------------------------------------
	External processes.
-----------------------------------------------------------------------------*/

static TMap<pid_t,int>* ExitCodeMap = NULL;

void* appCreateProc( const TCHAR* URL, const TCHAR* Parms )
{
	guard(appCreateProc);

	debugf( TEXT("Create Proc: %s %s"), URL, Parms );

	TCHAR LocalParms[PATH_MAX] = TEXT("");
	appStrcpy(LocalParms, URL);
	appStrcat(LocalParms, TEXT(" "));
	appStrcat(LocalParms, Parms);

	pid_t pid = fork();
	if (pid == 0)
		_exit( system( appToAnsi(LocalParms) ) );

	return (void*)pid;
	unguard;
}

UBOOL appGetProcReturnCode( void* ProcHandle, INT* ReturnCode )
{
	guard(appGetProcReturnCode);
	int* p = ExitCodeMap->Find( (pid_t)ProcHandle );
	if(p)
	{
		*ReturnCode = *p;
		ExitCodeMap->Remove( (pid_t)ProcHandle );
		return 1;
	}
	return 0;
	unguard;
}

/*-----------------------------------------------------------------------------
	Signal Handling.
-----------------------------------------------------------------------------*/

static void HandleChild(int Signal)
{
	int Status;
	pid_t pid;
	while( (pid=waitpid(-1, &Status, WNOHANG)) > 0 )
		ExitCodeMap->Set( pid, WEXITSTATUS(Status) );
}

static pid_t mainPID;
static INT AlreadyAborting	= 0;
static INT SignalExit		= 0;
static INT SignalCritical	= 0;

static void HandleSignal( int Signal )
{
	char* sig_text;
	switch (Signal)
	{
		case SIGHUP:
			sig_text = "Signal: SIGHUP [hangup]";
			SignalExit++;
			break;
		case SIGQUIT:
			sig_text = "Signal: SIGQUIT [quit]";
			SignalExit++;
			break;
		case SIGILL:
			sig_text = "Signal: SIGILL [illegal instruction]";
			SignalCritical++;
			break;
		case SIGTRAP:
			sig_text = "Signal: SIGTRAP [trap]";
			SignalCritical++;
			break;
		case SIGIOT:
			sig_text = "Signal: SIGIOT [iot trap]";
			SignalCritical++;
			break;
		case SIGBUS:
			sig_text = "Signal: SIGBUS [bus error]";
			SignalCritical++;
			break;
		case SIGFPE:
			sig_text = "Signal: SIGFPE [floating point exception]";
			SignalCritical++;
			break;
		case SIGSEGV:
			sig_text = "Signal: SIGSEGV [segmentation fault]";
			SignalCritical++;
			break;
		case SIGTERM:
			sig_text = "Signal: SIGTERM [terminate]";
			SignalExit++;
			break;
		case SIGINT:  // must have gone through HandleInterrupt previously.
			sig_text = "Signal: SIGINT [interrupt]";
			SignalCritical++;
			break;
		default:
			sig_text = "Signal: UNKNOWN SIGNAL [%i]";
			SignalCritical++;
			break;
	}

	if ( (SignalCritical > 0) || (SignalExit > 1) )
	{
		if (!AlreadyAborting)
		{
			void *aTrace[64];
			char **tString;
			int size, i;

			#if (defined __FreeBSD__)
			printf("!!! FIXME: No backtrace_symbols() on FreeBSD.\n");
			#elif (defined MACOSX)
			printf("!!! FIXME: No backtrace_symbols() on FreeBSD.\n");
			#else
			size = backtrace(aTrace, 64);
			tString = backtrace_symbols(aTrace, size);
			printf("\nBacktrace:\n");
			for (i = 0; i < size; i++)
				printf("[%2.i]  %s\n", i+1, tString[i]);
  			#endif

			// Avoid calling appExit again.
			AlreadyAborting = true;
			printf( "%s\n", sig_text );
			printf( "Aborting.\n" );
			appExit();
		}
		exit(1);
	}
	if ( SignalExit == 1 )
	{
		printf("%s\n", sig_text);
		printf("Requesting Exit.\n");
		appRequestExit( 0 );
	}
}


// special case for CTRL-C.  --ryan.
static void HandleInterrupt( int Signal )
{
    check(Signal==SIGINT);

    if (getpid() == mainPID)
    {
        static bool alreadyTriggered = false;

        if (alreadyTriggered)
        {
            // User is mashing CTRL-C, or appRequestExit() work is deadlocked.
            // Pass SIGINT to our standard interrupt handler for panic crash.
            HandleSignal(Signal);
        }

        alreadyTriggered = true;

        if (GIsRunning)   // already in main loop.
        {
            // let the engine try to shut down cleanly.
            debugf(TEXT("CTRL-C has been pressed.\n"));
            appRequestExit(0);
        }

        else
        {
            // Need to force an immediate termination with _exit(),
            //  since things with static destructors may not be initialized
            //  yet! Otherwise, this could cause a confusing segfault in
            //  the normal exit() processing. Please note that calling
            //  appRequestExit(1) will result in a normal exit() call.
            printf("CTRL-C before main loop ... forcing exit.\n");
            _exit(42);  // all threads should die.
        }
    }
}


/*-----------------------------------------------------------------------------
	Timing.
-----------------------------------------------------------------------------*/

//
// String timestamp.
//
CORE_API const TCHAR* appTimestamp()
{
	guard(appTimestamp);

	TCHAR* 		Result = appStaticString1024();
	time_t		CurTime;
	struct tm*	SysTime;

	CurTime = time( NULL );
	SysTime = localtime( &CurTime );
	appSprintf( Result, ANSI_TO_TCHAR(asctime( SysTime )) );
	// Strip newline.
	Result[appStrlen( Result )-1] = '\0';
	return Result;

	unguard;
}

//
// Get file time.
//
CORE_API DWORD appGetTime( const TCHAR* Filename )
{
	guard(appGetTime);

	struct utimbuf FileTime;
	if( utime(TCHAR_TO_ANSI(Filename),&FileTime)!=0 )
		return 0;
	//FIXME Is this the right "time" to return?
	return (DWORD)FileTime.modtime;

	unguard;
}

//
// Get time in seconds.
//
CORE_API DOUBLE appSecondsSlow()
{
#if 1
	static DOUBLE InitialTime = 0.0;
	static DOUBLE TimeCounter = 0.0;
	DOUBLE NewTime;
	struct timeval TimeOfDay;

	gettimeofday( &TimeOfDay, NULL );

	// Initialize.
	if( InitialTime == 0.0 )
		 InitialTime = TimeOfDay.tv_sec + TimeOfDay.tv_usec / 1000000.0;

	// Accumulate difference to prevent wraparound.
	NewTime = TimeOfDay.tv_sec + TimeOfDay.tv_usec / 1000000.0;
	TimeCounter += NewTime - InitialTime;
	InitialTime = NewTime;

	return TimeCounter;
#else
	return appSeconds();
#endif
}

#if !DEFINED_appCycles
CORE_API DWORD appCycles()
{
	return (DWORD)appSeconds();
}
#endif


//
// Get time in seconds. Origin is arbitrary.
//
#if !DEFINED_appSeconds
CORE_API DOUBLE appSeconds()
{
	return appSecondsSlow();
}
#endif

//
// Return the system time.
//
CORE_API void appSystemTime( INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec )
{
	guard(appSystemTime);

	time_t			CurTime;
	struct tm		*St;		// System time.
	struct timeval	Tv;			// Use timeval to get milliseconds.

	gettimeofday( &Tv, NULL );
	CurTime = time( NULL );
	St = localtime( &CurTime );

	Year		= St->tm_year + 1900;
	Month		= St->tm_mon + 1;
	DayOfWeek	= St->tm_wday;
	Day			= St->tm_mday;
	Hour		= St->tm_hour;
	Min			= St->tm_min;
	Sec			= St->tm_sec;
	MSec		= (INT) (Tv.tv_usec / 1000);

	unguard;
}

CORE_API void appSleep( FLOAT Seconds )
{
	guard(appSleep);

	INT SleepTime = (INT) (Seconds * 1000000);
	usleep( SleepTime );

	unguard;
}

/*-----------------------------------------------------------------------------
	Link functions.
-----------------------------------------------------------------------------*/

static bool FindBinaryInPath(FString &cmd)
{
    INT binpos = cmd.InStr(TEXT(" "));
    FString bin((binpos == -1) ? cmd : cmd.Left(binpos));

    if (bin.Len() == 0)
        return(false);

    if (bin.InStr(TEXT("/")) != -1)  // path specified; don't examine $PATH.
        return(access(appToAnsi(*bin), X_OK) != -1);

    const char *_envr = getenv("PATH");
    if (_envr == NULL)
    {
        debugf(TEXT("No $PATH environment var set"));
        return(false);
    }

    FString envr(_envr);

    while (true)
    {
        INT pos = envr.InStr(TEXT(":"));
        FString path((pos == -1) ? envr : envr.Left(pos));
        path += TEXT("/");

        FString binpath = path;
        binpath += bin;

        char realized[MAXPATHLEN];
        if (realpath(appToAnsi(*binpath), realized) != NULL)
        {
            struct stat statbuf;
            if (stat(realized, &statbuf) != -1)
            {
                /* dirs are usually "executable", but aren't programs. */
                if (!S_ISDIR(statbuf.st_mode))
                {
                    if (access(realized, X_OK) != -1)
                    {
                        cmd = FString(realized) + cmd.Mid(binpos);
                        return(true);
                    }
                }
            }
        }

        if (pos == -1)
            break;
        envr = envr.Mid(pos + 1);
    }

    return(false);
}


//
// Launch a uniform resource locator (i.e. http://www.epicgames.com/unreal).
// This is expected to return immediately as the URL is launched by another
// task.
//
void appLaunchURL( const TCHAR* URL, const TCHAR* Parms, FString* Error )
{
	guard(appLaunchURL);

    const char *_envr = getenv("BROWSER");
    if (!_envr)
    {
        debugf(TEXT("Tried to view URL but BROWSER environment var isn't set!"));
        debugf(TEXT(" URL to view was [%s]."), URL);
        debugf(TEXT(" Trying some standard browsers..."), URL);
        //_envr = "opera:netscape -raise -remote \"openURL(%s,new-window)\":mozilla:konqueror:galeon:xterm -e links:xterm -e lynx:";
        _envr = "opera:mozilla:galeon:konqueror:netscape:xterm -e links:xterm -e lynx:";
    }

    FString envr(appFromAnsi(_envr));

    bool foundBrowser = false;

    while (true)
    {
        INT pos = envr.InStr(TEXT(":"));
        FString cmd((pos == -1) ? envr : envr.Left(pos));
        INT had_percent_s = false;
        INT percentpos = 0;
        while (percentpos < cmd.Len() - 1)
        {
            percentpos++;
            if (cmd[percentpos - 1] != '%')
                continue;

            // need to deal with a '%' sequence...

            if (cmd[percentpos] == '%')
            {
                cmd = cmd.Left(percentpos) + cmd.Mid(percentpos + 1);
            }
            else if (cmd[percentpos] == 's')
            {
                cmd = cmd.Left(percentpos - 1) + URL + cmd.Mid(percentpos + 1);
                percentpos += appStrlen(URL);
                had_percent_s = true;
            }
            else
            {
                debugf(TEXT("Unrecognized percent sequence (%%%c) in BROWSER string."), (TCHAR) cmd[percentpos]);
                percentpos++;
            }
        }

        if (!had_percent_s)
        {
            cmd += TEXT(" ");
            cmd += URL;
        }

        if (FindBinaryInPath(cmd))
        {
            foundBrowser = true;
            debugf(TEXT("Spawning browser: %s"), *cmd);
            GUnixSpawnOnExit = new TCHAR[cmd.Len() + 1];
            appStrcpy(GUnixSpawnOnExit, *cmd);
            break;
        }

        if (pos == -1)
            break;
        envr = envr.Mid(pos + 1);
    }

    if (!foundBrowser)
    {
        debugf("Couldn't find any browser to launch!");
        debugf("Please set the environment variable BROWSER next time.");

        fprintf(stderr, "Couldn't find any browser to launch!\n");
        fprintf(stderr, "Please set the environment variable BROWSER next time.\n");
    }

    appRequestExit(0);
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
	struct stat Buf;
	INT Result = 0;

	Result = stat(TCHAR_TO_ANSI(Filename),&Buf);
	if( Result==0 )
	{
		time_t CurrentTime, FileTime;
		FileTime = Buf.st_mtime;
		time( &CurrentTime );
		DOUBLE DiffSeconds = difftime( CurrentTime, FileTime );
		return (INT)(DiffSeconds / 60.0 / 60.0 / 24.0);
	}
	return 0;
	unguard;
}

CORE_API void appCleanFileCache()
{
	guard(appCleanFileCache);

	// Delete all temporary files.
	guard(DeleteTemps);
	FString Temp = FString::Printf( TEXT("%s%s*.tmp"), *GSys->CachePath, PATH_SEPARATOR );
	TArray<FString> Found = GFileManager->FindFiles( *Temp, 1, 0 );
	for( INT i=0; i<Found.Num(); i++ )
	{
		Temp = FString::Printf( TEXT("%s%s%s"), *GSys->CachePath, PATH_SEPARATOR, *Found(i) );
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
			FString Temp = FString::Printf( TEXT("%s%s%s"), *GSys->CachePath, PATH_SEPARATOR, *Found(i) );
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
	Guids.
-----------------------------------------------------------------------------*/

//
// Create a new globally unique identifier.
//
CORE_API FGuid appCreateGuid()
{
	guard(appCreateGuid);

	FGuid Result;
	appGetGUID( (void*)&Result );
	return Result;

	unguard;
}

/*-----------------------------------------------------------------------------
	Clipboard
-----------------------------------------------------------------------------*/
static FString ClipboardText;
CORE_API void appClipboardCopy( const TCHAR* Str )
{
	guard(appClipboardCopy);
	ClipboardText = FString( Str );
	unguard;
}

CORE_API FString appClipboardPaste()
{
	guard(appClipboardPaste);
	return ClipboardText;
	unguard;
}
/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

// Get startup directory.
CORE_API const TCHAR* appBaseDir()
{
	guard(appBaseDir);
	static TCHAR BaseDir[PATH_MAX]=TEXT("");
    ANSICHAR AnsiBaseDir[PATH_MAX];

	if( !BaseDir[0] )
	{
		// If the executable isn't launched from its own directory, then the
		// dynamic linker won't be able to find the shared libraries.
		getcwd( AnsiBaseDir, sizeof(AnsiBaseDir) );
        appStrcpy( BaseDir, ANSI_TO_TCHAR(AnsiBaseDir));
		appStrcat( BaseDir, TEXT("/") );
	}
	return BaseDir;
	unguard;
}

// Get computer name.
CORE_API const TCHAR* appComputerName()
{
	guard(appComputerName);
	static TCHAR Result[256]=TEXT("");
    static ANSICHAR AnsiResult[256];
	if( !Result[0] )
    {
		gethostname( AnsiResult, sizeof(AnsiResult) );
        appStrcpy( Result, ANSI_TO_TCHAR(AnsiResult));
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
        ANSICHAR *str = getlogin();
		appStrncpy( Result, ANSI_TO_TCHAR(str), sizeof(Result) );
    }
	return Result;
	unguard;
}

// Get launch package base name.
CORE_API const TCHAR* appPackage()
{
	guard(appPackage);
    return(appStrcpy(appStaticString1024(), ANSI_TO_TCHAR(GModule)));
	unguard;
}

/*-----------------------------------------------------------------------------
	App init/exit.
-----------------------------------------------------------------------------*/

//
// Platform specific initialization.
//
void appPlatformPreInit()
{
#if UNICODE
    unixDetectUNICODE();
#endif

#if __LINUX_X86__
    GRunningOS = OS_LINUX_X86;
#elif (defined __FreeBSD__)
    #error determine if x86 or Alpha or whatnot...
    GRunningOS = OS_FREEBSD_X86;
#elif (defined MACOSX)
    GRunningOS = OS_MAC_OSX_PPC;
#else
    // (more to come. --ryan.)
    #error fill in your platform.
#endif

    mainPID = getpid();
}

#if (!defined USE_FILESTREAM_THREAD)

FCriticalSection::FCriticalSection() {}
FCriticalSection::~FCriticalSection() {}
void FCriticalSection::Lock() {}
void FCriticalSection::Unlock() [}
FScopeCriticalSection::FScopeCriticalSection( FCriticalSection* InCriticalSection ) {}
FScopeCriticalSection::~FScopeCriticalSection() {}

#else

FCriticalSection::FCriticalSection()
{
	guard(FCriticalSection::FCriticalSection);
	Handle = new pthread_mutex_t;
    int rc = pthread_mutex_init((pthread_mutex_t *) Handle, NULL);
    check(rc == 0);
	unguard;
}

FCriticalSection::~FCriticalSection()
{
	guard(FCriticalSection::~FCriticalSection);
    int rc = pthread_mutex_destroy((pthread_mutex_t *) Handle);
    check(rc == 0);
	delete (pthread_mutex_t *) Handle;
	unguard;
}

void FCriticalSection::Lock()
{
	int rc = pthread_mutex_lock((pthread_mutex_t *) Handle);
    check(rc == 0);
}

void FCriticalSection::Unlock()
{
	int rc = pthread_mutex_unlock((pthread_mutex_t *) Handle);
    check(rc == 0);
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


static pthread_t FileStreamingThreadId = 0;

static void *FileStreamingThread( void* Arg )
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
		usleep(1000);
	}
	for (INT i=0; i<GFileStream->MaxStreams; i++)
	{
		FScopeCriticalSection Lock( GFileStream->CriticalSection );

		if (GFileStream->Streams[i].Handle)
			GFileStream->Destroy(i);
	}
	GFileStream->Destroyed = 2;
	return NULL;
}
#endif


static void DoCPUID( int i, DWORD *A, DWORD *B, DWORD *C, DWORD *D )
{
// the very definition of gay: EBX is used by the caller,
//  who won't save it, and won't acknoledge we're clobbering it.
//  Do a pushall/popall, just in case.
#if __LINUX_X86__
    DWORD buf[4];

 	__asm__ __volatile__ (
        "pushl %%eax           \n\n"
        "pushl %%ebx           \n\n"
        "pushl %%ecx           \n\n"
        "pushl %%edx           \n\n"
        "cpuid                  \n\t"
        "movl  %%eax,  0(%%edi) \n\t"
        "movl  %%ebx,  4(%%edi) \n\t"
        "movl  %%ecx,  8(%%edi) \n\t"
        "movl  %%edx, 12(%%edi) \n\t"
        "popl  %%edx           \n\n"
        "popl  %%ecx           \n\n"
        "popl  %%ebx           \n\n"
        "popl  %%eax           \n\n"
            : /* output to memory directly. */
            : "a" (i), "D" (buf)
            : "memory"
    );

    *A = buf[0];
    *B = buf[1];
    *C = buf[2];
    *D = buf[3];
#else
	*A=*B=*C=*D=0;
#endif
}


typedef struct
{
    DWORD HighPart;
    DWORD LowPart;
} LARGE_INTEGER;

void appPlatformInit()
{
	guard(appPlatformInit);

    //printf("GRunningOS is (%d). ENGINE_VERSION is (%d).\n", (int) GRunningOS, (int) ENGINE_VERSION);

	// System initialization.
	GSys = new USystem;
	GSys->AddToRoot();
	for( INT i=0; i<GSys->Suppress.Num(); i++ )
		GSys->Suppress(i).SetFlags( RF_Suppress );

	// Randomize.
	srand( (unsigned)time( NULL ) );

	// Exit code handling
	ExitCodeMap = new TMap<pid_t,int>;

	GStartCycles = FCycles();

	// Check processor version with CPUID.
	try
	{
		TCHAR Brand[13], *Model, FeatStr[256]=TEXT("");
		DWORD A=0, B=0, C=0, D=0;
		DoCPUID(0,&A,&B,&C,&D);
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
		if( (D & 0x00008000) ) {appStrcat( FeatStr, TEXT(" CMov") ); GIsPentiumPro=1; }
		if( (D & 0x00000001) ) {appStrcat( FeatStr, TEXT(" FPU") );}
		if( (D & 0x00000010) ) {appStrcat( FeatStr, TEXT(" RDTSC") );}
		if( (D & 0x00000040) ) {appStrcat( FeatStr, TEXT(" PAE") );}
		if( (D & 0x00800000) && !ParseParam(appCmdLine(),TEXT("NOMMX")) ) {appStrcat( FeatStr, TEXT(" MMX") ); GIsMMX=1;}
		if( (D & 0x02000000) && !ParseParam(appCmdLine(),TEXT("NOSSE")) ) {appStrcat( FeatStr, TEXT(" SSE") ); GIsSSE=1;}

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

	// CPU speed.
	guard(CheckCpuSpeed);
	FLOAT CpuSpeed;
#if ASMLINUX
	try
	{
		GSecondsPerCycle = 1.f;

		for( INT i=0; i<3; i++ )
		{
			LARGE_INTEGER	StartCycles,
							EndCycles;
			volatile DOUBLE	DoubleDummy = 0.0;
			volatile INT	IntDummy	= 0;
			__asm__ __volatile__ (
                "rdtsc"
                    : "=a" (StartCycles.LowPart), "=d" (StartCycles.HighPart)
            );

			struct timeval	StartMsec;
            gettimeofday(&StartMsec, NULL);
			struct timeval EndMsec		= StartMsec;

            int diff = (((EndMsec.tv_sec - StartMsec.tv_sec) * 1000) +
                        ((EndMsec.tv_usec - StartMsec.tv_usec) / 1000));
			while( diff < 200 )
			{
				DoubleDummy += appSqrt(DoubleDummy) + 3.14;
				IntDummy	*= ((INT) (DoubleDummy) + 555) / 13;
				gettimeofday(&EndMsec, NULL);
                diff = (((EndMsec.tv_sec - StartMsec.tv_sec) * 1000) +
                        ((EndMsec.tv_usec - StartMsec.tv_usec) / 1000));
			}

			__asm__ __volatile__ (
                "rdtsc"
                    : "=a" (EndCycles.LowPart), "=d" (EndCycles.HighPart)
            );

			DOUBLE	C1	= (DOUBLE)(SQWORD)(((QWORD)StartCycles.LowPart) + ((QWORD)StartCycles.HighPart<<32)),
					C2	= (DOUBLE)(SQWORD)(((QWORD)EndCycles.LowPart) + ((QWORD)EndCycles.HighPart<<32));

			GSecondsPerCycle = Min( GSecondsPerCycle, 1.0 / (1000.f * ( C2 - C1 ) / (diff)) );
		}

		debugf( NAME_Init, TEXT("CPU Speed=%f MHz"), 0.000001 / GSecondsPerCycle );
        appStrcat( GMachineCPU, *FString::Printf(TEXT(" @ %d MHz"), appRound(0.000001 / GSecondsPerCycle) ) );
	}
	catch( ... )
	{
		debugf( NAME_Init, TEXT("Timestamp not supported (Possibly 486 or Cyrix processor)") );
        appStrcat( GMachineCPU, TEXT(" Unknown clock") );
		GSecondsPerCycle = 1;
	}

#elif (defined MACOSX)
	int mhz = GetCPUSpeed();  // (Carbon function.)
	GSecondsPerCycle = 0.000001 * ((float) mhz);
	debugf( NAME_Init, TEXT("CPU Speed=%d MHz"), (int) mhz);
        appStrcat( GMachineCPU, *FString::Printf(TEXT(" @ %d MHz"), (int) mhz) );

#else
	if( !Parse(appCmdLine(),TEXT("CPUSPEED="),CpuSpeed) )
	{
		#warning need CPU speed detection code.
		appErrorf( TEXT("Please specify CPUSPEED=x on cmdline.") );
	}
#endif

	if( Parse(appCmdLine(),TEXT("CPUSPEED="),CpuSpeed) )
	{
		GSecondsPerCycle = 0.000001/CpuSpeed;
		debugf( NAME_Init, TEXT("CPU Speed Overridden=%f MHz"), 0.000001 / GSecondsPerCycle );
        appStrcat( GMachineCPU, *FString::Printf(TEXT(" @ %d MHz"), appRound(0.000001 / GSecondsPerCycle) ) );
	}
	unguard;

	struct sigaction SigAction;

	// Only try once.
	INT DefaultFlag			= SA_RESETHAND;

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGSEGV, &SigAction, 0 );

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGILL, &SigAction, 0 );

	// Try multiple times.
	DefaultFlag				= 0;

	SigAction.sa_handler	= HandleChild;
	SigAction.sa_flags		= 0;
	sigemptyset( &SigAction.sa_mask );
	sigaction(SIGCHLD, &SigAction, 0);

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGHUP, &SigAction, 0 );

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGQUIT, &SigAction, 0 );

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGTRAP, &SigAction, 0 );

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGIOT, &SigAction, 0 );

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGBUS, &SigAction, 0 );

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGFPE, &SigAction, 0 );

	SigAction.sa_handler	= HandleSignal;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGTERM, &SigAction, 0 );

	SigAction.sa_handler	= HandleInterrupt;
	SigAction.sa_flags		= DefaultFlag;
	sigemptyset( &SigAction.sa_mask );
	sigaction( SIGINT, &SigAction, 0 );

	signal( SIGPIPE, SIG_IGN );

#ifdef USE_FILESTREAM_THREAD
	int rc = pthread_create(&FileStreamingThreadId, NULL,
                            FileStreamingThread, NULL);
	check(rc == 0);
    if (rc == 0)
        pthread_detach(FileStreamingThreadId);
#endif

	unguard;
}

void appPlatformPreExit()
{
}

void appPlatformExit()
{
    if (GUnixSpawnOnExit != NULL)
    {
        FString fstrcmd(GUnixSpawnOnExit);
        TArray<FString> args;
        fstrcmd.ParseIntoArray(TEXT(" "), &args);
        char *cmd = new char[args(0).Len() + 1];
        appToAnsi(*(args(0)), cmd);
        char **ansiargs = new char *[args.Num() + 1];
        for (int i = 0; i < args.Num(); i++)
        {
            ansiargs[i] = new char[args(i).Len() + 1];
            appToAnsi(*args(i), ansiargs[i]);

            // This is a total hack, but then again, so is GUnixSpawnOnExit.
            char *str = ansiargs[i];
            if (*str == '\"')
                memmove(str, str + 1, strlen(str));
            if (str[strlen(str) - 1] == '\"')
                str[strlen(str) - 1] = '\0';
        }
        ansiargs[args.Num()] = NULL;
        execv(cmd, ansiargs);  // hail mary.

        // if you hit this...there were problems.
        fprintf(stderr, "execv() failed. Command line:\n   ");
        for (int x = 0; ansiargs[x]; x++)
            fprintf(stderr, "%s ", ansiargs[x]);
        fprintf(stderr, "\n\n");
    }
}

/*-----------------------------------------------------------------------------
	Pathnames.
-----------------------------------------------------------------------------*/

#if __FILE_NOCASE
// See if file specified by (Path) exists with a different case. This
//  assumes that the directories specified exist with the given case, and
//  only looks at the file part of the path. The first file found via
//  readdir() is the one picked, which may not be the first in ASCII order.
//  If a file is found, (Path) is overwritten to reflect the new case, and
//  (true) is returned. If no file is found, (Path) is untouched, and (false)
//  is returned.
static bool find_alternate_filecase(char *Path)
{
    guard(find_alternate_filecase);

    bool retval = false;
    char *ptr = strrchr(Path, '/');
    char *filename = (ptr != NULL) ? ptr + 1 : Path;
    char *basedir = (ptr != NULL) ? Path : ((char *) ".");

    if (ptr != NULL)
        *ptr = '\0';  // separate dir and filename.

    // fast rejection: only check this if there's no wildcard in filename.
    if (strchr(filename, '*') == NULL)
    {
        DIR *dir = opendir(basedir);
        if (dir != NULL)
        {
            struct dirent *ent;
            while (((ent = readdir(dir)) != NULL) && (!retval))
            {
                if (strcasecmp(ent->d_name, filename) == 0)  // a match?
                {
                    strcpy(filename, ent->d_name);  // overwrite with new case.
                    retval = true;
                }
            }
            closedir(dir);
        }
    }

    if (ptr != NULL)
        *ptr = '/';  // recombine dir and filename into one path.

    return(retval);

    unguard;
}
#endif


// Convert pathname to Unix format.
char* appUnixPath( const char* Path )
{
	guard(appUnixPath);
	static char UnixPath[1024];
	char* Cur = UnixPath;
	strncpy( UnixPath, Path, 1024 );
    UnixPath[1023] = '\0';

	while( Cur = strchr( Cur, '\\' ) )
		*Cur = '/';

#if __FILE_NOCASE
    // fast rejection: only check this if the current path doesn't exist...
    if (access(UnixPath, F_OK) != 0)
    {
        Cur = UnixPath;
        if (*Cur == '/')
            Cur++;

        bool keep_looking = true;
        while (keep_looking)
        {
            Cur = strchr( Cur, '/' );
            if (Cur != NULL)
                *Cur = '\0';  // null-terminate so we have this chunk of path.

            // do insensitive check only if the current path doesn't exist...
            if (access(UnixPath, F_OK) != 0)
                keep_looking = find_alternate_filecase(UnixPath);

            if (Cur == NULL)
                keep_looking = false;
            else
            {
                *Cur = '/';   // reset string for next check.
                Cur++;
            }
        }
    }
#endif

	return UnixPath;
	unguard;
}


/*-----------------------------------------------------------------------------
	Networking.
-----------------------------------------------------------------------------*/

unsigned long appGetLocalIP( void )
{
    static bool lookedUp = false;
	static unsigned long LocalIP = 0;

	if( !lookedUp )
	{
        lookedUp = true;
	    char Hostname[256];
        Hostname[0] = '\0';
		if (gethostname( Hostname, sizeof(Hostname) ) == 0)
        {
			check(gethostbyname_mutex != NULL);
			gethostbyname_mutex->Lock();
	        struct hostent *Hostinfo = gethostbyname( Hostname );
            if (Hostinfo != NULL)
    		    LocalIP = *(unsigned long*)Hostinfo->h_addr_list[0];
			gethostbyname_mutex->Unlock();
        }
	}

	return LocalIP;
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

int stricmp( const char* s, const char* t )
{
	int	i;
	for( i = 0; tolower(s[i]) == tolower(t[i]); i++ )
		if( s[i] == '\0' )
			return 0;
	return s[i] - t[i];
}

int strnicmp( const char* s, const char* t, int n )
{
	int	i;
	if( n <= 0 )
		return 0;
	for( i = 0; tolower(s[i]) == tolower(t[i]); i++ )
		if( (s[i] == '\0') || (i == n - 1) )
			return 0;
	return s[i] - t[i];
}

char* strupr( char* s )
{
	int	i;
	for( i = 0; s[i] != '\0'; i++ )
		s[i] = toupper(s[i]);
	return s;
}

/*-----------------------------------------------------------------------------
	File Streaming.
-----------------------------------------------------------------------------*/

UBOOL FFileStream::Read( INT StreamId, INT Bytes )
{
	if ( Streams[StreamId].Handle && Streams[StreamId].Data )
	{
		switch ( Streams[StreamId].Type )
		{
			case ST_Regular:
			{
                FILE *fp = (FILE *) Streams[StreamId].Handle;
				UBOOL RetVal = fread( Streams[StreamId].Data, 1, Bytes, fp );
				if (RetVal > 0)
					Streams[StreamId].Data = (BYTE*)(Streams[StreamId].Data) + RetVal;
				if (feof(fp))
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
                            // !!! FIXME: This might be incorrect on a non-blocking file handle. --ryan.
							Streams[StreamId].EndOfFile = 1;
							return false;
						}
					}
					else if ( Read < 0 )
					{
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

  	return false;
}

UBOOL FFileStream::Create( INT StreamId, const TCHAR* Filename )
{
    char *unixpath = appUnixPath(appToAnsi(Filename));
	switch ( Streams[StreamId].Type )
	{
		case ST_Regular:
		{
            FILE *Handle = fopen(unixpath, "rb");
			if( Handle == NULL )
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
			void* Handle = fopen(unixpath, "rb");
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

    return false;
}

UBOOL FFileStream::Destroy( INT StreamId )
{
	switch ( Streams[StreamId].Type )
	{
		case ST_Regular:
		{
			if ( Streams[StreamId].Handle )
				fclose( (FILE *) Streams[StreamId].Handle );
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

    return false;
}


/*-----------------------------------------------------------------------------
	Stuff.
-----------------------------------------------------------------------------*/

CORE_API const void EdLoadErrorf( INT Type, const TCHAR* Fmt, ... )
{ }


#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

