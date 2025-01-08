/*=============================================================================
	ImportUI.cpp: UI import from text file code.
	Copyright (C) 2000-2001 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron R Leiby
=============================================================================*/

#include "UI.h"
#include "ImportUI.h"

//#undef NOTE
//#define NOTE NOTE1

THashTable<FString,UComponent*>	&GetEventScopeTable()	{ static THashTable<FString,UComponent*>	EventScopeTable	= THashTable<FString,UComponent*>();			return EventScopeTable;	}
THashTable<FString,UObject*>	&GetObjMap()			{ static THashTable<FString,UObject*>		ObjMap			= THashTable<FString,UObject*>();				return ObjMap;			}
THashTable<FString,FImage*>		&GetFImageMap()			{ static THashTable<FString,FImage*>		FImageMap		= THashTable<FString,FImage*>();				return FImageMap;		}
UMouseCursor*					&GetGMouse()			{ static UMouseCursor*						GMouse			= NULL;											return GMouse;			}
TArray<UMouseCursor*>			&GetGMouseList()		{ static TArray<UMouseCursor*>				GMouseList		= TArray<UMouseCursor*>();						return GMouseList;		}
UUIConsole*						&GetGConsole()			{ static UUIConsole*				 		GConsole		= NULL;											return GConsole;		}
FString							&GetGFilename()			{ static FString							GFilename		= FString();									return GFilename;		}
FString							&GetGUIPath()			{ static FString							GUIPath			= FString();									return GUIPath;			}
UBOOL							&GetGLocalize()			{ static UBOOL								GLocalize		= 0;											return GLocalize;		}
TArray<FString>					&GetGVarList()			{ static TArray<FString>					GVarList		= TArray<FString>();							return GVarList;		}
RealSectionData					&GetGVarMap()			{ static RealSectionData					GVarMap			= RealSectionData();							return GVarMap;			}
UComponent*						&GetGComponent()		{ static UComponent*						GComponent		= NULL;											return GComponent;		}
UBOOL							&GetGTransient()		{ static UBOOL								GTransient		= 0;											return GTransient;		}
FString							&GetGTokenSeparator()	{ static FString							GTokenSeparator	= TEXT(",");									return GTokenSeparator;	}


//------------------------------------------------------------------------------

const TCHAR*					GetIndentString()		
{
	static FString Total;
	static INT LastImportIndentLevel = -1;
	if( LastImportIndentLevel != ImportIndentLevel )
	{
		Total = TEXT("");
		for( INT i=0; i<ImportIndentLevel; i++ )
		{
			Total+=TEXT("  ");
		}
		LastImportIndentLevel = ImportIndentLevel;
	}
	return *Total;
}

//------------------------------------------------------------------------------
//
// Replaces registered variables in the given string.
//    Item=example,Aaron R Leiby
//    This is an %0% by %1%.
//    This is an example by Aaron R Leiby.
//

void DeVar( FString &S, SectionData Section )
{
	guard(DeVar);

	FString Left=TEXT("");
	FString Right=TEXT("");
	FString p=TEXT("%");

	for( INT i=0; i<GetGVarList().Num(); i++ )
	{
		NOTE(debugf(TEXT("Looking for %s"), p+FString::Printf(TEXT("%d"),i)+p));
		// If what we are looking for and what we are replacing is the same thing,
		// (example: "%2%" being replaced with "%2%")
		// then our work is already done and we don't need to do any replacing.
		// Attempting to do the replacement regardless would only result in an infinite loop!
		if( GetGVarList()(i) != FString::Printf(TEXT("%%%d%%"),i) )
			while( S.Split( p+FString::Printf(TEXT("%d"),i)+p, &Left, &Right ) )
				S = Left + GetGVarList()(i) + Right;
	}
	for( TMapBase<FString,FString>::TIterator VarIter( GetGVarMap() ); VarIter; ++VarIter )
		while( S.Split( FString::Printf(TEXT("%%%s%%"),*(VarIter.Key())), &Left, &Right ) )
			S = Left + VarIter.Value() + Right;

	unguard;
}

//------------------------------------------------------------------------------
UContainer* ImportMacro( SectionData Section )
{
	guard(UI::ImportMacro);

	UContainer* C=NULL;
	FString TemplateName=TEXT("");

	// Preserve existing list.
	TArray<FString> OldList;
	for( INT i=0; i<GetGVarList().Num(); i++ )
		new(OldList)FString(GetGVarList()(i));
	GetGVarList().Empty();

	BEGIN_PARSE

		PARSE(Container)	C=DynamicCastChecked<UContainer>(ImportComponent( NEXT_TOKEN, PARENT, Overrides ));	else appErrorf(TEXT("Macro definition does not specify Container to use."));
		PARSE(Component)	TemplateName=NEXT_TOKEN;															else appErrorf(TEXT("Macro definition does not specify Component to use."));

	END_PARSE

	PARSE_LIST_BEGIN(Item)

		// Add all the tokens as variables.
		while( *ParseText!=TEXT("") )
		{
			FString Parm=NEXT_TOKEN;
			new(GetGVarList())FString( Parm!=TEXT("") ? Parm : TEXT("NULL") );
		}

		C->AddComponent( ImportComponent( TemplateName, PARENT, Overrides ) );

		GetGVarList().Empty();

	PARSE_LIST_END

	// Restore existing list.
	for( INT i=0; i<OldList.Num(); i++ )
		new(GetGVarList())FString(OldList(i));
	
	return C;

	unguard;
}

static void ReadIn( RealSectionData& InData, RealSectionData &InVarMap )
{
	//the * dereferencing is to convert the retrieved FStrings into TCHAR*'s so they can be reinserted.
	//this is needed because TTypeInfo<FString>::ConstInitType is TCHAR*

	//iterate through Section, looking for keys beginning with %, and import the value
	for( TMapBase<FString,FString>::TIterator VarIter( InData ); VarIter; ++VarIter )
	{
		// If is of the form %Name
		if( VarIter.Key().Left( 1 ) == TEXT("%") )
		{
			FString LookupKey = VarIter.Key().Mid( 1 );
			FString SaveKey;
			//if of form %Name=Value, savekey == Value
			if( VarIter.Value() != TEXT("") )
			{
				SaveKey = VarIter.Value();
				// If they're doing Name=Value, it's to get past some parent use of Name= , so delete all existing Name's
				InData.Remove( *SaveKey );
			}
			else
				SaveKey = LookupKey;
			TArray<FString> List;
			InVarMap.MultiFind( *LookupKey, List );
			for( INT i=List.Num()-1; i>=0; i-- )
				InData.Add( *SaveKey, *List(i) );
		}
	}
}


