//#if 1 //NEW
//=============================================================================
// AUtil.cpp
// $Author: Mfox $
// $Date: 7/29/02 7:40p $
// $Revision: 9 $
//=============================================================================

#include "EnginePrivate.h"

IMPLEMENT_CLASS(AUtil);

const FLOAT DefaultTraceLimit		= 124500.0f;
const FLOAT DefaultTraceInterval	= 500.0f;
const FVector DefaultTraceDirection	= FVector(0,0,-1);

//-----------------------------------------------------------------------------
// STRING PARSING NATIVES
//-----------------------------------------------------------------------------

// NOTE (mdf): for now only have guards around execs which call these
// These originated in script code and at least some could pbly now make use
// of the native parsing functions.

FString AUtil::GetFloatString( float F, BYTE Precision )
{
	//debugf( L"UUU GetFloatString: %0.3f (%d) --> %s", F, Precision, FString::Printf( TEXT("%0.*f"), Precision, F) );
	return FString::Printf( TEXT("%0.*f"), Precision, F );
}

//-----------------------------------------------------------------------------

FString AUtil::GetVectorString( const FVector& V, BYTE Precision )
{
	//debugf( L"UUU GetVectorString: %0.3f,%0.3f,%0.3f (%d)", V.X, V.Y, V.Z, Precision );
	return GetFloatString( V.X, Precision ) + TEXT(",") + GetFloatString( V.Y, Precision ) + TEXT(",") + GetFloatString( V.Z, Precision );
}

//-----------------------------------------------------------------------------

FString AUtil::GetRotatorString( const FRotator& R, BYTE Precision )
{
	return GetFloatString( R.Yaw, Precision ) + TEXT(",") + GetFloatString( R.Pitch, Precision ) + TEXT(",") + GetFloatString( R.Roll, Precision );
}

//-----------------------------------------------------------------------------

