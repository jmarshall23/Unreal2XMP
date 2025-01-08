/*=============================================================================
	UnClass.cpp: Object class implementation.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FPropertyTag.
-----------------------------------------------------------------------------*/

//
// A tag describing a class property, to aid in serialization.
//
struct FPropertyTag
{
	// Archive for counting property sizes.
	class FArchiveCountSize : public FArchive
	{
	public:
		FArchiveCountSize( FArchive& InSaveAr )
		: Size(0), SaveAr(InSaveAr)
		{
			ArIsSaving     = InSaveAr.IsSaving();
			ArIsPersistent = InSaveAr.IsPersistent();
		}
		INT Size;
	private:
		FArchive& SaveAr;
		FArchive& operator<<( UObject*& Obj )
		{
			INT Index = SaveAr.MapObject(Obj);
			return *this << AR_INDEX(Index);
		}
		FArchive& operator<<( FName& Name )
		{
			INT Index = SaveAr.MapName(&Name);
			return *this << AR_INDEX(Index);
		}
		void Serialize( void* V, INT Length )
		{
			Size += Length;
		}
	};

	// Variables.
	BYTE	Type;		// Type of property, 0=end.
	BYTE	Info;		// Packed info byte.
	FName	Name;		// Name of property.
	FName	ItemName;	// Struct name if UStructProperty.
	INT		Size;       // Property size.
	INT		ArrayIndex;	// Index if an array; else 0.

