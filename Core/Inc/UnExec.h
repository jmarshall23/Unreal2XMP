#ifndef __UNEXEC_H__
#define __UNEXEC_H__
//****************************************************************************
//**
//**    UNEXEC.H
//**    Header - Extended "Exec" System (CDH)
//**
//****************************************************************************
//============================================================================
//    HEADERS
//============================================================================
//============================================================================
//    DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
/*
	Variables
*/
#define EXECVAR(xType, xName, xValue) \
	static xType xName = xValue; \
	static FExecVar xName##_execVar(TEXT(#xName), TEXT(#xType), &##xName)

#define EXECVAR_FLAGS(xName, xFlags) \
	static UBOOL xName##_execVarFlagsDummy = xName##_execVar.SetFlags(xFlags)

#define EXECVAR_DESC(xName, xDescription) \
	static UBOOL xName##_execVarDescDummy = xName##_execVar.SetDesc(xDescription)

// FExecVar flags
enum
{
	EXECVARF_Persist	= 0x00000001, // variable should be saved/loaded for persistance
};

/*
	Functions
*/
#define EXECFUNC(xName) \
	static void xName##_execFuncBody(INT inArgCount, TCHAR** inArgs); \
	static FExecFunc xName##_execFunc(TEXT(#xName), xName##_execFuncBody); \
	static void xName##_execFuncBody(INT inArgCount, TCHAR** inArgs)

#define EXECFUNC_FLAGS(xName, xFlags) \
	static UBOOL xName##_execFuncFlagsDummy = xName##_execFunc.SetFlags(xFlags)

#define EXECFUNC_DESC(xName, xDescription) \
	static UBOOL xName##_execFuncDescDummy = xName##_execFunc.SetDesc(xDescription)

/*
	Variable Handlers
*/
#define EXECVAR_HANDLER(xType) \
	class FExecVarHandler##xType \
	: public FExecVarHandler \
	{ \
	public: \
		FExecVarHandler##xType(const TCHAR* inName, FExecVarHandlerCallback inFunc) \
		: FExecVarHandler(inName, inFunc) \
		{} \
		void ExecVarSave(FExecVar* inVar); \
		void ExecVarLoad(FExecVar* inVar); \
	}; \
	void xType##_execVarHandlerBody(FExecVar* inVar, INT inArgCount, TCHAR** inArgs); \
	FExecVarHandler##xType xType##_execVarHandler(TEXT(#xType), xType##_execVarHandlerBody); \
	void xType##_execVarHandlerBody(FExecVar* inVar, INT inArgCount, TCHAR** inArgs)

#define EXECVAR_HANDLER_SAVE(xType) \
	void FExecVarHandler##xType::ExecVarSave(FExecVar* inVar)

#define EXECVAR_HANDLER_LOAD(xType) \
	void FExecVarHandler##xType::ExecVarLoad(FExecVar* inVar)

//============================================================================
//    CLASSES / STRUCTURES
//============================================================================
class FExecVar; // exec variable
class FExecFunc; // exec function
class FExecVarHandler; // exec variable handler
class FExecDispatch; // exec variable/function command dispatch

/*
	Callbacks
*/
typedef void (*FExecFuncCallback)(INT inArgCount, TCHAR** inArgs);
typedef void (*FExecVarHandlerCallback)(FExecVar* inVar, INT inArgCount, TCHAR** inArgs);

/*
	FExecVar - Exec hook to a variable that can be manipulated via the console
*/
class CORE_API FExecVar
{
public:
	const TCHAR* mName; // name of variable
	const TCHAR* mDescription; // optional description string
	DWORD mFlags; // EXECVARF_ flags
	void* mValuePtr; // pointer to actual variable value
	const TCHAR* mVarTypeName; // name of variable type
	FExecVarHandler* mVarTypeHandler; // handler for this var type, once known
	FExecVar* mNext; // next in list of variables

