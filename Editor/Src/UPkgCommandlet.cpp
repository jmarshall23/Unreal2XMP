/*=============================================================================
	UPkgCommandlet.cpp: Imports/Exports data to/from packages.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Warren Marshall.
=============================================================================*/

#include "EditorPrivate.h"

/*-----------------------------------------------------------------------------
	UPkgCommandlet.
-----------------------------------------------------------------------------*/

enum eTYPE {
	eTYPE_TEXTURE	= 0,
	eTYPE_SOUND		= 1,
} GType;

class UPkgCommandlet : public UCommandlet
{
	DECLARE_CLASS(UPkgCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(StaticConstructor::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 0;

		unguard;
	}
	INT Main( const TCHAR* Parms )
	{
		guard(UPkgCommandlet::Main);

		// Create the editor class.
		UClass* EditorEngineClass = UObject::StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail | LOAD_DisallowFiles, NULL );
		GEditor  = ConstructObject<UEditorEngine>( EditorEngineClass );
		GEditor->UseSound = 1;
		GEditor->Init();
		GIsRequestingExit = 1; // Causes ctrl-c to immediately exit.

		FString Command, Type, PkgPath, PkgName, PkgDir, Dir;

		// Make sure we got all params.
		if( !ParseToken(Parms,Command,0) )
			appErrorf(TEXT("Command not specified."));

		if( !ParseToken(Parms,Type,0) )
			appErrorf(TEXT("Type not specified"));
		if( !ParseToken(Parms,PkgPath,0) )
			appErrorf(TEXT("Package not specified"));
		if( !ParseToken(Parms,Dir,0) )
			appErrorf(TEXT("Directory not specified"));

		// Parse the package name 
		if( PkgPath.InStr( TEXT("\\") ) != -1 )
		{
			PkgName = PkgPath.Right( PkgPath.Len() - PkgPath.InStr( TEXT("\\"), 1 ) - 1 );
			PkgDir = PkgPath.Left( PkgPath.InStr( TEXT("\\"), 1 ) );
			if( !PkgDir.Len() )
				PkgDir = TEXT(".");
		}
		else
		{
			PkgName = PkgPath;
			PkgDir = TEXT(".");
		}

		UClass* Class = FindObjectChecked<UClass>( ANY_PACKAGE, *Type );

		// Make sure options are valid.
		if( Command != TEXT("import") && Command != TEXT("export") )
			appErrorf(TEXT("Command not valid."));
		if( !Class )
			appErrorf(TEXT("Type not valid."));

		FString Ext, PkgExt;
		if( Type == TEXT("Texture") )		{ GType = eTYPE_TEXTURE;	Ext = TEXT("pcx");	PkgExt = TEXT("utx"); }
		else if( Type == TEXT("Sound") )	{ GType = eTYPE_SOUND;		Ext = TEXT("wav");	PkgExt = TEXT("uax"); }

		// Do it.
		if( Command == TEXT("export") )
		{
			UObject* Package = LoadPackage(NULL,*(PkgDir + TEXT("\\") + PkgName + TEXT(".") + PkgExt),LOAD_NoFail);

			GFileManager->MakeDirectory( *Dir );

			for( TObjectIterator<UObject> It; It; ++It )
			{
				if( It->IsA(Class) && It->IsIn(Package) )
				{
					FString Filename;
					FString Params;
					FString Group = It->GetFullName();

					switch( GType )
					{
						case eTYPE_TEXTURE:
							{
								Params = FString::Printf( TEXT("mipmap=%d masked=%d"), (Cast<UTexture>(*It)->GetNumMips() > 1 )?1:0, (Cast<UTexture>(*It)->bMasked)?1:0 );
								GConfig->SetString( TEXT("ImportInfo"), It->GetName(), *Params, *(Dir + TEXT("\\") + TEXT("spec.ini") ) );
							}
							break;

						case eTYPE_SOUND:
							break;
					}

					if( Group.InStr( TEXT(".") ) != Group.InStr( TEXT("."), 1 ) )
					{
						Group = Group.Right( Group.Len() - Group.InStr( TEXT(".") ) - 1 );
						Group = Group.Left( Group.InStr( TEXT(".") ) );

						GFileManager->MakeDirectory( *(Dir + TEXT("\\") + Group) );

						Filename = Dir + TEXT("\\") + Group + TEXT("\\") + It->GetName() + TEXT(".") + Ext;
					}
					else
						Filename = Dir + TEXT("\\") + It->GetName() + TEXT(".") + Ext;

					if( UExporter::ExportToFile(*It, NULL, *Filename, 1, 0) )
						GWarn->Logf( TEXT("Exported %s"), *Filename );
					else
						appErrorf(TEXT("Can't export %s "), *Filename );
				}
			}
		}
		else if( Command == TEXT("import") )
		{
			// Grab a list of directories inside of the specified dir.  These will become group names.
			TArray<FString> Dirs = GFileManager->FindFiles( *(Dir + TEXT("\\") + TEXT("*")), 0, 1 );
			new(Dirs)FString(TEXT("."));	// So we also include the base directory

			// Import all the filenames in the dir and one level of sub dirs.
			TArray<FString> Files;
			for( int dir = 0 ; dir < Dirs.Num() ; dir++ )
			{
				Files = GFileManager->FindFiles( *(Dir + TEXT("\\") + Dirs(dir) + TEXT("\\") + TEXT("*.") + Ext), 1, 0 );

				for( int file = 0 ; file < Files.Num() ; file++ )
				{
					FString Filename = Dir + TEXT("\\") + Dirs(dir) + TEXT("\\") + Files(file);

					FString File = Files(file);
					File = File.Left( File.InStr( TEXT(".") ) );

					FString Params;
					GConfig->GetString( TEXT("ImportInfo"), *File, Params, *(Dir + TEXT("\\") + TEXT("spec.ini") ) );

					switch( GType )
					{
						case eTYPE_TEXTURE:
							{
								UBOOL bMipmaps = 1, bMasked = 0, bAlphaTexture = 0;
								Parse( *Params, TEXT("mipmap="), bMipmaps );
								Parse( *Params, TEXT("masked="), bMasked );
								Parse( *Params, TEXT("alphatexture="), bAlphaTexture );

								if( Dirs(dir) == TEXT(".") )
								{
									GWarn->Logf( TEXT("Importing : %s"), *File );
									GEditor->Exec( *FString::Printf( TEXT("TEXTURE IMPORT FILE=\"%s\" NAME=\"%s\" PACKAGE=\"%s\" MIPS=%d MASKED=%d ALPHATEXTURE=%d"),
										*Filename, *File, *PkgName,
										bMipmaps, bMasked, bAlphaTexture ) );
								}
								else
								{
									GWarn->Logf( TEXT("Importing : %s.%s"), *Dirs(dir), *File );
									GEditor->Exec( *FString::Printf( TEXT("TEXTURE IMPORT FILE=\"%s\" NAME=\"%s\" PACKAGE=\"%s\" GROUP=\"%s\" MIPS=%d MASKED=%d ALPHATEXTURE=%d"),
										*Filename, *File, *PkgName, *Dirs(dir),
										bMipmaps, bMasked, bAlphaTexture ) );
								}
							}
							break;

						case eTYPE_SOUND:
							{
								if( Dirs(dir) == TEXT(".") )
								{
									GWarn->Logf( TEXT("Importing : %s"), *File );
									GEditor->Exec( *FString::Printf( TEXT("AUDIO IMPORT FILE=\"%s\" NAME=\"%s\" PACKAGE=\"%s\""),
										*Filename, *File, *PkgName ) );
								}
								else
								{
									GWarn->Logf( TEXT("Importing : %s.%s"), *Dirs(dir), *File );
									GEditor->Exec( *FString::Printf( TEXT("AUDIO IMPORT FILE=\"%s\" NAME=\"%s\" PACKAGE=\"%s\" GROUP=\"%s\""),
										*Filename, *File, *PkgName, *Dirs(dir) ) );
								}
							}
							break;
					}
				}
			}

			GWarn->Logf( TEXT("Saving : %s"), *(PkgDir + TEXT("\\") + PkgName + TEXT(".") + PkgExt) );
			GEditor->Exec( *FString::Printf( TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\""),
				*PkgName, *(PkgDir + TEXT("\\") + PkgName + TEXT(".") + PkgExt) ) );
		}

		GIsRequestingExit=1;
		return 0;

		unguard;
	}
};
IMPLEMENT_CLASS(UPkgCommandlet)

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/