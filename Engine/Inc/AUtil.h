#if 1 //NEW
static FString GetFloatString( FLOAT F, BYTE Precision );
static FString GetVectorString( const FVector& V, BYTE Precision );
static FString GetRotatorString( const FRotator& R, BYTE Precision );
static FString PadString( const FString& InStr, INT PadLen, UBOOL bRightJustify, const FString& PadChar );
static UBOOL EndsInDigit( const FString& Str );
static void TruncateDigits( FString& Str );
static void StripSpaces( TCHAR* Text, UBOOL bStripLeading, UBOOL bStripTrailing );
static void StripSpaces( FString& Str, UBOOL bStripLeading, UBOOL bStripTrailing );
static UBOOL ParseToken( FString& S, FString& SOut );
static UBOOL ValidNameString( const FString& S );
static UBOOL ValidFloatString( const FString& S );
static UBOOL ValidIntString( const FString& S );
static UBOOL ValidByteString( const FString& S );
static UBOOL ValidBoolString( const FString& S );
static INT ParseString( FString& S, FString& SOut, FString SDefault );
static INT ParseName( FString& S, FName& NOut, UBOOL bAdd, FName NDefault );
static INT ParseFloat( FString& S, FLOAT& FOut, FLOAT FDefault );
static INT ParseInt( FString& S, INT& IOut, INT IDefault );
static INT ParseByte( FString& S, BYTE& BOut, BYTE BDefault );
static INT ParseBool( FString& S, BYTE& BOut, BYTE BDefault );
static INT ParseVector( FString& S, FVector& VOut, FVector VDefault );

static AActor* AUtil::TraceRecursive
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
);

static UBOOL AUtil::RSLC
(
	FCheckResult&	Hit,
	AActor&			Source,
	const FVector&	Start,
	const FVector&	TraceDirection,
	FLOAT			TraceInterval,
	FLOAT			TraceLimit,
	DWORD			TraceFlags,
	const FVector&	Extent
);

#endif