	// Constructors.
	FPropertyTag()
	{}
	FPropertyTag( FArchive& InSaveAr, UProperty* Property, INT InIndex, BYTE* Value )
	:	Type		( Property->GetID() )
	,	Name		( Property->GetFName() )
	,	ItemName	( NAME_None     )
	,	Size		( 0             )
	,	ArrayIndex	( InIndex       )
	,	Info		( Property->GetID() )
	{
		// Handle structs.
		UStructProperty* StructProperty = Cast<UStructProperty>( Property );
		if( StructProperty )
			ItemName = StructProperty->Struct->GetFName();

		// Set size.
		FArchiveCountSize ArCount( InSaveAr );
		SerializeTaggedProperty( ArCount, Property, Value, 0 );
		Size = ArCount.Size;

		// Update info bits.
		Info |=
		(	Size==1		? 0x00
		:	Size==2     ? 0x10
		:	Size==4     ? 0x20
		:	Size==12	? 0x30
		:	Size==16	? 0x40
		:	Size<=255	? 0x50
		:	Size<=65536 ? 0x60
		:			      0x70);
		UBoolProperty* Bool = Cast<UBoolProperty>( Property );
		if( ArrayIndex || (Bool && (*(BITFIELD*)Value & Bool->BitMask)) )
			Info |= 0x80;
	}

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FPropertyTag& Tag )
	{
		static TCHAR PrevTag[NAME_SIZE]=TEXT("");
		guard(FPropertyTag<<);
		BYTE SizeByte;
		_WORD SizeWord;
		INT SizeInt;

		// Name.
		guard(TagName);
		Ar << Tag.Name;
		if( Tag.Name == NAME_None )
			return Ar;
		appStrcpy( PrevTag, *Tag.Name );
		unguard;

		// Packed info byte:
		// Bit 0..3 = raw type.
		// Bit 4..6 = serialized size: [1 2 4 12 16 byte word int].
		// Bit 7    = array flag.
		Ar << Tag.Info;
		Tag.Type = Tag.Info & 0x0f;
		if( Tag.Type == NAME_StructProperty )
			Ar << Tag.ItemName;
		switch( Tag.Info & 0x70 )
		{
			case 0x00:
				Tag.Size = 1;
				break;
			case 0x10:
				Tag.Size = 2;
				break;
			case 0x20:
				Tag.Size = 4;
				break;
			case 0x30:
				Tag.Size = 12;
				break;
			case 0x40:
				Tag.Size = 16;
				break;
			case 0x50:
				SizeByte =  Tag.Size;
				Ar       << SizeByte;
				Tag.Size =  SizeByte;
				break;
			case 0x60:
				SizeWord =  Tag.Size;
				Ar       << SizeWord;
				Tag.Size =  SizeWord;
				break;
			case 0x70:
				SizeInt		=  Tag.Size;
				Ar          << SizeInt;
				Tag.Size    =  SizeInt;
				break;
		}
		if( (Tag.Info&0x80) && Tag.Type!=NAME_BoolProperty )
		{
			BYTE B
			=	(Tag.ArrayIndex<=127  ) ? (Tag.ArrayIndex    )
			:	(Tag.ArrayIndex<=16383) ? (Tag.ArrayIndex>>8 )+0x80
			:	                          (Tag.ArrayIndex>>24)+0xC0;
			Ar << B;
			if( (B & 0x80)==0 )
			{
				Tag.ArrayIndex = B;
			}
			else if( (B & 0xC0)==0x80 )
			{
				BYTE C = Tag.ArrayIndex & 255;
				Ar << C;
				Tag.ArrayIndex = ((INT)(B&0x7F)<<8) + ((INT)C);
			}
			else
			{
				BYTE C = Tag.ArrayIndex>>16;
				BYTE D = Tag.ArrayIndex>>8;
				BYTE E = Tag.ArrayIndex;
				Ar << C << D << E;
				Tag.ArrayIndex = ((INT)(B&0x3F)<<24) + ((INT)C<<16) + ((INT)D<<8) + ((INT)E);
			}
		}
		else Tag.ArrayIndex = 0;
		return Ar;
		unguardf(( TEXT("(After %s)"), PrevTag ));
	}

	// Property serializer.
	void SerializeTaggedProperty( FArchive& Ar, UProperty* Property, BYTE* Value, INT MaxReadBytes )
	{
		guard(FPropertyTag::SerializeTaggedProperty);
		if( Property->GetClass()==UBoolProperty::StaticClass() )
		{
			UBoolProperty* Bool = (UBoolProperty*)Property;
			check(Bool->BitMask!=0);
			if( Ar.IsLoading() )				
			{
				if( Info&0x80)	*(BITFIELD*)Value |=  Bool->BitMask;
				else			*(BITFIELD*)Value &= ~Bool->BitMask;
			}
		}
		else
		{
			Property->SerializeItem( Ar, Value, MaxReadBytes );
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	UField implementation.
-----------------------------------------------------------------------------*/

UField::UField( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags, UField* InSuperField )
: UObject				( EC_NativeConstructor, InClass, InName, InPackageName, InFlags )
, SuperField			( InSuperField )
, Next					( NULL )
, HashNext				( NULL )
{}
UField::UField( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags )
: UObject				( EC_StaticConstructor, InName, InPackageName, InFlags )
, Next					( NULL )
, HashNext				( NULL )
{}
UField::UField( UField* InSuperField )
:	SuperField( InSuperField )
{}
UClass* UField::GetOwnerClass()
{
	guardSlow(UField::GetOwnerClass);
	UObject* Obj;
	for( Obj=this; Obj->GetClass()!=UClass::StaticClass(); Obj=Obj->GetOuter() );
	return (UClass*)Obj;
	unguardSlow;
}
void UField::Bind()
{
	guard(UField::Bind);
	unguard;
}
void UField::PostLoad()
{
	guard(UField::PostLoad);
	Super::PostLoad();
	Bind();
	unguard;
}
void UField::Serialize( FArchive& Ar )
{
	guard(UField::Serialize);
	Super::Serialize( Ar );

	Ar << SuperField << Next;
	if( Ar.IsLoading() )
		HashNext = NULL;

	unguardobj;
}
INT UField::GetPropertiesSize()
{
	return 0;
}
UBOOL UField::MergeBools()
{
	return 1;
}
void UField::AddCppProperty( UProperty* Property )
{
	guard(UField::AddCppProperty);
	appErrorf(TEXT("UField::AddCppProperty"));
	unguard;
}
void UField::Register()
{
	guard(UField::Register);
	Super::Register();
	if( SuperField )
		SuperField->ConditionalRegister();
	unguard;
}
IMPLEMENT_CLASS(UField)

/*-----------------------------------------------------------------------------
	UStruct implementation.
-----------------------------------------------------------------------------*/

//
// Constructors.
//
UStruct::UStruct( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags, UStruct* InSuperStruct )
:	UField			( EC_NativeConstructor, UClass::StaticClass(), InName, InPackageName, InFlags, InSuperStruct )
,	ScriptText		( NULL )
,	CppText			( NULL )
,	Children		( NULL )
,	PropertiesSize	( InSize )
,	Script			()
,	TextPos			( 0 )
,	Line			( 0 )
,	RefLink			( NULL )
,	PropertyLink	( NULL )
,	ConfigLink	    ( NULL )
,	ConstructorLink	( NULL )
,	FriendlyName	( /*Uninitialized*/ )
{}
UStruct::UStruct( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags )
:	UField			( EC_StaticConstructor, InName, InPackageName, InFlags )
,	ScriptText		( NULL )
,	CppText			( NULL )
,	Children		( NULL )
,	PropertiesSize	( InSize )
,	Script			()
,	TextPos			( 0 )
,	Line			( 0 )
,	RefLink			( NULL )
,	PropertyLink	( NULL )
,	ConfigLink	    ( NULL )
,	ConstructorLink	( NULL )
,	FriendlyName	( /*Uninitialized*/ )
{}
UStruct::UStruct( UStruct* InSuperStruct )
:	UField( InSuperStruct )
,	PropertiesSize( InSuperStruct ? InSuperStruct->GetPropertiesSize() : 0 )
,	FriendlyName( GetFName() )
,	RefLink( NULL )
{}

//
// Add a property.
//
void UStruct::AddCppProperty( UProperty* Property )
{
	guard(UStruct::AddCppProperty);
	Property->Next = Children;
	Children       = Property;
	unguard;
}

//
// Register.
//
void UStruct::Register()
{
	guard(UStruct::Register);
	Super::Register();

	// Friendly name.
	FriendlyName = GetFName();

	unguard;
}

//
// Link offsets.
//
void UStruct::Link( FArchive& Ar, UBOOL Props )
{
	guard(UStruct::Link);

	// Link the properties.
	guard(LinkProperties);
	if( Props )
	{
		PropertiesSize = 0;
		if( GetInheritanceSuper() )
		{
			Ar.Preload( GetInheritanceSuper() );
			PropertiesSize = Align(GetInheritanceSuper()->GetPropertiesSize(),4);
		}
		UProperty* Prev = NULL;
		for( UField* Field=Children; Field; Field=Field->Next )
		{
			Ar.Preload( Field );
			if( Field->GetOuter()!=this )
				break;
			UProperty* Property = Cast<UProperty>( Field );
			if( Property )
			{
				if( Prev && (Prev->PropertyFlags & CPF_NoExport) && !(Property->PropertyFlags & CPF_NoExport) )
					appThrowf( TEXT("NoExport variables must be defined last: %s"), Property->GetName() );
				Property->Link( Ar, Prev );
				PropertiesSize = Property->Offset + Property->GetSize();
				Prev = Property;
			}
		}
		PropertiesSize = Align(PropertiesSize,4);
	}
	else
	{
		UProperty* Prev = NULL;
		for( UField* Field=Children; Field && Field->GetOuter()==this; Field=Field->Next )
		{
			UProperty* Property = Cast<UProperty>( Field );
			if( Property )
			{
				UBoolProperty*	BoolProperty = Cast<UBoolProperty>(Property);
				INT				SavedOffset = Property->Offset;
				BITFIELD		SavedBitMask = BoolProperty ? BoolProperty->BitMask : 0;

				Property->Link( Ar, Prev );
				Property->Offset = SavedOffset;
				Prev = Property;

				if(Cast<UBoolProperty>(Property))
					Cast<UBoolProperty>(Property)->BitMask = SavedBitMask;
			}
		}
	}
	unguard;

	// Link the references, structs, and arrays for optimized cleanup.
	// Note: Could optimize further by adding UProperty::NeedsDynamicRefCleanup, excluding things like arrays of ints.
	guard(LinkRefs);
	UProperty** RefLinkPtr = (UProperty**)&RefLink;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(this); It; ++It)
	{
		if( Cast<UObjectProperty>(*It) || Cast<UStructProperty>(*It) || Cast<UArrayProperty>(*It) || Cast<UDelegateProperty>(*It) )
		{
			*RefLinkPtr = *It;
			RefLinkPtr=&(*RefLinkPtr)->NextRef;
		}
	}
	*RefLinkPtr = NULL;
	unguard;

	// Link the cleanup.
	guard(LinkCleanup);
	TMap<UProperty*,INT> Map;
	UProperty** PropertyLinkPtr    = &PropertyLink;
	UProperty** ConfigLinkPtr      = &ConfigLink;
	UProperty** ConstructorLinkPtr = &ConstructorLink;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> ItC(this); ItC; ++ItC )
	{
		if( (ItC->PropertyFlags & CPF_Net) && !GIsEditor )
		{
			ItC->RepOwner = *ItC;
			FArchive TempAr;
			INT iCode = ItC->RepOffset;
			ItC->GetOwnerClass()->SerializeExpr( iCode, TempAr );
			Map.Set( *ItC, iCode );
			for( TFieldIterator<UProperty,CLASS_IsAUProperty> ItD(this); *ItD!=*ItC; ++ItD )
			{
				if( ItD->PropertyFlags & CPF_Net )
				{
					INT* iCodePtr = Map.Find( *ItD );
					check(iCodePtr);
					if
					(	iCode-ItC->RepOffset==*iCodePtr-ItD->RepOffset
					&&	appMemcmp(&ItC->GetOwnerClass()->Script(ItC->RepOffset),&ItD->GetOwnerClass()->Script(ItD->RepOffset),iCode-ItC->RepOffset)==0 )
					{
						ItD->RepOwner = ItC->RepOwner;
					}
				}
			}
		}
		if( ItC->PropertyFlags & CPF_NeedCtorLink )
		{
			*ConstructorLinkPtr = *ItC;
			ConstructorLinkPtr  = &(*ConstructorLinkPtr)->ConstructorLinkNext;
		}
		if( ItC->PropertyFlags & CPF_Config )
		{
			*ConfigLinkPtr = *ItC;
			ConfigLinkPtr  = &(*ConfigLinkPtr)->ConfigLinkNext;
		}
		*PropertyLinkPtr = *ItC;
		PropertyLinkPtr  = &(*PropertyLinkPtr)->PropertyLinkNext;
	}
	*PropertyLinkPtr    = NULL;
	*ConfigLinkPtr      = NULL;
	*ConstructorLinkPtr = NULL;
	unguard;

	unguard;
}