FString AUtil::PadString( const FString& InStr, INT PadLen, UBOOL bRightJustify, const FString& PadStr )
{
	guardSlow(AUtil::PadString);

	//debugf( L"UUU PadString BEGIN: '%s' (%d, '%s')", *InStr, PadLen, *PadStr );

	FString strResult( *InStr );
	FString strPad( *PadStr );

	//debugf( L"UUU PadString BEGIN: strResult: '%s' strPad: '%s'", *strResult, *strPad );

	INT NumPadChars = PadLen-strResult.Len();

	//debugf( L"UUU PadString NumPadChars=%d", NumPadChars );

	if( bRightJustify )
	{
		//debugf( L"  right justify" );
		
		for( INT ii=0; ii<NumPadChars; ii++ )
			strResult = strPad + strResult;
	}
	else
	{
		//debugf( L"  left justify" );

		for( INT ii=0; ii<NumPadChars; ii++ )
			strResult = strResult + strPad;
	}

	//debugf( L"UUU PadString END" );

	return strResult;
	unguardSlow;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::EndsInDigit( const FString& Str )
{
	//debugf( L"UUU EndsInDigit: %s (%d)", *Str, appIsDigit( (*Str)[Str.Len()-1] ) );
	return appIsDigit( (*Str)[Str.Len()-1] );
}

//-----------------------------------------------------------------------------

void AUtil::TruncateDigits( FString& Str )
{
	guardSlow(AUtil::TruncateDigits);

	//debugf( L"UUU TruncateDigits IN: %s", *Str );

	// keep chopping last character of end of string as long as this is a digit
	INT LastNonDigit = Str.Len()-1;
	while( LastNonDigit >= 0 && appIsDigit( (*Str)[LastNonDigit] ) )
		LastNonDigit--;

	if( LastNonDigit < 0 )
		Str.Empty();
	else
		Str = Str.Mid( 0, LastNonDigit+1 );

	//debugf( L"UUU TruncateDigits OUT: %s", *Str );
	unguardSlow;
}

//-----------------------------------------------------------------------------
//mdf-tbd: any way to efficiently collapse TCHAR* and FString* versions?

void AUtil::StripSpaces( TCHAR* Text, UBOOL bStripLeading, UBOOL bStripTrailing )
{
	guardSlow(AUtil::StripSpaces);

	if( bStripLeading )
	{
		INT FirstNonWhite = 0;
		while( appIsSpace( Text[ FirstNonWhite ] ) )
			FirstNonWhite++;

		if( FirstNonWhite > 0 )
			appStrcpy( Text, &Text[ FirstNonWhite ] );
	}

	if( bStripTrailing )
	{
		INT LastNonWhite = appStrlen( Text )-1;
		while( LastNonWhite >= 0 && appIsSpace( Text[ LastNonWhite ] ) )
			LastNonWhite--;

		Text[ LastNonWhite+1 ] = 0;
	}

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::StripSpaces( FString& Str, UBOOL bStripLeading, UBOOL bStripTrailing )
{
	guardSlow(AUtil::StripSpaces);

	//debugf( L"UUU StripSpaces IN: %s", *Str );

	if( bStripLeading )
	{
		INT FirstNonWhite = 0;
		while( appIsSpace( (*Str)[FirstNonWhite] ) )
			FirstNonWhite++;

		//debugf( L"UUU FirstNonWhite=%d", FirstNonWhite );
		if( FirstNonWhite > 0 )
			Str = Str.Mid( FirstNonWhite );
	}

	if( bStripTrailing )
	{
		INT LastNonWhite = Str.Len()-1;
		while( LastNonWhite >= 0 && appIsSpace( (*Str)[LastNonWhite] ) )
			LastNonWhite--;

		//debugf( L"UUU LastNonWhite=%d", LastNonWhite );
		if( LastNonWhite < 0 )
			Str.Empty();
		else
			Str = Str.Mid( 0, LastNonWhite+1 );
	}

	//debugf( L"UUU StripSpaces OUT: %s", *Str );
	unguardSlow;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::ValidNameString( const FString& S )
{
	guardSlow(AUtil::ValidNameString);

	for( INT ii=0; ii<S.Len(); ii++ )
		if( !appIsNameChar( (*S)[ii] ) )
			return false;

	return true;

	unguardSlow;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::ValidFloatString( const FString& S )
{
	guardSlow(AUtil::ValidFloatString);

	for( INT ii=0; ii<S.Len(); ii++ )
		if( !appIsFloatChar( (*S)[ii] ) )
			return false;

	// mdf-tbd: check for over/underflow?
	return true;

	unguardSlow;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::ValidIntString( const FString& S )
{
	guardSlow(AUtil::ValidIntString);

	for( INT ii=0; ii<S.Len(); ii++ )
		if( !appIsIntChar( (*S)[ii] ) )
			return false;

	// mdf-tbd: check for over/underflow?
	return true;

	unguardSlow;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::ValidByteString( const FString& S )
{
	guardSlow(AUtil::ValidByteString);

	for( INT ii=0; ii<S.Len(); ii++ )
		if( !appIsDigit( (*S)[ii] ) )
			return false;

	// mdf-tbd: check for over/underflow?
	return true;

	unguardSlow;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::ValidBoolString( const FString& S )
{
	guardSlow(AUtil::ValidBoolString);

	return( S.Len()==1 && ((*S)[0] == '0' || (*S)[0] == '1') );

	unguardSlow;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::ParseToken( FString& S, FString& SOut )
{
	guardSlow(AUtil::ParseToken);

	//debugf( L"UUU ParseToken S=%s", *S );

	// skip any leading whitespace
	// mdf-tbd: call StripSpaces
	INT Index = 0;
	while( appIsSpace( (*S)[Index] ) )
		Index++;

	S = S.Mid( Index );

	if( S.Len() > 0 )
	{
		TCHAR StartChar = (*S)[0];
	
		// find end of token
		INT EndPos = -1;
		if( StartChar == '"' )
		{
			// chop of opening "
			S = S.Mid( 1 );

			// look for a closing "
			INT Index = 0;
			while( Index < S.Len() && EndPos == -1 )
			{
				if( ((*S)[Index] == '\\') && (Index < S.Len()-1) && ((*S)[Index+1] == '"') ) // allow literal quotations
				{
					if (!Index)
						S = S.Mid(1);
					else
						S = S.Left(Index) + S.Mid(Index+1);

					Index++; // skip past quotation
				}
				else if( (*S)[Index] == '"' )
					EndPos = Index;
				else
					Index++;
			}
		}
		else
		{
			// look for whitespace
			INT Index = 0;
			while( Index < S.Len() && EndPos == -1 )
			{
				if( appIsSpace( (*S)[Index] ) )
					EndPos = Index;
				else
					Index++;
			}
		}
	
		if( EndPos != -1 )
		{
			// copy token to SOut and update S
			SOut = S.Left( EndPos );
			S = S.Mid( EndPos+1 );
		}
		else
		{
			// reached end of string copy rest of string to SOut?
			if( StartChar != '"' )
			{
				SOut = S;
				S.Empty();
			}
			else
			{
				// didn't find closing "?
			}
		}
	}
	
	//debugf( L"UUU ParseToken SOut=%s", *SOut );

	return( SOut.Len() != 0 );

	unguardSlow;
}

//-----------------------------------------------------------------------------
// mdf-tbd: for now these all return -1=error, 0=nothing found, 1=success.

INT AUtil::ParseString( FString& S, FString& SOut, FString SDefault )
{
	guardSlow(AUtil::ParseString);

	FString STmp = TEXT("");

	//debugf( L"UUU ParseString (%s)", *S );
	if( ParseToken( S, STmp ) )
	{
		// no such thing as an invalid string so -1 never returned?
		SOut = STmp;
		//debugf( L"  UUU -- valid input: %s", *SOut );
		return 1;
	}

	//debugf( L"  UUU -- no token found" );
	SOut = SDefault;
	return 0;

	unguardSlow;
}

//-----------------------------------------------------------------------------

INT AUtil::ParseName( FString& S, FName& NOut, UBOOL bAdd, FName NDefault )
{
	guardSlow(AUtil::ParseName);

	FString STmp = TEXT("");

	//debugf( L"UUU ParseName (%s) bAdd: %d", *S, bAdd );
	if( ParseToken( S, STmp ) )
	{
		//debugf( L"UUU ParseName #2 (%s)", *S );
		if( ValidNameString( STmp ) )
		{
			NOut = UObject::StringToName( STmp, bAdd );
			if( NOut != NAME_None )
			{
				//debugf( L"  UUU -- valid input: %s", *NOut );
				return 1;
			}
#if 1 //NEW: CDH...
			// NAME_None should be okay if the text is literally "None"
			else if (!appStricmp(*STmp, TEXT("None")))
				return 1;
#endif // ...CDH
			//debugf( L"  UUU -- StringToName failed?" );
		}

		//debugf( L"  UUU -- warning invalid input" );
		return -1;
	}

	//debugf( L"  UUU -- no token found" );
	NOut = NDefault;
	return 0;

	unguardSlow;
}

//-----------------------------------------------------------------------------

INT AUtil::ParseFloat( FString& S, FLOAT& FOut, FLOAT FDefault )
{
	guardSlow(AUtil::ParseFloat);

	FString STmp = TEXT("");

	//debugf( L"UUU ParseFloat (%s)", *S );
	if( ParseToken( S, STmp ) )
	{
		//debugf( L"UUU %s", *STmp );
		if( ValidFloatString( STmp ) )
		{
			FOut = appAtof( *STmp );
			//debugf( L"  UUU -- valid input: %0.3f", FOut );
			return 1;
		}

		//debugf( L"  UUU -- warning invalid input" );
		return -1;  
	}

	//debugf( L"  UUU -- no token found" );
	FOut = FDefault;
	return 0;

	unguardSlow;
}

//-----------------------------------------------------------------------------

INT AUtil::ParseInt( FString& S, INT& IOut, INT IDefault )
{
	guardSlow(AUtil::ParseInt);

	FString STmp = TEXT("");

	//debugf( L"UUU ParseInt (%s)", *S );
	if( ParseToken( S, STmp ) )
	{
		if( ValidIntString( STmp ) )
		{
			IOut = appAtoi( *STmp );
			//debugf( L"  UUU -- valid input: %d", IOut );
			return 1;
		}

		//debugf( L"  UUU -- warning invalid input" );
		return -1;
	}

	//debugf( L"  UUU -- no token found" );
	IOut = IDefault;
	return 0;

	unguardSlow;
}

//-----------------------------------------------------------------------------

INT AUtil::ParseByte( FString& S, BYTE& BOut, BYTE BDefault )
{
	guardSlow(AUtil::ParseByte);

	FString STmp = TEXT("");

	//debugf( L"UUU ParseByte (%s)", *S );
	if( ParseToken( S, STmp ) )
	{
		if( ValidByteString( STmp ) )
		{
			BOut = appAtoi( *STmp );
			//debugf( L"  UUU -- valid input: %d", BOut );
			return 1;
		}

		//debugf( L"  UUU -- warning invalid input" );
		return -1;
	}

	//debugf( L"  UUU -- no token found" );
	BOut = BDefault;
	return 0;

	unguardSlow;
}

//-----------------------------------------------------------------------------

INT AUtil::ParseBool( FString& S, BYTE& BOut, BYTE BDefault )
{
	guardSlow(AUtil::ParseBool);

	FString STmp = TEXT("");

	//debugf( L"UUU ParseBool (%s)", *S );
	if( ParseToken( S, STmp ) )
	{
		if( ValidBoolString( STmp ) )
		{
			BOut = appAtoi( *STmp );
			//debugf( L"  UUU -- valid input: %d", BOut );
			return 1;
		}

		//debugf( L"  UUU -- warning invalid input" );
		return -1;
	}

	//debugf( L"  UUU -- no token found" );
	BOut = BDefault;
	return 0;

	unguardSlow;
}

//-----------------------------------------------------------------------------

INT AUtil::ParseVector( FString& S, FVector& VOut, FVector VDefault )
{
	guardSlow(AUtil::ParseVector);

	//debugf( L"UUU ParseVector (%s)", *S );
	if( S.Len() != 0 )
	{
		FLOAT F1, F2, F3;

		F1 = F2 = F3 = 0.0f;

		// mdf-tbd:
		if( ParseFloat( S, F1, VDefault.X ) == 1 && ParseFloat( S, F2, VDefault.Y ) == 1 && ParseFloat( S, F3, VDefault.Z ) == 1 )
		{
			VOut.X = F1;
			VOut.Y = F2;
			VOut.Z = F3;
			return 1;
		}
	
		//debugf( L"  UUU -- warning invalid input" );
		return -1;
	}

	//debugf( L"  UUU -- no token found" );
	VOut = VDefault;
	return 0;

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execGetFloatString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execGetFloatString);

	P_GET_FLOAT(F);
	P_GET_BYTE_OPTX(Precision,UCONST_DefaultGetFloatPrecision);
	P_FINISH;

	*(FString*)Result = GetFloatString( F, Precision );

	unguardSlow;
}

//------------------------------------------------------------------------------

void AUtil::execGetVectorString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execGetVectorString);

	P_GET_VECTOR(V);
	P_GET_BYTE_OPTX(Precision,UCONST_DefaultGetFloatPrecision);
	P_FINISH;

	*(FString*)Result = GetVectorString( V, Precision );

	unguardSlow;
}

//------------------------------------------------------------------------------

void AUtil::execGetRotatorString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execGetRotatorString);

	P_GET_ROTATOR(R);
	P_GET_BYTE_OPTX(Precision,UCONST_DefaultGetFloatPrecision);
	P_FINISH;

	*(FString*)Result = GetRotatorString( R, Precision );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execPadString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execPadString);

	P_GET_STR(S);
	P_GET_INT(PadLen);
	P_GET_UBOOL_OPTX(bRightJustify,false);
	P_GET_STR_OPTX(PadStr,TEXT(" "));
	P_FINISH;

	if( PadStr.Len() == 0 )
		PadStr = TEXT(" ");
	*(FString*)Result = PadString( S, PadLen, bRightJustify, PadStr );

	unguardSlow;
}

//-----------------------------------------------------------------------------
// Returns true if the given string ends in a decimal digit.

void AUtil::execEndsInDigit( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execEndsInDigit);

	P_GET_STR( Str );
	P_FINISH;

	*(UBOOL*)Result = EndsInDigit( Str );
	unguardSlow;
}

//-----------------------------------------------------------------------------
// Strips any trailing decimal digits from the end of Str.

void AUtil::execTruncateDigits( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execTruncateDigits);

	P_GET_STR_REF( Str );
	P_FINISH;

	TruncateDigits( *Str );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execStripSpaces( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execStripSpaces);

	P_GET_STR_REF( Str );
	P_GET_UBOOL( bStripLeading );
	P_GET_UBOOL( bStripTrailing );
	P_FINISH;

	StripSpaces( *Str, bStripLeading, bStripTrailing );

	unguardSlow;
}

