/*=============================================================================
	ButtonBar : Class for handling the left hand button bar
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

extern WDlgAddSpecial* GDlgAddSpecial;

#define BUTTON_WIDTH	32
#define BUTTON_HEIGHT	32
#define CAPTION_HEIGHT	20

#define IDPB_MODE_CAMERA		19000
#define IDPB_MODE_SCALE			19001
#define IDPB_MODE_ROTATE		19002
#define IDPB_MODE_VERTEX_EDIT	19003
#define IDPB_MODE_BRUSH_CLIP	19004
#define IDPB_MODE_FACE_DRAG		19005
#define IDPB_TEXTURE_PAN		19006
#define IDPB_TEXTURE_ROTATE		19007
#define IDPB_MODE_ADD			19008
#define IDPB_MODE_SUBTRACT		19009
#define IDPB_MODE_INTERSECT		19010
#define IDPB_MODE_DEINTERSECT	19011
#define IDPB_MODE_POLYGON		19012
#define IDPB_MODE_TERRAINEDIT	19013
#define IDPB_MODE_MATINEE		19014
#define IDPB_ADD_VOLUME			19015
#define IDPB_MODE_GEOMETRY		19016
#define IDPB_ADD_SPECIAL		19020
#define IDPB_ADD_MOVER			19021
#define IDSC_LABEL				19022
#define IDPB_SHOW_SELECTED		19023
#define IDPB_HIDE_SELECTED		19024
#define IDPB_SHOW_ALL			19025
#define IDPB_INVERT_SELECTION	19026
#define IDPB_BRUSHCLIP			19027
#define IDPB_BRUSHCLIP_SPLIT	19028
#define IDPB_BRUSHCLIP_FLIP		19029
#define IDPB_BRUSHCLIP_DELETE	19030
#define IDPB_EXPAND				19031
#define IDSB_SCROLLBAR2			19032
#define IDPB_CAMERA_SPEED		19033
#define IDPB_MODE_ADDSTATICMESH	19034
#define IDPB_ADD_ANTIPORTAL		19035

#define IDPB_NEWMODE_CAMERA		19050

#define IDPB_USER_DEFINED		19075
#define IDPB_USER_DEFINED_MAX	19099
#define IDPB_BRUSH_BUILDERS		19100
#define IDMN_MOVER_TYPES		19200
#define IDMN_MOVER_TYPES_MAX	19250
#define IDMN_VOLUME_TYPES		19300
#define IDMN_VOLUME_TYPES_MAX	19350

typedef struct {
	INT ID;				// The command that is sent when the button is clicked
	HBITMAP hbm;		// The graphic that goes on the button
	FString Text;		// Differs based on Type (ToolTip or Label text)
	HWND hWnd;
	UBrushBuilder* Builder;
	UClass* Class;
	RECT rc;
	WWindow* pControl;
	FString ExecCommand;
} WBB_Button;

typedef struct {
	FString Name;
	INT ID;			// Starting at IDMN_MOVER_TYPES
} WBB_MoverType;

typedef struct {
	FString Name;
	INT ID;			// Starting at IDMN_VOLUME_TYPES
} WBB_VolumeType;

// --------------------------------------------------------------
//
// WBUTTONGROUP
// - Holds a group of related buttons
// - can be expanded/contracted
//
// --------------------------------------------------------------

enum ButtonGroupState {
	BGSTATE_DOWN	= 0,
	BGSTATE_UP		= 1
};

class WButtonGroup : public WWindow
{
	DECLARE_WINDOWCLASS(WButtonGroup,WWindow,Window)
	
	FString Caption;
	WButton* pExpandButton;
	TArray<WBB_Button> Buttons;
	HBITMAP hbmDown, hbmUp;
	INT iState, LastX, LastY;
	WToolTip* ToolTipCtrl;
	FString GroupName;
	TArray<WBB_MoverType> MoverTypes;
	TArray<WBB_VolumeType> VolumeTypes;
	WDlgBrushBuilder* pDBB;
	HBITMAP hbmCamSpeed[3];

	// Structors.
	WButtonGroup( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		iState = BGSTATE_DOWN;
		hbmDown = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_DOWN_ARROW), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmDown);
		hbmUp = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_UP_ARROW), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmUp);
		hbmCamSpeed[0] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED1), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[0]);
		hbmCamSpeed[1] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED2), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[1]);
		hbmCamSpeed[2] = (HBITMAP)LoadImageA( hInstance, MAKEINTRESOURCEA(IDBM_CAMSPEED3), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR );	check(hbmCamSpeed[2]);
		LastX = 2;
		LastY = CAPTION_HEIGHT;
		pExpandButton = NULL;
		pDBB = NULL;
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WButtonGroup::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0,
			320, 200,
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		SendMessageX( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );

		if(!GConfig->GetInt( TEXT("Groups"), *GroupName, iState, TEXT("UnrealEd.ini") ))	iState = BGSTATE_DOWN;
		UpdateButton();
		unguard;
	}
	void OnDestroy()
	{
		guard(WButtonGroup::OnDestroy);

		for( INT x = 0 ; x < Buttons.Num() ; ++x )
		{
			delete Buttons(x).pControl;
		}
		if( pExpandButton )
		{
			DestroyWindow( pExpandButton->hWnd );
			delete pExpandButton;
		}
		delete pDBB;
		DeleteObject(hbmDown);
		DeleteObject(hbmUp);
		DeleteObject(hbmCamSpeed[0]);
		DeleteObject(hbmCamSpeed[1]);
		DeleteObject(hbmCamSpeed[2]);
		delete ToolTipCtrl;
		WWindow::OnDestroy();
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WButtonGroup::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorIdX(NULL,IDC_CROSS));
		return 0;
		unguard;
	}
	void OnCreate()
	{
		guard(WButtonGroup::OnCreate);

		ToolTipCtrl = new WToolTip(this);
		ToolTipCtrl->OpenWindow();

		pExpandButton = new WButton( this, IDPB_EXPAND, FDelegate(this, (TDelegate)&WButtonGroup::OnExpandButton) );
		pExpandButton->OpenWindow( 1, 0, 0, 19, 19, TEXT(""), 1, BS_OWNERDRAW );
		UpdateButton();

		WWindow::OnCreate();
		unguard;
	}
	void OnExpandButton()
	{
		guard(WButtonGroup::OnExpandButton);
		iState = (iState == BGSTATE_DOWN) ? BGSTATE_UP : BGSTATE_DOWN;
		SendMessageX( OwnerWindow->hWnd, WM_COMMAND, WM_EXPANDBUTTON_CLICKED, 0L );
		UpdateButton();
		unguard;
	}
	void UpdateButton()
	{
		guard(WButtonGroup::UpdateButton);
		pExpandButton->SetBitmap( (iState == BGSTATE_DOWN) ? hbmUp : hbmDown );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WButtonGroup::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		RECT rect;
		::GetClientRect( hWnd, &rect );

		::MoveWindow( pExpandButton->hWnd, rect.right - 19, 0, 19, 19, 1 );

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnPaint()
	{
		guard(WButtonGroup::OnPaint);

		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		HBRUSH brushBack = CreateSolidBrush( RGB(128,128,128) );

		RECT rect;
		::GetClientRect( hWnd, &rect );

		FillRect( hDC, &rect, brushBack );
		MyDrawEdge( hDC, &rect, 1 );

		rect.top += 9;		rect.bottom = rect.top + 3;
		rect.left += 3;		rect.right -= 22;
		MyDrawEdge( hDC, &rect, 1 );

		EndPaint( *this, &PS );

		DeleteObject( brushBack );

		unguard;
	}
	// Adds a button onto the bar.  The button will be positioned automatically.
	void AddButton( INT _iID, UBOOL bAutoCheck, FString BMPFilename, FString Text, UClass* Class, UBOOL bNewLine, FString ExecCommand = TEXT("") )
	{
		guard(WButtonGroup::AddButton);

		if( bNewLine )
		{
			LastY += BUTTON_HEIGHT / 2;
			if( LastX >= BUTTON_WIDTH && LastX < (BUTTON_WIDTH * 2) )
				LastY += BUTTON_HEIGHT;

			LastX = 2;
		}

		// Add the button into the array and set it up
		new(Buttons)WBB_Button;
		WBB_Button* pWBButton = &(Buttons(Buttons.Num() - 1));
		check(pWBButton);

		if( BMPFilename.Len() )
		{
			FString Filename = *FString::Printf(TEXT("editorres\\%s.bmp"), *BMPFilename);
			pWBButton->hbm = (HBITMAP)LoadImageA( hInstance, appToAnsi( *Filename ), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
			if(!pWBButton->hbm)
				pWBButton->hbm = (HBITMAP)LoadImageA( hInstance, "BBGeneric", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
		}
		else
			pWBButton->hbm = NULL;

		pWBButton->ExecCommand = ExecCommand;
		pWBButton->ID = _iID;
		pWBButton->Text = Text;
		pWBButton->Class = Class;

		if( pWBButton->ID >= IDPB_BRUSH_BUILDERS )
			pWBButton->Builder = ConstructObject<UBrushBuilder>(Class);
		else
			pWBButton->Builder = NULL;

		// Create the physical button/label
		WCheckBox* pButton = new WCheckBox( this, _iID );
		pWBButton->pControl = pButton;
		pButton->OpenWindow( 1, LastX, LastY, BUTTON_WIDTH, BUTTON_HEIGHT, NULL, bAutoCheck, 1, BS_PUSHLIKE | BS_OWNERDRAW );
		pButton->UDNHelpTopic = _iID;
		pButton->SetBitmap( pWBButton->hbm );

		pWBButton->hWnd = pButton->hWnd;

		// Update position data
		LastX += BUTTON_WIDTH;
		if( LastX >= BUTTON_WIDTH * 2 )
		{
			LastX = 2;
			LastY += BUTTON_HEIGHT;
		}

		ToolTipCtrl->AddTool( pButton->hWnd, Text, _iID );

		unguard;
	}
	INT GetFullHeight()
	{
		guard(WButtonGroup::GetFullHeight);

		if( iState == BGSTATE_DOWN )
			return CAPTION_HEIGHT + (((Buttons.Num() + 1) / 2) * BUTTON_HEIGHT);
		else
			return CAPTION_HEIGHT;

		unguard;
	}
	void AddVolume()
	{
		guard(WButtonGroup::AddVolume);

		// Create a context menu with all the available kinds of volumes on it.
		CreateVolumeTypeList();

		HMENU menu = CreatePopupMenu();
		MENUITEMINFOA mif;

		mif.cbSize = sizeof(MENUITEMINFO);
		mif.fMask = MIIM_TYPE | MIIM_ID;
		mif.fType = MFT_STRING;

		for( INT x = 0 ; x < VolumeTypes.Num() ; ++x )
		{
			WBB_VolumeType* pwbbvt = &(VolumeTypes(x));

			mif.dwTypeData = TCHAR_TO_ANSI( *(pwbbvt->Name) );
			mif.wID = pwbbvt->ID;

			InsertMenuItemA( menu, 99999, FALSE, &mif );
		}

		POINT point;
		::GetCursorPos(&point);
		TrackPopupMenu( menu,
			TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
			point.x, point.y, 0,
			hWnd, NULL);
		DestroyMenu( menu );

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WButtonGroup::OnCommand);

		if( Command >= IDMN_MOVER_TYPES && Command <= IDMN_MOVER_TYPES_MAX )
		{
			// Figure out which mover was chosen
			for( INT x = 0 ; x < MoverTypes.Num() ; ++x )
			{
				WBB_MoverType* pwbbmt = &(MoverTypes(x));
				if( pwbbmt->ID == Command )
				{
					GUnrealEd->Exec( *FString::Printf(TEXT("BRUSH ADDMOVER CLASS=%s"), pwbbmt->Name ) );
					break;
				}
			}
		}

		if( Command >= IDMN_VOLUME_TYPES && Command <= IDMN_VOLUME_TYPES_MAX )
		{
			// Figure out which volume type was chosen
			for( INT x = 0 ; x < VolumeTypes.Num() ; ++x )
			{
				WBB_VolumeType* pwbbvt = &(VolumeTypes(x));
				if( pwbbvt->ID == Command )
				{
					GUnrealEd->Exec( *FString::Printf(TEXT("BRUSH ADDVOLUME CLASS=%s"), pwbbvt->Name ) );
					break;
				}
			}
		}

		switch( Command )
		{
			case WM_BB_RCLICK:
				{
					switch( LastlParam ) {

						case IDPB_ADD_MOVER:
						{
							// Create a context menu with all the available kinds of movers on it.
							CreateMoverTypeList();

							HMENU menu = CreatePopupMenu();
							MENUITEMINFOA mif;

							mif.cbSize = sizeof(MENUITEMINFO);
							mif.fMask = MIIM_TYPE | MIIM_ID;
							mif.fType = MFT_STRING;

							for( INT x = 0 ; x < MoverTypes.Num() ; ++x )
							{
								WBB_MoverType* pwbbmt = &(MoverTypes(x));

								mif.dwTypeData = TCHAR_TO_ANSI( *(pwbbmt->Name) );
								mif.wID = pwbbmt->ID;

								InsertMenuItemA( menu, 99999, FALSE, &mif );
							}

							POINT point;
							::GetCursorPos(&point);
							TrackPopupMenu( menu,
								TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
								point.x, point.y, 0,
								hWnd, NULL);
							DestroyMenu( menu );
						}
						break;

						case IDPB_ADD_VOLUME:
						{
							AddVolume();
						}
						break;

						default:
						{
							// Right clicking a brush builder button will bring up it's editable fields.
							WBB_Button* pwbb;

							for( INT x = 0 ; x < Buttons.Num() ; ++x )
							{
								pwbb = &(Buttons(x));
								if( pwbb->ID == LastlParam
										&& pwbb->Builder)
								{
									delete pDBB;
									pDBB = new WDlgBrushBuilder( NULL, this, pwbb->Builder );
									pDBB->DoModeless(1);
									break;
								}
							}
						}
						break;
					}
				}
				break;

			default:
				switch( HIWORD(Command) ) {

					case BN_CLICKED:
						ButtonClicked(LOWORD(Command));
						break;

					default:
						WWindow::OnCommand(Command);
						break;
				}
				break;
		}

		unguard;
	}
	void CreateMoverTypeList()
	{
		guard(WButtonGroup::CreateMoverTypeList);

		MoverTypes.Empty();

		INT ID = IDMN_MOVER_TYPES;

		UClass *Parent = NULL;
		ParseObject<UClass>(TEXT("PARENT=MOVER"),TEXT("PARENT="),Parent,ANY_PACKAGE);

		if( Parent )
			for( TObjectIterator<UClass> It ; It ; ++It )
				if( It->IsChildOf(Parent) )
				{ 
					new(MoverTypes)WBB_MoverType();
					WBB_MoverType* pwbbmt = &(MoverTypes(MoverTypes.Num() - 1));
					pwbbmt->ID = ID;
					pwbbmt->Name = It->GetName();
					ID++;
				}

		unguard;
	}
	void CreateVolumeTypeList()
	{
		guard(WButtonGroup::CreateVolumeTypeList);

		VolumeTypes.Empty();

		INT ID = IDMN_VOLUME_TYPES;

		// Now add the child classes of "volume"
		UClass *Parent = NULL;
		ParseObject<UClass>(TEXT("PARENT=VOLUME"),TEXT("PARENT="),Parent,ANY_PACKAGE);

		if( Parent )
			for( TObjectIterator<UClass> It ; It ; ++It )
				if(It->IsChildOf(Parent))
				{
					new(VolumeTypes)WBB_VolumeType();
					WBB_VolumeType* pwbbvt = &(VolumeTypes(VolumeTypes.Num() - 1));
					pwbbvt->ID = ID;
					pwbbvt->Name = It->GetName();
					ID++;
				}

		unguard;
	}
	// Searches the list for the button that was clicked, and executes the appropriate command.
	void ButtonClicked( INT ID )
	{
		guard(WButtonGroup::ButtonClicked);

		if( ID >= IDPB_USER_DEFINED && ID <= IDPB_USER_DEFINED_MAX )
		{
			for( INT x = 0 ; x < Buttons.Num() ; ++x )
			{
				WBB_Button* pwbb = &(Buttons(x));
				if( pwbb->ID == ID )
				{
					// Parse the string in case it contains multiple commands

					TArray<FString> StringArray;
					pwbb->ExecCommand.ParseIntoArray( TEXT("|"), &StringArray );

					// Send each command

					for( INT x = 0 ; x < StringArray.Num() ; ++x )
						GUnrealEd->Exec( *StringArray(x) );
					return;
				}
			}
		}

		switch( ID )
		{
			case IDPB_CAMERA_SPEED:
				{
					WCheckBox* pButton = GetButton(IDPB_CAMERA_SPEED);	check(pButton);
					if( GUnrealEd->MovementSpeed == 1 )
						GUnrealEd->Exec( TEXT("MODE SPEED=4") );
					else if( GUnrealEd->MovementSpeed == 4 )
						GUnrealEd->Exec( TEXT("MODE SPEED=16") );
					else
						GUnrealEd->Exec( TEXT("MODE SPEED=1") );
				}
				break;

			case IDPB_MODE_CAMERA:
				GUnrealEd->Exec(TEXT("MODE CAMERAMOVE"));
				break;

			case IDPB_MODE_SCALE:
				GUnrealEd->Exec(TEXT("MODE ACTORSNAP"));
				break;

			case IDPB_MODE_ROTATE:
				GUnrealEd->Exec(TEXT("MODE ACTORROTATE"));
				break;

			case IDPB_MODE_VERTEX_EDIT:
				GUnrealEd->Exec(TEXT("MODE VERTEXEDIT"));
				break;

			case IDPB_MODE_POLYGON:
				GUnrealEd->Exec(TEXT("MODE POLYGON"));
				break;

			case IDPB_MODE_GEOMETRY:
				GUnrealEd->Exec(TEXT("MODE GEOMETRY"));
				break;

			case IDPB_MODE_TERRAINEDIT:
				GUnrealEd->Exec(TEXT("MODE TERRAINEDIT"));
				break;

			case IDPB_MODE_MATINEE:
				GUnrealEd->Exec(TEXT("MODE MATINEE"));
				break;

			case IDPB_MODE_BRUSH_CLIP:
				GUnrealEd->Exec(TEXT("MODE BRUSHCLIP"));
				break;

			case IDPB_MODE_FACE_DRAG:
				GUnrealEd->Exec(TEXT("MODE FACEDRAG"));
				break;

			case IDPB_NEWMODE_CAMERA:
				GUnrealEd->Exec(TEXT("MODE NEWCAMERAMOVE"));
				break;

			case IDPB_SHOW_SELECTED:
				GUnrealEd->Exec( TEXT("ACTOR HIDE UNSELECTED") );
				break;

			case IDPB_HIDE_SELECTED:
				GUnrealEd->Exec( TEXT("ACTOR HIDE SELECTED") );
				break;

			case IDPB_SHOW_ALL:
				GUnrealEd->Exec( TEXT("ACTOR UNHIDE ALL") );
				break;

			case IDPB_INVERT_SELECTION:
				GUnrealEd->Exec( TEXT("ACTOR SELECT INVERT") );
				break;

			case IDPB_BRUSHCLIP:
				GUnrealEd->Exec( TEXT("BRUSHCLIP") );
				GUnrealEd->RedrawLevel( GUnrealEd->Level );
				break;

			case IDPB_BRUSHCLIP_SPLIT:
				GUnrealEd->Exec( TEXT("BRUSHCLIP SPLIT") );
				GUnrealEd->RedrawLevel( GUnrealEd->Level );
				break;

			case IDPB_BRUSHCLIP_FLIP:
				GUnrealEd->Exec( TEXT("BRUSHCLIP FLIP") );
				GUnrealEd->RedrawLevel( GUnrealEd->Level );
				break;

			case IDPB_BRUSHCLIP_DELETE:
				GUnrealEd->Exec( TEXT("BRUSHCLIP DELETE") );
				GUnrealEd->RedrawLevel( GUnrealEd->Level );
				break;

			case IDPB_TEXTURE_PAN:
				GUnrealEd->Exec(TEXT("MODE TEXTUREPAN"));
				break;

			case IDPB_TEXTURE_ROTATE:
				GUnrealEd->Exec(TEXT("MODE TEXTUREROTATE"));
				break;

			case IDPB_MODE_ADD:
				GUnrealEd->Exec( TEXT("BRUSH ADD") );
				break;

			case IDPB_MODE_ADDSTATICMESH:
				GBrowserStaticMesh->CreateFromSelection();
				break;

			case IDPB_MODE_SUBTRACT:
				GUnrealEd->Exec( TEXT("BRUSH SUBTRACT") );
				break;

			case IDPB_MODE_INTERSECT:
				GUnrealEd->Exec( TEXT("BRUSH FROM INTERSECTION") );
				break;

			case IDPB_MODE_DEINTERSECT:
				GUnrealEd->Exec( TEXT("BRUSH FROM DEINTERSECTION") );
				break;

			case IDPB_ADD_ANTIPORTAL:
				GUnrealEd->Exec( TEXT("BRUSH ADDANTIPORTAL") );
				break;

			case IDPB_ADD_SPECIAL:
				{
					if( !GDlgAddSpecial )
					{
						GDlgAddSpecial = new WDlgAddSpecial( NULL, this );
						GDlgAddSpecial->DoModeless(1);
					}
					else
						GDlgAddSpecial->Show(1);
				}
				break;

			case IDPB_ADD_MOVER:
				GUnrealEd->Exec( TEXT("BRUSH ADDMOVER") );
				break;

			case IDPB_ADD_VOLUME:
				AddVolume();
				break;

			default:
				// A brush builder must have been clicked.  Loop through the
				// list and see which one.
				WBB_Button* pwbb;

				for( INT x = 0 ; x < Buttons.Num() ; ++x )
				{
					pwbb = &(Buttons(x));
					if( pwbb->ID == ID )
					{
						check( pwbb->Builder );
						UBOOL GIsSavedScriptableSaved = 1;
						Exchange(GIsScriptable,GIsSavedScriptableSaved);
						pwbb->Builder->eventBuild();
						Exchange(GIsScriptable,GIsSavedScriptableSaved);
					}
				}
				break;
		}

		UpdateButtons();

		unguard;
	}
	// Updates the states of the buttons to match editor settings.
	void UpdateButtons()
	{
		guard(WButtonGroup::UpdateButtons);

		if( GetDlgItem( hWnd, IDPB_MODE_CAMERA ) )	// Make sure we're in the right group to avoid flicker
		{
			GetButton( IDPB_MODE_CAMERA )->SetCheck( GUnrealEd->Mode==EM_ViewportMove );
			GetButton( IDPB_MODE_SCALE )->SetCheck( GUnrealEd->Mode==EM_ActorSnapScale );
			GetButton( IDPB_MODE_ROTATE )->SetCheck( GUnrealEd->Mode==EM_ActorRotate );
			GetButton( IDPB_TEXTURE_PAN )->SetCheck( GUnrealEd->Mode==EM_TexturePan );
			GetButton( IDPB_TEXTURE_ROTATE )->SetCheck( GUnrealEd->Mode==EM_TextureRotate );
			GetButton( IDPB_MODE_VERTEX_EDIT )->SetCheck( GUnrealEd->Mode==EM_VertexEdit );
			GetButton( IDPB_MODE_BRUSH_CLIP )->SetCheck( GUnrealEd->Mode==EM_BrushClip );
			GetButton( IDPB_MODE_POLYGON)->SetCheck( GUnrealEd->Mode==EM_Polygon );
			GetButton( IDPB_MODE_FACE_DRAG )->SetCheck( GUnrealEd->Mode==EM_FaceDrag );
			GetButton( IDPB_MODE_TERRAINEDIT)->SetCheck( GUnrealEd->Mode==EM_TerrainEdit );
			GetButton( IDPB_MODE_MATINEE)->SetCheck( GUnrealEd->Mode==EM_Matinee );
			GetButton( IDPB_MODE_GEOMETRY)->SetCheck( GUnrealEd->Mode==EM_Geometry );

			// Force all the buttons to paint themselves
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_CAMERA ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_SCALE ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_ROTATE ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_TEXTURE_PAN ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_TEXTURE_ROTATE ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_VERTEX_EDIT ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_BRUSH_CLIP ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_FACE_DRAG ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_POLYGON ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_TERRAINEDIT ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_MATINEE ), NULL, 1 );
			InvalidateRect( GetDlgItem( hWnd, IDPB_MODE_GEOMETRY ), NULL, 1 );
		}

		if( GetDlgItem( hWnd, IDPB_NEWMODE_CAMERA ) )	// Make sure we're in the right group to avoid flicker
		{
			GetButton( IDPB_NEWMODE_CAMERA )->SetCheck( GUnrealEd->Mode==EM_NewCameraMove );

			// Force all the buttons to paint themselves
			InvalidateRect( GetDlgItem( hWnd, IDPB_NEWMODE_CAMERA ), NULL, 1 );
		}

		if( GetDlgItem( hWnd, IDPB_CAMERA_SPEED ) )
		{
			WCheckBox* pButton = GetButton(IDPB_CAMERA_SPEED);

			if( GUnrealEd->MovementSpeed == 1 )
				pButton->SetBitmap( hbmCamSpeed[0] );
			else if( GUnrealEd->MovementSpeed == 4 )
				pButton->SetBitmap( hbmCamSpeed[1] );
			else
				pButton->SetBitmap( hbmCamSpeed[2] );
			InvalidateRect( pButton->hWnd, NULL, 1 );
		}

		unguard;
	}
	WCheckBox* GetButton( INT InID )
	{
		guard(WButtonGroup::GetButton);
		for( INT x = 0 ; x < Buttons.Num() ; ++x )
		{
			WBB_Button* pwbb = &(Buttons(x));
			if(pwbb->ID == InID )
				return (WCheckBox*)pwbb->pControl;
		}

		check(0);	// this should never happen
		return NULL;
		unguard;
	}
	void RefreshBuilders()
	{
		guard(WButtonGroup::RecreateBuilders);

		for( INT x = 0 ; x < Buttons.Num() ; ++x )
		{
			WBB_Button* pwbb = &(Buttons(x));
			if(pwbb->ID >= IDPB_BRUSH_BUILDERS )
				pwbb->Builder = ConstructObject<UBrushBuilder>(pwbb->Class);
		}

		unguard;
	}
};

// --------------------------------------------------------------
//
// WBUTTONBAR
//
// --------------------------------------------------------------

class WButtonBar : public WWindow
{
	DECLARE_WINDOWCLASS(WButtonBar,WWindow,Window)

	TArray<WButtonGroup> Groups;
	INT LastX, LastY;
	WThinScrollBar* ScrollBar;
	INT ScrollPos;

	// Structors.
	WButtonBar( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		ScrollBar = NULL;
		ScrollPos = 0;
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WButtonBar::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			320,
			200,
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		SendMessageX( *this, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(0,0) );
		unguard;
	}
	void RefreshScrollBar( void )
	{
		guard(WButtonBar::RefreshScrollBar);

		if( !ScrollBar ) return;

		RECT rect;
		::GetClientRect( hWnd, &rect );

		if( (rect.bottom < GetHeightOfAllGroups()
				&& !IsWindowEnabled( ScrollBar->hWnd ) )
				|| (rect.bottom >= GetHeightOfAllGroups()
				&& IsWindowEnabled( ScrollBar->hWnd ) ) )
		{
			ScrollPos = 0;
		}

		ScrollBar->SetRange( GetHeightOfAllGroups() );
		ScrollBar->SetPos( ScrollPos, (rect.bottom / (FLOAT)GetHeightOfAllGroups()) * 100 );

		EnableWindow( ScrollBar->hWnd, (rect.bottom < GetHeightOfAllGroups()) );
		PositionChildControls();

		unguard;
	}
	INT GetHeightOfAllGroups()
	{
		guard(WButtonBar::GetHeightOfAllGroups);

		INT Height = 0;

		for( INT x = 0 ; x < Groups.Num() ; ++x )
			Height += Groups(x).GetFullHeight();

		return Height;

		unguard;
	}
	void OnDestroy()
	{
		guard(WButtonBar::OnDestroy);
		delete ScrollBar;
		Groups.Empty();
		WWindow::OnDestroy();
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WButtonBar::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorIdX(NULL,IDC_ARROW));
		return 0;
		unguard;
	}
	// Adds a button group.
	WButtonGroup* AddGroup( FString GroupName )
	{
		guard(WButtonBar::AddGroup);
		new(Groups)WButtonGroup( TEXT(""), this );
		WButtonGroup* pbuttongroup = &(Groups(Groups.Num() - 1));
		pbuttongroup->GroupName = GroupName;
		check(pbuttongroup);
		return pbuttongroup;
		unguard;
	}
	// Doing things like loading new maps and such will cause us to have to recreate
	// brush builder objects.
	void RefreshBuilders()
	{
		guard(WButtonBar::RecreateBuilders);

		for( INT x = 0 ; x < Groups.Num() ; ++x )
			Groups(x).RefreshBuilders();

		unguard;
	}
	void OnCreate()
	{
		guard(WButtonBar::OnCreate);
		WWindow::OnCreate();

		// Load the buttons onto the bar.
		WButtonGroup* Group;

		Group = AddGroup( TEXT("Modes") );
		Group->OpenWindow();
		Group->AddButton( IDPB_MODE_CAMERA, 0, TEXT("ModeCamera"), TEXT("Camera Movement"), NULL, 0 );
		Group->AddButton( IDPB_MODE_VERTEX_EDIT, 0, TEXT("ModeVertex"), TEXT("Vertex Editing"), NULL, 0 );
		Group->AddButton( IDPB_MODE_SCALE, 0, TEXT("ModeScale"), TEXT("Actor Scaling"), NULL, 0 );
		Group->AddButton( IDPB_MODE_ROTATE, 0, TEXT("ModeRotate"), TEXT("Actor Rotate"), NULL, 0 );
		Group->AddButton( IDPB_TEXTURE_PAN, 0, TEXT("TexturePan"), TEXT("Texture Pan"), NULL, 0 );
		Group->AddButton( IDPB_TEXTURE_ROTATE, 0, TEXT("TextureRotate"), TEXT("Texture Rotate"), NULL, 0 );
		Group->AddButton( IDPB_MODE_BRUSH_CLIP, 0, TEXT("ModeBrushClip"), TEXT("Brush Clipping"), NULL, 0 );
		Group->AddButton( IDPB_MODE_POLYGON, 0, TEXT("ModePolygon"), TEXT("Freehand Polygon Drawing"), NULL, 0 );
		Group->AddButton( IDPB_MODE_FACE_DRAG, 0, TEXT("ModeFaceDrag"), TEXT("Face Drag"), NULL, 0 );
		Group->AddButton( IDPB_MODE_TERRAINEDIT, 0, TEXT("ModeTerrainEdit"), TEXT("Terrain Editing"), NULL, 0 );
		Group->AddButton( IDPB_MODE_MATINEE, 0, TEXT("ModeMatinee"), TEXT("Matinee"), NULL, 0 );
		Group->AddButton( IDPB_MODE_GEOMETRY, 0, TEXT("ModeGeometry"), TEXT("Geometry Editing"), NULL, 0 );

		//Group = AddGroup( TEXT("NewModes") );
		//Group->OpenWindow();
		//for( INT x = 0 ; x < GEdModeTools->Modes.Num() ; ++x )
		//	Group->AddButton( IDPB_NEWMODE_CAMERA, 0, GEdModeTools->Modes(x).BitmapName, GEdModeTools->Modes(x).ToolTip, NULL, 0 );

		Group = AddGroup( TEXT("Clipping") );
		Group->OpenWindow();
		Group->AddButton( IDPB_BRUSHCLIP, 0, TEXT("BrushClip"), TEXT("Clip Selected Brushes"), NULL, 0 );
		Group->AddButton( IDPB_BRUSHCLIP_SPLIT, 0, TEXT("BrushClipSplit"), TEXT("Split Selected Brushes"), NULL, 0 );
		Group->AddButton( IDPB_BRUSHCLIP_FLIP, 0, TEXT("BrushClipFlip"), TEXT("Flip Clipping Normal"), NULL, 0 );
		Group->AddButton( IDPB_BRUSHCLIP_DELETE, 0, TEXT("BrushClipDelete"), TEXT("Delete Clipping Markers"), NULL, 0 );

		INT ID = IDPB_BRUSH_BUILDERS;

		Group = AddGroup( TEXT("Builders") );
		Group->OpenWindow();
		for( TObjectIterator<UClass> ItC; ItC; ++ItC )
			if( ItC->IsChildOf(UBrushBuilder::StaticClass()) && !(ItC->ClassFlags&CLASS_Abstract) )
			{
				UBrushBuilder* ubb = ConstructObject<UBrushBuilder>( *ItC );
				if( ubb )
				{
					Group->AddButton( ID, 0, ubb->BitmapFilename, ubb->ToolTip, *ItC, 0 );
					ID++;
				}
			}

		Group = AddGroup( TEXT("CSG") );
		Group->OpenWindow();
		Group->AddButton( IDPB_MODE_ADD, 0, TEXT("ModeAdd"), TEXT("Add"), NULL, 0 );
		Group->AddButton( IDPB_MODE_SUBTRACT, 0, TEXT("ModeSubtract"), TEXT("Subtract"), NULL, 0 );
		Group->AddButton( IDPB_MODE_INTERSECT, 0, TEXT("ModeIntersect"), TEXT("Intersect"), NULL, 0 );
		Group->AddButton( IDPB_MODE_DEINTERSECT, 0, TEXT("ModeDeintersect"), TEXT("Deintersect"), NULL, 0 );
		Group->AddButton( IDPB_ADD_SPECIAL, 0, TEXT("AddSpecial"), TEXT("Add Special Brush"), NULL, 0 );
		Group->AddButton( IDPB_MODE_ADDSTATICMESH, 0, TEXT("ModeAddStaticMesh"), TEXT("Add Static Mesh"), NULL, 0 );
		Group->AddButton( IDPB_ADD_MOVER, 0, TEXT("AddMover"), TEXT("Add Mover Brush (right click for all mover types)"), NULL, 0 );
		Group->AddButton( IDPB_ADD_ANTIPORTAL, 0, TEXT("AddAntiPortal"), TEXT("Add Antiportal"), NULL, 0 );
		Group->AddButton( IDPB_ADD_VOLUME, 0, TEXT("ModeAddVolume"), TEXT("Volume"), NULL, 0 );

		Group = AddGroup( TEXT("Misc") );
		Group->OpenWindow();
		Group->AddButton( IDPB_SHOW_SELECTED, 0, TEXT("ShowSelected"), TEXT("Show Selected Actors Only"), NULL, 0 );
		Group->AddButton( IDPB_HIDE_SELECTED, 0, TEXT("HideSelected"), TEXT("Hide Selected Actors"), NULL, 0 );
		Group->AddButton( IDPB_SHOW_ALL, 0, TEXT("ShowAll"), TEXT("Show All Actors"), NULL, 0 );
		Group->AddButton( IDPB_INVERT_SELECTION, 0, TEXT("InvertSelections"), TEXT("Invert Selection"), NULL, 0 );
		Group->AddButton( IDPB_CAMERA_SPEED, 0, TEXT("ModeCamera"), TEXT("Change Camera Speed"), NULL, 0 );

		Group = AddGroup( TEXT("UserDefined") );
		Group->OpenWindow();

		INT NumButtons;
		if(!GConfig->GetInt( TEXT("UserDefinedGroup"), TEXT("NumButtons"), NumButtons, TEXT("UnrealEd.ini") ))	NumButtons = 0;

		if( NumButtons )
		{
			for( INT x = 0 ; x < NumButtons ; ++x )
			{
				FString ButtonDef;
				FString ButtonName = FString::Printf(TEXT("Button%d"), x);

				TArray<FString> Fields;
				GConfig->GetString( TEXT("UserDefinedGroup"), *ButtonName, ButtonDef, TEXT("UnrealEd.ini") );
				ButtonDef.ParseIntoArray( TEXT(","), &Fields );

				Group->AddButton( IDPB_USER_DEFINED + x, 0, *Fields(1), *Fields(0), NULL, 0, *Fields(2) );
			}
		}

		ScrollBar = new WThinScrollBar( this, IDSB_SCROLLBAR2 );
		ScrollBar->OpenWindow( 64, 0, 4, 500 );
		ScrollBar->PosChangedDelegate = FDelegate(this, (TDelegate)&WButtonBar::OnScrollBarPosChanged);

		PositionChildControls();
		UpdateButtons();

		unguard;
	}
	void PositionChildControls()
	{
		guard(WButtonBar::PositionChildControls);

		RECT rect;
		INT LastY = -ScrollPos;

		::GetClientRect( hWnd, &rect );

		FDeferWindowPos dwp;

		// Figure out where each buttongroup window should go.
		for( INT x = 0 ; x < Groups.Num() ; ++x )
		{
			dwp.MoveWindow( Groups(x).hWnd, 0, LastY, rect.right - 4, Groups(x).GetFullHeight(), 1 );
			LastY += Groups(x).GetFullHeight();
		}

		dwp.MoveWindow( ScrollBar->hWnd, rect.right - 4, 0, 4, rect.bottom, 1 );

		unguard;
	}
	void OnPaint()
	{
		guard(WButtonBar::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		HBRUSH brushBack = CreateSolidBrush( RGB(128,128,128) );

		FRect Rect = GetClientRect();
		FillRect( hDC, Rect, brushBack );
		MyDrawEdge( hDC, Rect, 1 );

		EndPaint( *this, &PS );

		DeleteObject( brushBack );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WButtonBar::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		RefreshScrollBar();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WButtonBar::OnCommand);

		switch( Command )
		{
			case WM_EXPANDBUTTON_CLICKED:
			{
				RefreshScrollBar();
				PositionChildControls();
			}
			break;

			default:
				WWindow::OnCommand(Command);
				break;
		}

		unguard;
	}
	void UpdateButtons()
	{
		guard(WButtonBar::UpdateButtons);

		for( INT x = 0 ; x < Groups.Num() ; ++x )
			Groups(x).UpdateButtons();

		unguard;
	}
	void OnScrollBarPosChanged()
	{
		ScrollPos = ScrollBar->GetPos();
		RefreshScrollBar();
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