//------------------------------------------------------------------------------
UComponent* ImportComponent( FString Name, UObject* Outer, FString* Overrides, UBOOL bReload )
{
	guard(UI::ImportComponent);
	NOTE(debugf(TEXT("Loading %s from %s..."), *Name, *GetGFilename() ));

	// Special case 'NULL' component.
	if( Name.Caps()==TEXT("NULL") || Name.Caps()==TEXT("NONE") || Name.Caps()==TEXT("NOTHING") )
		return CreateComponent<UComponent>(TEXT("NULL"));

	// Check for macro parameters first.
	FString Parms=TEXT("");
	FString Left=TEXT("");
	TArray<FString> OldList;
	UBOOL bUseOldList = false;
	RealSectionData OldMap;
	UBOOL bUseOldMap = false;
	if( Name.Split(TEXT(":"),&Left,&Parms) )
	{
		Name = Left;

		NOTE(debugf(TEXT("Parsing inline macro (%s):"),Parms));

		// Preserve existing list.
		bUseOldList = true;
		OldList = GetGVarList();
		GetGVarList().Empty();
		
		// Add all the tokens as variables.
		FString* ParseText = &Parms;
		for( INT i=0; i<Parms.Len(); i++ )
			if( Parms.GetCharArray()(i)=='/' )
				Parms.GetCharArray()(i)=',';
		while( *ParseText!=TEXT("") )
		{
			FString Parm=NEXT_TOKEN;
			NOTE(debugf(TEXT("    %s"),Parm));
			new(GetGVarList())FString( Parm!=TEXT("") ? Parm : TEXT("NULL") );
		}
	}

	// Check for scope and load from file instead.
	FString Filename=TEXT("");
	FString ComponentName=TEXT("");
	if( Name.Split(TEXT("."),&Filename,&ComponentName) )
		return GetGConsole()->LoadComponent(Filename,ComponentName,Overrides);

	// Find section for Component in current file.
#if 1 //NEW MJL: nested ini files
	if( Name == TEXT("") )
	{
		appErrorf(NAME_Warning, TEXT("Must specify a class/component name") );
		return NULL;
	}
	ImportIndentLevel++;
	SectionData Section = NULL;
	FString Temp = FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s") UI_FILE_EXTENSION, *GetGUIPath(), *GetGFilename() );
	SectionData OrigSection = GConfig->GetSectionPrivate(*Name,0,1,*Temp,bReload);
	if( OrigSection )
	{
		Section = new RealSectionData(*OrigSection);
	}
	else
	{
		Section = new RealSectionData();
		Section->Add( TEXT("Class"), *Name );
	}
#else
	FString Temp = FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s") UI_FILE_EXTENSION, *GetGUIPath(), *GetGFilename() );
	SectionData Section = GConfig->GetSectionPrivate(*Name,0,1,*Temp,bReload);
	if(!Section){ debugf(NAME_Warning, TEXT("Section not found: %s"), *Name ); return NULL; }
#endif

	NOTE(debugf(TEXT("%sName: %s"),GetIndentString(), *Name));
	NOTE(debugf(TEXT("%sOverrides:\n%s"),GetIndentString(), Overrides?*Overrides:TEXT("NULL")));

#if 1 //NEW MJL: nested ini files
	// handle sub-settings, import the values into the NewVarMap
	RealSectionData NewVarMap;
	if( Overrides && *Overrides != TEXT("") )
	{
		bUseOldMap = true;
		OldMap = GetGVarMap();

		TCHAR* Ptr = const_cast<TCHAR*>( **Overrides );

		//here-in lies some rather ugly, convoluted, painful code. :(
//begin copy from FConfigCacheIni.h with minor modifications
		UBOOL Done = 0;
		while( !Done )
		{
			while( *Ptr=='\r' || *Ptr=='\n' )
				Ptr++;
			//ensure that our data begins with a tab, which all overrides should
			check( *Ptr=='\t' );
			Ptr++;

			TCHAR* Start = Ptr;
			//the point to jump to when we realize we need to slurp more data
			InLoop:

			while( *Ptr && *Ptr!='\r' && *Ptr!='\n' )
				Ptr++;
			if( *Ptr==0 )
				Done = 1;

			//having slurped in all the data, if we aren't at the end of a file...
			if( !Done )
			{
				//store off a copy of the current location
				TCHAR* PtrCpy = Ptr;
				//then slurp in the newlines
				while( *PtrCpy && (*PtrCpy=='\r' || *PtrCpy=='\n') )
					PtrCpy++;
				//and ensure that the next  line also begins with a tab (see above)
				check( *PtrCpy=='\t' );
				PtrCpy++;
				//if there is a second tab, then it's an override of the last line...
				if( *PtrCpy == '\t' )
				{
					//so we need to move our pointer to this current point
					Ptr = PtrCpy;
					//and copy the rest of the data in the string back over this tab
					//this ensures that the upcoming data, when it becomes an override,
					//will only have one tab at the begining of the line, to allow nesting
					do
					{
						*PtrCpy = *(PtrCpy+1);
						PtrCpy++;
					} while( *PtrCpy != '\0' );
					//go back to the InLoop point to slurp in more data,
					//since the upcoming line is an override for the original line
					goto InLoop;
				}
			}
			//otherwise, at this point, our lookahead on the tabs is rolled back
			//and we are at the end of a given key=value pair, so store it away

			*Ptr++ = 0;
			if( *Start )
			{
				TCHAR* Value = appStrstr(Start,TEXT("="));
				if( Value )
				{
					*Value++ = 0;
					if( *Value=='\"' && Value[appStrlen(Value)-1]=='\"' )
					{
						Value++;
						Value[appStrlen(Value)-1]=0;
					}
					NewVarMap.Add( Start, Value );
				}
				else
				{
					NewVarMap.Add( Start, TEXT("") );
				}
			}
		}
//end copy
		//if it's anonymously constructed, put the tabbed-keyvalues into the section
		if( !OrigSection )
			for( TMultiMap<FString,FString>::TIterator It( NewVarMap ); It; ++It )
				Section->Add( *It.Key(), *It.Value() );

		// interpolate GVarMap into NewVarMap and *Section
		ReadIn( NewVarMap, GetGVarMap() );
		for( TMultiMap<FString,FString>::TIterator It( NewVarMap ); It; ++It )
			GetGVarMap().Add( *It.Key(), *It.Value() );
		ReadIn( *Section, GetGVarMap() );
	}
	else
	{
		ReadIn( *Section, GetGVarMap() );
	}
#endif
	for( TMultiMap<FString,FString>::TIterator It( *Section ); It; ++It )
	{
		DeVar(It.Value(), Section);
	}

	NOTE0
	(
		for( TMultiMap<FString,FString>::TIterator It3( *Section ); It3; ++It3 )
			debugf(TEXT("%sSection: %s::%s"), GetIndentString(), *It3.Key(), *It3.Value() );
		for( TMultiMap<FString,FString>::TIterator It4( GetGVarMap() ); It4; ++It4 )
			debugf(TEXT("%sGVarMap(): %s::%s"), GetIndentString(), *It4.Key(), *It4.Value() );
	)
	// Grab the class.
	FString	ClassName=TEXT(""),
			HelperName=TEXT("");
	BEGIN_PARSE

		PARSE(Class) ClassName=NEXT_TOKEN; else
		PARSE(Helper) HelperName=NEXT_TOKEN; else
		appErrorf(TEXT("Class not specified."));

	END_PARSE

	// Special cases.
	if( ClassName==TEXT("Macro") )
		return ImportMacro( Section );

	FString OrigClassName = ClassName;

#if 1 //new stuff
	UComponent* C;

	// deal with non-component helper objects.
	if(HelperName!=TEXT(""))
	{
		NOTE(debugf(TEXT("Loading non-component class: %s"),HelperName));

		// Prepend "UI." if no package specified.
		FString PackageStr = FString(TEXT("UI"));
		FString ClassNameStr = HelperName;
		HelperName.Split(TEXT("$"),&PackageStr,&ClassNameStr);
		HelperName = PackageStr + FString(TEXT(".")) + ClassNameStr;

		UClass* Class = Cast<UClass>(UObject::StaticLoadObject( UClass::StaticClass(), NULL, *HelperName, NULL, LOAD_NoWarn, NULL ));
		if(!Class) appErrorf(TEXT("Class not found: %s"), *HelperName );

		UUIHelper* Helper = CastChecked<UUIHelper>(UObject::StaticConstructObject(Class,GetGConsole()));
		if(!Helper) appErrorf(TEXT("Unable to create component: %s"), Class->GetFullName() );
		Helper->Constructed();

		NOTE(debugf(TEXT("Created helper: %s"),Helper->GetFullName()));

		GetGConsole()->Helpers.AddItem(Helper);
		
		// Check for object registers.
		PARSE_LIST_BEGIN(RegisterObj)

			GetObjMap().Set( NEXT_TOKEN, Helper );
			NOTE(GetObjMap().Dump(*GLog));

		PARSE_LIST_END

		// Create a dummy object so whatever has imported us doesn't have to deal with a NULL pointer.
		C=CreateComponent<URealComponent>(TEXT("Helper"));	// Fix ARL: Maybe use a UIHelperComponent class as a wrapper (so it can delete the UIHelper and maybe provide additional functionality).
		check(C!=NULL);
	}
	else
	{
		FString RealName = Name;

		BEGIN_PARSE

			PARSE(Name)	RealName = NEXT_TOKEN;

		END_PARSE

		C=StringCreateComponent(*ClassName,*RealName);
	}

	if(!C)
	{
		// if we created a component on the fly because we couldn't load it, then don't try to auto-load it, (causes infinite recursion)
		if( !OrigSection )
			appErrorf(TEXT("UI Class not found: %s"), *ClassName );

		// if there's no class by that name, it must be an override of an existing component type
		// Create a new overrides string that contains a tab before every element
		// return the value of ImportComponent()
		FString NewOverrides;
		for( TMapBase<FString,FString>::TIterator VarIter( *Section ); VarIter; )
		{
			if ( VarIter.Key() == TEXT("Class") )
			{
				++VarIter;
				continue;
			}
			NewOverrides += FString(TEXT("\t")) + VarIter.Key() + TEXT("=") + VarIter.Value();
			// increment the loop
			++VarIter;
			// if we have another element coming up, add a newline
			if( VarIter )
				NewOverrides += TEXT("\n");
		}
		C = ImportComponent( OrigClassName, Outer, &NewOverrides, bReload );
		if( !C ) C = ImportComponent( ClassName, Outer, &NewOverrides, bReload );
		if( !C ) appErrorf(TEXT("UI Class not found: %s"), *ClassName );
	}
	else
	{
#endif
	NOTE(debugf(TEXT("Created %s (%s)"), *Name, *C->GetFullName() ));

	UComponent* OldGComponent=GetGComponent();
	GetGComponent()=C;

	// Localization.
	UBOOL OldGLocalize=GetGLocalize();
	BEGIN_PARSE

		PARSE(Localize)		GetGLocalize()=NEXT_TOKEN_BOOL;
		PARSE(Localized)	GetGLocalize()=NEXT_TOKEN_BOOL;

	END_PARSE

	// Check for event scope registers.
	PARSE_LIST_BEGIN(Register)

		GetEventScopeTable().Set( NEXT_TOKEN, C );
		NOTE(GetEventScopeTable().Dump(*GLog));

	PARSE_LIST_END

	C->Import( Section );
	check(GetGComponent()==C);	//Fix ARL: removeme

	// Send any initial events for this component after importing.
	PARSE_LIST_BEGIN(InitialEvent)

		DECLARE_NEXT_TOKEN_ACTIONEVENT(None,Event);
		if(Event) Event->OnEvent();

	PARSE_LIST_END

	GetGLocalize()=OldGLocalize;
	GetGComponent()=OldGComponent;
#if 1 //NEW MJL: nested ini files
	}
	delete Section;
#endif
	// Clean up macro var list.
	if( bUseOldList )
		GetGVarList() = OldList;
	if( bUseOldMap )
		GetGVarMap() = OldMap;
	ImportIndentLevel--;
	return C;

	unguardf((TEXT("(%s)"),*Name));
}

