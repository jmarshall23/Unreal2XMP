/*=============================================================================
//	UDumpIntCommandlet.cpp: Imports/Merges/Exports INTs for specified packages.
//
//	Copyright 2003 Epic Games. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "../../Core/Inc/FConfigCacheIni.h"

/*-----------------------------------------------------------------------------
 Internal helper functions.
-----------------------------------------------------------------------------*/

static INT GPropertyCount;

static INT Compare( FString& A, FString& B )
{
	return appStricmp( *A, *B );
}

static INT Compare (const UObject *p1, const UObject *p2)
{
	int rc;

	rc = appStrcmp (p1->GetClass()->GetName(), p2->GetClass()->GetName());

	if (rc != 0)
		return (rc);

	return (appStrcmp (p1->GetName(), p2->GetName()));
}

static void IntExportStruct( UClass* Class, UClass* SuperClass, UClass* OuterClass, UStruct* Struct, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, BYTE* DataBase, INT DataOffset, bool AtRoot = false );

#define PROPTEXTSIZE 65536

static void IntExportProp( UClass* Class, UClass* SuperClass, UClass* OuterClass, UProperty* Prop, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, BYTE* DataBase, INT DataOffset )
{
	UStructProperty* StructProperty = Cast<UStructProperty>( Prop );

	if( StructProperty )
	{
		IntExportStruct( Class, SuperClass, OuterClass, StructProperty->Struct, IntName, SectionName, KeyPrefix, DataBase, DataOffset );
		return;
	}

	if( !(Prop->PropertyFlags & CPF_Localized) ) 
		return;

	TArray<TCHAR> RealValueTemp(PROPTEXTSIZE);
	TArray<TCHAR> DefaultValueTemp(PROPTEXTSIZE);
	TCHAR* RealValue = &RealValueTemp(0);
	TCHAR* DefaultValue = &DefaultValueTemp(0);

	Prop->ExportTextItem( RealValue, DataBase + DataOffset, NULL, PPF_Delimited );
	INT RealLength = appStrlen( RealValue );
	check( RealLength < PROPTEXTSIZE );

	if( ( RealLength == 0 ) || !appStrcmp( RealValue, TEXT("\"\"") ) )
		return;

	if( Class && SuperClass && OuterClass && (OuterClass != Class) )
	{
		// Only export if value has changed from base class:


		BYTE* DefaultDataBase = (BYTE*)&SuperClass->Defaults(0);

		Prop->ExportTextItem( DefaultValue, DefaultDataBase + DataOffset, NULL, PPF_Delimited );
		check( appStrlen( DefaultValue ) < PROPTEXTSIZE );

		if( appStrcmp( DefaultValue, RealValue ) == 0 )
			return;
	}

	// if it's an instance, check it 
	if( Class && SuperClass && OuterClass && (OuterClass != Class) )
	{
		// Only export if value has changed from base class:

		BYTE* DefaultDataBase = (BYTE*)&SuperClass->Defaults(0);

		Prop->ExportTextItem( DefaultValue, DefaultDataBase + DataOffset, NULL, PPF_Delimited );
		check( appStrlen( DefaultValue ) < PROPTEXTSIZE );

		if( appStrcmp( DefaultValue, RealValue ) == 0 )
			return;
	}

	GConfig->SetString( SectionName, KeyPrefix, RealValue, IntName );
	GPropertyCount++;
}

static void IntExportStruct( UClass* Class, UClass* SuperClass, UClass* OuterClass, UStruct* Struct, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, BYTE* DataBase, INT DataOffset, bool AtRoot )
{
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

			INT NewOffset = DataOffset + (Prop->Offset) + (i * Prop->ElementSize );

			IntExportProp( Class, SuperClass, AtRoot ? CastChecked<UClass>(Prop->GetOuter()) : OuterClass, Prop, IntName, SectionName, *NewPrefix, DataBase, NewOffset );
		}
	}
}