//------------------------------------------------------------------------------

void AUtil::execValidNameString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execValidNameString);

	P_GET_STR( S );
	P_FINISH;

	*(UBOOL*)Result = ValidNameString( S );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execValidFloatString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execValidFloatString);

	P_GET_STR( S );
	P_FINISH;

	*(UBOOL*)Result = ValidFloatString( S );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execValidIntString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execValidIntString);

	P_GET_STR( S );
	P_FINISH;

	*(UBOOL*)Result = ValidIntString( S );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execValidByteString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execValidByteString);

	P_GET_STR( S );
	P_FINISH;

	*(UBOOL*)Result = ValidByteString( S );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execValidBoolString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execValidBoolString);

	P_GET_STR( S );
	P_FINISH;

	*(UBOOL*)Result = ValidBoolString( S );

	unguardSlow;
}

//------------------------------------------------------------------------------

void AUtil::execParseString( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execParseString);

	P_GET_STR_REF( S );
	P_GET_STR_REF( SOut );
	P_GET_STR_OPTX( SDefault, TEXT("") );
	P_FINISH;

	*(INT*)Result = ParseString( *S, *SOut, SDefault );

	unguardSlow;
}

//-----------------------------------------------------------------------------
// Use bAdd=true to force the name to be added to the name table if not already
// present.

