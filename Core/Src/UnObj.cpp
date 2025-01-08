/*=============================================================================
	UnObj.cpp: Unreal object manager.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"
#include "FTimerManager.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Object manager internal variables.
UBOOL						UObject::GObjInitialized		= 0;
UBOOL						UObject::GObjNoRegister			= 0;
UBOOL						UObject::GObjPurgingGarbage		= 0;
INT							UObject::GObjBeginLoadCount		= 0;
INT							UObject::GObjRegisterCount		= 0;
INT							UObject::GImportCount			= 0;
UObject*					UObject::GAutoRegister			= NULL;
UPackage*					UObject::GObjTransientPkg		= NULL;
TCHAR						UObject::GObjCachedLanguage[32] = TEXT("");
TCHAR						UObject::GLanguage[64]          = TEXT("int");
UObject*					UObject::GObjHash[4096];
TArray<UObject*>			UObject::GObjLoaded;
TArray<UObject*>			UObject::GObjObjects;
TArray<INT>					UObject::GObjAvailable;
TArray<UObject*>			UObject::GObjLoaders;
TArray<UObject*>			UObject::GObjRoot;
TArray<UObject*>			UObject::GObjRegistrants;
TArray<FPreferencesInfo>	UObject::GObjPreferences;
TArray<FRegistryObjectInfo> UObject::GObjDrivers;
TArray<UObject*>			UObject::PendingDeleteObjects;
static INT GGarbageRefCount=0;

/*-----------------------------------------------------------------------------
	UObject constructors.
-----------------------------------------------------------------------------*/

UObject::UObject()
{}
UObject::UObject( const UObject& Src )
{
	guard(UObject::UObject);
	check(&Src);
	if( Src.GetClass()!=GetClass() )
		appErrorf( TEXT("Attempt to copy-construct %s from %s"), GetFullName(), Src.GetFullName() );
	unguard;
}
UObject::UObject( EInPlaceConstructor, UClass* InClass, UObject* InOuter, FName InName, DWORD InFlags )
{
	guard(UObject::UObject);
	StaticAllocateObject( InClass, InOuter, InName, InFlags, NULL, GError, this );
	unguard;
}
UObject::UObject( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags )
:	Index		( INDEX_NONE				)
,	HashNext	( NULL						)
,	StateFrame	( NULL						)
,	_Linker		( NULL						)
,	ObjectFlags	( InFlags | RF_Native	    )
,	Class		( InClass					)
,	_LinkerIndex( INDEX_NONE				)
,	Outer       ( NULL						)
,	Name        ( NAME_None					)
{
	guard(UObject::UObject);

	// Make sure registration is allowed now.
	check(!GObjNoRegister);

	// Setup registration info, for processing now (if inited) or later (if starting up).
	check(sizeof(Outer       )>=sizeof(InPackageName));
	check(sizeof(_LinkerIndex)>=sizeof(GAutoRegister));

#if SERIAL_POINTER_INDEX
	check(sizeof(Name        )>=sizeof(INT          ));
	*(INT*)&Name                    = SerialPointerIndex((void *) InName);
#else
	check(sizeof(Name        )>=sizeof(InName       ));
	*(const TCHAR  **)&Name         = InName;
#endif

	*(const TCHAR  **)&Outer        = InPackageName;
	*(UObject      **)&_LinkerIndex = GAutoRegister;
	GAutoRegister                   = this;

	// Call native registration from terminal constructor.
	if( GetInitialized() && GetClass()==StaticClass() )
		Register();

	unguard;
}
UObject::UObject( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags )
:	Index		( INDEX_NONE				)
,	HashNext	( NULL						)
,	StateFrame	( NULL						)
,	_Linker		( NULL						)
,	ObjectFlags	( InFlags | RF_Native	    )
,	_LinkerIndex( INDEX_NONE				)
,	Outer       ( NULL						)
,	Name        ( NAME_None					)
{
	guard(UObject::UObject);

	// Setup registration info, for processing now (if inited) or later (if starting up).
	check(sizeof(Outer       )>=sizeof(InPackageName));
	check(sizeof(_LinkerIndex)>=sizeof(GAutoRegister));

#if SERIAL_POINTER_INDEX
	check(sizeof(Name        )>=sizeof(INT          ));
	*(INT*)&Name                    = SerialPointerIndex((void *) InName);
#else
	check(sizeof(Name        )>=sizeof(InName       ));
	*(const TCHAR  **)&Name         = InName;
#endif

	*(const TCHAR  **)&Outer        = InPackageName;

	
	// If we are not initialized yet, auto register.
	if (!GObjInitialized)
	{
		*(UObject      **)&_LinkerIndex = GAutoRegister;
		GAutoRegister                   = this;
	}
	unguard;
}
UObject& UObject::operator=( const UObject& Src )
{
	guard(UObject::operator=);
	check(&Src);
	if( Src.GetClass()!=GetClass() )
		appErrorf( TEXT("Attempt to assign %s from %s"), GetFullName(), Src.GetFullName() );
	return *this;
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UObject class initializer.
-----------------------------------------------------------------------------*/

void UObject::StaticConstructor()
{
	guard(UObject::StaticConstructor);
	unguard;
}

/*-----------------------------------------------------------------------------
	UObject implementation.
-----------------------------------------------------------------------------*/

//
// Rename this object to a unique name.
//
void UObject::Rename( const TCHAR* InName, UObject* NewOuter )
{
	guard(UObject::Rename);

	FName NewName = InName ? FName(InName) : MakeUniqueObjectName( NewOuter ? NewOuter : GetOuter(), GetClass() );
	UnhashObject( Outer ? Outer->GetIndex() : 0 );
	debugfSlow( TEXT("Renaming %s to %s"), *Name, *NewName );
	Name = NewName;
	if( NewOuter )
		Outer = NewOuter;
	HashObject();

	unguardobj;
}

//
// Shutdown after a critical error.
//
void UObject::ShutdownAfterError()
{
	guard(UObject::ShutdownAfterError);
	unguard;
}

//
// Make sure the object is valid.
//
UBOOL UObject::IsValid()
{
	guard(UObject::IsValid);
	if( !this )
	{
		debugf( NAME_Warning, TEXT("NULL object") );
		return 0;
	}
	else if( !GObjObjects.IsValidIndex(GetIndex()) )
	{
		debugf( NAME_Warning, TEXT("Invalid object index %i"), GetIndex() );
		debugf( NAME_Warning, TEXT("This is: %s"), GetFullName() );
		return 0;
	}
	else if( GObjObjects(GetIndex())==NULL )
	{
		debugf( NAME_Warning, TEXT("Empty slot") );
		debugf( NAME_Warning, TEXT("This is: %s"), GetFullName() );
		return 0;
	}
	else if( GObjObjects(GetIndex())!=this )
	{
		debugf( NAME_Warning, TEXT("Other object in slot") );
		debugf( NAME_Warning, TEXT("This is: %s"), GetFullName() );
		debugf( NAME_Warning, TEXT("Other is: %s"), GObjObjects(GetIndex())->GetFullName() );
		return 0;
	}
	else return 1;
	unguardobj;
}

//
// Do any object-specific cleanup required
// immediately after loading an object, and immediately
// after any undo/redo.
//
void UObject::PostLoad()
{
	guard(UObject::PostLoad);

	// Note that it has propagated.
	SetFlags( RF_DebugPostLoad );

	// Load per-object localization
	if( ObjectFlags&RF_PerObjectLocalized)
		LoadLocalized();
	unguardobj;
}

//
// Edit change notification.
//
void UObject::PostEditChange()
{
	guard(UObject::PostEditChange);
	Modify();
	unguardobj;
}

//
// Do any object-specific cleanup required immediately
// before an object is killed.  Child classes may override
// this if they have to do anything here.
//
void UObject::Destroy()
{
	guard(UObject::Destroy);
	SetFlags( RF_DebugDestroy );

	// Destroy properties.
	ExitProperties( (BYTE*)this, GetClass() );

	// Log message.
	if( GObjInitialized && !GIsCriticalError )
		debugfSlow( NAME_DevKill, TEXT("Destroying %s"), GetFullName() );

	// Unhook from linker.
	SetLinker( NULL, INDEX_NONE );

	// Remember outer name index.
	_LinkerIndex = Outer ? Outer->GetIndex() : 0;

	unguardobj;
}

//
// Set the object's linker.
//
void UObject::SetLinker( ULinkerLoad* InLinker, INT InLinkerIndex )
{
	guard(UObject::SetLinker);

	// Detach from existing linker.
	if( _Linker )
	{
		check(_Linker->ExportMap(_LinkerIndex)._Object!=NULL);
		check(_Linker->ExportMap(_LinkerIndex)._Object==this);
		_Linker->ExportMap(_LinkerIndex)._Object = NULL;
	}

	// Set new linker.
	_Linker      = InLinker;
	_LinkerIndex = InLinkerIndex;

	unguardobj;
}

//
// Return the object's path name.
//warning: Must be safe for NULL objects.
//warning: Must be safe for class-default meta-objects.
//
const TCHAR* UObject::GetPathName( UObject* StopOuter, TCHAR* Str ) const
{
	guard(UObject::GetPathName);

	// Return one of 8 circular results.
	TCHAR* Result = Str ? Str : appStaticString1024();
	if( this!=StopOuter )
	{
		*Result = 0;
		if( Outer!=StopOuter )
		{
			Outer->GetPathName( StopOuter, Result );
			appStrcat( Result, TEXT(".") );
		}
		appStrcat( Result, GetName() );
	}
	else appSprintf( Result, TEXT("None") );

	return Result;
	unguard; // Not unguardobj, because it causes crash recusion.
}

//
// Return the object's full name.
//warning: Must be safe for NULL objects.
//
const TCHAR* UObject::GetFullName( TCHAR* Str ) const
{
	guard(UObject::GetFullName);

	// Return one of 8 circular results.
	TCHAR* Result = Str ? Str : appStaticString1024();
	if( this )
	{
		appSprintf( Result, TEXT("%s "), GetClass()->GetName() );
		GetPathName( NULL, Result + appStrlen( Result ) );
	}
	else
	{
		appStrcpy( Result, TEXT("None") );
	}

	return Result;
	unguard; // Not unguardobj, because it causes crash recusion.
}

//
// Destroy the object if necessary.
//
UBOOL UObject::ConditionalDestroy()
{
	guard(UObject::ConditionalDestroy);
	if( Index!=INDEX_NONE && !(GetFlags() & RF_Destroyed) )
	{
		SetFlags( RF_Destroyed );
		ClearFlags( RF_DebugDestroy );
		Destroy();
		if( !(GetFlags()&RF_DebugDestroy) )
			appErrorf( TEXT("%s failed to route Destroy"), GetFullName() );
		return 1;
	}
	else return 0;
	unguardobj;
}

//
// Register if needed.
//
void UObject::ConditionalRegister()
{
	guard(UObject::ConditionalRegister);
	if( GetIndex()==INDEX_NONE )
	{
		// Verify this object is on the list to register.
		INT i;
		for( i=0; i<GObjRegistrants.Num(); i++ )
			if( GObjRegistrants(i)==this )
				break;
		check(i!=GObjRegistrants.Num());

		// Register it.
		Register();
	}
	unguardobj;
}

//
// PostLoad if needed.
//
void UObject::ConditionalPostLoad()
{
	guard(UObject::ConditionalPostLoad);
	if( GetFlags() & RF_NeedPostLoad )
	{
		ClearFlags( RF_NeedPostLoad | RF_DebugPostLoad );
		PostLoad();
		if( !(GetFlags() & RF_DebugPostLoad) )
 			appErrorf( TEXT("%s failed to route PostLoad"), GetFullName() );
	}
	unguardobj;
}

//
// UObject destructor.
//warning: Called at shutdown.
//
UObject::~UObject()
{
	guard(UObject::~UObject);

	// If not initialized, skip out.
	if( Index!=INDEX_NONE && GObjInitialized && !GIsCriticalError )
	{
		// Validate it.
		check(IsValid());

		// Destroy the object if necessary.
		ConditionalDestroy();

		// Remove object from table.
		UnhashObject( _LinkerIndex );
		GObjObjects(Index) = NULL;
		GObjAvailable.AddItem( Index );
	}

	// Free execution stack.
	if( StateFrame )
	{
		delete StateFrame;
		StateFrame=NULL;
	}

	unguard;
}

//
// Archive for persistent CRC's.
//
class FArchiveCRC : public FArchive
{
public:
	FArchiveCRC( UObject* Src, UObject* InBase )
	: CRC( 0 )
	, Base( InBase )
	{
		ArIsSaving = ArIsPersistent = 1;
		Src->Serialize( *this );
	}
	SIZE_T GetCRC()
	{
		return CRC;
	}
	void Serialize( void* Data, INT Num )
	{
		CRC = appMemCrc( Data, Num, CRC );
	}
	void SerializeCapsString( const TCHAR* Str )
	{
		TCHAR TCh;
		while( *Str )
		{
			TCh = appToUpper(*Str++);
			Serialize( &TCh, sizeof(TCh) );
		}
		TCh = 0;
		Serialize( &TCh, sizeof(TCh) );
	}
	virtual FArchive& operator<<( class FName& N )
	{
		SerializeCapsString( *N );
		return *this;
	}
	virtual FArchive& operator<<( class UObject*& Res )
	{
		if( Res )
		{
			SerializeCapsString( Res->GetClass()->GetName() );
			SerializeCapsString( Res->GetPathName(Res->IsIn(Base) ? Base : NULL) );
		}
		else SerializeCapsString( TEXT("None") );
		return *this;
	}
protected:
	DWORD CRC;
	UObject* Base;
};

//
// Note that the object has been modified.
//
void UObject::Modify()
{
	guard(UObject::Modify);

	// Perform transaction tracking.
	if( GUndo && (GetFlags() & RF_Transactional) )
		GUndo->SaveObject( this );

	unguardobj;
}

//
// UObject serializer.
//
void UObject::Serialize( FArchive& Ar )
{
	guard(UObject::Serialize);
	SetFlags( RF_DebugSerialize );

	// Make sure this object's class's data is loaded.
	if( Class != UClass::StaticClass() )
		Ar.Preload( Class );

	// Special info.
	if( (!Ar.IsLoading() && !Ar.IsSaving()) )
		Ar << Name << Outer << Class;
	if( !Ar.IsLoading() && !Ar.IsSaving() )
		Ar << _Linker;

	// Execution stack.
	//!!how does the stack work in conjunction with transaction tracking?
	guard(SerializeStack);
	if( !Ar.IsTrans() )
	{
		if( GetFlags() & RF_HasStack )
		{
			if( !StateFrame )
				StateFrame = new(TEXT("ObjectStateFrame")) FStateFrame( this );
			Ar << StateFrame->Node << StateFrame->StateNode;
			Ar << StateFrame->ProbeMask;
			Ar << StateFrame->LatentAction;
			if( StateFrame->Node )
			{
				Ar.Preload( StateFrame->Node );
				if( Ar.IsSaving() && StateFrame->Code )
					check(StateFrame->Code>=&StateFrame->Node->Script(0) && StateFrame->Code<&StateFrame->Node->Script(StateFrame->Node->Script.Num()));
				INT Offset = StateFrame->Code ? StateFrame->Code - &StateFrame->Node->Script(0) : INDEX_NONE;
				Ar << AR_INDEX(Offset);
				if( Offset!=INDEX_NONE )
					if( Offset<0 || Offset>=StateFrame->Node->Script.Num() )
						appErrorf( TEXT("%s: Offset mismatch: %i %i"), GetFullName(), Offset, StateFrame->Node->Script.Num() );
				StateFrame->Code = Offset!=INDEX_NONE ? &StateFrame->Node->Script(Offset) : NULL;
			}
			else StateFrame->Code = NULL;
		}
		else if( StateFrame )
		{
			delete StateFrame;
			StateFrame = NULL;
		}
	}
	unguard;

	// Serialize object properties which are defined in the class.
	if( Class != UClass::StaticClass() )
	{
		if( (Ar.IsLoading() || Ar.IsSaving()) && !Ar.IsTrans() )
			GetClass()->SerializeTaggedProperties( Ar, (BYTE*)this, Class );
		else
			GetClass()->SerializeBin( Ar, (BYTE*)this, 0 );
	}

	// Memory counting.
	SIZE_T Size = GetClass()->GetPropertiesSize();
	Ar.CountBytes( Size, Size );
	unguardobj;
}

//
// Export text object properties.
//
void UObject::ExportProperties
(
	FOutputDevice&	Out,
	UClass*			ObjectClass,
	BYTE*			Object,
	INT				Indent,
	UClass*			DiffClass,
	BYTE*			Diff
)
{
	const TCHAR* ThisName = TEXT("(none)");
	guard(UObject::ExportProperties);
	check(ObjectClass!=NULL);
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(ObjectClass); It; ++It )
	{
		if( It->Port() )
		{
			ThisName = It->GetName();			
			for( INT j=0; j<It->ArrayDim; j++ )
			{
				TCHAR Value[4096];
				if( It->IsA(UArrayProperty::StaticClass()) ) 
				{
					// Export dynamic array.
					UProperty* InnerProp = Cast<UArrayProperty>(*It)->Inner;
					UBOOL ExportObject = (It->PropertyFlags & CPF_ExportObject) && InnerProp->IsA(UObjectProperty::StaticClass());
											
					FArray* Arr = (FArray*)((BYTE*)Object + It->Offset + j*It->ElementSize);
					for( INT k=0;k<Arr->Num();k++ )
					{
						if( InnerProp->ExportText( k, Value, (BYTE*)Arr->GetData(), NULL, PPF_Delimited ) )
						{					
							if( ExportObject )
							{
								UObject* Obj = ((UObject**)Arr->GetData())[k];
								if( Obj && !(Obj->GetFlags() & RF_TagImp) )
								{
									// Don't export more than once.
									UExporter::ExportToOutputDevice( Obj, NULL, Out, TEXT("T3D"), Indent+1 );
									Obj->SetFlags( RF_TagImp );
								}
							}
		
							Out.Logf( TEXT("%s %s(%i)=%s\r\n"), appSpc(Indent), It->GetName(), k, Value );
						}
					}
				}
				else
				{
					// Export single element.
					if( It->ExportText( j, Value, Object, (DiffClass && DiffClass->IsChildOf(It.GetStruct())) ? Diff : NULL, PPF_Delimited ) )
					{
						if
						(	(It->IsA(UObjectProperty::StaticClass()))
						&&	(It->PropertyFlags & CPF_ExportObject) )
						{
							UObject* Obj = *(UObject **)((BYTE*)Object + It->Offset + j*It->ElementSize);					
							if( Obj && !(Obj->GetFlags() & RF_TagImp) )
							{
								// Don't export more than once.
								UExporter::ExportToOutputDevice( Obj, NULL, Out, TEXT("T3D"), Indent+1 );
								Obj->SetFlags( RF_TagImp );
							}
						}						

						if( It->ArrayDim == 1 )
							Out.Logf( TEXT("\t%s=%s\r\n"), It->GetName(), Value );
						else
							Out.Logf( TEXT("\t%s(%i)=%s\r\n"), It->GetName(), j, Value );
					}
				}
			}
		}
	}
	unguardf(( TEXT("(%s)"), ThisName ));
}

//
// Initialize script execution.
//
void UObject::InitExecution()
{
	guard(UObject::InitExecution);
	check(GetClass()!=NULL);

	if( StateFrame )
		delete StateFrame;
	StateFrame = new(TEXT("ObjectStateFrame"))FStateFrame( this );
	SetFlags( RF_HasStack );

	unguardobj;
}

INT UObject::GetPropertiesSize()
{
	guard(UObject::GetPropertiesSize);
	check(StaticClass()!=NULL);
	return StaticClass()->GetPropertiesSize();
	unguardobj;
}

//
// Command line.
//
UBOOL UObject::ScriptConsoleExec( const TCHAR* Str, FOutputDevice& Ar, UObject* Executor )
{
	guard(UObject::ScriptConsoleExec);

	// No script execution?
	if( !GIsScriptable )
		return 0;

	// Find UnrealScript exec function.
	TCHAR MsgStr[NAME_SIZE];
	FName Message;
	UFunction* Function;
	if
	(	!ParseToken(Str,MsgStr,ARRAY_COUNT(MsgStr),1)
	||	(Message=FName(MsgStr,FNAME_Find))==NAME_None 
	||	(Function=FindFunction(Message))==NULL 
	||	!(Function->FunctionFlags & FUNC_Exec) )
		return 0;

	// Parse all function parameters.
	BYTE* Parms = (BYTE*)appAlloca(Function->ParmsSize);
	appMemzero( Parms, Function->ParmsSize );
	UBOOL Failed = 0;
	INT Count = 0;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It,Count++ )
	{
		if( Count==0 && Executor )
		{
			UObjectProperty* Op = Cast<UObjectProperty>(*It);
			if( Op && Executor->IsA(Op->PropertyClass) )
			{
				// First parameter is implicit reference to object executing the command.
				*(UObject**)(Parms + It->Offset) = Executor;
				continue;
			}
		}
		ParseNext( &Str );
		Str = It->ImportText( Str, Parms+It->Offset, PPF_Localized );
		if( !Str )
		{
			if( !(It->PropertyFlags & CPF_OptionalParm) )
			{
				Ar.Logf( LocalizeError(TEXT("BadProperty"),TEXT("Core")), *Message, It->GetName() );
				Failed = 1;
			}
			break;
		}
	}
	if( !Failed )
		ProcessEvent( Function, Parms );
	//!!destructframe see also UObject::ProcessEvent
	{for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
		It->DestroyValue( Parms + It->Offset );}

	// Success.
	return 1;
	unguardobj;
}

//
// Find an UnrealScript field.
//warning: Must be safe with class default metaobjects.
//
UField* UObject::FindObjectField( FName InName, UBOOL Global )
{
	guardSlow(UObject::StaticFindField);
#if 1 /* Fast, VF-hashed version */
	INT iHash = InName.GetIndex() & (UField::HASH_COUNT-1);

	// Search current state scope.
	if( StateFrame && StateFrame->StateNode && !Global )
		for( UField* Node=StateFrame->StateNode->VfHash[iHash]; Node; Node=Node->HashNext )
			if( Node->GetFName()==InName )
				return Node;

	// Search the global scope.
	for( UField* Node=GetClass()->VfHash[iHash]; Node; Node=Node->HashNext )
		if( Node->GetFName()==InName )
			return Node;

	// Not found.
	return NULL;
#else /* Slow, unhashed version */
	// Search current state scope.
	if( StateFrame && StateFrame->StateNode && !Global )
		for( TFieldIterator<UField> It(StateFrame->StateNode); It; ++It )
			if( It->GetFName()==InName )
				return *It;

	// Search the global scope.
	for( TFieldIterator<UField> It(GetClass()); It; ++It )
		if( It->GetFName()==InName )
			return *It;

	// Not found.
	return NULL;
#endif
	unguardfSlow(( TEXT("%s (node %s)"), GetFullName(), *InName ));
}
UFunction* UObject::FindFunction( FName InName, UBOOL Global )
{
	guardSlow(UObject::FindFunction);
    return Cast<UFunction>( FindObjectField( InName, Global ),CLASS_IsAUFunction );
	unguardfSlow(( TEXT("%s (function %s)"), GetFullName(), *InName ));
}
UFunction* UObject::FindFunctionChecked( FName InName, UBOOL Global )
{
	guardSlow(UObject::FindFunctionChecked);
	if( !GIsScriptable )
		return NULL;
    UFunction* Result = Cast<UFunction>( FindObjectField( InName, Global ),CLASS_IsAUFunction );
	if( !Result )
		appErrorf( TEXT("Failed to find function %s in %s"), *InName, GetFullName() );
	return Result;
	unguardfSlow(( TEXT("%s (function %s)"), GetFullName(), *InName ));
}
UState* UObject::FindState( FName InName )
{
	guardSlow(UObject::FindState);
    return Cast<UState>( FindObjectField( InName, 1 ),CLASS_IsAUState );
	unguardfSlow(( TEXT("%s (state %s)"), GetFullName(), *InName ));
}
IMPLEMENT_CLASS(UObject);

/*-----------------------------------------------------------------------------
	UObject configuration.
-----------------------------------------------------------------------------*/

//
// Load configuration.
//warning: Must be safe on class-default metaobjects.
//
void UObject::LoadConfig( UBOOL Propagate, UClass* Class, const TCHAR* InFilename )
{
	guard(UObject::LoadConfig);
	if( !Class )
		Class = GetClass();
	if( !(Class->ClassFlags & CLASS_Config) )
		return;
	if( Propagate && Class->GetSuperClass() )
		LoadConfig( Propagate, Class->GetSuperClass(), InFilename );
	UBOOL PerObject = ((GetClass()->ClassFlags & CLASS_PerObjectConfig) && GetIndex()!=INDEX_NONE);
	const TCHAR* Filename
	=	InFilename
	?	InFilename
	:	(PerObject && Outer!=GObjTransientPkg)
	?	GetOuter()->GetName()
	:	*GetClass()->ClassConfigName;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Class); It; ++It )
	{
		if( It->PropertyFlags & CPF_Config )
		{
			TCHAR TempKey[256], Value[1024]=TEXT("");
			UClass*         BaseClass = (It->PropertyFlags & CPF_GlobalConfig) ? It->GetOwnerClass() : Class;
			const TCHAR*    Section   = PerObject ? GetName() : BaseClass->GetPathName();
			const TCHAR*    Key       = It->ArrayDim==1 ? It->GetName() : TempKey;
			UArrayProperty* Array     = Cast<UArrayProperty>( *It );
			UMapProperty*   Map       = Cast<UMapProperty>( *It );
			if( Array )
			{
				TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( Section, 0, 1, Filename );
				if( Sec )
				{
					TArray<FString> List;
					Sec->MultiFind( FString(Key), List );
					FArray* Ptr  = (FArray*)((BYTE*)this + It->Offset);
					INT     Size = Array->Inner->ElementSize;
					Array->DestroyValue( Ptr );
					Ptr->AddZeroed( Size, List.Num() );
					for( INT i=List.Num()-1,c=0; i>=0; i--,c++ )
						Array->Inner->ImportText( *List(i), (BYTE*)Ptr->GetData() + c*Size, 0 );
				}
			}
			else if( Map )
			{
				TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( Section, 0, 1, Filename );
				if( Sec )
				{
					TArray<FString> List;
					Sec->MultiFind( Key, List );
					//FArray* Ptr  = (FArray*)((BYTE*)this + It->Offset);
					//INT     Size = Array->Inner->ElementSize;
					//Map->DestroyValue( Ptr );//!!won't do dction
					//Ptr->AddZeroed( Size, List.Num() );
					//for( INT i=List.Num()-1,c=0; i>=0; i--,c++ )
					//	Array->Inner->ImportText( *List(i), (BYTE*)Ptr->GetData() + c*Size, 0 );
				}
			}
			else for( INT i=0; i<It->ArrayDim; i++ )
			{
				if( It->ArrayDim!=1 )
					appSprintf( TempKey, TEXT("%s[%i]"), It->GetName(), i );
				if( GConfig->GetString( Section, Key, Value, ARRAY_COUNT(Value), Filename ) )
					It->ImportText( Value, (BYTE*)this + It->Offset + i*It->ElementSize, 0 );
			}
		}
	}
	unguardobj;
}

static void LoadLocalizedProp( UProperty* Prop, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, BYTE* Data );
static void LoadLocalizedStruct( UStruct* Struct, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, BYTE* Data );

static void LoadLocalizedProp( UProperty* Prop, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, BYTE* Data )
{
	guard(LoadLocalizedProp);
	UStructProperty* StructProperty = Cast<UStructProperty>( Prop );
	
	if( StructProperty )
	{
        LoadLocalizedStruct(StructProperty->Struct, IntName, SectionName, KeyPrefix, Data );
        return;
    }

    if( !(Prop->PropertyFlags & CPF_Localized) ) 
        return;

	const TCHAR* LocalizedText = Localize( SectionName, KeyPrefix, IntName, NULL, 1 );

	if( *LocalizedText )
		Prop->ImportText( LocalizedText, Data, 0 );
	unguard;
}

static void LoadLocalizedStruct( UStruct* Struct, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, BYTE* Data )
{
	guard(LoadLocalizedStruct);
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It( Struct ); It; ++It )
	{
	    UProperty* Prop = *It;

	    for( INT i = 0; i < Prop->ArrayDim; i++ )
	    {
    	    FString NewPrefix;

            if( KeyPrefix )
                NewPrefix = FString::Printf( TEXT("%s."), KeyPrefix );

	        if( Prop->ArrayDim > 1 )
                NewPrefix += FString::Printf( TEXT("%s[%d]"), Prop->GetName(), i );
	        else
                NewPrefix += Prop->GetName();

            BYTE* NewData = Data + (Prop->Offset) + (i * Prop->ElementSize );

            LoadLocalizedProp( Prop, IntName, SectionName, *NewPrefix, NewData );
	    }
	}
	unguard;
}

//
// Load localized text.
// Warning: Must be safe on class-default metaobjects.
//

void UObject::LoadLocalized()
{
	guard(UObject::LoadLocalized);

	UClass* Class = GetClass();
	checkSlow( Class );
	
	if( !(Class->ClassFlags & CLASS_Localized) )
		return;
	if( GIsEditor )
		return;

	if( GetIndex()!=INDEX_NONE && ObjectFlags&RF_PerObjectLocalized && GetOuter()->GetOuter() )
	{
		// Localize as an instanced subobject
	    LoadLocalizedStruct( Class, GetOuter()->GetOuter()->GetName(), GetOuter()->GetName(), GetName(), (BYTE*)this );
	}
	else
		{
		const TCHAR* IntName     = GetIndex()==INDEX_NONE ? Class->GetOuter()->GetName() : GetOuter()->GetName();
		const TCHAR* SectionName = GetIndex()==INDEX_NONE ? Class->GetName()             : GetName();
	    LoadLocalizedStruct( Class, IntName, SectionName, NULL, (BYTE*)this );
	}
	unguardobj;
}

//
// Save configuration.
//warning: Must be safe on class-default metaobjects.
//!!may benefit from hierarchical propagation, deleting keys that match superclass...not sure what's best yet.
//
void UObject::SaveConfig( DWORD Flags, const TCHAR* InFilename )
{
	guard(UObject::SaveConfig);
	UBOOL PerObject = ((GetClass()->ClassFlags & CLASS_PerObjectConfig) && GetIndex()!=INDEX_NONE);
	const TCHAR* Filename
	=	InFilename
	?	InFilename
	:	(PerObject && Outer!=GObjTransientPkg)
	?	GetOuter()->GetName()
	:	*GetClass()->ClassConfigName;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{
		if( (It->PropertyFlags & Flags)==Flags )
		{
			TCHAR TempKey[256], Value[1024]=TEXT("");
			const TCHAR*    Key     = It->GetName();
			const TCHAR*    Section = PerObject ? GetName() : (It->PropertyFlags & CPF_GlobalConfig) ? It->GetOwnerClass()->GetPathName() : GetClass()->GetPathName();
			UArrayProperty* Array   = Cast<UArrayProperty>( *It );
			UMapProperty*   Map     = Cast<UMapProperty>( *It );
			if( Array )
			{
				TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( Section, 1, 0, Filename );
				check(Sec);
				Sec->Remove( Key );
				FArray* Ptr  = (FArray*)((BYTE*)this + It->Offset);
				INT     Size = Array->Inner->ElementSize;
				for( INT i=0; i<Ptr->Num(); i++ )
				{
					TCHAR Buffer[1024]=TEXT("");
					BYTE* Dest = (BYTE*)Ptr->GetData() + i*Size;
					Array->Inner->ExportTextItem( Buffer, Dest, Dest, 0 );
					Sec->Add( Key, Buffer );
				}
			}
			else if( Map )
			{
				TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( Section, 1, 0, Filename );
				check(Sec);
				Sec->Remove( Key );
				//FArray* Ptr  = (FArray*)((BYTE*)this + It->Offset);
				//INT     Size = Array->Inner->ElementSize;
				//for( INT i=0; i<Ptr->Num(); i++ )
				//{
				//	TCHAR Buffer[1024]="";
				//	BYTE* Dest = (BYTE*)Ptr->GetData() + i*Size;
				//	Array->Inner->ExportTextItem( Buffer, Dest, Dest, 0 );
				//	Sec->Add( Key, Buffer );
				//}
			}
			else for( INT Index=0; Index<It->ArrayDim; Index++ )
			{
				if( It->ArrayDim!=1 )
				{
					appSprintf( TempKey, TEXT("%s[%i]"), It->GetName(), Index );
					Key = TempKey;
				}
				It->ExportText( Index, Value, (BYTE*)this, (BYTE*)this, 0 );
				GConfig->SetString( Section, Key, Value, Filename );
			}
		}
	}
	GConfig->Flush( 0 );
	UClass* BaseClass;
	for( BaseClass=GetClass(); BaseClass->GetSuperClass(); BaseClass=BaseClass->GetSuperClass() )
		if( !(BaseClass->GetSuperClass()->ClassFlags & CLASS_Config) )
			break;
	if( BaseClass )
		for( TObjectIterator<UClass> It; It; ++It )
			if( It->IsChildOf(BaseClass) )
				It->GetDefaultObject()->LoadConfig( 1 );//!!propagate??
	unguardobj;
}

//
// Reset configuration.
//
void UObject::ResetConfig( UClass* Class )
{
	guard(UObject::ResetConfig);
	const TCHAR* SrcFilename=NULL;
	if( Class->ClassConfigName==NAME_System )
		SrcFilename = TEXT("Default.ini");
	else if( Class->ClassConfigName==NAME_User )
		SrcFilename = TEXT("DefUser.ini");
	else
		return;
	TCHAR Buffer[32000];
	if( GConfig->GetSection( Class->GetPathName(), Buffer, ARRAY_COUNT(Buffer), SrcFilename  ) )
	{
		TCHAR* NewKey;
		for( TCHAR* Key=Buffer; *Key; Key=NewKey )
		{	
			NewKey = Key + appStrlen(Key) + 1;
			TCHAR* Value=appStrstr(Key,TEXT("="));
			if( Value )
			{
				*Value++ = 0;
				GConfig->SetString( Class->GetPathName(), Key, Value, *Class->ClassConfigName );
			}
		}
	}
	for( TObjectIterator<UClass> ItC; ItC; ++ItC )
		if( ItC->IsChildOf(Class) )
			ItC->GetDefaultObject()->LoadConfig( 1 );//!!propagate?
	for( TObjectIterator<UObject> ItO; ItO; ++ItO )
		if( ItO->IsA(Class) )
			{ItO->LoadConfig( 1 ); ItO->PostEditChange();}//!!propagate?
	unguard;
}

/*-----------------------------------------------------------------------------
	UObject IUnknown implementation.
-----------------------------------------------------------------------------*/

// Note: This are included as part of UObject in case we ever want
// to make Unreal objects into Component Object Model objects.
// The Unreal framework is set up so that it would be easy to componentize
// everything, but there is not yet any benefit to doing so, thus these
// functions aren't implemented.

//
// Query the object on behalf of an Ole client.
//
DWORD STDCALL UObject::QueryInterface( const FGuid &RefIID, void **InterfacePtr )
{
	guard(UObject::QueryInterface);
	// This is not implemented and might not ever be.
	*InterfacePtr = NULL;
	return 0;
	unguardobj;
}

//
// Add a reference to the object on behalf of an Ole client.
//
DWORD STDCALL UObject::AddRef()
{
	guard(UObject::AddRef);
	// This is not implemented and might not ever be.
	return 0;
	unguardobj;
}

//
// Release the object on behalf of an Ole client.
//
DWORD STDCALL UObject::Release()
{
	guard(UObject::Release);
	// This is not implemented and might not ever be.
	return 0;
	unguardobj;
}

/*-----------------------------------------------------------------------------
	Mo Functions.
-----------------------------------------------------------------------------*/

//
// Object accessor.
//
UObject* UObject::GetIndexedObject( INT Index )
{
	guardSlow(UObject::GetIndexedObject);
	if( Index>=0 && Index<GObjObjects.Num() )
		return GObjObjects(Index);
	else
		return NULL;
	unguardSlow;
}

//
// Find an optional object.
//
UObject* UObject::StaticFindObject( UClass* ObjectClass, UObject* InObjectPackage, const TCHAR* InName, UBOOL ExactClass )
{
	guard(UObject::StaticFindObject);

	// Resolve the object and package name.
	UObject* ObjectPackage = InObjectPackage!=ANY_PACKAGE ? InObjectPackage : NULL;
	if( !ResolveName( ObjectPackage, InName, 0, 0 ) )
		return NULL;

	// Make sure it's an existing name.
	FName ObjectName(InName,FNAME_Find);
	if( ObjectName==NAME_None )
		return NULL;

	// Find in the specified package.
	INT iHash = GetObjectHash( ObjectName, ObjectPackage ? ObjectPackage->GetIndex() : 0 );
	for( UObject* Hash=GObjHash[iHash]; Hash!=NULL; Hash=Hash->HashNext )
	{
		/*
		InName: the object name to search for. Two possibilities.
			A = No dots. ie: 'S_Actor', a texture in Engine
			B = Dots. ie: 'Package.Name' or 'Package.Group.Name', or an even longer chain of outers. The first one needs to be relative to InObjectPackage.
			I'll define InName's package to be everything before the last period, or "" otherwise.
		InObjectPackage: the package or Outer to look for the object in. Can be ANY_PACKAGE, or NULL. Three possibilities:
			A = Non-null. Search for the object relative to this package.
			B = Null. We're looking for the object, as specified exactly in InName.
			C = ANY_PACKAGE. Search anywhere for the package (resrictions on functionality, see below)
		ObjectPackage: The package we need to be searching in. NULL means we don't care what package to search in
			InName.A &&  InObjectPackage.C ==> ObjectPackage = NULL
			InName.A && !InObjectPackage.C ==> ObjectPackage = InObjectPackage
			InName.B &&  InObjectPackage.C ==> ObjectPackage = InName's package
			InName.B && !InObjectPackage.C ==> ObjectPackage = InName's package, but as a subpackage of InObjectPackage
		*/
		if
		(	(Hash->GetFName()==ObjectName)
			/*If there is no package (no InObjectPackage specified, and InName's package is "") 
			and the caller specified any_package, then accept it, regardless of its package.*/ 
		&&	(	(!ObjectPackage && InObjectPackage==ANY_PACKAGE)
			/*Or, if the object is directly within this package, (whether that package is non-NULL or not), 
			then accept it immediately.*/ 
			||	(Hash->Outer == ObjectPackage)
			/*Or finally, if the caller specified any_package, but they specified InName's package, 
			then check if the object IsIn (is recursively within) InName's package. 
			We don't check if IsIn if ObjectPackage is NULL, since that would be true for every object. 
			This check also allows you to specify Package.Name to access an object in Package.Group.Name.*/ 
			||	(InObjectPackage == ANY_PACKAGE && ObjectPackage && Hash->IsIn(ObjectPackage)) )
		&&	(ObjectClass==NULL || (ExactClass ? Hash->GetClass()==ObjectClass : Hash->IsA(ObjectClass))) )
			return Hash;
	}

	// Not found.
	return NULL;
	unguard;
}

//
// Find an object; can't fail.
//
UObject* UObject::StaticFindObjectChecked( UClass* ObjectClass, UObject* ObjectParent, const TCHAR* InName, UBOOL ExactClass )
{
	guard(UObject::StaticFindObjectChecked);
	UObject* Result = StaticFindObject( ObjectClass, ObjectParent, InName, ExactClass );
	if( !Result )
		appErrorf( LocalizeError(TEXT("ObjectNotFound"),TEXT("Core")), ObjectClass->GetName(), ObjectParent==ANY_PACKAGE ? TEXT("Any") : ObjectParent ? ObjectParent->GetName() : TEXT("None"), InName );
	return Result;
	unguard;
}

//
// Binary initialize object properties to zero or defaults.
//
void UObject::InitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* Defaults, INT DefaultsCount, UObject* DestObject, UObject* SuperObject )
{
	guard(UObject::InitProperties);
	check(DataCount>=sizeof(UObject));
	INT Inited = sizeof(UObject);

	// Find class defaults if no template was specified.
	//warning: At startup, DefaultsClass->Defaults.Num() will be zero for some native classes.
	guardSlow(FindDefaults);
	if( !Defaults && DefaultsClass && DefaultsClass->Defaults.Num() )
	{
		Defaults      = &DefaultsClass->Defaults(0);
		DefaultsCount =  DefaultsClass->Defaults.Num();
	}
	unguardSlow;

	// Copy defaults.
	guardSlow(DefaultsFill);
	if( Defaults )
	{
		checkSlow(DefaultsCount>=Inited);
		checkSlow(DefaultsCount<=DataCount);
		appMemcpy( Data+Inited, Defaults+Inited, DefaultsCount-Inited );
		Inited = DefaultsCount;
	}
	unguardSlow;

	// Zero-fill any remaining portion.
	guardSlow(ZeroFill);
	if( Inited < DataCount )
		appMemzero( Data+Inited, DataCount-Inited );
	unguardSlow;

	// This is a duplicate. Set all transients to their default value.
	if( SuperObject )
	{
		checkSlow(DestObject);
		BYTE* ClassDefaults = &DefaultsClass->Defaults(0);
		for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(DestObject->GetClass()); It; ++It )
			if( It->PropertyFlags & CPF_Transient )
				appMemcpy( Data + It->Offset, ClassDefaults + It->Offset, It->ArrayDim*It->ElementSize );
	}

	// Construct anything required.
	guardSlow(ConstructProperties);
	if( DefaultsClass )
	{
		for( UProperty* P=DefaultsClass->ConstructorLink; P; P=P->ConstructorLinkNext )
		{
			if( P->Offset < DefaultsCount )
			{
				appMemzero( Data + P->Offset, P->GetSize() );//bad for bools, but not a real problem because they aren't constructed!!
				P->CopyCompleteValue( Data + P->Offset, Defaults + P->Offset, SuperObject ? SuperObject : DestObject );
			}
		}
	}
	unguardSlow;

	unguard;
}

//
// Destroy properties.
//
void UObject::ExitProperties( BYTE* Data, UClass* Class )
{
	UProperty* P=NULL;
	guard(UObject::ExitProperties);
	for( P=Class->ConstructorLink; P; P=P->ConstructorLinkNext )
		P->DestroyValue( Data + P->Offset );
	unguardf(( TEXT("(%s)"), P->GetFullName() ));
}

//
// Init class default object.
//
void UObject::InitClassDefaultObject( UClass* InClass, UBOOL SetOuter )
{
	guard(UObject::InitClassDefaultObject);

	// Init UObject portion.
	appMemset( this, 0, sizeof(UObject) );
	*(void**)this = *(void**)InClass;
	Class         = InClass;
	Index         = INDEX_NONE;

	// Init post-UObject portion.
	if( SetOuter )
		Outer = InClass->GetOuter();
	InitProperties( (BYTE*)this, InClass->GetPropertiesSize(), InClass->GetSuperClass(), NULL, 0, SetOuter ? this : NULL );

	unguard;
}

//
// Global property setting.
//
void UObject::GlobalSetProperty( const TCHAR* Value, UClass* Class, UProperty* Property, INT Offset, UBOOL Immediate )
{
	guard(UObject::GlobalSetProperty);

	// Apply to existing objects of the class, with notification.
	if( Immediate )
	{
		for( FObjectIterator It; It; ++It )
		{
			if( It->IsA(Class) )
			{
				Property->ImportText( Value, (BYTE*)*It + Offset, PPF_Localized );
				It->PostEditChange();
			}
		}
	}

	// Apply to defaults.
	Property->ImportText( Value, &Class->Defaults(Offset), PPF_Localized );
	Class->GetDefaultObject()->SaveConfig();

	unguard;
}

/*-----------------------------------------------------------------------------
	Object registration.
-----------------------------------------------------------------------------*/

//
// Preregister an object.
//warning: Sometimes called at startup time.
//
void UObject::Register()
{
	guard(UObject::Register);
	check(GObjInitialized);

	// Get stashed registration info.
	const TCHAR* InOuter = *(const TCHAR**)&Outer;

#if SERIAL_POINTER_INDEX
	INT idx = *(INT*)&Name;
	check(idx < GTotalSerializedPointers);
	const TCHAR* InName  = (const TCHAR*) GSerializedPointers[idx];
#else
	const TCHAR* InName  = *(const TCHAR**)&Name;
#endif

	// Set object properties.
	Outer        = CreatePackage(NULL,InOuter);
	Name         = InName;
	_LinkerIndex = INDEX_NONE;

	// Validate the object.
	if( Outer==NULL )
		appErrorf( TEXT("Autoregistered object %s is unpackaged"), GetFullName() );
	if( GetFName()==NAME_None )
		appErrorf( TEXT("Autoregistered object %s has invalid name"), GetFullName() );
	if( StaticFindObject( NULL, GetOuter(), GetName() ) )
		appErrorf( TEXT("Autoregistered object %s already exists"), GetFullName() );

	// Add to the global object table.
	AddObject( INDEX_NONE );

	unguard;
}

//
// Handle language change.
//
void UObject::LanguageChange()
{
	guard(UObject::LanguageChange);
	LoadLocalized();
	unguard;
}

/*-----------------------------------------------------------------------------
	StaticInit & StaticExit.
-----------------------------------------------------------------------------*/

void SerTest( FArchive& Ar, DWORD& Value, DWORD Max )
{
	DWORD NewValue=0;
	for( DWORD Mask=1; NewValue+Mask<Max && Mask; Mask*=2 )
	{
		BYTE B = ((Value&Mask)!=0);
		Ar.SerializeBits( &B, 1 );
		if( B )
			NewValue |= Mask;
	}
	Value = NewValue;
}

//
// Init the object manager and allocate tables.
//
void UObject::StaticInit()
{
	guard(UObject::StaticInit);
	GObjNoRegister = 1;

	// Checks.
	check(sizeof(BYTE)==1);
	check(sizeof(SBYTE)==1);
	check(sizeof(_WORD)==2);
	check(sizeof(DWORD)==4);
	check(sizeof(QWORD)==8);
	check(sizeof(ANSICHAR)==1);
	check(sizeof(UNICHAR)==2);
	check(sizeof(SWORD)==2);
	check(sizeof(INT)==4);
	check(sizeof(SQWORD)==8);
	check(sizeof(UBOOL)==4);
	check(sizeof(FLOAT)==4);
	check(sizeof(DOUBLE)==8);
	check(ENGINE_MIN_NET_VERSION<=ENGINE_VERSION);

	// Development.
	GCheckConflicts = ParseParam(appCmdLine(),TEXT("CONFLICTS"));
	GNoGC           = ParseParam(appCmdLine(),TEXT("NOGC"));

	// Init hash.
	for( INT i=0; i<ARRAY_COUNT(GObjHash); i++ )
		GObjHash[i] = NULL;

	// If statically linked, initialize registrants.
	#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS;
	#endif
	
	// Note initialized.
	GObjInitialized = 1;

	// Add all autoregistered classes.
	ProcessRegistrants();

	// Allocate special packages.
	GObjTransientPkg = new( NULL, TEXT("Transient") )UPackage;
	GObjTransientPkg->AddToRoot();

	debugf( NAME_Init, TEXT("Object subsystem initialized") );
	unguard;
}

//
// Process all objects that have registered.
//
void UObject::ProcessRegistrants()
{
	guard(UObject::ProcessRegistrants);
	if( ++GObjRegisterCount==1 )
	{
		// Make list of all objects to be registered.
		for( ; GAutoRegister; GAutoRegister=*(UObject **)&GAutoRegister->_LinkerIndex )
			GObjRegistrants.AddItem( GAutoRegister );
		for( INT i=0; i<GObjRegistrants.Num(); i++ )
			GObjRegistrants(i)->ConditionalRegister();
		GObjRegistrants.Empty();
		check(!GAutoRegister);
	}
	GObjRegisterCount--;
	unguard;
}

//
// Profile comparator.
//
#if USCRIPT_PROFILE
static QSORT_RETURN CDECL CompareFunctions( const UFunction** A, const UFunction** B )
{
#if 1 //NEW: added by Legend on 4/12/2000
	// fix overflow in return value (cycles are 64-bit SQWORDS, QSORT_RETURN is an INT)
	if( (*B)->Cycles > (*A)->Cycles )
		return 1;
	else if( (*B)->Cycles == (*A)->Cycles )
		return 0;
	else
		return -1;
#else
	return (*B)->Cycles - (*A)->Cycles;
#endif
}

static void DumpStats()
{
	if( ParseParam(appCmdLine(),TEXT("PROFILE")) && !GIsCriticalError )
	{
		TArray<UFunction*> List;
		for( TObjectIterator<UFunction> ItF; ItF; ++ItF )
			if( ItF->Calls!=0 )
				List.AddItem( *ItF );

		debugf( TEXT("Profile of %i ticks (%i functions):"), (INT)GTicks, List.Num() );
		debugf( TEXT("                                                                        Function     usec/tick        cyc/call    calls/tick         calls  %%Total") );
		debugf( TEXT("--------------------------------------------------------------------------------  ------------    ------------  ------------  ------------  ------") );

		appQsort( &List(0), List.Num(), sizeof(List(0)), (QSORT_COMPARE)CompareFunctions );

		DOUBLE TotalCycles = 0.0;
		for( INT i=0; i<List.Num(); i++ )
			TotalCycles += List(i)->Cycles;

		for( INT i=0; i<List.Num(); i++ )
		{
   			debugf
   			(
   				TEXT("%80s  %12.3f  %14.1f  %12.3f  %12.0f  %6.3f"),
   
   				List(i)->GetPathName(),
   				1000.0*1000.0*GSecondsPerCycle*(DOUBLE)List(i)->Cycles/(DOUBLE)GTicks,
   				(DOUBLE)List(i)->Cycles/(DOUBLE)List(i)->Calls,
   				(DOUBLE)List(i)->Calls/(DOUBLE)GTicks,
   				(DOUBLE)List(i)->Calls,
				(DOUBLE)List(i)->Cycles / TotalCycles * 100
   			);
		}

		// Reset stats.
		for( TObjectIterator<UFunction> It; It; ++It )
			It->Calls = It->Cycles=0;
		GTicks=1;
	}
}
#endif

//
// Shut down the object manager.
//
void UObject::StaticExit()
{
	guard(UObject::StaticExit);
	check(GObjLoaded.Num()==0);
	check(GObjRegistrants.Num()==0);
	check(!GAutoRegister);

	// Dump all profile results.
#if USCRIPT_PROFILE
	DumpStats();
#endif

	// Cleanup root.
	GObjTransientPkg->RemoveFromRoot();

	// Tag all objects as unreachable.
	for( FObjectIterator It; It; ++It )
		It->SetFlags( RF_Unreachable | RF_TagGarbage );

	// Tag all names as unreachable.
	for( INT i=0; i<FName::GetMaxNames(); i++ )
		if( FName::GetEntry(i) )
			FName::GetEntry(i)->Flags |= RF_Unreachable;

	// Purge all objects.
	GExitPurge = 1;
	PurgeGarbage();
	GObjObjects.Empty();

	// Empty arrays to prevent falsely-reported memory leaks.
	GObjLoaded			.Empty();
	GObjObjects			.Empty();
	GObjAvailable		.Empty();
	GObjLoaders			.Empty();
	GObjRoot			.Empty();
	GObjRegistrants		.Empty();
	GObjPreferences		.Empty();
	GObjDrivers			.Empty();

	GObjInitialized = 0;
	debugf( NAME_Exit, TEXT("Object subsystem successfully closed.") );
	unguard;
}

/*-----------------------------------------------------------------------------
	UObject Tick.
-----------------------------------------------------------------------------*/

//
// Mark one unit of passing time. This is used to update the object
// caching status. This must be called when the object manager is
// in a clean state (outside of all code which retains pointers to
// object data that was gotten).
//
void UObject::StaticTick()
{
	guard(UObject::StaticTick);
	check(GObjBeginLoadCount==0);

	// Check natives.
	CORE_API extern int GNativeDuplicate;
	if( GNativeDuplicate )
		appErrorf( TEXT("Duplicate native registered: %i"), GNativeDuplicate );

	CORE_API extern int GCastDuplicate;
	if( GCastDuplicate )
		appErrorf( TEXT("Duplicate cast registered: %i"), GCastDuplicate );

	unguard;
}

/*-----------------------------------------------------------------------------
   Shutdown.
-----------------------------------------------------------------------------*/

//
// Make sure this object has been shut down.
//
void UObject::ConditionalShutdownAfterError()
{
	if( !(GetFlags() & RF_ErrorShutdown) )
	{
		SetFlags( RF_ErrorShutdown );
		try
		{
			ShutdownAfterError();
		}
		catch( ... )
		{
			debugf( NAME_Exit, TEXT("Double fault in object ShutdownAfterError") );
		}
	}
}

//
// After a critical error, shutdown all objects which require
// mission-critical cleanup, such as restoring the video mode,
// releasing hardware resources.
//
void UObject::StaticShutdownAfterError()
{
	guard(UObject::StaticShutdownAfterError);
	if( GObjInitialized )
	{
		static UBOOL Shutdown=0;
		if( Shutdown )
			return;
		Shutdown = 1;
		debugf( NAME_Exit, TEXT("Executing UObject::StaticShutdownAfterError") );
		try
		{
			for( INT i=0; i<GObjObjects.Num(); i++ )
				if( GObjObjects(i) )
					GObjObjects(i)->ConditionalShutdownAfterError();
		}
		catch( ... )
		{
			debugf( NAME_Exit, TEXT("Double fault in object manager ShutdownAfterError") );
		}
	}
	unguard;
}

//
// Bind package to DLL.
//warning: Must only find packages in the \Unreal\System directory!
//
void UObject::BindPackage( UPackage* Pkg )
{
	guard(UPackage::BindPackage);
	if( !Pkg->DllHandle && !Pkg->GetOuter() && !Pkg->AttemptedBind )
	{
		TCHAR PathName[256];
		appSprintf( PathName, TEXT("%s%s"), appBaseDir(), Pkg->GetName() );
		Pkg->AttemptedBind  = 1;
		GObjNoRegister      = 0;
		Pkg->DllHandle      = appGetDllHandle( PathName );
		GObjNoRegister      = 1;
		if( Pkg->DllHandle )
		{
			#if !__STATIC_LINK
			debugf( NAME_Progress, TEXT("Bound to %s%s"), Pkg->GetName(), DLLEXT );
			#endif
			ProcessRegistrants();
			debugf( NAME_Progress, TEXT("Calling PackageInit") );
			void (*PackageInit)();
			PackageInit = (void(*)())appGetDllExport(Pkg->DllHandle, TEXT("PackageInit"));
			if (PackageInit)
			{
				debugf( NAME_Progress, TEXT("Found packageinit") );
				PackageInit();
			}
		}
	}
	unguard;
}



/*---------------------------------------------------------------------------------
   By-name member access functionality.    Useful for non-native object dissection.
-----------------------------------------------------------------------------------*/

UBOOL UObject::FindArrayProperty( FString Name, FArray** Array, INT* ElementSize )
{
	BYTE* Base = (BYTE*) this;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{				
		UArrayProperty* ArrayProperty;
		if( (ArrayProperty=Cast<UArrayProperty>(*It)) != NULL )
		{			
			FString ArrayName = It->GetName();					
			if( ArrayName == Name)
			{						
				*Array = (FArray*)(Base+It->Offset);
				*ElementSize = ArrayProperty->Inner->ElementSize;
				return true;
			}
		}
	}
	return false;
}

UBOOL UObject::FindObjectProperty( FString Name, UObject** Object )
{
	BYTE* Base = (BYTE*) this;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{				
		UObjectProperty* ObjectProperty;
		if( (ObjectProperty=Cast<UObjectProperty>(*It)) != NULL )
		{			
			FString ObjectName = It->GetName();					
			if( ObjectName == Name)
			{						
				*Object = (UObject*)(Base+It->Offset);
				return true;
			}
		}
	}
	return false;
}

UBOOL UObject::FindStructProperty( FString Name, UStruct** Struct )
{
	BYTE* Base = (BYTE*) this;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{				
		UStructProperty* StructProperty;
		if( (StructProperty=Cast<UStructProperty>(*It)) != NULL )
		{			
			FString StructName = It->GetName();					
			if( StructName == Name)
			{						
				*Struct = (UStruct*)(Base+It->Offset);
				return true;
			}
		}
	}
	return false;
}

UBOOL UObject::FindFloatProperty( FString Name, FLOAT* FloatVar )
{
	BYTE* Base = (BYTE*) this;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{				
		UFloatProperty* FloatProperty;
		if( (FloatProperty=Cast<UFloatProperty>(*It)) != NULL )
		{			
			FString FloatName = It->GetName();					
			if( FloatName == Name)
			{						
				*FloatVar = *(FLOAT*)(Base+It->Offset);
				return true;
			}
		}
	}
	return false;
}

UBOOL UObject::FindIntProperty( FString Name, INT* IntVar )
{
	BYTE* Base = (BYTE*) this;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{				
		UIntProperty* IntProperty;
		if( (IntProperty=Cast<UIntProperty>(*It)) != NULL )
		{			
			FString IntName = It->GetName();					
			if( IntName == Name)
			{						
				*IntVar = *(INT*)(Base+It->Offset);
				return true;
			}
		}
	}
	return false;
}

UBOOL UObject::FindBoolProperty( FString Name, UBOOL* BoolVar )
{
	BYTE* Base = (BYTE*) this;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{				
		UBoolProperty* BoolProperty;
		if( (BoolProperty=Cast<UBoolProperty>(*It)) != NULL )
		{			
			FString BoolName = It->GetName();					
			if( BoolName == Name)
			{						
				*BoolVar = *(UBOOL*)(Base+It->Offset);
				return true;
			}
		}
	}
	return false;
}

UBOOL UObject::FindFNameProperty( FString Name, FName* FNameVar )
{	
	BYTE* Base = (BYTE*) this; 
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{				
		UNameProperty* FNameProperty;
		if( (FNameProperty=Cast<UNameProperty>(*It)) != NULL )
		{			
			FString FNameName = It->GetName();					
			if( FNameName == Name)
			{						
				*FNameVar = *(FName*)(Base+It->Offset);
				return true;
			}
		}
	}
	return false;		
}



/*-----------------------------------------------------------------------------
   Command line.
-----------------------------------------------------------------------------*/

//
// Archive for enumerating other objects referencing a given object.
//
class FArchiveShowReferences : public FArchive
{
public:
	FArchiveShowReferences( FOutputDevice& InAr, UObject* InOuter, UObject* InObj, TArray<UObject*>& InExclude )
	: DidRef( 0 ), Ar( InAr ), Parent( InOuter ), Obj( InObj ), Exclude( InExclude )
	{
		Obj->Serialize( *this );
	}
	UBOOL DidRef;
private:
	FArchive& operator<<( UObject*& Obj )
	{
		guard(FArchiveShowReferences<<Obj);
		if( Obj && Obj->GetOuter()!=Parent )
		{
			INT i;
			for( i=0; i<Exclude.Num(); i++ )
				if( Exclude(i) == Obj->GetOuter() )
					break;
			if( i==Exclude.Num() )
			{
				if( !DidRef )
					Ar.Logf( TEXT("   %s references:"), Obj->GetFullName() );
				Ar.Logf( TEXT("      %s"), Obj->GetFullName() );
				DidRef=1;
			}
		}
		return *this;
		unguard;
	}
	FOutputDevice& Ar;
	UObject* Parent;
	UObject* Obj;
	TArray<UObject*>& Exclude;
};

//
// Archive for finding who references an object.
//
class FArchiveFindCulprit : public FArchive
{
public:
	FArchiveFindCulprit( UObject* InFind, UObject* Src )
	: Find(InFind), Count(0)
	{
		Src->Serialize( *this );
	}
	INT GetCount()
	{
		return Count;
	}
	FArchive& operator<<( class UObject*& Obj )
	{
		if( Obj==Find )
			Count++;
		return *this;
	}
protected:
	UObject* Find;
	INT Count;
};

//
// Archive for finding shortest path from root to a particular object.
// Depth-first search.
//
struct FTraceRouteRecord
{
	INT Depth;
	UObject* Referencer;
	FTraceRouteRecord( INT InDepth, UObject* InReferencer )
	: Depth(InDepth), Referencer(InReferencer)
	{}
};
class FArchiveTraceRoute : public FArchive
{
public:
	static TArray<UObject*> FindShortestRootPath( UObject* Obj )
	{
		guard(FArchiveTraceRoute::FindShortestRootPath);
		TMap<UObject*,FTraceRouteRecord> Routes;
		FArchiveTraceRoute Rt( Routes );
		TArray<UObject*> Result;
		if( Routes.Find(Obj) )
		{
			Result.AddItem( Obj );
			for( ; ; )
			{
				FTraceRouteRecord* Rec = Routes.Find(Obj);
				if( Rec->Depth==0 )
					break;
				Obj = Rec->Referencer;
				Result.Insert(0);
				Result(0) = Obj;
			}
		}
		return Result;
		unguard;
	}
private:
	FArchiveTraceRoute( TMap<UObject*,FTraceRouteRecord>& InRoutes )
	: Routes(InRoutes), Depth(0), Prev(NULL)
	{
		{for( FObjectIterator It; It; ++It )
			It->SetFlags( RF_TagExp );}
		UObject::SerializeRootSet( *this, RF_Native, 0 );
		{for( FObjectIterator It; It; ++It )
			It->ClearFlags( RF_TagExp );}
	}
	FArchive& operator<<( class UObject*& Obj )
	{
		if( Obj )
		{
			FTraceRouteRecord* Rec = Routes.Find(Obj);
			if( !Rec || Depth<Rec->Depth )
				Routes.Set( Obj, FTraceRouteRecord(Depth,Prev) );
		}
		if( Obj && (Obj->GetFlags() & RF_TagExp) )
		{
			Obj->ClearFlags( RF_TagExp );
			UObject* SavedPrev = Prev;
			Prev = Obj;
			Depth++;
			Obj->Serialize( *this );
			Depth--;
			Prev = SavedPrev;
		}
		return *this;
	}
	TMap<UObject*,FTraceRouteRecord>& Routes;
	INT Depth;
	UObject* Prev;
};

//
// Show the inheretance graph of all loaded classes.
//
static void ShowClasses( UClass* Class, FOutputDevice& Ar, int Indent )
{
	Ar.Logf( TEXT("%s%s"), appSpc(Indent), Class->GetName() );
	for( TObjectIterator<UClass> It; It; ++It )
		if( It->GetSuperClass() == Class )
			ShowClasses( *It, Ar, Indent+2 );
}

struct FItem
{
	UClass*	Class;
	INT		Count;
	SIZE_T	Num, Max;
	UBOOL	NumDeleted;
	FItem( UClass* InClass=NULL )
	: Class(InClass), Count(0), Num(0), Max(0), NumDeleted(0)
	{}
	void Add( FArchiveCountMem& Ar )
	{Count++; Num+=Ar.GetNum(); Max+=Ar.GetMax();}
};
struct FSubItem
{
	UObject* Object;
	SIZE_T Num, Max;
	FSubItem( UObject* InObject, SIZE_T InNum, SIZE_T InMax )
	: Object( InObject ), Num( InNum ), Max( InMax )
	{}
};
#if 1 //NEW: (mdf) Logging -- sort objects by name -- added by Legend on 4/12/2000
CORE_API QSORT_RETURN CDECL CompareObjectNames( const UObject** A, const UObject** B )
{
	QSORT_RETURN RetVal;

	static TCHAR	TempNameA[1024],
					TempNameB[1024];
	TCHAR* End;

	// Hack off and store trailing digits.
	appStrcpy( TempNameA, (*A)->GetName() );
	End = TempNameA + appStrlen(TempNameA);
	while( End>TempNameA && appIsDigit(End[-1]) )
		End--;
	INT ValA = appAtoi(End);
	*End = 0;

	// Hack off and store trailing digits.
	appStrcpy( TempNameB, (*B)->GetName() );
	End = TempNameB + appStrlen(TempNameB);
	while( End>TempNameB && appIsDigit(End[-1]) )
		End--;
	INT ValB = appAtoi(End);
	*End = 0;

	// First sort against names (sans trailing digits).
	RetVal = appStrcmp( TempNameA, TempNameB );

	// Then sort by the digits themselves.
	if( RetVal == 0 )
		RetVal = (ValA - ValB);

	return RetVal;
}
#endif
static QSORT_RETURN CDECL CompareSubItems( const FSubItem* A, const FSubItem* B )
{
#if 1 //NEW: (mdf) Logging -- if Max's tie, use name to break tie for easier object finding -- added by Legend on 4/12/2000
	QSORT_RETURN RetVal = (B->Max - A->Max);

	if( RetVal == 0 )
	{
		RetVal = CompareObjectNames( (const UObject**)&A->Object, (const UObject**)&B->Object );
	}

	return RetVal;
#else
	return B->Max - A->Max;
#endif
}
static QSORT_RETURN CDECL CompareItems( const FItem* A, const FItem* B )
{
	return B->Max - A->Max;
}

UBOOL UObject::StaticExec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UObject::StaticExec);
	const TCHAR *Str = Cmd;
	if( ParseCommand(&Str,TEXT("MEM")) )
	{
		GMalloc->DumpAllocs();
		return 1;
	}
#if USCRIPT_PROFILE
	else if( ParseCommand(&Str,TEXT("RESETPROFILE")) )
	{
		for( TObjectIterator<UFunction> It; It; ++It )
			It->Calls = It->Cycles=0;
		GTicks=1;
		return 1;
	}
#endif
	else if( ParseCommand(&Str,TEXT("DUMPNATIVES")) )
	{
		// Linux: Defined out because of error: "taking
		// the address of a bound member function to form
		// a pointer to member function, say '&Object::execUndefined'
		#if _MSC_VER
		for( INT i=0; i<EX_Max; i++ )
			if( GNatives[i] == &execUndefined )
				debugf( TEXT("Native index %i is available"), i );
		#endif
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("GET")) )
	{
		// Get a class default variable.
		TCHAR ClassName[256], PropertyName[256];
		UClass* Class;
		UProperty* Property;
		if
		(	ParseToken( Str, ClassName, ARRAY_COUNT(ClassName), 1 )
		&&	(Class=FindObject<UClass>( ANY_PACKAGE, ClassName))!=NULL )
		{
			if
			(	ParseToken( Str, PropertyName, ARRAY_COUNT(PropertyName), 1 )
			&&	(Property=FindField<UProperty>( Class, PropertyName))!=NULL )
			{
				TCHAR Temp[256]=TEXT("");
				if( Class->Defaults.Num() )
					Property->ExportText( 0, Temp, &Class->Defaults(0), &Class->Defaults(0), PPF_Localized );
				Ar.Log( Temp );
			}
			else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized property %s"), PropertyName );
		}
		else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized class %s"), ClassName );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("SET")) )
	{
		// Set a class default variable.
		TCHAR ClassName[256], PropertyName[256];
		UClass* Class;
		UProperty* Property;
		if
		(	ParseToken( Str, ClassName, ARRAY_COUNT(ClassName), 1 )
		&&	(Class=FindObject<UClass>( ANY_PACKAGE, ClassName))!=NULL )
		{
			if
			(	ParseToken( Str, PropertyName, ARRAY_COUNT(PropertyName), 1 )
			&&	(Property=FindField<UProperty>( Class, PropertyName))!=NULL )
			{
				while( *Str==' ' )
					Str++;
				GlobalSetProperty( Str, Class, Property, Property->Offset, 1 );
			}
			else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized property %s"), PropertyName );
		}
		else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized class %s"), ClassName );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("OBJ")) )
	{
		if( ParseCommand(&Str,TEXT("GARBAGE")) )
		{
			// Purge unclaimed objects.
			UBOOL GSavedNoGC=GNoGC;
			GNoGC = 0;
			CollectGarbage( RF_Native | (GIsEditor ? RF_Standalone : 0) );
			GNoGC = GSavedNoGC;
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("MARK")) )
		{
			debugf( TEXT("Marking objects") );
			for( FObjectIterator It; It; ++It )
				It->SetFlags( RF_Marked );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("MARKCHECK")) )
		{
			debugf( TEXT("Unmarked objects:") );
			for( FObjectIterator It; It; ++It )
				if( !(It->GetFlags() & RF_Marked) )
					debugf( TEXT("%s"), It->GetFullName() );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("REFS")) )
		{
			UClass* Class;
			UObject* Object;
			if
			(	ParseObject<UClass>( Str, TEXT("CLASS="), Class, ANY_PACKAGE )
			&&	ParseObject(Str,TEXT("NAME="), Class, Object, ANY_PACKAGE ) )
			{
				Ar.Logf( TEXT("") );
				Ar.Logf( TEXT("Referencers of %s:"), Object->GetFullName() );
				for( FObjectIterator It; It; ++It )
				{
					FArchiveFindCulprit ArFind(Object,*It);
					if( ArFind.GetCount() )
						Ar.Logf( TEXT("   %s"), It->GetFullName() );
				}
				Ar.Logf(TEXT("") );
				Ar.Logf(TEXT("Shortest reachability from root to %s:"), Object->GetFullName() );
				TArray<UObject*> Rt = FArchiveTraceRoute::FindShortestRootPath(Object);
				for( INT i=0; i<Rt.Num(); i++ )
					Ar.Logf(TEXT("   %s%s"), Rt(i)->GetFullName(), i!=0 ? TEXT("") : (Rt(i)->GetFlags()&RF_Native)?TEXT(" (native)"):TEXT(" (root)") );
				if( !Rt.Num() )
					Ar.Logf(TEXT("   (Object is not currently rooted)"));
				Ar.Logf(TEXT("") );
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("HASH")) )
		{
			// Hash info.
			FName::DisplayHash( Ar );
			INT ObjCount=0, HashCount=0;
			for( FObjectIterator It; It; ++It )
				ObjCount++;
			for( INT i=0; i<ARRAY_COUNT(GObjHash); i++ )
			{
				INT c=0;
				for( UObject* Hash=GObjHash[i]; Hash; Hash=Hash->HashNext )
					c++;
				if( c )
					HashCount++;
				//debugf( "%i: %i", i, c );
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("CLASSES")) )
		{
			ShowClasses( StaticClass(), Ar, 0 );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("DEPENDENCIES")) )
		{
			UPackage* Pkg;
			if( ParseObject<UPackage>(Str,TEXT("PACKAGE="),Pkg,NULL) )
			{
				TArray<UObject*> Exclude;
				for( int i=0; i<16; i++ )
				{
					TCHAR Temp[32];
					appSprintf( Temp, TEXT("EXCLUDE%i="), i );
					FName F;
					if( Parse(Str,Temp,F) )
						Exclude.AddItem( CreatePackage(NULL,*F) );
				}
				Ar.Logf( TEXT("Dependencies of %s:"), Pkg->GetPathName() );
				for( FObjectIterator It; It; ++It )
					if( It->GetOuter()==Pkg )
						FArchiveShowReferences ArShowReferences( Ar, Pkg, *It, Exclude );
			}
			return 1;
		}
		else if ( ParseCommand(&Str,TEXT("DUMPOBJECTS")) )
		{
			UClass* Class=NULL;
			UPackage* Package=NULL;
			ParseObject<UClass>(Str,TEXT("CLASS="),Class,ANY_PACKAGE);
			ParseObject<UPackage>(Str,TEXT("PACKAGE="),Package,NULL);
			Ar.Logf( TEXT("Dumping all objects of type %s in %s:"),Class?Class->GetName():TEXT("NULL"),Package?Package->GetName():TEXT("NULL"));
			if(Class && Package)
				for(FObjectIterator It;It;++It)
					if(It->IsA(Class) && It->IsIn(Package))
						Ar.Logf(TEXT("	%s"),It->GetFullName());
			return 1;
		}
		else if ( ParseCommand(&Str,TEXT("DUMPALLOBJECTS")) )
		{
			Ar.Logf( TEXT("Dumping all objects:"));
			for(INT i=0;i<GObjObjects.Num();i++)
				Ar.Logf(TEXT("	[%i]: %s"),i,GetFullNameSafe(GObjObjects(i)));
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("LIST")) )
		{
			UBOOL bVerbose = ParseParam(Str,TEXT("V")) || ParseParam(Str,TEXT("VERBOSE"));
			UBOOL bShowDeleted = ParseParam(Str,TEXT("D")) || ParseParam(Str,TEXT("DELETED"));
		   	INT DeletedCount = 0;
			Ar.Log( TEXT(" ") );
			UClass*   CheckType     = NULL;
			UPackage* CheckPackage  = NULL;
			UPackage* InsidePackage = NULL;
			ParseObject<UClass>  ( Str, TEXT("CLASS="  ), CheckType,     ANY_PACKAGE );
			ParseObject<UPackage>( Str, TEXT("PACKAGE="), CheckPackage,  NULL );
			ParseObject<UPackage>( Str, TEXT("INSIDE=" ), InsidePackage, NULL );
			TArray<FItem> List;
			TArray<FSubItem> Objects;
			FItem Total;
			for( FObjectIterator It; It; ++It )
			{
				if
				(	(CheckType     && It->IsA(CheckType))
				||	(CheckPackage  && It->GetOuter()==CheckPackage)
				||	(InsidePackage && It->IsIn(InsidePackage))
				||	(!CheckType && !CheckPackage && !InsidePackage) )
				{
					if( It->IsPendingKill() )
					{
						DeletedCount++;
						if( !bShowDeleted )
							continue;
					}
					FArchiveCountMem Count( *It );
					INT i;
					for( i=0; i<List.Num(); i++ )
						if( List(i).Class == It->GetClass() )
							break;
					if( i==List.Num() )
						i = List.AddItem(FItem( It->GetClass() ));
					if( CheckType || CheckPackage || InsidePackage )
						new(Objects)FSubItem( *It, Count.GetNum(), Count.GetMax() );
					List(i).Add( Count );
					Total.Add( Count );
					if( It->IsPendingKill() )
						List(i).NumDeleted++;
				}
			}
			if( bVerbose && Objects.Num() )
			{
				appQsort( &Objects(0), Objects.Num(), sizeof(Objects(0)), (QSORT_COMPARE)CompareSubItems );

				Ar.Log( L" " );
				Ar.Logf( L"%-78s %10s %10s %10s", L"Object", L"NumBytes", L"MaxBytes", bShowDeleted ? L"Deleted" : L"" );

				for( INT i=0; i<Objects.Num(); i++ )
				{
					if( bShowDeleted || !Objects(i).Object->IsPendingKill() )
						Ar.Logf( L"%-78s %10i %10i %10s", Objects(i).Object->GetFullName(), Objects(i).Num, Objects(i).Max, Objects(i).Object->IsPendingKill() ? L"X" : L" " );
				}
				Ar.Log( L" " );
			}
			
			if( List.Num() )
			{
				appQsort( &List(0), List.Num(), sizeof(List(0)), (QSORT_COMPARE)CompareItems );
				if( bShowDeleted )
				{
					Ar.Logf( L"%-30s %6s %10s %10s %10s", L"Class", L"Count", L"NumKBytes", L"MaxKBytes", L"Deleted" );
					for( INT i=0; i<List.Num(); i++ )
						Ar.Logf( L"%-30s %6i %10i %10i %10i", List(i).Class->GetName(), List(i).Count, List(i).Num/1024, List(i).Max/1024, List(i).NumDeleted );
					Ar.Log( L" " );
					Ar.Logf( L"%i Deleted Objects", DeletedCount );
				}
				else
				{
					Ar.Logf( L"%-30s %6s %10s %10s", L"Class", L"Count", L"NumKBytes", L"MaxKBytes" );
					for( INT i=0; i<List.Num(); i++ )
						Ar.Logf( L"%-30s %6i %10i %10i", List(i).Class->GetName(), List(i).Count, List(i).Num/1024, List(i).Max/1024 );
					Ar.Log( L"" );
				}			
			}
			Ar.Logf( L"%i Objects (%.3fM / %.3fM)", Total.Count, (FLOAT)Total.Num/1024.0/1024.0, (FLOAT)Total.Max/1024.0/1024.0 );
			Ar.Log( L" " );
			Ar.Log( L"NOTE: use -v(erbose) to list objects, -d(eleted) to show deleted objects" );
			Ar.Log( L" " );

			return 1;
		}
		else if( ParseCommand(&Str,TEXT("VFHASH")) )
		{
			Ar.Logf( TEXT("Class VfHashes:") );
			for( TObjectIterator<UState> It; It; ++It )
			{
				Ar.Logf( TEXT("%s:"), It->GetName() );
				for( INT i=0; i<UField::HASH_COUNT; i++ )
				{
					INT c=0;
					for( UField* F=It->VfHash[i]; F; F=F->HashNext )
						c++;
					Ar.Logf( TEXT("   %i: %i"), i, c );
				}
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("LINKERS")) )
		{
			Ar.Logf( TEXT("Linkers:") );
			for( INT i=0; i<GObjLoaders.Num(); i++ )
			{
				ULinkerLoad* Linker = CastChecked<ULinkerLoad>( GObjLoaders(i) );
				INT NameSize = 0;
				for( INT i=0; i<Linker->NameMap.Num(); i++ )
					if( Linker->NameMap(i) != NAME_None )
						NameSize += sizeof(FNameEntry) - (NAME_SIZE - appStrlen(*Linker->NameMap(i)) - 1) * sizeof(TCHAR);
				Ar.Logf
				(
					TEXT("%s (%s): Names=%i (%iK/%iK) Imports=%i (%iK) Exports=%i (%iK) Gen=%i Lazy=%i"),
					*Linker->Filename,
					Linker->LinkerRoot->GetFullName(),
					Linker->NameMap.Num(),
					Linker->NameMap.Num() * sizeof(FName) / 1024,
					NameSize / 1024,
					Linker->ImportMap.Num(),
					Linker->ImportMap.Num() * sizeof(FObjectImport) / 1024,
					Linker->ExportMap.Num(),
					Linker->ExportMap.Num() * sizeof(FObjectExport) / 1024,
					Linker->Summary.Generations.Num(),
					Linker->LazyLoaders.Num()
				);
			}
			return 1;
		}
		else return 0;
	}
	else if( ParseCommand(&Str,TEXT("GTIME")) )
	{
		debugf( TEXT("GTime = %f"), GTempDouble );
		return 1;
	}
	else return 0; // Not executed

	unguard;
}

/*-----------------------------------------------------------------------------
   File loading.
-----------------------------------------------------------------------------*/

//
// Safe load error-handling.
//
void UObject::SafeLoadError( DWORD LoadFlags, const TCHAR* Error, const TCHAR* Fmt, ... )
{
	// Variable arguments setup.
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt, Fmt );

	guard(UObject::SafeLoadError);
#if 1 //NEW: Fix (remove bogus warnings from daily build and send warning to output also)
	// removes spam about missing packages but don't remove valid warnings 
	// (can we set a flag on this somewhere instead?) Doesn't use localized version!
	if( !ParseParam( appCmdLine(), TEXT("noloadwarn") ) ||
		(!appStrfind( TempStr, TEXT("None.make") ) && 
	   	 (appStrfind( TempStr, TEXT("Can't find file for package") ) || 
		  appStrfind( TempStr, TEXT("Failed to find object") ))) )
	{
			/*
			mdf-tbd: I'm not sure what the point of using GWarn vs debugf is/was. For
			situations where neither LOAD_Quiet nor LOAD_NoWarn is specified this 
			just causes 2 identical warnings to be logged. UW has the same behavior
			afaict. Was the intent to have GWarn->Logf go to the console perhaps? For
			now I'm disabling the old approach.

			if( !(LoadFlags & LOAD_Quiet) )
				debugf( NAME_Warning, TempStr );
			if( !(LoadFlags & LOAD_NoWarn) )
				GWarn->Logf( NAME_Warning, TempStr );
			*/

			if( !(LoadFlags & (LOAD_Quiet|LOAD_NoWarn)) )
				debugf( NAME_Warning, TempStr );
	}

	if(   LoadFlags & LOAD_Throw   ) appThrowf( TEXT("%s"), Error   );
	if(   LoadFlags & LOAD_NoFail  ) appErrorf( TEXT("%s"), TempStr );
#else
	if( !(LoadFlags & LOAD_Quiet)  ) debugf( NAME_Warning, TempStr );
	if(   LoadFlags & LOAD_Throw   ) appThrowf( TEXT("%s"), Error   );
	if(   LoadFlags & LOAD_NoFail  ) appErrorf( TEXT("%s"), TempStr );
	if( !(LoadFlags & LOAD_NoWarn) ) GWarn->Logf( TEXT("%s"), TempStr );
#endif
	unguard;
}

//
// Find or create the linker for a package.
//
ULinkerLoad* UObject::GetPackageLinker
(
	UObject*		InOuter,
	const TCHAR*	InFilename,
	DWORD			LoadFlags,
	UPackageMap*	Sandbox,
	FGuid*			CompatibleGuid
)
{
	guard(UObject::GetPackageLinker);
	check(GObjBeginLoadCount);

	// See if there is already a linker for this package.
	ULinkerLoad* Result = NULL;
	if( InOuter )
		for( INT i=0; i<GObjLoaders.Num() && !Result; i++ )
			if( GetLoader(i)->LinkerRoot == InOuter )
				Result = GetLoader( i );

	// Try to load the linker.
	try
	{
		// See if the linker is already loaded.
		TCHAR NewFilename[256]=TEXT("");
		if( Result )
		{
			// Linker already found.
			appStrcpy( NewFilename, TEXT("") );
		}
		else if( !InFilename )
		{
			// Resolve filename from package name.
			if( !InOuter )
				appThrowf( LocalizeError(TEXT("PackageResolveFailed"),TEXT("Core")) );
			if( !appFindPackageFile( InOuter->GetName(), CompatibleGuid, NewFilename ) )
			{
				// See about looking in the dll.
				if( (LoadFlags & LOAD_AllowDll) && InOuter->IsA(UPackage::StaticClass()) && ((UPackage*)InOuter)->DllHandle )
					return NULL;
				appThrowf( LocalizeError(TEXT("PackageNotFound"),TEXT("Core")), InOuter->GetName() );
			}
		}
		else
		{
			// Verify that the file exists.
			if( !appFindPackageFile( InFilename, CompatibleGuid, NewFilename ) )
				appThrowf( LocalizeError(TEXT("FileNotFound"),TEXT("Core")), InFilename );

			// Resolve package name from filename.
			TCHAR Tmp[256], *T=Tmp;
			appStrncpy( Tmp, InFilename, ARRAY_COUNT(Tmp) );
			while( 1 )
			{
				if( appStrstr(T,PATH_SEPARATOR) )
					T = appStrstr(T,PATH_SEPARATOR)+appStrlen(PATH_SEPARATOR);
				else if( appStrstr(T,TEXT("/")) )
					T = appStrstr(T,TEXT("/"))+1;
				else if( appStrstr(T,TEXT(":")) )
					T = appStrstr(T,TEXT(":"))+1;
				else
					break;
			}
			if( appStrstr(T,TEXT(".")) )
				*appStrstr(T,TEXT(".")) = 0;
			UPackage* FilenamePkg = CreatePackage( NULL, T );

			// If no package specified, use package from file.
			if( InOuter==NULL )
			{
				if( !FilenamePkg )
					appThrowf( LocalizeError(TEXT("FilenameToPackage"),TEXT("Core")), InFilename );
				InOuter = FilenamePkg;
				for( INT i=0; i<GObjLoaders.Num() && !Result; i++ )
					if( GetLoader(i)->LinkerRoot == InOuter )
						Result = GetLoader(i);
			}
			else if( InOuter != FilenamePkg )//!!should be tested and validated in new UnrealEd
			{
				// Loading a new file into an existing package, so reset the loader.
				debugf( TEXT("New File, Existing Package (%s, %s)"), InOuter->GetFullName(), FilenamePkg->GetFullName() );
				ResetLoaders( InOuter, 0, 1 );
			}
		}

		// Make sure the package is accessible in the sandbox.
		if( Sandbox && !Sandbox->SupportsPackage(InOuter) )
			appThrowf( LocalizeError(TEXT("Sandbox"),TEXT("Core")), InOuter->GetName() );

		// Create new linker.
		if( !Result )
			Result = new ULinkerLoad( InOuter, NewFilename, LoadFlags );

		// Verify compatibility.
		if( CompatibleGuid && Result->Summary.Guid!=*CompatibleGuid )
			appThrowf( LocalizeError(TEXT("PackageVersion"),TEXT("Core")), InOuter->GetName() );
	}
	catch( const TCHAR* Error )
	{
		// If we're in the editor (and not running from UCC) we don't want this to be a fatal error.
		if( GIsEditor && !GIsUCC )
		{
			EdLoadErrorf( FEdLoadError::TYPE_FILE, InFilename ? InFilename : InOuter ? InOuter->GetPathName() : TEXT("NULL") );
			debugf( LocalizeError(TEXT("FailedLoad"),TEXT("Core")), InFilename ? InFilename : InOuter ? InOuter->GetName() : TEXT("NULL"), Error);
		}
		else
			SafeLoadError( LoadFlags, Error, LocalizeError(TEXT("FailedLoad"),TEXT("Core")), InFilename ? InFilename : InOuter ? InOuter->GetName() : TEXT("NULL"), Error );
	}

	// Success.
	return Result;
	unguard;
}

//
// Find or optionally create a package.
//
UPackage* UObject::CreatePackage( UObject* InOuter, const TCHAR* InName )
{
	guard(UObject::CreatePackage);

	ResolveName( InOuter, InName, 1, 0 );
	UPackage* Result = FindObject<UPackage>( InOuter, InName );
	if( !Result )
		Result = new( InOuter, InName, RF_Public )UPackage;
	return Result;

	unguard;
}

//
// Resolve a package and name.
//
UBOOL UObject::ResolveName( UObject*& InPackage, const TCHAR*& InName, UBOOL Create, UBOOL Throw )
{
	guard(UObject::ResolveName);
	check(InName);

	// See if the name is specified in the .ini file.
	UBOOL SystemIni = (appStrnicmp( InName, TEXT("ini:"), 4 )==0);
	UBOOL UserIni   = (appStrnicmp( InName, TEXT("usr:"), 4 )==0);
	if( (SystemIni||UserIni) && appStrlen(InName)<1024 && appStrstr(InName,TEXT(".")) )
	{
		// Get .ini key and section.
		TCHAR Section[256];
		appStrncpy( Section, InName+4, ARRAY_COUNT(Section) );
		TCHAR* Key = Section;
		while( appStrstr(Key,TEXT(".")) )
			Key = appStrstr(Key,TEXT("."))+1;
		check(Key!=Section);
		Key[-1] = 0;

		// Look up name.
		TCHAR* Temp = appStaticString1024();
		if( !GConfig->GetString( Section, Key, Temp, 1024, SystemIni?TEXT("System"):TEXT("User") ) )
		{
			if( Throw )
				appThrowf( LocalizeError(TEXT("ConfigNotFound"),TEXT("Core")), InName );
			return 0;
		}
		InName = Temp;
	}

	// Handle specified packages.
	while( appStrstr(InName,TEXT(".")) )
	{
		TCHAR PartialName[256];
		appStrncpy( PartialName, InName, ARRAY_COUNT(PartialName) );
		*appStrstr( PartialName, TEXT(".") ) = 0;
		if( Create )
		{
			InPackage = CreatePackage( InPackage, PartialName );
		}
		else
		{
			UObject* NewPackage = FindObject<UPackage>( InPackage, PartialName );
			if( !NewPackage )
			{
				NewPackage = FindObject<UObject>( InPackage, PartialName );
				if( !NewPackage )
					return 0;
			}
			InPackage = NewPackage;
		}
		InName = appStrstr(InName,TEXT("."))+1;
	}
	return 1;
	unguard;
}

//
// Load an object.
//
UObject* UObject::StaticLoadObject( UClass* ObjectClass, UObject* InOuter, const TCHAR* InName, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox )
{
	guard(UObject::StaticLoadObject);
	check(ObjectClass);
	check(InName);

	// check for empty object name -- give more meaningful error message
	if( InName[0] == '\0' )
	{
		debugf( NAME_Warning, TEXT( "(StaticLoadObject) -- name for class %s is NULL" ), ObjectClass->GetName() );

		// uncomment this line to force stack dump
		//check( 0 );
		return NULL;
	}

	// Try to load.
	UObject* Result=NULL;
	BeginLoad();
	try
	{
		// Create a new linker object which goes off and tries load the file.
		ULinkerLoad* Linker = NULL;
		ResolveName( InOuter, InName, 1, 1 );
		UObject*	TopOuter = InOuter;
		while( TopOuter && TopOuter->GetOuter() )//!!can only load top-level packages from files
			TopOuter = TopOuter->GetOuter();
		if( !(LoadFlags & LOAD_DisallowFiles) )
			Linker = GetPackageLinker( TopOuter, Filename, LoadFlags | LOAD_Throw | LOAD_AllowDll, Sandbox, NULL );
		//!!this sucks because it supports wildcard sub-package matching of InName, which requires a long search.
		//!!also because linker classes require exact match
		if( Linker )
			Result = Linker->Create( ObjectClass, InName, LoadFlags, 0 );
		if( !Result )
			Result = StaticFindObject( ObjectClass, InOuter, InName );
		if( !Result )
			appThrowf( LocalizeError(TEXT("ObjectNotFound"),TEXT("Core")), ObjectClass->GetName(), InOuter ? InOuter->GetPathName() : TEXT("None"), InName );
		EndLoad();
	}
	catch( const TCHAR* Error )
	{
		EndLoad();
		if( InOuter && !(LoadFlags & LOAD_NoRemap) && !Filename )
		{
			// Try loading a remapped package.
			TArray<FString> Remaps;
			TMultiMap<FString,FString>* PackageRemap = GConfig->GetSectionPrivate(TEXT("PackageRemap"),0,1);
			if(PackageRemap) PackageRemap->MultiFind( InOuter->GetName(), Remaps );
			for( INT i=0; i<Remaps.Num(); i++ )
			{
				InOuter = CreatePackage( NULL, *Remaps(i) );
				Result  = StaticLoadObject( ObjectClass, InOuter, InName, Filename, LoadFlags|LOAD_NoRemap, Sandbox );
				if( Result )
					return Result;
			}
		}
		SafeLoadError( LoadFlags, Error, LocalizeError(TEXT("FailedLoadObject"),TEXT("Core")), ObjectClass->GetName(), InOuter ? InOuter->GetPathName() : TEXT("None"), InName, Error );
	}
	return Result;
	unguardf(( TEXT("(%s %s.%s %s)"), ObjectClass->GetPathName(), InOuter ? InOuter->GetPathName() : TEXT("None"), InName, Filename ? Filename : TEXT("NULL") ));
}

//
// Load a class.
//
UClass* UObject::StaticLoadClass( UClass* BaseClass, UObject* InOuter, const TCHAR* InName, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox )
{
	guard(UObject::StaticLoadClass);
	check(BaseClass);
	try
	{
		UClass* Class = LoadObject<UClass>( InOuter, InName, Filename, LoadFlags | LOAD_Throw, Sandbox );
		if( Class && !Class->IsChildOf(BaseClass) )
			appThrowf( LocalizeError(TEXT("LoadClassMismatch"),TEXT("Core")), Class->GetFullName(), BaseClass->GetFullName() );
		return Class;
	}
	catch( const TCHAR* Error )
	{
		// Failed.
		SafeLoadError( LoadFlags, Error, Error );
		return NULL;
	}
	unguard;
}

//
// Load all objects in a package.
//
UObject* UObject::LoadPackage( UObject* InOuter, const TCHAR* Filename, DWORD LoadFlags )
{
	guard(UObject::LoadPackage);
	UObject* Result;

    if( *Filename == '\0' )
        return NULL;

	// Try to load.
	BeginLoad();
	try
	{
		// Create a new linker object which goes off and tries load the file.
		ULinkerLoad* Linker = GetPackageLinker( InOuter, Filename ? Filename : InOuter->GetName(), LoadFlags | LOAD_Throw, NULL, NULL );
        if( !Linker )
            return( NULL );

		if( !(LoadFlags & LOAD_Verify) )
			Linker->LoadAllObjects();
		Result = Linker->LinkerRoot;
		EndLoad();
	}
	catch( const TCHAR* Error )
	{
		EndLoad();
		SafeLoadError( LoadFlags, Error, LocalizeError(TEXT("FailedLoadPackage"),TEXT("Core")), Error );
		Result = NULL;
	}
	return Result;
	unguard;
}

//
// Verify a linker.
//
void UObject::VerifyLinker( ULinkerLoad* Linker )
{
	guard(UObject::VerifyLinker);
	Linker->Verify();
	unguard;
}

//
// Begin loading packages.
//warning: Objects may not be destroyed between BeginLoad/EndLoad calls.
//
void UObject::BeginLoad()
{
	guard(UObject::BeginLoad);
	if( ++GObjBeginLoadCount == 1 )
	{
		// Validate clean load state.
		//!!needed? check(GObjLoaded.Num()==0);
		check(!GAutoRegister);
		for( INT i=0; i<GObjLoaders.Num(); i++ )
			check(GetLoader(i)->Success);
	}
	unguard;
}

//
// End loading packages.
//
void UObject::EndLoad()
{
	guard(UObject::EndLoad);
	check(GObjBeginLoadCount>0);
	if( --GObjBeginLoadCount == 0 )
	{
		try
		{
			TArray<UObject*>	ObjLoaded;

			while(GObjLoaded.Num())
			{
				appMemcpy(&ObjLoaded(ObjLoaded.Add(GObjLoaded.Num())),&GObjLoaded(0),GObjLoaded.Num() * sizeof(UObject*));
				GObjLoaded.Empty();

				// Finish loading everything.
				//warning: Array may expand during iteration.
				guard(PreLoadObjects);
				debugfSlow( NAME_DevLoad, TEXT("Loading objects...") );
				for( INT i=0; i<ObjLoaded.Num(); i++ )
				{
					// Preload.
					UObject* Obj = ObjLoaded(i);
					{
						if( Obj->GetFlags() & RF_NeedLoad )
						{
							check(Obj->GetLinker());
							Obj->GetLinker()->Preload( Obj );
						}
					}
				}
				unguard;

				if(GObjLoaded.Num())
					continue;

				// Postload objects.
				guard(PostLoadObjects);
				for( INT i=0; i<ObjLoaded.Num(); i++ )
					ObjLoaded(i)->ConditionalPostLoad();
				unguard;
			}

			// Dissociate all linker import object references, since they 
			// may be destroyed, causing their pointers to become invalid.
			guard(DissociateImports);
			if( GImportCount )
			{
				for( INT i=0; i<GObjLoaders.Num(); i++ )
				{
					for( INT j=0; j<GetLoader(i)->ImportMap.Num(); j++ )
					{
						FObjectImport& Import = GetLoader(i)->ImportMap(j);
						if( Import.XObject && !(Import.XObject->GetFlags() & RF_Native) )
							Import.XObject = NULL;
					}
				}
			}
			GImportCount=0;
			unguard;
		}
		catch( const TCHAR* Error )
		{
			// Any errors here are fatal.
			appErrorf( Error );
		}
	}
	unguard;
}

//
// Empty the loaders.
//
void UObject::ResetLoaders( UObject* Pkg, UBOOL DynamicOnly, UBOOL ForceLazyLoad )
{
	guard(UObject::ResetLoaders);
	for( INT i=GObjLoaders.Num()-1; i>=0; i-- )
	{
		ULinkerLoad* Linker = CastChecked<ULinkerLoad>( GetLoader(i) );
		if( Pkg==NULL || Linker->LinkerRoot==Pkg )
		{
			if( DynamicOnly )
			{
				// Reset runtime-dynamic objects.
				for( INT i=0; i<Linker->ExportMap.Num(); i++ )
				{
					UObject*& Object = Linker->ExportMap(i)._Object;
					if( Object && !(Object->GetClass()->ClassFlags & CLASS_RuntimeStatic))
						Linker->DetachExport( i );
				}
			}
			else
			{
				// Fully reset the loader.
				if( ForceLazyLoad )
					Linker->DetachAllLazyLoaders( 1 );
			}
		}
		else
		{
			for(INT i = 0;i < Linker->ImportMap.Num();i++)
			{
				if(Linker->ImportMap(i).SourceLinker && Linker->ImportMap(i).SourceLinker->LinkerRoot == Pkg)
				{
					Linker->ImportMap(i).SourceLinker = NULL;
					Linker->ImportMap(i).SourceIndex = INDEX_NONE;
				}
			}
		}
	}
	for( INT i=GObjLoaders.Num()-1; i>=0; i-- )
	{
		ULinkerLoad* Linker = CastChecked<ULinkerLoad>( GetLoader(i) );
		if( Pkg==NULL || Linker->LinkerRoot==Pkg )
		{
			if( !DynamicOnly )
			{
				delete Linker;
			}
		}
	}
	unguardf(( TEXT("(%s %d %d)"), Pkg ? Pkg->GetName() : TEXT("NULL"), DynamicOnly, ForceLazyLoad ));
}

/*-----------------------------------------------------------------------------
	File saving.
-----------------------------------------------------------------------------*/

//
// Archive for tagging objects and names that must be exported
// to the file.  It tags the objects passed to it, and recursively
// tags all of the objects this object references.
//
class FArchiveSaveTagExports : public FArchive
{
public:
	FArchiveSaveTagExports( UObject* InOuter )
	: Parent(InOuter)
	{
		ArIsSaving = ArIsPersistent = 1;
	}
	FArchive& operator<<( UObject*& Obj )
	{
		guard(FArchiveSaveTagExports<<Obj);



#if 1 //NEW: added by Legend on 4/12/2000
	
		if( Obj && !Obj->IsPendingKill() && Obj->IsIn(Parent) && !(Obj->GetFlags() & (RF_Transient|RF_TagExp)) )
#else
		if( Obj && Obj->IsIn(Parent) && !(Obj->GetFlags() & (RF_Transient|RF_TagExp)) )//&& !Obj->IsPendingKill() )
#endif
		{
			//bool bWasAlreadyInStack = false;
			//static TArray<UObject*> CurrentlyVisiting;
			//
			//if (Obj->IsA(UProperty::StaticClass()))
			//{
			//	UProperty* Prop = (UProperty*)Obj;
			//
			//	const TCHAR* PropName = Prop->GetName();
			//	const TCHAR* PropClassName = Prop->GetClass() ? Prop->GetClass()->GetName() : TEXT("None");
			//	const TCHAR* PropOuterName = Prop->GetOuter() ? Prop->GetOuter()->GetName() : TEXT("None");
			//
			//	GWarn->Logf(TEXT("Processing PROPERTY: %s, Class: %s, Outer: %s"),
			//		PropName,
			//		PropClassName,
			//		PropOuterName);
			//
			//	bWasAlreadyInStack = !CurrentlyVisiting.AddItem(Prop->GetOuter());
			//	if (bWasAlreadyInStack)
			//	{
			//		const TCHAR* CycleOuterName = Prop->GetOuter() ? Prop->GetOuter()->GetName() : TEXT("None");
			//
			//		// We have a cycle! Dump out info and possibly skip or break here.
			//		GWarn->Logf(TEXT("** CYCLE DETECTED: Already visited %s (0x%08X) **"),
			//			CycleOuterName,
			//			reinterpret_cast<uintptr_t>(Prop->GetOuter()));
			//		// Break, or set a debug breakpoint, or handle however you need.
			//	}
			//}
			//else if (Obj->IsA(UClass::StaticClass()))
			//{
			//	UClass* Cls = (UClass*)Obj;
			//
			//	const TCHAR* ClassName = Cls->GetName();
			//	const TCHAR* SuperClassName = (Cls->GetSuperClass() ? Cls->GetSuperClass()->GetName() : TEXT("None"));
			//	const TCHAR* OuterName = (Cls->GetOuter() ? Cls->GetOuter()->GetName() : TEXT("None"));
			//
			//	GWarn->Logf(TEXT("Processing CLASS: %s, Super: %s, Outer: %s"),
			//		ClassName,
			//		SuperClassName,
			//		OuterName);
			//
			//	// Example of cycle detection using the SuperClass (or you could check Outer, etc.)
			//	bWasAlreadyInStack = !CurrentlyVisiting.AddItem(Cls->GetSuperClass());
			//	if (bWasAlreadyInStack)
			//	{
			//		const TCHAR* CycleSuperClassName = Cls->GetSuperClass() ? Cls->GetSuperClass()->GetName() : TEXT("None");
			//
			//		// We have a cycle! Dump out info and possibly skip or break here.
			//		GWarn->Logf(TEXT("** CYCLE DETECTED: Already visited super class %s (0x%08X) **"),
			//			CycleSuperClassName,
			//			reinterpret_cast<uintptr_t>(Cls->GetSuperClass()));
			//		// Break, or set a debug breakpoint, or handle however you need.
			//	}
			//}
			//else
			//{
			//	const TCHAR* ObjName = Obj->GetName();
			//	const TCHAR* ObjClassName = (Obj->GetClass() ? Obj->GetClass()->GetName() : TEXT("None"));
			//
			//	GWarn->Logf(TEXT("Processing Object: %s, Class: %s"),
			//		ObjName,
			//		ObjClassName);
			//
			//	bWasAlreadyInStack = !CurrentlyVisiting.AddItem(Obj->GetClass());
			//	if (bWasAlreadyInStack)
			//	{
			//		const TCHAR* CycleClassName = Obj->GetClass() ? Obj->GetClass()->GetName() : TEXT("None");
			//
			//		// We have a cycle! Dump out info and possibly skip or break here.
			//		GWarn->Logf(TEXT("** CYCLE DETECTED: Already visited %s (0x%08X) **"),
			//			CycleClassName,
			//			reinterpret_cast<uintptr_t>(Obj->GetClass()));
			//		// Break, or set a debug breakpoint, or handle however you need.
			//	}
			//}


			// Set flags.
			Obj->SetFlags(RF_TagExp);
			if( !(Obj->GetFlags() & RF_NotForEdit  ) ) Obj->SetFlags(RF_LoadForEdit);
			if( !(Obj->GetFlags() & RF_NotForClient) ) Obj->SetFlags(RF_LoadForClient);
			if( !(Obj->GetFlags() & RF_NotForServer) ) Obj->SetFlags(RF_LoadForServer);

			// Recurse with this object's class and package.
			UClass*  Class  = Obj->GetClass();
			UObject* Parent = Obj->GetOuter();
			*this << Class << Parent;

			// Recurse with this object's children.
			Obj->Serialize( *this );

		}
		return *this;
		unguard;
	}
	UObject* Parent;
};

//
// QSort comparators.
//
static ULinkerSave* GTempSave;
INT CDECL LinkerNameSort( const void* A, const void* B )
{
	return GTempSave->MapName((FName*)B) - GTempSave->MapName((FName*)A);
}
INT CDECL LinkerImportSort( const void* A, const void* B )
{
	return GTempSave->MapObject(((FObjectImport*)B)->XObject) - GTempSave->MapObject(((FObjectImport*)A)->XObject);
}
INT CDECL LinkerExportSort( const void* A, const void* B )
{
	return GTempSave->MapObject(((FObjectExport*)B)->_Object) - GTempSave->MapObject(((FObjectExport*)A)->_Object);
}

//
// Archive for tagging objects and names that must be listed in the
// file's imports table.
//
class FArchiveSaveTagImports : public FArchive
{
public:
	DWORD ContextFlags;
	FArchiveSaveTagImports( ULinkerSave* InLinker, DWORD InContextFlags )
	: ContextFlags( InContextFlags ), Linker( InLinker )
	{
		ArIsSaving = ArIsPersistent = 1;
	}
	FArchive& operator<<( UObject*& Obj )
	{
		guard(FArchiveSaveTagImports<<Obj);
		if( Obj && !Obj->IsPendingKill() )
		{
			if( !(Obj->GetFlags() & RF_Transient) || (Obj->GetFlags() & RF_Public) )
			{
				Linker->ObjectIndices(Obj->GetIndex())++;
				if( !(Obj->GetFlags() & RF_TagExp ) )
				{
					Obj->SetFlags( RF_TagImp );
					if( !(Obj->GetFlags() & RF_NotForEdit  ) ) Obj->SetFlags(RF_LoadForEdit);
					if( !(Obj->GetFlags() & RF_NotForClient) ) Obj->SetFlags(RF_LoadForClient);
					if( !(Obj->GetFlags() & RF_NotForServer) ) Obj->SetFlags(RF_LoadForServer);
					UObject* Parent = Obj->GetOuter();
					if( Parent )
						*this << Parent;
				}
			}
		}
		return *this;
		unguard;
	}
	FArchive& operator<<( FName& Name )
	{
		guard(FArchiveSaveTagImports<<Name);
		Name.SetFlags( RF_TagExp | ContextFlags );
		Linker->NameIndices(Name.GetIndex())++;
		return *this;
		unguard;
	}
	ULinkerSave* Linker;
};

//
// Save one specific object into an Unrealfile.
//
UBOOL UObject::SavePackage( UObject* InOuter, UObject* Base, DWORD TopLevelFlags, const TCHAR* Filename, FOutputDevice* Error, ULinkerLoad* Conform )
{
	guard(UObject::SavePackage);
	check(InOuter);
	check(Filename);
	DWORD Time=0; clock(Time);

	// Tag parent flags.
	//!!should be user-controllable in UnrealEd.
	if( Cast<UPackage>( InOuter ) )
		Cast<UPackage>( InOuter )->PackageFlags |= PKG_AllowDownload;

	// Make temp file.
	TCHAR TempFilename[256];
	appStrncpy( TempFilename, Filename, ARRAY_COUNT(TempFilename) );
	INT c = appStrlen(TempFilename);
	while( c>0 && TempFilename[c-1]!=PATH_SEPARATOR[0] && TempFilename[c-1]!='/' && TempFilename[c-1]!=':' )
		c--;
	TempFilename[c]=0;
	appStrcat( TempFilename, TEXT("Save.tmp") );

	// Init.
	GWarn->StatusUpdatef( 0, 0, LocalizeProgress(TEXT("Saving"),TEXT("Core")), Filename );
	UBOOL Success = 0;

	// If we have a loader for the package, unload it to prevent conflicts.
	ResetLoaders( InOuter, 0, 1 );

	// Untag all objects and names.
	guard(Untag);
	for( FObjectIterator It; It; ++It )
		It->ClearFlags( RF_TagImp | RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
	for( INT i=0; i<FName::GetMaxNames(); i++ )
		if( FName::GetEntry(i) )
			FName::GetEntry(i)->Flags &= ~(RF_TagImp | RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer);
	unguard;

	// Export objects.
	guard(TagExports);
	FArchiveSaveTagExports Ar( InOuter );
	if( Base )
		Ar << Base;
	for( FObjectIterator It; It; ++It )
	{
		if( (It->GetFlags() & TopLevelFlags) && It->IsIn(InOuter) )
		{
			UObject* Obj = *It;
			Ar << Obj;
		}
	}
	unguard;

	ULinkerSave* Linker = NULL;
	try
	{
		// Allocate the linker.
		Linker = new ULinkerSave( InOuter, TempFilename );

		// Import objects and names.
		guard(TagImports);
		for( FObjectIterator It; It; ++It )
		{
			if( It->GetFlags() & RF_TagExp )
			{
				// Build list.
				FArchiveSaveTagImports Ar( Linker, It->GetFlags() & RF_LoadContextFlags );
				It->Serialize( Ar );
				UClass* Class = It->GetClass();
				Ar << Class;
				if( It->IsIn(GetTransientPackage()) )
					appErrorf( LocalizeError(TEXT("TransientImport"),TEXT("Core")), It->GetFullName() );
			}
		}
		unguard;

		// Export all relevant object, class, and package names.
		guard(ExportNames);
		for( FObjectIterator It; It; ++It )
		{
			if( It->GetFlags() & (RF_TagExp|RF_TagImp) )
			{
				It->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
				if( It->GetOuter() )
					It->GetOuter()->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
				if( It->GetFlags() & RF_TagImp )
				{
					It->Class->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
					check(It->Class->GetOuter());
					It->Class->GetOuter()->GetFName().SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
#if 0 //NEW (mdf) -- check is bogus for protected data and compiler should catch problems anyway
					if( !(It->GetFlags() & RF_Public) )
					{
						GWarn->Logf( TEXT("Referencers of %s:"), It->GetFullName() );
						for( FObjectIterator RefIt; RefIt; ++RefIt )
						{
							FArchiveFindCulprit ArFind(*It,*RefIt);
							if( ArFind.GetCount() )
								GWarn->Logf( TEXT("   %s"), RefIt->GetFullName() );
						}
						appThrowf( LocalizeError(TEXT("FailedSavePrivate"),TEXT("Core")), Filename, It->GetFullName() );
					}
#endif
				}
				else debugfSlow( NAME_DevSave, TEXT("Saving %s"), It->GetFullName() );
			}
		}
		unguard;

		// Write fixed-length file summary to overwrite later.
		guard(SaveSummary);
		if( Conform )
		{
			// Conform to previous generation of file.
			debugf( TEXT("Conformal save, relative to: %s, Generation %i"), *Conform->Filename, Conform->Summary.Generations.Num()+1 );
			Linker->Summary.Guid        = Conform->Summary.Guid;
			Linker->Summary.Generations = Conform->Summary.Generations;
		}
		else
		{
			// First generation file.
			Linker->Summary.Guid        = appCreateGuid();
			Linker->Summary.Generations = TArray<FGenerationInfo>();
		}
		new(Linker->Summary.Generations)FGenerationInfo( 0, 0 );
		*Linker << Linker->Summary;
		unguard;

		// Build NameMap.
		guard(BuildNameMap);
		Linker->Summary.NameOffset = Linker->Tell();
		for( INT i=0; i<FName::GetMaxNames(); i++ )
		{
			if( FName::GetEntry(i) )
			{
				FName Name( (EName)i );
				if( Name.GetFlags() & RF_TagExp )
					Linker->NameMap.AddItem( Name );
			}
		}
		unguard;

		// Sort names by usage count in order to maximize compression.
		guard(SortNames);
		GTempSave = Linker;
		INT FirstSort = 0;
		if( Conform )
		{
#if DO_GUARD_SLOW
			TArray<FName> Orig = Linker->NameMap;
#endif
			for( INT i=0; i<Conform->NameMap.Num(); i++ )
			{
				INT Index = Linker->NameMap.FindItemIndex(Conform->NameMap(i));
				if( Conform->NameMap(i)==NAME_None || Index==INDEX_NONE )
				{
					Linker->NameMap.Add();
					Linker->NameMap.Last() = Linker->NameMap(i);
				}
				else Exchange( Linker->NameMap(i), Linker->NameMap(Index) );
				Linker->NameMap(i) = Conform->NameMap(i);
			}
#if DO_GUARD_SLOW
			for( INT i=0; i<Conform->NameMap.Num(); i++ )
				check(Linker->NameMap(i)==Conform->NameMap(i));
			for( INT i=0; i<Orig.Num(); i++ )
				check(Linker->NameMap.FindItemIndex(Orig(i))!=INDEX_NONE);
#endif
			FirstSort = Conform->NameMap.Num();
		}
		appQsort( &Linker->NameMap(FirstSort), Linker->NameMap.Num()-FirstSort, sizeof(Linker->NameMap(0)), LinkerNameSort );
		unguard;

		// Save names.
		guard(SaveNames);
		Linker->Summary.NameCount = Linker->NameMap.Num();
		for( INT i=0; i<Linker->NameMap.Num(); i++ )
		{
			*Linker << *FName::GetEntry( Linker->NameMap(i).GetIndex() );
			Linker->NameIndices(Linker->NameMap(i).GetIndex()) = i;
		}
		unguard;

		// Build ImportMap.
		guard(BuildImportMap);
		for( FObjectIterator It; It; ++It )
			if( It->GetFlags() & RF_TagImp )
				new( Linker->ImportMap )FObjectImport( *It );
		Linker->Summary.ImportCount = Linker->ImportMap.Num();
		unguard;

		// Sort imports by usage count.
		guard(SortImports);
		GTempSave = Linker;
		appQsort( &Linker->ImportMap(0), Linker->ImportMap.Num(), sizeof(Linker->ImportMap(0)), LinkerImportSort );
		unguard;

		// Build ExportMap.
		guard(BuildExports);
		for( FObjectIterator It; It; ++It )
			if( It->GetFlags() & RF_TagExp )
				new( Linker->ExportMap )FObjectExport( *It );
		unguard;

		// Sort exports by usage count.
		guard(SortExports);
		INT FirstSort = 0;
		if( Conform )
		{
			TArray<FObjectExport> Orig = Linker->ExportMap;
			Linker->ExportMap.Empty( Linker->ExportMap.Num() );
			TArray<BYTE> Used; Used.AddZeroed( Orig.Num() );
			TMap<FString,INT> Map;
			{for( INT i=0; i<Orig.Num(); i++ )
				Map.Set( Orig(i)._Object->GetFullName(), i );}
			{for( INT i=0; i<Conform->ExportMap.Num(); i++ )
			{
				INT* Find = Map.Find( Conform->GetExportFullName(i,Linker->LinkerRoot->GetPathName()) );
				if( Find )
				{
					new(Linker->ExportMap)FObjectExport( Orig(*Find) );
					check(Linker->ExportMap.Last()._Object == Orig(*Find)._Object);
					Used( *Find ) = 1;
				}
				else new(Linker->ExportMap)FObjectExport( NULL );
			}}
			FirstSort = Conform->ExportMap.Num();
			{for( INT i=0; i<Used.Num(); i++ )
				if( !Used(i) )
					new(Linker->ExportMap)FObjectExport( Orig(i) );}
#if DO_GUARD_SLOW
			{for( INT i=0; i<Orig.Num(); i++ )
			{
				INT j = 0;
				for( j=0; j<Linker->ExportMap.Num(); j++ )
					if( Linker->ExportMap(j)._Object == Orig(i)._Object )
						break;
				check(j<Linker->ExportMap.Num());
			}}
#endif
		}
		appQsort( &Linker->ExportMap(FirstSort), Linker->ExportMap.Num()-FirstSort, sizeof(Linker->ExportMap(0)), LinkerExportSort );
		Linker->Summary.ExportCount = Linker->ExportMap.Num();
		unguard;

		// Set linker reverse mappings.
		guard(SetLinkerMappings);
		{for( INT i=0; i<Linker->ExportMap.Num(); i++ )
			if( Linker->ExportMap(i)._Object )
				Linker->ObjectIndices(Linker->ExportMap(i)._Object->GetIndex()) = i+1;}
		{for( INT i=0; i<Linker->ImportMap.Num(); i++ )
			Linker->ObjectIndices(Linker->ImportMap(i).XObject->GetIndex()) = -i-1;}
		unguard;

		// Save exports.
		guard(SaveExports);
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			FObjectExport& Export = Linker->ExportMap(i);
			if( Export._Object )
			{
				// Set class index.
				if( !Export._Object->IsA(UClass::StaticClass()) )
				{
					Export.ClassIndex = Linker->ObjectIndices(Export._Object->GetClass()->GetIndex());
					check(Export.ClassIndex!=0);
				}
				if( Export._Object->IsA(UStruct::StaticClass()) )
				{
					UStruct* Struct = (UStruct*)Export._Object;
					if( Struct->SuperField )
					{
						Export.SuperIndex = Linker->ObjectIndices(Struct->SuperField->GetIndex());
						check(Export.SuperIndex!=0);
					}
				}

				// Set package index.
				if( Export._Object->GetOuter() != InOuter )
				{
					check(Export._Object->GetOuter()->IsIn(InOuter));
					Export.PackageIndex = Linker->ObjectIndices(Export._Object->GetOuter()->GetIndex());
					check(Export.PackageIndex>0);
				}

				// Save it.
				Export.SerialOffset = Linker->Tell();
				Export._Object->Serialize( *Linker );
				Export.SerialSize = Linker->Tell() - Linker->ExportMap(i).SerialOffset;
			}
		}
		unguard;

		// Save the import map.
		guard(SaveImportMap);
		Linker->Summary.ImportOffset = Linker->Tell();
		for( INT i=0; i<Linker->ImportMap.Num(); i++ )
		{
			FObjectImport& Import = Linker->ImportMap( i );

			// Set the package index.
			if( Import.XObject->GetOuter() )
			{
				check(!Import.XObject->GetOuter()->IsIn(InOuter));
				Import.PackageIndex = Linker->ObjectIndices(Import.XObject->GetOuter()->GetIndex());
				if(Import.PackageIndex>=0)
					appErrorf(TEXT("PackageIndex: %d %s"),Import.PackageIndex,Import.XObject->GetOuter()->GetFullName());
			}

			// Save it.
			*Linker << Import;
		}
		unguard;

		// Save the export map.
		guard(SaveExportMap);
		Linker->Summary.ExportOffset = Linker->Tell();
		for( int i=0; i<Linker->ExportMap.Num(); i++ )
			*Linker << Linker->ExportMap( i );
		unguard;

		// Rewrite updated file summary.
		guard(RewriteSummary);
		GWarn->StatusUpdatef( 0, 0, LocalizeProgress(TEXT("Closing"),TEXT("Core")) );
		Linker->Summary.Generations.Last().ExportCount = Linker->Summary.ExportCount;
		Linker->Summary.Generations.Last().NameCount   = Linker->Summary.NameCount;
		Linker->Seek(0);
		*Linker << Linker->Summary;
		unguard;

		Success = 1;
	}
	catch( const TCHAR* Msg )
	{
		// Delete the temporary file.
		GFileManager->Delete( TempFilename );
		Error->Logf( NAME_Warning, TEXT("%s"), Msg );
	}
	if( Linker )
	{
		delete Linker;
	}
	unclock(Time);
	debugf( NAME_Log, TEXT("Save=%f"), GSecondsPerCycle*1000*Time );
	if( Success )
	{
		// Move the temporary file.
		debugf( NAME_Log, TEXT("Moving '%s' to '%s'"), TempFilename, Filename );
		if( !GFileManager->Move( Filename, TempFilename ) )
		{
			GFileManager->Delete( TempFilename );
			GWarn->Logf( LocalizeError(TEXT("SaveWarning"),TEXT("Core")), Filename );
			Success = 0;
		}
	}
	return Success;
	unguard;
}

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

//
// Return whether statics are initialized.
//
UBOOL UObject::GetInitialized()
{
	return UObject::GObjInitialized;
}

//
// Return the static transient package.
//
UPackage* UObject::GetTransientPackage()
{
	return UObject::GObjTransientPkg;
}

//
// Return the ith loader.
//
ULinkerLoad* UObject::GetLoader( INT i )
{
	return (ULinkerLoad*)GObjLoaders(i);
}

//
// Add an object to the root array. This prevents the object and all
// its descendents from being deleted during garbage collection.
//
void UObject::AddToRoot()
{
	guard(UObject::AddToRoot);
	GObjRoot.AddItem( this );
	unguard;
}

//
// Remove an object from the root array.
//
void UObject::RemoveFromRoot()
{
	guard(UObject::RemoveFromRoot);
	GObjRoot.RemoveItem( this );
	unguard;
}

/*-----------------------------------------------------------------------------
	Object name functions.
-----------------------------------------------------------------------------*/

//
// Create a unique name by combining a base name and an arbitrary number string.  
// The object name returned is guaranteed not to exist.
//
FName UObject::MakeUniqueObjectName( UObject* Parent, UClass* Class )
{
	guard(UObject::MakeUniqueObjectName);
	check(Class);
	
	TCHAR NewBase[NAME_SIZE], Result[NAME_SIZE];
	TCHAR TempIntStr[NAME_SIZE];

	// Make base name sans appended numbers.
	appStrcpy( NewBase, Class->GetName() );
	TCHAR* End = NewBase + appStrlen(NewBase);
	while( End>NewBase && appIsDigit(End[-1]) )
		End--;
	*End = 0;

	// Append numbers to base name.
	do
	{
		appSprintf( TempIntStr, TEXT("%i"), Class->ClassUnique++ );
		appStrncpy( Result, NewBase, NAME_SIZE-appStrlen(TempIntStr)-1 );
		appStrcat( Result, TempIntStr );
	} while( StaticFindObject( NULL, Parent, Result ) );

	return Result;
	unguard;
}

/*-----------------------------------------------------------------------------
	Object hashing.
-----------------------------------------------------------------------------*/

//
// Add an object to the hash table.
//
void UObject::HashObject()
{
	guard(UObject::HashObject);

	INT iHash       = GetObjectHash( Name, Outer ? Outer->GetIndex() : 0 );
	HashNext        = GObjHash[iHash];
	GObjHash[iHash] = this;

	unguard;
}

//
// Remove an object from the hash table.
//
void UObject::UnhashObject( INT OuterIndex )
{
	guard(UObject::UnhashObject);
	INT       iHash   = GetObjectHash( Name, OuterIndex );
	UObject** Hash    = &GObjHash[iHash];
	INT       Removed = 0;
	while( *Hash != NULL )
	{
		if( *Hash != this )
		{
			Hash = &(*Hash)->HashNext;
 		}
		else
		{
			*Hash = (*Hash)->HashNext;
			Removed++;
		}
	}
	check(Removed!=0);
	check(Removed==1);
	unguard;
}

/*-----------------------------------------------------------------------------
	Creating and allocating data for new objects.
-----------------------------------------------------------------------------*/

//
// Add an object to the table.
//
void UObject::AddObject( INT InIndex )
{
	guard(UObject::AddObject);

	// Find an available index.
	if( InIndex==INDEX_NONE )
	{
		if( GObjAvailable.Num() )
		{
			InIndex = GObjAvailable.Pop();
			check(GObjObjects(InIndex)==NULL);
		}
		else InIndex = GObjObjects.Add();
	}

	// Add to global table.
	GObjObjects(InIndex) = this;
	Index = InIndex;
	HashObject();

	unguard;
}

//
// Create a new object or replace an existing one.
// If Name is NAME_None, tries to create an object with an arbitrary unique name.
// No failure conditions.
//
UObject* UObject::StaticAllocateObject
(
	UClass*			InClass,
	UObject*		InOuter,
	FName			InName,
	DWORD			InFlags,
	UObject*		InTemplate,
	FOutputDevice*	Error,
	UObject*		Ptr,
	UObject*		SuperObject
)
{
	guard(UObject::StaticAllocateObject);
	check(Error);
	check(!InClass || InClass->ClassWithin);
	check(!InClass || InClass->ClassConstructor);

	// Validation checks.
	if( !InClass )
	{
		Error->Logf( TEXT("Empty class for object %s"), *InName );
		return NULL;
	}
	if( InClass->GetIndex()==INDEX_NONE && GObjRegisterCount==0 )
	{
		Error->Logf( TEXT("Unregistered class for %s"), *InName );
		return NULL;
	}
	if( InClass->ClassFlags & CLASS_Abstract )
	{
		Error->Logf( LocalizeError(TEXT("Abstract"),TEXT("Core")), *InName, InClass->GetName() );
		return NULL;
	}
	if( !InOuter && InClass!=UPackage::StaticClass() )
	{
		Error->Logf( LocalizeError(TEXT("NotPackaged"),TEXT("Core")), InClass->GetName(), *InName );
		return NULL;
	}
	if( InOuter && !InOuter->IsA(InClass->ClassWithin) )
	{
		Error->Logf( LocalizeError(TEXT("NotWithin"),TEXT("Core")), InClass->GetName(), *InName, InOuter->GetClass()->GetName(), InClass->ClassWithin->GetName() );
		return NULL;
	}

	// Compose name, if public and unnamed.
	//if( InName==NAME_None && (InFlags&RF_Public) )
	if( InName==NAME_None )//oldver?? what about compatibility matching of unnamed objects?
		InName = MakeUniqueObjectName( InOuter, InClass );

	// Check for name conflicts.
	if( GCheckConflicts && InName!=NAME_None )
		for( UObject* Hash=GObjHash[GetObjectHash(InName,InOuter?InOuter->GetIndex():0)]; Hash!=NULL; Hash=Hash->HashNext )
			if
			(	Hash->GetFName()==InName
			&&	Hash->Outer==InOuter
			&&	Hash->GetClass()!=InClass )
				debugf( NAME_Log, TEXT("CONFLICT: %s - %s"), Hash->GetFullName(), InClass->GetName() );

	// See if object already exists.
	UObject* Obj                        = StaticFindObject( InClass, InOuter, *InName );//oldver: Should use NULL instead of InClass to prevent conflicts by name rather than name-class.
	UClass*  Cls                        = Cast<UClass>( Obj );
	INT      Index                      = INDEX_NONE;
	UClass*  ClassWithin				= NULL;
	DWORD    ClassFlags                 = 0;
	void     (*ClassConstructor)(void*) = NULL;
	if( !Obj )
	{
		// Create a new object.
		Obj = Ptr ? Ptr : (UObject*)appMalloc( InClass->GetPropertiesSize(), *InName );
	}
	else
	{
		// Replace an existing object without affecting the original's address or index.
		check(!Ptr || Ptr==Obj);
		debugfSlow( NAME_DevReplace, TEXT("Replacing %s"), Obj->GetName() );

		// Can only replace if class is identical.
		if( Obj->GetClass() != InClass )
			appErrorf( LocalizeError(TEXT("NoReplace"),TEXT("Engine")), Obj->GetFullName(), InClass->GetName() );

		// Remember flags, index, and native class info.
		InFlags |= (Obj->GetFlags() & RF_Keep);
		Index    = Obj->Index;
		if( Cls )
		{
			ClassWithin		 = Cls->ClassWithin;
			ClassFlags       = Cls->ClassFlags & CLASS_Abstract;
			ClassConstructor = Cls->ClassConstructor;
		}

		// Destroy the object.
		Obj->~UObject();
		check(GObjAvailable.Num() && GObjAvailable.Last()==Index);
		GObjAvailable.Pop();
	}

	// If class is transient, objects must be transient.
	if( InClass->ClassFlags & CLASS_Transient )
		InFlags |= RF_Transient;

	// Set the base properties.
	Obj->Index			 = INDEX_NONE;
	Obj->HashNext		 = NULL;
	Obj->StateFrame      = NULL;
	Obj->_Linker		 = NULL;
	Obj->_LinkerIndex	 = INDEX_NONE;
	Obj->Outer			 = InOuter;
	Obj->ObjectFlags	 = InFlags;
	Obj->Name			 = InName;
	Obj->Class			 = InClass;

	// Init the properties.
	if( InTemplate && InTemplate->GetClass()!=InClass )
	{
		InitProperties( (BYTE*)Obj, InClass->GetPropertiesSize(), InClass, NULL, 0, (InFlags&RF_NeedLoad) ? NULL : Obj, SuperObject );

		// Manually copy the changed common variables.
		UClass* CommonBase = Cast<UClass>(InClass->GetCommonBase(InTemplate->GetClass()));	check(CommonBase);
		UObject* Defaults = CommonBase->GetDefaultObject();
		//debugf(TEXT("Manually copying changed variables using %s as the common base."),CommonBase->GetName());
		for( TFieldIterator<UProperty> It(CommonBase); It; ++It )
		{
			UProperty* Property = *It;
			if( (Property->PropertyFlags & CPF_Edit) && !(Property->PropertyFlags & CPF_EditConst) )
			{
				for( INT Index=0; Index<Property->ArrayDim; Index++ )
				{
					INT Offset = Property->Offset + Index*Property->ElementSize;
					BYTE* TemplateVar = (BYTE*)InTemplate + Offset;
					BYTE* DefaultsVar = (BYTE*)Defaults + Offset;
					BYTE* NewClassVar = (BYTE*)Obj + Offset; 
					if( !Property->Identical( TemplateVar, DefaultsVar ) )		// if it was changed from the defaults.
					{
						//debugf(TEXT("Copying %s[%d]"),Property->GetName(),Index);
						Property->CopySingleValue( NewClassVar, TemplateVar );	// copy it to the new object.
					}
					//else debugf(TEXT("Skipping %s[%d]"),Property->GetName(),Index);
				}
			}
			//else debugf(TEXT("Skipping %s"),Property->GetName());
		}
	} else
	InitProperties( (BYTE*)Obj, InClass->GetPropertiesSize(), InClass, (BYTE*)InTemplate, InClass->GetPropertiesSize(), (InFlags&RF_NeedLoad) ? NULL : Obj, SuperObject );

	// Add to global table.
	Obj->AddObject( Index );
	check(Obj->IsValid());

	// If config is per-object, load it.
	if( InClass->ClassFlags & CLASS_PerObjectConfig )
	{
		Obj->LoadConfig();
		Obj->LoadLocalized();
	}

	// Restore class information if replacing native class.
	if( Cls )
	{
		Cls->ClassWithin	   = ClassWithin;
		Cls->ClassFlags       |= ClassFlags;
		Cls->ClassConstructor  = ClassConstructor;
	}

	// Done afterward so we get better error messages.
	if( GObjPurgingGarbage )
		appErrorf(TEXT("Attempt to create object during garbage collection!"));

	// Success.
	return Obj;
	unguardf(( TEXT("(%s %s)"), InClass ? InClass->GetName() : TEXT("NULL"), *InName ));
}

//
// Construct an object.
//
UObject* UObject::StaticConstructObject
(
	UClass*			InClass,
	UObject*		InOuter,
	FName			InName,
	DWORD			InFlags,
	UObject*		InTemplate,
	FOutputDevice*	Error,
	UObject*		SuperObject
)
{
	guard(UObject::StaticConstructObject);
	check(Error);

	// Allocate the object.
	UObject* Result = StaticAllocateObject( InClass, InOuter, InName, InFlags, InTemplate, Error, NULL, SuperObject );
	if( Result )
		(*InClass->ClassConstructor)( Result );
	return Result;

	unguard;
}

/*-----------------------------------------------------------------------------
   Garbage collection.
-----------------------------------------------------------------------------*/

//
// Serialize the global root set to an archive.
//
void UObject::SerializeRootSet( FArchive& Ar, DWORD KeepFlags, DWORD RequiredFlags )
{
	guard(UObject::SerializeRootSet);
	Ar << GObjRoot;
	for( FObjectIterator It; It; ++It )
	{
		if
		(	(It->GetFlags() & KeepFlags)
		&&	(It->GetFlags()&RequiredFlags)==RequiredFlags )
		{
			UObject* Obj = *It;
			Ar << Obj;
		}
	}
	unguard;
}

//
// Archive for finding unused objects.
//
class FArchiveTagUsed : public FArchive
{
public:
	FArchiveTagUsed()
	: Context( NULL )
	{
		guard(FArchiveTagUsed::FArchiveTagUsed);
		GGarbageRefCount=0;

		// Tag all objects as unreachable.
		for( FObjectIterator It; It; ++It )
			It->SetFlags( RF_Unreachable | RF_TagGarbage );

		// Tag all names as unreachable.
		for( INT i=0; i<FName::GetMaxNames(); i++ )
			if( FName::GetEntry(i) )
				FName::GetEntry(i)->Flags |= RF_Unreachable;

		unguard;
	}
private:
	FArchive& operator<<( UObject*& Object )
	{
		guardSlow(FArchiveTagUsed<<Obj);
		GGarbageRefCount++;

		// Object could be a misaligned pointer.
		// Copy the contents of the pointer into a temporary and work on that.
		UObject* Obj;
		appMemcpy(&Obj, &Object, sizeof(INT));
		
#if DO_GUARD_SLOW
		guard(CheckValid);
		if( Obj )
			check(Obj->IsValid());
		unguard;
#endif
		if( Obj && (Obj->GetFlags() & RF_EliminateObject) )
		{
			// Dereference it.
			Obj = NULL;
		}
		else if( Obj && (Obj->GetFlags() & RF_Unreachable) )
		{
			// Only recurse the first time object is claimed.
			guard(TestReach);
			Obj->ClearFlags( RF_Unreachable | RF_DebugSerialize );

			// Recurse.
			if( Obj->GetFlags() & RF_TagGarbage )
			{
				// Recurse down the object graph.
				UObject* OriginalContext = Context;
				Context = Obj;
				Obj->Serialize( *this );
				if( !(Obj->GetFlags() & RF_DebugSerialize) )
					appErrorf( TEXT("%s failed to route Serialize"), Obj->GetFullName() );
				Context = OriginalContext;
			}
			else
			{
				// For debugging.
				debugfSlow( NAME_Log, TEXT("%s is referenced by %s"), Obj->GetFullName(), Context ? Context->GetFullName() : NULL );
			}
			unguardf(( TEXT("(%s)"), Obj->GetFullName() ));
		}

		// Contents of pointer might have been modified.
		// Copy the results back into the misaligned pointer.
		appMemcpy(&Object, &Obj, sizeof(INT));
		return *this;
		unguardSlow;
	}
	FArchive& operator<<( FName& Name )
	{
		guardSlow(FArchiveTagUsed::Name);
		Name.ClearFlags( RF_Unreachable );
		return *this;
		unguardSlow;
	}
	UObject* Context;
};

//
// Purge garbage.
//
void UObject::PurgeGarbage()
{
	guard(UObject::PurgeGarbage);
	INT CountBefore=0, CountPurged=0;
	if( GNoGC )
	{
		debugf( NAME_Log, TEXT("Not purging garbage") );
		return;
	}

	// Dump all profile results.
#if USCRIPT_PROFILE
	if( !GExitPurge )
		DumpStats();
#endif

	debugf( NAME_Log, TEXT("Purging garbage") );

	GObjPurgingGarbage=1;

	// Dispatch all Destroy messages.
	guard(DispatchDestroys);
	for( INT i=0; i<GObjObjects.Num(); i++ )
	{
		guard(DispatchDestroy);
		CountBefore += (GObjObjects(i)!=NULL);
		if
		(	GObjObjects(i)
		&&	(GObjObjects(i)->GetFlags() & RF_Unreachable)
		&&  (!(GObjObjects(i)->GetFlags() & RF_Native) || GExitPurge) )
		{
			debugfSlow( NAME_DevGarbage, TEXT("Garbage collected object %i: %s"), i, GObjObjects(i)->GetFullName() );
			GObjObjects(i)->ConditionalDestroy();
			CountPurged++;
		}
		unguardf(( TEXT("(%i: %s)"), i, GObjObjects(i)->GetFullName() ));
	}

	unguard;

	if( !FTimerManager::GetInstance()->ValidateRefs() )
		debugf( L"WARNING (UObject::PurgeGarbage): FTimerManager had references to destroyed actor(s)!" );

	// Purge all unreachable objects.
	//warning: Can't use FObjectIterator here because classes may be destroyed before objects.
	FName DeleteName=NAME_None;
	guard(DeleteGarbage);
	for( INT i=0; i<GObjObjects.Num(); i++ )
	{
		guard(DeleteObject);
		if
		(	GObjObjects(i)
		&&	(GObjObjects(i)->GetFlags() & RF_Unreachable) 
		&& !(GObjObjects(i)->GetFlags() & RF_Native) )
		{
			DeleteName = GObjObjects(i)->GetFName();
			delete GObjObjects(i);
		}
		unguardf(( TEXT("(%i)"), i ));
	}
	unguardf(( TEXT("(%s)"), *DeleteName ));

	// Purge all unreachable names.
	guard(Names);
	for( INT i=0; i<FName::GetMaxNames(); i++ )
	{
		FNameEntry* Name = FName::GetEntry(i);
		if
		(	(Name)
		&&	(Name->Flags & RF_Unreachable)
		&& !(Name->Flags & RF_Native     ) )
		{
			debugfSlow( NAME_DevGarbage, TEXT("Garbage collected name %i: %s"), i, Name->Name );
			FName::DeleteEntry(i);
		}
	}
	unguard;

	GObjPurgingGarbage=0;

	debugf(TEXT("Garbage: objects: %i->%i; refs: %i"),CountBefore,CountBefore-CountPurged,GGarbageRefCount);
	unguard;
}

//
// Delete all unreferenced objects.
//
void UObject::CollectGarbage( DWORD KeepFlags )
{
	guard(UObject::CollectGarbage);
	debugf( NAME_Log, TEXT("Collecting garbage") );
#if 0 // jmarshall - broken.
	// Tag and purge garbage.
	FArchiveTagUsed TagUsedAr;
	SerializeRootSet( TagUsedAr, KeepFlags, RF_TagGarbage );
#endif
	// Purge it.
	PurgeGarbage();

	unguard;
}

//
// Returns whether an object is referenced, not counting the
// one reference at Obj. No side effects.
//
UBOOL UObject::IsReferenced( UObject*& Obj, DWORD KeepFlags, UBOOL IgnoreReference )
{
	guard(UObject::RefCount);

	// Remember it.
	UObject* OriginalObj = Obj;
	if( IgnoreReference )
		Obj = NULL;

	// Tag all garbage.
	FArchiveTagUsed TagUsedAr;
	OriginalObj->ClearFlags( RF_TagGarbage );
	SerializeRootSet( TagUsedAr, KeepFlags, RF_TagGarbage );

	// Stick the reference back.
	Obj = OriginalObj;

	// Return whether this is tagged.
	return (Obj->GetFlags() & RF_Unreachable)==0;
	unguard;
}

//
// Attempt to delete an object. Only succeeds if unreferenced.
//
UBOOL UObject::AttemptDelete( UObject*& Obj, DWORD KeepFlags, UBOOL IgnoreReference )
{
	guard(UObject::AttemptDelete);
	if( !(Obj->GetFlags() & RF_Native) && !IsReferenced( Obj, KeepFlags, IgnoreReference ) )
	{
		PurgeGarbage();
		return 1;
	}
	else return 0;
	unguard;
}

/*-----------------------------------------------------------------------------
	Importing and exporting.
-----------------------------------------------------------------------------*/

//
// Import an object from a file.
//
void UObject::ParseParms( const TCHAR* Parms )
{
	guard(ParseObjectParms);
	if( !Parms )
		return;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{
		if( It->GetOuter()!=UObject::StaticClass() )
		{
			FString Value;
			if( Parse(Parms,*(FString(It->GetName())+TEXT("=")),Value) )
				It->ImportText( *Value, (BYTE*)this + It->Offset, PPF_Localized );
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	Driver support.
-----------------------------------------------------------------------------*/

//
// Cache a list of globally registered objects into memory.
//
void UObject::CacheDrivers( UBOOL ForceRefresh )
{
	guard(UObject::CacheDrivers);
	TCHAR Buffer[31500];
	if( ForceRefresh || appStricmp(GObjCachedLanguage,UObject::GetLanguage())!=0 )
	{
		appStrcpy( GObjCachedLanguage, UObject::GetLanguage() );
		GObjPreferences.Empty();
		GObjDrivers.Empty();
		for( INT i=0; i<GSys->Paths.Num(); i++ )
		{
			TCHAR Filename[256];
			appSprintf( Filename, TEXT("%s%s"), appBaseDir(), *GSys->Paths(i) );
			TCHAR* Tmp = appStrstr( Filename, TEXT("*.") );
			if( Tmp )
			{
				appSprintf( Tmp, TEXT("*.int") );
				TArray<FString> Files = GFileManager->FindFiles( Filename, 1, 0 );
				for( INT j=0; j<Files.Num(); j++ )
				{
					appSprintf( Tmp, TEXT("%s%s"), appBaseDir(), *Files(j) );
					TCHAR* End = Tmp + appStrlen( Tmp ) - 4;
					appSprintf( End, TEXT(".%s"), UObject::GetLanguage() );
					UBOOL Success = GConfig->GetSection( TEXT("Public"), Buffer, ARRAY_COUNT(Buffer), Tmp );
					if( !Success )
					{
						appSprintf( End, TEXT(".int") );
						Success = GConfig->GetSection( TEXT("Public"), Buffer, ARRAY_COUNT(Buffer), Tmp );
					}
					if( Success )
					{
						TCHAR* Next;
						for( TCHAR* Key=Buffer; *Key; Key=Next )
						{
							Next = Key + appStrlen(Key) + 1;
							TCHAR* Value = appStrstr(Key,TEXT("="));
							if( Value )
							{
								*Value++ = 0;
								if( *Value=='(' )
									*Value++=0;
								if( *Value && Value[appStrlen(Value)-1]==')' )
									Value[appStrlen(Value)-1]=0;
								if( appStricmp(Key,TEXT("Object"))==0 )
								{
									FRegistryObjectInfo& Info = *new(GObjDrivers)FRegistryObjectInfo;
									Parse( Value, TEXT("Name="), Info.Object );
									Parse( Value, TEXT("Class="), Info.Class );
									Parse( Value, TEXT("MetaClass="), Info.MetaClass );
									Parse( Value, TEXT("Description="), Info.Description );
									Parse( Value, TEXT("Autodetect="), Info.Autodetect );
								}
								else if( appStricmp(Key,TEXT("Preferences"))==0 )
								{
									FPreferencesInfo& Info = *new(GObjPreferences)FPreferencesInfo;
									Parse( Value, TEXT("Caption="), Info.Caption );
									Parse( Value, TEXT("Parent="), Info.ParentCaption );
									Parse( Value, TEXT("Class="), Info.Class );
									Parse( Value, TEXT("Category="), Info.Category );
									ParseUBOOL( Value, TEXT("Immediate="), Info.Immediate );
									Info.Category.SetFlags( RF_Native );
								}
							}
						}
					}
				}
			}
		}

		TArray<FString> CoreLangs = GFileManager->FindFiles( TEXT("Core.*"), 1, 0 );

		for( INT i=0; i<CoreLangs.Num(); i++ )
		{
			FString Ext;
			if( CoreLangs(i).Split(FString(TEXT(".")),NULL,&Ext,1) )
			{
				Ext = FString(TEXT(".")) + Ext;
				if( Ext!=DLLEXT && Ext!=TEXT(".u") && Ext!=TEXT(".ilk") )
				{
					FRegistryObjectInfo& Info = *new(GObjDrivers)FRegistryObjectInfo;
					Info.Object		= appStrstr(*CoreLangs(i),TEXT("."))+1;
					Info.Class		= TEXT("Class");
					Info.MetaClass	= ULanguage::StaticClass()->GetPathName();
				}
			}
		}
	}
	unguard;
}

//
// Get a list of globally registered objects complying to 
// the specified class and optional metaclass (base class of UClasses).
//
void UObject::GetRegistryObjects
(
	TArray<FRegistryObjectInfo>&	Results,
	UClass*							Class,
	UClass*							MetaClass,
	UBOOL							ForceRefresh
)
{
	guard(UObject::GetDriverClasses);
	check(Class);
	check(Class!=UClass::StaticClass() || MetaClass);
	CacheDrivers( ForceRefresh );
	const TCHAR* ClassName = Class->GetName();
	const TCHAR* MetaClassName = MetaClass ? MetaClass->GetPathName() : TEXT("");
	for( INT i=0; i<GObjDrivers.Num(); i++ )
	{
		if
		(	appStricmp(*GObjDrivers(i).Class, ClassName)==0
		&&	appStricmp(*GObjDrivers(i).MetaClass, MetaClassName)==0 )
			new(Results)FRegistryObjectInfo(GObjDrivers(i));
	}
	unguard;
}

//
// Get a list of everything that should show up in the hierarchical
// preferences window.
//
void UObject::GetPreferences( TArray<FPreferencesInfo>& Results, const TCHAR* ParentCaption, UBOOL ForceRefresh )
{
	guard(UObject::GetPreferences);
	CacheDrivers( ForceRefresh );
	Results.Empty();
	for( INT i=0; i<GObjPreferences.Num(); i++ )
		if( appStricmp(*GObjPreferences(i).ParentCaption,ParentCaption)==0 )
			new(Results)FPreferencesInfo(GObjPreferences(i));
	unguard;
}

/*-----------------------------------------------------------------------------
	UTextBuffer implementation.
-----------------------------------------------------------------------------*/

UTextBuffer::UTextBuffer( const TCHAR* InText )
: Text( InText )
{}
void UTextBuffer::Serialize( const TCHAR* Data, EName Event )
{
	guard(UTextBuffer::Serialize);
	Text += (TCHAR*)Data;
	unguardobj;
}
void UTextBuffer::Serialize( FArchive& Ar )
{
	guard(UTextBuffer::Serialize);
	Super::Serialize(Ar);
	Ar << Pos << Top << Text;
	unguardobj;
}
IMPLEMENT_CLASS(UTextBuffer);

/*-----------------------------------------------------------------------------
	UEnum implementation.
-----------------------------------------------------------------------------*/

UEnum::UEnum( UEnum* InSuperEnum )
: UField( InSuperEnum )
{}
void UEnum::Serialize( FArchive& Ar )
{
	guard(UEnum::Serialize);
	Super::Serialize(Ar);
	Ar << Names;
	unguardobj;
}
IMPLEMENT_CLASS(UEnum);

/*-----------------------------------------------------------------------------
	FCompactIndex implementation.
-----------------------------------------------------------------------------*/

//
// FCompactIndex serializer.
//
FArchive& operator<<( FArchive& Ar, FCompactIndex& I )
{
	guard(FCompactIndex<<);
	if( !Ar.IsLoading() && !Ar.IsSaving() )
	{
		Ar << I.Value;
	}
	else
	{
		INT   Original = I.Value;
		DWORD V        = Abs(I.Value);
		BYTE  B0       = ((I.Value>=0) ? 0 : 0x80) + ((V < 0x40) ? V : ((V & 0x3f)+0x40));
		I.Value        = 0;
		Ar << B0;
		if( B0 & 0x40 )
		{
			V >>= 6;
			BYTE B1 = (V < 0x80) ? V : ((V & 0x7f)+0x80);
			Ar << B1;
			if( B1 & 0x80 )
			{
				V >>= 7;
				BYTE B2 = (V < 0x80) ? V : ((V & 0x7f)+0x80);
				Ar << B2;
				if( B2 & 0x80 )
				{
					V >>= 7;
					BYTE B3 = (V < 0x80) ? V : ((V & 0x7f)+0x80);
					Ar << B3;
					if( B3 & 0x80 )
					{
						V >>= 7;
						BYTE B4 = V;
						Ar << B4;
						I.Value = B4;
					}
					I.Value = (I.Value << 7) + (B3 & 0x7f);
				}
				I.Value = (I.Value << 7) + (B2 & 0x7f);
			}
			I.Value = (I.Value << 7) + (B1 & 0x7f);
		}
		I.Value = (I.Value << 6) + (B0 & 0x3f);
		if( B0 & 0x80 )
			I.Value = -I.Value;
		if( Ar.IsSaving() && I.Value!=Original )
			appErrorf( TEXT("Mismatch: %08X %08X"), I.Value, Original );
	}
	return Ar;
	unguard;
}

/*-----------------------------------------------------------------------------
	Implementations.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(ULinker);
IMPLEMENT_CLASS(ULinkerLoad);
IMPLEMENT_CLASS(ULinkerSave);
IMPLEMENT_CLASS(USubsystem);

/*-----------------------------------------------------------------------------
	UCommandlet.
-----------------------------------------------------------------------------*/

UCommandlet::UCommandlet()
: HelpCmd(E_NoInit), HelpOneLiner(E_NoInit), HelpUsage(E_NoInit), HelpWebLink(E_NoInit)
{}
INT UCommandlet::Main( const TCHAR* Parms )
{
	guard(UCommandlet::Main);
	return eventMain( Parms );
	unguard;
}
void UCommandlet::execMain( FFrame& Stack, RESULT_DECL )
{
	guardSlow(UCommandlet::execMain);

	P_GET_STR(Parms);
	P_FINISH;

	*(INT*)Result = Main( *Parms );

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( UCommandlet, INDEX_NONE, execMain );
IMPLEMENT_CLASS(UCommandlet);

/*-----------------------------------------------------------------------------
	UPackage.
-----------------------------------------------------------------------------*/

UPackage::UPackage()
{
	guard(UPackage::UPackage);

	// Bind to a matching DLL, if any.
	BindPackage( this );
	bDirty = 0;

	unguard;
}
void UPackage::Serialize( FArchive& Ar )
{
	guard(UPackage::Serialize);
	Super::Serialize( Ar );
	unguard;
}
void UPackage::Destroy()
{
	guard(UPackage::Destroy);
	if( DllHandle )
	{
		#if !__STATIC_LINK
		debugf( NAME_Progress, TEXT("Unbound to %s%s"), GetName(), DLLEXT );
		#endif
		//Danger: If package is garbage collected before its autoregistered classes,
		//the autoregistered classes will be unmapped from memory.
		//appFreeDllHandle( DllHandle );
	}
	Super::Destroy();
	unguard;
}
void* UPackage::GetDllExport( const TCHAR* ExportName, UBOOL Checked )
{
	guard(UPackage::GetDllExport);
	void* Result;
	#if !__STATIC_LINK
	if( !DllHandle )
	{
		if( Checked && !ParseParam(appCmdLine(),TEXT("nobind")) )
			appErrorf( LocalizeError(TEXT("NotDll"),TEXT("Core")), GetName(), ExportName );
		Result = NULL;
	}
	else
	#endif
	{
		#if __STATIC_LINK
		Result = FindNative( (TCHAR*) ExportName );
		#else
		Result = appGetDllExport( DllHandle, ExportName );
		#endif
		if( !Result && Checked && !ParseParam(appCmdLine(),TEXT("nobind")) )
			appErrorf( LocalizeError(TEXT("NotInDll"),TEXT("Core")), ExportName, GetName() );
		if( Result )
			debugfSlow( NAME_DevBind, TEXT("Found %s in %s%s"), ExportName, GetName(), DLLEXT );
		
	}
	return Result;
	unguard;
}
IMPLEMENT_CLASS(UPackage);

/*-----------------------------------------------------------------------------
	UTextBufferFactory.
-----------------------------------------------------------------------------*/

void UTextBufferFactory::StaticConstructor()
{
	guard(UTextBufferFactory::StaticConstructor);

	SupportedClass = UTextBuffer::StaticClass();
	bCreateNew     = 0;
	bText          = 1;
	new(Formats)FString( TEXT("txt;Text files") );

	unguard;
}
UTextBufferFactory::UTextBufferFactory()
{}
UObject* UTextBufferFactory::FactoryCreateText
(
	ULevel*				InLevel,
	UClass*				Class,
	UObject*			InOuter,
	FName				InName,
	DWORD				InFlags,
	UObject*			Context,
	const TCHAR*		Type,
	const TCHAR*&		Buffer,
	const TCHAR*		BufferEnd,
	FFeedbackContext*	Warn
)
{
	guard(UTextBufferFactory::FactoryCreateText);

	// Import.
	UTextBuffer* Result = new(InOuter,InName,InFlags)UTextBuffer;
	Result->Text = Buffer;
	return Result;

	unguard;
}
IMPLEMENT_CLASS(UTextBufferFactory);

/*----------------------------------------------------------------------------
	ULanguage.
----------------------------------------------------------------------------*/

const TCHAR* UObject::GetLanguage()
{
	guard(UObject::GetLanguage);
	return GLanguage;
	unguard;
}
void UObject::SetLanguage( const TCHAR* LangExt )
{
	guard(UObject::SetLanguage);
	if( appStricmp(LangExt,GLanguage)!=0 )
	{
		appStrcpy( GLanguage, LangExt );
		appStrcpy( GNone,  LocalizeGeneral( TEXT("None"),  TEXT("Core")) );
		appStrcpy( GTrue,  LocalizeGeneral( TEXT("True"),  TEXT("Core")) );
		appStrcpy( GFalse, LocalizeGeneral( TEXT("False"), TEXT("Core")) );
		appStrcpy( GYes,   LocalizeGeneral( TEXT("Yes"),   TEXT("Core")) );
		appStrcpy( GNo,    LocalizeGeneral( TEXT("No"),    TEXT("Core")) );
		for( FObjectIterator It; It; ++It )
			It->LanguageChange();
	}
	unguard;
}
IMPLEMENT_CLASS(ULanguage);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