//
// Serialize all of the class's data that belongs in a particular
// bin and resides in Data.
//
void UStruct::SerializeBin( FArchive& Ar, BYTE* Data, INT MaxReadBytes )
{
	FName PropertyName(NAME_None);
	INT MaxReadPos = Ar.Tell() + MaxReadBytes;
	INT Index=0;

	//!!OLDVER - before structs were serialized by tagged properties.
	UBOOL DecoHack = Ar.Ver() < 115 && !appStrcmp( GetName(), TEXT("DecorationLayer") );

	guard(UStruct::SerializeBin);
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(this); It; ++It )
	{
		PropertyName = It->GetFName();
		
		//!!OLDVER
		if( DecoHack && (!appStrcmp(*PropertyName, TEXT("LitDirectional")) || !appStrcmp(*PropertyName, TEXT("DisregardTerrainLighting"))) )
		{
			debugf( NAME_Warning, TEXT("DecoHack: skipping property %s"), *PropertyName );
			continue;
		}

		if( It->ShouldSerializeValue(Ar) )
			for( Index=0; Index<It->ArrayDim; Index++ )
			{	
				if( Ar.Ver() >= 118 || !MaxReadBytes || !Ar.IsLoading() || Ar.Tell() < MaxReadPos )
					It->SerializeItem( Ar, Data + It->Offset + Index*It->ElementSize, 0 );
				else
					debugf( NAME_Warning, TEXT("Binary-serialized struct property %s of %s does not exist on disk.  Skipping."), It->GetName(), GetName() );
			}
	}
	unguardf(( TEXT("(%s %s[%i])"), GetFullName(), *PropertyName, Index ));
}
void UStruct::SerializeTaggedProperties( FArchive& Ar, BYTE* Data, UClass* DefaultsClass )
{
	FName PropertyName(NAME_None);
	INT Index=-1;
	guard(UStruct::SerializeTaggedProperties);
	check(Ar.IsLoading() || Ar.IsSaving());

#if 0 //!!MERGE
	TMultiMap<FString,FString>* PropRemap = GConfig->GetSectionPrivate(TEXT("PropRemap"),0,1);
#endif

	// Find defaults.
	BYTE* Defaults      = NULL;
	INT   DefaultsCount = 0;
	if( DefaultsClass )
	{
		Defaults      = &DefaultsClass->Defaults(0);
		DefaultsCount =  DefaultsClass->Defaults.Num();
	}

	// Load/save.
#if VF_HASH_VARIABLES
	UClass* C = Cast<UClass>(this);
#endif
	if( Ar.IsLoading() )
	{
		// Load all stored properties.
		INT Count=0;
		guard(LoadStream);
		while( 1 )
		{
			FPropertyTag Tag;
			Ar << Tag;
			if( Tag.Name == NAME_None )
				break;
			PropertyName = Tag.Name;
#if 0 //!!MERGE
			FString* Rename = PropRemap ? PropRemap->Find( FString::Printf(TEXT("%s.%s"),GetName(),*Tag.Name) ) : NULL;
			if( Rename )
			{
				Tag.Name = FName(**Rename,FNAME_Intrinsic);
				debugf( NAME_Log, TEXT("Renaming property %s of %s to %s"), *PropertyName, GetFullName(), *Tag.Name );
				PropertyName = Tag.Name;
			}
			else
			{
				// try it again, this time without the class name specifier.
				Rename = PropRemap ? PropRemap->Find( *Tag.Name ) : NULL;
				if( Rename )
				{
					Tag.Name = FName(**Rename,FNAME_Intrinsic);
					debugf( NAME_Log, TEXT("Renaming property %s of %s to %s"), *PropertyName, GetFullName(), *Tag.Name );
					PropertyName = Tag.Name;
				}
			}
#endif
			UProperty* Property=NULL;
#if VF_HASH_VARIABLES
			if( C )
				for( UField* Node=C->VfHash[Tag.Name.GetIndex() & (UField::HASH_COUNT-1)]; Node; Node=Node->HashNext )
					if( Node->GetFName()==Tag.Name )
						{Property = Cast<UProperty>(Node); break;}
#else
			for( Property=PropertyLink; Property; Property=Property->PropertyLinkNext )
				if( Property->GetFName()==Tag.Name )
					break;
#endif
			if( !Property )
			{
				debugf( NAME_Warning, TEXT("Property %s of %s not found (%s)"), *Tag.Name, GetFullName(), DefaultsClass ? DefaultsClass->GetFullName() : L"?" );
#if 1 //NEW (mdf)				
				check( !GIsStrict);
#endif
			}
			else if( Cast<UArrayProperty>(Property) && Tag.Type!=NAME_ArrayProperty && Cast<UArrayProperty>(Property)->Inner->GetID()==Tag.Type )
			{
				//oldver: Convert fixed-length arrays into dynamic arrays
				debugf( NAME_Warning, TEXT("Converting in %s of %s to dynamic array."), *Tag.Name, GetName() );
				UProperty* InnerProp = Cast<UArrayProperty>(Property)->Inner;
				FArray* Arr = (FArray*)(Data + Property->Offset);
				if( Arr->Num() <= Tag.ArrayIndex )
					Arr->AddZeroed( InnerProp->ElementSize, 1+Tag.ArrayIndex-Arr->Num() );
				Tag.SerializeTaggedProperty( Ar, InnerProp, (BYTE*)Arr->GetData()+Tag.ArrayIndex * InnerProp->ElementSize, 0 );
				continue;
			}
			else if( Tag.Type==NAME_ByteProperty && Property->GetID()==NAME_FloatProperty )
			{
				//oldver: Convert bytes to floats.
				BYTE B;
				Ar << B;
				*(FLOAT*)(Data + Property->Offset + Tag.ArrayIndex * Property->ElementSize ) = (FLOAT) B;
				continue;
			}
			else if( Tag.Type!=Property->GetID() )
			{
				debugf( NAME_Warning, TEXT("Type mismatch in %s of %s: file %i, class %i"), *Tag.Name, GetName(), Tag.Type, Property->GetID() );
			}
			else if( Tag.ArrayIndex>=Property->ArrayDim )
			{
				debugf( NAME_Warning, TEXT("Array bounds in %s of %s: %i/%i"), *Tag.Name, GetName(), Tag.ArrayIndex, Property->ArrayDim );
			}
			else if( Tag.Type==NAME_StructProperty && Tag.ItemName!=CastChecked<UStructProperty>(Property)->Struct->GetFName() )
			{
				debugf( NAME_Warning, TEXT("Property %s of %s struct type mismatch %s/%s"), *Tag.Name, GetName(), *Tag.ItemName, CastChecked<UStructProperty>(Property)->Struct->GetName() );
			}
			else if( !Property->ShouldSerializeValue(Ar) )
			{
				if( appStricmp(*Tag.Name,TEXT("XLevel"))!=0 )
					debugf( NAME_Warning, TEXT("Property %s of %s is not serializable"), *Tag.Name, GetName() );
			}
			else
			{
				// This property is ok.
				INT StartPos = Ar.Tell();							
				Tag.SerializeTaggedProperty( Ar, Property, Data + Property->Offset + Tag.ArrayIndex*Property->ElementSize, Tag.Size );

				if( Ar.Ver() < 118 )
				{
					// As of Ar.Ver()==118, structs are serialized as tagged properties, so this backwards compatible
					// code only applies to content saved before this.

					// Seek back to the correct location in the file due to a mismatch between the UStructProperty's layout and 
					// what was serialized with UStruct::SerializeBin()
					INT BytesSerialized = Ar.Tell() - StartPos;	
					if( BytesSerialized != Tag.Size )
					{
						// This can happen if a struct has items removed from it.
						debugf( NAME_Warning, TEXT("Tag %s of %s size mismatch.  Serialized %d but expected %d"), *Tag.Name, GetName(), BytesSerialized, Tag.Size );
						Ar.Seek( StartPos + Tag.Size );
					}
				}

//				if( !Cast<UClass>(this) )
//					debugf(TEXT("Serializing property %s: of struct %s"), Property->GetName(), GetName() );

				continue;
			}

			// Skip unknown or bad property.
			if( appStricmp(*Tag.Name,TEXT("XLevel"))!=0 )
				debugf( NAME_Warning, TEXT("Skipping %i bytes of type %i"), Tag.Size, Tag.Type );
			BYTE B;
			for( INT i=0; i<Tag.Size; i++ )
				Ar << B;
		}
		unguardf(( TEXT("(Count %i)"), Count ));
		Count = 0;
	}
	else
	{
		// Save tagged properties.
		guard(SaveStream);
		for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(this); It; ++It )
		{
			if( It->ShouldSerializeValue(Ar) )
			{
				PropertyName = It->GetFName();
				for( Index=0; Index<It->ArrayDim; Index++ )
				{
					INT Offset = It->Offset + Index*It->ElementSize;
					if( (!IsA(UClass::StaticClass())&&!Defaults) || !It->Matches( Data, (Offset+It->ElementSize<=DefaultsCount) ? Defaults : NULL, Index) )
					{
 						FPropertyTag Tag( Ar, *It, Index, Data + Offset );
						Ar << Tag;
						Tag.SerializeTaggedProperty( Ar, *It, Data + Offset, 0 );
					}
				}
			}
		}
		FName Temp(NAME_None);
		Ar << Temp;
		unguard;
	}
	unguardf(( TEXT("(%s[%i])"), *PropertyName, Index ));
}
void UStruct::Destroy()
{
	guard(UStruct::Destroy);
	Script.Empty();
	Super::Destroy();
	unguard;
}

