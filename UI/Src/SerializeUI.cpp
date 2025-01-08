/*=============================================================================
	SerializeUI.cpp: User Interface Serialization.
	Copyright 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron Leiby
=============================================================================*/

#include "UI.h"
#include "ImportUI.h"

FArchive& operator<<( FArchive& Ar, NotifyList& N )
{
	Ar << *(DWORD*)&N;
	if( N )
	{
		if( Ar.IsLoading() )
			N = new NotifyListType();
		Ar << *N;
	}
	return Ar;
}

FArchive& operator<<( FArchive& Ar, FImage* &I )
{
	Ar << *(DWORD*)&I;
	if( I )
	{
		if( Ar.IsLoading() )
			I = new FImage();
		Ar << *I;
	}
	return Ar;
}


/*----------------------------------------------------------------------------
	FArchiveUI.
----------------------------------------------------------------------------*/

struct TPair
{
	FString Key;
	uiPath Value;

	TPair(){}
	TPair(FString _Key,uiPath _Value): Key(_Key),Value(_Value){}

	friend FArchive& operator<<( FArchive& Ar, TPair& P )
		{ return Ar << P.Key << P.Value; }
};

void FArchiveUI::Fixup()
{
	guard(FArchiveUI::Fixup);

	for( INT i=0; i<FixupList.Num(); i++ )
		FixupList(i).Exec(*this);

	TArray<TPair> Pairs;
	if( IsSaving() )
		for(THashTable<FString,UComponent*>::TIterator It(GetEventScopeTable());It;++It)
			new(Pairs)TPair(It.Key(),It.Value()->GetFullPath());
	*this << Pairs;
	if( IsLoading() )
	{
		GetEventScopeTable().Empty();
		for(INT i=0;i<Pairs.Num();i++)
		{
			UComponent* C = GetGConsole()->Root->GetComponent(Pairs(i).Value);
			if(!C) appErrorf(TEXT("Component not found! (%s)"),*GetPathString(Pairs(i).Value));
			GetEventScopeTable().Set(Pairs(i).Key,C);
		}
	}

//ARL	*this << GetObjMap();

	unguard;
}


/*----------------------------------------------------------------------------
	LoadUI.
----------------------------------------------------------------------------*/

LoadUI::LoadUI( const TCHAR* InFilename )
{
	guard(LoadUI::LoadUI);

	Loader = GFileManager->CreateFileReader( InFilename, 0, GError );
	if( !Loader )
		appThrowf( LocalizeError(TEXT("OpenFailed"),TEXT("Core")) );

	// Set status info.
	ArVer       = UI_PACKAGE_FILE_VERSION;
	ArIsLoading = ArIsPersistent = 1;
//	ArForEdit   = GIsEditor;
//	ArForClient = 1;
//	ArForServer = 1;

	// Read summary from file.
	DWORD PackageTag = UI_PACKAGE_FILE_TAG;
	*this << PackageTag;

	// Check tag.
	if( PackageTag != UI_PACKAGE_FILE_TAG )
	{
		GWarn->Logf( LocalizeError(TEXT("BinaryFormat"),TEXT("Core")), InFilename );
		throw( LocalizeError(TEXT("Aborted"),TEXT("Core")) );
	}

	// Get package versioning.
	*this << ArVer;

	// Load in name table.
	FNameTable::GetInstance()->Serialize(*this);

	// Load in event table.
	*this << GetuiEventTable();

	// Create helpers.
	INT NumHelpers;
	*this << AR_INDEX(NumHelpers);
	for(INT i=0;i<NumHelpers;i++)
	{
		FString HelperName;
		*this << HelperName;

		UClass* Class = Cast<UClass>(UObject::StaticLoadObject( UClass::StaticClass(), NULL, *HelperName, NULL, LOAD_NoWarn, NULL ));
		if(!Class) appErrorf(TEXT("Class not found: %s"), *HelperName );

		UUIHelper* Helper = CastChecked<UUIHelper>(UObject::StaticConstructObject(Class,GetGConsole()));
		if(!Helper) appErrorf(TEXT("Unable to create component: %s"), Class->GetFullName() );
		Helper->Constructed();

		NOTE(debugf(TEXT("Created helper: %s"),Helper->GetFullName()));

		GetGConsole()->Helpers.AddItem(Helper);
	}

	// Load in class tags.
	TMap<BYTE,FString> ClassTags;
	*this << ClassTags;
	for(TMap<BYTE,FString>::TIterator It(ClassTags);It;++It)
	{
		uiClass* C = StringFindClass(*It.Value(),1);
		TagMap.Set(It.Key(),C);
	}

	unguard;
}

LoadUI::~LoadUI()
{
	guard(LoadUI::~LoadUI);
	if( Loader )
		delete Loader;
	Loader = NULL;
	unguard;
}

UComponent* LoadUI::CreateComponentFromTag( BYTE Tag )
{
	guard(LoadUI::CreateComponentFromTag);
	uiClass* Class = TagMap.FindRef(Tag);

	if( Class==NULL )
		appErrorf(TEXT("Error loading component!  Invalid Tag '%d'"),(DWORD)Tag);

	return Class->CreateInstanceNoInit();
	unguard;
}

