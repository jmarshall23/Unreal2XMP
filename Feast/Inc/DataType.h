//=============================================================================
// DataType.h
//=============================================================================

TMap<DWORD,FString>& GetGStringMap() { static TMap<DWORD,FString> GStrMap = TMap<DWORD,FString>(); return GStrMap; }

// Rainbow class to store intermediate and final results of AST traversal
struct DataType
{
	enum
	{ 
		EBool,
		EInt,
		EFloat,
		EString,
		EObject,
		EComponent,
		ETag
	} Type;

	union
	{
		bool		boolVal;
		INT			intVal;
		FLOAT		floatVal;
		DWORD		stringVal;		// index into the string table
		UObject*	objectVal;
		UComponent*	componentVal;
		DWORD		tagVal;
	};

	DataType()				: Type(EBool),		boolVal(true)	{}
	DataType(bool b)		: Type(EBool),		boolVal(b)		{}
	DataType(INT i)			: Type(EInt),		intVal(i)		{}
	DataType(FLOAT f)		: Type(EFloat),		floatVal(f)		{}
	DataType(UObject* o)	: Type(EObject),	objectVal(o)	{}
	DataType(UComponent* c)	: Type(EComponent),	componentVal(c)	{}
	DataType(FName n)		: Type(ETag),		tagVal(n.GetIndex()) {} 
	DataType(FString s)		: Type(EString),	stringVal( FName( *s, FNAME_Add ).GetIndex() ) { GetGStringMap().Set( stringVal, *s ); }

	bool IsBool()		{ return Type == EBool;			}
	bool IsInt()		{ return Type == EInt;			}
	bool IsFloat()		{ return Type == EFloat;		}
	bool IsString()		{ return Type == EString;		}
	bool IsObject()		{ return Type == EObject;		}
	bool IsComponent()	{ return Type == EComponent;	}
	bool IsTag()		{ return Type == ETag;			}

	bool AsBool(bool& b);
	bool AsInt(INT& i);
	bool AsFloat(FLOAT& f);
	bool AsString(FString& s);
	bool AsObject(UObject*& o);
	bool AsComponent(UComponent*& c);
	bool AsTag(FName& n);
};


//
// DataType Implementation
//

static const TCHAR* DataTypeNames[] =
{
	TEXT("Bool"),
	TEXT("Int"),
	TEXT("Float"),
	TEXT("String"),
	TEXT("Object"),
	TEXT("Component")
};


bool DataType::AsBool(bool& b)
{
	guard(DataType::AsBool);
	switch( Type )
	{
	case EBool:
		b=boolVal;
		return true;
	default:
		return false;
	}
	unguard;
}


bool DataType::AsInt(INT& i)
{
	guard(DataType::AsInt);
	switch( Type )
	{
	case EInt:
		i = intVal;
		return true;
	case EFloat:
		i = (INT)floatVal;				// MIB FIX: might cause some odd inequality behavior (eg. 1.5 == 1.2 being true)
		return true;
	default:
		return false;
	}
	unguard;
}


bool DataType::AsFloat(FLOAT& f)
{
	guard(DataType::AsFloat);
	switch( Type )
	{
	case EInt:
		f = (FLOAT)intVal;
		return true;
	case EFloat:
		f=floatVal;
		return true;
	default:
		return false;
	}
	unguard;
}


bool DataType::AsString(FString& s)
{
	guard(DataType::AsString);
	switch( Type )
	{
	case EString:
		s = GetGStringMap().FindRef( stringVal );
		return true;
	case EBool:
		s = boolVal ? TEXT("True") : TEXT("False");
		return true;
	case EInt:
		s = FString::Printf(TEXT("%d"),intVal);
		return true;
	case EFloat:
		s = FString::Printf(TEXT("%f"),floatVal);
		return true;
	default:
		return false;
	}
	unguard;
}


bool DataType::AsObject(UObject*& o)
{
	guard(DataType::AsObject);

	switch( Type )
	{
	case EObject:
		o = objectVal;
		return true;
	default:
		return false;
	}

	unguard;
}


bool DataType::AsComponent(UComponent*& c)
{
	guard(DataType::AsComponent);

	switch( Type )
	{
	case EComponent:
		c = componentVal;
		return true;
	default:
		return false;
	}

	unguard;
}


bool DataType::AsTag(FName& n)
{
	guard(DataType::AsTag);
	FNameEntry* Entry = NULL;

	switch( Type )
	{
	case ETag:
	{
		Entry = FName::GetEntry(tagVal);
		if( Entry )	n = Entry->Name;
		return Entry != NULL;
	}
	default:
		return false;
	}

	unguard;
}

