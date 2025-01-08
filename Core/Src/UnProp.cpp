/*=============================================================================
	UnClsPrp.cpp: FProperty implementation
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

//!!fix hardcoded lengths
/*-----------------------------------------------------------------------------
	Helpers.
-----------------------------------------------------------------------------*/

//
// Parse a hex digit.
//
static INT HexDigit( TCHAR c )
{
	if( c>='0' && c<='9' )
		return c - '0';
	else if( c>='a' && c<='f' )
		return c + 10 - 'a';
	else if( c>='A' && c<='F' )
		return c + 10 - 'A';
	else
		return 0;
}

//
// Parse a token.
//
const TCHAR* ReadToken( const TCHAR* Buffer, TCHAR* String, INT MaxLength, UBOOL DottedNames=0 )
{
	guard(ReadToken);
	INT Count=0;
	if( *Buffer == 0x22 )
	{
		// Get quoted string.
		Buffer++;
		while( *Buffer && *Buffer!=0x22 && *Buffer!=13 && *Buffer!=10 && Count<MaxLength-1 )
		{
			if( *Buffer != '\\' )
			{
				String[Count++] = *Buffer++;
			}
			else if( *++Buffer=='\\' )
			{
				String[Count++] = '\\';
				Buffer++;
			}
			else
			{
				String[Count++] = HexDigit(Buffer[0])*16 + HexDigit(Buffer[1]);
				Buffer += 2;
			}
		}
		if( Count==MaxLength-1 )
		{
			debugf( NAME_Warning, TEXT("ReadToken: Quoted string too long") );
			return NULL;
		}
		if( *Buffer++!=0x22 )
		{
			GWarn->Logf( NAME_Warning, TEXT("ReadToken: Bad quoted string") );
			return NULL;
		}
	}
	else if( appIsAlnum( *Buffer ) )
	{
		// Get identifier.
		while
		(	(appIsAlnum(*Buffer) || *Buffer=='_' || *Buffer=='-' || (DottedNames && *Buffer=='.' ))
		&&	Count<MaxLength-1 )
			String[Count++] = *Buffer++;
		if( Count==MaxLength-1 )
		{
			debugf( NAME_Warning, TEXT("ReadToken: Alphanumeric overflow") );
			return NULL;
		}
	}
	else
	{
		// Get just one.
		String[Count++] = *Buffer;
	}
	String[Count] = 0;
	return Buffer;
	unguard;
}

/*-----------------------------------------------------------------------------
	UProperty implementation.
-----------------------------------------------------------------------------*/

//
// Constructors.
//
UProperty::UProperty()
:	UField( NULL )
,	ArrayDim( 1 )
,	NextRef( NULL )
{}
UProperty::UProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
:	UField( NULL )
,	ArrayDim( 1 )
,	PropertyFlags( InFlags )
,	Category( InCategory )
,	Offset( InOffset )
,	NextRef( NULL )
{
	GetOuterUField()->AddCppProperty( this );
}
void UProperty::CleanupDestroyed( BYTE* Data ) const {}
UBOOL UProperty::ValidateRefs( BYTE* Data ) const { return true; } //NEW (mdf) tbd: object refs validation

//
// Serializer.
//
void UProperty::Serialize( FArchive& Ar )
{
	guard(UProperty::Serialize);
	Super::Serialize( Ar );

	// Archive the basic info.
	Ar << ArrayDim << PropertyFlags << Category;
	if( PropertyFlags & CPF_Net )
		Ar << RepOffset;
	if( Ar.IsLoading() )
	{
		Offset = 0;
		ConstructorLinkNext = NULL;
	}
	unguardobj;
}

//
// Export this class property to an output
// device as a C++ header file.
//
void UProperty::ExportCpp( FOutputDevice& Out, UBOOL IsLocal, UBOOL IsParm, UBOOL IsEvent, UBOOL IsStruct ) const
{
	guard(UProperty::ExportCpp)
	TCHAR ArrayStr[80] = TEXT("");
	if
	(	IsParm
	&&	IsA(UStrProperty::StaticClass())
	&&	!(PropertyFlags & CPF_OutParm) )
		Out.Log( TEXT("const ") );
	ExportCppItem( Out, IsEvent || IsStruct );
	if( ArrayDim != 1 )
		appSprintf( ArrayStr, TEXT("[%i]"), ArrayDim );
	if( IsA(UBoolProperty::StaticClass()) )
	{
		if( ArrayDim==1 && !IsLocal && !IsParm )
			Out.Logf( TEXT(" %s%s:1"), GetName(), ArrayStr );
		else if( IsParm && (PropertyFlags & CPF_OutParm) )
			Out.Logf( TEXT("& %s%s"), GetName(), ArrayStr );
		else
			Out.Logf( TEXT(" %s%s"), GetName(), ArrayStr );
	}
	else if( IsA(UStrProperty::StaticClass()) )
	{
		if( IsParm && ArrayDim>1 )
			Out.Logf( TEXT("* %s"), GetName() );
		else if( IsParm )
			Out.Logf( TEXT("& %s"), GetName() );
		else if( IsLocal )
			Out.Logf( TEXT(" %s"), GetName() );
		else if( IsStruct )
			Out.Logf( TEXT(" %s%s"), GetName(), ArrayStr );
		else
			Out.Logf( TEXT("NoInit %s%s"), GetName(), ArrayStr );
	}
	else
	{
		if( IsParm && ArrayDim>1 )
			Out.Logf( TEXT("* %s"), GetName() );
		else if( IsParm && (PropertyFlags & CPF_OutParm) )
			Out.Logf( TEXT("& %s%s"), GetName(), ArrayStr );
		else
			Out.Logf( TEXT(" %s%s"), GetName(), ArrayStr );
	}
	unguardobj;
}

//
// Export the contents of a property.
//
UBOOL UProperty::ExportText
(
	INT		Index,
	TCHAR*	ValueStr,
	BYTE*	Data,
	BYTE*	Delta,
	INT		PortFlags
) const
{
	guard(UProperty::ExportText);
	ValueStr[0]=0;
	if( Data==Delta || !Matches(Data,Delta,Index) )
	{
		ExportTextItem
		(
			ValueStr,
			Data + Offset + Index * ElementSize,
			Delta ? (Delta + Offset + Index * ElementSize) : NULL,
			PortFlags
		);
		return 1;
	}
	else return 0;
	unguardobj;
}

//
// Copy a unique instance of a value.
//
void UProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UProperty::CopySingleValue);
	appMemcpy( Dest, Src, ElementSize );
	unguardobjSlow;
}

//
// Destroy a value.
//
void UProperty::DestroyValue( void* Dest ) const
{}

//
// Net serialization.
//
UBOOL UProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UProperty::NetSerializeItem);
	SerializeItem( Ar, Data, 0 );
	return 1;
	unguardobjSlow;
}

//
// Return whether the property should be exported.
//
UBOOL UProperty::Port() const
{
	return 
	(	GetSize()
	&&	(Category!=NAME_None || !(PropertyFlags & (CPF_Transient | CPF_Native)))
	&&	GetFName()!=NAME_Class );
}

//
// Return type id for encoding properties in .u files.
//
BYTE UProperty::GetID() const
{
	return GetClass()->GetFName().GetIndex();
}

//
// Copy a complete value.
//
void UProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UProperty::CopyCompleteValue);
	for( INT i=0; i<ArrayDim; i++ )
		CopySingleValue( (BYTE*)Dest+i*ElementSize, (BYTE*)Src+i*ElementSize, SuperObject );
	unguardobjSlow;
}

//
// Link property loaded from file.
//
void UProperty::Link( FArchive& Ar, UProperty* Prev )
{}

IMPLEMENT_CLASS(UProperty);

/*-----------------------------------------------------------------------------
	UByteProperty.
-----------------------------------------------------------------------------*/

void UByteProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UByteProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize = sizeof(BYTE);
	Offset      = Align( GetOuterUField()->GetPropertiesSize(), sizeof(BYTE) );
	unguardobj;
}
void UByteProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UByteProperty::CopySingleValue);
	*(BYTE*)Dest = *(BYTE*)Src;
	unguardSlow;
}
void UByteProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UByteProperty::CopyCompleteValue);
	if( ArrayDim==1 )
		*(BYTE*)Dest = *(BYTE*)Src;
	else
		appMemcpy( Dest, Src, ArrayDim );
	unguardSlow;
}
UBOOL UByteProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UByteProperty::Identical);
	return *(BYTE*)A == (B ? *(BYTE*)B : 0);
	unguardobjSlow;
}
void UByteProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UByteProperty::SerializeItem);
	Ar << *(BYTE*)Value;
	unguardobjSlow;
}
UBOOL UByteProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	Ar.SerializeBits( Data, Enum ? appCeilLogTwo(Enum->Names.Num()) : 8 );
	return 1;
	unguardobjSlow;
}
void UByteProperty::Serialize( FArchive& Ar )
{
	guard(UByteProperty::Serialize);
	Super::Serialize( Ar );
	Ar << Enum;
	unguardobj;
}
void UByteProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UByteProperty::ExportCppItem);
	Out.Log( TEXT("BYTE") );
	unguardobj;
}
void UByteProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UByteProperty::ExportTextItem);
	if( Enum )
		appSprintf( ValueStr, TEXT("%s"), *PropertyValue < Enum->Names.Num() ? *Enum->Names(*PropertyValue) : TEXT("(INVALID)") );
	else
		appSprintf( ValueStr, TEXT("%i"), *PropertyValue );
	unguardobj;
}
const TCHAR* UByteProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UByteProperty::ImportText);
	TCHAR Temp[1024];
	if( Enum )
	{
		Buffer = ReadToken( Buffer, Temp, ARRAY_COUNT(Temp) );
		if( !Buffer )
			return NULL;
		FName EnumName = FName( Temp, FNAME_Find );
		if( EnumName != NAME_None )
		{
			INT EnumIndex=0;
			if( Enum->Names.FindItem( EnumName, EnumIndex ) )
			{
				*(BYTE*)Data = EnumIndex;
				return Buffer;
			}
		}
	}
	if( appIsDigit(*Buffer) )
	{
		*(BYTE*)Data = appAtoi( Buffer );
		while( *Buffer>='0' && *Buffer<='9' )
			Buffer++;
	}
	else
	{
		//debugf( "Import: Missing byte" );
		return NULL;
	}
	return Buffer;
	unguardobj;
}
IMPLEMENT_CLASS(UByteProperty);

/*-----------------------------------------------------------------------------
	UIntProperty.
-----------------------------------------------------------------------------*/

void UIntProperty::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	if( PropertyFlags & CPF_HasMinMax )
		Ar << MinValue << MaxValue << Interval;
}

void UIntProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UIntProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize = sizeof(INT);
	Offset      = Align( GetOuterUField()->GetPropertiesSize(), sizeof(INT) );
	unguardobj;
}
void UIntProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UIntProperty::CopySingleValue);
	*(INT*)Dest = *(INT*)Src;
	unguardSlow;
}
void UIntProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UIntProperty::CopyCompleteValue);
	if( ArrayDim==1 )
		*(INT*)Dest = *(INT*)Src;
	else
		for( INT i=0; i<ArrayDim; i++ )
			((INT*)Dest)[i] = ((INT*)Src)[i];
	unguardSlow;
}
UBOOL UIntProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UIntProperty::Identical);
	return *(INT*)A == (B ? *(INT*)B : 0);
	unguardobjSlow;
}
void UIntProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UIntProperty::SerializeItem);
	Ar << *(INT*)Value;
	unguardobjSlow;
}
UBOOL UIntProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UIntProperty::NetSerializeItem);

 	if( PropertyFlags & CPF_HasMinMax )
 	{
		DWORD Max = (Abs(MinValue) + Abs(MaxValue) + 1) / Interval;
		INT IntData = Clamp<INT>(*(INT*)Data,MinValue,MaxValue);
		DWORD MidData( ( IntData - MinValue + Interval / 2 ) / Interval );
		Ar.SerializeInt( MidData, Max );
		if( Ar.IsLoading() )
			*(INT*)Data = MidData * Interval + MinValue;
	}
	else Ar << *(INT*)Data;
	return 1;
	unguardSlow;
}
void UIntProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UIntProperty::ExportCppItem);
	Out.Log( TEXT("INT") );
	unguardobj;
}
void UIntProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UIntProperty::ExportTextItem);
	appSprintf( ValueStr, TEXT("%i"), *(INT *)PropertyValue );
	unguardobj;
}
const TCHAR* UIntProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UIntProperty::ImportText);
	if( *Buffer=='-' || (*Buffer>='0' && *Buffer<='9') )
		*(INT*)Data = appAtoi( Buffer );
	while( *Buffer=='-' || (*Buffer>='0' && *Buffer<='9') )
		Buffer++;
	return Buffer;
	unguardobj;
}
IMPLEMENT_CLASS(UIntProperty);

/*-----------------------------------------------------------------------------
	UDelegateProperty.
-----------------------------------------------------------------------------*/

void UDelegateProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UDelegateProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize = sizeof(FScriptDelegate);
	Offset      = Align( GetOuterUField()->GetPropertiesSize(), sizeof(INT) );
	PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
void UDelegateProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UDelegateProperty::CopySingleValue);
	*(FScriptDelegate*)Dest = *(FScriptDelegate*)Src;
	unguardSlow;
}
void UDelegateProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UDelegateProperty::CopyCompleteValue);
    if( SuperObject )
	{
		if( ArrayDim==1)
		{
			UClass* Cls = Cast<UClass>(((FScriptDelegate*)Src)->Object);			
			((FScriptDelegate*)Dest)->FunctionName = ((FScriptDelegate*)Src)->FunctionName;
			((FScriptDelegate*)Dest)->Object = (Cls && SuperObject->IsA(Cls)) ? SuperObject : ((FScriptDelegate*)Src)->Object;
		}
		else
		{
			for( INT i=0; i<ArrayDim; i++ )
			{
				UClass* Cls = Cast<UClass>(((FScriptDelegate*)Src)[i].Object);			
				((FScriptDelegate*)Dest)[i].FunctionName = ((FScriptDelegate*)Src)[i].FunctionName;
				((FScriptDelegate*)Dest)[i].Object = (Cls && SuperObject->IsA(Cls)) ? SuperObject : ((FScriptDelegate*)Src)[i].Object;
			}
		}
	}
	else
	{
	if( ArrayDim==1 )
		*(FScriptDelegate*)Dest = *(FScriptDelegate*)Src;
	else
		for( INT i=0; i<ArrayDim; i++ )
			((FScriptDelegate*)Dest)[i] = ((FScriptDelegate*)Src)[i];
	}
	unguardSlow;
}
UBOOL UDelegateProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UDelegateProperty::Identical);
	FScriptDelegate* DA = (FScriptDelegate*)A;
	FScriptDelegate* DB = (FScriptDelegate*)B;
	if( !DB )
		return DA->Object==NULL;
	return ( DA->Object == DB->Object && DA->FunctionName == DB->FunctionName );
	unguardobjSlow;
}
void UDelegateProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UDelegateProperty::SerializeItem);
	Ar << *(FScriptDelegate*)Value;
	unguardobjSlow;
}
UBOOL UDelegateProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UDelegateProperty::NetSerializeItem);
	Ar << *(FScriptDelegate*)Data;
	return 1;
	unguardSlow;
}
void UDelegateProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UDelegateProperty::ExportCppItem);
	Out.Log( TEXT("FScriptDelegate") );
	unguardobj;
}
void UDelegateProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UDelegateProperty::ExportTextItem);
	appSprintf( ValueStr, TEXT("%s.%s"), (*(FScriptDelegate*)PropertyValue).Object->GetName(), *(*(FScriptDelegate*)PropertyValue).FunctionName );
	unguardobj;
}
const TCHAR* UDelegateProperty::ImportText( const TCHAR* Buffer, BYTE* PropertyValue, INT PortFlags ) const
{
	guard(UDelegateProperty::ImportText);
	TCHAR ObjName[NAME_SIZE];
	TCHAR FuncName[NAME_SIZE];
	// Get object name
	INT i;
	for( i=0; *Buffer && *Buffer != '.'; Buffer++ )
		ObjName[i++] = *Buffer;
	ObjName[i] = '\0';
	// Get function name
	if( *Buffer )
	{
		Buffer++;
		for( i=0; *Buffer; Buffer++ )
			FuncName[i++] = *Buffer;
		FuncName[i] = '\0';                
	}
	else
		FuncName[0] = '\0';
	UObject* Object = StaticFindObject( UObject::StaticClass(), ANY_PACKAGE, ObjName );
	UFunction* Func = NULL;
	if( Object )
	{
        UClass* Cls = Cast<UClass>(Object);
		if( !Cls )
			Cls = Object->GetClass();
		// Check function params.
		Func = FindField<UFunction>( Cls, FuncName );
		if( Func )
		{
			// Find the delegate UFunction to check params
			checkSlow(appStrlen(GetName()) > 2 );
			appStrcpy( FuncName, &(GetName())[2] );
			check(appStrlen(FuncName) > 10 );
			FuncName[appStrlen(FuncName)-10] = '\0';	//!! hackish   GetName() is something like __DelegateName__Delegate
            UFunction* Delegate = FindField<UFunction>( CastChecked<UClass>(GetOuter()), FuncName );
			check(Delegate);
			// check return type and params
			if(	Func->NumParms == Delegate->NumParms )
			{
				INT Count=0;
				for( TFieldIterator<UProperty,CLASS_IsAUProperty> It1(Func),It2(Delegate); Count<Delegate->NumParms; ++It1,++It2,++Count )
				{
					if( It1->GetClass()!=It2->GetClass() || (It1->PropertyFlags&CPF_OutParm)!=(It2->PropertyFlags&CPF_OutParm) )
					{
						Func = NULL;
						break;
					}
				}
			}
			else
				Func = NULL;
		}
	}
	(*(FScriptDelegate*)PropertyValue).Object		= Func ? Object				: NULL;
	(*(FScriptDelegate*)PropertyValue).FunctionName = Func ? Func->GetFName()	: NAME_None;
	return Buffer;
	unguardobj;
}

