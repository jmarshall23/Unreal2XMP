/*=============================================================================
	ALAudio.h: Unreal OpenAL Audio public header file.
	Copyright 1997-2001 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Daniel Vogel.
	* Ported to Linux by Ryan C. Gordon.
=============================================================================*/

#ifndef _INC_ALAUDIO
#define _INC_ALAUDIO

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef ALAUDIO_API
	#define ALAUDIO_API DLL_IMPORT
#endif

/*-----------------------------------------------------------------------------
	Dependencies.
-----------------------------------------------------------------------------*/

#ifdef WIN32
#include <windows.h>
#endif

#include "Core.h"
#include "Engine.h"
#include "UnRender.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Audio compiler specific includes.
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	Audio public includes.
-----------------------------------------------------------------------------*/

#include "ALAudioSubsystem.h"

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif
