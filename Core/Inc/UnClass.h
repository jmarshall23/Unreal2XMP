/*=============================================================================
	UnClass.h: UClass definition.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney.
=============================================================================*/
#ifdef _MSC_VER
#pragma warning( disable : 4121 )
#endif
// vogel: alignment of a member was sensitive to packing

/*-----------------------------------------------------------------------------
	Constants.
-----------------------------------------------------------------------------*/

// Boundary to align class properties on.
enum {PROPERTY_ALIGNMENT=4 };

#include "THashTable.h"

/*-----------------------------------------------------------------------------
	FRepRecord.
-----------------------------------------------------------------------------*/

//
// Information about a property to replicate.
//
struct FRepRecord
{
	UProperty* Property;
	INT Index;
	FRepRecord(UProperty* InProperty,INT InIndex)
	: Property(InProperty), Index(InIndex)
	{}
};

/*-----------------------------------------------------------------------------
	FDependency.
-----------------------------------------------------------------------------*/

//
// One dependency record, for incremental compilation.
//
class CORE_API FDependency
{
public:
	// Variables.
	UClass*		Class;
	UBOOL		Deep;
	DWORD		ScriptTextCRC;

	// Functions.
	FDependency();
	FDependency( UClass* InClass, UBOOL InDeep );
	UBOOL IsUpToDate();
	CORE_API friend FArchive& operator<<( FArchive& Ar, FDependency& Dep );
};

/*-----------------------------------------------------------------------------
	FRepLink.
-----------------------------------------------------------------------------*/

//
// A tagged linked list of replicatable variables.
//
class FRepLink
{
public:
	UProperty*	Property;		// Replicated property.
	FRepLink*	Next;			// Next replicated link per class.
	FRepLink( UProperty* InProperty, FRepLink* InNext )
	:	Property	(InProperty)
	,	Next		(InNext)
	{}
};

/*-----------------------------------------------------------------------------
	FLabelEntry.
-----------------------------------------------------------------------------*/

//
// Entry in a state's label table.
//
struct CORE_API FLabelEntry
{
	// Variables.
	FName	Name;
	INT		iCode;

	// Functions.
	FLabelEntry( FName InName, INT iInCode );
	CORE_API friend FArchive& operator<<( FArchive& Ar, FLabelEntry &Label );
};

/*-----------------------------------------------------------------------------
	UField.
-----------------------------------------------------------------------------*/

// To speed up DOREP, moved from UnClass.cpp
#define VF_HASH_VARIABLES 0 /* Undecided!! */

//
// Base class of UnrealScript language objects.
//
class CORE_API UField : public UObject
{
	DECLARE_ABSTRACT_CLASS(UField,UObject,0,Core)
	NO_DEFAULT_CONSTRUCTOR(UField)

	// Constants.
#if VF_HASH_VARIABLES
	enum {HASH_COUNT = 256*4};
#else
	enum {HASH_COUNT = 256};
#endif

	// Variables.
	UField*			SuperField;
	UField*			Next;
	UField*			HashNext;

	// Constructors.
	UField( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags, UField* InSuperField );
	UField( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags );
	UField( UField* InSuperField );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void PostLoad();
	void Register();

	// UField interface.
	virtual void AddCppProperty( UProperty* Property );
	virtual UBOOL MergeBools();
	virtual void Bind();
	virtual UClass* GetOwnerClass();
	virtual INT GetPropertiesSize();
};

/*-----------------------------------------------------------------------------
	TFieldIterator.
-----------------------------------------------------------------------------*/