void UDelegateProperty::Serialize( FArchive& Ar )
{
	guard(UDelegateProperty::Serialize);
	Super::Serialize( Ar );
	Ar << Function;
	unguardobj;
}
void UDelegateProperty::CleanupDestroyed( BYTE* Data ) const
{
	guard(UDelegateProperty::CleanupDestroyed);
	for(int i=0; i<ArrayDim; i++)
	{
		FScriptDelegate* Delegate = (FScriptDelegate*)(Data+i*ElementSize);
		if(Delegate->Object)
		{
			check(Delegate->Object->IsValid());
			if( Delegate->Object->IsPendingKill() )
			{
				Delegate->Object->Modify();
				Delegate->Object = NULL;
				Delegate->FunctionName = NAME_None;
			}
		}
	}
	unguardobj;
}
#if 1 //NEW (mdf) tbd: object refs validation
UBOOL UDelegateProperty::ValidateRefs( BYTE* Data ) const
{
	guard(UDelegateProperty::ValidateRefs);

	UBOOL bResult = true;
	for(int i=0; i<ArrayDim; i++)
	{
		FScriptDelegate* Delegate = (FScriptDelegate*)(Data+i*ElementSize);
		if( Delegate->Object && (!Delegate->Object->IsValid() || Delegate->Object->IsPendingKill()) )
		{
			debugf( L"WARNING (ASSERT): %s had a reference to destroyed actor %s!", GetName(),  Delegate->Object->IsValid() ?  Delegate->Object->GetName() : L"NA" );
			bResult = false;
			Delegate->Object = NULL;
		}
	}
	return bResult;
	
	unguardobj;
}	
#endif
IMPLEMENT_CLASS(UDelegateProperty);

/*-----------------------------------------------------------------------------
	UBoolProperty.
-----------------------------------------------------------------------------*/

void UBoolProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UBoolProperty::Link);
	Super::Link( Ar, Prev );
	UBoolProperty* PrevBool = Cast<UBoolProperty>( Prev );
	if( GetOuterUField()->MergeBools() && PrevBool && NEXT_BITFIELD(PrevBool->BitMask) )
	{
		Offset  = Prev->Offset;
		BitMask = NEXT_BITFIELD(PrevBool->BitMask);
	}
	else
	{
		Offset  = Align(GetOuterUField()->GetPropertiesSize(),sizeof(BITFIELD));
		BitMask = FIRST_BITFIELD;
	}
	ElementSize = sizeof(BITFIELD);
	unguardobj;
}
void UBoolProperty::Serialize( FArchive& Ar )
{
	guard(UBoolProperty::Serialize);
	Super::Serialize( Ar );
	if( !Ar.IsLoading() && !Ar.IsSaving() )
#if __MWERKS__
	{
		QWORD Temp = BitMask;
		Ar << Temp;
		BitMask = Temp;
	}
#else
		Ar << BitMask;
#endif
	unguardobj;
}
void UBoolProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UBoolProperty::ExportCppItem);
	Out.Log( TEXT("BITFIELD") );
	unguardobj;
}
void UBoolProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UBoolProperty::ExportTextItem);
	TCHAR* Temp
	=	(TCHAR*) ((PortFlags & PPF_Localized)
	?	(((*(BITFIELD*)PropertyValue) & BitMask) ? GTrue  : GFalse )
	:	(((*(BITFIELD*)PropertyValue) & BitMask) ? TEXT("True") : TEXT("False")));
	appSprintf( ValueStr, TEXT("%s"), Temp );
	unguardobj;
}
const TCHAR* UBoolProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UBoolProperty::ImportText);
	TCHAR Temp[1024];
	Buffer = ReadToken( Buffer, Temp, ARRAY_COUNT(Temp) );
	if( !Buffer )
		return NULL;
	if( appStricmp(Temp,TEXT("1"))==0 || appStricmp(Temp,TEXT("True"))==0 || appStricmp(Temp,GTrue)==0 )
	{
		*(BITFIELD*)Data |= BitMask;
	}
	else if( appStricmp(Temp,TEXT("0"))==0 || appStricmp(Temp,TEXT("False"))==0  || appStricmp(Temp,GFalse)==0 )
	{
		*(BITFIELD*)Data &= ~BitMask;
	}
	else
	{
		//debugf( "Import: Failed to get bool" );
		return NULL;
	}
	return Buffer;
	unguardobj;
}
UBOOL UBoolProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UBoolProperty::Identical);
	return ((*(BITFIELD*)A ^ (B ? *(BITFIELD*)B : 0)) & BitMask) == 0;
	unguardobjSlow;
}
void UBoolProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UBoolProperty::SerializeItem);
	BYTE B = (*(BITFIELD*)Value & BitMask) ? 1 : 0;
	Ar << B;
	if( B ) *(BITFIELD*)Value |=  BitMask;
	else    *(BITFIELD*)Value &= ~BitMask;
	unguardobjSlow;
}
UBOOL UBoolProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	BYTE Value = ((*(BITFIELD*)Data & BitMask)!=0);
	Ar.SerializeBits( &Value, 1 );
	if( Value )
		*(BITFIELD*)Data |= BitMask;
	else
		*(BITFIELD*)Data &= ~BitMask;
	return 1;
	unguardobjSlow;
}
void UBoolProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UProperty::CopySingleValue);
	*(BITFIELD*)Dest = (*(BITFIELD*)Dest & ~BitMask) | (*(BITFIELD*)Src & BitMask);
	unguardobjSlow;
}
IMPLEMENT_CLASS(UBoolProperty);

/*-----------------------------------------------------------------------------
	UFloatProperty.
-----------------------------------------------------------------------------*/

void UFloatProperty::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	if( PropertyFlags & CPF_HasMinMax )
		Ar << MinValue << MaxValue << Interval;
}

void UFloatProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UFloatProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize = sizeof(FLOAT);
	Offset      = Align( GetOuterUField()->GetPropertiesSize(), sizeof(FLOAT) );
	unguardobj;
}
void UFloatProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UFloatProperty::CopySingleValue);
	*(FLOAT*)Dest = *(FLOAT*)Src;
	unguardSlow;
}
void UFloatProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UFloatProperty::CopyCompleteValue);
	if( ArrayDim==1 )
		*(FLOAT*)Dest = *(FLOAT*)Src;
	else
		for( INT i=0; i<ArrayDim; i++ )
			((FLOAT*)Dest)[i] = ((FLOAT*)Src)[i];
	unguardSlow;
}
UBOOL UFloatProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UFloatProperty::Identical);
	return *(FLOAT*)A == (B ? *(FLOAT*)B : 0);
	unguardobjSlow;
}
void UFloatProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UFloatProperty::SerializeItem);
	Ar << *(FLOAT*)Value;
	unguardobjSlow;
}
UBOOL UFloatProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UFloatProperty::NetSerializeItem);
	if( PropertyFlags & CPF_HasMinMax )
	{
		DWORD Max = appRound((0.0f + Abs(MinValue) + Abs(MaxValue)) / Interval) + 1; //the +1 is for the extra pole in the telephone-pole problem :)
		//BYTE Bits = appCeilLogTwo( Steps );
		//Bits = Clamp<BYTE>( Bits, 0, 32 );
		FLOAT FloatData = Clamp<FLOAT>(*(FLOAT*)Data,MinValue,MaxValue);
		//DWORD Max = 1<<(Bits);
		DWORD MidData( ( FloatData - MinValue + Interval / 2 ) / Interval );
		Ar.SerializeInt( MidData, Max );
		if( Ar.IsLoading() )
			*(FLOAT*)Data = MidData * Interval + MinValue;
	}
	else
	Ar << *(FLOAT*)Data;
	return 1;
	unguardSlow;
}
void UFloatProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UFloatProperty::ExportCppItem);
	Out.Log( TEXT("FLOAT") );
	unguardobj;
}
void UFloatProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UFloatProperty::ExportTextItem);
	appSprintf( ValueStr, TEXT("%f"), *(FLOAT*)PropertyValue );
	unguardobj;
}
const TCHAR* UFloatProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UFloatProperty::ImportText);
	*(FLOAT*)Data = appAtof(Buffer);
	while( *Buffer && *Buffer!=',' && *Buffer!=')' && *Buffer!=13 && *Buffer!=10 )
		Buffer++;
	return Buffer;
	unguardobj;
}
IMPLEMENT_CLASS(UFloatProperty);

/*-----------------------------------------------------------------------------
	UObjectProperty.
-----------------------------------------------------------------------------*/

