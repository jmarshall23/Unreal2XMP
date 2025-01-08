//=============================================================================
// UI.h: UI-specific package header
//=============================================================================

#ifndef _INC_UI
#define _INC_UI

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef UI_API
	#define UI_API DLL_IMPORT
#endif

#ifndef UI_CALLBACK
	#define UI_CALLBACK __cdecl
#endif

/*-----------------------------------------------------------------------------
	Dependencies.
-----------------------------------------------------------------------------*/

#include "Core.h"
#include "Engine.h"
#include "UnCon.h"

#include "..\..\feast\inc\feast.h"

#include "THashTable.h"
#include "TDoubleLinkList.h"


/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

class uiClass;
class UUIObject;
class UComponent;
class UContainer;
class UMouseCursor;
class ActionEvent;
class ComponentNotifyInfo;

enum EFontSize;

typedef UComponent* FComponentHandle;	// for script access.


/*-----------------------------------------------------------------------------
	Debugging.
-----------------------------------------------------------------------------*/

//#define NOTE(func) func
#define NOTE(func)
#define NOTE0(func)
#define NOTE1(func) func


/*-----------------------------------------------------------------------------
	Additional serialization support.
-----------------------------------------------------------------------------*/

#include "SerializeUI.h"


/*-----------------------------------------------------------------------------
	Access support.
-----------------------------------------------------------------------------*/

#define BIT(n) (1<<(n))

enum AccessorMasks
{
	ACCESSOR_None			=0,

	//Component
	ACCESSOR_Location		=BIT(0),
	ACCESSOR_X				=BIT(1),
	ACCESSOR_Y				=BIT(2),
	ACCESSOR_Size			=BIT(3),
	ACCESSOR_Width			=BIT(4),
	ACCESSOR_Height			=BIT(5),
	ACCESSOR_Color			=BIT(6),
	ACCESSOR_Alpha			=BIT(7),
	ACCESSOR_R				=BIT(8),
	ACCESSOR_G				=BIT(9),
	ACCESSOR_B				=BIT(10),
	ACCESSOR_Enabled		=BIT(11),
	ACCESSOR_OnEnabled		=BIT(12),
	ACCESSOR_MouseEnter		=BIT(13),
	ACCESSOR_MouseExit		=BIT(14),
	ACCESSOR_MousePress		=BIT(15),
	ACCESSOR_MouseRelease	=BIT(16),
	ACCESSOR_MouseClick		=BIT(17),
#define MAX_BIT_COMPONENT		(18)

	//Additional bases
#define MAX_BIT_CONTAINER					(MAX_BIT_COMPONENT+0)
#define MAX_BIT_FIXEDSIZECONTAINER			(MAX_BIT_CONTAINER+0)
#define MAX_BIT_REALCOMPONENT				(MAX_BIT_COMPONENT+0)
#define MAX_BIT_IMAGECOMPONENTBASE			(MAX_BIT_REALCOMPONENT+0)
#define MAX_BIT_TEXTCOMPONENTBASE			(MAX_BIT_REALCOMPONENT+0)

	//MultiStateComponent
	ACCESSOR_State						=BIT(MAX_BIT_CONTAINER+0),
#define MAX_BIT_MULTISTATECOMPONENT			(MAX_BIT_CONTAINER+1)

	//Button
	ACCESSOR_ButtonAction				=BIT(MAX_BIT_CONTAINER+0),
#define MAX_BIT_BUTTON						(MAX_BIT_CONTAINER+1)

	//ImageComponent
	ACCESSOR_ImageComponent				=BIT(MAX_BIT_IMAGECOMPONENTBASE+0),
#define MAX_BIT_IMAGECOMPONENT				(MAX_BIT_IMAGECOMPONENTBASE+1)

	//Checkbox
	ACCESSOR_CheckboxAccess				=BIT(MAX_BIT_IMAGECOMPONENT+0),
	ACCESSOR_CheckboxModify				=BIT(MAX_BIT_IMAGECOMPONENT+1),
#define MAX_BIT_CHECKBOX					(MAX_BIT_IMAGECOMPONENT+2)