FArchiveUI& LoadUI::operator<<( UComponent* &C )
{
	guard(LoadUI<<UComponent);
	BYTE Tag;
	*this << Tag;

	C = CreateComponentFromTag(Tag);
	C->Serialize(*this);

	return *this;
	unguard;
}

FArchive& LoadUI::operator<<( class UObject*& Res )
{
	guard(LoadUI<<UObject);
	*this << *(DWORD*)&Res;
	if( Res )
	{
		FString PathName;
		*this << PathName;
		Res = UObject::StaticFindObject( NULL, ANY_PACKAGE, *PathName );
		if(!Res) Res = UObject::StaticLoadObject( UObject::StaticClass(), NULL, *PathName, NULL, LOAD_NoWarn, NULL );
		if(!Res) appErrorf(TEXT("Object not found! '%s'"),*PathName);
	}
	return *this;
	unguard;
}

// FArchive interface.
void LoadUI::Seek( INT InPos )
{
	guard(LoadUI::Seek);
	Loader->Seek( InPos );
	unguard;
}

INT LoadUI::Tell()
{
	guard(LoadUI::Tell);
	return Loader->Tell();
	unguard;
}

INT LoadUI::TotalSize()
{
	guard(LoadUI::TotalSize);
	return Loader->TotalSize();
	unguard;
}

void LoadUI::Serialize( void* V, INT Length )
{
	guard(LoadUI::Serialize);
	Loader->Serialize( V, Length );
	unguard;
}

/*----------------------------------------------------------------------------
	SaveUI.
----------------------------------------------------------------------------*/

SaveUI::SaveUI( const TCHAR* InFilename )
{
	// Create file saver.
	Saver = GFileManager->CreateFileWriter( InFilename, 0, GThrow );
	if( !Saver )
		appThrowf( LocalizeError(TEXT("SaveFailed"),TEXT("Core")) );

	// Set main summary info.
	DWORD PackageTag = UI_PACKAGE_FILE_TAG;
	*this << PackageTag;

	// Set status info.
	ArVer			= UI_PACKAGE_FILE_VERSION;
	ArIsSaving		= 1;
	ArIsPersistent	= 1;
//	ArForEdit		= GIsEditor;
//	ArForClient		= 1;
//	ArForServer		= 1;

	// Set package version.
	*this << ArVer;

	// Save out name table.
	FNameTable::GetInstance()->Serialize(*this);

	// Save out event table.
	*this << GetuiEventTable();

	// Save helpers.
	INT NumHelpers = GetGConsole()->Helpers.Num();
	*this << AR_INDEX(NumHelpers);
	for(INT i=0;i<NumHelpers;i++)
	{
		FString HelperName = GetGConsole()->Helpers(i)->GetClass()->GetPathName();
		*this << HelperName;
	}

	// Save out class tags.
	BYTE Tag=1;	//reserve tag=0 for errors.
	TMap<BYTE,FString> ClassTags;
	for(uiClass* C=uiClass::GetClassList();C;C=C->NextClass())
	{
		ClassMap.Set(C,Tag);
		ClassTags.Set(Tag,C->GetName());
		if(++Tag==0) appErrorf(TEXT("Error saving class tags: Number of classes exceeds 255 limit!"));
	}
	*this << ClassTags;
}

SaveUI::~SaveUI()
{
	guard(SaveUI::~SaveUI);
	if( Saver )
		delete Saver;
	Saver = NULL;
	unguard;
}

BYTE SaveUI::GetTagFromComponent( UComponent* C )
{
	guard(SaveUI::GetTagFromComponent);
	BYTE Tag = ClassMap.FindRef(C->GetClass());

	if( Tag==0 )
		appErrorf(TEXT("Error saving component!  Class tag not registered for '%s'"),C->GetClass()->GetName());

	return Tag;
	unguard;
}

FArchiveUI& SaveUI::operator<<( UComponent* &C )
{
	guard(SaveUI<<UComponent);
	if( C==NULL )
		appErrorf(TEXT("Attempt to serialize NULL component!"));
	if( C->IsTransient() )
		appErrorf(TEXT("Attempt to serialize Transient component! (%s)"),*C->GetFullName());

	BYTE Tag = GetTagFromComponent(C);
	*this << Tag;

	C->Serialize(*this);

	return *this;
	unguard;
}

FArchive& SaveUI::operator<<( class UObject*& Res )
{
	guard(SaveUI<<UObject);
	*this << *(DWORD*)&Res;
	if( Res )
	{
		FString PathName = Res->GetPathName();
		*this << PathName;
	}
	return *this;
	unguard;
}

// FArchive interface.
void SaveUI::Seek( INT InPos )
{
	Saver->Seek( InPos );
}

INT SaveUI::Tell()
{
	return Saver->Tell();
}

void SaveUI::Serialize( void* V, INT Length )
{
	Saver->Serialize( V, Length );
}