void UObjectProperty::CleanupDestroyed( BYTE* Data ) const
{
	guard(UObjectProperty::CleanupDestroyed);
	for(int i=0; i<ArrayDim; i++)
	{
		UObject*& Obj=*(UObject**)(Data+i*ElementSize);
		if(Obj)
		{
			check(Obj->IsValid());
			if( Obj->IsPendingKill() )
			{
				Obj->Modify();
				Obj = NULL;
			}
		}
	}
	unguardobj;
}
#if 1 //NEW (mdf) tbd: object refs validation
UBOOL UObjectProperty::ValidateRefs( BYTE* Data ) const
{
	guard(UObjectProperty::ValidateRefs);
	
	UBOOL bResult = true;
	for(int i=0; i<ArrayDim; i++)
	{
		UObject*& Obj=*(UObject**)(Data+i*ElementSize);
		if( Obj && (!Obj->IsValid() || Obj->IsPendingKill()) )
		{
			debugf( L"WARNING (ASSERT): %s had a reference to destroyed actor %s!", GetName(), Obj->IsValid() ? Obj->GetName() : L"NA" );
			bResult = false;
			Obj = NULL;
		}
	}
	return bResult;
	
	unguardobj;		
}	
#endif
void UObjectProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UObjectProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize = sizeof(UObject*);
	Offset      = Align( GetOuterUField()->GetPropertiesSize(), sizeof(UObject*) );
	if( ((PropertyFlags & CPF_EditInline) && (PropertyFlags & CPF_ExportObject)) || (PropertyClass->ClassFlags & CLASS_AutoInstancedProps) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
void UObjectProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UObjectProperty::CopySingleValue);
	*(UObject**)Dest = *(UObject**)Src;
	unguardSlow;
}
void UObjectProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UObjectProperty::CopyCompleteValue);
	if( !(GIsEditor&&GIsUCC) && (PropertyFlags & CPF_NeedCtorLink) && SuperObject )
	{
		for( INT i=0; i<ArrayDim; i++ )
		{
			if( ((UObject**)Src)[i] )
			{
				UClass* Cls = ((UObject**)Src)[i]->GetClass();
				((UObject**)Dest)[i] = StaticConstructObject( Cls, SuperObject->GetOuter(), NAME_None, 0, ((UObject**)Src)[i], GError, SuperObject ); 
			}
			else
				((UObject**)Dest)[i] = ((UObject**)Src)[i];
		}
	}
	else
	{
		for( INT i=0; i<ArrayDim; i++ )
			((UObject**)Dest)[i] = ((UObject**)Src)[i];
	}
	unguardSlow;
}
UBOOL UObjectProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UObjectProperty::Identical);
	return *(UObject**)A == (B ? *(UObject**)B : NULL);
	unguardobjSlow;
}
void UObjectProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UObjectProperty::SerializeItem);
	Ar << *(UObject**)Value;
	unguardobjSlow;
}
UBOOL UObjectProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	return Map->SerializeObject( Ar, PropertyClass, *(UObject**)Data );
	unguardobjSlow;
}
void UObjectProperty::Serialize( FArchive& Ar )
{
	guard(UObjectProperty::Serialize);
	Super::Serialize( Ar );
	Ar << PropertyClass;
	unguardobj;
}
void UObjectProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UObjectProperty::ExportCppItem);
	Out.Logf( TEXT("class %s*"), PropertyClass->GetNameCPP() );
	unguardobj;
}
void UObjectProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UObjectProperty::ExportTextItem);
	UObject* Temp = *(UObject **)PropertyValue;
	if( Temp != NULL )
		appSprintf( ValueStr, TEXT("%s'%s'"), Temp->GetClass()->GetName(), Temp->GetPathName() );
	else
		appStrcpy( ValueStr, TEXT("None") );
	unguardobj;
}
const TCHAR* UObjectProperty::ImportText( const TCHAR* InBuffer, BYTE* Data, INT PortFlags ) const
{
	guard(UObjectProperty::ImportText);
    const TCHAR* Buffer = InBuffer;
	TCHAR Temp[1024], Other[1024];
	Buffer = ReadToken( Buffer, Temp, ARRAY_COUNT(Temp), 1 );
	if( !Buffer )
	{
		return NULL;
	}
	if( appStricmp( Temp, TEXT("None") )==0 )
	{
		*(UObject**)Data = NULL;
	}
	else
	{
		while( *Buffer == ' ' )
			Buffer++;
		if( *Buffer++ != '\'' )
		{
			*(UObject**)Data = StaticFindObject( PropertyClass, ANY_PACKAGE, Temp );
			if( !*(UObject**)Data )
            {
                if( PortFlags & PPF_CheckReferences )
					GWarn->Logf( NAME_Error, TEXT("%s: unresolved reference to '%s'"), GetFullName(), InBuffer );
 				return NULL;
            }
		}
		else
		{
			Buffer = ReadToken( Buffer, Other, ARRAY_COUNT(Temp), 1 );
			if( !Buffer )
				return NULL;
			if( *Buffer++ != '\'' )
				return NULL;
			UClass* ObjectClass = FindObject<UClass>( ANY_PACKAGE, Temp );
			if( !ObjectClass )
            {
                if( PortFlags & PPF_CheckReferences )
					GWarn->Logf( NAME_Error, TEXT("%s: unresolved cast in '%s'"), GetFullName(), InBuffer );
				return NULL;
            }

			// Try the find the object.
			*(UObject**)Data = StaticFindObject( ObjectClass, ANY_PACKAGE, Other );

			// If we can't find it, try to load it.
			if( !*(UObject**)Data )
				*(UObject**)Data = StaticLoadObject( ObjectClass, NULL, Other, NULL, LOAD_NoWarn, NULL );

            if( *(UObject**)Data && (PortFlags & PPF_CheckReferences) && !(*(UObject**)Data)->GetClass()->IsChildOf(PropertyClass) )
				GWarn->Logf( NAME_Error, TEXT("%s: bad cast in '%s'"), GetFullName(), InBuffer );

			// If we couldn't find it or load it, we'll have to do without it.
			if( !*(UObject**)Data )
            {
                if( PortFlags & PPF_CheckReferences )
					GWarn->Logf( NAME_Error, TEXT("%s: unresolved reference to '%s'"), GetFullName(), InBuffer );
				return NULL;
            }
		}
	}
	return Buffer;
	unguardobj;
}
IMPLEMENT_CLASS(UObjectProperty);

/*-----------------------------------------------------------------------------
	UClassProperty.
-----------------------------------------------------------------------------*/

void UClassProperty::Serialize( FArchive& Ar )
{
	guard(UClassProperty::Serialize);
	Super::Serialize( Ar );
	Ar << MetaClass;
	check(MetaClass);
	unguardobj;
}
const TCHAR* UClassProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UClassProperty::ImportText);
	const TCHAR* Result = UObjectProperty::ImportText( Buffer, Data, PortFlags );
	if( Result )
	{
		// Validate metaclass.
		UClass*& C = *(UClass**)Data;
		if( C && C->GetClass()!=UClass::StaticClass() || !C->IsChildOf(MetaClass) )
			C = NULL;
	}
	return Result;
	unguard;
}
IMPLEMENT_CLASS(UClassProperty);

/*-----------------------------------------------------------------------------
	UNameProperty.
-----------------------------------------------------------------------------*/

void UNameProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UNameProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize = sizeof(FName);
	Offset      = Align( GetOuterUField()->GetPropertiesSize(), sizeof(FName) );
	unguardobj;
}
void UNameProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UNameProperty::CopySingleValue);
	*(FName*)Dest = *(FName*)Src;
	unguardSlow;
}
void UNameProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UNameProperty::CopyCompleteValue);
	if( ArrayDim==1 )
		*(FName*)Dest = *(FName*)Src;
	else
		for( INT i=0; i<ArrayDim; i++ )
			((FName*)Dest)[i] = ((FName*)Src)[i];
	unguardSlow;
}
UBOOL UNameProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UNameProperty::Identical);
	return *(FName*)A == (B ? *(FName*)B : FName(NAME_None));
	unguardobjSlow;
}
void UNameProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UNameProperty::SerializeItem);
	Ar << *(FName*)Value;
	unguardobjSlow;
}
void UNameProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UNameProperty::ExportCppItem);
	Out.Log( TEXT("FName") );
	unguardobj;
}
void UNameProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UNameProperty::ExportTextItem);
	FName Temp = *(FName*)PropertyValue;
	if( !(PortFlags & PPF_Delimited) )
		appStrcpy( ValueStr, *Temp );
	else
		appSprintf( ValueStr, TEXT("\"%s\""), *Temp );
	unguardobj;
}
const TCHAR* UNameProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UNameProperty::ImportText);
	TCHAR Temp[1024];
	Buffer = ReadToken( Buffer, Temp, ARRAY_COUNT(Temp) );
	if( !Buffer )
		return NULL;
	*(FName*)Data = FName(Temp);
	return Buffer;
	unguardobj;
}
IMPLEMENT_CLASS(UNameProperty);

/*-----------------------------------------------------------------------------
	UStrProperty.
-----------------------------------------------------------------------------*/

void UStrProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UStrProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize    = sizeof(FString);
	Offset         = Align( GetOuterUField()->GetPropertiesSize(), PROPERTY_ALIGNMENT );
	if( !(PropertyFlags & CPF_Native) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
UBOOL UStrProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UStrProperty::Identical);
	return appStricmp( **(const FString*)A, B ? **(const FString*)B : TEXT("") )==0;
	unguardobjSlow;
}
void UStrProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UStrProperty::SerializeItem);
	Ar << *(FString*)Value;
	unguardobjSlow;
}
void UStrProperty::Serialize( FArchive& Ar )
{
	guard(UStrProperty::Serialize);
	Super::Serialize( Ar );
	unguardobj;
}
void UStrProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UStrProperty::ExportCppItem);
	Out.Log( TEXT("FString") );
	unguardobj;
}
void UStrProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UStrProperty::ExportTextItem);
	if( !(PortFlags & PPF_Delimited) )
		appStrcpy( ValueStr, **(FString*)PropertyValue );
	else
		appSprintf( ValueStr, TEXT("\"%s\""), **(FString*)PropertyValue );
	unguardobj;
}
const TCHAR* UStrProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UStrProperty::ImportText);
	if( !(PortFlags & PPF_Delimited) )
	{
		*(FString*)Data = Buffer;
	}
	else
	{
		TCHAR Temp[4096];//!!
		Buffer = ReadToken( Buffer, Temp, ARRAY_COUNT(Temp) );
		if( !Buffer )
			return NULL;
		*(FString*)Data = Temp;
	}
	return Buffer;
	unguardobj;
}
void UStrProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UStrProperty::CopySingleValue);
	*(FString*)Dest = *(FString*)Src;
	unguardobjSlow;
}
void UStrProperty::DestroyValue( void* Dest ) const
{
	guardSlow(UStrProperty::DestroyValue);
	for( INT i=0; i<ArrayDim; i++ )
		(*(FString*)((BYTE*)Dest+i*ElementSize)).~FString();
	unguardobjSlow;
}
IMPLEMENT_CLASS(UStrProperty);

/*-----------------------------------------------------------------------------
	UFixedArrayProperty.
-----------------------------------------------------------------------------*/

void UFixedArrayProperty::CleanupDestroyed( BYTE* Data ) const
{
	guard(UFixedArrayProperty::CleanupDestroyed);
	for(int i=0; i<Count; i++)
		Inner->CleanupDestroyed(Data+i*GetSize());
	unguardobj;
}
#if 1 //NEW (mdf) tbd: object refs validation
UBOOL UFixedArrayProperty::ValidateRefs( BYTE* Data ) const
{
	guard(UFixedArrayProperty::ValidateRefs);
	
	UBOOL bResult = true;
	for(int i=0; i<Count; i++)
		if( !Inner->ValidateRefs( Data+i*GetSize() ) )
			bResult = false;
	return bResult;
	
	unguardobj;
}
#endif
void UFixedArrayProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UFixedArrayProperty::Link);
	checkSlow(Count>0);
	Super::Link( Ar, Prev );
	Ar.Preload( Inner );
	Inner->Link( Ar, NULL );
	ElementSize    = Inner->ElementSize * Count;
	Offset         = Align( GetOuterUField()->GetPropertiesSize(), PROPERTY_ALIGNMENT );
	if( !(PropertyFlags & CPF_Native) )
		PropertyFlags |= (Inner->PropertyFlags & CPF_NeedCtorLink);
	unguardobj;
}
UBOOL UFixedArrayProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UFixedArrayProperty::Identical);
	checkSlow(Inner);
	for( INT i=0; i<Count; i++ )
		if( !Inner->Identical( (BYTE*)A+i*Inner->ElementSize, B ? ((BYTE*)B+i*Inner->ElementSize) : NULL ) )
			return 0;
	return 1;
	unguardobjSlow;
}
void UFixedArrayProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UFixedArrayProperty::SerializeItem);
	checkSlow(Inner);
	for( INT i=0; i<Count; i++ )
		Inner->SerializeItem( Ar, (BYTE*)Value + i*Inner->ElementSize, MaxReadBytes>0?MaxReadBytes/Count:0 );
	unguardobjSlow;
}
UBOOL UFixedArrayProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	return 1;
	unguardobjSlow;
}
void UFixedArrayProperty::Serialize( FArchive& Ar )
{
	guard(UFixedArrayProperty::Serialize);
	Super::Serialize( Ar );
	Ar << Inner << Count;
	checkSlow(Inner);
	unguardobj;
}
void UFixedArrayProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UFixedArrayProperty::ExportCppItem);
	checkSlow(Inner);
	Inner->ExportCppItem( Out );
	Out.Logf( TEXT("[%i]"), Count );
	unguardobj;
}
void UFixedArrayProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UFixedArrayProperty::ExportTextItem);
	checkSlow(Inner);
	*ValueStr++ = '(';
	for( INT i=0; i<Count; i++ )
	{
		if( i>0 )
			*ValueStr++ = ',';
		Inner->ExportTextItem( ValueStr, PropertyValue + i*Inner->ElementSize, DefaultValue ? (DefaultValue + i*Inner->ElementSize) : NULL, PortFlags|PPF_Delimited );
		ValueStr += appStrlen(ValueStr);
	}
	*ValueStr++ = ')';
	*ValueStr++ = 0;
	unguardobj;
}
const TCHAR* UFixedArrayProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UFixedArrayProperty::ImportText);
	checkSlow(Inner);
	if( *Buffer++ != '(' )
		return NULL;
	appMemzero( Data, ElementSize );
	for( INT i=0; i<Count; i++ )
	{
		Buffer = Inner->ImportText( Buffer, Data + i*Inner->ElementSize, PortFlags|PPF_Delimited );
		if( !Buffer )
			return NULL;
		if( *Buffer!=',' && i!=Count-1 )
			return NULL;
		Buffer++;
	}
	if( *Buffer++ != ')' )
		return NULL;
	return Buffer;
	unguardobj;
}
void UFixedArrayProperty::AddCppProperty( UProperty* Property, INT InCount )
{
	guard(UFixedArrayProperty::AddCppProperty);
	check(!Inner);
	check(Property);
	check(InCount>0);

	Inner = Property;
	Count = InCount;

	unguardobj;
}
void UFixedArrayProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UFixedArrayProperty::CopySingleValue);
	for( INT i=0; i<Count; i++ )
		Inner->CopyCompleteValue( (BYTE*)Dest + i*Inner->ElementSize, Src ? ((BYTE*)Src + i*Inner->ElementSize) : NULL );
	unguardobjSlow;
}
void UFixedArrayProperty::DestroyValue( void* Dest ) const
{
	guardSlow(UFixedArrayProperty::DestroyValue);
	for( INT i=0; i<Count; i++ )
		Inner->DestroyValue( (BYTE*)Dest + i*Inner->ElementSize );
	unguardobjSlow;
}
IMPLEMENT_CLASS(UFixedArrayProperty);

/*-----------------------------------------------------------------------------
	UArrayProperty.
-----------------------------------------------------------------------------*/