//------------------------------------------------------------------------------
void UComponent::Import( SectionData Section )
{
	guard(UComponent::Import);

	PARSE_LIST_BEGIN(Exec)

		GetGConsole()->ConsoleCommand( *NEXT_TOKEN );

	PARSE_LIST_END

	BEGIN_PARSE

		PARSE(Location)		{ DECLARE_NEXT_TOKEN_FLOAT(X); DECLARE_NEXT_TOKEN_FLOAT(Y);								SetLocationC			(X,Y);					}
		PARSE(Alignment)	{ DECLARE_NEXT_TOKEN_ENUM(EAlignment,XAxis); DECLARE_NEXT_TOKEN_ENUM(EAlignment,YAxis);	SetAlignmentC			(XAxis,YAxis);			}
		PARSE(Align)		{ DECLARE_NEXT_TOKEN_ENUM(EAlignment,XAxis); DECLARE_NEXT_TOKEN_ENUM(EAlignment,YAxis);	SetAlignmentC			(XAxis,YAxis);			}
		PARSE(Size)			{ DECLARE_NEXT_TOKEN_FLOAT(Width); DECLARE_NEXT_TOKEN_FLOAT(Height);					SetSizeC				(Width,Height);			}
		PARSE(Style)		{ DECLARE_NEXT_TOKEN_ENUM(ERenderStyle,Style);											SetStyle				(Style);				}
		PARSE(Enabled)		{ DECLARE_NEXT_TOKEN_BOOL(bEnabled);													SetEnabled				(bEnabled);				}
		PARSE(DrawOrder)	DrawOrder = NEXT_TOKEN_FLOAT;
		PARSE(Color)		SetColor( NEXT_TOKEN_COLOR );
		PARSE(Alpha)		SetAlpha( NEXT_TOKEN_INT );
		PARSE(AlphaPct)		SetAlphaPct( NEXT_TOKEN_FLOAT );
		PARSE(X)			SetX( NEXT_TOKEN_FLOAT );
		PARSE(Y)			SetY( NEXT_TOKEN_FLOAT );
		PARSE(Width)		SetWidth( NEXT_TOKEN_FLOAT );
		PARSE(Height)		SetHeight( NEXT_TOKEN_FLOAT );

		// Accessors.
		PARSE(LocationAccess)	AddAccessor(NEXT_TOKEN_ACTIONEVENT(Location));
		PARSE(XAccess)			AddAccessor(NEXT_TOKEN_ACTIONEVENT(X));
		PARSE(YAccess)			AddAccessor(NEXT_TOKEN_ACTIONEVENT(Y));
		PARSE(SizeAccess)		AddAccessor(NEXT_TOKEN_ACTIONEVENT(Size));
		PARSE(WidthAccess)		AddAccessor(NEXT_TOKEN_ACTIONEVENT(Width));
		PARSE(HeightAccess)		AddAccessor(NEXT_TOKEN_ACTIONEVENT(Height));
		PARSE(ColorAccess)		AddAccessor(NEXT_TOKEN_ACTIONEVENT(Color));
		PARSE(AlphaAccess)		AddAccessor(NEXT_TOKEN_ACTIONEVENT(Alpha));
		PARSE(RAccess)			AddAccessor(NEXT_TOKEN_ACTIONEVENT(R));
		PARSE(GAccess)			AddAccessor(NEXT_TOKEN_ACTIONEVENT(G));
		PARSE(BAccess)			AddAccessor(NEXT_TOKEN_ACTIONEVENT(B));
		PARSE(EnabledAccess)	AddAccessor(NEXT_TOKEN_ACTIONEVENT(Enabled));

		//locks.
		PARSE(LockLocation)		if(NEXT_TOKEN_BOOL) Lock(LOCK_Location);
		PARSE(LockSize)			if(NEXT_TOKEN_BOOL) Lock(LOCK_Size);
		PARSE(LockStyle)		if(NEXT_TOKEN_BOOL) Lock(LOCK_Style);
		PARSE(LockColor)		if(NEXT_TOKEN_BOOL) Lock(LOCK_Color);
		PARSE(LockAlpha)		if(NEXT_TOKEN_BOOL) Lock(LOCK_Alpha);
		PARSE(LockAlphaPct)		if(NEXT_TOKEN_BOOL) Lock(LOCK_AlphaPct);
		PARSE(LockAlignment)	if(NEXT_TOKEN_BOOL) Lock(LOCK_Alignment);
		PARSE(LockSendEvent)	if(NEXT_TOKEN_BOOL) Lock(LOCK_SendEvent);
		//not implemented yet.
		PARSE(LockX)			if(NEXT_TOKEN_BOOL) Lock(LOCK_X);
		PARSE(LockY)			if(NEXT_TOKEN_BOOL) Lock(LOCK_Y);
		PARSE(LockWidth)		if(NEXT_TOKEN_BOOL) Lock(LOCK_Width);
		PARSE(LockHeight)		if(NEXT_TOKEN_BOOL) Lock(LOCK_Height);
		PARSE(LockR)			if(NEXT_TOKEN_BOOL) Lock(LOCK_R);
		PARSE(LockG)			if(NEXT_TOKEN_BOOL) Lock(LOCK_G);
		PARSE(LockB)			if(NEXT_TOKEN_BOOL) Lock(LOCK_B);
		PARSE(LockAlignmentX)	if(NEXT_TOKEN_BOOL) Lock(LOCK_AlignmentX);
		PARSE(LockAlignmentY)	if(NEXT_TOKEN_BOOL) Lock(LOCK_AlignmentY);
		//selector.
		PARSE(LockBorders)		if(NEXT_TOKEN_BOOL) Lock(LOCK_Borders);


	END_PARSE

	PARSE_LIST_BEGIN(OnEnabled)

		AddAccessor(NEXT_TOKEN_ACTIONEVENT(OnEnabled));

	PARSE_LIST_END

	// Mouse support.
	PARSE_LIST_BEGIN(MouseEnter)

		SetMouseEnterEvent( GetGMouse(), NEXT_TOKEN_ACTIONEVENT(MouseEnter) );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(MouseExit)

		SetMouseExitEvent( GetGMouse(), NEXT_TOKEN_ACTIONEVENT(MouseExit) );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(MousePress)

		SetMousePressEvent( GetGMouse(), NEXT_TOKEN_ACTIONEVENT(MousePress) );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(MouseRelease)

		SetMouseReleaseEvent( GetGMouse(), NEXT_TOKEN_ACTIONEVENT(MouseRelease) );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(MouseClick)

		SetMouseClickEvent( GetGMouse(), NEXT_TOKEN_ACTIONEVENT(MouseClick) );

	PARSE_LIST_END

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Import( SectionData Section )
{
	guard(UContainer::Import);

	PARSE_LIST_BEGIN(Component)

		AddComponent( ImportComponent( NEXT_TOKEN, PARENT, Overrides ) );

	PARSE_LIST_END

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UScaleContainer::Import( SectionData Section )
{
	guard(UScaleContainer::Import);

	BEGIN_PARSE

		PARSE(RelativeSize)	{ RelativeSize.Width=NEXT_TOKEN_FLOAT; RelativeSize.Height=NEXT_TOKEN_FLOAT; }

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UTileContainer::Import( SectionData Section )
{
	guard(UTileContainer::Import);

	BEGIN_PARSE

		PARSE(Horizontal)		bVertical = !(NEXT_TOKEN_BOOL);
		PARSE(Vertical)			bVertical = NEXT_TOKEN_BOOL;

		PARSE(ReverseJustify)	bReverseJustify = NEXT_TOKEN_BOOL;
		PARSE(RightJustify)		bReverseJustify = NEXT_TOKEN_BOOL;
		PARSE(BottomJustify)	bReverseJustify = NEXT_TOKEN_BOOL;
		PARSE(LeftJustify)		bReverseJustify = !(NEXT_TOKEN_BOOL);
		PARSE(TopJustify)		bReverseJustify = !(NEXT_TOKEN_BOOL);

		PARSE(ReverseOrder)		bReverseOrder = NEXT_TOKEN_BOOL;

		PARSE(ShortCircuit)		bShortCircuit = NEXT_TOKEN_BOOL;

		PARSE(FixedSpacing)		FixedSpacing = NEXT_TOKEN_FLOAT;

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UMultiStateComponent::Import( SectionData Section )
{
	guard(UMultiStateComponent::Import);

	PARSE_LIST_BEGIN(State)

		UComponent* State = ImportComponent( NEXT_TOKEN, PARENT, Overrides );

		// optionally override location.
		if( *ParseText!=TEXT("") )
		{
			DECLARE_NEXT_TOKEN_FLOAT(X);
			DECLARE_NEXT_TOKEN_FLOAT(Y);
			State->SetLocationC(X,Y);
		}

		AddState( State );

	PARSE_LIST_END

	BEGIN_PARSE

		PARSE(InitialState)		SetState(NEXT_TOKEN_INT);

	END_PARSE

	PARSE_LIST_BEGIN(Transition)

		DECLARE_NEXT_TOKEN_SCOPED_EVENT;

		if( Scope )
		{
			DECLARE_NEXT_TOKEN_INT(iStart);
			DECLARE_NEXT_TOKEN_INT(iEnd);
			DECLARE_NEXT_TOKEN_FLOAT(Time);

			UComponent* Template = ImportComponent( NEXT_TOKEN, PARENT, Overrides );

			DECLARE_NEXT_TOKEN_BOOL_OPTX(bNoOverride,false);

			uiEvent AdditionalEvent1=EVENT_None;
			{
				DECLARE_NEXT_TOKEN_SCOPED_EVENT;
				if( Scope ){ AdditionalEvent1=Event; Scope->RegisterEvent( Event, this ); }
			}

			uiEvent AdditionalEvent2=EVENT_None;
			{
				DECLARE_NEXT_TOKEN_SCOPED_EVENT;
				if( Scope ){ AdditionalEvent2=Event; Scope->RegisterEvent( Event, this ); }
			}

			uiEvent AdditionalEvent3=EVENT_None;
			{
				DECLARE_NEXT_TOKEN_SCOPED_EVENT;
				if( Scope ){ AdditionalEvent3=Event; Scope->RegisterEvent( Event, this ); }
			}

			AddTransition( Event, iStart, iEnd, Time, Template, bNoOverride, AdditionalEvent1, AdditionalEvent2, AdditionalEvent3 );
			Scope->RegisterEvent( Event, this );
			// Fix ARL: Register AdditionalEvents as well?
		}
		else debugf( NAME_Warning, TEXT("(%s)UMultiStateComponent::Import Transition[%d] - Scope not found: %s"), *GetFullName(), Transitions.Num(), *ScopeStr );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(TriggerEvent)

		DECLARE_NEXT_TOKEN_INT(iState);
		DECLARE_NEXT_TOKEN_FLOAT(Pct);
		DECLARE_NEXT_TOKEN_ACTIONEVENT(None,Event);

		if(Event) AddTriggerEvent(iState,Event,Pct);

	PARSE_LIST_END

	BEGIN_PARSE

		PARSE(NextTransitionEvent)
		{
			DECLARE_NEXT_TOKEN_SCOPED_EVENT;

			if( Scope )
			{
				NextTransitionEvent = Event;
				Scope->RegisterEvent( Event, this );
			}
			else debugf( NAME_Warning, TEXT("(%s)UMultiStateComponent::Import NextTransitionEvent - Scope not found: %s"), *GetFullName(), *ScopeStr );
		}

		PARSE(PrevTransitionEvent)
		{
			DECLARE_NEXT_TOKEN_SCOPED_EVENT;

			if( Scope )
			{
				PrevTransitionEvent = Event;
				Scope->RegisterEvent( Event, this );
			}
			else debugf( NAME_Warning, TEXT("(%s)UMultiStateComponent::Import PrevTransitionEvent - Scope not found: %s"), *GetFullName(), *ScopeStr );
		}

		PARSE(NextStateEvent)
		{
			DECLARE_NEXT_TOKEN_SCOPED_EVENT;

			if( Scope )
			{
				NextStateEvent = Event;
				Scope->RegisterEvent( Event, this );
			}
			else debugf( NAME_Warning, TEXT("(%s)UMultiStateComponent::Import NextStateEvent - Scope not found: %s"), *GetFullName(), *ScopeStr );
		}

		PARSE(PrevStateEvent)
		{
			DECLARE_NEXT_TOKEN_SCOPED_EVENT;

			if( Scope )
			{
				PrevStateEvent = Event;
				Scope->RegisterEvent( Event, this );
			}
			else debugf( NAME_Warning, TEXT("(%s)UMultiStateComponent::Import PrevStateEvent - Scope not found: %s"), *GetFullName(), *ScopeStr );
		}

	END_PARSE

	BEGIN_PARSE

		PARSE(TweenLocation)	bTweenLocation		= NEXT_TOKEN_BOOL;
		PARSE(TweenSize)		bTweenSize			= NEXT_TOKEN_BOOL;
		PARSE(TweenColor)		bTweenColor			= NEXT_TOKEN_BOOL;
		PARSE(TweenAlpha)		bTweenAlpha			= NEXT_TOKEN_BOOL;
		PARSE(TweenLinear)		bTweenLinear		= NEXT_TOKEN_BOOL;
		PARSE(TweenReverseEase)	bTweenReverseEase	= NEXT_TOKEN_BOOL;

		PARSE(StateAccess)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(State) );

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UImage::Import( SectionData Section )
{
	guard(UImage::Import);
	
	BEGIN_PARSE
	
		PARSE(Texture)		Material = NEXT_TOKEN_MATERIAL;
		PARSE(Material)		Material = NEXT_TOKEN_MATERIAL;
	
	END_PARSE
	
	PARSE_LIST_BEGIN(Image)

		DECLARE_NEXT_TOKEN(Name);
		DECLARE_NEXT_TOKEN_FLOAT(X);
		DECLARE_NEXT_TOKEN_FLOAT(Y);
		DECLARE_NEXT_TOKEN_FLOAT(W);
		DECLARE_NEXT_TOKEN_FLOAT(H);
		DECLARE_NEXT_TOKEN_BOOL_OPTX(bTile,0);
		AddImage(Name,X,Y,W,H,bTile);

	PARSE_LIST_END

	PARSE_LIST_BEGIN(Image2)

		DECLARE_NEXT_TOKEN(Name);
		DECLARE_NEXT_TOKEN_FLOAT(X);
		DECLARE_NEXT_TOKEN_FLOAT(Y);
		DECLARE_NEXT_TOKEN_FLOAT(X2);
		DECLARE_NEXT_TOKEN_FLOAT(Y2);
		DECLARE_NEXT_TOKEN_BOOL_OPTX(bTile,0);
		AddImage(Name,X,Y,X2-X,Y2-Y,bTile);

	PARSE_LIST_END

	PARSE_LIST_BEGIN(FullImage)

		DECLARE_NEXT_TOKEN(Name);
		DECLARE_NEXT_TOKEN_BOOL_OPTX(bTile,0);
		AddFullImage(Name,bTile);

	PARSE_LIST_END

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UImageComponentBase::Import( SectionData Section )
{
	guard(UImageComponentBase::Import);

	BEGIN_PARSE

		PARSE(Tile)		bTile = NEXT_TOKEN_BOOL;

	END_PARSE

	Init();

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UImageComponent::Import( SectionData Section )
{
	guard(UImageComponent::Import);

	BEGIN_PARSE

		PARSE(Image)		Image = NEXT_TOKEN_IMAGE;
		PARSE(Access)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageComponent) );
		PARSE(Accessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageComponent) );

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UTimedImage::Import( SectionData Section )
{
	guard(UTimedImage::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(Lifespan)		SetLifespan( NEXT_TOKEN_FLOAT );
		PARSE(FadeinPct)	FadeinPct = NEXT_TOKEN_FLOAT;
		PARSE(FadeoutPct)	FadeoutPct = NEXT_TOKEN_FLOAT;
		PARSE(FadePct)		{ FadeinPct = NEXT_TOKEN_FLOAT; FadeoutPct = NEXT_TOKEN_FLOAT; }

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void UAnimatedImageComponent::Import( SectionData Section )
{
	guard(UAnimatedImageComponent::Import);

	PARSE_LIST_BEGIN(Image)

		Images.AddItem( NEXT_TOKEN_IMAGE );

	PARSE_LIST_END

	BEGIN_PARSE

		PARSE(Tween)		SetTween( NEXT_TOKEN_FLOAT );
		PARSE(TweenAccess)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(AnimatedImageTween) );

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UTextComponentBase::Import( SectionData Section )
{
	guard(UTextComponentBase::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(Font)			SetFont( NEXT_TOKEN_OBJ(UFont) );
		PARSE(LocalizedFont)SetFont( NEXT_TOKEN_OBJ_LOCALIZED(UFont) );

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void ULabel::Import( SectionData Section )
{
	guard(ULabel::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(Wrap)			bWrap = NEXT_TOKEN_BOOL;
		PARSE(Text)			SetText( NEXT_TOKEN_LOCALIZED, GetSize()!=FDimension(0,0) );
		PARSE(Format)		Format = NEXT_TOKEN_LOCALIZED;
		PARSE(Access)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(Label) );
		PARSE(Accessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(Label) );
		PARSE(FloatAccess)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(LabelFloatAccess) );

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void ULabelShadow::Import( SectionData Section )
{
	guard(ULabelShadow::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(ShadowColor)	ShadowColor = NEXT_TOKEN_COLOR;
		PARSE(ShadowOffset)	{ DECLARE_NEXT_TOKEN_FLOAT(X); DECLARE_NEXT_TOKEN_FLOAT(Y); ShadowOffset = FDimension(X,Y); }

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void UTimedLabel::Import( SectionData Section )
{
	guard(UTimedLabel::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(Lifespan)		SetLifespan( NEXT_TOKEN_FLOAT );
		PARSE(FadeinPct)	FadeinPct = NEXT_TOKEN_FLOAT;
		PARSE(FadeoutPct)	FadeoutPct = NEXT_TOKEN_FLOAT;
		PARSE(FadePct)		{ FadeinPct = NEXT_TOKEN_FLOAT; FadeoutPct = NEXT_TOKEN_FLOAT; }

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void UMultiImageComponent::Import( SectionData Section )
{
	guard(UMultiImageComponent::Import);

	PARSE_LIST_BEGIN(Images)

		Images.AddItem( NEXT_TOKEN_IMAGE );

	PARSE_LIST_END

	BEGIN_PARSE

		PARSE(Index)		Index = NEXT_TOKEN_INT;
		PARSE(Access)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(MultiImageComponent) );
		PARSE(Accessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(MultiImageComponent) );
		PARSE(AutoSize)		bAutoResize = NEXT_TOKEN_BOOL;
		PARSE(AutoResize)	bAutoResize = NEXT_TOKEN_BOOL;

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::Import( SectionData Section )
{
	guard(UMouseCursor::Import);

	BEGIN_PARSE

		// Legacy support for when MouseCursors used to be subclasses of ImageComponent.

		UImageComponent* Image=NULL;

		PARSE(Image)		{ Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=NEXT_TOKEN_IMAGE;									Image->Init(); }
		PARSE(Access)		{ Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageComponent) );	Image->Init(); }
		PARSE(Accessor)		{ Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageComponent) );	Image->Init(); }
		PARSE(Tile)			{ if(Image) Image->bTile = NEXT_TOKEN_BOOL; }

	END_PARSE

	BEGIN_PARSE

		PARSE(MoveTime)		MoveTime = NEXT_TOKEN_FLOAT;
		PARSE(MoveTimer)	MoveTime = NEXT_TOKEN_FLOAT;

	END_PARSE

	Super::Import( Section );
	SetConsole( CastChecked<UUIConsole>(GetConsole()) );		// Fix ARL: Use GConsole? (doesn't seem to stay set -- keeps getting reset to NULL)

#if MOUSECURSOR_PARTICLESYSTEMS

	BEGIN_PARSE

		PARSE(Particles)	MouseTrailName = *ParseText;

	END_PARSE

#endif

	GetGMouse() = this;	// Fix ARL: This should be pushed onto a stack and popped off when the parent is finished importing.  That way containers can have different mouse cursors for their component.

	unguard;
}

//------------------------------------------------------------------------------
void UButton::Import( SectionData Section )
{
	guard(UButton::Import);

	BEGIN_PARSE

		FImage* UpImg=Image->Image;			PARSE(UpImage)		UpImg = NEXT_TOKEN_IMAGE;
		FImage* DownImg=Image->DownImage;	PARSE(DownImage)	DownImg = NEXT_TOKEN_IMAGE;
		SetImages( UpImg, DownImg );
		Init();

		PARSE(Border)		Border = NEXT_TOKEN_FLOAT;
		PARSE(Font)			SetFont( NEXT_TOKEN_OBJ(UFont) );
		PARSE(LocalizedFont)SetFont( NEXT_TOKEN_OBJ_LOCALIZED(UFont) );
		PARSE(Text)			SetText( NEXT_TOKEN_LOCALIZED );
		PARSE(Access)		Text->AddAccessor( NEXT_TOKEN_ACTIONEVENT(Label) );

	END_PARSE

	PARSE_LIST_BEGIN(Action)

		AddAccessor( NEXT_TOKEN_ACTIONEVENT(ButtonAction) );

	PARSE_LIST_END

	SetMouse( GetGMouse() );
	UComponent::Import( Section );	// NOTE[aleiby]: Not Super because we don't want people adding additional components to this even though it is indeed a container.

	unguard;
}

//------------------------------------------------------------------------------
void UHoldButton::Import( SectionData Section )
{
	guard(UHoldButton::Import);

	BEGIN_PARSE

		PARSE(RepeatDelay)	RepeatDelay	= NEXT_TOKEN_FLOAT;
		PARSE(RepeatRate)	RepeatRate	= NEXT_TOKEN_FLOAT;

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UCheckBox::Import( SectionData Section )
{
	guard(UCheckBox::Import);

	BEGIN_PARSE

		PARSE(SetTarget)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(CheckboxModify) );
		PARSE(GetTarget)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(CheckboxAccess) );
		PARSE(Checked)		SetChecked( NEXT_TOKEN_BOOL );
		PARSE(Check)		Check = NEXT_TOKEN_IMAGE;

	END_PARSE

	SetMouse( GetGMouse() );
	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UInputSelector::Import( SectionData Section )
{
	guard(UInputSelector::Import);

	BEGIN_PARSE

		PARSE(SelectedColor)	SelectedColor = NEXT_TOKEN_COLOR;
		PARSE(InputAccess)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(InputSelector) );
		PARSE(InputAccessor)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(InputSelector) );
		PARSE(InputReset)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(InputSelectorReset) );
		PARSE(DisplayAccess)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(InputSelectorDisplay) );
		PARSE(StartEvent)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(StartListening) );
		PARSE(StopEvent)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(StopListening) );

	END_PARSE

	SetMouse( GetGMouse() );
	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UNumWatcher::Import( SectionData Section )
{
	guard(UNumWatcher::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(Float)	bFloat = NEXT_TOKEN_BOOL;
		PARSE(Watch)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(NumWatcherWatch) );

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void UFrame::Import( SectionData Section )
{
	guard(UFrame::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(FrameLocation)			{ DECLARE_NEXT_TOKEN_FLOAT(X);		DECLARE_NEXT_TOKEN_FLOAT(Y);		SetFrameLocation(FPoint(X,Y));			}
		PARSE(FrameSize)				{ DECLARE_NEXT_TOKEN_FLOAT(Width);	DECLARE_NEXT_TOKEN_FLOAT(Height);	SetFrameSize(FDimension(Width,Height));	}

		PARSE(FrameXPct)				SetFrameXPct( NEXT_TOKEN_FLOAT );
		PARSE(FrameYPct)				SetFrameYPct( NEXT_TOKEN_FLOAT );

		PARSE(FrameWidthPct)			SetFrameWidthPct( NEXT_TOKEN_FLOAT );
		PARSE(FrameHeightPct)			SetFrameHeightPct( NEXT_TOKEN_FLOAT );

		PARSE(FrameLocationAccessor)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameLocationAccess) );
		PARSE(FrameLocationModifier)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameLocationModify) );

		PARSE(FrameSizeAccessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameSizeAccess) );
		PARSE(FrameSizeModifier)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameSizeModify) );

		PARSE(FrameXPctAccessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameXPctAccess) );
		PARSE(FrameXPctModifier)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameXPctModify) );

		PARSE(FrameYPctAccessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameYPctAccess) );
		PARSE(FrameYPctModifier)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameYPctModify) );

		PARSE(FrameWidthPctAccessor)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameWidthPctAccess) );
		PARSE(FrameWidthPctModifier)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameWidthPctModify) );

		PARSE(FrameHeightPctAccessor)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameHeightPctAccess) );
		PARSE(FrameHeightPctModifier)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(FrameHeightPctModify) );

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void UScrollFrame::Import( SectionData Section )
{
	guard(UScrollFrame::Import);

	Init();
	SetMouse( GetGMouse() );

	PARSE_LIST_BEGIN(Item)

		AddItem( ImportComponent( NEXT_TOKEN, PARENT, Overrides ) );

	PARSE_LIST_END

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(FrameLocation)	{ DECLARE_NEXT_TOKEN_FLOAT(X);		DECLARE_NEXT_TOKEN_FLOAT(Y);		SetFrameLocation(FPoint(X,Y));			}
		PARSE(FrameSize)		{ DECLARE_NEXT_TOKEN_FLOAT(Width);	DECLARE_NEXT_TOKEN_FLOAT(Height);	SetFrameSize(FDimension(Width,Height));	}

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void USelector::Import( SectionData Section )
{
	guard(USelector::Import);

	SetMouse( GetGMouse() );

	BEGIN_PARSE

		PARSE(HideBorders)	bHideBorders = NEXT_TOKEN_BOOL;
		PARSE(Highlight)	SetHighlight( NEXT_TOKEN_COLOR );
		PARSE(UnHighlight)	SetUnHighlight( NEXT_TOKEN_COLOR );
		PARSE(Font)			SetFont( NEXT_TOKEN_OBJ(UFont) );
		PARSE(LocalizedFont)SetFont( NEXT_TOKEN_OBJ_LOCALIZED(UFont) );
		PARSE(Background)	SetBackground( NEXT_TOKEN_IMAGE );
		PARSE(TopFrame)		TopFrame = NEXT_TOKEN_IMAGE;
		PARSE(BottomFrame)	BottomFrame = NEXT_TOKEN_IMAGE;
		Init();

		PARSE(Group)		SetGroup( DynamicCast<UContainer>(GetEventScopeTable().FindRef(NEXT_TOKEN)) );

		PARSE(Accessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorAccess) );
		PARSE(Modifier)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorModify) );
		PARSE(CurrentText)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorText) );
		PARSE(Locked)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorLocked) );
		PARSE(CurrentAccess)AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorCurrent) );
		PARSE(CurrentModify)AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorModifyIndex) );

	END_PARSE

	PARSE_LIST_BEGIN(PreSelect)

		AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorPreSelect) );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(PostSelect)

		AddAccessor( NEXT_TOKEN_ACTIONEVENT(SelectorPostSelect) );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(Item)

		DECLARE_NEXT_TOKEN_LOCALIZED(Text);
		DECLARE_NEXT_TOKEN_ACTIONEVENT_OPTX(SelectorModify,Action,NULL);

		AddItem( Text, Action );

	PARSE_LIST_END

	BEGIN_PARSE

		PARSE(Current)		SelectItem( NEXT_TOKEN_LOCALIZED );

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void USlider::Import( SectionData Section )
{
	guard(USlider::Import);

	BEGIN_PARSE

		PARSE(Handle)		Handle = NEXT_TOKEN_IMAGE;
		PARSE(Format)		Format = NEXT_TOKEN_LOCALIZED;
		PARSE(SideBuffer)	SideBuffer = NEXT_TOKEN_FLOAT;
		PARSE(Step)			Step = NEXT_TOKEN_FLOAT;
		PARSE(Value)		SetValue( NEXT_TOKEN_FLOAT );
		PARSE(Range)		{ DECLARE_NEXT_TOKEN_FLOAT(Min); DECLARE_NEXT_TOKEN_FLOAT(Max); SetRange(Min,Max); }
		PARSE(SetTarget)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(SliderModify) );
		PARSE(GetTarget)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(SliderAccess) );

	END_PARSE

	SetMouse( GetGMouse() );
	Super::Import( Section );

	unguard;
}