static UBOOL IntExport (UObject *Package, const TCHAR *IntName, UBOOL ExportFresh, UBOOL ExportInstances)
{
	TArray<UObject *> Objects;
	INT objectNumber;

	// Flush/empty any existing file so as to not confuse the GConfig object.

	if( ExportFresh )
	{
		GConfig->UnloadFile( IntName );

		if (GFileManager->FileSize (IntName) >= 0)
			GFileManager->Delete (IntName, 0, 1);
	}

	for( FObjectIterator It; It; ++It )
	{
		UObject *Obj = *It;

		if( !Obj->IsIn(Package) )
			continue;

		if( Obj->GetFlags() & (RF_Transient | RF_NotForClient | RF_NotForServer | RF_Destroyed) )
			continue;

		Objects.AddItem (Obj);
	}

	if( Objects.Num() )
		Sort (&Objects(0), Objects.Num());

	GPropertyCount = 0;

	for (objectNumber = 0; objectNumber < Objects.Num(); objectNumber++)
	{
		UClass* Class = Cast<UClass>( Objects(objectNumber) );

		if( Class && (Class->ClassFlags & CLASS_Localized) )
		{
			GWarn->Logf(TEXT("Exporting class %s"), Class->GetName() );
			IntExportStruct( Class, Class->GetSuperClass(), Class, Class, IntName, Class->GetName(), NULL, &Class->Defaults(0), 0, true );
		}
		else if( Objects(objectNumber)->GetClass()->ClassFlags & CLASS_Localized )
		{
			UClass* SubObjectOuter = Cast<UClass>(Objects(objectNumber)->GetOuter());
			if( SubObjectOuter )	// if it's a subobject, export as [OuterClass] SubObjectName.Key=Value
			{
				// Find the Class property which references this subobject
				if( Objects(objectNumber)->GetFlags()&RF_PerObjectLocalized )
					IntExportStruct( NULL, NULL, NULL, Objects(objectNumber)->GetClass(), IntName, Objects(objectNumber)->GetOuter()->GetName(), Objects(objectNumber)->GetName(), (BYTE*)(Objects(objectNumber)), 0, true );
			}
			else
				if( ExportInstances )
				{
					IntExportStruct( NULL, NULL, NULL, Objects(objectNumber)->GetClass(), IntName, Objects(objectNumber)->GetName(), NULL, (BYTE*)(Objects(objectNumber)), 0, true );
				}
		}
	}

	GConfig->Flush( 0, IntName );

	GWarn->Logf( NAME_Log, TEXT("Exported %d properties."), GPropertyCount );

	return 1;
}

static void IntGetNameFromPackageName (const FString &PackageName, FString &IntName)
{
	INT i;

	IntName = PackageName;

	i = IntName.InStr (TEXT ("."), 1);

	if (i >= 0)
		IntName = IntName.Left (i);

	IntName += TEXT (".int");

	i = IntName.InStr (TEXT ("/"), 1);

	if (i >= 0)
		IntName = IntName.Right (IntName.Len () - i - 1);

	i = IntName.InStr (TEXT ("\\"), 1);

	if (i >= 0)
		IntName = IntName.Right (IntName.Len () - i - 1);

	IntName = FString (appBaseDir()) + IntName;
}

static FString GetDirName( const FString &Path )
{
    INT chopPoint;

    chopPoint = Max (Path.InStr( TEXT("/"), 1 ) + 1, Path.InStr( TEXT("\\"), 1 ) + 1);

    if (chopPoint < 0)
        chopPoint = Path.InStr( TEXT("*"), 1 );

    if (chopPoint < 0)
        return (TEXT(""));

    return (Path.Left( chopPoint ) );
}

/*-----------------------------------------------------------------------------
	UDumpIntCommandlet.
-----------------------------------------------------------------------------*/

class UDumpIntCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDumpIntCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UDumpIntCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 1;

		unguard;
	}

	INT Main( const TCHAR *Parms )
	{
		guard(UDumpIntCommandlet::Main);

    	FString PackageWildcard;
        INT Count = 0;

		UClass* EditorEngineClass = UObject::StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail | LOAD_DisallowFiles, NULL );
		GEditor  = ConstructObject<UEditorEngine>( EditorEngineClass );
		GEditor->UseSound = 0;
        GEditor->InitEditor();
		GIsRequestingExit = 1; // Causes ctrl-c to immediately exit.

        while( ParseToken(Parms, PackageWildcard, 0) )
        {
            TArray<FString> FilesInPath;
            TArray <UObject*> Packages;
            FString PathPrefix;
            INT i;

            PathPrefix = GetDirName( PackageWildcard );

		    FilesInPath = GFileManager->FindFiles( *PackageWildcard, 1, 0 );

            if( !FilesInPath.Num() )
            {
                GWarn->Logf( NAME_Error, TEXT("No packages found matching %s!"), *PackageWildcard );
                continue;
            }

            Sort( &FilesInPath(0), FilesInPath.Num() );

		    for( i = 0; i < FilesInPath.Num(); i++ )
            {
                FString PackageName = FilesInPath(i);
                FString FileName = PathPrefix + FilesInPath(i);
                FString IntName;

                GWarn->Logf (NAME_Log, TEXT("Loading %s..."), *PackageName );
                UPackage* Package = CastChecked<UPackage>( LoadPackage( NULL, *FileName, LOAD_NoWarn ) );

                IntGetNameFromPackageName ( PackageName, IntName );

                IntExport( Package, *IntName, true, PackageName.Right(2).Caps() != TEXT(".u") );

                UObject::CollectGarbage( RF_Native );

                Count++;
            }
        }

        if( !Count )
            GWarn->Log( NAME_Error, TEXT("Syntax: ucc DumpInt <file[s]>") );

		GIsRequestingExit=1;
		return 0;

		unguard;
	}
};
IMPLEMENT_CLASS(UDumpIntCommandlet)