void UArrayProperty::CleanupDestroyed( BYTE* Data ) const
{
	guard(UArrayProperty::CleanupDestroyed);
	for(int i=0; i<ArrayDim; i++)
	{
		FArray* A=(FArray*)( Data + i*ElementSize );
		for(int j=0; j<A->Num(); j++)
			Inner->CleanupDestroyed((BYTE*)A->GetData() + j*Inner->GetSize());
	}
	unguardobj;
}
#if 1 //NEW (mdf) tbd: object refs validation
UBOOL UArrayProperty::ValidateRefs( BYTE* Data ) const
{
	guard(UArrayProperty::ValidateRefs);

	UBOOL bResult = true;
	for(int i=0; i<ArrayDim; i++)
	{
		FArray* A=(FArray*)( Data + i*ElementSize );
		for(int j=0; j<A->Num(); j++)
			if( !Inner->ValidateRefs((BYTE*)A->GetData() + j*Inner->GetSize()) )
				bResult = false;
	}
	return bResult;
	
	unguardobj;
}
#endif
void UArrayProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UArrayProperty::Link);
	Super::Link( Ar, Prev );
	Ar.Preload( Inner );
	Inner->Link( Ar, NULL );
	ElementSize    = sizeof(FArray);
	Offset         = Align( GetOuterUField()->GetPropertiesSize(), PROPERTY_ALIGNMENT );
	if( !(PropertyFlags & CPF_Native) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
UBOOL UArrayProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UArrayProperty::Identical);
	checkSlow(Inner);
	INT n = ((FArray*)A)->Num();
	if( n!=(B ? ((FArray*)B)->Num() : 0) )
		return 0;
	INT   c = Inner->ElementSize;
	BYTE* p = (BYTE*)((FArray*)A)->GetData();
	if( B )
	{
		BYTE* q = (BYTE*)((FArray*)B)->GetData();
		for( INT i=0; i<n; i++ )
			if( !Inner->Identical( p+i*c, q+i*c ) )
				return 0;
	}
	else
	{
		for( INT i=0; i<n; i++ )
			if( !Inner->Identical( p+i*c, 0 ) )
				return 0;
	}
	return 1;
	unguardobjSlow;
}
void UArrayProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UArrayProperty::SerializeItem);
	checkSlow(Inner);
	INT   c = Inner->ElementSize;
	INT   n = ((FArray*)Value)->Num();
	Ar << AR_INDEX(n);
	if( Ar.IsLoading() )
	{
		((FArray*)Value)->Empty( c );
		((FArray*)Value)->AddZeroed( c, n );
	}
	BYTE* p = (BYTE*)((FArray*)Value)->GetData();
	for( INT i=0; i<n; i++ )
		Inner->SerializeItem( Ar, p+i*c, MaxReadBytes>0?MaxReadBytes/n:0 );
	unguardobjSlow;
}
UBOOL UArrayProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	return 1;
	unguardobjSlow;
}
void UArrayProperty::Serialize( FArchive& Ar )
{
	guard(UArrayProperty::Serialize);
	Super::Serialize( Ar );
	Ar << Inner;
	checkSlow(Inner);
	unguardobj;
}
void UArrayProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UArrayProperty::ExportCppItem);
	checkSlow(Inner);
	if( IsParam )
		bOrdered ? Out.Log( TEXT("TOrderedArray<") ) : Out.Log( TEXT("TArray<") );
	else
		bOrdered ? Out.Log( TEXT("TOrderedArrayNoInit<") ) : Out.Log( TEXT("TArrayNoInit<") );
	Inner->ExportCppItem( Out );
	Out.Log( TEXT(">") );
	unguardobj;
}
void UArrayProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UArrayProperty::ExportTextItem);
	checkSlow(Inner);
	*ValueStr++ = '(';
	FArray* Array       = (FArray*)PropertyValue;
	FArray* Default     = (FArray*)DefaultValue;
	INT     ElementSize = Inner->ElementSize;
	for( INT i=0; i<Array->Num(); i++ )
	{
		if( i>0 )
			*ValueStr++ = ',';
		Inner->ExportTextItem( ValueStr, (BYTE*)Array->GetData() + i*ElementSize, (Default && Default->Num()>=i) ? (BYTE*)Default->GetData() + i*ElementSize : 0, PortFlags|PPF_Delimited );
		ValueStr += appStrlen(ValueStr);
	}
	*ValueStr++ = ')';
	*ValueStr++ = 0;
	unguardobj;
}
const TCHAR* UArrayProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UArrayProperty::ImportText);
	checkSlow(Inner);
	while( *Buffer == ' ' )
		Buffer++;
	if( *Buffer++ != '(' )
		return NULL;
	while( *Buffer == ' ' )
		Buffer++;
	FArray* Array       = (FArray*)Data;
	INT     ElementSize = Inner->ElementSize;
	Array->Empty( ElementSize );
	while( *Buffer != ')' )
	{
		INT Index = Array->Add( 1, ElementSize );
		appMemzero( (BYTE*)Array->GetData() + Index*ElementSize, ElementSize );
		Buffer = Inner->ImportText( Buffer, (BYTE*)Array->GetData() + Index*ElementSize, PortFlags|PPF_Delimited );
		if( !Buffer )
			return NULL;
		while( *Buffer == ' ' )
			Buffer++;
		if( *Buffer!=',' )
			break;
		Buffer++;
		while( *Buffer == ' ' )
			Buffer++;
	}
	if( *Buffer++ != ')' )
		return NULL;
	return Buffer;
	unguardobj;
}
void UArrayProperty::AddCppProperty( UProperty* Property )
{
	guard(UArrayProperty::AddCppProperty);
	check(!Inner);
	check(Property);

	Inner = Property;

	unguardobj;
}
void UArrayProperty::CopyCompleteValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UArrayProperty::CopyCompleteValue);
	FArray* SrcArray  = (FArray*)Src;
	FArray* DestArray = (FArray*)Dest;
	INT     Size      = Inner->ElementSize;
	DestArray->Empty( Size, SrcArray->Num() );//!!must destruct it if really copying
	if( Inner->PropertyFlags & CPF_NeedCtorLink )
	{
		// Copy all the elements.
		DestArray->AddZeroed( Size, SrcArray->Num() );
		BYTE* SrcData  = (BYTE*)SrcArray->GetData();
		BYTE* DestData = (BYTE*)DestArray->GetData();
		for( INT i=0; i<DestArray->Num(); i++ )
			Inner->CopyCompleteValue( DestData+i*Size, SrcData+i*Size, SuperObject );
	}
	else
	{
		// Copy all the elements.
		DestArray->Add( SrcArray->Num(), Size );
		appMemcpy( DestArray->GetData(), SrcArray->GetData(), SrcArray->Num()*Size );
	}
	unguardobjSlow;
}
void UArrayProperty::DestroyValue( void* Dest ) const
{
	guardSlow(UArrayProperty::DestroyValue);
	//!! fix for ucc make crash
	if( (Offset == 0) && GetOuter()->IsA(UClass::StaticClass()) )
 		return;
	FArray* DestArray = (FArray*)Dest;
	if( Inner->PropertyFlags & CPF_NeedCtorLink )
	{
		BYTE* DestData = (BYTE*)DestArray->GetData();
		INT   Size     = Inner->ElementSize;
		for( INT i=0; i<DestArray->Num(); i++ )
			Inner->DestroyValue( DestData+i*Size );
	}
	DestArray->~FArray();
	unguardobjSlow;
}
IMPLEMENT_CLASS(UArrayProperty);

/*-----------------------------------------------------------------------------
	UMapProperty.
-----------------------------------------------------------------------------*/

void UMapProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UMapProperty::Link);
	Super::Link( Ar, Prev );
	Ar.Preload( Key );
	Key->Link( Ar, NULL );
	Ar.Preload( Value );
	Value->Link( Ar, NULL );
	ElementSize    = sizeof(TMap<BYTE,BYTE>);
	Offset         = Align( GetOuterUField()->GetPropertiesSize(), PROPERTY_ALIGNMENT );
	if( !(PropertyFlags&CPF_Native) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
UBOOL UMapProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UMapProperty::Identical);
	checkSlow(Key);
	checkSlow(Value);
	/*
	INT n = ((FArray*)A)->Num();
	if( n!=(B ? ((FArray*)B)->Num() : 0) )
		return 0;
	INT   c = Inner->ElementSize;
	BYTE* p = (BYTE*)((FArray*)A)->GetData();
	if( B )
	{
		BYTE* q = (BYTE*)((FArray*)B)->GetData();
		for( INT i=0; i<n; i++ )
			if( !Inner->Identical( p+i*c, q+i*c ) )
				return 0;
	}
	else
	{
		for( INT i=0; i<n; i++ )
			if( !Inner->Identical( p+i*c, 0 ) )
				return 0;
	}
	*/
	return 1;
	unguardobjSlow;
}
void UMapProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UMapProperty::SerializeItem);
	checkSlow(Key);
	checkSlow(Value);
	/*
	INT   c = Inner->ElementSize;
	INT   n = ((FArray*)Value)->Num();
	Ar << AR_INDEX(n);
	if( Ar.IsLoading() )
	{
		((FArray*)Value)->Empty( c );
		((FArray*)Value)->Add( n, c );
	}
	BYTE* p = (BYTE*)((FArray*)Value)->GetData();
	for( INT i=0; i<n; i++ )
		Inner->SerializeItem( Ar, p+i*c );
	*/
	unguardobjSlow;
}
UBOOL UMapProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	return 1;
	unguardobjSlow;
}
void UMapProperty::Serialize( FArchive& Ar )
{
	guard(UMapProperty::Serialize);
	Super::Serialize( Ar );
	Ar << Key << Value;
	checkSlow(Key);
	checkSlow(Value);
	unguardobj;
}
void UMapProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UMapProperty::ExportCppItem);
	checkSlow(Key);
	checkSlow(Value);
	Out.Log( TEXT("TMap<") );
	Key->ExportCppItem( Out );
	Out.Log( TEXT(",") );
	Value->ExportCppItem( Out );
	Out.Log( TEXT(">") );
	unguardobj;
}
void UMapProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UMapProperty::ExportTextItem);
	checkSlow(Key);
	checkSlow(Value);
	/*
	*ValueStr++ = '(';
	FArray* Array       = (FArray*)PropertyValue;
	FArray* Default     = (FArray*)DefaultValue;
	INT     ElementSize = Inner->ElementSize;
	for( INT i=0; i<Array->Num(); i++ )
	{
		if( i>0 )
			*ValueStr++ = ',';
		Inner->ExportTextItem( ValueStr, (BYTE*)Array->GetData() + i*ElementSize, Default ? (BYTE*)Default->GetData() + i*ElementSize : 0, PortFlags|PPF_Delimited );
		ValueStr += appStrlen(ValueStr);
	}
	*ValueStr++ = ')';
	*ValueStr++ = 0;
	*/
	unguardobj;
}
const TCHAR* UMapProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UMapProperty::ImportText);
	checkSlow(Key);
	checkSlow(Value);
	/*
	if( *Buffer++ != '(' )
		return NULL;
	FArray* Array       = (FArray*)Data;
	INT     ElementSize = Inner->ElementSize;
	Array->Empty( ElementSize );
	while( *Buffer != ')' )
	{
		INT Index = Array->Add( 1, ElementSize );
		appMemzero( (BYTE*)Array->GetData() + Index*ElementSize, ElementSize );
		Buffer = Inner->ImportText( Buffer, (BYTE*)Array->GetData() + Index*ElementSize, PortFlags|PPF_Delimited );
		if( !Buffer )
			return NULL;
		if( *Buffer!=',' )
			break;
		Buffer++;
	}
	if( *Buffer++ != ')' )
		return NULL;
	*/
	return Buffer;
	unguardobj;
}
void UMapProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UMapProperty::CopySingleValue);
	/*
	TMap<BYTE,BYTE>* SrcMap    = (TMap<BYTE,BYTE>*)Src;
	TMap<BYTE,BYTE>* DestMap   = (TMap<BYTE,BYTE>*)Dest;
	INT              KeySize   = Key->ElementSize;
	INT              ValueSize = Value->ElementSize;
	DestMap->Empty( Size, SrcArray->Num() );//must destruct it if really copying
	if( Inner->PropertyFlags & CPF_NeedsCtorLink )
	{
		// Copy all the elements.
		DestArray->AddZeroed( Size, SrcArray->Num() );
		BYTE* SrcData  = (BYTE*)SrcArray->GetData();
		BYTE* DestData = (BYTE*)DestArray->GetData();
		for( INT i=0; i<DestArray->Num(); i++ )
			Inner->CopyCompleteValue( DestData+i*Size, SrcData+i*Size );
	}
	else
	{
		// Copy all the elements.
		DestArray->Add( SrcArray->Num(), Size );
		appMemcpy( DestArray->GetData(), SrcArray->GetData(), SrcArray->Num()*Size );
	}*/
	unguardobjSlow;
}
void UMapProperty::DestroyValue( void* Dest ) const
{
	guardSlow(UMapProperty::DestroyValue);
	/*
	FArray* DestArray = (FArray*)Dest;
	if( Inner->PropertyFlags & CPF_NeedsCtorLink )
	{
		BYTE* DestData = (BYTE*)DestArray->GetData();
		INT   Size     = Inner->ElementSize;
		for( INT i=0; i<DestArray->Num(); i++ )
			Inner->DestroyValue( DestData+i*Size );
	}
	DestArray->~FArray();
	*/
	unguardobjSlow;
}
IMPLEMENT_CLASS(UMapProperty);

