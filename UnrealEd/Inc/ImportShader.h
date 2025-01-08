/*=============================================================================
	ImportShader : Code to handle the importing of RenderMonkey shaders
	Copyright 2002 Demiurge Studios. All Rights Reserved.

	Revision history:
		* Created by Albert Reed
=============================================================================*/

// DS_SHADER

#ifndef _IMPORT_SHADER_H
#define _IMPORT_SHADER_H

#include "Res\resource.h"

// --------------------------------------------------------------
//
// IMPORT SHADER Dialog
//
// --------------------------------------------------------------

// Mostly stolen from WDlgImportTexture
class WDlgImportShader : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgImportShader,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WLabel FilenameStatic;
	WEdit PackageEdit;
	WEdit GroupEdit;
	WCheckBox CreateShaderLibrary;
	WEdit SourcePackageEdit;

	FString defPackage, defGroup;
	TArray<FString>* Filenames;

	FString Package, Group, SourcePackage;
	BOOL bOKToAll;
	INT iCurrentFilename;

	// Constructor.
	WDlgImportShader( UObject* InContext, WBrowser* InOwnerWindow )
	:	WDialog			( TEXT("Import Shader"), IDDIALOG_IMPORT_SHADER, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)&WDlgImportShader::OnOk) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)&WDlgImportShader::EndDialogFalse) )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	GroupEdit		( this, IDEC_GROUP )
	,	FilenameStatic	( this, IDSC_FILENAME )
	,   CreateShaderLibrary (this, IDC_CreateLibrary)
	,   SourcePackageEdit(this, IDEC_SOURCE_PACKAGE)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportShader::OnInitDialog);
		WDialog::OnInitDialog();

		PackageEdit.SetText( *defPackage );
		GroupEdit.SetText( *defGroup );

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();

		unguard;
	}
	virtual INT DoModal( FString InDefPackage, FString InDefGroup, TArray<FString>* InFilenames)
	{
		guard(WDlgImportShader::DoModal);

		defPackage = InDefPackage;
		defGroup = InDefGroup;
		Filenames = InFilenames;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgImportShader::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*Filenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgImportShader::OnOkAll);
		if( GetDataFromUser() )
		{
			ImportFile( (*Filenames)(iCurrentFilename) );
			bOKToAll = TRUE;
			SetNextFilename();
		}
		unguard;
	}
	void OnSkip()
	{
		guard(WDlgImportShader::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void ImportTexture( void )
	{
		guard(WDlgImportShader::ImportTexture);
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportShader::RefreshName);
		FilenameStatic.SetText( *(*Filenames)(iCurrentFilename) );

		FString Name = GetFilenameOnly( (*Filenames)(iCurrentFilename) );

        // gam --- strip spaces from the name
        INT i = Name.InStr( TEXT(" ") );

        while( i >= 0 )
        {
            FString Left, Right;

            Left = Name.Left (i);
            Right = Name.Right (Name.Len() - i - 1);

            Name = Left + Right;

            i = Name.InStr( TEXT(" ") );
        }
        // --- gam

		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgImportShader::GetDataFromUser);
		Package = PackageEdit.GetText();
		Group = GroupEdit.GetText();

		SourcePackage = SourcePackageEdit.GetText();

        if( !Package.Len() )
		{
			appMsgf( 0, TEXT("You must specify a package.") );
			return FALSE;
		}

        if( appStrchr(*Package, ' ' ) != NULL )
		{
			appMsgf( 0, TEXT("Package can't have spaces.") );
			return FALSE;
		}

        if( appStrchr(*Group, ' ' ) != NULL )
		{
			appMsgf( 0, TEXT("Group can't have spaces.") );
			return FALSE;
		}

		if(CreateShaderLibrary.IsChecked() && !SourcePackage.Len())
		{
			appMsgf( 0, TEXT("You must specify a source package.") );
			return FALSE;
		}

        if(CreateShaderLibrary.IsChecked() && appStrchr(*SourcePackage, ' ' ) != NULL )
		{
			appMsgf( 0, TEXT("Package can't have spaces.") );
			return FALSE;
		}

		return TRUE;
		unguard;
	}
	void SetNextFilename( void )
	{
		guard(WDlgImportShader::SetNextFilename);
		iCurrentFilename++;
		if( iCurrentFilename == Filenames->Num() ) {
			EndDialogTrue();
			return;
		}

		if( bOKToAll ) {
			RefreshName();
			GetDataFromUser();
			ImportFile( (*Filenames)(iCurrentFilename) );
			SetNextFilename();
			return;
		};

		RefreshName();

		unguard;
	}
	void ImportFile( FString Filename )
	{
		guard(WDlgImportShader::ImportFile);
		FString Cmd;

		if( Group.Len() )
			Cmd = FString::Printf(TEXT("SHADER IMPORT FILE=\"%s\" PACKAGE=\"%s\" GROUP=\"%s\""),
				*(*Filenames)(iCurrentFilename), *Package, *Group );
		else
			Cmd = FString::Printf(TEXT("SHADER IMPORT FILE=\"%s\" PACKAGE=\"%s\""),
				*(*Filenames)(iCurrentFilename), *Package);

		if(CreateShaderLibrary.IsChecked())
		{
			Cmd += FString::Printf(TEXT(" BATCH_PACKAGE=\"%s\""), *SourcePackage);
		}

		

		GUnrealEd->Exec( *Cmd );

		unguard;
	}
};

#endif