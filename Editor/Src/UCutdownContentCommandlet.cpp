/*=============================================================================
	UCutdownContentCommandlet.cpp: Load the specified levels and save only the content
	                     packages which they reference.
	Copyright 2001 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Jack Porter
	* Example: ucc editor.cutdowncontent mymap.unr DM-*.unr Demo-*.unr

=============================================================================*/

#include "EditorPrivate.h"

/*-----------------------------------------------------------------------------
	Defines
-----------------------------------------------------------------------------*/

// Define this to generate codedrop content with epic's master file locations.
// Don't change this and check it in unless you also change the Build script.
// - Jack
#define EPICCODEDROP 1

#ifdef EPICCODEDROP

#define OLDUNRDIR TEXT("\\\\server\\WarfareDev\\codedrop\\Warfare\\Maps")
#define OLDUTXDIR TEXT("\\\\server\\WarfareDev\\codedrop\\Warfare\\Textures")
#define OLDUSXDIR TEXT("\\\\server\\WarfareDev\\codedrop\\Warfare\\StaticMeshes")
#define SERVERSOUNDSPATH TEXT("\\\\server\\WarfareDev\\codedrop\\Warfare\\Sounds\\*.uax")
#define SERVERMUSICPATH TEXT("\\\\server\\WarfareDev\\codedrop\\Warfare\\Music\\*.umx")
#define SERVERANIMATIONSPATH TEXT("\\\\server\\WarfareDev\\codedrop\\Warfare\\Animations\\*.ukx")

#define NEWUNRDIR TEXT("\\\\server\\WarfareDev\\WarfareContent-CODEDROP\\CutDown\\Maps")
#define NEWUTXDIR TEXT("\\\\server\\WarfareDev\\WarfareContent-CODEDROP\\CutDown\\Textures")
#define NEWUSXDIR TEXT("\\\\server\\WarfareDev\\WarfareContent-CODEDROP\\CutDown\\StaticMeshes")

#else

// You may want to change these.
#define OLDUNRDIR TEXT("..\\Maps")
#define OLDUTXDIR TEXT("..\\Textures")
#define OLDUSXDIR TEXT("..\\StaticMeshes")

#define NEWUNRDIR TEXT("..\\NewMaps")
#define NEWUTXDIR TEXT("..\\NewTextures")
#define NEWUSXDIR TEXT("..\\NewStaticMeshes")

#endif


/*-----------------------------------------------------------------------------
	UCutdownContentCommandlet
-----------------------------------------------------------------------------*/

