//****************************************************************************
//**
//**    UNEXEC.CPP
//**    Extended "Exec" System (CDH)
//**
//****************************************************************************
//============================================================================
//    HEADERS
//============================================================================
#include "CorePrivate.h"

//============================================================================
//    DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
//============================================================================
//    CLASSES / STRUCTURES
//============================================================================
//============================================================================
//    PRIVATE DATA
//============================================================================
EXECVAR(INT, StubVar, 0); // stub variable, not actually used
EXECVAR_DESC(StubVar, TEXT("Stub variable"));
//EXECVAR_FLAGS(StubVar, EXECVARF_Persist); // commented out, just an example

//============================================================================
//    GLOBAL DATA
//============================================================================
// Global dispatch instance
CORE_API FExecDispatch* GExecDisp = NULL;

// Buffer for dispatch memory via placement new.  Necessary since registration
// is done before main(), when appMalloc() is set to use a valid interface.
static BYTE GExecDispBuffer[sizeof(FExecDispatch)];

//============================================================================
//    PRIVATE FUNCTIONS
//============================================================================
static void EXEC_Init()
{
	if (!GExecDisp)
		GExecDisp = new(GExecDispBuffer) FExecDispatch;
}

EXECFUNC(StubFunc) // stub function, not actually used
{
	if (inArgCount < 2)
		GExecDisp->Printf(TEXT("Uhh... hello?"));
	else
		GExecDisp->Printf(TEXT("Huh huh huh... you said \"%s\"..."), inArgs[1]);
}

EXECFUNC(ListVars) // lists all exec variables, optionally only those beginning with parameter
{
	if (!GExecDisp || !GExecDisp->mVarList)
		return;
	GExecDisp->Printf(TEXT("Variables:"));
	for (FExecVar* v = GExecDisp->mVarList; v; v = v->mNext)
	{
		if ((inArgCount > 1) && (inArgs[1][0]) && (appStrnicmp(v->mName, inArgs[1], appStrlen(inArgs[1]))))
			continue;
		if (v->mDescription && v->mDescription[0])
			GExecDisp->Printf(TEXT("  %s - %s"), v->mName, v->mDescription);
		else
			GExecDisp->Printf(TEXT("  %s"), v->mName);
	}
	GExecDisp->Printf(TEXT(""));
}

EXECFUNC(ListFuncs) // lists all exec functions, optionally only those beginning with parameter
{
	if (!GExecDisp || !GExecDisp->mFuncList)
		return;
	GExecDisp->Printf(TEXT("Functions:"));
	for (FExecFunc* f = GExecDisp->mFuncList; f; f = f->mNext)
	{
		if ((inArgCount > 1) && (inArgs[1][0]) && (appStrnicmp(f->mName, inArgs[1], appStrlen(inArgs[1]))))
			continue;
		if (f->mDescription && f->mDescription[0])
			GExecDisp->Printf(TEXT("  %s - %s"), f->mName, f->mDescription);
		else
			GExecDisp->Printf(TEXT("  %s"), f->mName);
	}
	GExecDisp->Printf(TEXT(""));
}

EXECFUNC(ListClasses) // lists all Unreal classes, another example function
{	
	for (TObjectIterator<UClass> ItC; ItC; ++ItC)
		GExecDisp->Printf(TEXT("  %s"), ItC->GetName());
}

/*
	Variable Handlers
*/

