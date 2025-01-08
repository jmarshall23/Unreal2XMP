/*=============================================================================
	UnObjVer.h: Unreal object version.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Version coding.
-----------------------------------------------------------------------------*/

// Offset demo version number by 1000
#if DEMOVERSION
#define DEMO_VERSION_OFFSET 0
#else
#define DEMO_VERSION_OFFSET 1000
#endif


// Earliest engine build that is network compatible with this one.
#define ENGINE_MIN_NET_VERSION (1000+DEMO_VERSION_OFFSET)

// Engine build number, for displaying to end users.
#define ENGINE_VERSION (1226+DEMO_VERSION_OFFSET)

// Base protocol version to negotiate in network play.
#define ENGINE_NEGOTIATION_VERSION (1000+DEMO_VERSION_OFFSET)

// Prevents incorrect files from being loaded.
#define PACKAGE_FILE_TAG 0x9E2A83C1

// The current Unrealfile version.
#define PACKAGE_FILE_VERSION 126

// Licensee Version Number.
#define PACKAGE_FILE_VERSION_LICENSEE	0x00
#define VERSION_ENGINE_ONLY				0x00

// The earliest file version which we can load with complete
// backwards compatibility. Must be at least PACKAGE_FILE_VERSION.
#define PACKAGE_MIN_VERSION 60

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

