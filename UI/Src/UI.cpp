//=============================================================================
// UI.cpp
// $Author: Aleiby $
// $Date: 3/19/02 1:32a $
// $Revision: 15 $
//=============================================================================

#include "UI.h"
#include "ImportUI.h"

// Register names.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) UI_API FName UI_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "UIClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

IMPLEMENT_PACKAGE_EX(UI)
{
	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name) UI_##name = FName(TEXT(#name),FNAME_Intrinsic);
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#include "UIClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
}


EXECFUNC(SendEvent)
{
	if( !GIsEditor )
	{
		NOTE(debugf(TEXT("Sending Event: %s"), FString(inArgs[1])));
		if(inArgCount<1){ GExecDisp->Printf( TEXT("Warning! SendEvent: No event specified.") ); return; }
		FString ObjStr = FString(TEXT("Root"));
		FString EventStr = FString(inArgs[1]);
		FString(inArgs[1]).Split( TEXT("."), &ObjStr, &EventStr );
		UComponent* C = GetEventScopeTable().FindRef(ObjStr);
		if(!C){ GExecDisp->Printf( TEXT("Warning! Send event failed.  '%s' is not a registered component."), *ObjStr ); return; }
		C->SendEvent( GetEvent(EventStr) );
	}
}

EXECFUNC(LoadUI)
{
	NOTE(debugf(TEXT("LoadUI: %s"), FString(inArgs[1])));
	if(inArgCount<1){ GExecDisp->Printf( TEXT("Warning! LoadUI: No filename specified.") ); return; }

	UUIConsole* C=GetGConsole();

	// Clean up old stuff.
	if( C )
	{
		if( C->Root )
		{
			C->Root->DeleteUObject();
			C->Root = NULL;

			GetEventScopeTable().Empty();
			GetObjMap().Empty();
			GetGMouse() = NULL;
		}

		C->Root = GetGConsole()->LoadComponent( FString(inArgs[1]), TEXT("Root") );
	}
}

void StaticInitComponent( UComponent* C, const TCHAR* Name, UBOOL bTransient )
{
	C->Name = (bTransient || GetGTransient())
		? FNameTable::GetInstance()->CreateTransientName(Name)
		: FNameTable::GetInstance()->CreateName(Name);
	C->InitDefaults();
	C->Constructed();
}

uiClass* StringFindClass( const TCHAR* ClassName, UBOOL bCritical )
{
	for(uiClass* C=uiClass::GetClassList();C;C=C->NextClass())
		if( appStricmp( ClassName, C->GetName() )==0 )
			return C;

	if( bCritical )
		appErrorf(TEXT("StringFindClass: Class '%s' not found!"),ClassName);

	return NULL;
}

UComponent* StringCreateComponent( const TCHAR* Class, const TCHAR* Name )
{
	NOTE(for(uiClass* C=uiClass::GetClassList();C;C=C->NextClass()) debugf(TEXT("%s"),C->GetName()));

	uiClass* C = StringFindClass(Class);
	if(C) return C->CreateInstance(Name);

	// try tacking a 'U' on the front.
	FString UName = FString(TEXT("U")) + FString(Class);
	C = StringFindClass(*UName);
	if(C) return C->CreateInstance(Name);

	return NULL;
}

uiName::uiName(FString _Name){ *this=FNameTable::GetInstance()->FindName(_Name); }
FString uiName::operator*(){ return FNameTable::GetInstance()->GetName(*this); }
uiName uiName::None(){ static uiName None=FNameTable::GetInstance()->CreateName(TEXT("None")); return None; }
uiName uiName::Parent(){ static uiName Parent=FNameTable::GetInstance()->CreateName(TEXT("Parent")); return Parent; }

FString GetPathString( uiPath Path )
{
	guard(GetPathString);
	FString PathString = Path.Num() ? *Path.Pop() : TEXT("(empty)");
	while(Path.Num())
	{
		PathString += TEXT(".");
		PathString += *Path.Pop();
	}
	return PathString;
	unguard;
}


//
// Additional uiEvent support.
//

TEventTable &GetuiEventTable(){ static TEventTable uiEventTable=TEventTable(); return uiEventTable; }
static uiEvent uiLastEvent=EVENT_None;

uiEvent GetEvent( FString EventName )
{
	guard(UI::GetEvent);
	NOTE(debugf(TEXT("UI::GetEvent( %s )"), *EventName ));

	uiEvent* Event = GetuiEventTable().Find( EventName );

	if( Event )
		return *Event;
	else
		{ GetuiEventTable().Set( EventName, ++uiLastEvent ); return uiLastEvent; }

	unguard;
}

FString LookupEventName( uiEvent Event )
{
	guard(UI::LookupEventName);
	NOTE(debugf(TEXT("UI::LookupEventName( %d )"), Event ));

	FString* S=GetuiEventTable().FindKey(Event);
	return S ? *S : FString(TEXT("<event not found>"));

	unguard;
}


//
// UIHelper natives.
//

//------------------------------------------------------------------------------
void UUIHelper::execGetConsole( FFrame& Stack, RESULT_DECL )
{
	guard(UUIHelper::execGetConsole);
	NOTE(debugf(TEXT("(%s)UUIHelper::execGetConsole"), GetFullName() ));

	P_FINISH;

	*(UConsole**)Result = GetGConsole();

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIHelper::execGetUIConsole( FFrame& Stack, RESULT_DECL )
{
	guard(UUIHelper::execGetUIConsole);
	NOTE(debugf(TEXT("(%s)UUIHelper::execGetUIConsole"), GetFullName() ));

	P_FINISH;

	*(UUIConsole**)Result = GetGConsole();

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIHelper::execGetPlayerOwner( FFrame& Stack, RESULT_DECL )
{
	guard(UUIHelper::execGetPlayerOwner);
	NOTE(debugf(TEXT("(%s)UUIHelper::execGetPlayerOwner"), GetFullName() ));

	P_FINISH;

	*(APlayerController**)Result = GetGConsole()->Viewport->Actor;

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIHelper::execGetTimeSeconds( FFrame& Stack, RESULT_DECL )
{
	guard(UUIHelper::execGetTimeSeconds);
	NOTE(debugf(TEXT("(%s)UUIHelper::execGetTimeSeconds"), GetFullName() ));

	P_FINISH;

	*(FLOAT*)Result = appSeconds();

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIHelper::execGetClient( FFrame& Stack, RESULT_DECL )
{
	guard(UUIHelper::execGetClient);
	NOTE(debugf(TEXT("(%s)UUIHelper::execGetClient"), GetFullName() ));

	P_FINISH;

	*(UClient**)Result = GetGConsole()->Viewport->Actor->GetLevel()->Engine->Client;

	unguardexec;
}

//------------------------------------------------------------------------------
void UUIHelper::execConsoleCommand( FFrame& Stack, RESULT_DECL )
{
	guard(UUIHelper::execGetClient);
	NOTE(debugf(TEXT("(%s)UUIHelper::execGetClient"), GetFullName() ));

	P_GET_STR(Cmd);
	P_FINISH;

	*(UBOOL*)Result = GetGConsole()->ConsoleCommand(*Cmd);

	unguardexec;
}

IMPLEMENT_CLASS(UUIHelper);

// Other.
UI_API	INT					ImportIndentLevel=0;

//-----------------------------------------------------------------------------
// end of UI.cpp
//-----------------------------------------------------------------------------