	//Frame
	ACCESSOR_FrameLocationAccess		=BIT(MAX_BIT_CONTAINER+0),
	ACCESSOR_FrameLocationModify		=BIT(MAX_BIT_CONTAINER+1),
	ACCESSOR_FrameSizeAccess			=BIT(MAX_BIT_CONTAINER+2),
	ACCESSOR_FrameSizeModify			=BIT(MAX_BIT_CONTAINER+3),
	ACCESSOR_FrameWidthPctAccess		=BIT(MAX_BIT_CONTAINER+4),
	ACCESSOR_FrameWidthPctModify		=BIT(MAX_BIT_CONTAINER+5),
	ACCESSOR_FrameHeightPctAccess		=BIT(MAX_BIT_CONTAINER+6),
	ACCESSOR_FrameHeightPctModify		=BIT(MAX_BIT_CONTAINER+7),
	ACCESSOR_FrameXPctAccess			=BIT(MAX_BIT_CONTAINER+8),
	ACCESSOR_FrameXPctModify			=BIT(MAX_BIT_CONTAINER+9),
	ACCESSOR_FrameYPctAccess			=BIT(MAX_BIT_CONTAINER+10),
	ACCESSOR_FrameYPctModify			=BIT(MAX_BIT_CONTAINER+11),
#define MAX_BIT_FRAME						(MAX_BIT_CONTAINER+12)
/*
	ACCESSOR_FrameXAbsPct				=BIT(MAX_BIT_CONTAINER+12),
	ACCESSOR_FrameYAbsPct				=BIT(MAX_BIT_CONTAINER+13),
#define MAX_BIT_FRAME						(MAX_BIT_CONTAINER+14)
*/
	//ImageWedge
	ACCESSOR_ImageWedgeRadius			=BIT(MAX_BIT_IMAGECOMPONENT+0),
	ACCESSOR_ImageWedgeAngle			=BIT(MAX_BIT_IMAGECOMPONENT+1),
#define MAX_BIT_IMAGEWEDGE					(MAX_BIT_IMAGECOMPONENT+2)

	//ImageWipe
	ACCESSOR_ImageWipe					=BIT(MAX_BIT_FIXEDSIZECONTAINER+0),
#define MAX_BIT_IMAGEWIPE					(MAX_BIT_FIXEDSIZECONTAINER+1)

	//Label
	ACCESSOR_Label						=BIT(MAX_BIT_TEXTCOMPONENTBASE+0),
	ACCESSOR_LabelFloatAccess			=BIT(MAX_BIT_TEXTCOMPONENTBASE+1),
#define MAX_BIT_LABEL						(MAX_BIT_TEXTCOMPONENTBASE+2)

	//InputSelector
	ACCESSOR_InputSelector				=BIT(MAX_BIT_LABEL+0),
	ACCESSOR_InputSelectorReset			=BIT(MAX_BIT_LABEL+1),
	ACCESSOR_InputSelectorDisplay		=BIT(MAX_BIT_LABEL+2),
	ACCESSOR_StartListening				=BIT(MAX_BIT_LABEL+3),
	ACCESSOR_StopListening				=BIT(MAX_BIT_LABEL+4),
#define MAX_BIT_INPUTSELECTOR				(MAX_BIT_LABEL+5)

	//MultiImageComponent
	ACCESSOR_MultiImageComponent		=BIT(MAX_BIT_IMAGECOMPONENTBASE+0),
#define MAX_BIT_MULTIIMAGECOMPONENT			(MAX_BIT_IMAGECOMPONENTBASE+1)

	//AnimatedImageComponent
	ACCESSOR_AnimatedImageTween			=BIT(MAX_BIT_IMAGECOMPONENTBASE+0),
#define MAX_BIT_ANIMATEDIMAGECOMPONENT		(MAX_BIT_IMAGECOMPONENTBASE+1)

	//NumWatcher
	ACCESSOR_NumWatcherWatch			=BIT(MAX_BIT_TEXTCOMPONENTBASE+0),
#define MAX_BIT_NUMWATCHER					(MAX_BIT_TEXTCOMPONENTBASE+1)