// Fix ARL: With composited components, rather than copying all the import code from the subcomponents,
// it might make more sense to just construct the subcomponents (Init) and then pass the SectionData
// through their Import function.

//------------------------------------------------------------------------------
void UScrollBar::Import( SectionData Section )
{
	guard(UScrollBar::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(Horizontal)		bVertical = !(NEXT_TOKEN_BOOL);
		PARSE(Vertical)			bVertical = NEXT_TOKEN_BOOL;

		PARSE(Range)			{ DECLARE_NEXT_TOKEN_FLOAT(Min); DECLARE_NEXT_TOKEN_FLOAT(Max); SetRange(Min,Max); }
		PARSE(VisibleRange)		SetVisibleRange( NEXT_TOKEN_FLOAT );
		PARSE(Visible)			SetVisibleRange( NEXT_TOKEN_FLOAT );
		PARSE(InitialValue)		SetValue( NEXT_TOKEN_FLOAT );
		PARSE(Value)			SetValue( NEXT_TOKEN_FLOAT );

		PARSE(ScrollAmt)		ScrollAmt	= NEXT_TOKEN_FLOAT;
		PARSE(PageAmt)			PageAmt		= NEXT_TOKEN_FLOAT;

		PARSE(RepeatDelay)		RepeatDelay	= NEXT_TOKEN_FLOAT;
		PARSE(RepeatRate)		RepeatRate	= NEXT_TOKEN_FLOAT;

		PARSE(UpButtonUp)		UpButtonUp		= NEXT_TOKEN_IMAGE;
		PARSE(UpButtonDown)		UpButtonDown	= NEXT_TOKEN_IMAGE;
		PARSE(DownButtonUp)		DownButtonUp	= NEXT_TOKEN_IMAGE;
		PARSE(DownButtonDown)	DownButtonDown	= NEXT_TOKEN_IMAGE;
		PARSE(BackgroundImage)	BackgroundImage	= NEXT_TOKEN_IMAGE;
		PARSE(SliderImage)		SliderImage		= NEXT_TOKEN_IMAGE;

	END_PARSE

	Init();
	SetMouse( GetGMouse() );

	unguard;
}

//------------------------------------------------------------------------------
void USoundComponent::Import( SectionData Section )
{
	guard(USoundComponent::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(Sound)		Sound = *ParseText;
		PARSE(Volume)		Volume = NEXT_TOKEN_FLOAT;
		PARSE(Interrupt)	bInterrupt = NEXT_TOKEN_BOOL;

	END_PARSE

	PARSE_LIST_BEGIN(PlayEvent)

		DECLARE_NEXT_TOKEN_SCOPED_EVENT;
		if( Scope )
			Scope->RegisterEvent( Event, this );

	PARSE_LIST_END

	Init();
	Super::Import( Section );	// Fix ARL: Necessary?

	unguard;
}

//------------------------------------------------------------------------------
void UTextArea::Import( SectionData Section )
{
	guard(UTextArea::Import);

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(ForceWrap)	bForceWrap = NEXT_TOKEN_BOOL;
		PARSE(ForceUnWrap)	bForceUnWrap = NEXT_TOKEN_BOOL;
		PARSE(TopDown)		bTopDown = NEXT_TOKEN_BOOL;
		PARSE(FIFO)			bFIFO = NEXT_TOKEN_BOOL;
		PARSE(MaxLines)		MaxLines = NEXT_TOKEN_INT;
		PARSE(Font)			SetFont( NEXT_TOKEN_OBJ(UFont) );
		PARSE(LocalizedFont)SetFont( NEXT_TOKEN_OBJ_LOCALIZED(UFont) );
		PARSE(Access)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(TextAreaAccess) );
		PARSE(Accessor)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(TextAreaAccess) );
		PARSE(Console)
		{
			if( NEXT_TOKEN_BOOL )
				CastChecked<UUIConsole>(GetConsole())->MessageAreas.AddUniqueItem(this);
		}

	END_PARSE

	PARSE_LIST_BEGIN(Text)

		AddText( NEXT_TOKEN_LOCALIZED );

	PARSE_LIST_END

	unguard;
}