FString UStruct::FunctionMD5()
{
	int i;
	FString MD5;

	for (i=0; i<16; i++)
		MD5 += FString::Printf(TEXT("%02x"), FunctionMD5Digest[i]);	

	return MD5;
}

void UStruct::Serialize( FArchive& Ar )
{
	guard(UStruct::Serialize);
	Super::Serialize( Ar );

	//debugf(TEXT("UStruct::Serialize %s"), GetFullName());

	// Serialize stuff.
	Ar << ScriptText << Children;
	Ar << FriendlyName;
	check(FriendlyName!=NAME_None);

	if( Ar.Ver() >= 120 )
		Ar << CppText;

	// Compiler info.
	Ar << Line << TextPos;

	// Script code.
	INT ScriptSize = Script.Num();
	Ar << ScriptSize;
	if( Ar.IsLoading() )
	{
		Script.Empty();
		Script.Add( ScriptSize );
	}
	INT iCode = 0;
	while( iCode < ScriptSize )
		SerializeExpr( iCode, Ar );
	if( iCode != ScriptSize )
		appErrorf( TEXT("Script serialization mismatch: Got %i, expected %i"), iCode, ScriptSize );

	// Caculate the Code MD5 for this function

	FMD5Context PContext;
	appMD5Init( &PContext );
	appMD5Update( &PContext, (unsigned char*) Script.GetData(), Script.Num() );
	appMD5Final( FunctionMD5Digest, &PContext );

	Ar.ThisContainsCode();
	// Link the properties.
	if( Ar.IsLoading() )
		Link( Ar, 1 );

	unguardobj;
}

//
// Actor reference cleanup.
//
void UStruct::CleanupDestroyed( BYTE* Data )
{
	guard(UStruct::CleanupDestroyed);
	if( GIsEditor )
	{
		// Slow cleanup in editor where optimized structures don't exist.
		for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(this); It; ++It )
			(*It)->CleanupDestroyed(Data+It->Offset);
	}
	else
	{
		// Optimal cleanup during gameplay.
		for( UProperty* Ref=RefLink; Ref; Ref=Ref->NextRef )
			Ref->CleanupDestroyed(Data+Ref->Offset);
	}
	unguardobj;
}

#if 1 //NEW (mdf) tbd: object refs validation
UBOOL UStruct::ValidateRefs( BYTE* Data )
{
	guard(UStruct::ValidateRefs);
	
	UBOOL bResult = true;
	for( UProperty* Ref=RefLink; Ref; Ref=Ref->NextRef )
		if( !Ref->ValidateRefs( Data+Ref->Offset) )
			bResult = false;
	return bResult;
	
	unguardobj;		
}
#endif

IMPLEMENT_CLASS(UStruct);

/*-----------------------------------------------------------------------------
	UState.
-----------------------------------------------------------------------------*/

UState::UState( UState* InSuperState )
: UStruct( InSuperState )
{}
UState::UState( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags, UState* InSuperState )
:	UStruct			( EC_NativeConstructor, InSize, InName, InPackageName, InFlags, InSuperState )
,	ProbeMask		( 0 )
,	IgnoreMask		( 0 )
,	StateFlags		( 0 )
,	LabelTableOffset( 0 )
{}
UState::UState( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, DWORD InFlags )
:	UStruct			( EC_StaticConstructor, InSize, InName, InPackageName, InFlags )
,	ProbeMask		( 0 )
,	IgnoreMask		( 0 )
,	StateFlags		( 0 )
,	LabelTableOffset( 0 )
{}
void UState::Destroy()
{
	guard(UState::Destroy);
	Super::Destroy();
	unguard;
}
void UState::Serialize( FArchive& Ar )
{
	guard(UState::Serialize);
	Super::Serialize( Ar );

	// Class/State-specific union info.
	Ar << ProbeMask << IgnoreMask;
	Ar << LabelTableOffset << StateFlags;

	unguard;
}
void UState::Link( FArchive& Ar, UBOOL Props )
{
	guard(UState::Link);
	Super::Link( Ar, Props );

	// Initialize hash.
	if( GetSuperState() )
		appMemcpy( VfHash, GetSuperState()->VfHash, sizeof(VfHash) );
	else
		appMemzero( VfHash, sizeof(VfHash) );

	// Add all stuff at this node to the hash.
#if VF_HASH_VARIABLES
	for( TFieldIterator<UField> It(this); It && It->GetOuter()==this; ++It )
#else
	for( TFieldIterator<UStruct> It(this); It && It->GetOuter()==this; ++It )
#endif
	{
		INT iHash          = It->GetFName().GetIndex() & (UField::HASH_COUNT-1);
		It->HashNext       = VfHash[iHash];
		VfHash[iHash]      = *It;
	}

	unguard;
}
IMPLEMENT_CLASS(UState);

/*-----------------------------------------------------------------------------
	UClass implementation.
-----------------------------------------------------------------------------*/

//
// Register the native class.
//
void UClass::Register()
{
	guard(UClass::Register);
	Super::Register();

	// Get stashed registration info.
	const TCHAR* InClassConfigName = *(TCHAR**)&ClassConfigName;
	ClassConfigName = InClassConfigName;

	// Init default object.
	Defaults.Empty( GetPropertiesSize() );
	Defaults.Add( GetPropertiesSize() );
	GetDefaultObject()->InitClassDefaultObject( this );

	// Perform static construction.
	if( !GetSuperClass() || GetSuperClass()->ClassStaticConstructor!=ClassStaticConstructor )
		(GetDefaultObject()->*ClassStaticConstructor)();

	// Propagate inhereted flags.
	if( SuperField )
		ClassFlags |= (GetSuperClass()->ClassFlags & CLASS_Inherit);

	// Link the cleanup.
	FArchive ArDummy;
	Link( ArDummy, 0 );

	// Load defaults.
	GetDefaultObject()->LoadConfig();
	GetDefaultObject()->LoadLocalized();

	unguardf(( TEXT("(%s)"), GetName() ));
}