	//ScriptComponent
	ACCESSOR_ScriptComponentTarget		=BIT(MAX_BIT_COMPONENT+0),
#define MAX_BIT_SCRIPTCOMPONENT				(MAX_BIT_COMPONENT+1)

	//ScrollBar
	ACCESSOR_ScrollBarGetValue			=BIT(MAX_BIT_FIXEDSIZECONTAINER+0),
	ACCESSOR_ScrollBarGetRange			=BIT(MAX_BIT_FIXEDSIZECONTAINER+1),
	ACCESSOR_ScrollBarGetVisibleRange	=BIT(MAX_BIT_FIXEDSIZECONTAINER+2),
	ACCESSOR_ScrollBarSetValue			=BIT(MAX_BIT_FIXEDSIZECONTAINER+3),
	ACCESSOR_ScrollBarSetRange			=BIT(MAX_BIT_FIXEDSIZECONTAINER+4),
	ACCESSOR_ScrollBarSetVisibleRange	=BIT(MAX_BIT_FIXEDSIZECONTAINER+5),
#define MAX_BIT_SCROLLBAR					(MAX_BIT_FIXEDSIZECONTAINER+6)

	//Selector
	ACCESSOR_SelectorAccess				=BIT(MAX_BIT_CONTAINER+0),
	ACCESSOR_SelectorText				=BIT(MAX_BIT_CONTAINER+1),
	ACCESSOR_SelectorCurrent			=BIT(MAX_BIT_CONTAINER+2),
	ACCESSOR_SelectorPreSelect			=BIT(MAX_BIT_CONTAINER+3),
	ACCESSOR_SelectorPostSelect			=BIT(MAX_BIT_CONTAINER+4),
	ACCESSOR_SelectorLocked				=BIT(MAX_BIT_CONTAINER+5),
	ACCESSOR_SelectorModify				=BIT(MAX_BIT_CONTAINER+6),	// note: this is shared by selectoritem, so if we add additional accessors to label this will start causing us problems.
	ACCESSOR_SelectorModifyIndex		=BIT(MAX_BIT_CONTAINER+7),	// (same applies)
#define MAX_BIT_SELECTOR					(MAX_BIT_CONTAINER+8)

	//Slider
	ACCESSOR_SliderAccess				=BIT(MAX_BIT_IMAGECOMPONENT+0),
	ACCESSOR_SliderModify				=BIT(MAX_BIT_IMAGECOMPONENT+1),
#define MAX_BIT_SLIDER						(MAX_BIT_IMAGECOMPONENT+2)

	//TextField
	ACCESSOR_TextFieldCommand			=BIT(MAX_BIT_TEXTCOMPONENTBASE+0),
	ACCESSOR_TextFieldAccess			=BIT(MAX_BIT_TEXTCOMPONENTBASE+1),
	ACCESSOR_TextFieldEnterEvent		=BIT(MAX_BIT_TEXTCOMPONENTBASE+2),
	ACCESSOR_TextFieldEscapeEvent		=BIT(MAX_BIT_TEXTCOMPONENTBASE+3),
#define MAX_BIT_TEXTFIELD					(MAX_BIT_TEXTCOMPONENTBASE+4)

	//TextArea
	ACCESSOR_TextAreaAccess				=BIT(MAX_BIT_FIXEDSIZECONTAINER+0),
#define MAX_BIT_TEXTAREA					(MAX_BIT_FIXEDSIZECONTAINER+1)
};


/*-----------------------------------------------------------------------------
	FNameTable.
	Name index (Names) stored in hi 16 bits.
	Name subscript (Indices) stored in lo 16 bits.
-----------------------------------------------------------------------------*/

#define UI_TransientMask BIT(15)