void AUtil::execParseName( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execParseName);

	P_GET_STR_REF( S );
	P_GET_NAME_REF( NOut );
	P_GET_UBOOL_OPTX( bAdd, false )
	P_GET_NAME_OPTX( NDefault, NAME_None )
	P_FINISH;

	*(INT*)Result = ParseName( *S, *NOut, bAdd, NDefault );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execParseFloat( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execParseFloat);

    P_GET_STR_REF( S );
	P_GET_FLOAT_REF( FOut );
	P_GET_FLOAT_OPTX( FDefault, 0.0f )
	P_FINISH;

	*(INT*)Result = ParseFloat( *S, *FOut, FDefault );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execParseInt( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execParseInt);

    P_GET_STR_REF( S );
	P_GET_INT_REF( IOut );
	P_GET_INT_OPTX( IDefault, 0 )
	P_FINISH;

	*(INT*)Result = ParseInt( *S, *IOut, IDefault );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execParseByte( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execParseByte);

    P_GET_STR_REF( S );
	P_GET_BYTE_REF( BOut );
	P_GET_BYTE_OPTX( BDefault, 0 )
	P_FINISH;

	*(INT*)Result = ParseByte( *S, *BOut, BDefault );

	unguardSlow;
}

//-----------------------------------------------------------------------------
// Only accepts 0 or 1. Note: bools can't be out parameters so uses bytes.

