/*=============================================================================
	BrowserParticles : Browser window for particle effects
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Aaron Leiby

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>
extern FString GLastDir[eLASTDIR_MAX];

// --------------------------------------------------------------
//
// WBROWSERPARTICLES
//
// --------------------------------------------------------------

#define ID_BP_TOOLBAR	29020
TBBUTTON tbBXButtons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_PB_IMPORT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	INT ID;
} ToolTips_BX[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Import Particles"), IDMN_PB_FileOpen,
	NULL, 0
};

class WBrowserParticles : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserParticles,WBrowser,Window)

	TMap<DWORD,FWindowAnchor> Anchors;

	FContainer *Container;
	WComboBox *pComboPackage, *pComboGroup;
	WListBox *pListParticles;
	WCheckBox *pCheckGroupAll;
	HWND hWndToolBar;
	WToolTip *ToolTipCtrl;
	MRUList* mrulist;

	// Structors.
	WBrowserParticles( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	{
		Container = NULL;
		pComboPackage = pComboGroup = NULL;
		pListParticles = NULL;
		pCheckGroupAll = NULL;
		MenuID = IDMENU_BrowserParticles;
		BrowserID = eBROWSER_PARTICLES;
		Description = TEXT("Particles");
	}

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserParticles::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserParticles::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_BrowserParticles) );

		Container = new FContainer();

		// PACKAGES
		//
		pComboPackage = new WComboBox( this, IDCB_PACKAGE );
		pComboPackage->OpenWindow( 1, 1 );
		pComboPackage->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserParticles::OnComboPackageSelChange);

		// GROUP
		//
		pComboGroup = new WComboBox( this, IDCB_GROUP );
		pComboGroup->OpenWindow( 1, 1 );
		pComboGroup->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserParticles::OnComboGroupSelChange);

		// PARTICLES LIST
		//
		pListParticles = new WListBox( this, IDLB_PARTICLES );
		pListParticles->OpenWindow( 1, 0, 0, 0, 1 );
		pListParticles->SelectionChangeDelegate = FDelegate(this, (TDelegate)&WBrowserParticles::OnListParticlesSelectionChange);
		pListParticles->DoubleClickDelegate = FDelegate(this, (TDelegate)&WBrowserParticles::OnListParticlesDblClick);

		// CHECK BOXES
		//
		pCheckGroupAll = new WCheckBox( this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)&WBrowserParticles::OnGroupAllClick) );
		pCheckGroupAll->OpenWindow( 1, 0, 0, 1, 1, TEXT("All"), 1, 0, BS_PUSHLIKE );

		hWndToolBar = CreateToolbarEx( 
			hWnd, WS_CHILD | WS_BORDER | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserParticles_TOOLBAR,
			2,
			hInstance,
			IDB_BrowserParticles_TOOLBAR,
			(LPCTBBUTTON)&tbBXButtons,
			3,
			16,16,
			16,16,
			sizeof(TBBUTTON));
		check(hWndToolBar);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( INT tooltip = 0 ; ToolTips_BX[tooltip].ID > 0 ; ++tooltip )
		{
			// Figure out the rectangle for the toolbar button.
			INT index = SendMessageX( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_BX[tooltip].ID, 0 );
			RECT rect;
			SendMessageX( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_BX[tooltip].ToolTip, tooltip, &rect );
		}

		mrulist = new MRUList( *PersistentName );
		mrulist->ReadINI();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		INT Top = 0;
		Anchors.Set( (DWORD)hWndToolBar,			FWindowAnchor( hWnd, hWndToolBar,			ANCHOR_TL, 0, 0,		ANCHOR_RIGHT|ANCHOR_HEIGHT, 0, STANDARD_TOOLBAR_HEIGHT ) );
		Top += STANDARD_TOOLBAR_HEIGHT+4;
		Anchors.Set( (DWORD)pComboPackage->hWnd,	FWindowAnchor( hWnd, pComboPackage->hWnd,	ANCHOR_TL, 4, Top,		ANCHOR_RIGHT|ANCHOR_HEIGHT, -4, STANDARD_CTRL_HEIGHT ) );
		Top += STANDARD_CTRL_HEIGHT+2; 
		Anchors.Set( (DWORD)pCheckGroupAll->hWnd,	FWindowAnchor( hWnd, pCheckGroupAll->hWnd,	ANCHOR_TL, 4, Top,		ANCHOR_WIDTH|ANCHOR_HEIGHT, 64, STANDARD_CTRL_HEIGHT ) );
		Anchors.Set( (DWORD)pComboGroup->hWnd,		FWindowAnchor( hWnd, pComboGroup->hWnd,		ANCHOR_TL, 4+64+2, Top,	ANCHOR_RIGHT|ANCHOR_HEIGHT, -4, STANDARD_CTRL_HEIGHT ) );
		Top += STANDARD_CTRL_HEIGHT+2; 
		Anchors.Set( (DWORD)pListParticles->hWnd,	FWindowAnchor( hWnd, pListParticles->hWnd,	ANCHOR_TL, 4, Top,		ANCHOR_BR, -4, -4 ) );

		Container->SetAnchors( &Anchors );

		RefreshAll();
		PositionChildControls();

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserParticles::OnDestroy);

		mrulist->WriteINI();
		delete mrulist;

		delete Container;
		delete pComboPackage;
		delete pComboGroup;
		delete pListParticles;
		delete pCheckGroupAll;

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;

		WBrowser::OnDestroy();
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserParticles::UpdateMenu);

		HMENU menu = IsDocked() ? GetMenu( OwnerWindow->hWnd ) : GetMenu( hWnd );
		CheckMenuItem( menu, IDMN_MB_DOCK, MF_BYCOMMAND | (IsDocked() ? MF_CHECKED : MF_UNCHECKED) );

		if( mrulist
				&& GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserParticles::OnCommand);
		switch( Command ) {

			case IDMN_MRU1:
			case IDMN_MRU2:
			case IDMN_MRU3:
			case IDMN_MRU4:
			case IDMN_MRU5:
			case IDMN_MRU6:
			case IDMN_MRU7:
			case IDMN_MRU8:
			{
				FString Filename = mrulist->Items[Command - IDMN_MRU1];
				GUnrealEd->Exec( *FString::Printf(TEXT("OBJ LOAD FILE=\"%s\""), *Filename ));

				mrulist->MoveToTop( Command - IDMN_MRU1 );
				mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

				FString Package = Filename.Right( Filename.Len() - (Filename.InStr( TEXT("\\"), 1) + 1) );
				Package = Package.Left( Package.InStr( TEXT(".")) );

				GBrowserMaster->RefreshAll();
				pComboPackage->SetCurrent( pComboPackage->FindStringExact( *Package ) );
				RefreshGroups();
				RefreshParticlesList();
			}
			break;

			case IDMN_PB_IMPORT:
				{
					OPENFILENAMEA ofn;
					char File[8192] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 8192;
					ofn.lpstrFilter = "Particles Packages\0*.u;*.unr\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UPS]) );
					ofn.lpstrDefExt = "u";
					ofn.lpstrTitle = "Open Particle Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	
					if( GetOpenFileNameA(&ofn) )
					{
						INT iNULLs = FormatFilenames( File );
		
						TArray<FString> StringArray;
						FString S = appFromAnsi( File );
						S.ParseIntoArray( TEXT("|"), &StringArray );

						INT iStart = 0;
						FString Prefix = TEXT("\0");

						if( iNULLs )
						{
							iStart = 1;
							Prefix = *(StringArray(0));
							Prefix += TEXT("\\");
						}

						if( StringArray.Num() > 0 )
						{
							if( StringArray.Num() == 1 )
							{
								SavePkgName = *(StringArray(0));
								SavePkgName = SavePkgName.Right( SavePkgName.Len() - (SavePkgName.Left( SavePkgName.InStr(TEXT("\\"), 1)).Len() + 1 ));
							}
							else
								SavePkgName = *(StringArray(1));
							SavePkgName = SavePkgName.Left( SavePkgName.InStr(TEXT(".")) );
						}

						if( StringArray.Num() == 1 )
							GLastDir[eLASTDIR_UPS] = StringArray(0).Left( StringArray(0).InStr( TEXT("\\"), 1 ) );
						else
							GLastDir[eLASTDIR_UPS] = StringArray(0);

						GWarn->BeginSlowTask( TEXT(""), 1 );

						for( INT x = iStart ; x < StringArray.Num() ; ++x )
						{
							GWarn->StatusUpdatef( x, StringArray.Num(), TEXT("Loading %s"), *(StringArray(x)) );

							TCHAR l_chCmd[512];
							appSprintf( l_chCmd, TEXT("OBJ LOAD FILE=\"%s%s\""), *Prefix, *(StringArray(x)) );
							GUnrealEd->Exec( l_chCmd );

							mrulist->AddItem( *(StringArray(x)) );
							if( GBrowserMaster->GetCurrent()==BrowserID )
								mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
						}

						GWarn->EndSlowTask();

						GBrowserMaster->RefreshAll();
						pComboPackage->SetCurrent( pComboPackage->FindStringExact( *SavePkgName ) );
						RefreshGroups();
						RefreshParticlesList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			default:
				WBrowser::OnCommand(Command);
				break;
		}
		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserParticles::RefreshAll);
		RefreshPackages();
		RefreshGroups();
		RefreshParticlesList();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserParticles::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		UpdateMenu();
		unguard;
	}
	
	void RefreshPackages( void )
	{
		guard(WBrowserParticles::RefreshPackages);

		// PACKAGES
		//
		pComboPackage->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GUnrealEd->Get( TEXT("OBJ"), TEXT("PACKAGES CLASS=ParticleGenerator"), GetPropResult );

		TArray<FString> StringArray;
		GetPropResult.ParseIntoArray( TEXT(","), &StringArray );

		for( INT x = 0 ; x < StringArray.Num() ; ++x )
		{
			pComboPackage->AddString( *(StringArray(x)) );
		}

		pComboPackage->SetCurrent( 0 );
		unguard;
	}
	void RefreshGroups( void )
	{
		guard(WBrowserParticles::RefreshGroups);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

		// GROUPS
		//
		pComboGroup->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		TCHAR l_ch[256];
		appSprintf( l_ch, TEXT("GROUPS CLASS=ParticleGenerator PACKAGE=\"%s\""), *Package );
		GUnrealEd->Get( TEXT("OBJ"), l_ch, GetPropResult );

		TArray<FString> StringArray;
		GetPropResult.ParseIntoArray( TEXT(","), &StringArray );

		for( INT x = 0 ; x < StringArray.Num() ; ++x )
		{
			pComboGroup->AddString( *(StringArray(x)) );
		}

		pComboGroup->SetCurrent( 0 );

		unguard;
	}
	void RefreshParticlesList( void )
	{
		guard(WBrowserParticles::RefreshParticlesList);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );

		// PARTICLES
		//
		pListParticles->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		TCHAR l_ch[256];

		if( pCheckGroupAll->IsChecked() )
			appSprintf( l_ch, TEXT("QUERY TYPE=ParticleGenerator PACKAGE=\"%s\""), *Package );
		else
			appSprintf( l_ch, TEXT("QUERY TYPE=ParticleGenerator PACKAGE=\"%s\" GROUP=\"%s\""), *Package, *Group );

		GUnrealEd->Get( TEXT("OBJ"), l_ch, GetPropResult );

		TArray<FString> StringArray;
		GetPropResult.ParseIntoArray( TEXT(" "), &StringArray );

		for( INT x = 0 ; x < StringArray.Num() ; ++x )
		{
			pListParticles->AddString( *(StringArray(x)) );
		}

		pListParticles->SetCurrent( 0, 1 );

		unguard;
	}
	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserParticles::PositionChildControls);
		if( Container ) Container->RefreshControls();
		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnComboPackageSelChange()
	{
		guard(WBrowserParticles::OnComboPackageSelChange);
		RefreshGroups();
		RefreshParticlesList();
		unguard;
	}
	void OnComboGroupSelChange()
	{
		guard(WBrowserParticles::OnComboGroupSelChange);
		RefreshParticlesList();
		unguard;
	}
	void OnListParticlesSelectionChange()
	{
		guard(WBrowserParticles::OnListParticlesSelectionChange);
		GUnrealEd->Exec( *(FString::Printf(TEXT("SETCURRENTPARTICLESYSTEM IMAGE=%s"), GetCurrentPathName() )));
		unguard;
	}
	void OnListParticlesDblClick()
	{
		guard(WBrowserParticles::OnListParticlesDblClick);
		AActor* Actor;
		if( ParseObject<AActor>( *(FString(TEXT("OBJECT=")) + GetCurrentPathName()), TEXT("OBJECT="), Actor, ANY_PACKAGE ) )
		{
			TCHAR Temp[256];
			appSprintf( Temp, TEXT("Object %s Properties"), Actor->GetPathName() );
			WObjectProperties* ObjectProperties = new WObjectProperties( TEXT("ObjectProperties"), CPF_Edit, Temp, NULL, 1 );
			ObjectProperties->OpenWindow( ObjectProperties->List.hWnd );
			((FObjectsItem*)ObjectProperties->GetRoot())->SetObjects( (UObject**)&Actor, 1 );
			ObjectProperties->SetNotifyHook( GUnrealEd );
			ObjectProperties->ForceRefresh();
			ObjectProperties->Show(1);
		}
		unguard;
	}
	void OnGroupAllClick()
	{
		guard(WBrowserParticles::OnGroupAllClick);
		EnableWindow( pComboGroup->hWnd, !pCheckGroupAll->IsChecked() );
		RefreshParticlesList();
		unguard;
	}
	virtual FString GetCurrentPathName( void )
	{
		guard(WBrowserParticles::GetCurrentPathName);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );
		FString Name = pListParticles->GetString( pListParticles->GetCurrent() );

		if( Group.Len() )
			return *FString::Printf(TEXT("%s.%s.%s"), *Package, *Group, *Name );
		else
			return *FString::Printf(TEXT("%s.%s"), *Package, *Name );

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