class UI_API uiName
{
public:
	uiName(){}
	uiName(FString);
	uiName(DWORD _Name): Name(_Name){}
	uiName(_WORD _Name,_WORD _Sub):iName(_Name),iSub(_Sub){}
	_WORD GetIndex(){ return iSub; }
	UBOOL operator==(const uiName &Other){ return Name==Other.Name; }
	UBOOL LooseCompare(const uiName &Other){ return iName==Other.iName; }	// I wish C++ would allow me to use the ~= operator.
	FString operator*();	// get human readable text.
	union { struct{ _WORD iName,iSub; }; DWORD Name; };
	friend FArchive &operator<<( FArchive& Ar, uiName& N ){ return Ar << N.Name; }
	static uiName None();
	static uiName Parent();
};

typedef TArray<uiName> uiPath;	// Chain of uiNames identifying a componet's location to another.
FString UI_API GetPathString( uiPath Path );

class UI_API FNameTable
{
public:
	FNameTable():Names(),Indices(){}
	static FNameTable* GetInstance()
	{
		// Fix ARL: Memory leak!!
		static FNameTable* StaticNameTable = new FNameTable();
		return StaticNameTable;
	}
	uiName FindName(FString Name)
	{
		NOTE(debugf(TEXT("(%d)FNameTable::FindName( %s )"),this,*Name));
		// Strip off subscript.
		static TCHAR BaseName[1024];	// Fix ARL: Probably doesn't need to be this big.
		appStrcpy( BaseName, *Name );
		TCHAR* End = BaseName + appStrlen(BaseName);
		checkSlow(*End==0);
		while( End>BaseName && appIsDigit(End[-1]) )
			End--;
		INT Sub = appAtoi(End);
		*End = 0;
		NOTE(debugf(TEXT("   looking for %s%d"),BaseName,Sub));
		// linear search for existing entry.	// Fix ARL: SLOOOOOW!!!
		for(_WORD i=0;i<Names.Num();i++)
			if(Names(i)==BaseName)
			{
				NOTE(debugf(TEXT("   Names[%d]=%s Indices[%d]=%d"),i,*Names(i),i,(DWORD)Indices(i)));
				if( Sub<=Indices(i) )
					return uiName(i,Sub);
				return uiName::None();
			}
		return uiName::None();
	}
	uiName CreateName(const TCHAR* Name)
	{
		NOTE(debugf(TEXT("(%d)FNameTable::CreateName( %s )"),this,Name));
		// linear search for existing entry.	// Fix ARL: SLOOOOOW!!!
		for(_WORD i=0;i<Names.Num();i++)
			if(Names(i)==Name)
			{
				Indices(i)++;
				if(Indices(i) & UI_TransientMask)
					appErrorf(TEXT("FNameTable: Capacity exceeded for name %s!"),Names(i));
				return uiName(i,Indices(i));
			}
		// not found, add a new entry.
		_WORD n=Names.Num();
		new(Names)FString(Name);
		Indices.AddZeroed();	//NOTE[aleiby]: The first element of one or the other of these will never get used, but too bad... we need to keep the array sizes in sync with each other.
		TransientIndices.AddItem(UI_TransientMask);
		check(Names.Num()==Indices.Num());	//checkSlow
		check(Names.Num()==TransientIndices.Num());	//checkSlow
		if(Names.Num()==MAXINT) appErrorf(TEXT("FNameTable: Name entry overflow!"));
		return uiName(n,Indices(n));
	}
	uiName CreateTransientName(const TCHAR* Name)
	{
		// linear search for existing entry.	// Fix ARL: SLOOOOOW!!!
		for(_WORD i=0;i<Names.Num();i++)
			if(Names(i)==Name)
			{
				TransientIndices(i)++;
				if(TransientIndices(i)==0)
					TransientIndices(i)=UI_TransientMask;
				return uiName(i,TransientIndices(i));
			}
		// not found, add a new entry.
		_WORD n=Names.Num();
		new(Names)FString(Name);
		Indices.AddZeroed();	//NOTE[aleiby]: The first element of one or the other of these will never get used, but too bad... we need to keep the array sizes in sync with each other.
		TransientIndices.AddItem(UI_TransientMask);
		check(Names.Num()==Indices.Num());	//checkSlow
		check(Names.Num()==TransientIndices.Num());	//checkSlow
		if(Names.Num()==MAXINT) appErrorf(TEXT("FNameTable: Name entry overflow!"));
		return uiName(n,TransientIndices(n));
	}
	FString GetName(uiName Name)
	{
		//NOTE[aleiby]: Maybe use a static circular array of TCHAR[1024]s and copy the name
		// into that for temporary use only.  This will mean we no longer have to allocate
		// tons of FStrings on the stack, but it also means that all code using GetFullName, etc.
		// will needs to be aware that the string (const TCHAR*) returned is only temporary
		// and will get overwritten when the circular array wraps back around.
#if 0 //NEW
		// Update: I tried it, and it's actually slightly slower.  Go figure.
		// Sticking with FStrings since that's safer.
		if(Name.iName<Names.Num())
		{
			static TCHAR TempNames[16][256];
			static BYTE NameIndex=0;
			NameIndex = (NameIndex+1)%ARRAY_COUNT(TempNames);
			TCHAR* TempName = TempNames[ NameIndex ];
			appSprintf( TempName, TEXT("%s%d"), *Names(Name.iName), (DWORD)Name.iSub );
			return TempName;
		}
		else return TEXT("Unknown");
#else		
		if(Name.iName<Names.Num())
			return FString::Printf(TEXT("%s%d"),*Names(Name.iName),(DWORD)Name.iSub);
		return FString(TEXT("Unknown"));
#endif
	}
	void Serialize(FArchive& Ar)
	{
		Ar << Names << Indices;

		if( Ar.IsLoading() )
		{
			TransientIndices.Empty();
			TransientIndices.Add(Indices.Num());
			for(INT i=0;i<TransientIndices.Num();i++)
				TransientIndices(i)=UI_TransientMask;
		}
	}
private:
	TArray<FString> Names;
	TArray<_WORD> Indices;
	TArray<_WORD> TransientIndices;
};