//------------------------------------------------------------------------------
void UScrollTextArea::Import( SectionData Section )
{
	guard(UScrollTextArea::Import);

	Init();
	SetMouse( GetGMouse() );

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UTimedTextArea::Import( SectionData Section )
{
	guard(UTimedTextArea::Import);

	BEGIN_PARSE

		PARSE(TextLifespan)	TextLifespan = NEXT_TOKEN_FLOAT;
		PARSE(FadeinPct)	FadeinPct = NEXT_TOKEN_FLOAT;
		PARSE(FadeoutPct)	FadeoutPct = NEXT_TOKEN_FLOAT;
		PARSE(FadePct)		{ FadeinPct = NEXT_TOKEN_FLOAT; FadeoutPct = NEXT_TOKEN_FLOAT; }
		PARSE(MinInterval)	MinInterval = NEXT_TOKEN_FLOAT;

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UTextField::Import( SectionData Section )
{
	guard(UTextField::Import);

	Super::Import( Section );
	SetConsole( GetGConsole() );
	SetMouse( GetGMouse() );

	BEGIN_PARSE

		PARSE(AutoSize)			bAutoSize = NEXT_TOKEN_BOOL;
		PARSE(InputRange)		{ DECLARE_NEXT_TOKEN_INT(Min); DECLARE_NEXT_TOKEN_INT(Max); SetInputRange(Min,Max); }
		PARSE(Command)			AddAccessor( NEXT_TOKEN_ACTIONEVENT(TextFieldCommand) );
		PARSE(Access)			AddAccessor( NEXT_TOKEN_ACTIONEVENT(TextFieldAccess) );
		PARSE(Prefix)			Prefix = NEXT_TOKEN_LOCALIZED;
		PARSE(Postfix)			Postfix = NEXT_TOKEN_LOCALIZED;
		PARSE(Persistent)		bPersistent = NEXT_TOKEN_BOOL;
		PARSE(AutoSelect)		bAutoSelect = NEXT_TOKEN_BOOL;
		PARSE(DeselectOnEnter)	bDeselectOnEnter = NEXT_TOKEN_BOOL;
		PARSE(GreedyFocus)		bGreedyFocus = NEXT_TOKEN_BOOL;
		PARSE(TextColor)		TextColor = NEXT_TOKEN_COLOR;
		PARSE(InitialText)		LastStr = NEXT_TOKEN_LOCALIZED;
		PARSE(Cursor)			SetCursor( ImportComponent( NEXT_TOKEN, PARENT, Overrides ) );
		PARSE(EnterEvent)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(TextFieldEnterEvent) );
		PARSE(EscapeEvent)		AddAccessor( NEXT_TOKEN_ACTIONEVENT(TextFieldEscapeEvent) );

	END_PARSE

	PARSE_LIST_BEGIN(Ignore)

		AddIgnoredKey( ***ParseText );

	PARSE_LIST_END

	unguard;
}

//------------------------------------------------------------------------------
void UImageWipe::Import( SectionData Section )
{
	guard(UImageWipe::Import);

	BEGIN_PARSE

		PARSE(TopImage)			SetTopImage( NEXT_TOKEN_IMAGE );
		PARSE(BottomImage)		SetBottomImage( NEXT_TOKEN_IMAGE );

		PARSE(Access)			AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageWipe) );
		PARSE(Accessor)			AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageWipe) );

		PARSE(InitialPct)		Pct = NEXT_TOKEN_FLOAT;
		PARSE(Pct)				Pct = NEXT_TOKEN_FLOAT;

		PARSE(Vertical)			bVertical = NEXT_TOKEN_BOOL;
		PARSE(Vert)				bVertical = NEXT_TOKEN_BOOL;

		PARSE(Invert)			bInvert = NEXT_TOKEN_BOOL;
		PARSE(Reverse)			bInvert = NEXT_TOKEN_BOOL;

		PARSE(AutoSize)			if(NEXT_TOKEN_BOOL && TopImage) SetSize(TopImage->GetSize());

	END_PARSE

	Super::Import( Section );

	BEGIN_PARSE

		PARSE(TopComponent)		SetTopImageComponent( DynamicCastChecked<UImageComponent>(ImportComponent( NEXT_TOKEN, PARENT, Overrides )) );
		PARSE(BottomComponent)	SetBottomImageComponent( DynamicCastChecked<UImageComponent>(ImportComponent( NEXT_TOKEN, PARENT, Overrides )) );

	END_PARSE

	unguard;
}