void AUtil::execParseBool( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execParseBool);

    P_GET_STR_REF( S );
	P_GET_BYTE_REF( BOut );
	P_GET_BYTE_OPTX( BDefault, 0 )
	P_FINISH;

	*(INT*)Result = ParseBool( *S, *BOut, BDefault );

	unguardSlow;
}

//-----------------------------------------------------------------------------

void AUtil::execParseVector( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AUtil::execParseVector);

    P_GET_STR_REF( S );
	P_GET_VECTOR_REF( VOut );
	P_GET_VECTOR_OPTX( VDefault, FVector(0.0f,0.0f,0.0f));
	P_FINISH;

	*(INT*)Result = ParseVector( *S, *VOut, VDefault );

	unguardSlow;
}

//-----------------------------------------------------------------------------
// MISC
//-----------------------------------------------------------------------------

void AUtil::execPointPlaneDistance( FFrame& Stack, RESULT_DECL )
{
	guard(AUtil::execPointPlaneDistance)
	P_GET_VECTOR( Point );
	P_GET_VECTOR( PlaneBase );
	P_GET_VECTOR( PlaneNormal );
	P_FINISH;
	
	*(FLOAT*)Result = Abs( FPointPlaneDist( Point, PlaneBase, PlaneNormal ) );
	
	unguard;
}

//-----------------------------------------------------------------------------

