//=============================================================================
// U2.cpp
//=============================================================================

#include "U2.h"

// Register names.

#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) U2_API FName U2_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "U2Classes.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY


//-----------------------------------------------------------------------------
// package implementation
//-----------------------------------------------------------------------------

IMPLEMENT_PACKAGE_EX(U2)
{

	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name) U2_##name = FName(TEXT(#name),FNAME_Intrinsic);
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#include "U2Classes.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY

}

//-----------------------------------------------------------------------------
// intrinsic class implementations
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS(ADeploySpinner);
IMPLEMENT_CLASS(ATeamMeshActor);
IMPLEMENT_CLASS(AHackTrigger);

//-----------------------------------------------------------------------------
// end of U2.cpp
//-----------------------------------------------------------------------------