//
// Find the class's native constructor.
//
void UClass::Bind()
{
	guard(UClass::Bind);
	UStruct::Bind();
	check(GIsEditor || GetSuperClass() || this==UObject::StaticClass());
	if( !ClassConstructor && (GetFlags() & RF_Native) )
	{
		// Find the native implementation.
		TCHAR ProcName[256];
		appSprintf( ProcName, TEXT("autoclass%s"), GetNameCPP() );

		// Find export from the DLL.
		UPackage* ClassPackage = GetOuterUPackage();
		UClass** ClassPtr = (UClass**)ClassPackage->GetDllExport( ProcName, 0 );
		if( ClassPtr )
		{
			check(*ClassPtr);
			check(*ClassPtr==this);
			ClassConstructor = (*ClassPtr)->ClassConstructor;
		}
		else if( !GIsEditor )
		{
			appErrorf( TEXT("Can't bind to native class %s"), GetPathName() );
		}
	}
	if( !ClassConstructor && GetSuperClass() )
	{
		// Chase down constructor in parent class.
		GetSuperClass()->Bind();
		ClassConstructor = GetSuperClass()->ClassConstructor;
	}
	check(GIsEditor || ClassConstructor);
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UClass UObject implementation.
-----------------------------------------------------------------------------*/

static INT Compare( UField* A, UField* B )
{
	if( !A->GetLinker() || !B->GetLinker() )
		return 0;
#if ENGINE_VERSION<230
	INT Diff = CompareGuids( &A->GetLinker()->Summary.Guid, &B->GetLinker()->Summary.Guid );
	if( Diff )
		return Diff;
#endif
	return A->GetLinkerIndex() - B->GetLinkerIndex();
}

void UClass::Destroy()
{
	guard(UClass::Destroy);

	// Empty arrays.
	//warning: Must be emptied explicitly in order for intrinsic classes
	// to not show memory leakage on exit.
	NetFields.Empty();
	Dependencies.Empty();
	PackageImports.Empty();
	ExitProperties( &Defaults(0), this );
	Defaults.Empty();
	DefaultPropText=TEXT("");
	if( NetPropertyMap )
	{
		delete NetPropertyMap;
		NetPropertyMap = NULL;
	}
	Super::Destroy();
	unguard;
}
void UClass::PostLoad()
{
	guard(UClass::PostLoad);
	check(ClassWithin);
	Super::PostLoad();

	// Postload super.
	if( GetSuperClass() )
		GetSuperClass()->ConditionalPostLoad();

	unguardobj;
}
void UClass::Link( FArchive& Ar, UBOOL Props )
{
	guard(UClass::Link);
	Super::Link( Ar, Props );
	if( !GIsEditor )
	{
		NetFields.Empty();
#if ENGINE_VERSION<230
		ClassReps.Empty();
		for( TFieldIterator<UField> It(this); It; ++It )
#else
		ClassReps = SuperField ? GetSuperClass()->ClassReps : TArray<FRepRecord>();
		for( TFieldIterator<UField> It(this); It && It->GetOwnerClass()==this; ++It )
#endif
		{
#if NEWNET // no old var-rep
			UFunction* F;
			if( (F=Cast<UFunction>(*It))!=NULL )
#else
			UProperty* P;
			UFunction* F;
			if( (P=Cast<UProperty>(*It))!=NULL )
			{
				if( P->PropertyFlags&CPF_Net )
				{
					NetFields.AddItem( *It );
					if( P->GetOuter()==this )
					{
						P->RepIndex = ClassReps.Num();
						for( INT i=0; i<P->ArrayDim; i++ )
							new(ClassReps)FRepRecord(P,i);
					}
				}
			}
			else if( (F=Cast<UFunction>(*It))!=NULL )
#endif
			{
				if( (F->FunctionFlags&FUNC_Net) && !F->GetSuperFunction() )
					NetFields.AddItem( *It );
			}
		}
#if NEWNET // add in our NetProperties as well
		//mjl-tbd: speed up and optimize the storage of NetPropertyMap
		NetPropertyMap = NULL;
		if( SuperField && ( GetSuperClass()->NetPropertyMap || GetSuperClass() == UObject::StaticClass() ) )
			NetPropertyMap = new THashTable<FName,FRepRecord,6>( GetSuperClass()->NetPropertyMap );
		for( INT i=0; i<NetProperties.Num(); i++ )
		{
			UProperty* Prop = NetProperties(i);
			if( !NetPropertyMap->Find( Prop->GetFName() ) )
			{
				NetPropertyMap->Set( Prop->GetFName(), FRepRecord( Prop, ClassReps.Num() ) );
				NetFields.AddItem(Prop);
				for( INT i=0; i<Prop->ArrayDim; i++ )
					new(ClassReps)FRepRecord(Prop,i);
			}
		}
		NetProperties.Empty();
#endif

		NetFields.Shrink();
		Sort( &NetFields(0), NetFields.Num() );
	}
	unguard;
}
void UClass::Serialize( FArchive& Ar )
{
	guard(UClass::Serialize);

#if NEWNET
	//needs to go before Super::Serialize, so that when it calls Link, NetProperties will be available
	Ar << NetProperties;
#endif
	Super::Serialize( Ar );

	// Variables.
	if( Ar.Ver() <= 61 )//oldver
	{
		INT OldClassRecordSize=0;
		Ar << OldClassRecordSize; 
		SetFlags( RF_Public | RF_Standalone );
	}
	Ar << ClassFlags << ClassGuid;
	Ar << Dependencies << PackageImports;
	if( Ar.Ver()>=62 )
		Ar << ClassWithin << ClassConfigName;
	else
		ClassConfigName = FName(TEXT("System"));

	if( Ar.Ver() >= 99 )
		Ar << HideCategories;

	// Defaults.
	if( Ar.IsLoading() )
	{
		check(GetPropertiesSize()>=sizeof(UObject));
		check(!GetSuperClass() || !(GetSuperClass()->GetFlags()&RF_NeedLoad));
		Defaults.Empty( GetPropertiesSize() );
		Defaults.Add( GetPropertiesSize() );
		GetDefaultObject()->InitClassDefaultObject( this );
		SerializeTaggedProperties( Ar, &Defaults(0), GetSuperClass() );
		GetDefaultObject()->LoadConfig();
		GetDefaultObject()->LoadLocalized();
		ClassUnique = 0;
		if( Ar.Ver()<=61 )//oldver
			ClassWithin = UObject::StaticClass();
	}
	else if( Ar.IsSaving() )
	{
		check(Defaults.Num()==GetPropertiesSize());
		SerializeTaggedProperties( Ar, &Defaults(0), GetSuperClass() );
	}
	else
	{
		check(Defaults.Num()==GetPropertiesSize());
		Defaults.CountBytes( Ar );
		SerializeBin( Ar, &Defaults(0), 0 );
	}
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UClass constructors.
-----------------------------------------------------------------------------*/

//
// Internal constructor.
//
UClass::UClass()
:	ClassWithin( UObject::StaticClass() )
{}

//
// Create a new UClass given its superclass.
//
UClass::UClass( UClass* InBaseClass )
:	UState( InBaseClass )
,	ClassWithin( UObject::StaticClass() )
{
	guard(UClass::UClass);
	if( GetSuperClass() )
	{
		ClassWithin = GetSuperClass()->ClassWithin;
		Defaults = GetSuperClass()->Defaults;
		Bind();		
	}
	unguardobj;
}

//
// UClass autoregistry constructor.
//warning: Called at DLL init time.
//
UClass::UClass
(
	ENativeConstructor,
	DWORD			InSize,
	DWORD			InClassFlags,
	UClass*			InSuperClass,
	UClass*			InWithinClass,
	FGuid			InGuid,
	const TCHAR*	InNameStr,
	const TCHAR*    InPackageName,
	const TCHAR*    InConfigName,
	DWORD			InFlags,
	void			(*InClassConstructor)(void*),
	void			(UObject::*InClassStaticConstructor)()
)
:	UState					( EC_NativeConstructor, InSize, InNameStr, InPackageName, InFlags, InSuperClass!=this ? InSuperClass : NULL )
,	ClassFlags				( InClassFlags | CLASS_Parsed | CLASS_Compiled )
,	ClassUnique				( 0 )
,	ClassGuid				( InGuid )
,	ClassWithin				( InWithinClass )
,	ClassConfigName			()
,	Dependencies			()
,	PackageImports			()
,	Defaults				()
,	NetFields				()
,	ClassConstructor		( InClassConstructor )
,	ClassStaticConstructor	( InClassStaticConstructor )
{
	*(const TCHAR**)&ClassConfigName = InConfigName;
}

// Called when statically linked.
UClass::UClass
(
	EStaticConstructor,
	DWORD			InSize,
	DWORD			InClassFlags,
	FGuid			InGuid,
	const TCHAR*	InNameStr,
	const TCHAR*    InPackageName,
	const TCHAR*    InConfigName,
	DWORD			InFlags,
	void			(*InClassConstructor)(void*),
	void			(UObject::*InClassStaticConstructor)()
)
:	UState					( EC_StaticConstructor, InSize, InNameStr, InPackageName, InFlags )
,	ClassFlags				( InClassFlags | CLASS_Parsed | CLASS_Compiled )
,	ClassUnique				( 0 )
,	ClassGuid				( InGuid )
,	ClassConfigName			()
,	Dependencies			()
,	PackageImports			()
,	Defaults				()
,	NetFields				()
,	ClassConstructor		( InClassConstructor )
,	ClassStaticConstructor	( InClassStaticConstructor )
{
	*(const TCHAR**)&ClassConfigName = InConfigName;
}

IMPLEMENT_CLASS(UClass);

/*-----------------------------------------------------------------------------
	FDependency.
-----------------------------------------------------------------------------*/

//
// FDepdendency inlines.
//
FDependency::FDependency()
{}
FDependency::FDependency( UClass* InClass, UBOOL InDeep )
:	Class( InClass )
,	Deep( InDeep )
,	ScriptTextCRC( Class ? Class->GetScriptTextCRC() : 0 )
{}
UBOOL FDependency::IsUpToDate()
{
	guard(FDependency::IsUpToDate);
	check(Class!=NULL);
	return Class->GetScriptTextCRC()==ScriptTextCRC;
	unguard;
}
CORE_API FArchive& operator<<( FArchive& Ar, FDependency& Dep )
{
	return Ar << Dep.Class << Dep.Deep << Dep.ScriptTextCRC;
}

/*-----------------------------------------------------------------------------
	FLabelEntry.
-----------------------------------------------------------------------------*/

FLabelEntry::FLabelEntry( FName InName, INT iInCode )
:	Name	(InName)
,	iCode	(iInCode)
{}
CORE_API FArchive& operator<<( FArchive& Ar, FLabelEntry &Label )
{
	Ar << Label.Name;
	Ar << Label.iCode;
	return Ar;
}

/*-----------------------------------------------------------------------------
	UStruct implementation.
-----------------------------------------------------------------------------*/

#if SERIAL_POINTER_INDEX
void *GSerializedPointers[MAX_SERIALIZED_POINTERS];
INT GTotalSerializedPointers = 0;

// !!! FIXME: This has GOT to be a mad performance hit.  --ryan.
INT SerialPointerIndex(void *ptr)
{
    for (INT i = 0; i < GTotalSerializedPointers; i++)
    {
        if (GSerializedPointers[i] == (void *) ptr)
            return(i);
    }

    check(GTotalSerializedPointers < MAX_SERIALIZED_POINTERS);
    GSerializedPointers[GTotalSerializedPointers] = ptr;
    //printf("Added new serialized pointer: number (%d) at (%p).\n", GTotalSerializedPointers, ptr);
    return(GTotalSerializedPointers++);
}
#endif


//
// Serialize an expression to an archive.
// Returns expression token.
//
EExprToken UStruct::SerializeExpr( INT& iCode, FArchive& Ar )
{
	EExprToken Expr=(EExprToken)0;
	guard(SerializeExpr);
	#define XFER(T) {Ar << *(T*)&Script(iCode); iCode += sizeof(T); }

#if !SERIAL_POINTER_INDEX
    #define XFERPTR(T) XFER(T)
#else
    #define XFERPTR(T) \
    { \
        T x = NULL; \
        if (!Ar.IsLoading()) \
            x = (T) GSerializedPointers[*(INT*)&Script(iCode)]; \
        Ar << x; \
        *(INT*)&Script(iCode) = SerialPointerIndex(x); \
        iCode += sizeof (INT); \
    }
#endif

	// To mantain compatability between debug and non-debug clases
	#define HANDLE_OPTIONAL_DEBUG_INFO() \
    if (iCode < Script.Num()) \
    { \
	    int RemPos = Ar.Tell(); \
	    int OldiCode = iCode;	\
	    XFER(BYTE); \
	    int NextCode = Script(iCode-1); \
	    int GVERSION = -1;	\
	    if ( NextCode == EX_DebugInfo ) \
	    {	\
		    XFER(INT); \
		    GVERSION = *(INT*)&Script(iCode-sizeof(INT));	\
	    }	\
	    iCode = OldiCode;	\
	    Ar.Seek( RemPos );	\
	    if ( GVERSION == 100 )	\
		    SerializeExpr( iCode, Ar );	\
    } \

	// Get expr token.
	XFER(BYTE);
	Expr = (EExprToken)Script(iCode-1);
	if( Expr >= EX_FirstNative )
	{
		// Native final function with id 1-127.
		while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms );
		HANDLE_OPTIONAL_DEBUG_INFO();
	}
	else if( Expr >= EX_ExtendedNative )
	{
		// Native final function with id 256-16383.
		XFER(BYTE);
		while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms );
		HANDLE_OPTIONAL_DEBUG_INFO();
	}
	else switch( Expr )
	{
		case EX_PrimitiveCast:
		{
			// A type conversion.
			XFER(BYTE); //which kind of conversion
			SerializeExpr( iCode, Ar );
			break;
		}
		case EX_Let:
		case EX_LetBool:
		case EX_LetDelegate:
		{
			SerializeExpr( iCode, Ar ); // Variable expr.
			SerializeExpr( iCode, Ar ); // Assignment expr.
			break;
		}
		case EX_Jump:
		{
			XFER(_WORD); // Code offset.
			break;
		}
		case EX_LocalVariable:
		case EX_InstanceVariable:
		case EX_DefaultVariable:
		{
			XFERPTR(UProperty*);
			break;
		}
		case EX_DebugInfo:
		{
			XFER(INT);	// Version
			XFER(INT);	// Line number
			XFER(INT);	// Character pos
			do XFER(BYTE) while( Script(iCode-1) );	// String identifier
			break;
		}
		case EX_BoolVariable:
		case EX_Nothing:
		case EX_EndFunctionParms:
		case EX_IntZero:
		case EX_IntOne:
		case EX_True:
		case EX_False:
		case EX_NoObject:
		case EX_Self:
		case EX_IteratorPop:
		case EX_Stop:
		case EX_IteratorNext:
		{
			break;
		}
		case EX_EatString:
		{
			SerializeExpr( iCode, Ar ); // String expression.
			break;
		}
		case EX_Return:
		{
			SerializeExpr( iCode, Ar ); // Return expression.
			break;
		}
		case EX_FinalFunction:
		{
			XFERPTR(UStruct*); // Stack node.
			while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms ); // Parms.
			HANDLE_OPTIONAL_DEBUG_INFO();
			break;
		}
		case EX_VirtualFunction:
		case EX_GlobalFunction:
		{
			XFER(FName); // Virtual function name.
			while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms ); // Parms.
			HANDLE_OPTIONAL_DEBUG_INFO();
			break;
		}
		case EX_DelegateFunction:
		{
			XFERPTR(UProperty*);	// Delegate property
			XFER(FName);		// Delegate function name (in case the delegate is NULL)
			break;
		}
		case EX_NativeParm:
		{
			XFERPTR(UProperty*);
			break;
		}
		case EX_ClassContext:
		case EX_Context:
		{
			SerializeExpr( iCode, Ar ); // Object expression.
			XFER(_WORD); // Code offset for NULL expressions.
			XFER(BYTE); // Zero-fill size if skipped.
			SerializeExpr( iCode, Ar ); // Context expression.
			break;
		}
		case EX_ArrayElement:
		case EX_DynArrayElement:
		{
			SerializeExpr( iCode, Ar ); // Index expression.
			SerializeExpr( iCode, Ar ); // Base expression.
			break;
		}
		case EX_DynArrayLength:
		{
			SerializeExpr( iCode, Ar ); // Base expression.
			break;
		}
		case EX_DynArrayInsert:
		case EX_DynArrayRemove:
		{
			SerializeExpr( iCode, Ar ); // Base expression
			SerializeExpr( iCode, Ar ); // Index
			SerializeExpr( iCode, Ar ); // Count
			break;
 		}
		case EX_New:
		{
			SerializeExpr( iCode, Ar ); // Parent expression.
			SerializeExpr( iCode, Ar ); // Name expression.
			SerializeExpr( iCode, Ar ); // Flags expression.
			SerializeExpr( iCode, Ar ); // Class expression.
			break;
		}
		case EX_IntConst:
		{
			XFER(INT);
			break;
		}
		case EX_FloatConst:
		{
			XFER(FLOAT);
			break;
		}
		case EX_StringConst:
		{
			do XFER(BYTE) while( Script(iCode-1) );
			break;
		}
		case EX_UnicodeStringConst:
		{
			do XFER(_WORD) while( Script(iCode-1) );
			break;
		}
		case EX_ObjectConst:
		{
			XFERPTR(UObject*);
			break;
		}
		case EX_NameConst:
		{
			XFER(FName);
			break;
		}
		case EX_RotationConst:
		{
			XFER(INT); XFER(INT); XFER(INT);
			break;
		}
		case EX_RangeConst:
		{
			XFER(FLOAT); XFER(FLOAT);
			break;
		}
		case EX_VectorConst:
		{
			XFER(FLOAT); XFER(FLOAT); XFER(FLOAT);
			break;
		}
		case EX_ByteConst:
		case EX_IntConstByte:
		{
			XFER(BYTE);
			break;
		}
		case EX_MetaCast:
		{
			XFERPTR(UClass*);
			SerializeExpr( iCode, Ar );
			break;
		}
		case EX_DynamicCast:
		{
			XFERPTR(UClass*);
			SerializeExpr( iCode, Ar );
			break;
		}
		case EX_JumpIfNot:
		{
			XFER(_WORD); // Code offset.
			SerializeExpr( iCode, Ar ); // Boolean expr.
			break;
		}
		case EX_Iterator:
		{
			SerializeExpr( iCode, Ar ); // Iterator expr.
			XFER(_WORD); // Code offset.
			break;
		}
		case EX_Switch:
		{
			XFER(BYTE); // Value size.
			SerializeExpr( iCode, Ar ); // Switch expr.
			break;
		}
		case EX_Assert:
		{
			XFER(_WORD); // Line number.
			SerializeExpr( iCode, Ar ); // Assert expr.
			break;
		}