void AUtil::execLineConvexPolygonIntersection( FFrame& Stack, RESULT_DECL )
{
	guard(AUtil::execLineConvexPolygonIntersection)

	P_GET_VECTOR( LineStart );
	P_GET_VECTOR( LineEnd );
	P_GET_DYNARRAY_REF(FVector,Points);
	P_GET_VECTOR_REF( IntersectionPoint );
	P_FINISH;

#ifdef DEBUGUTIL
	debugf( L"execLineConvexPolygonIntersection #Points: %d", Points->Num() );
	ADDLINE( LineStart, LineEnd, COLOR_YELLOW );
	for( INT ii=0; ii<Points->Num()-1; ii++ )
		ADDLINE( (*Points)(ii), (*Points)(ii+1), COLOR_BLUE );
	ADDLINE( (*Points)(Points->Num()-1), (*Points)(0), COLOR_BLUE );
#endif

	UBOOL bHit = FLineConvexPolygonIntersection( LineStart, LineEnd, *Points, *IntersectionPoint );

#ifdef DEBUGUTIL
	if( bHit )
	{
		debugf( L"  Hit!" );
		ADDCYLINDER( IntersectionPoint, 2, 2, COLOR_CYAN );
	}
	else
	{
		debugf( L"  Miss!" );
	}	
#endif

	*(UBOOL*)Result = bHit;

	unguard;
}

//-----------------------------------------------------------------------------

void AUtil::execLineBoxIntersection( FFrame& Stack, RESULT_DECL )
{
	guard(AUtil::execLineBoxIntersection)

	P_GET_VECTOR( LineStart );
	P_GET_VECTOR( LineEnd );
	P_GET_VECTOR( BoxMin );
	P_GET_VECTOR( BoxMax );
	P_FINISH;

	FBox Box( BoxMin, BoxMax );
	FVector	Direction = LineEnd - LineStart;
	FVector OneOverDirection = FVector( 1.0f / Direction.X, 1.0f / Direction.Y, 1.0f / Direction.Z );

	*(UBOOL*)Result = FLineBoxIntersection( Box, LineStart, LineEnd, Direction, OneOverDirection );

	unguard;
}

//-----------------------------------------------------------------------------
// TRACING RELATED NATIVES -- see Util.uc for descriptions.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Recursive minion to TraceRecursive -- don't call this directly -- call
// TraceRecursive instead.
// mdf-tbd: collapse this and DoRSLC 

static AActor* DoTraceRecursive
(	AActor&			Instance,
	FVector&		HitLocation,
	FVector&		HitNormal,
	const FVector&	StartLoc,
	FLOAT			TraceInterval,
	const FVector&	TraceDirection,
	FLOAT			TraceLimit,
	const FVector& 	Extent,
	DWORD			TraceFlags
)
{
	AActor* HitActor;
	FVector EndLoc;

	EndLoc = StartLoc + TraceDirection * TraceInterval;

	// trace
	FCheckResult Hit(1.0f);
	Instance.GetLevel()->SingleLineCheck( Hit, &Instance, EndLoc, StartLoc, TraceFlags, Extent );
	HitActor = Hit.Actor;
	HitLocation = Hit.Location;
	HitNormal = Hit.Normal;

	if( TraceLimit > 0.0f )
	{
		TraceLimit -= TraceInterval;
		if( TraceLimit <= 0.0f )
			return HitActor;	// stop whether we've found anything or not
	}

	if( !HitActor )
	{
		if( TraceInterval > TraceLimit )
			TraceInterval = TraceLimit;

		// we didn't hit anything -- continue tracing from where we stopped.
		HitActor = DoTraceRecursive( Instance, HitLocation, HitNormal, EndLoc, TraceInterval, TraceDirection, TraceLimit, Extent, TraceFlags );
	}

	return HitActor;
}

//-----------------------------------------------------------------------------

AActor* AUtil::TraceRecursive
(	AActor&			Instance,
	FVector&		HitLocation,
	FVector&		HitNormal,
	const FVector&	StartLoc,
	FLOAT			TraceInterval,
	UBOOL			bTraceActors,
	const FVector&	TraceDirection,
	FLOAT			TraceLimit,
	const FVector& 	Extent,
	DWORD			TraceFlags,
	UBOOL			bTraceVisibleNonColliding
)
{
	if( TraceLimit <= 0.0f )
		TraceLimit = DefaultTraceLimit;

	if( TraceInterval > TraceLimit )
		TraceInterval = TraceLimit;

	if( TraceInterval <= 0.0f )
		TraceInterval = DefaultTraceInterval;

	if( !TraceFlags )
	{
		if( bTraceActors )
			TraceFlags = TRACE_AllColliding | TRACE_ProjTargets;
		else 
			TraceFlags = TRACE_World;
	}
	if( bTraceVisibleNonColliding )
		TraceFlags |= TRACE_VisibleNonColliding;

	return DoTraceRecursive( Instance, HitLocation, HitNormal, StartLoc, TraceInterval, TraceDirection, TraceLimit, Extent, TraceFlags );
}