/*-----------------------------------------------------------------------------
	UCompareIntCommandlet.
-----------------------------------------------------------------------------*/

class UCompareIntCommandlet : public UCommandlet
{
	DECLARE_CLASS(UCompareIntCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UCompareIntCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 1;

		unguard;
	}

	INT Main( const TCHAR *Parms )
	{
		guard(UCompareIntCommandlet::Main);

		UClass* EditorEngineClass = UObject::StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail | LOAD_DisallowFiles, NULL );
		GEditor  = ConstructObject<UEditorEngine>( EditorEngineClass );
		GEditor->UseSound = 0;
        GEditor->InitEditor();
		GIsRequestingExit = 1; // Causes ctrl-c to immediately exit.

		FString Wildcard;

		if( !ParseToken(Parms, Wildcard, 0) )
			appErrorf(TEXT("Example: uuc compareint *.frt"));
		TArray<FString> ForeignFiles = GFileManager->FindFiles( *Wildcard, 1, 0 );
        if( !ForeignFiles.Num() )
            appErrorf( TEXT("No files matching %s found"), *Wildcard );

		for( INT i=0;i<ForeignFiles.Num();i++ )
		{
			UBOOL NewFile = 1;
			INT d = ForeignFiles(i).InStr(TEXT("."), 1);
			FString IntFile = ForeignFiles(i).Left(d) + TEXT(".int");

			FConfigFile* IntSections = ((FConfigCacheIni*)(GConfig))->Find( *IntFile, 0 );
			if( !IntSections )
				continue;
			FConfigFile* LocSections = ((FConfigCacheIni*)(GConfig))->Find( *ForeignFiles(i), 0 );
			check( LocSections );

			FString LastSection;
			for( TMap<FString,FConfigSection>::TIterator It(*IntSections); It; ++It )
			{
				for( TMultiMap<FString,FString>::TIterator It2(It.Value()); It2; ++It2 )
				{
					FString Section = It.Key();
					FString Key		= It2.Key();
					FString Value	= It2.Value();
					FConfigSection* LocSec;
					if( (LocSec=LocSections->Find(*Section))==NULL || LocSec->Find(*Key)==NULL )
					{
						if( NewFile )
						{
							GWarn->Logf(TEXT("------------------------------- %s -------------------------------"), *ForeignFiles(i) );
							NewFile = 0;
						}
						if( It.Key() != LastSection )
						{
							GWarn->Logf(TEXT("\n[%s]"), *Section );
							LastSection = Section;
						}
						GWarn->Logf(TEXT("%s=\"%s\""), *Key, *Value );
					}
				}                				
			}
			GConfig->UnloadFile( *ForeignFiles(i) );
			GConfig->UnloadFile( *IntFile );
		}

		return 0;

		unguard;
	}
};
IMPLEMENT_CLASS(UCompareIntCommandlet)


/*-----------------------------------------------------------------------------
	UMergeIntCommandlet.
-----------------------------------------------------------------------------*/

class UMergeIntCommandlet : public UCommandlet
{
	DECLARE_CLASS(UMergeIntCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UMergeIntCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 1;

		unguard;
	}

	INT Main( const TCHAR *Parms )
	{
		guard(UMergeIntCommandlet::Main);

		UClass* EditorEngineClass = UObject::StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail | LOAD_DisallowFiles, NULL );
		GEditor  = ConstructObject<UEditorEngine>( EditorEngineClass );
		GEditor->UseSound = 0;
        GEditor->InitEditor();
		GIsRequestingExit = 1; // Causes ctrl-c to immediately exit.

		FString DiffFile, IntExt;

		if( !ParseToken(Parms, DiffFile, 0) )
			appErrorf(TEXT("You must specify a change file"));
		if( !ParseToken(Parms, IntExt, 0) )
			appErrorf(TEXT("You must specify an intfile extension"));
		FString DiffText;
		if( GFileManager->FileSize(*DiffFile) < 0 || !appLoadFileToString( DiffText, *DiffFile ) )
			appErrorf(TEXT("Could not open %s"),*DiffFile);