	// Construction
	FExecVar(const TCHAR* inName, const TCHAR* inVarTypeName, void* inValuePtr);
	~FExecVar();

	// Methods
	UBOOL SetFlags(DWORD inFlags) { mFlags = inFlags; return(1); } // set flags, used by EXECVAR_FLAGS macro
	UBOOL SetDesc(const TCHAR* inDesc) { mDescription = inDesc; return(1); } // set description, used by EXECVAR_DESC macro
};

/*
	FExecFunc - Exec hook to a string-argument function that can be called via the console
*/
class CORE_API FExecFunc
{
public:
	const TCHAR* mName; // name of function
	const TCHAR* mDescription; // optional description string
	DWORD mFlags; // EXECFUNCF_ flags
	FExecFuncCallback mFunc; // actual function to call
	FExecFunc* mNext; // next in list of functions

	// Construction
	FExecFunc(const TCHAR* inName, FExecFuncCallback inFunc);
	~FExecFunc();

	// Methods
	UBOOL SetFlags(DWORD inFlags) { mFlags = inFlags; return(1); } // set flags, used by EXECFUNC_FLAGS macro
	UBOOL SetDesc(const TCHAR* inDesc) { mDescription = inDesc; return(1); } // set description, used by EXECFUNC_DESC macro
};

/*
	FExecVarHandler - base class of handlers for manipulating variables via function call
*/
class CORE_API FExecVarHandler
{
public:
	const TCHAR* mName; // name of handler, same as variable type being handled
	DWORD mFlags; // EXECVAR_HANDLERF_ flags
	FExecVarHandlerCallback mFunc; // function to call for command processing
	FExecVarHandler* mNext; // next in list of var handlers

	// Construction
	FExecVarHandler(const TCHAR* inName, FExecVarHandlerCallback inFunc);
	~FExecVarHandler();

	// Methods - Persistance
	virtual void ExecVarSave(FExecVar* inVar) {} // abstract method for saving persistant vars
	virtual void ExecVarLoad(FExecVar* inVar) {} // abstract method for loading persistant vars
};

/*
	FExecDispatch - Dispatches string commands to exec vars and functions
*/
class CORE_API FExecDispatch
: public FExec
{
public:
	FExecVar* mVarList; // list of all registered variables
	FExecFunc* mFuncList; // list of all registered functions
	FExecVarHandler* mVarHandlerList; // list of all registered variable handlers
	FOutputDevice* mPrintDevice; // output device used for prints
	void* UserData;

	// Construction
	void* operator new(size_t size, void* ptr) { return(ptr); }
	void operator delete(void* ptr, void*) {}

	FExecDispatch();
	~FExecDispatch();

	// Methods
	FExecVar* FindVar(const TCHAR* inName); // locate variable by name
	void RegisterVar(FExecVar* inVar); // register a variable with the dispatcher

	FExecFunc* FindFunc(const TCHAR* inName); // locate function by name
	void RegisterFunc(FExecFunc* inFunc); // register a function with the dispatcher

	FExecVarHandler* FindVarHandler(const TCHAR* inName); // locate handler by name	
	void RegisterVarHandler(FExecVarHandler* inVarHandler); // register a var handler with the dispatcher

	void SavePersistVars(); // save all persistant variable values
	void LoadPersistVars(); // load all persistant variable values

	void Printf(TCHAR* inFmt, ... ); // output if a print device is currently active, used inside callbacks

	UBOOL Execf(FOutputDevice& inDev, TCHAR* inFmt, ... ); // dispatch and execute a function or var handler

	// FExec implementation
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);
};

//============================================================================
//    GLOBAL DATA
//============================================================================
//============================================================================
//    GLOBAL FUNCTIONS
//============================================================================
//============================================================================
//    INLINE CLASS METHODS
//============================================================================
//============================================================================
//    TRAILING HEADERS
//============================================================================

//****************************************************************************
//**
//**    END HEADER UNEXEC.H
//**
//****************************************************************************
#endif // __UNEXEC_H__
