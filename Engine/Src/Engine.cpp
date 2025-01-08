/*=============================================================================
	Engine.cpp: Unreal engine package.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Global subsystems in the engine.
ENGINE_API FMemStack			GEngineMem;
ENGINE_API FMemCache			GCache;
#ifndef CONSOLE
ENGINE_API FTerrainTools		GTerrainTools;
ENGINE_API FRebuildTools		GRebuildTools;
#endif
ENGINE_API FMatineeTools		GMatineeTools;
ENGINE_API FLOAT				GRealDeltaTime=0;				/* Real DeltaTime between last tick and this one */
ENGINE_API FLOAT				GGameDeltaTime=0;				/* Game-level DeltaTime between last tick and this one (handles SLOMO, etc) */

namespace std
{
	class _String_base
	{ 
	public:
		__declspec(dllexport) void __thiscall _Xlen(void) const { }
		__declspec(dllexport) void __thiscall _Xran(void) const { }
	};

};

#ifdef _MSC_VER
extern "C" FILE _iob[] = { *stdin, *stdout, *stderr };
#endif

/*-----------------------------------------------------------------------------
	Package implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_PACKAGE(Engine);

/*-----------------------------------------------------------------------------
	The end.
-----------------------------------------------------------------------------*/

