/*=============================================================================
	ImportUI.h: UI import from text file code.
	Copyright (C) 2000-2001 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron R Leiby
=============================================================================*/

THashTable<FString,UComponent*>	&GetEventScopeTable();
THashTable<FString,UObject*>	&GetObjMap();
THashTable<FString,FImage*>		&GetFImageMap();
UMouseCursor*					&GetGMouse();
TArray<UMouseCursor*>			&GetGMouseList();
UI_API UUIConsole*				&GetGConsole();
FString							&GetGFilename();
FString							&GetGUIPath();
UBOOL							&GetGLocalize();
TArray<FString>					&GetGVarList();
RealSectionData					&GetGVarMap();
UComponent*						&GetGComponent();
UBOOL							&GetGTransient();
UI_API FString					&GetGTokenSeparator();
UI_API const TCHAR*				GetIndentString();
void							DeVar( FString &S, SectionData Section );

//typedef TMultiMap<FString,FString>* SectionData;

#define UI_FILE_EXTENSION TEXT(".ui")

#undef PARENT	//temp!!
#define PARENT GetGConsole() /*Fix ARL: Use "this" or GetOuter() or GConsole?*/

#define BEGIN_PARSE { FString* ParseText; FString* Overrides = NULL; \
	NOTE(guard(Printing MultiMap); \
	for( TMultiMap<FString,FString>::TIterator It(*Section); It; ++It ) \
		debugf(TEXT("%sSection: %s::%s"), GetIndentString(), *It.Key(), *It.Value() ); \
	unguard;)

#define END_PARSE }