/*-----------------------------------------------------------------------------
	UStructProperty.
-----------------------------------------------------------------------------*/

void UStructProperty::CleanupDestroyed( BYTE* Data ) const
{
	guard(UStructProperty::CleanupDestroyed);
	for(int i=0; i<ArrayDim; i++)
		Struct->CleanupDestroyed(Data+i*ElementSize);
	unguardobj;
}
#if 1 //NEW (mdf) tbd: object refs validation
UBOOL UStructProperty::ValidateRefs( BYTE* Data ) const
{
	guard(UStructProperty::ValidateRefs);
	
	UBOOL bResult = true;
	for(int i=0; i<ArrayDim; i++)
		if( !Struct->ValidateRefs( Data+i*ElementSize ) )
			bResult = false;
	return bResult;
	
	unguardobj;
}
#endif
void UStructProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UStructProperty::Link);
	Super::Link( Ar, Prev );
	Ar.Preload( Struct );
	ElementSize    = Struct->PropertiesSize;
	Offset         = Align( GetOuterUField()->GetPropertiesSize(), ElementSize==2 ? 2 : ElementSize>=4 ? 4 : 1 );
	if( Struct->ConstructorLink && !(PropertyFlags & CPF_Native) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
UBOOL UStructProperty::Identical( const void* A, const void* B ) const
{
	guardSlow(UStructProperty::Identical);
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Struct); It; ++It )
		for( INT i=0; i<It->ArrayDim; i++ )
			if( !It->Matches(A,B,i) )
				return 0;
	return 1;
	unguardobjSlow;
}
void UStructProperty::SerializeItem( FArchive& Ar, void* Value, INT MaxReadBytes ) const
{
	guardSlow(UStructProperty::SerializeItem);
	if( !(Ar.IsLoading() || Ar.IsSaving()) || 
		Ar.Ver() < 118 || 
		Struct->GetFName()==NAME_Vector ||
		Struct->GetFName()==NAME_Rotator ||
		Struct->GetFName()==NAME_Color )
	{
		Ar.Preload( Struct );
		Struct->SerializeBin( Ar, (BYTE*)Value, MaxReadBytes );
	}
	else
	{
		Struct->SerializeTaggedProperties( Ar, (BYTE*)Value, NULL );
	}
	unguardobjSlow;
}
UBOOL UStructProperty::NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const
{
	guardSlow(UStructProperty::NetSerializeItem);
#if NEWNET
	INT Mapped = 1;
#endif
	if( Struct->GetFName()==NAME_Vector )
	{
		FVector& V = *(FVector*)Data;
		INT X(appRound(V.X)), Y(appRound(V.Y)), Z(appRound(V.Z));
		DWORD Bits = Clamp<DWORD>( appCeilLogTwo(1+Max(Max(Abs(X),Abs(Y)),Abs(Z))), 1, 20 )-1;
		Ar.SerializeInt( Bits, 20 );
		INT   Bias = 1<<(Bits+1);
		DWORD Max  = 1<<(Bits+2);
		DWORD DX(X+Bias), DY(Y+Bias), DZ(Z+Bias);
		Ar.SerializeInt( DX, Max );
		Ar.SerializeInt( DY, Max );
		Ar.SerializeInt( DZ, Max );
		if( Ar.IsLoading() )
			V = FVector((INT)DX-Bias,(INT)DY-Bias,(INT)DZ-Bias);
	}
	else if( Struct->GetFName()==NAME_Rotator )
	{
		FRotator& R = *(FRotator*)Data;
//MJL: NEWNET more accurate rotator replication, in (3.1) 3.1-4.1 bytes. old system had 0.3-3.3 (2.3) bytes. (parens indicate most likely value)
//MJL: commented out my own code for now, just because it is not AS optimized. My code is if 0'ed for now
#if 0
		DWORD Pitch(0), Yaw(0), Roll(0);
		if( Ar.IsSaving() )
		{
			Pitch = (R.Pitch&65535)>>4;
			Yaw = (R.Yaw&65535)>>4;
			Roll = (R.Roll&65535) >> 8;
		}
		Ar.SerializeInt( Pitch, 65536>>4 );
		Ar.SerializeInt( Yaw, 65536>>4 );
		BYTE B = (Roll!=0);
		Ar.SerializeBits( &B, 1 );
		if( B )
			Ar.SerializeInt( Roll, 65536>>8 );
		if( Ar.IsLoading() )
			R = FRotator(Pitch<<4,Yaw<<4,Roll<<8);
#else
		BYTE Pitch(R.Pitch>>8), Yaw(R.Yaw>>8), Roll(R.Roll>>8), B;
		B = (Pitch!=0);
		Ar.SerializeBits( &B, 1 );
		if( B )
			Ar << Pitch;
		else
			Pitch = 0;
		B = (Yaw!=0);
		Ar.SerializeBits( &B, 1 );
		if( B )
			Ar << Yaw;
		else
			Yaw = 0;
		B = (Roll!=0);
		Ar.SerializeBits( &B, 1 );
		if( B )
			Ar << Roll;
		else
			Roll = 0;
		if( Ar.IsLoading() )
			R = FRotator(Pitch<<8,Yaw<<8,Roll<<8);
#endif
	}
	else if( Struct->GetFName()==NAME_Plane )
	{
		FPlane& P = *(FPlane*)Data;
		SWORD X(appRound(P.X)), Y(appRound(P.Y)), Z(appRound(P.Z)), W(appRound(P.W));
		Ar << X << Y << Z << W;
		if( Ar.IsLoading() )
			P = FPlane(X,Y,Z,W);
	}
	else if ( Struct->GetFName()==NAME_CompressedPosition )
	{
		FCompressedPosition& C = *(FCompressedPosition*)Data;

		INT VX = appRound(C.Velocity.X);
		INT VY = appRound(C.Velocity.Y);
		INT VZ = appRound(C.Velocity.Z);
		DWORD VelBits = Clamp<DWORD>( appCeilLogTwo(1+Max(Max(Abs(VX),Abs(VY)),Abs(VZ))), 1, 20 )-1;

		// Location
		INT X(appRound(C.Location.X)), Y(appRound(C.Location.Y)), Z(appRound(C.Location.Z));
		DWORD Bits = Clamp<DWORD>( appCeilLogTwo(1+Max(Max(Abs(X),Abs(Y)),Abs(Z))), 1, 20 )-1;
		Ar.SerializeInt( Bits, 20 );
		INT   Bias = 1<<(Bits+1);
		DWORD Max  = 1<<(Bits+2);
		DWORD DX(X+Bias), DY(Y+Bias), DZ(Z+Bias);
		Ar.SerializeInt( DX, Max );
		Ar.SerializeInt( DY, Max );
		Ar.SerializeInt( DZ, Max );
		if( Ar.IsLoading() )
			C.Location = FVector((INT)DX-Bias,(INT)DY-Bias,(INT)DZ-Bias);

		// Rotation
		BYTE Pitch(C.Rotation.Pitch>>8), Yaw(C.Rotation.Yaw>>8);
		Ar << Pitch;
		Ar << Yaw;
		if( Ar.IsLoading() )
			C.Rotation = FRotator(Pitch<<8,Yaw<<8,0);
		
		// Velocity
		Ar.SerializeInt( VelBits, 20 );
		Bias = 1<<(VelBits+1);
		Max  = 1<<(VelBits+2);
		DX = VX+Bias;
		DY = VY+Bias;
		DZ = VZ+Bias;
		Ar.SerializeInt( DX, Max );
		Ar.SerializeInt( DY, Max );
		Ar.SerializeInt( DZ, Max );
		if( Ar.IsLoading() )
			C.Velocity = FVector((INT)DX-Bias,(INT)DY-Bias,(INT)DZ-Bias);
	}
	else
	{
		for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Struct); It; ++It )
			if( Map->ObjectToIndex(*It)!=INDEX_NONE )
				for( INT i=0; i<It->ArrayDim; i++ )