/*-----------------------------------------------------------------------------
	Component creation.
-----------------------------------------------------------------------------*/

#define NEW_COMPONENT(clas) CreateComponent<clas>(TEXT(#clas))
#define NEW_SUBCOMPONENT(clas) CreateComponent<clas>(TEXT(#clas),IsTransient())
#define NEW_TRANSCOMPONENT(clas) CreateComponent<clas>(TEXT(#clas),1)

void UI_API StaticInitComponent( UComponent* C, const TCHAR* Name, UBOOL bTransient=0 );

template< class T > T* CreateComponent( const TCHAR* Name, UBOOL bTransient=0 )
{
	T* C = new T();
	StaticInitComponent(C,Name,bTransient);
	return C;
}

uiClass* StringFindClass( const TCHAR* ClassName, UBOOL bCritical=0 );
UComponent* StringCreateComponent( const TCHAR* Class, const TCHAR* Name=TEXT("") );

template< class T > T* DynamicCastChecked( UComponent* A )
{
	T* C = DynamicCast<T>(A);
	if(!C) appErrorf(TEXT("DynamicCastChecked of %s to %s failed!"),A?*A->GetName():TEXT("NULL"),T::StaticClass()->GetName());
	return C;
}

template< class T > T* DynamicCast( UComponent* A )
{
	return (A && A->IsA(T::StaticClass())) ? (T*)A : NULL;
}


/*-----------------------------------------------------------------------------
	uiEvent.
-----------------------------------------------------------------------------*/

typedef signed int uiEvent;		// Fix ARL: This should be unsigned, but UnrealScript exports the definitions simply as INTs which are signed.
typedef THashTable<FString,uiEvent> TEventTable;

#define EVENT_None 0
#define UI_EVENT(e) GetEvent(TEXT(#e))

uiEvent UI_API GetEvent( FString EventName );
FString LookupEventName( uiEvent Event );

TEventTable UI_API &GetuiEventTable();

// Not sure why this doesn't "just work" in THashTable itself.
//FArchive UI_API &operator<<( FArchive &Ar, TEventTable &H );


/*-----------------------------------------------------------------------------
	Helpers.
-----------------------------------------------------------------------------*/

