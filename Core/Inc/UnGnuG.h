/*=============================================================================
	UnGnuG.h: Unreal definitions for Gnu G++. Unfinished. Unsupported.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*----------------------------------------------------------------------------
	Platform compiler definitions.
----------------------------------------------------------------------------*/
#ifndef _INCL_UNGNUG_H_
#define _INCL_UNGNUG_H_

#if ((defined __LINUX_X86__) || (defined __FreeBSD__))
	#define __UNIX__  1
	#define __LINUX__ 1
	#define __INTEL__ 1
	#define __INTEL_BYTE_ORDER__ 1
	#if __ICC
		#undef ASM
		#undef ASM3DNOW
		#undef ASMKNI
		#define ASMLINUX 1
		#define COMPILER "Compiled with Intel C++"
	#else
		#undef ASM
		#undef ASM3DNOW
		#undef ASMKNI
		#define ASMLINUX 1
		#define COMPILER "Compiled with GNU g++ ("__VERSION__")"
	#endif
#elif (defined MACOSX)
	#define __UNIX__  1
	#undef ASM
	#undef ASM3DNOW
	#undef ASMKNI
	#define COMPILER "Compiled with MacOS X GNU g++ ("__VERSION__")"
	#error Unsupported platform.
#endif

// alloca
#include <stdlib.h>

// va_list
#include <stdarg.h>

// FLT_MAX
#include <float.h>


/*----------------------------------------------------------------------------
	Platform specifics types and defines.
----------------------------------------------------------------------------*/

// Undo any Windows defines.
#undef BYTE
#undef WORD
#undef DWORD
#undef INT
#undef FLOAT
#undef MAXBYTE
#undef MAXWORD
#undef MAXDWORD
#undef MAXINT
#undef VOID
#undef CDECL

// Make sure HANDLE is defined.
#define HANDLE DWORD
#define HINSTANCE DWORD

// Sizes.
enum {DEFAULT_ALIGNMENT = 8 }; // Default boundary to align memory allocations on.
enum {CACHE_LINE_SIZE   = 32}; // Cache line size.

#if __GNUG__
#define GCC_PACK(n)  __attribute__((packed,aligned(n)))
#define GCC_ALIGN(n) __attribute__((aligned(n)))
#define GCC_MOVE_ALIGN(n) 
#else
#define GCC_PACK(n) 
#define GCC_ALIGN(n) 
#define GCC_MOVE_ALIGN(n) 
#endif

//#define GCC_MOVE_ALIGN(n) __attribute__((aligned(n))) __attribute__((section (".bss")))

// Optimization macros
#define DISABLE_OPTIMIZATION  
#define ENABLE_OPTIMIZATION  

// Function type macros.
#define DLL_IMPORT
#define DLL_EXPORT			extern "C"
#define DLL_EXPORT_CLASS
#define VARARGS
#define CDECL
#define STDCALL
#define FORCEINLINE inline
#define ZEROARRAY 0 /* Zero-length arrays in structs */
#define __cdecl

#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
#define VSNPRINTF vsnprintf
#else
#define VSNPRINTF wvsnprintf
#endif

// Variable arguments.
#define GET_VARARGS(msg,len,lastarg,fmt)	\
{	\
	va_list ArgPtr;	\
	va_start( ArgPtr, lastarg );	\
	VSNPRINTF( msg, len, fmt, ArgPtr );	\
	va_end( ArgPtr );	\
}

#define GET_VARARGS_RESULT(msg,len,lastarg,fmt,result)	\
{	\
	va_list ArgPtr;	\
	va_start( ArgPtr, lastarg );	\
	result = VSNPRINTF( msg, len, fmt, ArgPtr );	\
	va_end( ArgPtr );	\
}


// Unsigned base types.
typedef unsigned char		BYTE;		// 8-bit  unsigned.
typedef unsigned short		_WORD;		// 16-bit unsigned.
typedef unsigned int		DWORD;		// 32-bit unsigned.
typedef unsigned long long	QWORD;		// 64-bit unsigned.
#if __GNUC__
//!!vogel: ???
//typedef unsigned int		OWORD __attribute__ ((mode (TI)));
#endif

// Signed base types.
typedef	signed char			SBYTE;		// 8-bit  signed.
typedef signed short		SWORD;		// 16-bit signed.
typedef signed int  		INT;		// 32-bit signed.
typedef signed long long	SQWORD;		// 64-bit signed.

// Character types.
typedef char			    ANSICHAR;	// An ANSI character.
typedef unsigned char		ANSICHARU;	// An ANSI character.