//
// For iterating through a linked list of fields.
//
template <class T, EClassFlags Flag=CLASS_None> class TFieldIterator 
{
public:
	TFieldIterator( UStruct* InStruct )
	: Struct( InStruct )
	, Field( InStruct ? InStruct->Children : NULL )
	{
		IterateToNext();
	}
	inline operator UBOOL()
	{
		return Field != NULL;
	}
	inline void operator++()
	{
		checkSlow(Field);
		Field = Field->Next;
		IterateToNext();
	}
	inline T* operator*()
	{
		checkSlow(Field);
		return (T*)Field;
	}
	inline T* operator->()
	{
		checkSlow(Field);
		return (T*)Field;
	}
	inline UStruct* GetStruct()
	{
		return Struct;
	}
protected:
	inline void IterateToNext()
	{
		if ( Flag != CLASS_None )
		{
			while( Struct )
			{
				while( Field )
				{
					if( (Field->GetClass()->ClassFlags & Flag) )
						return;
					Field = Field->Next;
				}
				Struct = Struct->GetInheritanceSuper();
				if( Struct )
					Field = Struct->Children;
			}
		}
		else
		{
			while( Struct )
			{
				while( Field )
				{
					if( Field->IsA(T::StaticClass()) )
						return;
					Field = Field->Next;
				}
				Struct = Struct->GetInheritanceSuper();
				if( Struct )
					Field = Struct->Children;
			}
		}
	}
	UStruct* Struct;
	UField* Field;
};

/*-----------------------------------------------------------------------------
	UStruct.
-----------------------------------------------------------------------------*/

enum EStructFlags
{
	// State flags.
	STRUCT_Native		= 0x00000001,	
	STRUCT_Export		= 0x00000002,
};

//
// An UnrealScript structure definition.
//
class CORE_API UStruct : public UField
{
	DECLARE_CLASS(UStruct,UField,0,Core)
	NO_DEFAULT_CONSTRUCTOR(UStruct)

	// Variables.
	UTextBuffer*		ScriptText;
	UTextBuffer*		CppText;
	UField*				Children;
	INT					PropertiesSize;
	FName				FriendlyName;
	TArray<BYTE>		Script;

	// Compiler info.
	INT					TextPos;
	INT					Line;
	DWORD					StructFlags;

	// In memory only.
	UProperty*			RefLink;
	UProperty*			PropertyLink;
	UProperty*			ConfigLink;
	UProperty*			ConstructorLink;

	// Constructors.
	UStruct( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags, UStruct* InSuperStruct );
	UStruct( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags );
	UStruct( UStruct* InSuperStruct );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void PostLoad();
	void Destroy();
	void Register();

	// UField interface.
	void AddCppProperty( UProperty* Property );

	// UStruct interface.
	virtual UStruct* GetInheritanceSuper() {return GetSuperStruct();}
	virtual void Link( FArchive& Ar, UBOOL Props );
	virtual void SerializeBin( FArchive& Ar, BYTE* Data, INT MaxReadBytes );
	virtual void SerializeTaggedProperties( FArchive& Ar, BYTE* Data, UClass* DefaultsClass );
	virtual void CleanupDestroyed( BYTE* Data );
	virtual UBOOL ValidateRefs( BYTE* Data ); //NEW (mdf) tbd: object refs validation
	virtual EExprToken SerializeExpr( INT& iCode, FArchive& Ar );
	INT GetPropertiesSize()
	{
		return PropertiesSize;
	}
	DWORD GetScriptTextCRC()
	{
		return ScriptText ? appStrCrc(*ScriptText->Text) : 0;
	}
	void SetPropertiesSize( INT NewSize )
	{
		PropertiesSize = NewSize;
	}
	UBOOL IsChildOf( const UStruct* SomeBase ) const
	{
		guardSlow(UStruct::IsChildOf);
		for( const UStruct* Struct=this; Struct; Struct=Struct->GetSuperStruct() )
			if( Struct==SomeBase ) 
				return 1;
		return 0;
		unguardobjSlow;
	}
	UBOOL IsChildOf( const TCHAR* SomeBaseName ) const
	{
		guardSlow(UStruct::IsChildOf(string));
		for( const UStruct* Struct=this; Struct; Struct=Struct->GetSuperStruct() )
			if( appStricmp(Struct->GetName(),SomeBaseName)==0 ) 
				return 1;
		return appStricmp(SomeBaseName,TEXT(""))==0;
		unguardobjSlow;
	}
	UStruct* GetCommonBase( const UStruct* Other )
	{
		guardSlow(UStruct::GetCommonBase);
		for( UStruct* Struct=this; Struct; Struct=Struct->GetSuperStruct() )
			if( Other->IsChildOf(Struct) ) 
				return Struct;
		return NULL;
		unguardobjSlow
	}
	virtual TCHAR* GetNameCPP()
	{
		TCHAR* Result = appStaticString1024();
		appSprintf( Result, TEXT("F%s"), GetName() );
		return Result;
	}
	UStruct* GetSuperStruct() const
	{
		guardSlow(UStruct::GetSuperStruct);
		checkSlow(!SuperField||SuperField->IsA(UStruct::StaticClass()));
		return (UStruct*)SuperField;
		unguardSlow;
	}
	UBOOL StructCompare( const void* A, const void* B );
	FString FunctionMD5();		// Returns the Quick MD5 hash for this package	

protected:

