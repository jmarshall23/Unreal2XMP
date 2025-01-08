/*=============================================================================
	SearchActors : Searches for actors using various criteria
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

class WDlgSearchActors : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgSearchActors,WDialog,UnrealEd)

	// Variables.
	WButton CloseButton, GotoActorButton, DeleteActorButton;
	WListBox ActorList;
	WEdit NameEdit, EventEdit, TagEdit;

	// Constructor.
	WDlgSearchActors( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Search for Actors"), IDDIALOG_SEARCH, InOwnerWindow )
	,	CloseButton		( this, IDPB_CLOSE,			FDelegate(this,(TDelegate)&WDlgSearchActors::OnClose) )
	,	GotoActorButton	( this, IDPB_GOTOACTOR,		FDelegate(this,(TDelegate)&WDlgSearchActors::OnActorListDblClick) )
	,	DeleteActorButton( this, IDPB_DELETEACTOR,	FDelegate(this,(TDelegate)&WDlgSearchActors::OnDeleteActor) )
	,	NameEdit		( this, IDEC_NAME )
	,	EventEdit		( this, IDEC_EVENT )
	,	TagEdit			( this, IDEC_TAG )
	,	ActorList		( this, IDLB_NAMES )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgSearchActors::OnInitDialog);
		WDialog::OnInitDialog();
		ActorList.DoubleClickDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnActorListDblClick);
		NameEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnNameEditChange);
		EventEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnEventEditChange);
		TagEdit.ChangeDelegate = FDelegate(this, (TDelegate)&WDlgSearchActors::OnTagEditChange);
		RefreshActorList();
		unguard;
	}
	virtual void OnShowWindow( UBOOL bShow )
	{
		guard(WDlgSearchActors::OnShowWindow);
		WWindow::OnShowWindow( bShow );
		RefreshActorList();
		unguard;
	}
	virtual void DoModeless( UBOOL bShow )
	{
		guard(WDlgSearchActors::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_SEARCH), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show( bShow );
		unguard;
	}
	void RefreshActorList( void )
	{
		guard(WDlgSearchActors::RefreshActorList);
		ActorList.Empty();
		LockWindowUpdate( ActorList.hWnd );

		FString Name, Event, Tag;
		HWND hwndFocus = ::GetFocus();

		Name = NameEdit.GetText();
		Event = EventEdit.GetText();
		Tag = TagEdit.GetText();

		if( GUnrealEd
				&& GUnrealEd->Level )
		{
			for( INT i = 0 ; i < GUnrealEd->Level->Actors.Num() ; ++i )
			{
				AActor* pActor = GUnrealEd->Level->Actors(i);
				if( pActor )
				{
					FString ActorName = pActor->GetName(),
						ActorEvent = *(pActor->Event),
						ActorTag = *(pActor->Tag);
					if( Name != ActorName.Left( Name.Len() ) )
						continue;
					if( Event.Len() && Event != ActorEvent.Left( Event.Len() ) )
						continue;
					if( Tag.Len() && Tag != ActorTag.Left( Tag.Len() ) )
						continue;

					ActorList.AddString( pActor->GetName() );
				}
			}
		}

		LockWindowUpdate( NULL );
		::SetFocus( hwndFocus );
		unguard;
	}
	UBOOL OnClose()
	{
		guard(WDlgSearchActors::OnClose);
		Show(0);
		return 1;
		unguard;
	}
	void OnActorListDblClick()
	{
		guard(WDlgSearchActors::OnActorListDblClick);
		GUnrealEd->SelectNone( GUnrealEd->Level, 0 );
		GUnrealEd->Exec( *FString::Printf(TEXT("CAMERA ALIGN NAME=%s"), *(ActorList.GetString( ActorList.GetCurrent()) ) ) );
		GUnrealEd->NoteSelectionChange( GUnrealEd->Level );
		unguard;
	}
	void OnDeleteActor()
	{
		OnActorListDblClick();
		GUnrealEd->Exec( TEXT("DELETE") );
		RefreshActorList();
	}
	void OnNameEditChange()
	{
		guard(WDlgSearchActors::OnNameEditChange);
		RefreshActorList();
		unguard;
	}
	void OnEventEditChange()
	{
		guard(WDlgSearchActors::OnEventEditChange);
		RefreshActorList();
		unguard;
	}
	void OnTagEditChange()
	{
		guard(WDlgSearchActors::OnTagEditChange);
		RefreshActorList();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