#if 1 //NEW: CDH... line numbers
		case EX_LineNumber:
		{
			XFER(_WORD); // Line number.
			SerializeExpr( iCode, Ar );
			break;
		}
#endif // ...CDH line numbers
		case EX_Case:
		{
			_WORD W;
//			_WORD* W=(_WORD*)&Script(iCode);
			XFER(_WORD); // Code offset.
			appMemcpy(&W, &Script(iCode-sizeof(_WORD)), sizeof(_WORD));
			if( W != MAXWORD )
				SerializeExpr( iCode, Ar ); // Boolean expr.
			break;
		}
		case EX_LabelTable:
		{
			check((iCode&3)==0);
			for( ; ; )
			{
				FLabelEntry* E = (FLabelEntry*)&Script(iCode);
				XFER(FLabelEntry);
				if( E->Name == NAME_None )
					break;
			}
			break;
		}
		case EX_GotoLabel:
		{
			SerializeExpr( iCode, Ar ); // Label name expr.
			break;
		}
		case EX_Skip:
		{
			XFER(_WORD); // Skip size.
			SerializeExpr( iCode, Ar ); // Expression to possibly skip.
			break;
		}
		case EX_StructCmpEq:
		case EX_StructCmpNe:
		{
			XFERPTR(UStruct*); // Struct.
			SerializeExpr( iCode, Ar ); // Left expr.
			SerializeExpr( iCode, Ar ); // Right expr.
			break;
		}
		case EX_StructMember:
		{
			XFERPTR(UProperty*); // Property.
			SerializeExpr( iCode, Ar ); // Inner expr.
			break;
		}
		case EX_DelegateProperty:
		{
			XFER(FName);	// Name of function we're assigning to the delegate.
			break;
		}
		default:
		{
			// This should never occur.
			appErrorf( TEXT("Bad expr token %02x -- check for missing .u files"), Expr );
			break;
		}
	}
	return Expr;
	#undef XFER
	#undef XFERPTR
	unguardf(( TEXT("(%02X)"), Expr ));
}