	// Cheat Protection
		
	BYTE FunctionMD5Digest[16];		// Holds a MD5 digest for this function
};

/*-----------------------------------------------------------------------------
	UFunction.
-----------------------------------------------------------------------------*/

//
// An UnrealScript function.
//
class CORE_API UFunction : public UStruct
{
	DECLARE_CLASS(UFunction,UStruct,CLASS_IsAUFunction,Core)
	DECLARE_WITHIN(UState)
	NO_DEFAULT_CONSTRUCTOR(UFunction)

	// Persistent variables.
	DWORD FunctionFlags;
	_WORD iNative;
	_WORD RepOffset;
	BYTE  OperPrecedence;

	// Variables in memory only.
	BYTE  NumParms;
	_WORD ParmsSize;
	_WORD ReturnValueOffset;
	void (UObject::*Func)( FFrame& TheStack, RESULT_DECL );
#if USCRIPT_PROFILE
	SQWORD Calls,Cycles;
#endif

	// Constructors.
	UFunction( UFunction* InSuperFunction );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void PostLoad();

	// UField interface.
	void Bind();

	// UStruct interface.
	UBOOL MergeBools() {return 0;}
	UStruct* GetInheritanceSuper() {return NULL;}
	void Link( FArchive& Ar, UBOOL Props );

	// UFunction interface.
	UFunction* GetSuperFunction() const
	{
		guardSlow(UFunction::GetSuperFunction);
		checkSlow(!SuperField||SuperField->IsA(UFunction::StaticClass()));
		return (UFunction*)SuperField;
		unguardSlow;
	}
	UProperty* GetReturnProperty();
};

/*-----------------------------------------------------------------------------
	UState.
-----------------------------------------------------------------------------*/

//
// An UnrealScript state.
//
class CORE_API UState : public UStruct
{
	DECLARE_CLASS(UState,UStruct,CLASS_IsAUState,Core)
	NO_DEFAULT_CONSTRUCTOR(UState)

	// Variables.
	QWORD ProbeMask;
	QWORD IgnoreMask;
	DWORD StateFlags;
	_WORD LabelTableOffset;
	UField* VfHash[HASH_COUNT];

	// Constructors.
	UState( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags, UState* InSuperState );
	UState( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags );
	UState( UState* InSuperState );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();

	// UStruct interface.
	UBOOL MergeBools() {return 1;}
	UStruct* GetInheritanceSuper() {return GetSuperState();}
	void Link( FArchive& Ar, UBOOL Props );