		const TCHAR* Ptr = *DiffText;
		TMap<FString,FString> FileMap;
		FString StrLine;
		FString CurrentFile;
		while(ParseLine(&Ptr,StrLine))
		{
			if( StrLine.Left(5) == TEXT("-----") )
			{
				CurrentFile = StrLine.Mid( StrLine.InStr(TEXT(" ")) + 1 );
				CurrentFile = CurrentFile.Left( CurrentFile.InStr(TEXT(".")) );
			}
			else
			{
				FString* ExistingStr = FileMap.Find( *CurrentFile );
				if( ExistingStr )
					FileMap.Set( *CurrentFile, *(*ExistingStr + TEXT("\r\n") + StrLine) );
				else
					FileMap.Set( *CurrentFile, *StrLine );
			}		
		}

		for( TMultiMap<FString,FString>::TIterator It(FileMap); It; ++It )
		{
			appSaveStringToFile( It.Value(), *(It.Key()+TEXT(".temp")) );

			FConfigFile* NewSections = ((FConfigCacheIni*)(GConfig))->Find( *(It.Key()+TEXT(".temp")), 0 );
			check( NewSections );
			
			for( TMap<FString,FConfigSection>::TIterator It2(*NewSections); It2; ++It2 )
			{
				for( TMultiMap<FString,FString>::TIterator It3(It2.Value()); It3; ++It3 )
				{
					FConfigFile* ExistingFile = ((FConfigCacheIni*)(GConfig))->Find( *(It.Key()+TEXT(".")+IntExt), 0 );
					ExistingFile->Quotes = 1;

					GWarn->Logf(TEXT("%s.%s: [%s] %s=\"%s\""), *It.Key(), *IntExt, *It2.Key(), *It3.Key(), *It3.Value() );
					GConfig->SetString( *It2.Key(), *It3.Key(), *It3.Value(), *(It.Key()+TEXT(".")+IntExt));
				}
			}
		}

		for( TMultiMap<FString,FString>::TIterator It(FileMap); It; ++It )
			GFileManager->Delete(*(It.Key()+TEXT(".temp")));

		return 0;

		unguard;
	}
};
IMPLEMENT_CLASS(UMergeIntCommandlet)

/*-----------------------------------------------------------------------------
	URearrangeIntCommandlet.
-----------------------------------------------------------------------------*/

class URearrangeIntCommandlet : public UCommandlet
{
	DECLARE_CLASS(URearrangeIntCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(URearrangeIntCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 1;

		unguard;
	}

	INT Main( const TCHAR *Parms )
	{
		guard(URearrangeIntCommandlet::Main);

		UClass* EditorEngineClass = UObject::StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail | LOAD_DisallowFiles, NULL );
		GEditor  = ConstructObject<UEditorEngine>( EditorEngineClass );
		GEditor->UseSound = 0;
        GEditor->InitEditor();
		GIsRequestingExit = 1; // Causes ctrl-c to immediately exit.

		FString NewInt, OldInt;

		if( !ParseToken(Parms, NewInt, 0) )
			appErrorf(TEXT("Example: uuc RearrangeInt newint.frt oldint.frt"));

		if( !ParseToken(Parms, OldInt, 0) )
			appErrorf(TEXT("Example: uuc RearrangeInt newint.frt oldint.frt"));

		FConfigFile* IntSections = ((FConfigCacheIni*)(GConfig))->Find( *OldInt, 0 );
		check( IntSections );

		for( TMap<FString,FConfigSection>::TIterator It(*IntSections); It; ++It )
		{
			for( TMultiMap<FString,FString>::TIterator It2(It.Value()); It2; ++It2 )
			{
				FString Section = It.Key();
				FString Key		= It2.Key();
				FString Value	= It2.Value();

				FString TempStr;
				if( GConfig->GetString( *Section, *Key, TempStr, *NewInt ) )
					GConfig->SetString( *Section, *Key, *Value, *NewInt );
				else
				{
					// new subobject format
					INT i = Section.InStr(TEXT("."));
					if( i != -1 )
					{
						Key = Section.Mid(i+1) + TEXT(".") + Key;
						Section = Section.Left(i);
						if( GConfig->GetString( *Section, *Key, TempStr, *NewInt ) )
							GConfig->SetString( *Section, *Key, *Value, *NewInt );
					}
				}
			}
		}

		return 0;
		unguard;
	}
};
IMPLEMENT_CLASS(URearrangeIntCommandlet);