#if NEWNET
					Mapped &= It->NetSerializeItem( Ar, Map, (BYTE*)Data+It->Offset+i*It->ElementSize );
#else
					It->NetSerializeItem( Ar, Map, (BYTE*)Data+It->Offset+i*It->ElementSize );
#endif
	}
#if NEWNET
	return Mapped;
#else
	return 1;
#endif
	unguardobjSlow;
}
void UStructProperty::Serialize( FArchive& Ar )
{
	guard(UStructProperty::Serialize);
	Super::Serialize( Ar );
	Ar << Struct;
	unguardobj;
}
void UStructProperty::ExportCppItem( FOutputDevice& Out, UBOOL IsParam ) const
{
	guard(UStructProperty::ExportCppItem);
	Out.Logf( TEXT("%s"), Struct->GetNameCPP() );
	unguardobj;
}
void UStructProperty::ExportTextItem( TCHAR* ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const
{
	guard(UStructProperty::ExportTextItem);
	INT Count=0;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Struct); It; ++It )
	{
		if( It->Port() )
		{
			for( INT Index=0; Index<It->ArrayDim; Index++ )
			{
				TCHAR Value[65536];
				if( It->ExportText(Index,Value,PropertyValue,DefaultValue,PPF_Delimited) )
				{
					Count++;
					if( Count == 1 )
						*ValueStr++ = '(';
					else
						*ValueStr++ = ',';
					if( It->ArrayDim == 1 )
						ValueStr += appSprintf( ValueStr, TEXT("%s="), It->GetName() );
					else
						ValueStr += appSprintf( ValueStr, TEXT("%s[%i]="), It->GetName(), Index );
					ValueStr += appSprintf( ValueStr, TEXT("%s"), Value );
				}
			}
		}
	}
	if( Count > 0 )
	{
		*ValueStr++ = ')';
		*ValueStr = 0;
	}
	unguardobj;
}
const TCHAR* UStructProperty::ImportText( const TCHAR* InBuffer, BYTE* Data, INT PortFlags ) const
{
    const TCHAR* Buffer = InBuffer;

	guard(UStructProperty::ImportText);
	while( *Buffer == ' ' )
		Buffer++;
	if( *Buffer++ == '(' )
	{
		while( *Buffer == ' ' )
			Buffer++;
		// Parse all properties.
		while( *Buffer != ')' )
		{
			// Get key name.
			TCHAR Name[NAME_SIZE];
			int Count=0;
			while( Count<NAME_SIZE-1 && *Buffer && *Buffer!='=' && *Buffer!='[' )
				Name[Count++] = *Buffer++;
			Name[Count++] = 0;

			while( *Buffer == ' ' )
				Buffer++;
			// Get optional array element.
			INT Element=0;
			if( *Buffer=='[' )
			{
				const TCHAR* Start=++Buffer;
				while( *Buffer>='0' && *Buffer<='9' )
					Buffer++;
				while( *Buffer == ' ' )
					Buffer++;
				if( *Buffer++ != ']' )
				{
					GWarn->Logf( NAME_Error, TEXT("ImportText: Illegal array element in: %s"), InBuffer );
					return NULL;
				}
				while( *Buffer == ' ' )
					Buffer++;
				Element = appAtoi( Start );
			}

			// Verify format.
			if( *Buffer++ != '=' )
			{
				GWarn->Logf( NAME_Error, TEXT("ImportText: Missing key name in: %s"), InBuffer );
				return NULL;
			}

            if( Element < 0 )
            {
				GWarn->Logf( NAME_Error, TEXT("ImportText: Negative array index in: %s"), InBuffer );
				return NULL;
            }
			while( *Buffer == ' ' )
				Buffer++;

			// See if the property exists in the struct.
			FName GotName( Name, FNAME_Find );
			UBOOL Parsed = 0;
			if( GotName!=NAME_None )
			{
				for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Struct); It; ++It )
				{
					UProperty* Property = *It;

					if(	Property->GetFName()!=GotName || 
						Property->GetSize()==0 ||
						!Property->Port() )
                        continue;

					if( Element >= Property->ArrayDim )
					{
				        GWarn->Logf( NAME_Error, TEXT("ImportText: Array index out of bounds in: %s"), InBuffer );
				        return NULL;
                    }
					Buffer = Property->ImportText( Buffer, Data + Property->Offset + Element*Property->ElementSize, PortFlags|PPF_Delimited );
					if( Buffer == NULL )
                    {
				        GWarn->Logf( NAME_Error, TEXT("ImportText failed in: %s"), InBuffer );
						return NULL;
                    }

					Parsed = 1;
                    break;
				}
			}

			// If not parsed, skip this property in the stream.
			if( !Parsed )
			{
			    GWarn->Logf( NAME_Error, TEXT("Unknown member %s in %s"), Name, GetName() );
				while( *Buffer == ' ' )
					Buffer++;
				INT SubCount=0;
				while
				(	*Buffer
				&&	*Buffer!=10
				&&	*Buffer!=13 
				&&	(SubCount>0 || *Buffer!=')')
				&&	(SubCount>0 || *Buffer!=',') )
				{
					if( *Buffer == 0x22 )
					{
						while( *Buffer && *Buffer!=0x22 && *Buffer!=10 && *Buffer!=13 )
							Buffer++;
						if( *Buffer != 0x22 )
						{
							GWarn->Logf( NAME_Error, TEXT("ImportText: Bad quoted string in: %s"), InBuffer );
							return NULL;
						}
					}
					else if( *Buffer == '(' )
					{
						SubCount++;
					}
					else if( *Buffer == ')' )
					{
						SubCount--;
						if( SubCount < 0 )
						{
							GWarn->Logf( NAME_Error, TEXT("ImportText: Bad parenthesised struct in: "), InBuffer );
							return NULL;
						}
					}
					Buffer++;
				}
				if( SubCount > 0 )
				{
					GWarn->Logf( NAME_Error, TEXT("ImportText: Incomplete parenthesised struct in: "), InBuffer );
					return NULL;
				}
			}
			while( *Buffer == ' ' )
				Buffer++;

			// Skip comma.
			if( *Buffer==',' )
			{
				// Skip comma.
				Buffer++;
			}
			else if( *Buffer!=')' )
			{
				GWarn->Logf( NAME_Error, TEXT("ImportText: Bad termination in: %s"), InBuffer );
				return NULL;
			}
			while( *Buffer == ' ' )
				Buffer++;
		}

		// Skip trailing ')'.
		Buffer++;
	}
	else
	{
		debugf( NAME_Warning, TEXT("ImportText: Struct missing '(' in: %s"), InBuffer );
		return NULL;
	}
	return Buffer;
	unguardobj;
}
void UStructProperty::CopySingleValue( void* Dest, void* Src, UObject* SuperObject ) const
{
	guardSlow(UStructProperty::CopySingleValue);
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Struct); It; ++It )
		It->CopyCompleteValue( (BYTE*)Dest + It->Offset, (BYTE*)Src + It->Offset, SuperObject );
	//could just do memcpy + ReinstanceCompleteValue
	unguardobjSlow;
}
void UStructProperty::DestroyValue( void* Dest ) const
{
	guardSlow(UStructProperty::DestroyValue);
	for( UProperty* P=Struct->ConstructorLink; P; P=P->ConstructorLinkNext )
	{
		if( ArrayDim <= 0 )
		{
			P->DestroyValue( (BYTE*) Dest + P->Offset );
		}
		else
		{
			for( INT i=0; i<ArrayDim; i++ )
				P->DestroyValue( (BYTE*)Dest + i*ElementSize + P->Offset );
		}
	}
	unguardobjSlow;
}
IMPLEMENT_CLASS(UStructProperty);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