// FLOAT handler
EXECVAR_HANDLER(FLOAT)
{
	FLOAT* val = (FLOAT*)inVar->mValuePtr;

	if (inArgCount < 2)
	{
		GExecDisp->Printf(TEXT(" float %s = %.4f"), inVar->mName, *val);
		return;
	}
	if (inArgCount < 3)
	{
		if (!appStrcmp(inArgs[1], TEXT("++")))
			*val += 1.f;
		else if (!appStrcmp(inArgs[1], TEXT("--")))
			*val -= 1.f;
		else
			*val = appAtof(inArgs[1]);
		return;
	}
	if (!appStrcmp(inArgs[1], TEXT("=")))
		*val = appAtof(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("+=")))
		*val += appAtof(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("-=")))
		*val -= appAtof(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("*=")))
		*val *= appAtof(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("/=")))
		*val /= appAtof(inArgs[2]);
}
EXECVAR_HANDLER_SAVE(FLOAT)
{
	GConfig->SetFloat(TEXT("Core.ExecDispatch"), inVar->mName, *((FLOAT*)inVar->mValuePtr), NULL);
}
EXECVAR_HANDLER_LOAD(FLOAT)
{
	GConfig->GetFloat(TEXT("Core.ExecDispatch"), inVar->mName, *((FLOAT*)inVar->mValuePtr), NULL);
}

// INT handler
EXECVAR_HANDLER(INT)
{
	INT* val = (INT*)inVar->mValuePtr;

	if (inArgCount < 2)
	{
		GExecDisp->Printf(TEXT(" int %s = %d"), inVar->mName, *val);
		return;
	}
	if (inArgCount < 3)
	{
		if (!appStrcmp(inArgs[1], TEXT("++")))
			*val += 1;
		else if (!appStrcmp(inArgs[1], TEXT("--")))
			*val -= 1;
		else if (!appStrcmp(inArgs[1], TEXT("^^")))
			*val = (*val == 0);
		else if (!appStricmp(inArgs[1], TEXT("tog")))
			*val = (*val == 0);
		else if (!appStricmp(inArgs[1], TEXT("on")))
			*val = 1;
		else if (!appStricmp(inArgs[1], TEXT("off")))
			*val = 0;
		else
			*val = appAtoi(inArgs[1]);
		return;
	}
	if (!appStrcmp(inArgs[1], TEXT("=")))
		*val = appAtoi(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("+=")))
		*val += appAtoi(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("-=")))
		*val -= appAtoi(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("*=")))
		*val *= appAtoi(inArgs[2]);
	else if (!appStrcmp(inArgs[1], TEXT("/=")))
		*val /= appAtoi(inArgs[2]);
}
EXECVAR_HANDLER_SAVE(INT)
{
	GConfig->SetInt(TEXT("Core.ExecDispatch"), inVar->mName, *((INT*)inVar->mValuePtr), NULL);
}
EXECVAR_HANDLER_LOAD(INT)
{
	GConfig->GetInt(TEXT("Core.ExecDispatch"), inVar->mName, *((INT*)inVar->mValuePtr), NULL);
}

// UBOOL handler
EXECVAR_HANDLER(UBOOL)
{
	UBOOL* val = (UBOOL*)inVar->mValuePtr;

	if (inArgCount < 2)
	{
		GExecDisp->Printf(TEXT(" ubool %s is %s"), inVar->mName, *val ? TEXT("on") : TEXT("off"));
		return;
	}
	if (inArgCount < 3)
	{
		if (!appStrcmp(inArgs[1], TEXT("^^")))
			*val = (*val == 0);
		else if (!appStricmp(inArgs[1], TEXT("tog")))
			*val = (*val == 0);	
		else if (!appStricmp(inArgs[1], TEXT("on")))
			*val = 1;
		else if (!appStricmp(inArgs[1], TEXT("off")))
			*val = 0;
		else
			*val = (appAtoi(inArgs[1]) != 0);
		return;
	}
	if (!appStrcmp(inArgs[1], TEXT("=")))
		*val = (appAtoi(inArgs[2]) != 0);
}
EXECVAR_HANDLER_SAVE(UBOOL)
{
	GConfig->SetBool(TEXT("Core.ExecDispatch"), inVar->mName, *((UBOOL*)inVar->mValuePtr), NULL);
}
EXECVAR_HANDLER_LOAD(UBOOL)
{
	GConfig->GetBool(TEXT("Core.ExecDispatch"), inVar->mName, *((UBOOL*)inVar->mValuePtr), NULL);
}

