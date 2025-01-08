/*=============================================================================
	SerializeUI.h: User Interface Serialization.
	Copyright 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron Leiby
=============================================================================*/

#define UI_PACKAGE_FILE_TAG			0x854930
#define UI_PACKAGE_FILE_VERSION		1

#define SERIALIZE_BOOL(b) \
	{ BYTE Value=b; Ar.SerializeBits(&Value,1); b=Value; } \

// Fix ARL: Make a checked version of this and check if existing uses should use that instead.
// It would be nice to be able to factor out the Ar.IsSaving/Ar.IsLoading checks across multiple uses.
#define SERIALIZE_INDEXED_COMPONENT(type,c) \
	INT c##Index; \
	if( Ar.IsSaving() ) \
		c##Index = GetComponentIndex(c); \
	Ar << AR_INDEX(c##Index); \
	if( Ar.IsLoading() ) \
		c = (c##Index>=0) ? DynamicCastChecked<type>(Components(c##Index)) : NULL; \

#define SERIALIZE_PATHED_COMPONENT(type,c) \
	struct c##Fixup{ static void UI_CALLBACK Fixup( FArchive& Ar, void* _owner ){ \
		guard(c##Fixup); \
		type* owner = (type*)_owner; \
		uiPath Path; \
		if( Ar.IsSaving() ){ \
			if(!owner->c) appErrorf(TEXT("Attempt to serialize NULL pathed component!")); \
			Path = owner->c->GetFullPath(); } \
		Ar << Path; \
		if( Ar.IsLoading() ){ \
			owner->c = GetGConsole()->Root->GetComponent(Path); \
			if(!owner->c) appErrorf(TEXT("Component not found! (%s)"),*GetPathString(Path)); } \
		unguard }}; \
	((FArchiveUI&)Ar).NeedsFixup(this,c##Fixup::Fixup); \

#define SERIALIZE_MOUSE(type) \
	struct type##MouseFixup{ static void UI_CALLBACK Fixup( FArchive& Ar, void* _owner ){ \
		guard(type##MouseFixup); \
		type* owner = (type*)_owner; \
		INT MouseIndex; \
		if( Ar.IsSaving() ) \
			MouseIndex = owner->Mouse ? owner->Mouse->GetMouseIndex() : -1; \
		Ar << AR_INDEX(MouseIndex); \
		if( Ar.IsLoading() ) \
			owner->Mouse = (MouseIndex>=0) ? GetGMouseList()(MouseIndex) : NULL; \
		unguard }}; \
	((FArchiveUI&)Ar).NeedsFixup(this,type##MouseFixup::Fixup); \

typedef TMap<uiClass*,BYTE> ClassTagMap;
typedef TMap<BYTE,uiClass*> TagClassMap;


/*----------------------------------------------------------------------------
	FArchiveUI.
----------------------------------------------------------------------------*/

class FArchiveUI : public FArchive
{
public:
	virtual FArchiveUI& operator<<( class UComponent*& Res )
	{
		return *this;
	}

private:
	typedef void(UI_CALLBACK *FixupFunc)(FArchive& Ar,void*);
	struct TFixupCallback
	{
		void* Target;
		FixupFunc Function;
		TFixupCallback( void* _Target, FixupFunc _Function ): Target(_Target), Function(_Function){}
		void Exec( FArchive& Ar ){ Function(Ar,Target); }
	};

	TArray<TFixupCallback> FixupList;

public:
	void NeedsFixup( void* Target, FixupFunc Function )
	{
		new(FixupList)TFixupCallback(Target,Function);
	}
	void Fixup();

public:
	// Friend archivers.
	friend FArchiveUI& operator<<( FArchiveUI& Ar, ANSICHAR& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, BYTE& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, SBYTE& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, _WORD& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, SWORD& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, DWORD& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, INT& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, FLOAT& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, DOUBLE& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI &Ar, QWORD& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
	friend FArchiveUI& operator<<( FArchiveUI& Ar, SQWORD& A )
		{ return (FArchiveUI&)((FArchive&)Ar << A); }
};


/*----------------------------------------------------------------------------
	LoadUI.
----------------------------------------------------------------------------*/

class LoadUI : public FArchiveUI
{
public:
	LoadUI( const TCHAR* InFilename );
	virtual ~LoadUI();

	FArchiveUI& operator<<( UComponent* &C );
	FArchive& operator<<( class UObject*& Res );

private:
	FArchive* Loader;
	TagClassMap TagMap;

	UComponent* CreateComponentFromTag( BYTE Tag );

	// FArchive interface.
	void Seek( INT InPos );
	INT Tell();
	INT TotalSize();
	void Serialize( void* V, INT Length );
};

/*----------------------------------------------------------------------------
	SaveUI.
----------------------------------------------------------------------------*/

class SaveUI : public FArchiveUI
{
public:
	SaveUI( const TCHAR* InFilename );
	virtual ~SaveUI();

	FArchiveUI& operator<<( UComponent* &C );
	FArchive& operator<<( class UObject*& Res );

private:
	FArchive* Saver;
	ClassTagMap ClassMap;

	BYTE GetTagFromComponent( UComponent* C );

	// FArchive interface.
	void Seek( INT InPos );
	INT Tell();
	void Serialize( void* V, INT Length );
};