//-----------------------------------------------------------------------------

void AUtil::execTraceRecursive( FFrame& Stack, RESULT_DECL )
{
	guard(AUtil::execTraceRecursive);

	P_GET_ACTOR( Instance );
	P_GET_VECTOR_REF( HitLocation );
	P_GET_VECTOR_REF( HitNormal );
	P_GET_VECTOR( StartLoc );
	P_GET_UBOOL_OPTX( bTraceActors, false );
	P_GET_FLOAT_OPTX( TraceInterval, DefaultTraceInterval );
	P_GET_VECTOR_OPTX( TraceDirection, DefaultTraceDirection );
	P_GET_FLOAT_OPTX( TraceLimit, DefaultTraceLimit );
	P_GET_VECTOR_OPTX( Extent, FVector(0,0,0) );
	P_GET_INT_OPTX( TraceFlags, 0 );
	P_GET_UBOOL_OPTX( bTraceVisibleNonColliding, false );
	P_FINISH;

	*(AActor**)Result = TraceRecursive( *Instance, *HitLocation, *HitNormal, StartLoc, TraceInterval, bTraceActors, TraceDirection.SafeNormal(), TraceLimit, Extent, TraceFlags, bTraceVisibleNonColliding );

	unguard;
}

//-----------------------------------------------------------------------------
// Recursive minion to RSLC -- don't call this directly -- call RSLC instead.

static UBOOL DoRSLC
(
	FCheckResult&	Hit,
	AActor&			Source,
	const FVector&	Start,
	const FVector&	TraceDirection,
	FLOAT			TraceInterval,
	FLOAT			TraceLimit,
	DWORD			TraceFlags,
	const FVector&	Extent
)
{
	FVector End;
	UBOOL Result;

	End  = Start + TraceDirection * TraceInterval;

	// trace
	Hit = FCheckResult(1.0f);
	Result = Source.GetLevel()->SingleLineCheck( Hit, &Source, End, Start, TraceFlags, Extent );

	if( TraceLimit > 0.0f )
	{
		TraceLimit -= TraceInterval;
		if( TraceLimit <= 0.0f )
			return Result;	// Stop whether we've found anything or not.
	}

	if( Result )
	{
		if( TraceInterval > TraceLimit )
			TraceInterval = TraceLimit;

		// we didn't hit anything -- continue tracing from where we stopped.
		Result = DoRSLC( Hit, Source, End, TraceDirection, TraceInterval, TraceLimit, TraceFlags, Extent );
	}

	return Result;
}

//-----------------------------------------------------------------------------

UBOOL AUtil::RSLC
(
	FCheckResult&	Hit,
	AActor&			Source,
	const FVector&	Start,
	const FVector&	TraceDirection,
	FLOAT			TraceInterval,
	FLOAT			TraceLimit,
	DWORD			TraceFlags,
	const FVector&	Extent
)
{
	if( TraceLimit <= 0.0f )
		TraceLimit = DefaultTraceLimit;

	if( TraceInterval > TraceLimit )
		TraceInterval = TraceLimit;

	if( TraceInterval <= 0.0f )
		TraceInterval = DefaultTraceInterval;

	if( !TraceFlags )
		TraceFlags = TRACE_AllColliding;

	return DoRSLC( Hit, Source, Start, TraceDirection, TraceInterval, TraceLimit, TraceFlags, Extent );
}

/* See Util.uc
//-----------------------------------------------------------------------------

void AUtil::execRSLC( FFrame& Stack, RESULT_DECL )
{
	guard(AUtil::execRSLC);

	P_GET_STRUCT_REF( FCheckResult, Hit );
	P_GET_ACTOR( Source );
	P_GET_VECTOR( Start );
	P_GET_VECTOR_OPTX( TraceDirection, DefaultTraceDirection );
	P_GET_FLOAT_OPTX( TraceInterval, DefaultTraceInterval );
	P_GET_FLOAT_OPTX( TraceLimit, DefaultTraceLimit );
	P_GET_INT_OPTX( TraceFlags, TRACE_AllColliding );
	P_GET_VECTOR_OPTX( Extent, FVector(0,0,0) );
	P_FINISH;

	*(UBOOL*)Result = RSLC( *Hit, *Source, Start, TraceDirection.SafeNormal(), TraceInterval, TraceLimit, TraceFlags, Extent );

	unguard;
}
*/