inline INT WrappedIncrement( INT& i, INT Limit )
{
	guard(UI::WrappedIncrement);
	check(Limit>0);
	i=(i+1)%Limit;
	return i;
	unguard;
}

inline INT WrappedDecrement( INT& i, INT Limit )
{
	guard(UI::WrappedDecrement);
	check(Limit>0);
	i--;
	if(i>=0)
		i=i%Limit;
	else
		i=Limit-((((Limit-i)-1)%Limit)+1);
	return i;
	unguard;
}

inline FString Chr( INT i )
{
	guard(UI::Chr);

	TCHAR Temp[2];
	Temp[0] = i;
	Temp[1] = 0;

	return (FString)Temp;

	unguard;
}


/*-----------------------------------------------------------------------------
	Function parms.
-----------------------------------------------------------------------------*/

struct KeyTypeParms
{
    EInputKey Key;
    UBOOL bHandled;	// ReturnValue
};

struct KeyEventParms
{
    EInputKey Key;
    EInputAction Action;
    FLOAT Delta;
    UBOOL bHandled;	// ReturnValue
};

struct MouseNotifyParms
{
	UMouseCursor* Mouse;
	UBOOL bHandled;	// ReturnValue
};


/*-----------------------------------------------------------------------------
	ActionEvents.
-----------------------------------------------------------------------------*/

#include "ActionEvents.h"


/*-----------------------------------------------------------------------------
	Typedefs.
-----------------------------------------------------------------------------*/

typedef TMultiMap<FString,FString>	RealSectionData;
typedef RealSectionData* 			SectionData;

typedef ComponentNotifyInfo			NotifyTypeClass;
typedef NotifyTypeClass*			NotifyType;
typedef TOrderedArray<NotifyType>	NotifyListType;
typedef NotifyListType*				NotifyList;

FArchive UI_API &operator<<( FArchive &Ar, NotifyList &N );
FArchive UI_API &operator<<( FArchive &Ar, FImage* &N );

/*-----------------------------------------------------------------------------
	Structures.
-----------------------------------------------------------------------------*/

struct UI_API FSTRUCT_TriggerDef
{
	FSTRUCT_TriggerDef(){}
    FLOAT Time;
    class ActionEvent* Event;
	FSTRUCT_TriggerDef( FLOAT _Time, ActionEvent* _Event ): Time(_Time), Event(_Event) {}
	FSTRUCT_TriggerDef operator=( INT Value ){ Time=Value; Event=NULL; return *this; }
	friend FArchive& operator<<( FArchive& Ar, FSTRUCT_TriggerDef& T )
	{
		return Ar << T.Time << T.Event;
	}
};

struct UI_API FSTRUCT_Transition
{
	FSTRUCT_Transition(){}
    uiEvent Event[4];
    INT Start;
    INT End;
    FLOAT Time;
    class UComponent* Template;
    BITFIELD bNoOverride:1 GCC_PACK(4);
	BITFIELD bReverseEase:1;
    TArray<FSTRUCT_TriggerDef> Triggers GCC_PACK(4);
	friend FArchive& operator<<( FArchive& Ar, FSTRUCT_Transition& T )
	{
		Ar << T.Event[0] << T.Event[1] << T.Event[2] << T.Event[3] << T.Start << T.End << T.Time;
		//T.Template serialized outside of this scope (see UMultiStateComponent::Serialize)
		SERIALIZE_BOOL(T.bNoOverride);
		SERIALIZE_BOOL(T.bReverseEase);
		Ar << T.Triggers;
		return Ar;
	}
};


/*-----------------------------------------------------------------------------
	Includes.
-----------------------------------------------------------------------------*/

#include "UIClasses.h"
#include "UIComponents.h"

//Importing Interface
UComponent* ImportComponent( FString Name, UObject* Outer, FString* Overrides, UBOOL bReload=false );

UI_API	extern	INT					ImportIndentLevel;

#endif //_INC_UI

//-----------------------------------------------------------------------------
//	end of UI.h
//-----------------------------------------------------------------------------