	// UState interface.
	UState* GetSuperState() const
	{
		guardSlow(UState::GetSuperState);
		checkSlow(!SuperField||SuperField->IsA(UState::StaticClass()));
		return (UState*)SuperField;
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	UEnum.
-----------------------------------------------------------------------------*/

//
// An enumeration, a list of names usable by UnrealScript.
//
class CORE_API UEnum : public UField
{
	DECLARE_CLASS(UEnum,UField,0,Core)
	DECLARE_WITHIN(UStruct)
	NO_DEFAULT_CONSTRUCTOR(UEnum)

	// Variables.
	TArray<FName> Names;

	// Constructors.
	UEnum( UEnum* InSuperEnum );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UEnum interface.
	UEnum* GetSuperEnum() const
	{
		guardSlow(UEnum::GetSuperEnum);
		checkSlow(!SuperField||SuperField->IsA(UEnum::StaticClass()));
		return (UEnum*)SuperField;
		unguardSlow;
	}
};

#define GetEnumEx(e,i,p) *StaticGetEnum(FString(TEXT(#e)),i,p)
static FString StaticGetEnum( FString EnumTypeStr, INT i, INT Pre=0 )
{
	guard(Core::StaticGetEnum);

	UEnum* E = Cast<UEnum>(UObject::StaticLoadObject( UEnum::StaticClass(), NULL, *EnumTypeStr, NULL, LOAD_NoWarn, NULL ));

	if( E && i>=0 && i<E->Names.Num() )
		return FString(*E->Names(i)).Mid(Pre);
	else
		return TEXT("None");
	unguard;
}

/*-----------------------------------------------------------------------------
	UClass.
-----------------------------------------------------------------------------*/

//
// An object class.
//
class CORE_API UClass : public UState
{
	DECLARE_CLASS(UClass,UState,0,Core)
	DECLARE_WITHIN(UPackage)

	// Variables.
	DWORD				ClassFlags;
	INT					ClassUnique;
	FGuid				ClassGuid;
	UClass*				ClassWithin;
	FName				ClassConfigName;
	TArray<FRepRecord>	ClassReps;
	TArray<UField*>		NetFields;
	TArray<UProperty*>	NetProperties;
	THashTable<FName,FRepRecord,6>*	NetPropertyMap;
	TArray<FDependency> Dependencies;
	TArray<FName>		PackageImports;
	TArray<BYTE>		Defaults;
	TArray<FName>		HideCategories;
    TArray<FName>       DependentOn;
	void(*ClassConstructor)(void*);
	void(UObject::*ClassStaticConstructor)();

	// In memory only.
	FString				DefaultPropText;

	// Constructors.
	UClass();
	UClass( UClass* InSuperClass );
	UClass( ENativeConstructor, DWORD InSize, DWORD InClassFlags, UClass* InBaseClass, UClass* InWithinClass, FGuid InGuid, const TCHAR* InNameStr, const TCHAR* InPackageName, const TCHAR* InClassConfigName, DWORD InFlags, void(*InClassConstructor)(void*), void(UObject::*InClassStaticConstructor)() );
	UClass( EStaticConstructor, DWORD InSize, DWORD InClassFlags, FGuid InGuid, const TCHAR* InNameStr, const TCHAR* InPackageName, const TCHAR* InClassConfigName, DWORD InFlags, void(*InClassConstructor)(void*), void(UObject::*InClassStaticConstructor)() );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void PostLoad();
	void Destroy();
	void Register();

	// UField interface.
	void Bind();

	// UStruct interface.
	UBOOL MergeBools() {return 1;}
	UStruct* GetInheritanceSuper() {return GetSuperClass();}
	TCHAR* GetNameCPP()
	{
		TCHAR* Result = appStaticString1024();
		UClass* C;
		for( C=this; C; C=C->GetSuperClass() )
			if( appStricmp(C->GetName(),TEXT("Actor"))==0 )
				break;
		appSprintf( Result, TEXT("%s%s"), C ? TEXT("A") : TEXT("U"), GetName() );
		return Result;
	}
	void Link( FArchive& Ar, UBOOL Props );

	// UClass interface.
	void AddDependency( UClass* InClass, UBOOL InDeep )
	{
		guard(UClass::AddDependency);
		INT i;
		for( i=0; i<Dependencies.Num(); i++ )
			if( Dependencies(i).Class==InClass )
				Dependencies(i).Deep |= InDeep;
		if( i==Dependencies.Num() )
			new(Dependencies)FDependency( InClass, InDeep );
		unguard;
	}
	UClass* GetSuperClass() const
	{
		guardSlow(UClass::GetSuperClass);
		return (UClass *)SuperField;
		unguardSlow;
	}
	UObject* GetDefaultObject()
	{
		guardSlow(UClass::GetDefaultObject);
		check(Defaults.Num()==GetPropertiesSize());
		return (UObject*)&Defaults(0);
		unguardobjSlow;
	}
	class AActor* GetDefaultActor()
	{
		guardSlow(UClass::GetDefaultActor);
		check(Defaults.Num());
		return (AActor*)&Defaults(0);
		unguardobjSlow;
	}
	UBOOL HasNativesToExport( UObject* Outer )
	{
		guardSlow(UClass::HasNativesToExport);
		if( GetFlags() & RF_Native )
		{
			if( GetFlags() & RF_TagExp )
				return 1;		
			if( ScriptText && GetOuter() == Outer )
				for( TFieldIterator<UFunction> Function(this); Function && Function.GetStruct()==this; ++Function )
					if( Function->FunctionFlags & FUNC_Native )
						return 1;
		}
		return 0;			
		unguardobjSlow;
	}

private:
	// Hide IsA because calling IsA on a class almost always indicates
	// an error where the caller should use IsChildOf.
	UBOOL IsA( UClass* Parent ) const {return UObject::IsA(Parent);}
};

/*-----------------------------------------------------------------------------
	UConst.
-----------------------------------------------------------------------------*/

//
// An UnrealScript constant.
//
class CORE_API UConst : public UField
{
	DECLARE_CLASS(UConst,UField,0,Core)
	DECLARE_WITHIN(UStruct)
	NO_DEFAULT_CONSTRUCTOR(UConst)

	// Variables.
	FString Value;

	// Constructors.
	UConst( UConst* InSuperConst, const TCHAR* InValue );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UConst interface.
	UConst* GetSuperConst() const
	{
		guardSlow(UConst::GetSuperStruct);
		checkSlow(!SuperField||SuperField->IsA(UConst::StaticClass()));
		return (UConst*)SuperField;
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	FNotifyInfo.
-----------------------------------------------------------------------------*/

//
// Used for sending UnrealScript notifications.
//
class CORE_API FNotifyInfo
{
public:
	FNotifyInfo(){}
	FNotifyInfo( UObject* _Target, FName _NotifyFunc )
		: Target(_Target)
		{ SetFunction(_NotifyFunc); }
	FNotifyInfo( UObject* _Target, const TCHAR* _NotifyFunc )
		: Target(_Target)
		{ SetFunction(FName(_NotifyFunc,FNAME_Find)); if(!Function) debugf(TEXT("   -- %s"),_NotifyFunc); }
	FNotifyInfo( UObject* _Target, FString _NotifyFunc )
		: Target(_Target)
		{ SetFunction(FName(*_NotifyFunc,FNAME_Find)); if(!Function) debugf(TEXT("   -- %s"),*_NotifyFunc); }
	FNotifyInfo operator=( INT Value )	// for initialization purposes in auto-generated headers (as native UnrealScript function parameters).
		{ Target=NULL; Function=NULL; return *this; }
	UBOOL operator==( const FNotifyInfo& Other ) const
		{ return Target==Other.Target && Function==Other.Function; }
	inline UBOOL IsValid()
		{ return Target!=NULL && Function!=NULL; }
	operator UBOOL()
		{ return IsValid(); }
	void SendNotification( void* Parms=NULL )
		{ if(IsValid()) Target->ProcessEvent(Function,Parms); }
	UObject* GetTarget()
		{ return Target; }
	UFunction* GetFunction()
		{ return Function; }
	friend FArchive& operator<<( FArchive& Ar, FNotifyInfo &N )
		{ return Ar << N.Target << N.Function; }
private:
    UObject* Target;
    UFunction* Function;
	void SetFunction( FName FuncName )
	{
		guard(FNotifyInfo::SetFunction);
		if( Target )
		{
			Function = Target->FindFunction( FuncName );
			if( !Function )
				debugf( NAME_Warning, TEXT("Cannot find notify function %s in %s."), *FuncName, Target->GetFullName() );  // (state = '%s') Target->GetStateName() -- this crashes sometimes.
		} else debugf( NAME_Warning, TEXT("Invalid Target: NULL (%s)"), *FuncName );
		unguard;
	}
};

//
// Used for sending UnrealScript notifications (with Interface-only check)
//
class CORE_API FInterfaceNotifyInfo
{
public:
	FInterfaceNotifyInfo(){}
	FInterfaceNotifyInfo( UObject* _Target, FName _NotifyFunc )
		: Target(_Target)
		{ SetFunction(_NotifyFunc); }
	FInterfaceNotifyInfo( UObject* _Target, const TCHAR* _NotifyFunc )
		: Target(_Target)
		{ SetFunction(FName(_NotifyFunc,FNAME_Find)); if(!Function) debugf(TEXT("   -- %s"),_NotifyFunc); }
	FInterfaceNotifyInfo( UObject* _Target, FString _NotifyFunc )
		: Target(_Target)
		{ SetFunction(FName(*_NotifyFunc,FNAME_Find)); if(!Function) debugf(TEXT("   -- %s"),*_NotifyFunc); }
	FInterfaceNotifyInfo operator=( INT Value )	// for initialization purposes in auto-generated headers (as native UnrealScript function parameters).
		{ Target=NULL; Function=NULL; return *this; }
	UBOOL operator==( const FInterfaceNotifyInfo& Other ) const
		{ return Target==Other.Target && Function==Other.Function; }
	inline UBOOL IsValid()
		{ return Target!=NULL && Function!=NULL; }
	operator UBOOL()
		{ return IsValid(); }
	void SendNotification( void* Parms=NULL )
		{ if(IsValid()) Target->ProcessEvent(Function,Parms); }
	UObject* GetTarget()
		{ return Target; }
	UFunction* GetFunction()
		{ return Function; }
	friend FArchive& operator<<( FArchive& Ar, FInterfaceNotifyInfo &N )
		{ return Ar << N.Target << N.Function; }
private:
    UObject* Target;
    UFunction* Function;
	void SetFunction( FName FuncName )
	{
		guard(FInterfaceNotifyInfo::SetFunction);
		if( Target )
		{
			Function = Target->FindFunction( FuncName );
			if( !Function )
				debugf( NAME_Warning, TEXT("Cannot find notify function '%s' in '%s'."), *FuncName, Target->GetFullName() );  // (state = '%s') Target->GetStateName() -- this crashes sometimes.
			else if( !(Function->FunctionFlags & FUNC_Interface) )
				appErrorf( LocalizeError(TEXT("BadInterface"),TEXT("Core")), Function->GetFullName() );
		} else debugf( NAME_Warning, TEXT("Invalid Target: NULL (%s)"), *FuncName );
		unguard;
	}
};

// NOTE[aleiby]: This isn't operational yet.
template< class TClassType, class TFuncParms > class FNativeNotifyInfo
{
public:
	typedef void(TClassType::*FuncPtr)(TFuncParms);
	FNativeNotifyInfo(){}
	FNativeNotifyInfo( TClassType* InTarget, FuncPtr InFunction )
		: Target(InTarget)
		, Function(InFunction) {}
	FNativeNotifyInfo operator=( INT Value )	// for initialization purposes in auto-generated headers (as native UnrealScript function parameters).
		{ Target=NULL; Function=NULL; return *this; }
	UBOOL operator==( const FNativeNotifyInfo& Other ) const
		{ return Target==Other.Target && Function==Other.Function; }
	inline UBOOL IsValid()
		{ return Target!=NULL && Function!=NULL; }
	operator UBOOL()
		{ return IsValid(); }
	void SendNotification(TFuncParms Parms)
		{ if(IsValid()) (Target->*Function)(Parms); }
	void SendNotification()
		{ if(IsValid()) (Target->*Function)(); }
	UObject* GetTarget()
		{ return Target; }
	FuncPtr GetFunction()
		{ return Function; }
	friend FArchive& operator<<( FArchive& Ar, FNativeNotifyInfo &N )
	{
		Ar << N.Target;

		// Fix ARL: I'm quite certain this won't work.
		DWORD Offset = (DWORD)N.Function - (DWORD)N.Target;
		Ar << Offset;
		(DWORD)N.Function = (DWORD)N.Target + Offset;

		return Ar;
	}
private:
    TClassType* Target;
    FuncPtr Function;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