class UCutdownContentCommandlet : public UCommandlet
{
	DECLARE_CLASS(UCutdownContentCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UCutdownContentCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 0;
		ShowErrorCount  = 1;

		unguard;
	}
	INT Main( const TCHAR* Parms )
	{
		guard(UCutdownContentCommandlet::Main);

#ifdef EPICCODEDROP

		// Delete and recreate the existing cutdown directories.
		if( !GFileManager->DeleteDirectory( NEWUNRDIR, 0, 1 ) )
			appErrorf( TEXT("Failed to remove directory tree: %s"), NEWUNRDIR );
		if( !GFileManager->MakeDirectory( NEWUNRDIR, 1 ) )
			appErrorf( TEXT("Failed to create directory: %s"), NEWUNRDIR );
		if( !GFileManager->DeleteDirectory( NEWUTXDIR, 0, 1 ) )
			appErrorf( TEXT("Failed to remove directory tree: %s"), NEWUTXDIR );
		if( !GFileManager->MakeDirectory( NEWUTXDIR, 1 ) )
			appErrorf( TEXT("Failed to create directory: %s"), NEWUTXDIR );
		if( !GFileManager->DeleteDirectory( NEWUSXDIR, 0, 1 ) )
			appErrorf( TEXT("Failed to remove directory tree: %s"), NEWUSXDIR );
		if( !GFileManager->MakeDirectory( NEWUSXDIR, 1 ) )
			appErrorf( TEXT("Failed to create directory: %s"), NEWUSXDIR );

		// Add the old source directories to the search path
		GSys->Paths.InsertZeroed(0,6);
		GSys->Paths(0) = FString(OLDUNRDIR)+TEXT("\\*.unr");
		GSys->Paths(1) = FString(OLDUTXDIR)+TEXT("\\*.utx");
		GSys->Paths(2) = FString(OLDUSXDIR)+TEXT("\\*.usx");
		GSys->Paths(3) = SERVERSOUNDSPATH;
		GSys->Paths(4) = SERVERMUSICPATH;
		GSys->Paths(5) = SERVERANIMATIONSPATH;
		
		GWarn->Logf( TEXT("Search paths are:") );
		for( INT i=0;i<GSys->Paths.Num();i++ )
			GWarn->Logf(TEXT("  %s"), GSys->Paths(i) );

#endif

		// Some stuff triggered by PostLoad has stat calculations in it.
		GEngineStats.Init();

		GLazyLoad = 0;

		TArray<FString> UTXList = GFileManager->FindFiles( *FString::Printf(TEXT("%s\\%s"), OLDUTXDIR, TEXT("*.utx")), 1, 0 );
		TArray<FString> USXList = GFileManager->FindFiles( *FString::Printf(TEXT("%s\\%s"), OLDUSXDIR, TEXT("*.usx")), 1, 0 );
	
		FString Wildcard;
		while( ParseToken( Parms, Wildcard, 0 ) )
		{
			TArray<FString> UNRList = GFileManager->FindFiles( *FString::Printf(TEXT("%s\\%s"), OLDUNRDIR, *Wildcard), 1, 0 );
			for( INT i=0;i<UNRList.Num();i++ )
			{	
				UObject::CollectGarbage(RF_Native | RF_Standalone);

				FString OldUNRName = FString::Printf(TEXT("%s\\%s"), OLDUNRDIR, *UNRList(i));
				FString NewUNRName = FString::Printf(TEXT("%s\\%s"), NEWUNRDIR, *UNRList(i));

				GWarn->Logf(TEXT("  Loading %s"), *OldUNRName );
				UObject* Package = LoadPackage( NULL, *OldUNRName, 0 );
				check(Package);
				ULevel* Level = FindObject<ULevel>( Package, TEXT("MyLevel") );
				check(Level);
			
				GWarn->Logf(TEXT("  Saving %s"), *NewUNRName );
				SavePackage( Package, Level, 0, *NewUNRName, GWarn );
			}
		}

#if 0
		GWarn->Logf(TEXT("Compressing Textures...."));
		UClass* TexClass = FindObjectChecked<UClass>( ANY_PACKAGE, TEXT("Texture") );
		for( TObjectIterator<UObject> It; It; ++It )
		{
			if( It->IsA(TexClass) )
			{
				UTexture* Texture	= (UTexture*) *It;

				TCHAR TextureName[1024];
				appStrcpy(TextureName, Texture->GetName());
				if ( !  (
					appStrstr(appStrupr(TextureName), TEXT("ALPHA"))	||
					appStrstr(appStrupr(TextureName), TEXT("TERRAIN"))  ||
					appStrstr(appStrupr(TextureName), TEXT("LAYER"))	||
					(Texture->Format != TEXF_RGBA8)
				))
				{
					UBOOL ConvertToDXT1 = true;
							
					INT Width	= Texture->Mips(0).USize;
					INT Height	= Texture->Mips(0).VSize;
							
					// Texture are always lazy loaded.
					Texture->Mips(0).DataArray.Load();
					check( &Texture->Mips(0).DataArray(0) )

					INT* Data = (INT*) &Texture->Mips(0).DataArray(0);

					INT Alpha = ETrueColor_A;
					for( INT Y=0; Y<Height; Y++ );
						for( INT X=0; X<Width; X++ );
							Alpha &= (*(Data++) & ETrueColor_A);

					if( Alpha != ETrueColor_A )
						ConvertToDXT1 = false;
							
					GWarn->Logf(TEXT("Compressing: %s"), Texture->GetPathName() );
					Texture->Compress( ConvertToDXT1 ? TEXF_DXT1 : TEXF_DXT5, 1 );
				}
			}
		}
#endif

		GWarn->Logf(TEXT("Saving Textures...."));
		for (INT i=0; i<UTXList.Num(); i++)
		{	
			UPackage* Package = FindObject<UPackage>( NULL, *(UTXList(i).Left(UTXList(i).Len()-4)) );
			if( Package )
			{
				FString NewUTXName = FString::Printf(TEXT("%s\\%s"), NEWUTXDIR, *UTXList(i));
				GWarn->Logf(TEXT("  Saving %s"), *NewUTXName );
				SavePackage( Package, NULL, RF_Standalone, *NewUTXName, NULL );
			}
		}

		GWarn->Logf(TEXT("Saving Static Meshes...."));
		for (INT i=0; i<USXList.Num(); i++)
		{
			UPackage* Package = FindObject<UPackage>( NULL, *(USXList(i).Left(USXList(i).Len()-4)) );
			if( Package )
			{
				FString NewUSXName = FString::Printf(TEXT("%s\\%s"), NEWUSXDIR, *USXList(i) );
				GWarn->Logf(TEXT("  Saving %s"), *NewUSXName );
				SavePackage( Package, NULL, RF_Standalone, *NewUSXName, NULL );
			}
		}

		GIsRequestingExit=1;
		return 0;
		unguard;
	}
};
IMPLEMENT_CLASS(UCutdownContentCommandlet)

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