#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
typedef unsigned short      UNICHAR;	// A unicode character.
typedef unsigned short      UNICHARU;	// A unicode character.
#else
typedef wchar_t             UNICHAR;	// A unicode character.
typedef wchar_t             UNICHARU;	// A unicode character.
#endif

// Other base types.
typedef signed int			UBOOL;		// Boolean 0 (false) or 1 (true).
typedef float				FLOAT;		// 32-bit IEEE floating point.
typedef double				DOUBLE;		// 64-bit IEEE double.
typedef unsigned int        SIZE_T;     // Corresponds to C SIZE_T.

// Bitfield type.
#if FORCE_EXTREME_PACKING
typedef BYTE                BITFIELD;	// For bitfields.
#else
typedef unsigned int		BITFIELD;	// For bitfields.
#endif

#ifndef MACOSX
typedef unsigned int size_t;
#endif

// Make sure characters are signed.
#ifdef __CHAR_UNSIGNED__
	#error "Bad compiler option: Characters must be signed"
#endif

// Strings.
#if __UNIX__
#define LINE_TERMINATOR TEXT("\n")
#define PATH_SEPARATOR TEXT("/")
#define DLLEXT TEXT(".so")
#else
#define LINE_TERMINATOR TEXT("\r\n")
#define PATH_SEPARATOR TEXT("\\")
#define DLLEXT TEXT(".dll")
#endif

// NULL.
#undef NULL
#define NULL 0

// Package implementation.
#define IMPLEMENT_PACKAGE_PLATFORM(pkgname) \
	BYTE GLoaded##pkgname;

// Platform support options.
#define PLATFORM_NEEDS_ARRAY_NEW 1
#define FORCE_ANSI_LOG           0

#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
#define TCHAR_CALL_OS(funcW,funcA) (funcA)
#define TCHAR_TO_ANSI(str) str
#define ANSI_TO_TCHAR(str) str

#else

    CORE_API ANSICHAR* unixToANSI( ANSICHAR* ACh, const UNICHAR* InUCh );
    CORE_API INT unixGetSizeANSI( const UNICHAR* InUCh );
    CORE_API UNICHAR* unixToUNICODE( UNICHAR* UCh, const ANSICHAR* InACh );
    CORE_API INT unixGetSizeUNICODE( const ANSICHAR* InACh );
    CORE_API UNICHAR* unixANSIToUNICODE(char* str);
    CORE_API INT unixDetectUNICODE( void );
    #define UNICODE_BY_HAND 1
    #define _UNICODE 1

    #if defined(NO_ANSI_OS_SUPPORT)
	    #define TCHAR_CALL_OS(funcW,funcA) (funcW)
        #define TCHAR_TO_ANSI(str) str
	    #define ANSI_TO_TCHAR(str) str
    #else
	    #define TCHAR_CALL_OS(funcW,funcA) (GUnicodeOS ? (funcW) : (funcA))
    	#define TCHAR_TO_ANSI(str) unixToANSI((ANSICHAR*)appAlloca(unixGetSizeANSI(str)),str)
    	#define ANSI_TO_TCHAR(str) unixToUNICODE((TCHAR*)appAlloca(unixGetSizeUNICODE(str)),str)
    #endif

#endif


#if UNICODE_BY_HAND

CORE_API void unicode_str_to_stdout(const UNICHAR *str);

// These are implemented as portable C in Core/Src/UnUnix.cpp. You should
//  use these if your platform doesn't supply them, or your platform does
//  something dumb like make you use four byte unicode (like GNU).
//  Chances are that the versions supplied with your platform are going to
//  be much, much more optimized. You have been warned.
CORE_API UNICHAR* wcscpy( UNICHAR* Dest, const UNICHAR* Src);
CORE_API UNICHAR* wcsncpy( UNICHAR* Dest, const UNICHAR* Src, INT MaxLen );
CORE_API UNICHAR* wcscat( UNICHAR* String, const UNICHAR *Add );
CORE_API INT wcslen( const UNICHAR* String );
CORE_API INT wcscmp( const UNICHAR* Str1, const UNICHAR *Str2 );
CORE_API INT wcsncmp( const UNICHAR* Str1, const UNICHAR *Str2, INT max );
CORE_API UNICHAR* wcschr( const UNICHAR* String, const UNICHAR Find );
CORE_API UNICHAR* wcsstr( const UNICHAR* String, const UNICHAR* Find );
CORE_API INT _wcsicmp( const UNICHAR* String1, const UNICHAR* String2 );
CORE_API UNICHAR* _wcsupr( UNICHAR* String );
CORE_API INT wcstoul( const UNICHAR* Start, UNICHAR** End, INT Base );
CORE_API INT _wtoi( const UNICHAR* Str );
CORE_API INT _wcsnicmp( const UNICHAR* Str1, const UNICHAR *Str2, INT max );
CORE_API INT wprintf( const UNICHAR* fmt, ... );
CORE_API INT swscanf( const UNICHAR* fmt, ... );
CORE_API INT wvsnprintf( UNICHAR *buf, INT max, const UNICHAR *fmt, va_list args );
CORE_API INT iswspace( UNICHAR ch  );
CORE_API UNICHAR *_itow( const INT Num, UNICHAR *Buffer, const INT BufSize );
CORE_API QWORD _wcstoui64( const UNICHAR* Start, UNICHAR** End, INT Base );
CORE_API UNICHAR* _ui64tow( QWORD Num, UNICHAR *Buffer, INT Base );
CORE_API FLOAT _wtof( const UNICHAR* Str );
#endif

