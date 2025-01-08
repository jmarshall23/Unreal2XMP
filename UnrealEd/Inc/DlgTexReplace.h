/*=============================================================================
	TexReplace : Replace one texture with another in the level
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

class WDlgTexReplace : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgTexReplace,WDialog,UnrealEd)

	// Variables.
	WButton Set1Button, Set2Button, ReplaceButton, CancelButton;
	WLabel TexName1Label, TexName2Label;

	UViewport *pViewport1, *pViewport2;
	UMaterial *pTexture1, *pTexture2;

	// Constructor.
	WDlgTexReplace( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog		( TEXT("Replace Textures"), IDDIALOG_TEX_REPLACE, InOwnerWindow )
	,	Set1Button	( this, IDPB_SET1, FDelegate(this,(TDelegate)&WDlgTexReplace::OnSet1Button) )
	,	Set2Button	( this, IDPB_SET2, FDelegate(this,(TDelegate)&WDlgTexReplace::OnSet2Button) )
	,	ReplaceButton	( this, IDPB_REPLACE, FDelegate(this,(TDelegate)&WDlgTexReplace::OnReplaceButton) )
	,	CancelButton	( this, IDCANCEL, FDelegate(this,(TDelegate)&WDlgTexReplace::EndDialogTrue) )
	,	TexName1Label	(this, IDSC_TEX_NAME1 )
	,	TexName2Label	(this, IDSC_TEX_NAME2 )
	{
		pViewport1 = pViewport2 = NULL;
		pTexture1 = pTexture2 = NULL;
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgTexReplace::OnInitDialog);
		WDialog::OnInitDialog();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgTexReplace::OnDestroy);
		WDialog::OnDestroy();

		delete pViewport1;
		delete pViewport2;

		unguard;
	}
	virtual void DoModeless( UBOOL bShow )
	{
		guard(WDlgTexReplace::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_TEX_REPLACE), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show( bShow );
		unguard;
	}
	void Refresh()
	{
		guard(WDlgTexReplace::Refresh);

		::LockWindowUpdate( hWnd );

		if( pTexture1 )
		{
			delete pViewport1;

			pViewport1 = GUnrealEd->Client->NewViewport( TEXT("TEXREPLACE1") );
			GUnrealEd->Level->SpawnViewActor( pViewport1 );
			pViewport1->Input->Init( pViewport1 );
			check(pViewport1->Actor);
			pViewport1->Actor->ShowFlags = SHOW_StandardView | SHOW_ChildWindow | SHOW_RealTime;
			pViewport1->Actor->RendMap   = REN_TexView;
			pViewport1->Actor->Misc2 = NULL;
			pViewport1->Group = NAME_None;
			pViewport1->MiscRes = pTexture1;

			RECT rect;
			::GetWindowRect( GetDlgItem( hWnd, IDSC_TEXTURE1 ), &rect );
			INT Width = min( rect.right - rect.left, pTexture1->MaterialUSize()),
				Height = min( rect.bottom - rect.top, pTexture1->MaterialVSize());
			::ScreenToClient( hWnd, &(*((POINT*)&rect)) );
			pViewport1->OpenWindow( (DWORD)hWnd, 0, Width, Height, rect.left, rect.top );
		}

		if( pTexture2 )
		{
			delete pViewport2;

			pViewport2 = GUnrealEd->Client->NewViewport( TEXT("TEXREPLACE2") );
			GUnrealEd->Level->SpawnViewActor( pViewport2 );
			pViewport2->Input->Init( pViewport2 );
			check(pViewport2->Actor);
			pViewport2->Actor->ShowFlags = SHOW_StandardView | SHOW_ChildWindow | SHOW_RealTime;
			pViewport2->Actor->RendMap   = REN_TexView;
			pViewport2->Group = NAME_None;
			pViewport2->MiscRes = pTexture2;

			RECT rect;
			::GetWindowRect( GetDlgItem( hWnd, IDSC_TEXTURE2 ), &rect );
			INT Width = min( rect.right - rect.left, pTexture2->MaterialUSize()),
				Height = min( rect.bottom - rect.top, pTexture2->MaterialVSize());
			::ScreenToClient( hWnd, &(*((POINT*)&rect)) );
			pViewport2->OpenWindow( (DWORD)hWnd, 0, Width, Height, rect.left, rect.top );
		}

		::LockWindowUpdate( NULL );

		unguard;
	}
	void OnSet1Button()
	{
		guard(WDlgTexReplace::OnSet1Button);
		pTexture1 = GUnrealEd->CurrentMaterial;
		FString Name = pTexture1->GetFullName();
		Name = Name.Right( Name.Len() - 8 );
		TexName1Label.SetText( *Name );
		Refresh();
		unguard;
	}
	void OnSet2Button()
	{
		guard(WDlgTexReplace::OnSet2Button);
		pTexture2 = GUnrealEd->CurrentMaterial;
		FString Name = pTexture2->GetFullName();
		Name = Name.Right( Name.Len() - 8 );
		TexName2Label.SetText( *Name );
		Refresh();
		unguard;
	}
	void OnReplaceButton()
	{
		guard(WDlgTexReplace::OnReplaceButton);

		GUnrealEd->Trans->Begin( TEXT("Replace Textures") );

		for( TArray<AActor*>::TIterator ItA(GUnrealEd->Level->Actors); ItA; ++ItA )
		{
			AActor* Actor = *ItA;
			if( Actor )
			{
				// BSP

				UModel* M = Actor->IsA(ALevelInfo::StaticClass()) ? GUnrealEd->Level->Model : Actor->Brush;
				if( M )
				{
					//!!MAT
					M->Surfs.ModifyAllItems();
					for( TArray<FBspSurf>::TIterator ItS(M->Surfs); ItS; ++ItS )
						if( ItS->Material==pTexture1 )
							ItS->Material = pTexture2;
					if( M->Polys )
					{
						M->Polys->Element.ModifyAllItems();
						for( TArray<FPoly>::TIterator ItP(M->Polys->Element); ItP; ++ItP )
							if( ItP->Material==pTexture1 )
								ItP->Material = pTexture2;
					}
				}

				// SKINS ARRAY

				for( int x = 0 ; x < Actor->Skins.Num() ; ++x )
				{
					Actor->Modify();
					if( Actor->Skins(x) == pTexture1 )
						Actor->Skins(x) = pTexture2;
				}

				// SPRITES/MISC

				if( Actor->Texture == pTexture1 )
				{
					Actor->Modify();
					Actor->Texture = pTexture2;
				}

				// TERRAIN

				ATerrainInfo* TI = Cast<ATerrainInfo>( Actor );
				if( TI )
				{
					TI->Modify();
					for( INT x = 0 ; x < ARRAY_COUNT(TI->Layers) ; ++x )
						if( TI->Layers[x].Texture == pTexture1 )
							TI->Layers[x].Texture = pTexture2;
				}
			}
		}
		GUnrealEd->Trans->End();

		GUnrealEd->RedrawLevel(NULL);

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