void UStruct::PostLoad()
{
	guard(UStruct::PostLoad);
	Super::PostLoad();
	unguard;
}

/*-----------------------------------------------------------------------------
	UFunction.
-----------------------------------------------------------------------------*/

UFunction::UFunction( UFunction* InSuperFunction )
: UStruct( InSuperFunction )
{}
void UFunction::Serialize( FArchive& Ar )
{
	guard(UFunction::Serialize);
	Super::Serialize( Ar );

	// Function info.
	if( Ar.Ver()<=63 )
		Ar << ParmsSize;//oldver
	Ar << iNative;
	if( Ar.Ver()<=63 )
		Ar << NumParms;//oldver
	Ar << OperPrecedence;
	if( Ar.Ver()<=63 )
		Ar << ReturnValueOffset;//oldver
	Ar << FunctionFlags;

	// Replication info.
	if( FunctionFlags & FUNC_Net )
		Ar << RepOffset;

	// Precomputation.
	if( Ar.IsLoading() )
	{
		NumParms          = 0;
		ParmsSize         = 0;
		ReturnValueOffset = MAXWORD;
		for( UProperty* Property=Cast<UProperty>(Children); Property && (Property->PropertyFlags & CPF_Parm); Property=Cast<UProperty>(Property->Next) )
		{
			NumParms++;
			ParmsSize = Property->Offset + Property->GetSize();
			if( Property->PropertyFlags & CPF_ReturnParm )
				ReturnValueOffset = Property->Offset;
		}
	}

	unguard;
}
void UFunction::PostLoad()
{
	guard(UFunction::PostLoad);
	Super::PostLoad();
	unguard;
}
UProperty* UFunction::GetReturnProperty()
{
	guard(UFunction::GetReturnProperty);
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(this); It && (It->PropertyFlags & CPF_Parm); ++It )
		if( It->PropertyFlags & CPF_ReturnParm )
			return *It;
	return NULL;
	unguard;
}
void UFunction::Bind()
{
	guard(UFunction::Bind);
	if( !(FunctionFlags & FUNC_Native) )
	{
		// Use UnrealScript processing function.
		check(iNative==0);
		Func = &UObject::ProcessInternal;
	}
	else if( iNative != 0 )
	{
		// Find hardcoded native.
		check(iNative<EX_Max);
		check(GNatives[iNative]!=0);
		Func = GNatives[iNative];
	}
	else
	{
		// Find dynamic native.
		TCHAR Proc[256];
		appSprintf( Proc, TEXT("int%sexec%s"), GetOwnerClass()->GetNameCPP(), GetName() );
		UPackage* ClassPackage = GetOwnerClass()->GetOuterUPackage();
		Native* Ptr = (Native*)ClassPackage->GetDllExport( Proc, 1 );
		if( Ptr )
			Func = *Ptr;
	}
	unguard;
}
void UFunction::Link( FArchive& Ar, UBOOL Props )
{
	guard(UFunction::Link);
	Super::Link( Ar, Props );
	unguard;
}
IMPLEMENT_CLASS(UFunction);

/*-----------------------------------------------------------------------------
	UConst.
-----------------------------------------------------------------------------*/

UConst::UConst( UConst* InSuperConst, const TCHAR* InValue )
:	UField( InSuperConst )
,	Value( InValue )
{}
void UConst::Serialize( FArchive& Ar )
{
	guard(UConst::Serialize);
	Super::Serialize( Ar );
	Ar << Value;
	unguard;
}
IMPLEMENT_CLASS(UConst);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