// Memory
#define appAlloca(size) ((size==0) ? 0 : alloca((size+7)&~7))
//#define appAlloca(size) ((size==0) ? 0 : malloc((size+7)&~7))

// System identification.
extern "C"
{
	extern HINSTANCE      hInstance;
	extern CORE_API UBOOL GIsMMX;
	extern CORE_API UBOOL GIsPentiumPro;
	extern CORE_API UBOOL GIsSSE;
}

// Module name
extern ANSICHAR GModule[32];
extern CORE_API DOUBLE GSecondsPerCycle;

char *strlwr(char *str);


CORE_API DOUBLE appSecondsSlow();

//!!vogel: sync with UnVcWin32.h.
//
// JP: Commented out these functions as they don't work reliably for some numbers
// eg appFloor(113) returns 112!
// This is due to the rounding for multiples of 0.5 depending on other bits of the number!
//
#if 0
/*
#define DEFINED_appRound 1
inline INT appRound( FLOAT F )
{
#if (defined __GNUC__) && (defined __i386__) && (defined ASMLINUX)
	static INT temp;
	__asm__ __volatile__( 
		"flds %1    \n\t"
		"fistpl %0  \n\t"
		: "=m" (temp)
		: "m" (F)
	);
	return temp;
#else
	return (INT)(F);
#endif
}

//
// Converts to integer equal to or less than.
//
#define DEFINED_appFloor 1
inline INT appFloor( FLOAT F )
{
#if (defined __GNUC__) && (defined __i386__) && (defined ASMLINUX)
	static INT temp;
	F -= 0.5f;
	__asm__ __volatile__( 
		"flds %1    \n\t"
		"fistpl %0  \n\t"
		: "=m" (temp)
		: "m" (F)
	);
	return temp;
#else
	static FLOAT Half=0.5;
	return (INT)(F - Half);
#endif
}
*/
#endif

//
// CPU cycles, related to GSecondsPerCycle.
//
#if ASMLINUX
#define DEFINED_appCycles 1
inline DWORD appCycles()
{
	DWORD r;
	asm("rdtsc" : "=a" (r) : "d" (r));
	return r;
}
#endif

//
// Seconds, arbitrarily based.
//
#if ASMLINUX
#define DEFINED_appSeconds 1
inline DOUBLE appSeconds()
{
	DWORD L,H;
	asm("rdtsc" : "=a" (L), "=d" (H));
	//!!vogel: add big number to make bugs apparent.
	return ((DOUBLE)L +  4294967296.0 * (DOUBLE)H) * GSecondsPerCycle + 16777216.0;
}
#endif

//
// Memory copy.
//
#define DEFINED_appMemcpy 1
inline void appMemcpy( void* Dest, const void* Src, INT Count )
{
	//!!vogel: TODO
	memcpy( Dest, Src, Count );
}

//
// Memory zero.
//
#define DEFINED_appMemzero 1
inline void appMemzero( void* Dest, INT Count )
{
	//!!vogel: TODO
	memset( Dest, 0, Count );
}

#if ASMLINUX
inline DWORD _rotr(DWORD val, INT shift)
{
	DWORD retval;
    asm("rorl %%cl, %%eax" : "=a" (retval) : "a" (val), "c" (shift));
	return retval;
}
#endif


#if ASMLINUX
inline DWORD _rotl(DWORD val, INT shift)
{
	DWORD retval;
    asm("roll %%cl, %%eax" : "=a" (retval) : "a" (val), "c" (shift));
	return retval;
}
#endif

#endif  // include-once blocker.

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/