//------------------------------------------------------------------------------
void UImageWedge::Import( SectionData Section )
{
	guard(UImageWedge::Import);

	BEGIN_PARSE

		PARSE(Radius)		Radius = NEXT_TOKEN_FLOAT;
		PARSE(Angle)		Angle = NEXT_TOKEN_FLOAT;

		PARSE(RadiusAccess)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageWedgeRadius) );
		PARSE(AngleAccess)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(ImageWedgeAngle) );

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UScriptComponent::Import( SectionData Section )
{
	guard(UScriptComponent::Import);

	BEGIN_PARSE

		PARSE(Target)	AddAccessor( NEXT_TOKEN_ACTIONEVENT(ScriptComponentTarget) );

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UVarWatcher::Import( SectionData Section )
{
	guard(UVarWatcher::Import);

	BEGIN_PARSE

		PARSE(Watch)
		{
			UObject* WatchObj = GetObjMap().FindRef( NEXT_TOKEN );
			FName PropName = NEXT_TOKEN_NAME;
			DECLARE_NEXT_TOKEN_INT_OPTX(ArrayIndex,0);
			SetWatch(WatchObj,PropName,ArrayIndex);
		}

		PARSE(FullDesc)			bFullDesc = NEXT_TOKEN_BOOL;
		PARSE(FullDescription)	bFullDesc = NEXT_TOKEN_BOOL;

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UWindow::Import( SectionData Section )
{
	guard(UWindow::Import);

	BEGIN_PARSE

#define UWINDOW_PARSE_IMAGE(img) \
		FImage* img=NULL; PARSE(img) img = NEXT_TOKEN_IMAGE;

		UWINDOW_PARSE_IMAGE(Top)
		UWINDOW_PARSE_IMAGE(Bottom)
		UWINDOW_PARSE_IMAGE(Left)
		UWINDOW_PARSE_IMAGE(Right)
		UWINDOW_PARSE_IMAGE(NE)
		UWINDOW_PARSE_IMAGE(NW)
		UWINDOW_PARSE_IMAGE(SE)
		UWINDOW_PARSE_IMAGE(SW)
		UWINDOW_PARSE_IMAGE(Background)

#undef UWINDOW_PARSE_IMAGE

		SetImages( Top, Bottom, Left, Right, NE, NW, SE, SW, Background );

	END_PARSE

	Super::Import( Section );

	unguard;
}

//------------------------------------------------------------------------------
void UKeyEvent::Import( SectionData Section )
{
	guard(UKeyEvent::Import);

	Super::Import( Section );

	PARSE_LIST_BEGIN(Key)

		DECLARE_NEXT_TOKEN_ENUM(EInputKey,Key);
		DECLARE_NEXT_TOKEN_ENUM(EInputAction,Action);
		DECLARE_NEXT_TOKEN_ACTIONEVENT(None,Event);

		AddEvent(Key,Action,Event);

	PARSE_LIST_END

	SetConsole( GetGConsole() );

	unguard;
}

//------------------------------------------------------------------------------
void UEditor::Import( SectionData Section )
{
	guard(UEditor::Import);

	Super::Import( Section );
	SetMouse( GetGMouse() );
	SetConsole( GetGConsole() );

	unguard;
}

//------------------------------------------------------------------------------
void UCredits::Import( SectionData Section )
{
	guard(UCredits::Import);

	BEGIN_PARSE

		PARSE(Rate)	Rate = NEXT_TOKEN_FLOAT;

		PARSE(StartEvent)
		{
			DECLARE_NEXT_TOKEN_SCOPED_EVENT;

			if( Scope )
			{
				StartEvent = Event;
				Scope->RegisterEvent( Event, this );
			}
			else debugf( NAME_Warning, TEXT("(%s)UCredits::Import StartEvent - Scope not found: %s"), *GetFullName(), *ScopeStr );
		}

		PARSE(StopEvent)
		{
			DECLARE_NEXT_TOKEN_SCOPED_EVENT;

			if( Scope )
			{
				StopEvent = Event;
				Scope->RegisterEvent( Event, this );
			}
			else debugf( NAME_Warning, TEXT("(%s)UCredits::Import StopEvent - Scope not found: %s"), *GetFullName(), *ScopeStr );
		}

	END_PARSE

	PARSE_LIST_BEGIN(ElementStyle)

		DECLARE_NEXT_TOKEN(Tag);
		DECLARE_NEXT_TOKEN(ComponentName);
		AddStyle( *Tag, *ComponentName );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(Element)

		DECLARE_NEXT_TOKEN(Tag);
		DECLARE_NEXT_TOKEN_LOCALIZED_OPTX(Text,TEXT(""));
		AddElement( *FString::Printf(TEXT("%s|%s"),*Tag,*Text) );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(LocalEvent)

		DECLARE_NEXT_TOKEN_FLOAT(Pct);
		DECLARE_NEXT_TOKEN_EVENT(Event);
		AddLocalEvent( Pct, Event );

	PARSE_LIST_END

	PARSE_LIST_BEGIN(GlobalEvent)

		DECLARE_NEXT_TOKEN_FLOAT(Pct);
		DECLARE_NEXT_TOKEN_ACTIONEVENT(None,Event);
		AddGlobalEvent( Pct, Event );

	PARSE_LIST_END

	Super::Import( Section );

	unguard;
}
