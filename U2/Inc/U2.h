//=============================================================================
// U2.h: U2-specific package header
//=============================================================================

#ifndef _INC_U2
#define _INC_U2

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef U2_API
	#define U2_API DLL_IMPORT
#endif

/*-----------------------------------------------------------------------------
	Dependencies.
-----------------------------------------------------------------------------*/

#include "Core.h"
#include "Engine.h"
#include "UI.h"
#include "Legend.h"
#include "ParticleSystems.h"

/*-----------------------------------------------------------------------------
	Debugging.
-----------------------------------------------------------------------------*/

//#define NOTE(func) func
#define NOTE(func)
#define NOTE0(func)
#define NOTE1(func) func

/*-----------------------------------------------------------------------------
	Includes.
-----------------------------------------------------------------------------*/

#include "U2Classes.h"

#endif //_INC_U2

//-----------------------------------------------------------------------------
//	end of U2.h
//-----------------------------------------------------------------------------