#define PARSE(name) \
	ParseText = Section->Find( TEXT(#name) ); \
	FString temp##name, Overrides##name; \
	if( ParseText ) \
	{ \
		NOTE(debugf(TEXT("%sParse(%s)"), GetIndentString(), TEXT(#name) )); \
		SplitNewline( *ParseText, temp##name, Overrides##name ); \
		NOTE(debugf(TEXT("%s[%s]"), GetIndentString(), *temp##name )); \
		ParseText = &temp##name; \
		Overrides = &Overrides##name; \
	}	/*NOTE[aleiby]: Copy parse text into a temp buffer so we don't screw up the original data.*/ \
	if( ParseText )

#define PARSE_LIST_BEGIN(name) \
	BEGIN_PARSE \
	TArray<FString> List; \
	Section->MultiFind( TEXT(#name), List ); \
	NOTE(if( List.Num() ) debugf(TEXT("%sParseList(%s)"), GetIndentString(), TEXT(#name) )); \
	for( INT i=List.Num()-1; i>=0; i-- ) \
	{ \
		ParseText = &List(i); \
		FString Temp, TempOverrides; \
		if( ParseText ) \
		{ \
			SplitNewline( *ParseText, Temp, TempOverrides ); \
			ParseText = &Temp; \
			Overrides = &TempOverrides; \
		}	/*NOTE[aleiby]: Copy parse text into a temp buffer so we don't screw up the original data.*/ \
		NOTE(debugf(TEXT("%sParseText(%d): %s"), GetIndentString(), List.Num()-1-i, ParseText ? **ParseText : TEXT("NULL") )); \
		check(ParseText);

#define PARSE_LIST_END } END_PARSE

#define NEXT_TOKEN						GetToken			(*ParseText)
#define NEXT_TOKEN_INT					appAtoi				(*NEXT_TOKEN)
#define NEXT_TOKEN_FLOAT				appAtof				(*NEXT_TOKEN)
#define NEXT_TOKEN_ENUM(e)				(e)StrToEnum		(FString(TEXT(#e)),NEXT_TOKEN)
#define NEXT_TOKEN_ENUM_EX(e,p)			(e)StrToEnum		(FString(TEXT(#e)),NEXT_TOKEN,FString(TEXT(#p)))
#define NEXT_TOKEN_NAME					FName				(*NEXT_TOKEN,FNAME_Intrinsic)
#define NEXT_TOKEN_EVENT				GetEvent			(NEXT_TOKEN)
#define NEXT_TOKEN_BOOL					NEXT_TOKEN.Caps()==TEXT("TRUE") ? true : false
#define NEXT_TOKEN_COLOR				ParseColor			(ParseText)
#define NEXT_TOKEN_OBJ(clas)			uiLoadObject<clas>	(NEXT_TOKEN)
#define NEXT_TOKEN_OBJ_LOCALIZED(clas)	uiLoadObject<clas>	(LocalizeGeneral(*NEXT_TOKEN,*GetGFilename()))
#define NEXT_TOKEN_MATERIAL				uiLoadMaterial		(NEXT_TOKEN)
#define NEXT_TOKEN_ACTIONEVENT(type)	GetActionEvent		(ACCESSOR_##type,ParseText)
#define NEXT_TOKEN_LOCALIZED			GetGLocalize() ? LocalizeGeneral(*NEXT_TOKEN,*GetGFilename()) : NEXT_TOKEN
#define NEXT_TOKEN_IMAGE				GetFImageMap().FindRef(NEXT_TOKEN)

#define DECLARE_NEXT_TOKEN(name)					FString      name = NEXT_TOKEN;
#define DECLARE_NEXT_TOKEN_INT(name)				INT          name = NEXT_TOKEN_INT;
#define DECLARE_NEXT_TOKEN_FLOAT(name)				FLOAT        name = NEXT_TOKEN_FLOAT;
#define DECLARE_NEXT_TOKEN_ENUM(e,name)				e            name = NEXT_TOKEN_ENUM(e);
#define DECLARE_NEXT_TOKEN_ENUM_EX(e,p,name)		e            name = NEXT_TOKEN_ENUM_EX(e,p);
#define DECLARE_NEXT_TOKEN_NAME(name)				FName        name = NEXT_TOKEN_NAME;
#define DECLARE_NEXT_TOKEN_EVENT(name)				uiEvent      name = NEXT_TOKEN_EVENT;
#define DECLARE_NEXT_TOKEN_BOOL(name)				UBOOL        name = NEXT_TOKEN_BOOL;
#define DECLARE_NEXT_TOKEN_COLOR(name)				FColor       name = NEXT_TOKEN_COLOR;
#define DECLARE_NEXT_TOKEN_OBJ(clas,name)			clas         name = NEXT_TOKEN_OBJ(clas);
#define DECLARE_NEXT_TOKEN_OBJ_LOCALIZED(clas,name)	clas         name = NEXT_TOKEN_OBJ_LOCALIZED(clas);
#define DECLARE_NEXT_TOKEN_MATERIAL(name)			UMaterial*   name = NEXT_TOKEN_MATERIAL;
#define DECLARE_NEXT_TOKEN_ACTIONEVENT(type,name)	ActionEvent* name = NEXT_TOKEN_ACTIONEVENT(type);
#define DECLARE_NEXT_TOKEN_LOCALIZED(name)			FString      name = NEXT_TOKEN_LOCALIZED;
#define DECLARE_NEXT_TOKEN_IMAGE(name)				FImage*      name = NEXT_TOKEN_IMAGE;

#define DECLARE_NEXT_TOKEN_OPTX(name,def)					FString      name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN;
#define DECLARE_NEXT_TOKEN_INT_OPTX(name,def)				INT          name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_INT;
#define DECLARE_NEXT_TOKEN_FLOAT_OPTX(name,def)				FLOAT        name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_FLOAT;
#define DECLARE_NEXT_TOKEN_ENUM_OPTX(e,name,def)			e            name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_ENUM(e);
#define DECLARE_NEXT_TOKEN_ENUM_EX_OPTX(e,p,name,def)		e            name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_ENUM_EX(e,p);
#define DECLARE_NEXT_TOKEN_NAME_OPTX(name,def)				FName        name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_NAME;
#define DECLARE_NEXT_TOKEN_EVENT_OPTX(name,def)				uiEvent      name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_EVENT;
#define DECLARE_NEXT_TOKEN_BOOL_OPTX(name,def)				UBOOL        name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_BOOL;
#define DECLARE_NEXT_TOKEN_COLOR_OPTX(name,def)				FColor       name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_COLOR;
#define DECLARE_NEXT_TOKEN_OBJ_OPTX(clas,name,def)			clas         name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_OBJ(clas);
#define DECLARE_NEXT_TOKEN_OBJ_LOCALIZED_OPTX(clas,name,def)clas         name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_OBJ_LOCALIZED(clas);
#define DECLARE_NEXT_TOKEN_MATERIAL_OPTX(name,def)			UMaterial*   name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_MATERIAL;
#define DECLARE_NEXT_TOKEN_ACTIONEVENT_OPTX(type,name,def)	ActionEvent* name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_ACTIONEVENT(type);
#define DECLARE_NEXT_TOKEN_LOCALIZED_OPTX(name,def)			FString      name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_LOCALIZED;
#define DECLARE_NEXT_TOKEN_IMAGE_OPTX(name,def)				FImage*      name = def; if( *ParseText!=TEXT("") ) name = NEXT_TOKEN_IMAGE;

#define DECLARE_NEXT_TOKEN_SCOPED_EVENT \
	FString ScopedEvent = NEXT_TOKEN; \
	FString ScopeStr = FString(TEXT("Root")); \
	FString EventStr = ScopedEvent; \
	ScopedEvent.Split(TEXT("."),&ScopeStr,&EventStr); \
	NOTE(GetEventScopeTable().Dump(*GLog)); \
	UComponent* Scope = GetEventScopeTable().FindRef(ScopeStr); \
	uiEvent Event = GetEvent(EventStr);

static inline void SplitNewline( FString &Text, FString &Main, FString &Overrides )
{
	guard(SplitNewLine);
	NOTE(debugf(TEXT("SplitNewLine: %s, %s, %s"), *Text, *Main, *Overrides ));
	FString DummyMain;
	int N = Text.InStr( TEXT("\n") );
	int R = Text.InStr( TEXT("\r") );
	if( N != -1 || R != -1 )
	{
		if( N != -1 && R != -1 )
			Text.Split(TEXT("\r\n"),&DummyMain,&Overrides);
		else if( R != -1 )
			Text.Split(TEXT("\r"),  &DummyMain,&Overrides);
		else //if(N != -1)
			Text.Split(TEXT("\n"),  &DummyMain,&Overrides);
	}
	else
	{
		DummyMain = Text;
		Overrides = TEXT("");
	}

	FString Tabs;
	Main = DummyMain;
	if( Main.Left(1) == TEXT("\t") )
		DummyMain.Split( TEXT("\t"), &Tabs, &Main, 1 );
	unguard;
}

static inline FString GetToken( FString &Text )
{
	NOTE(debugf(TEXT("GetToken: %s"), *Text ));

	FString Left = Text;
	FString Right = FString(TEXT(""));

/*	TArray<TCHAR> Chars = Text.GetCharArray();
	//if we have a string delimiter character
	if( Chars(0) == *TEXT("'") || Chars(0) == *TEXT("\"") )
	{
		//eliminate the first character, split on it (so we have the 'inside of string', and 'comma after quote and rest of string'
		Text.Mid(1).Split( &(Chars(0)), &Left, &Right );
		//cut off the comma we have on the right side of the quote
		Right = Right.Mid(1);
	}
	else
*/	{
		Text.Split( GetGTokenSeparator(), &Left, &Right );
	}
	Text = Right;

	// Get rid of C++ comments
	FString Useful,Junk;
	if( Left.Split( TEXT("//"), &Useful, &Junk) )
		Left = Useful;
	// Get rid of trailing spaces
	INT i=Left.Len() - 1;
	for(; i >= 0; i-- )
		if( (*Left)[i] != ' ' )
			break;
	if( Left.Len() - i -1 > 0 )
		Left = Left.LeftChop( Left.Len() - 1 - i );
	/// Get rid of leading spaces
	for( i=0; i < Left.Len(); i++ )
		if( (*Left)[i] != ' ' )
			break;
	if( i > 0 )
		Left = Left.Right( Left.Len() - i );
	// Parse next comma delimited string
	NOTE(debugf(TEXT("GotToken: %s"), *Left ));
	return Left;
}

static inline INT StrToEnum( FString EnumTypeStr, FString EnumStr, FString PkgStr=TEXT("UI") )
{
	guard(ImportUI::StrToEnum);
	NOTE(debugf(TEXT("UI::StrToEnum( %s, %s )"), *EnumTypeStr, *EnumStr ));
	if(EnumStr==TEXT("")) return 0;
	UEnum* E = Cast<UEnum>(UObject::StaticLoadObject( UEnum::StaticClass(), NULL, *(PkgStr+TEXT(".")+EnumTypeStr), NULL, LOAD_NoWarn | LOAD_Quiet, NULL ));
	if(!E) E = Cast<UEnum>(UObject::StaticLoadObject( UEnum::StaticClass(), NULL, *(FString(TEXT("Core."))+EnumTypeStr), NULL, LOAD_NoWarn | LOAD_Quiet, NULL ));
	if(!E) E = Cast<UEnum>(UObject::StaticLoadObject( UEnum::StaticClass(), NULL, *(FString(TEXT("Engine."))+EnumTypeStr), NULL, LOAD_NoWarn | LOAD_Quiet, NULL ));
	if(!E){ debugf( NAME_Warning, TEXT("StrToEnum: invalid enum type: %s"), *EnumTypeStr ); return 0; }
	for( INT i=0; i<E->Names.Num(); i++ )
	{
		FString S = *E->Names(i);
		S = S.Mid(S.InStr(FString(TEXT("_")))+1);
		NOTE(debugf(TEXT("  comparing enum: %s | %s"), *S, *EnumStr ));
		if( S==EnumStr )
			return i;
	}
	debugf( NAME_Warning, TEXT("StrToEnum: invalid enumeration: %s %s"), *EnumTypeStr, *EnumStr );
	return 0;
	unguard;
}

//
// Action=ConsoleCommand,set input space jump	(use ConsoleCommand followed by the command itself to send ConsoleCommands)
// Action=Event,Root.ShowMenu					(use Event to send uiEvents to any registered Component)
// Action=Player,TraceToggleActors				(use Player to call UnrealScript functions on the local client Controller)
// Action=PlayerPawn,GetHealth					(use PlayerPawn to call UnrealScript functions on the local client Controller's Pawn)
// Action=Weapon,GetLabelText					(use Player's current weapon to call UnrealScript function on.)
// Action=Keyframe|A1|B1|A2|B2...				(use Keyframe to easily tween between a set of floats over time.  An is the float value, Bn is how long it takes to tween to that value from the previous value.)
// Action=Exp(Parent.Label.Width/2+8)			(use Exp for specifying regular expressions.  See UIFeastNodes.h for details on special cases.)
// Action=NULL									(use NULL or None if you don't want to do anything.  Useful for Templates that don't always require Accessors.)
// Action=CodeMonkey,Quit						(otherwise specify a registered object followed by an UnrealScript function)
//
static inline ActionEvent* GetActionEvent( DWORD Type, FString* ParseText )
{
	guard(ImportUI::GetActionEvent);
	NOTE(debugf(TEXT("ImportUI::GetActionEvent( %s )"), *ParseText ));

	ActionEvent* AEvent=NULL;

	FString S=NEXT_TOKEN;
	FString SCaps=S.Caps();

	if( SCaps==TEXT("CONSOLECOMMAND") )
	{
		AEvent = new ConsoleCommandEvent(Type,*ParseText);
	}
	else if( SCaps==TEXT("EVENT") )
	{
		DECLARE_NEXT_TOKEN_SCOPED_EVENT;
		AEvent = new EventSender(Type,Scope,Event);
	}
	else if( SCaps==TEXT("DISABLEEVENT") )
	{
		DECLARE_NEXT_TOKEN_SCOPED_EVENT;
		AEvent = new EventDisabler(Type,Scope,Event);
	}
	else if( SCaps==TEXT("ENABLEEVENT") )
	{
		DECLARE_NEXT_TOKEN_SCOPED_EVENT;
		AEvent = new EventEnabler(Type,Scope,Event);
	}
	else if( SCaps==TEXT("PLAYER") )
	{
		AEvent = new PlayerEvent(Type,NEXT_TOKEN);
	}
	else if( SCaps==TEXT("PLAYERPAWN") )
	{
		AEvent = new PlayerPawnEvent(Type,NEXT_TOKEN);
	}
	else if( SCaps==TEXT("WEAPON") )
	{
		AEvent = new WeaponEvent(Type,NEXT_TOKEN);
	}
	else if( SCaps==TEXT("PLAYERID") )
	{
		AEvent = new PlayerIDEvent(Type,NEXT_TOKEN,GetGComponent());
	}
	else if( SCaps.InStr(TEXT("KEYFRAME"))==0 )
	{
		KeyframeEvent* Keyframe = new KeyframeEvent(Type);
		for( INT i=0; i<S.Len(); i++ )
			if( S.GetCharArray()(i)=='|' )
				S.GetCharArray()(i)=',';
		ParseText=&S; NEXT_TOKEN;	// hack off "Keyframe".
		while( *ParseText!=TEXT("") )
		{
			FLOAT Value=NEXT_TOKEN_FLOAT;
			FLOAT Delta=NEXT_TOKEN_FLOAT;
			Keyframe->AddFrame(Value,Delta);
		}
		AEvent = Keyframe;
	}
	else if( SCaps.InStr(TEXT("EXP"))==0 )
	{
		AEvent = new ExpressionEvent(Type,S.Mid(3),GetGComponent());
	}
	else if( SCaps==TEXT("GAMEFLAG") )
	{
		FString FlagName=NEXT_TOKEN;
		DECLARE_NEXT_TOKEN_BOOL_OPTX(Default,true);
		AEvent = new GameFlagEvent(Type,FlagName,Default);
	}
	else if( SCaps==TEXT("GRIFLAG") )
	{
		FString FlagName=NEXT_TOKEN;
		DECLARE_NEXT_TOKEN_BOOL_OPTX(Default,true);
		AEvent = new GRIFlagEvent(Type,FlagName,Default);
	}
	else if( SCaps==TEXT("PLAYERCOMMAND") )
	{
		AEvent = new PlayerCommandEvent(Type,*ParseText);
	}
	else if( SCaps==TEXT("TRIGGER") )
	{
		AEvent = new TriggerEvent(Type,FName(**ParseText));
	}
	else if( SCaps==TEXT("NULL") || SCaps==TEXT("NONE") )
	{
		AEvent = NULL;
	}
	else if( SCaps!=TEXT("") )
	{
		UObject* NotifyObj = GetObjMap().FindRef(S);
		if( NotifyObj )
			AEvent = new ScriptEvent(Type,NotifyObj,NEXT_TOKEN);
		else
			debugf( NAME_Warning, TEXT("ImportUI::GetActionEvent: Object '%s' is not a registered object.  (Use RegisterObj to register objects.)"), S );
	}

	return AEvent;

	unguard;
}

static inline FColor ParseColor( FString* ParseText )
{
	guard(ImportUI::ParseColor);
	NOTE(debugf(TEXT("ImportUI::ParseColor( %s )"), *ParseText ));

	// Pre-mixed colors (add more as deemed worthy).
	// from http://www.htmlgoodies.com/tutors/colors.html
	if( ParseText )
	{
		FString S=ParseText->Caps();
		// Fix ARL: Stuff these in a table so it's easier/faster to search for colors.
		if( S==TEXT("ALICEBLUE")			) return FColor(0xF0,0xF8,0xFF,0xFF);
		if( S==TEXT("ANTIQUEWHITE")			) return FColor(0xFA,0xEB,0xD7,0xFF);
		if( S==TEXT("AQUA")					) return FColor(0x00,0xFF,0xFF,0xFF);
		if( S==TEXT("AQUAMARINE")			) return FColor(0x7F,0xFF,0xD4,0xFF);
		if( S==TEXT("AZURE")				) return FColor(0xF0,0xFF,0xFF,0xFF);
		if( S==TEXT("BEIGE")				) return FColor(0xF5,0xF5,0xDC,0xFF);
		if( S==TEXT("BISQUE")				) return FColor(0xFF,0xE4,0xC4,0xFF);
		if( S==TEXT("BLACK")				) return FColor(0x00,0x00,0x00,0xFF);
		if( S==TEXT("BLANCHEDALMOND")		) return FColor(0xFF,0xEB,0xCD,0xFF);
		if( S==TEXT("BLUE")					) return FColor(0x00,0x00,0xFF,0xFF);
		if( S==TEXT("BLUEVIOLET")			) return FColor(0x8A,0x2B,0xE2,0xFF);
		if( S==TEXT("BROWN")				) return FColor(0xA5,0x2A,0x2A,0xFF);
		if( S==TEXT("BURLYWOOD")			) return FColor(0xDE,0xB8,0x87,0xFF);
		if( S==TEXT("CADETBLUE")			) return FColor(0x5F,0x9E,0xA0,0xFF);
		if( S==TEXT("CHARTREUSE")			) return FColor(0x7F,0xFF,0x00,0xFF);
		if( S==TEXT("CHOCOLATE")			) return FColor(0xD2,0x69,0x1E,0xFF);
		if( S==TEXT("CORAL")				) return FColor(0xFF,0x7F,0x50,0xFF);
		if( S==TEXT("CORNFLOWERBLUE")		) return FColor(0x64,0x95,0xED,0xFF);
		if( S==TEXT("CORNSILK")				) return FColor(0xFF,0xF8,0xDC,0xFF);
		if( S==TEXT("CRIMSON")				) return FColor(0xDC,0x14,0x3C,0xFF);
		if( S==TEXT("CYAN")					) return FColor(0x00,0xFF,0xFF,0xFF);
		if( S==TEXT("DARKBLUE")				) return FColor(0x00,0x00,0x8B,0xFF);
		if( S==TEXT("DARKCYAN")				) return FColor(0x00,0x8B,0x8B,0xFF);
		if( S==TEXT("DARKGOLDENROD")		) return FColor(0xB8,0x86,0x0B,0xFF);
		if( S==TEXT("DARKGRAY")				) return FColor(0xA9,0xA9,0xA9,0xFF);
		if( S==TEXT("DARKGREEN")			) return FColor(0x00,0x64,0x00,0xFF);
		if( S==TEXT("DARKKHAKI")			) return FColor(0xBD,0xB7,0x6B,0xFF);
		if( S==TEXT("DARKMAGENTA")			) return FColor(0x8B,0x00,0x8B,0xFF);
		if( S==TEXT("DARKOLIVEGREEN")		) return FColor(0x55,0x6B,0x2F,0xFF);
		if( S==TEXT("DARKORANGE")			) return FColor(0xFF,0x8C,0x00,0xFF);
		if( S==TEXT("DARKORCHID")			) return FColor(0x99,0x32,0xCC,0xFF);
		if( S==TEXT("DARKRED")				) return FColor(0x8B,0x00,0x00,0xFF);
		if( S==TEXT("DARKSALMON")			) return FColor(0xE9,0x96,0x7A,0xFF);
		if( S==TEXT("DARKSEAGREEN")			) return FColor(0x8F,0xBC,0x8F,0xFF);
		if( S==TEXT("DARKSLATEBLUE")		) return FColor(0x48,0x3D,0x8B,0xFF);
		if( S==TEXT("DARKSLATEGRAY")		) return FColor(0x2F,0x4F,0x4F,0xFF);
		if( S==TEXT("DARKTURQUOISE")		) return FColor(0x00,0xCE,0xD1,0xFF);
		if( S==TEXT("DARKVIOLET")			) return FColor(0x94,0x00,0xD3,0xFF);
		if( S==TEXT("DEEPPINK")				) return FColor(0xFF,0x14,0x93,0xFF);
		if( S==TEXT("DEEPSKYBLUE")			) return FColor(0x00,0xBF,0xFF,0xFF);
		if( S==TEXT("DIMGRAY")				) return FColor(0x69,0x69,0x69,0xFF);
		if( S==TEXT("DODGERBLUE")			) return FColor(0x1E,0x90,0xFF,0xFF);
		if( S==TEXT("FIREBRICK")			) return FColor(0xB2,0x22,0x22,0xFF);
		if( S==TEXT("FLORALWHITE")			) return FColor(0xFF,0xFA,0xF0,0xFF);
		if( S==TEXT("FORESTGREEN")			) return FColor(0x22,0x8B,0x22,0xFF);
		if( S==TEXT("FUCHSIA")				) return FColor(0xFF,0x00,0xFF,0xFF);
		if( S==TEXT("GAINSBORO")			) return FColor(0xDC,0xDC,0xDC,0xFF);
		if( S==TEXT("GHOSTWHITE")			) return FColor(0xF8,0xF8,0xFF,0xFF);
		if( S==TEXT("GOLD")					) return FColor(0xFF,0xD7,0x00,0xFF);
		if( S==TEXT("GOLDENROD")			) return FColor(0xDA,0xA5,0x20,0xFF);
		if( S==TEXT("GRAY")					) return FColor(0x80,0x80,0x80,0xFF);
		if( S==TEXT("GREEN")				) return FColor(0x00,0x80,0x00,0xFF);
		if( S==TEXT("GREENYELLOW")			) return FColor(0xAD,0xFF,0x2F,0xFF);
		if( S==TEXT("HONEYDEW")				) return FColor(0xF0,0xFF,0xF0,0xFF);
		if( S==TEXT("HOTPINK")				) return FColor(0xFF,0x69,0xB4,0xFF);
		if( S==TEXT("INDIANRED")			) return FColor(0xCD,0x5C,0x5C,0xFF);
		if( S==TEXT("INDIGO")				) return FColor(0x4B,0x00,0x82,0xFF);
		if( S==TEXT("IVORY")				) return FColor(0xFF,0xFF,0xF0,0xFF);
		if( S==TEXT("KHAKI")				) return FColor(0xF0,0xE6,0x8C,0xFF);
		if( S==TEXT("LAVENDAR")				) return FColor(0xE6,0xE6,0xFA,0xFF);
		if( S==TEXT("LAVENDERBLUSH")		) return FColor(0xFF,0xF0,0xF5,0xFF);
		if( S==TEXT("LAWNGREEN")			) return FColor(0x7C,0xFC,0x00,0xFF);
		if( S==TEXT("LEMONCHIFFON")			) return FColor(0xFF,0xFA,0xCD,0xFF);
		if( S==TEXT("LIGHTBLUE")			) return FColor(0xAD,0xD8,0xE6,0xFF);
		if( S==TEXT("LIGHTCORAL")			) return FColor(0xF0,0x80,0x80,0xFF);
		if( S==TEXT("LIGHTCYAN")			) return FColor(0xE0,0xFF,0xFF,0xFF);
		if( S==TEXT("LIGHTGOLDENRODYELLOW")	) return FColor(0xFA,0xFA,0xD2,0xFF);
		if( S==TEXT("LIGHTGREEN")			) return FColor(0x90,0xEE,0x90,0xFF);
		if( S==TEXT("LIGHTGREY")			) return FColor(0xD3,0xD3,0xD3,0xFF);
		if( S==TEXT("LIGHTPINK")			) return FColor(0xFF,0xB6,0xC1,0xFF);
		if( S==TEXT("LIGHTSALMON")			) return FColor(0xFF,0xA0,0x7A,0xFF);
		if( S==TEXT("LIGHTSEAGREEN")		) return FColor(0x20,0xB2,0xAA,0xFF);
		if( S==TEXT("LIGHTSKYBLUE")			) return FColor(0x87,0xCE,0xFA,0xFF);
		if( S==TEXT("LIGHTSLATEGRAY")		) return FColor(0x77,0x88,0x99,0xFF);
		if( S==TEXT("LIGHTSTEELBLUE")		) return FColor(0xB0,0xC4,0xDE,0xFF);
		if( S==TEXT("LIGHTYELLOW")			) return FColor(0xFF,0xFF,0xE0,0xFF);
		if( S==TEXT("LIME")					) return FColor(0x00,0xFF,0x00,0xFF);
		if( S==TEXT("LIMEGREEN")			) return FColor(0x32,0xCD,0x32,0xFF);
		if( S==TEXT("LINEN")				) return FColor(0xFA,0xF0,0xE6,0xFF);
		if( S==TEXT("MAGENTA")				) return FColor(0xFF,0x00,0xFF,0xFF);
		if( S==TEXT("MAROON")				) return FColor(0x80,0x00,0x00,0xFF);
		if( S==TEXT("MEDIUMAUQAMARINE")		) return FColor(0x66,0xCD,0xAA,0xFF);
		if( S==TEXT("MEDIUMBLUE")			) return FColor(0x00,0x00,0xCD,0xFF);
		if( S==TEXT("MEDIUMORCHID")			) return FColor(0xBA,0x55,0xD3,0xFF);
		if( S==TEXT("MEDIUMPURPLE")			) return FColor(0x93,0x70,0xD8,0xFF);
		if( S==TEXT("MEDIUMSEAGREEN")		) return FColor(0x3C,0xB3,0x71,0xFF);
		if( S==TEXT("MEDIUMSLATEBLUE")		) return FColor(0x7B,0x68,0xEE,0xFF);
		if( S==TEXT("MEDIUMSPRINGGREEN")	) return FColor(0x00,0xFA,0x9A,0xFF);
		if( S==TEXT("MEDIUMTURQUOISE")		) return FColor(0x48,0xD1,0xCC,0xFF);
		if( S==TEXT("MEDIUMVIOLETRED")		) return FColor(0xC7,0x15,0x85,0xFF);
		if( S==TEXT("MIDNIGHTBLUE")			) return FColor(0x19,0x19,0x70,0xFF);
		if( S==TEXT("MINTCREAM")			) return FColor(0xF5,0xFF,0xFA,0xFF);
		if( S==TEXT("MISTYROSE")			) return FColor(0xFF,0xE4,0xE1,0xFF);
		if( S==TEXT("MOCCASIN")				) return FColor(0xFF,0xE4,0xB5,0xFF);
		if( S==TEXT("NAVAJOWHITE")			) return FColor(0xFF,0xDE,0xAD,0xFF);
		if( S==TEXT("NAVY")					) return FColor(0x00,0x00,0x80,0xFF);
		if( S==TEXT("OLDLACE")				) return FColor(0xFD,0xF5,0xE6,0xFF);
		if( S==TEXT("OLIVE")				) return FColor(0x80,0x80,0x00,0xFF);
		if( S==TEXT("OLIVEDRAB")			) return FColor(0x68,0x8E,0x23,0xFF);
		if( S==TEXT("ORANGE")				) return FColor(0xFF,0xA5,0x00,0xFF);
		if( S==TEXT("ORANGERED")			) return FColor(0xFF,0x45,0x00,0xFF);
		if( S==TEXT("ORCHID")				) return FColor(0xDA,0x70,0xD6,0xFF);
		if( S==TEXT("PALEGOLDENROD")		) return FColor(0xEE,0xE8,0xAA,0xFF);
		if( S==TEXT("PALEGREEN")			) return FColor(0x98,0xFB,0x98,0xFF);
		if( S==TEXT("PALETURQUOISE")		) return FColor(0xAF,0xEE,0xEE,0xFF);
		if( S==TEXT("PALEVIOLETRED")		) return FColor(0xD8,0x70,0x93,0xFF);
		if( S==TEXT("PAPAYAWHIP")			) return FColor(0xFF,0xEF,0xD5,0xFF);
		if( S==TEXT("PEACHPUFF")			) return FColor(0xFF,0xDA,0xB9,0xFF);
		if( S==TEXT("PERU")					) return FColor(0xCD,0x85,0x3F,0xFF);
		if( S==TEXT("PINK")					) return FColor(0xFF,0xC0,0xCB,0xFF);
		if( S==TEXT("PLUM")					) return FColor(0xDD,0xA0,0xDD,0xFF);
		if( S==TEXT("POWDERBLUE")			) return FColor(0xB0,0xE0,0xE6,0xFF);
		if( S==TEXT("PURPLE")				) return FColor(0x80,0x00,0x80,0xFF);
		if( S==TEXT("RED")					) return FColor(0xFF,0x00,0x00,0xFF);
		if( S==TEXT("ROSYBROWN")			) return FColor(0xBC,0x8F,0x8F,0xFF);
		if( S==TEXT("ROYALBLUE")			) return FColor(0x41,0x69,0xE1,0xFF);
		if( S==TEXT("SADDLEBROWN")			) return FColor(0x8B,0x45,0x13,0xFF);
		if( S==TEXT("SALMON")				) return FColor(0xFA,0x80,0x72,0xFF);
		if( S==TEXT("SANDYBROWN")			) return FColor(0xF4,0xA4,0x60,0xFF);
		if( S==TEXT("SEAGREEN")				) return FColor(0x2E,0x8B,0x57,0xFF);
		if( S==TEXT("SEASHELL")				) return FColor(0xFF,0xF5,0xEE,0xFF);
		if( S==TEXT("SIENNA")				) return FColor(0xA0,0x52,0x2D,0xFF);
		if( S==TEXT("SILVER")				) return FColor(0xC0,0xC0,0xC0,0xFF);
		if( S==TEXT("SKYBLUE")				) return FColor(0x87,0xCE,0xEB,0xFF);
		if( S==TEXT("SLATEBLUE")			) return FColor(0x6A,0x5A,0xCD,0xFF);
		if( S==TEXT("SLATEGRAY")			) return FColor(0x70,0x80,0x90,0xFF);
		if( S==TEXT("SNOW")					) return FColor(0xFF,0xFA,0xFA,0xFF);
		if( S==TEXT("SPRINGGREEN")			) return FColor(0x00,0xFF,0x7F,0xFF);
		if( S==TEXT("STEELBLUE")			) return FColor(0x46,0x82,0xB4,0xFF);
		if( S==TEXT("TAN")					) return FColor(0xD2,0xB4,0x8C,0xFF);
		if( S==TEXT("TEAL")					) return FColor(0x00,0x80,0x80,0xFF);
		if( S==TEXT("THISTLE")				) return FColor(0xD8,0xBF,0xD8,0xFF);
		if( S==TEXT("TOMATO")				) return FColor(0xFF,0x63,0x47,0xFF);
		if( S==TEXT("TURQUOISE")			) return FColor(0x40,0xE0,0xD0,0xFF);
		if( S==TEXT("VIOLET")				) return FColor(0xEE,0x82,0xEE,0xFF);
		if( S==TEXT("WHEAT")				) return FColor(0xF5,0xDE,0xB3,0xFF);
		if( S==TEXT("WHITE")				) return FColor(0xFF,0xFF,0xFF,0xFF);
		if( S==TEXT("WHITESMOKE")			) return FColor(0xF5,0xF5,0xF5,0xFF);
		if( S==TEXT("YELLOW")				) return FColor(0xFF,0xFF,0x00,0xFF);
		if( S==TEXT("YELLOWGREEN")			) return FColor(0x9A,0xCD,0x32,0xFF);
	}

	DECLARE_NEXT_TOKEN_INT_OPTX(R,0);
	DECLARE_NEXT_TOKEN_INT_OPTX(G,0);
	DECLARE_NEXT_TOKEN_INT_OPTX(B,0);
	DECLARE_NEXT_TOKEN_INT_OPTX(A,255);

	return FColor(R,G,B,A);

	unguard;
}


template<class T> inline T* uiLoadObject( const FString &S )
{
	guard(ImportUI::LoadObject);
	T* obj = Cast<T>(UObject::StaticFindObject( T::StaticClass(), NULL, *S ));
	if(!obj) obj = CastChecked<T>(UObject::StaticLoadObject( T::StaticClass(), NULL, *S, NULL, LOAD_NoWarn, NULL ));
	GetGConsole()->CollectAsset(obj);
	return obj;
	unguard;
}


//
// Polymorphically loads material using StaticLoadObject.
//
static inline UMaterial* uiLoadMaterial( const FString &S )
{
	guard(ImportUI::LoadMaterial);

	for( TObjectIterator<UClass> It; It; ++It )
	{
		if( It->IsChildOf(UMaterial::StaticClass()) )
		{
			UMaterial* M = Cast<UMaterial>(UObject::StaticLoadObject( *It, NULL, *S, NULL, LOAD_Quiet|LOAD_NoWarn, NULL ));
			if(M)
			{
				GetGConsole()->CollectAsset(M);
				return M;
			}
		}
	}

	return NULL;

	unguard;
}