//============================================================================
//    GLOBAL FUNCTIONS
//============================================================================
//============================================================================
//    CLASS METHODS
//============================================================================
/*
	FExecVar
*/
FExecVar::FExecVar(const TCHAR* inName, const TCHAR* inVarTypeName, void* inValuePtr)
: mName(inName)
, mDescription(TEXT(""))
, mFlags(0)
, mValuePtr(inValuePtr)
, mVarTypeName(inVarTypeName)
, mVarTypeHandler(NULL)
, mNext(NULL)
{
	EXEC_Init();
	mVarTypeHandler = GExecDisp->FindVarHandler(mVarTypeName);
	GExecDisp->RegisterVar(this);
}
FExecVar::~FExecVar()
{}

/*
	FExecFunc
*/
FExecFunc::FExecFunc(const TCHAR* inName, FExecFuncCallback inFunc)
: mName(inName)
, mDescription(TEXT(""))
, mFlags(0)
, mFunc(inFunc)
, mNext(NULL)
{
	EXEC_Init();
	GExecDisp->RegisterFunc(this);
}
FExecFunc::~FExecFunc()
{}

/*
	FExecVarHandler
*/
FExecVarHandler::FExecVarHandler(const TCHAR* inName, FExecVarHandlerCallback inFunc)
: mName(inName)
, mFlags(0)
, mFunc(inFunc)
, mNext(NULL)
{
	EXEC_Init();
	GExecDisp->RegisterVarHandler(this);
}
FExecVarHandler::~FExecVarHandler()
{}

/*
	FExecDispatch
*/
FExecDispatch::FExecDispatch()
: mVarList(NULL)
, mFuncList(NULL)
, mVarHandlerList(NULL)
, mPrintDevice(NULL)
{
}
FExecDispatch::~FExecDispatch()
{}

FExecVar* FExecDispatch::FindVar(const TCHAR* inName)
{
	for (FExecVar* v = mVarList; v; v = v->mNext)
	{
		if (!appStricmp(v->mName, inName))
			return(v);
	}
	return(NULL);
}
void FExecDispatch::RegisterVar(FExecVar* inVar)
{
	inVar->mNext = mVarList;
	mVarList = inVar;
}

FExecFunc* FExecDispatch::FindFunc(const TCHAR* inName)
{
	for (FExecFunc* f = mFuncList; f; f = f->mNext)
	{
		if (!appStricmp(f->mName, inName))
			return(f);
	}
	return(NULL);
}
void FExecDispatch::RegisterFunc(FExecFunc* inFunc)
{
	inFunc->mNext = mFuncList;
	mFuncList = inFunc;
}

FExecVarHandler* FExecDispatch::FindVarHandler(const TCHAR* inName)
{
	for (FExecVarHandler* vh = mVarHandlerList; vh; vh = vh->mNext)
	{
		if (!appStricmp(vh->mName, inName))
			return(vh);
	}
	return(NULL);
}
void FExecDispatch::RegisterVarHandler(FExecVarHandler* inVarHandler)
{
	inVarHandler->mNext = mVarHandlerList;
	mVarHandlerList = inVarHandler;
}

void FExecDispatch::SavePersistVars()
{
	for (FExecVar* v = mVarList; v; v = v->mNext)
	{
		if (!(v->mFlags & EXECVARF_Persist))
			continue;
		if (!v->mVarTypeHandler)
		{
			v->mVarTypeHandler = FindVarHandler(v->mVarTypeName);
			if (!v->mVarTypeHandler)
				appErrorf(TEXT("FExecDispatch::SavePersistVars: Unresolved type \"%s\""), v->mVarTypeName);
		}
		v->mVarTypeHandler->ExecVarSave(v);
	}
}
void FExecDispatch::LoadPersistVars()
{
	for (FExecVar* v = mVarList; v; v = v->mNext)
	{
		if (!(v->mFlags & EXECVARF_Persist))
			continue;
		if (!v->mVarTypeHandler)
		{
			v->mVarTypeHandler = FindVarHandler(v->mVarTypeName);
			if (!v->mVarTypeHandler)
				appErrorf(TEXT("FExecDispatch::LoadPersistVars: Unresolved type \"%s\""), v->mVarTypeName);
		}
		v->mVarTypeHandler->ExecVarLoad(v);
	}
}

