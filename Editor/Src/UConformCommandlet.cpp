/*=============================================================================
	UConformCommandlet.cpp: Unreal file conformer.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#include "EditorPrivate.h"
/*-----------------------------------------------------------------------------
	UConformCommandlet.
-----------------------------------------------------------------------------*/

class UConformCommandlet : public UCommandlet
{
	DECLARE_CLASS(UConformCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UConformCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 1;

		unguard;
	}
	INT Main( const TCHAR* Parms )
	{
		guard(UConformCommandlet::Main);
		FString Src, Old;
		if( !ParseToken(Parms,Src,0) )
			appErrorf(TEXT("Source file not specified"));
		if( !ParseToken(Parms,Old,0) )
			appErrorf(TEXT("Old file not specified"));
		GWarn->Log( TEXT("Loading...") );
		BeginLoad();
		ULinkerLoad* OldLinker = UObject::GetPackageLinker( CreatePackage(NULL,*(Old+FString(TEXT("_OLD")))), *Old, LOAD_NoWarn|LOAD_NoVerify, NULL, NULL );
		EndLoad();
		UObject* NewPackage = LoadPackage( NULL, *Src, LOAD_NoFail );
		if( !OldLinker )
			appErrorf( TEXT("Old file '%s' load failed"), *Old );
		if( !NewPackage )
			appErrorf( TEXT("New file '%s' load failed"), *Src );
		GWarn->Log( TEXT("Saving...") );
		SavePackage( NewPackage, NULL, RF_Standalone, *Src, GError, OldLinker );
		GWarn->Logf( TEXT("File %s successfully conformed to %s..."), *Src, *Old );
		GIsRequestingExit=1;
		return 0;
		unguard;
	}
};
IMPLEMENT_CLASS(UConformCommandlet)

/*-----------------------------------------------------------------------------
	UCheckUnicodeCommandlet.
-----------------------------------------------------------------------------*/

class UCheckUnicodeCommandlet : public UCommandlet
{
	DECLARE_CLASS(UCheckUnicodeCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UCheckUnicodeCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 1;

		unguard;
	}
	INT Main( const TCHAR* Parms )
	{
		guard(UConformCommandlet::Main);

		FString Path, Wildcard;
		if( !ParseToken(Parms,Path,0) )
			appErrorf(TEXT("Missing path"));
		if( !ParseToken(Parms,Wildcard,0) )
			appErrorf(TEXT("Missing wildcard"));
		GWarn->Log( TEXT("Files:") );
		TArray<FString> Files=GFileManager->FindFiles(*(Path*Wildcard),1,0);
		INT Pages[256];
		BYTE* Chars = (BYTE*)appMalloc(65536*sizeof(BYTE), TEXT("UnicodeCheck") );
		appMemzero(Pages,sizeof(Pages));
		appMemzero(Chars,sizeof(65536*sizeof(BYTE)));
		for( TArray<FString>::TIterator i(Files); i; ++i )
		{
			FString S;
			GWarn->Logf( TEXT("Checking: %s"),*(Path * *i));
			verify(appLoadFileToString(S,*(Path * *i)));
			for( INT i=0; i<S.Len(); i++ )
			{
				GWarn->Logf(TEXT("Found Character: %i"), (*S)[i]);
				if( Chars[(*S)[i]] == 0 )
				{
					Pages[(*S)[i]/256]++;
					Chars[(*S)[i]] = 1;
				}
			}

		}
		INT T=0, P=0;
		{for( INT i=0; i<254; i++ )
			if( Pages[i] )
			{
				GWarn->Logf(TEXT("%x: %i"), i, Pages[i]);
				T += Pages[i];
				P++;
			}
		}
		GWarn->Logf(TEXT("Total Characters: %i"), T);
		GWarn->Logf(TEXT("Total Pages: %i"), P);
		
		GIsRequestingExit=1;
		return 0;
		unguard;
	}
};
IMPLEMENT_CLASS(UCheckUnicodeCommandlet)

/*-----------------------------------------------------------------------------
	UPackageFlagCommandlet.
-----------------------------------------------------------------------------*/

class UPackageFlagCommandlet : public UCommandlet
{
	DECLARE_CLASS(UPackageFlagCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UPackageFlagCommandlet::StaticConstructor);

		LogToStdout     = 0;
		IsClient        = 1;
		IsEditor        = 1;
		IsServer        = 1;
		LazyLoad        = 1;
		ShowErrorCount  = 1;