void FExecDispatch::Printf(TCHAR* inFmt, ... )
{
	static TCHAR buf[2048];
	appGetVarArgs(buf, 2047,(const TCHAR *&)inFmt);
	if (mPrintDevice)
		mPrintDevice->Log(buf);
}

UBOOL FExecDispatch::Execf(FOutputDevice& inDev, TCHAR* inFmt, ... )
{
	static TCHAR buf[2048];
	appGetVarArgs(buf, 2047, (const TCHAR *&)inFmt);
	return(Exec(buf, inDev));
}

UBOOL FExecDispatch::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	TCHAR buf[2048];
	TCHAR* cmdArgv[32];
	INT cmdArgc;
	TCHAR* ptr;
	UBOOL inQuoteState = 0;
	UBOOL inWhiteSpace = 1;

	appStrncpy(buf, Cmd, 2047);

	mPrintDevice = &Ar;

	cmdArgc = 0;
	for (ptr=buf; *ptr; ptr++)
	{
		if (!inQuoteState)
		{
			if ((*ptr == ' ') || (*ptr == '\t'))
			{
				*ptr = 0;
				inWhiteSpace = 1;
			}
			else if ((*ptr == '/') && (*(ptr+1) == '/'))
			{ // eol comment, we're done
				*ptr = 0;
				ptr--;
			}
			else if (inWhiteSpace)
			{
				// we were in whitespace, but no we're obviously not, so add an argument
				if (cmdArgc >= 32)
				{ // too many arguments
					*ptr = 0;
					ptr--;
				}
				else
				{
					cmdArgv[cmdArgc] = ptr;
					cmdArgc++;
					inWhiteSpace = 0;
					if (*ptr == '\"')
					{ // if an argument begins with quotations, continue until next quotation
						inQuoteState = 1;
						cmdArgv[cmdArgc-1] = ptr+1; // skip quotation
					}
				}
			}
		}
		else // inQuoteState
		{
			if (*ptr == '\"')
			{ // done with quotation
				*ptr = 0;
				inQuoteState = 0;
				inWhiteSpace = 1;
			}
		}
	}

	// got the arguments, now dispatch the command
	if (!cmdArgc)
	{
		mPrintDevice = NULL;
		return(0); // nothing to execute
	}

	// variables get checked first
	UBOOL found = 0;
	FExecVar* vLast = NULL;
	for (FExecVar* v = mVarList; v; vLast = v, v = v->mNext)
	{
		if (appStricmp(v->mName, cmdArgv[0]))
			continue;
		
		// got a match
		if (!v->mVarTypeHandler)
		{
			v->mVarTypeHandler = FindVarHandler(v->mVarTypeName);
			if (!v->mVarTypeHandler)
				appErrorf(TEXT("FExecDispatch::Exec: Unresolved type \"%s\""), v->mVarTypeName);
		}
		v->mVarTypeHandler->mFunc(v, cmdArgc, cmdArgv);

		// move to top of list, to speed up consecutive accesses
		if (vLast)
		{
			vLast->mNext = v->mNext;
			v->mNext = mVarList;
			mVarList = v;
		}

		found = 1;
		break;
	}

	// check functions
	if (!found)
	{
		FExecFunc* fLast = NULL;
		for (FExecFunc* f = mFuncList; f; fLast = f, f = f->mNext)
		{
			if (appStricmp(f->mName, cmdArgv[0]))
				continue;

			// got a match
			f->mFunc(cmdArgc, cmdArgv);

			// move to top of list, to speed up consecutive accesses
			if (fLast)
			{
				fLast->mNext = f->mNext;
				f->mNext = mFuncList;
				mFuncList = f;
			}

			found = 1;
			break;
		}
	}

	// done, return whether something was found or not
	mPrintDevice = NULL;
	return(found);
}

//****************************************************************************
//**
//**    END MODULE UNEXEC.CPP
//**
//****************************************************************************