		unguard;
	}
	INT Main( const TCHAR* Parms )
	{
		guard(UPackageFlagCommandlet::Main);
		TCHAR* FlagNames[] = 
					{
						TEXT("AllowDownload"),
						TEXT("ClientOptional"),
						TEXT("ServerSideOnly"),
						TEXT("BrokenLinks"),
						TEXT("Unsecure"),
						TEXT("Need")
					};
		DWORD Flags[] = 
					{
						PKG_AllowDownload,
						PKG_ClientOptional,
						PKG_ServerSideOnly,
						PKG_BrokenLinks,
						PKG_Unsecure,
						PKG_Need
					};
		INT NumFlags = 6;
		FString Src, Dest;
		if( !ParseToken(Parms,Src,0) )
			appErrorf(TEXT("Source Package file not specified"));
		BeginLoad();
		ULinkerLoad* OldLinker = UObject::GetPackageLinker( CreatePackage(NULL,*(Src+FString(TEXT("_OLD")))), *Src, LOAD_NoWarn|LOAD_NoVerify, NULL, NULL );
		EndLoad();

		UPackage* Package = Cast<UPackage>(LoadPackage( NULL, *Src, LOAD_NoFail ));
		if( !Package )
			appErrorf( TEXT("Source package '%s' load failed"), *Src );

		GWarn->Logf( TEXT("Loaded %s."), *Src );
		GWarn->Logf( TEXT("Current flags: %d"), (INT)Package->PackageFlags );
		for( INT i=0;i<NumFlags;i++ )
			if( Package->PackageFlags & Flags[i] )
				GWarn->Logf( TEXT(" %s"), FlagNames[i]);
		GWarn->Log( TEXT("") );
		if( ParseToken(Parms,Dest,0) )
		{
			FString Flag;
			while( ParseToken(Parms,Flag,0) )
			{
				for( INT i=0;i<NumFlags;i++ )
				{
					if( !appStricmp( &(*Flag)[1], FlagNames[i] ) )
					{
						switch((*Flag)[0])
						{
						case '+':
							Package->PackageFlags |= Flags[i];
							break;
						case '-':
							Package->PackageFlags &= ~Flags[i];
							break;
						}
					}
				}
			}	

			if( !SavePackage( Package, NULL, RF_Standalone, *Dest, GError, OldLinker ) )
				appErrorf( TEXT("Saving package '%s' failed"), *Dest );

			GWarn->Logf( TEXT("Saved %s."), *Dest );
			GWarn->Logf( TEXT("New flags: %d"), (INT)Package->PackageFlags );
			for( INT i=0;i<NumFlags;i++ )
				if( Package->PackageFlags & Flags[i] )
					GWarn->Logf( TEXT(" %s"), FlagNames[i]);
		}
		GIsRequestingExit=1;
		return 0;
		unguard;
	}
};
IMPLEMENT_CLASS(UPackageFlagCommandlet)

/*-----------------------------------------------------------------------------
	UDataRipCommandlet.
-----------------------------------------------------------------------------*/

class UDataRipCommandlet : public UCommandlet
{
	DECLARE_CLASS(UDataRipCommandlet,UCommandlet,CLASS_Transient,Editor);
	void StaticConstructor()
	{
		guard(UDataRipCommandlet::StaticConstructor);

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
		guard(UDataRipCommandlet::Main);

		FString Src, Dest;
		if( !ParseToken(Parms,Src,0) )
			appErrorf(TEXT("Source package file not specified"));
		if( !ParseToken(Parms,Dest,0) )
			appErrorf(TEXT("Destination package file not specified"));

		UClass* EditorEngineClass = UObject::StaticLoadClass( UEditorEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail | LOAD_DisallowFiles, NULL );
		GEditor  = ConstructObject<UEditorEngine>( EditorEngineClass );
		GEditor->UseSound = 0;
        GEditor->InitEditor();
		GIsRequestingExit = 1; // Causes ctrl-c to immediately exit.

		// strip extension
		FString SrcPkg = Src;
		INT i = SrcPkg.InStr( PATH_SEPARATOR, 1 );
		if( i != -1 )
			SrcPkg = SrcPkg.Mid(i+1);
		i = SrcPkg.InStr( TEXT(".") );
		if( i != -1 )
			SrcPkg = SrcPkg.Left(i);

		BeginLoad();
		ULinkerLoad* Conform = UObject::GetPackageLinker( CreatePackage(NULL,*(SrcPkg+TEXT("_OLD"))), *Src, LOAD_NoFail, NULL, NULL );
		UPackage* Package = Cast<UPackage>(LoadPackage( NULL, *Src, LOAD_NoFail ));
		EndLoad();

		if( !Package )
			appErrorf( TEXT("Source package '%s' load failed"), *Src );
		GWarn->Logf( TEXT("Loaded %s."), *Src );
		UClass* FireClass = CastChecked<UClass>(StaticLoadObject( UClass::StaticClass(), NULL, TEXT("Fire.FireTexture"), NULL, LOAD_NoWarn, NULL ));

		// Blank sound.
		TArray<BYTE> BlankSoundData;
		appLoadFileToArray( BlankSoundData, TEXT("C:\\Documents and Settings\\Default User\\Templates\\sndrec.wav") );

		// Clear textures and sounds data
		for( TObjectIterator<UObject> It; It; ++It )
		{
			if( It->IsIn(Package) )
			{
				if( It->IsA(UTexture::StaticClass()) && !It->IsA(FireClass) )
				{
					UTexture* T = Cast<UTexture>(*It);
					T->Mips.Empty();
				}
				if( It->GetClass() == USound::StaticClass() ) //!! must be a USound, not a procedural sound etc.
				{
					USound* S = Cast<USound>(*It);
					FSoundData& SoundData = S->Data;
					SoundData.Load();
					SoundData.Empty();
					SoundData.AddZeroed( BlankSoundData.Num() );
					appMemcpy( &SoundData(0), &BlankSoundData(0), BlankSoundData.Num() );
				}
			}
		}

		if( !SavePackage( Package, NULL, RF_Standalone, *Dest, GError, Conform ) )
			appErrorf( TEXT("Saving package '%s' failed"), *Dest );
		GWarn->Logf( TEXT("Saved %s."), *Dest );

		GIsRequestingExit=1;
		return 0;
		unguard;
	}
};
IMPLEMENT_CLASS(UDataRipCommandlet)


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
