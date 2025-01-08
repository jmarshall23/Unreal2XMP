/*=============================================================================
	UIComponents.h: UI Component definitions.
	Copyright (C) 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron R Leiby
=============================================================================*/

// Fix ARL: Need to empty all TArrays, etc. in Destroy functions.

#include "SmallObj.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif

enum EFontSize
{
	FS_Huge 				=0,
	FS_Big					=1,
	FS_Medium				=2,
	FS_Small				=3,
	FS_Smallest 			=4,
	FS_ReallySmall			=5,
	FS_CompletelyUnreadable =6,
};

class UI_API uiClass : public Loki::SmallObject
{
public:
	uiClass(uiClass* _Super,const TCHAR* _Name)
		:Super(_Super),Name(_Name)
	{
		// add to the global class list.
		NOTE(debugf(TEXT("Creating class: %s"),GetName()));
		ClassList = GetClassList();
		GetClassList() = this;
		NOTE(for(uiClass* C=uiClass::GetClassList();C;C=C->NextClass()) debugf(TEXT("%s"),C->GetName()));
	}
	uiClass* GetSuper(){ return Super; }
	const TCHAR* GetName(){ return Name; }
	uiClass* NextClass(){ return ClassList; }
	virtual UComponent* CreateInstance(const TCHAR* Name)=0;
	virtual UComponent* CreateInstanceNoInit()=0;
	static uiClass* &GetClassList(){ static uiClass* ClassList=NULL; return ClassList; }
private:
	uiClass* Super;
	const TCHAR* Name;
	uiClass* ClassList;
};
inline DWORD GetTypeHash( const uiClass* A ){ return (DWORD)A; }

#define DECLARE_UI_BASECLASS_DEF( TClass, TSuperClass, TuiSuperClass, TCreateInstance, TCreateInstanceNoInit ) \
public: \
	typedef TSuperClass Super;\
	TClass(){} /*uninitialized*/ \
	virtual ~TClass(){} \
	class TClass##Class : public uiClass { \
		public: TClass##Class(uiClass* C,const TCHAR* N):uiClass(C,N){} \
		public: UComponent* CreateInstance(const TCHAR* Name){ return TCreateInstance; } \
		public: UComponent* CreateInstanceNoInit(){ return TCreateInstanceNoInit; } }; \
	static uiClass* StaticClass(){ static uiClass* MyClass = new TClass##Class(TuiSuperClass,TEXT(#TClass)); return MyClass; } \
	virtual uiClass* GetClass(){ return TClass::StaticClass(); } \

#define DECLARE_UI_BASECLASS( TClass, TSuperClass ) \
	DECLARE_UI_BASECLASS_DEF( TClass, TSuperClass, NULL, CreateComponent<TClass>(*Name?Name:GetName()), new TClass() ) \

#define DECLARE_UI_CLASS( TClass, TSuperClass ) \
	DECLARE_UI_BASECLASS_DEF( TClass, TSuperClass, TSuperClass::StaticClass(), CreateComponent<TClass>(*Name?Name:GetName()), new TClass() ) \
	friend FArchiveUI &operator<<( FArchiveUI& Ar, TClass*& Res ) \
		{ return Ar << *(UComponent**)&Res; } \

#define DECLARE_UI_ABSTRACT_CLASS( TClass, TSuperClass ) \
	DECLARE_UI_BASECLASS_DEF( TClass, TSuperClass, TSuperClass::StaticClass(), NULL, NULL ) \

// force initialization of static classes at startup (ensures all classes are in the global class list).
#define IMPLEMENT_UI_CLASS( TClass ) \
	static uiClass* Static##TClass = TClass::StaticClass(); \

#define DEFAULT_PROPERTIES \
public: \
	virtual void InitDefaults() \

#define SUPER_PROPS \
	Super::InitDefaults(); \


//------------------------------------------------------------------------------
// Notes:
// + Component coordinates are relative to the LayoutManager being used.
// + If a function doesn't appear to be working, make sure it got implemented first.
// + This class only specifies the interface for a component, so Containers
//	 don't wind up inheriting all the unneeded extra baggage.
// + A function name followed by a capital C generally indicates a component version of the function.  
//	 Resize( Dimension D ) vs. ResizeC( float Width, float Height )
// + Location:
//	 (0,0) ==> Upper left of component.
//	 +X ==> Right across.
//	 +Y ==> Down.
// + Size should probably be relative to its parent so we can scale a container up
//	 and all of its components would scale up correctly as a unit.	For Unreal2
//	 however, we are going to want all of our assets to be pixel-perfect, so we
//	 will simply ignore this nicety for now and leave things absolute.
// + All components should be added to a container.  It's good to have a single
//	 root container that directly or indirectly contains all components.  That
//	 way you only have to worry about deleting that one container, and all the
//	 others will get correctly cleaned up.
//------------------------------------------------------------------------------
class UI_API UComponent : public Loki::SmallObject
{
public:
	FPoint Location;		// Relative to Parent and Alignment-- see notes above.
	FAlignment Align;		// Left, Right, Top, Bottom, Center.
	FLOAT DrawOrder;		// Larger numbers are drawn on top of smaller numbers.
	uiName Name;			// Name key (use GetName to access in string format).
	class UComponent* Parent;

	TArrayNoInit<class UObject*> Listeners;
	TArrayNoInit<INT> DisabledEvents;
	TArrayNoInit<class UComponent*> Senpai; // those we are dependant upon (via RegisterEvent).

	// MouseClick state trackers.
	BITFIELD bMouseClick_Over:1; GCC_PACK(4);
	BITFIELD bMouseClick_PressedWhileOver:1;
	
	DECLARE_UI_BASECLASS(UComponent,Loki::SmallObject)
	#include "UComponent.h"
	
	// Serialization.
	virtual void Serialize( FArchive &Ar );

	DEFAULT_PROPERTIES
	{
		DrawOrder=1.0f;
	}
};


//------------------------------------------------------------------------------
// Note:
// + If you delete a Container, all of its sub-components will also be deleted,
//	 so you should remove any components you want to keep *before* deleting
//	 the container itself.
//------------------------------------------------------------------------------
class UI_API UContainer : public UComponent
{
public:
	TOrderedArrayNoInit<class UComponent*> Components;	// Sub-components arranged by DrawOrder.
	DECLARE_UI_CLASS(UContainer,UComponent)
	//overrides
	void Serialize( FArchive &Ar );
	void Import( SectionData Section );
	void Destroy();
	//interface
	UBOOL		AddComponent		( UComponent* C, INT Index=-1 );					// Adds the specified component to the end of this Container (or at the specified Index).
	UBOOL		RemoveComponent 	( UComponent* C );									// Removes the specified component.  Returns false if the component wasn't contained to begin with.
	UBOOL		HasComponent		( UComponent* C );									// Recursively searches for the specified Component, and returns true if found.
	INT			GetComponentIndex	( UComponent* C );									// Find the specified component's index in our Components array (or -1 if not found).
	UComponent* GetComponent		( INT Index ){ return Components(Index); }			// Return the contained component at the given index.  Will return None for requests out-of-bounds.
	INT 		NumComponents() 	{ return Components.Num(); }						// Returns the number of contained Components.
	INT 		RemoveComponentType 													// Removes all contained components of the specified class type and (optionally) its subclasses.  Returns the number of actual Components removed.
		( uiClass* ComponentType, UBOOL bAndSubclasses=0, UBOOL bDestroy=0 );			// (You can also optionally have all removed components destroyed as well.)
	//Composite calculations
	FRectangle		GetBoundsI();
	FRectangle		GetScreenBoundsI();
	UComponent* 	GetComponentAtI( const FPoint& P );
	void			SetSizeI( const FDimension& D );
	//Component interface dispatchers
	FDimension		GetSizeI();
	ERenderStyle	GetStyleI();
	void			SetStyleI( ERenderStyle S );
	FColor			GetColorI();
	void			SetColorI( const FColor& C );
	BYTE			GetAlphaI();
	void			SetAlphaI( BYTE A );
	void			SetAlphaPctI( FLOAT Pct );
	void			SetVisibilityI( UBOOL bVisible );
	UBOOL			IsVisibleI();
	UBOOL			IsShowingI();
	UBOOL			IsDisplayableI();
	void			SetEnabledI( UBOOL bEnabled );
	UBOOL			IsEnabledI();
	void			ValidateI();
	void			InvalidateI();
	UBOOL			IsValidI();
	//
	void			Tick( FLOAT DeltaSeconds );
	void			Precache(UPDATE_DEF);
	void			Update(UPDATE_DEF);
	void			Reset();
	void			Shrink();
	void			SetTween( FLOAT Pct );
	void			Tween( UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct );
	//
	UComponent*		GetComponentI( uiName Name );
	UComponent*		GetComponentI( uiPath Path );
	UComponent*		FindLooseComponentI( uiName Name );
	void			GetLooseComponentsI( uiName Name, TArray<UComponent*> &Components );

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		DrawOrder=0.0f;
	}
};

//------------------------------------------------------------------------------
class UI_API UFixedSizeContainer : public UContainer
{
public:
	FDimension Size;
	DECLARE_UI_CLASS(UFixedSizeContainer,UContainer)
	//component overrides
	void			SetSizeI( const FDimension& D );
	FDimension		GetSizeI();
	FRectangle		GetScreenBoundsI();
	FRectangle		GetBoundsI();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Size;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};

//------------------------------------------------------------------------------
class UI_API UScaleContainer : public UContainer
{
public:
	FDimension RelativeSize;
	DECLARE_UI_CLASS(UScaleContainer,UContainer)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << RelativeSize;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};



enum ImageComponentIndex
{
	ICI_Top 				=0,
	ICI_Bottom				=1,
	ICI_Left				=2,
	ICI_Right				=3,
	ICI_NE					=4,
	ICI_NW					=5,
	ICI_SE					=6,
	ICI_SW					=7,
	ICI_Background			=8,
	ICI_MAX 				=9,
};

//------------------------------------------------------------------------------
class UI_API UWindow : public UFixedSizeContainer
{
public:
	DECLARE_UI_CLASS(UWindow,UFixedSizeContainer)
	//overrides
	void Import( SectionData Section );
	//interface
	virtual void SetImages
	(
		FImage* Top,
		FImage* Bottom,
		FImage* Left,
		FImage* Right,
		FImage* NE,
		FImage* NW,
		FImage* SE,
		FImage* SW,
		FImage* Background
	);

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		DrawOrder=1.0f;
	}
};


//------------------------------------------------------------------------------
class UI_API UTextArea : public UFixedSizeContainer
{
public:
	//TextComponentBase
	BITFIELD bForceWrap:1 GCC_PACK(4);
	BITFIELD bForceUnWrap:1;
	BITFIELD bTopDown:1;	// draw lines from top to bottom.
	BITFIELD bFIFO:1;		// start drawing from the oldest text to the newest text.
	INT MaxLines;			// maximum number of lines of text. (0=infinite)
	class UFont* Font GCC_PACK(4);
	INT StartLineIndex; 	// what line of text to start drawing at (zero-based).
	INT SN; 				// number of sentinel objects in Components list.
	DECLARE_UI_CLASS(UTextArea,UFixedSizeContainer)
	//overrides
	void Constructed();
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	//interface
	virtual void AddText( FString NewText, UBOOL bWrap=false );
	virtual void Empty();
	//TextComponentBase interface
	virtual void	SetFont( UFont* NewFont );
	virtual UFont*	GetFont( UCanvas* C=NULL );
protected:
	virtual class ULabel* CreateLabel();	// override in subclasses if you want to use a different type of label.
	void GetIndexBounds( INT& Start, INT& End )
	{
		End = Components.Num()-1;
		Start = MaxLines ? Max( SN, (End - MaxLines)+1 ) : SN;
	}

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		SERIALIZE_BOOL(bForceWrap)
		SERIALIZE_BOOL(bForceUnWrap)
		SERIALIZE_BOOL(bTopDown)
		SERIALIZE_BOOL(bFIFO)
		Ar << Font << StartLineIndex << SN;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		bTopDown=true;
		bFIFO=true;
		StartLineIndex=0;
		DrawOrder=1.0f;
		SN=1;
	}
};



//------------------------------------------------------------------------------
class UI_API UTimedTextArea : public UTextArea
{
public:
	FLOAT TextLifespan;
	FLOAT FadeinPct;
	FLOAT FadeoutPct;
	FLOAT MinInterval;	// forced time difference between each line of text's Lifespan.
						// (if a whole bunch of text comes in a once, but you want each
						// line to disappear one at a time, then set this variable to the
						// amount of time between lines disappearing.)

	DECLARE_UI_CLASS(UTimedTextArea,UTextArea)
	//overrides
	ULabel* CreateLabel();
	void Import( SectionData Section );
	//interface
	void AddText( FString NewText, UBOOL bWrap );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << MaxLines << TextLifespan << FadeinPct << FadeoutPct << MinInterval;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UScrollTextArea : public UTextArea
{
public:
	class UScrollBar* ScrollBar;
	DECLARE_UI_CLASS(UScrollTextArea,UTextArea)
	//overrides
	void AddText( FString NewText, UBOOL bWrap );
	void Update(UPDATE_DEF);
	void SetSizeI( const FDimension& D );
	void Import( SectionData Section );
	void Serialize( FArchive &Ar );
	//interface
	virtual void Init();
	virtual void SetMouse( UMouseCursor* M );	// !!must call Init first!!
private:
	//ScrollBar hooks
	void UI_CALLBACK OnGetValue(void*);
	void UI_CALLBACK OnGetRange(void*);
	void UI_CALLBACK OnGetVisibleRange(void*);
	void UI_CALLBACK OnSetValue(void*);

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		SN=2;
	}
};


//------------------------------------------------------------------------------
// Description: A frame with scrollbars and a resize handle.
//------------------------------------------------------------------------------
class UI_API UScrollFrame : public UFixedSizeContainer
{
public:
	class UFrame* Frame;
	class UScrollBar* SBVertical;
	class UScrollBar* SBHorizontal;
	class UResizeHandleSE* SizeHandle;
	DECLARE_UI_CLASS(UScrollFrame,UFixedSizeContainer)
	//overrides
	void Serialize( FArchive &Ar );
	void Import( SectionData Section );
	void Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct );
	//component overrides
	void SetSizeI( const FDimension& D );
	//interface
	virtual void Init();
	virtual void SetMouse( UMouseCursor* M );	// !!must call Init first!!
	virtual void AddItem( UComponent* C );
	//frame redirects
	void		SetFrameLocation( const FPoint& P );
	FPoint		GetFrameLocation();
	void		SetFrameSize( const FDimension& D );
	FDimension	GetFrameSize();
private:
	//ScrollBar hooks
	void UI_CALLBACK OnGetValueV(void*);
	void UI_CALLBACK OnGetRangeV(void*);
	void UI_CALLBACK OnGetVisibleRangeV(void*);
	void UI_CALLBACK OnSetValueV(void*);
	//
	void UI_CALLBACK OnGetValueH(void*);
	void UI_CALLBACK OnGetRangeH(void*);
	void UI_CALLBACK OnGetVisibleRangeH(void*);
	void UI_CALLBACK OnSetValueH(void*);

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};

#define UCONST_MIN_SLIDER_SIZE 6.0

//------------------------------------------------------------------------------
class UI_API UScrollBar : public UFixedSizeContainer
{
public:
	BITFIELD bVertical:1 GCC_PACK(4);	// set to false for horizontal scrollbars.
	
	FLOAT Value GCC_PACK(4);			// current value of the scrollbar - top of the visible range (always scaled between 0 and 1).
										// use GetValue and SetValue for access to the actual value - which will usually represent the top visible line of text in a text area.

	FLOAT _Min; 						// min value for this scrollbar (usually zero).
	FLOAT _Max; 						// max value for this scrollbar (usually set to the total number of lines of text in a text area).
	FLOAT _Range;						// how much is currently visible (usually set to the number of visible lines of text in a text area).

	FLOAT ScrollAmt;					// amount to scroll by when Up/Down key is hit (or the user clicks on the up or down button).
	FLOAT PageAmt;						// amount to scroll by when PageUp/PageDown is hit (or the user clicks on the background area).

	FLOAT RepeatDelay;					// number of seconds a button must be held before starting to repeat.
	FLOAT RepeatRate;					// seconds between repeats.

	// Images.
	FImage* UpButtonUp;
	FImage* UpButtonDown;
	FImage* DownButtonUp;
	FImage* DownButtonDown;
	FImage* BackgroundImage;
	FImage* SliderImage;

	// Fix ARL: Replace Slider image with scalable "window" type component.

	// Internals.
	class UHoldButton* UpButton;
	class UHoldButton* DownButton;
	class UImageComponent* Background;
	class UImageComponent* Slider;

	class UMouseCursor* SliderMouse;	// only valid while pressed.
	class UMouseCursor* PageUpMouse;	// only valid while pressed.
	class UMouseCursor* PageDownMouse;	// only valid while pressed.

	FLOAT LastMouseY;
	FLOAT NextRepeatTime;

	DECLARE_UI_CLASS(UScrollBar,UFixedSizeContainer)

	//interface
	virtual FLOAT	GetValue();
	virtual void	GetRange( FLOAT &Min, FLOAT &Max );
	virtual FLOAT	GetVisibleRange();
	//
	virtual void	SetValue( FLOAT NewValue );
	virtual void	SetRange( FLOAT NewMin, FLOAT NewMax );
	virtual void	SetVisibleRange( FLOAT NewRange );
	//
	virtual void	ScrollUp();
	virtual void	ScrollDown();
	//
	virtual void	PageUp();
	virtual void	PageDown();
	//
	virtual void	Init();
	virtual void	SetMouse( UMouseCursor* M );	// !!must call Init first!!
	//
private:
	void UI_CALLBACK OnUpClick(void*){ ScrollUp(); }
	void UI_CALLBACK OnDownClick(void*){ ScrollDown(); }
public:
	//
	INT GetIntValue(){ return GetValue(); }
	//mouse support
	void UI_CALLBACK MousePress( void* Parms);
	void UI_CALLBACK MouseRelease( void* Parms );
	//overrides
	void SetSizeI( const FDimension& D );
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
protected:
	//Horizontal/Vertical switch functions
	virtual void	SetPos( UComponent* C, FLOAT Pos );
	virtual FLOAT	GetPos( UComponent* C );
	virtual void	SetDim( UComponent* C, FLOAT Dim );
	virtual FLOAT	GetDim( UComponent* C );
	virtual void	SetOtherDim( UComponent* C, FLOAT Dim );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		SERIALIZE_BOOL(bVertical)
		Ar << Value << _Min << _Max << _Range << ScrollAmt << PageAmt << RepeatDelay << RepeatRate;
//!!ARL		Ar << UpButtonUp << UpButtonDown << DownButtonUp << DownButtonDown << BackgroundImage << SliderImage;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		bVertical=true;
		Size=FDimension(16,16);
		Value=0.0f;
		_Min=0.0f;
		_Max=100.0f;
		_Range=20.0f;
		ScrollAmt=1.0f;
		PageAmt=32.0f;
		RepeatDelay=0.5f;
		RepeatRate=0.05f;
	}
};


//------------------------------------------------------------------------------
class UI_API UImageWipe : public UFixedSizeContainer
{
public:
	FLOAT Pct;	// amount of top image to display.
	BITFIELD bVertical:1 GCC_PACK(4);
	BITFIELD bInvert:1;
	class UImageComponent* TopImage GCC_PACK(4);	// homegrown version
	class UImageComponent* BottomImage; 			// homegrown version
	class UImageComponent* TopImageComponent;		// premade version
	class UImageComponent* BottomImageComponent;	// premade version
	DECLARE_UI_CLASS(UImageWipe,UFixedSizeContainer)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void Serialize( FArchive &Ar );
	void Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct );
	//interface
	void SetTopImage( FImage* _Image );
	void SetBottomImage( FImage* _Image );
	void SetTopImageComponent( UImageComponent* C );
	void SetBottomImageComponent( UImageComponent* C );

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		DrawOrder=1.0f;
	}
};


//------------------------------------------------------------------------------
// Description: Container that vertically or horizontally tiles its contents.
//------------------------------------------------------------------------------
class UI_API UTileContainer : public UContainer
{
public:
	FLOAT FixedSpacing;					// override calling GetSize on contained components (useful for optimizing)
	BITFIELD bVertical:1 GCC_PACK(4);	// tile vertically or horizontally.
	BITFIELD bReverseJustify:1;
	BITFIELD bReverseOrder:1;
	BITFIELD bShortCircuit;
	DECLARE_UI_CLASS(UTileContainer,UContainer)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	//component overrides
	FRectangle GetBoundsI();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << FixedSpacing;
		SERIALIZE_BOOL(bVertical)
		SERIALIZE_BOOL(bReverseJustify)
		SERIALIZE_BOOL(bReverseOrder)
		SERIALIZE_BOOL(bShortCircuit);
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		FixedSpacing=-1.0f;
		bVertical=true;
	}
};


//------------------------------------------------------------------------------
// Description: This component can have many representations, but can only
// represent one component at a time.  Transitions can be defined to get
// from one state to the next.
//------------------------------------------------------------------------------
// Notes:
// + The behavior of this object is obtained by storing a different component
// object for each state (and even for transitions).  Only one component is
// "Active" at any given time.	Any Component interface function calls are
// propogated down to the currently active component.
//------------------------------------------------------------------------------
class UI_API UMultiStateComponent : public UContainer
{
public:
	TArrayNoInit<class UComponent*> States; // Fix ARL: Do these references need to be cleared on delete?
	INT CurrentState;
	TArrayNoInit<FSTRUCT_Transition> Transitions;

	INT NextTransitionEvent;
	INT PrevTransitionEvent;
	INT NextStateEvent;
	INT PrevStateEvent;

	INT TransitionIndex;
	FLOAT TransitionTime;
	FLOAT LastPct;
	INT TransitionTag;	// transient

	// Tween flags.
	// Fix ARL: Make these per-transition vars?
	BITFIELD bTweenLocation:1 GCC_PACK(4);
	BITFIELD bTweenSize:1;
	BITFIELD bTweenColor:1;
	BITFIELD bTweenAlpha:1;
	BITFIELD bTweenLinear:1;
	BITFIELD bTweenReverseEase:1;
	BITFIELD bTweenReverseEaseOverride:1;

	BITFIELD bTransitioning:1;	// for native C++ state faking.

	DECLARE_UI_CLASS(UMultiStateComponent,UContainer)
	#include "UMultiStateComponent.h"

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		bTweenLocation=true;
		bTweenSize=false;
		bTweenColor=false;
		bTweenAlpha=false;
	}
};

//------------------------------------------------------------------------------

class UI_API UPlayerID : public UFixedSizeContainer
{
public:
	APawn* CurrentPawn;					// transient
	DECLARE_UI_CLASS(UPlayerID,UContainer)
	UBOOL IsEnabledI(){ return true; }	// override container nature.
	//overrides
	void Update(UPDATE_DEF);

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UButton : public UContainer
{
public:
	class UButtonImage* Image;
	class ULabel* Text;
	FLOAT Border; // how much bigger the button image is than the text (by default).
	DECLARE_UI_CLASS(UButton,UContainer)
	//overrides
	void Serialize( FArchive &Ar );
	void Import( SectionData Section );
	void Constructed();
	//interface
	virtual void	SetImages( FImage* Up, FImage* Down );
	virtual void	Init();
	virtual void	SetFontSize( EFontSize NewSize );
	virtual void	SetFont( UFont* NewFont );
	virtual UFont*	GetFont( UCanvas* C=NULL );
	virtual void	SetText( FString NewText, UBOOL bNoUpdateSize=false );
	virtual FString GetText();
	//component overrides
	void SetSizeI( const FDimension& D );
	//mouse support
	virtual void SetMouse( UMouseCursor* M );
	protected: void UI_CALLBACK Click(void*); public:

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Border=4.0f;
	}
};


//------------------------------------------------------------------------------
// Description: Like a button, but does it's action immediately when pressed,
// instead of waiting for a mouse up event (i.e. clicked).	Also has the ability
// to repeatedly send its event if held for a longer period of time.
//------------------------------------------------------------------------------
class UI_API UHoldButton : public UButton
{
public:
	FLOAT RepeatDelay;	// number of seconds a button must be held before starting to repeat.
	FLOAT RepeatRate;	// seconds between repeats.
	FLOAT NextRepeatTime;
	class UMouseCursor* Mouse;
	DECLARE_UI_CLASS(UHoldButton,UButton)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void SetMouse( UMouseCursor* M );
	//mouse support
	void UI_CALLBACK MousePress( void* Parms);
	void UI_CALLBACK MouseRelease( void* Parms );

	void Serialize( FArchive &Ar );

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
// Description: A Frame is a Container of Components, but its Bounds describe
// the drawing area rather than actually resizing the components within.
//------------------------------------------------------------------------------
class UI_API UFrame : public UContainer
{
public:
	FPoint FrameLocation;
	FDimension FrameSize;
	DECLARE_UI_CLASS(UFrame,UContainer)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct );
	//component overrides
	FRectangle GetBoundsI();
	FRectangle GetScreenBoundsI();
	FDimension GetSizeI();
	//interface
	virtual void		SetFrameLocation( const FPoint& P );
	virtual FPoint		GetFrameLocation();
	virtual void		SetFrameSize( const FDimension& D );
	virtual FDimension	GetFrameSize();
	//helpers
	void	SetFrameWidthPct( FLOAT Pct );
	FLOAT	GetFrameWidthPct();
	void	SetFrameHeightPct( FLOAT Pct );
	FLOAT	GetFrameHeightPct();
	void	SetFrameXPct( FLOAT Pct );
	FLOAT	GetFrameXPct();
	void	SetFrameYPct( FLOAT Pct );
	FLOAT	GetFrameYPct();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << FrameLocation << FrameSize;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
// Notes:
// + All variables are protected to enforce access via Component interface 
//	 functions.  This way containers can propogate those requests to their
//	 sub-components.
//------------------------------------------------------------------------------
class UI_API URealComponent : public UComponent
{
public:
	// Fix ARL: Should these be private?
	FDimension Size;
	BYTE Style;
//	BYTE AlignmentBuffer[3];
	FColor DrawColor;
	BITFIELD bVisible:1 GCC_PACK(4);
	BITFIELD bEnabled:1;
	BITFIELD bValid:1;
	DECLARE_UI_CLASS(URealComponent,UComponent)
	//overrides
	FDimension		GetSizeI();
	void			SetSizeI( const FDimension& D );
	ERenderStyle	GetStyleI();
	void			SetStyleI( ERenderStyle S );
	FColor			GetColorI();
	void			SetColorI( const FColor& C );
	BYTE			GetAlphaI();
	void			SetAlphaI( BYTE A );
	void			SetVisibilityI( UBOOL bVisible );
	UBOOL			IsVisibleI();
	UBOOL			IsShowingI();
	UBOOL			IsDisplayableI();
	void			SetEnabledI( UBOOL bEnabled );
	UBOOL			IsEnabledI();
	void			ValidateI();
	void			InvalidateI();
	UBOOL			IsValidI();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Size << Style << DrawColor;
		SERIALIZE_BOOL(bVisible)
		SERIALIZE_BOOL(bEnabled)
		SERIALIZE_BOOL(bValid)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Style=STY_Alpha;
		DrawColor=FColor(255,255,255,255);
		bEnabled=true;
	}
};



//------------------------------------------------------------------------------
class UI_API UTextComponentBase : public URealComponent
{
public:
	BYTE FontSize;
	class UFont* Font;	// If None then font is dynamically loaded (and thus scaled) based on FontSize and current screen resolution.
	DECLARE_UI_CLASS(UTextComponentBase,URealComponent)
	//overrides
	void Constructed();
	void Import( SectionData Section );
	//interface
	virtual void	SetFontSize( EFontSize NewSize );
	virtual void	SetFont( UFont* NewFont );
	virtual UFont*	GetFont( UCanvas* C=NULL );
	virtual FLOAT	GetFontHeight( UFont* F );
	virtual void	AdjustSizeFromText( const FString &S );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << FontSize << Font;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		FontSize=FS_Small;
	}
};


//------------------------------------------------------------------------------
class UI_API UVarWatcher : public UTextComponentBase
{
public:
	class UObject* Obj;
	class UProperty* Prop;
	INT Index;
	BITFIELD bFullDesc:1 GCC_PACK(4);
	DECLARE_UI_CLASS(UVarWatcher,UTextComponentBase)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	//interface
	virtual void SetWatch( UObject* WatchObj, FName PropName, INT ArrayIndex=0 );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Obj << Prop << Index;
		SERIALIZE_BOOL(bFullDesc)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
// Notes:
// + You need to either add this component to a container that has a console, 
// or use new(GetConsole()) class'Label'; before calling SetText().
//------------------------------------------------------------------------------
class UI_API ULabel : public UTextComponentBase
{
public:
	FStringNoInit Text;
	FStringNoInit Format;
	BITFIELD bWrap:1 GCC_PACK(4);	// set to true if you want text to wrap within the given size of this label.
	DECLARE_UI_CLASS(ULabel,UTextComponentBase)
	//overrides
	void SetText( const FString& NewText, UBOOL bNoUpdateSize=false );
	FString GetText();
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	FDimension GetSizeI();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Text << Format;
		SERIALIZE_BOOL(bWrap)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		DrawOrder=1.0f;
		Format=TEXT("%3.1f");
	}
};


//------------------------------------------------------------------------------
// Description: A label with drop shadow.
//------------------------------------------------------------------------------
class UI_API ULabelShadow : public ULabel
{
public:
	FColor ShadowColor;
	FDimension ShadowOffset;
	DECLARE_UI_CLASS(ULabelShadow,ULabel)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void SetAlphaI( BYTE A );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << ShadowColor << ShadowOffset;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		ShadowColor=FColor(30,30,30,100);
		ShadowOffset=FDimension(1,1);
	}
};


//------------------------------------------------------------------------------
// Description: A label with a Lifespan.
//------------------------------------------------------------------------------
class UI_API UTimedLabel : public ULabel
{
public:
	FLOAT Lifespan;
	FLOAT InitialLifespan;
	FLOAT FadeinPct;
	FLOAT FadeoutPct;
	DECLARE_UI_CLASS(UTimedLabel,ULabel)
	//interface
	virtual void SetLifespan( FLOAT Seconds );
	//overrides
	void Reset();
	void Update(UPDATE_DEF);
	void Import( SectionData Section );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Lifespan << InitialLifespan << FadeinPct << FadeoutPct;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API USelectorList : public UMultiStateComponent
{
public:
	INT CurrentItem;
	class UTextComponentBase* TextInfo; // for faking multiple inheritance.
	class UFrame* OpenState;
	class UFrame* ClosedState;
	class UFrame* TransitionState;
	class UMouseCursor* Mouse;
	FColor Highlight;
	FImage* Image;
	FColor UnHighlight;
	BITFIELD bOpen:1 GCC_PACK(4);
	DECLARE_UI_CLASS(USelectorList,UMultiStateComponent)
	//overrides
	void Constructed();
	void Serialize( FArchive &Ar );
	//interface
	virtual void	AddItem( FString Text, ActionEvent* Action=NULL, ActionEvent* Action2=NULL );	// NOTE[aleiby]: This is the only method that should be used to add things to the Frames - otherwise they might get out of sync and wreak havoc.
	virtual void	RemoveItem( FString Text ); 													// NOTE[aleiby]: If you add something multiple times, you must remove it multiple times.
	virtual FString GetItem( INT i );
	virtual INT		GetItemIndex( class USelectorItem* Item );
	virtual INT 	NumItems();
	virtual void	Empty();
	virtual void	Select( INT i );
	virtual void	SelectItem( FString Text );
	virtual UBOOL	HasItem( FString Item );
	virtual FString GetCurrentItem();
	virtual INT		GetCurrentItemIndex();
	virtual void	SetMouse( UMouseCursor* M );
	virtual void	SetHighlight( const FColor& C );
	virtual void	SetUnHighlight( const FColor& C );
	virtual void	Expand();
	virtual void	Close();
	//TextComponentBase interface
	virtual void	SetFontSize( EFontSize NewSize );
	virtual void	SetFont( UFont* NewFont );
	virtual UFont*	GetFont( UCanvas* C=NULL );
	//component interface redirects
	void			SetSizeI( const FDimension& D ) { UContainer::SetSizeI(D);			}
	FDimension		GetSizeI()						{ return UContainer::GetSizeI();	}
	ERenderStyle	GetStyleI() 					{ return UContainer::GetStyleI();	}
	void			SetStyleI( ERenderStyle S ) 	{ UContainer::SetStyleI(S); 		}
	FColor			GetColorI() 					{ return UContainer::GetColorI();	}
	void			SetColorI( const FColor& C )	{ UContainer::SetColorI(C); UnHighlight=C;	}
protected:
	//helpers
	void AddSelectorItem( FString Text, UContainer* C, FLOAT Y=0.0f, ActionEvent* Action=NULL, ActionEvent* Action2=NULL );
	void AddLabel( FString Text, UContainer* C, FLOAT Y=0.0f );

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Highlight=FColor(0,0,255,255);
		UnHighlight=FColor(255,255,255,255);
	}
};


//------------------------------------------------------------------------------
class UI_API USelectorItem : public ULabel
{
public:
	FColor Highlight;
	FColor UnHighlight;
	DECLARE_UI_CLASS(USelectorItem,ULabel)
	//mouse support
	virtual void SetMouse( UMouseCursor* M );
	void UI_CALLBACK MouseEnter  ( void* Parms );
	void UI_CALLBACK MouseExit	 ( void* Parms );
	void UI_CALLBACK MouseRelease( void* Parms );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Highlight << UnHighlight;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API USelector : public UContainer
{
public:
	FImage* TopFrame;
	FImage* BottomFrame;
	class USelectorList* List;
	class UImageComponent* TopFrameImage;
	class UImageComponent* BottomFrameImage;
	class UComponent* Group;	// used for grouping multiple selectors (this must be a common parent).
	BITFIELD bHideBorders:1 GCC_PACK(4);
	DECLARE_UI_CLASS(USelector,UContainer)
	//SelectorList interface
	virtual void	AddItem( FString Item, ActionEvent* Action=NULL );
	virtual void	RemoveItem( FString Item );
	virtual FString GetItem( INT i );
	virtual INT 	NumItems();
	virtual void	Empty();
	virtual void	Select( INT i );
	virtual void	SelectItem( FString Item );
	virtual UBOOL	HasItem( FString Item );
	virtual FString GetCurrentItem();
	virtual INT		GetCurrentItemIndex();
	virtual void	SetHighlight( const FColor& C );
	virtual void	SetUnHighlight( const FColor& C );
	//ImageComponent interface
	virtual void Init();
	virtual void SetBackground( FImage* _Image );
	//TextComponentBase interface
	virtual void	SetFontSize( EFontSize NewSize );
	virtual void	SetFont( UFont* NewFont );
	virtual UFont*	GetFont( UCanvas* C=NULL );
	//interface
	virtual void	Refresh();
	virtual void	SetGroup( UContainer* C );
	virtual void	PreSelect();
	virtual void	PostSelect();
	//mouse support
	virtual void SetMouse( UMouseCursor* M );
	void UI_CALLBACK MousePress  ( void* Parms );
	void UI_CALLBACK MouseRelease( void* Parms );
	void UI_CALLBACK MouseEnter  ( void* Parms );
	//overrides
	void Constructed();
	void Serialize( FArchive &Ar );
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void HandleEvent( uiEvent Event );
	UContainer* GetGroup(){ return Group ? (UContainer*)Group : Super::GetGroup(); }
	//component overrides
	FRectangle GetBoundsI();
	FRectangle GetScreenBoundsI();

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		DrawOrder=1.0f;
	}
};


//------------------------------------------------------------------------------
// Description: Special class for choosing keyboard or mouse inputs.
//------------------------------------------------------------------------------
class UI_API UInputSelector : public ULabel
{
public:
	FColor SelectedColor;
	BITFIELD bListening:1 GCC_PACK(4);
	DECLARE_UI_CLASS(UInputSelector,ULabel)
	#include "UInputSelector.h"

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << SelectedColor;
		SERIALIZE_BOOL(bListening)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		SelectedColor=FColor(200,80,80,255);
	}
};


//------------------------------------------------------------------------------
class UI_API UTextField : public UTextComponentBase
{
public:
	FStringNoInit TypedStr;
	FStringNoInit LastStr;

	FStringNoInit Prefix;	// to TypedStr
	FStringNoInit Postfix;	// to TypedStr

	FColor TextColor;		// Color of the persistent text.
	
	// Fix ARL: This is a bad precident to set.  This should be added to a container instead.
	class UComponent* Cursor GCC_PACK(4);	// Set to the component you want drawn as the cursor.
	INT CursorPos;
	INT CursorOffset;
	
	INT InputRangeMax;
	INT InputRangeMin;
	TArrayNoInit<INT> IgnoredKeys;

	BITFIELD bPersistent:1 GCC_PACK(4); 	// Keep displaying text after user presses Enter?
	BITFIELD bAutoSelect:1;					// Automatically select when enabled?
	BITFIELD bDeselectOnEnter:1;
	BITFIELD bAutoSize:1;					// Set to true if you wish this component to automatically resize itself according to the size of the text.
	BITFIELD bGreedyFocus:1;				// Don't Deselect if click on something else.

	BITFIELD bHasFocus:1;
	BITFIELD bDirtyHist:1;					// used internally to monitor new items added to the history list.
	BITFIELD bWaitForDraw:1;				// used internally to protect us from grabbing the keystroke that enabled us.

	DECLARE_UI_CLASS(UTextField,UTextComponentBase)

	//native variables.
	typedef TDoubleLinkList<FString> HistoryNode;
	HistoryNode* History;
	//overrides
	void Constructed();
	void Destroy();
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void SetEnabledI( UBOOL bEnabled );
	UComponent* GetComponentI( uiPath Path );
	//interface
	virtual void SetConsole( UUIConsole* C );
	virtual void SetInputRange( INT IMin, INT IMax );
	void UI_CALLBACK BeginType(void*);
	virtual void Deselect();
	virtual void SetMouse( UMouseCursor* M );
	virtual void SetCursor( UComponent* C );
	virtual void SetCursorPosition( INT i );
	virtual INT  GetCursorPosition();
	virtual INT  GetSelectionLeft();
	virtual INT  GetSelectionRight();
	virtual INT  GetSelectionSize();
	//mouse events
	void UI_CALLBACK MousePress( void* Parms );
	//key events
	void UI_CALLBACK KeyType( void* Parms );
	void UI_CALLBACK KeyEvent( void* Parms );
	//helpers
	UBOOL IsKeyIgnored( INT AsciiVal );
	void AddIgnoredKey( INT AsciiVal );
	void RemoveIgnoredKey( INT AsciiVal );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << TypedStr << LastStr << Prefix << Postfix << TextColor;
		(FArchiveUI&)Ar << Cursor;	// this is okay since it's not referenced anywhere else.
		SERIALIZE_BOOL(bPersistent)
		SERIALIZE_BOOL(bDeselectOnEnter)
		SERIALIZE_BOOL(bAutoSize)
		SERIALIZE_BOOL(bGreedyFocus)
		Ar << CursorPos << CursorOffset << InputRangeMax << InputRangeMin << IgnoredKeys;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		InputRangeMin=32;
		InputRangeMax=256;
		bHasFocus=false;
		bPersistent=false;
		bAutoSelect=false;
		TextColor=FColor(255,128,128,255);
		bDirtyHist=true;
		bDeselectOnEnter=true;
	}
};


//------------------------------------------------------------------------------
class UI_API UNumWatcher : public UTextComponentBase
{
public:
	BITFIELD bFloat:1 GCC_PACK(4);
	DECLARE_UI_CLASS(UNumWatcher,UTextComponentBase)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		SERIALIZE_BOOL(bFloat)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
// How to use this class:
//	 SC = new class'SoundComponent';
//	 SC.Sound = "UISounds.Explosion1";
//	 SC.Init();
//	 C.RegisterEvent( 'Blah', SC );
//	 C.SendEvent( 'Blah' );
//------------------------------------------------------------------------------
class UI_API USoundComponent : public URealComponent
{
public:
	FStringNoInit Sound;
	FLOAT Volume;
	class USound* MySound;
	BITFIELD bInterrupt:1 GCC_PACK(4);
	DECLARE_UI_CLASS(USoundComponent,URealComponent)
	//overrides
	void Import( SectionData Section );
	void HandleEvent( uiEvent Event );
	//interface
	virtual void Init();
	virtual void LoadSound();
	virtual void PlaySound();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Sound << Volume << MySound;
		SERIALIZE_BOOL(bInterrupt)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Volume=1.0f;
	}
};


//------------------------------------------------------------------------------
class UI_API UImage : public UComponent
{
public:
	UMaterial* Material;
	TArrayNoInit<FImage*> Images;
	DECLARE_UI_CLASS(UImage,UComponent)
	//interface
	virtual FImage* AddImage(FString Name,FLOAT X,FLOAT Y,FLOAT W,FLOAT H,UBOOL bTile=0);
	virtual FImage* AddFullImage(FString Name,UBOOL bTile=0);
	virtual FImage* FindImage(FString Name);
	//overrides
	void Import( SectionData Section );
	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Material << Images;
	}
};


//------------------------------------------------------------------------------
class UI_API UImageComponentBase : public URealComponent
{
public:
	BITFIELD bTile:1 GCC_PACK(4);
	DECLARE_UI_ABSTRACT_CLASS(UImageComponentBase,URealComponent)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	//interface
	virtual void		Init();
	virtual FImage*		GetImage()=0;

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		SERIALIZE_BOOL(bTile)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UImageComponent : public UImageComponentBase
{
public:
	FImage* Image;
	DECLARE_UI_CLASS(UImageComponent,UImageComponentBase)
	void SetImage( FImage* _Image );
	FImage* GetImage();
	void Import( SectionData Section );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
//!!ARL		Ar << Image;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UTimedImage : public UImageComponent
{
public:
	FLOAT Lifespan;
	FLOAT InitialLifespan;
	FLOAT FadeinPct;
	FLOAT FadeoutPct;
	DECLARE_UI_CLASS(UTimedImage,UImageComponent)
	//interface
	virtual void SetLifespan( FLOAT Seconds );
	//overrides
	void Reset();
	void Update(UPDATE_DEF);
	void Import( SectionData Section );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Lifespan << InitialLifespan << FadeinPct << FadeoutPct;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
// Notes:
// + Value is always scaled between 0.0 and 1.0 and represents the percenage
//	 between the Min and Max values.
// + Min and Max default to 0 and 255 respectively to mimic a byte range.
// + SetValue takes a new value between Min and Max.
// + GetValue returns the current value scaled between Min and Max.
// + Use SetRange to change Min and Max.
//------------------------------------------------------------------------------
class UI_API USlider : public UImageComponent
{
public:
	FImage* Handle;
	FStringNoInit Format;
	FLOAT SideBuffer;	// extra unused distance on each end of the slider.
	FLOAT Step; 		// set this to >0 to give the slider stepped (digital) motion.
	FLOAT Value;		// percent between Min and Max.
	FLOAT Min;
	FLOAT Max;
	class UMouseCursor* Mouse;	// only valid while pressed.
	DECLARE_UI_CLASS(USlider,UImageComponent)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	//mouse support
	void UI_CALLBACK MousePress( void* Parms);
	void UI_CALLBACK MouseRelease( void* Parms );
	//interface
	void SetImage( FImage* _Image, FImage* _Handle );
	virtual void	SetValue( FLOAT NewValue );
	virtual FLOAT	GetValue();
	virtual INT 	GetIntValue();
	virtual FString	GetStringValue();
	virtual void	SetRange( FLOAT NewMin, FLOAT NewMax );
	virtual void	SetMouse( UMouseCursor* M );
	virtual void	SetTargetValue();
	virtual void	GetTargetValue();

	void Serialize( FArchive &Ar );

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Min=0.0f;
		Max=255.0f;
		SideBuffer=8.0f;
		Format=TEXT("%3.1f");
	}
};


enum EMouseAction
{
	MA_Enter				=0,
	MA_Exit 				=1,
	MA_Press				=2,
	MA_Release				=3,
	MA_MAX					=4,
};

//------------------------------------------------------------------------------
// Notes:
// + I'm tempted to move all the notification functionality up into UIConsole.
//	 It would be one less level of indirection.
//	 Pros:
//	   1) Easier for components to register.  You just need to get the console
//		  rather than search for the mouse.
//	   2) Faster since all the code would be right in UIConsole rather than 
//		  getting redirected through a mouse component.
//	   3) More homogeneous.  If all notification/registration code is in one
//		  place (UIConsole) then it becomes easier to maintain and comprehend.
//	 Cons:
//	   1) Restricts number of mouse cursors.  There may arise situations where
//		  two or more different mouse cursors are desired (like different mouse
//		  modes or something), and in such a situation, you would want to 
//		  register different components and events with different mouse cursors.
//	   2) It's currently implemented this way, and it works.  If it ain't broke,
//		  don't fix it.
// + If we do decide to move all notification code into UIConsole, it would
//	 probably behoove us to make MouseCursor simply an image that follows
//	 the mouse around, and then break out all the particle system stuff into
//	 a subclass or something that makes a particle system follow the mouse
//	 around rather than a simple image.
//------------------------------------------------------------------------------
class UI_API UMouseCursor : public UContainer
{
public:
//	FPoint MousePos;	// On-screen mouse position.  Use GetScreenCoords() for access.

	INT MouseIndex;		// Our index into GMouseList.

	BITFIELD bAutoConstrain:1 GCC_PACK(4);	// Set this to true if you want the mouse constraints to match the viewport.
	FLOAT MinX GCC_PACK(4); // Horizontal constriants.
	FLOAT MaxX; 			// Horizontal constriants.
	FLOAT MinY; 			// Vertical constraints.
	FLOAT MaxY; 			// Vertical constraints.

	TArrayNoInit<class UComponent*> NewComponents;	// Listening components that have just become relevant this tick.
	TArrayNoInit<class UComponent*> OldComponents;	// Listening components that have just become un-relevant this tick.
	FLOAT LastRelevancyUpdateTime;					// The last Level.TimeSeconds that the above two lists were updated.
	FLOAT TimeStamp;								// Updated every tick (cumulation of DeltaSeconds).

	FLOAT MoveTime, MoveTimer;	// Set MoveTime if you want MouseOn/MouseOff events -- separated by MoveTime seconds.

	DECLARE_UI_CLASS(UMouseCursor,UContainer)
	#include "UMouseCursor.h"

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		MaxX=640;
		MaxY=480;
		bAutoConstrain=true;
		DrawOrder=9999;
	}
};


//------------------------------------------------------------------------------
class UI_API UImageWedge : public UImageComponent
{
public:
	FLOAT Radius;
	FLOAT Angle;
	DECLARE_UI_CLASS(UImageWedge,UImageComponent)
	//overrides
	void Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct );
	void Import( SectionData Section );
	void Update(UPDATE_DEF);
	//interface
	FLOAT GetRadius();
	FLOAT GetAngle();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Radius << Angle;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UCheckBox : public UImageComponent
{
public:
	FImage* Check;
	BITFIELD bChecked:1 GCC_PACK(4);
	DECLARE_UI_CLASS(UCheckBox,UImageComponent)
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	//interface
	void SetImage( FImage* _Image, FImage* _Check );
	virtual void	SetChecked( UBOOL B );
	virtual UBOOL	GetChecked();
	virtual void	SetMouse( UMouseCursor* M );
	
	void UI_CALLBACK Toggle(void*);
	
	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
//!!ARL		Ar << Check;
		SERIALIZE_BOOL(bChecked)
	}

	// Fix ARL: Get rid of all default properties, and use template components instead -- defined in .ui file format.
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Size=FDimension(13,13);
		bTile=true;
	}
};



//------------------------------------------------------------------------------
class UI_API UButtonImage : public UImageComponent
{
public:
	FImage* DownImage;
	DECLARE_UI_CLASS(UButtonImage,UImageComponent)
	//overrides
	FImage* GetImage();
	//interface
	void SetImage( FImage* _Image, FImage* _DownImage );
	virtual void SetMouse( UMouseCursor* M );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
//!!ARL		Ar << DownImage;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UMultiImageComponent : public UImageComponentBase
{
public:
	INT Index;
	TArray<FImage*> Images;
	UBOOL bAutoResize;
	DECLARE_UI_CLASS(UMultiImageComponent,UImageComponentBase)
	//overrides
	void Import( SectionData Section );
	FImage* GetImage();
	FDimension GetSizeI();
	void Init();
	//interface
	void SetImage( FImage* _Image );
	INT GetIndex();

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Index;
//!!ARL		for(INT i=0;i<Images.Num();i++)
//!!ARL			Ar << Images(i);
		SERIALIZE_BOOL(bAutoResize);
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UAnimatedImageComponent : public UImageComponentBase
{
public:
	TArray<FImage*> Images;
	FLOAT Tween;
	DECLARE_UI_CLASS(UAnimatedImageComponent,UImageComponentBase)
	//overrides
	void		SetTween( FLOAT Pct );
	void		SetImage( FImage* _Image );
	FImage*		GetImage();
	void		Import( SectionData Section );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
//!!ARL		for(INT i=0;i<Images.Num();i++)
//!!ARL			Ar << Images(i);
		Ar << Tween;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UScriptComponent : public UComponent
{
public:
	DECLARE_UI_CLASS(UScriptComponent,UComponent)
	//overrides
	void Import( SectionData Section );
	//interface
	ActionEvent* GetTarget(){ return (AccessFlags & ACCESSOR_ScriptComponentTarget) ? GetAccessor(ACCESSOR_ScriptComponentTarget) : NULL; }
	void SendScriptEvent(UCanvas* C);

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UKeyEvent : public UComponent
{
public:
	struct FKeyEvent
	{
		EInputKey Key;
		EInputAction Action;
		ActionEvent* Event;
		FKeyEvent(){}
		FKeyEvent(EInputKey _Key,EInputAction _Action,ActionEvent* _Event):Key(_Key),Action(_Action),Event(_Event){}
		friend FArchive &operator<<( FArchive& Ar, FKeyEvent& E )
			{ return Ar << *(BYTE*)&E.Key << *(BYTE*)&E.Action << E.Event; }
	};
	TArray<FKeyEvent> Events;
	BITFIELD bEnabled:1 GCC_PACK(4);
	DECLARE_UI_CLASS(UKeyEvent,UComponent)
	//overrides
	void Destroy();
	void Import( SectionData Section );
	//component overrides
	void SetEnabledI( UBOOL bEnabled );
	UBOOL IsEnabledI();
	//interface
	virtual void SetConsole( UUIConsole* C );
	virtual void AddEvent( EInputKey Key, EInputAction Action, ActionEvent* Event );
	//key events
	void UI_CALLBACK KeyEvent( void* Parms );

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Events;
		SERIALIZE_BOOL(bEnabled)
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
class UI_API UEditor : public UContainer
{
public:
    class UComponent* Current;
    class UComponentHandle* Mover;
    class UComponentHandle* ResizerN;
    class UComponentHandle* ResizerS;
    class UComponentHandle* ResizerE;
    class UComponentHandle* ResizerW;
    class UComponentHandle* ResizerNE;
    class UComponentHandle* ResizerNW;
    class UComponentHandle* ResizerSE;
    class UComponentHandle* ResizerSW;
    DECLARE_UI_CLASS(UEditor,UContainer)
	//overrides
	void Destroy();
	void Import( SectionData Section );
	//input support
	void UI_CALLBACK MousePress		( void* Parms );
	void UI_CALLBACK MouseRelease	( void* Parms );
	void UI_CALLBACK KeyEvent		( void* Parms );
	//interface
	virtual void SetMouse( UMouseCursor* M );
	virtual void SetConsole( UUIConsole* C );
	//helpers
	void Select( UComponent* NewComponent );

	// Fix ARL: Maybe use hardcoded indices to keep track of component handles rather than pointers?
	void Serialize( FArchive &Ar ){ Super::Serialize(Ar); }

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


//------------------------------------------------------------------------------
// Description: Base class for all ui editing handles.
//------------------------------------------------------------------------------
class UI_API UComponentHandle : public UImageComponent
{
public:
	class UComponent* Target;
	class UMouseCursor* Mouse;
	FPoint LastMousePos;
	DECLARE_UI_ABSTRACT_CLASS(UComponentHandle,UImageComponent)
	//overrides
	void Update(UPDATE_DEF);
	//mouse support
	void UI_CALLBACK MousePress( void* Parms );
	void UI_CALLBACK MouseRelease( void* Parms );
	//interface
	virtual void SetMouse( UMouseCursor* M );
	//pure virtuals
	virtual void UpdateComponent( UComponent* C, FDimension Delta )=0;

	void Serialize( FArchive &Ar );

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};
//------------------------------------------------------------------------------
class UI_API UMoveHandle : public UComponentHandle
{
public:
	DECLARE_UI_CLASS(UMoveHandle,UComponentHandle)
	void Update(UPDATE_DEF);
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Center,ALMT_Center);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandle : public UComponentHandle
{
public:
	DECLARE_UI_ABSTRACT_CLASS(UResizeHandle,UComponentHandle)
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleN : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleN,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Center,ALMT_Top);
		Location=FPoint(0,-16);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleS : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleS,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Center,ALMT_Bottom);
		Location=FPoint(0,16);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleE : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleE,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Right,ALMT_Center);
		Location=FPoint(16,0);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleW : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleW,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Left,ALMT_Center);
		Location=FPoint(-16,0);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleNE : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleNE,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Right,ALMT_Top);
		Location=FPoint(8,-8);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleNW : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleNW,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Left,ALMT_Top);
		Location=FPoint(-8,-8);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleSE : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleSE,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Right,ALMT_Bottom);
		Location=FPoint(8,8);
	}
};
//------------------------------------------------------------------------------
class UI_API UResizeHandleSW : public UResizeHandle
{
public:
	DECLARE_UI_CLASS(UResizeHandleSW,UResizeHandle)
	void UpdateComponent( UComponent* C, FDimension Delta );
	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Align=FAlignment(ALMT_Left,ALMT_Bottom);
		Location=FPoint(-8,8);
	}
};


//------------------------------------------------------------------------------
class UI_API UCredits : public UFixedSizeContainer
{
protected:
	struct FCreditEventLocal
	{
		FLOAT Pct;
		uiEvent Event;
		FCreditEventLocal(){}
		FCreditEventLocal(FLOAT _Pct,uiEvent _Event):Pct(_Pct),Event(_Event){}
		friend FArchive &operator<<( FArchive& Ar, FCreditEventLocal& E )
			{ return Ar << E.Pct << E.Event; }
	};
	struct FCreditEventGlobal
	{
		FLOAT Pct;
		ActionEvent* Event;
		FCreditEventGlobal(){}
		FCreditEventGlobal(FLOAT _Pct,ActionEvent* _Event):Pct(_Pct),Event(_Event){}
		friend FArchive &operator<<( FArchive& Ar, FCreditEventGlobal& E )
			{ return Ar << E.Pct << E.Event; }
	};
public:
	FLOAT							Rate;
	FLOAT							Timer;
	INT								Index;
	TMap<FString,FString>			Styles;
	TArray<FString>					Elements;
	TArray<FCreditEventLocal>		LocalEvents;
	TArray<FCreditEventGlobal>		GlobalEvents;
	uiEvent							StartEvent;
	uiEvent							StopEvent;
	BITFIELD						bOn:1; GCC_PACK(4);
	DECLARE_UI_CLASS(UCredits,UFixedSizeContainer)
	//interface
	void AddStyle( const TCHAR* Tag, const TCHAR* ComponentName )	{ Styles.Set(Tag,ComponentName); }
	void AddElement( const TCHAR* E )								{ new(Elements)FString(E); }
	void AddLocalEvent( FLOAT Pct, uiEvent Event )					{ new(LocalEvents)FCreditEventLocal(Pct,Event); }
	void AddGlobalEvent( FLOAT Pct, ActionEvent* Event )			{ new(GlobalEvents)FCreditEventGlobal(Pct,Event); }
	void Start();
	void Stop();
	//helpers
	void UpdateComponent( UComponent* C, FLOAT DeltaTime );
	void SendGlobalEvents( FLOAT OldPct, FLOAT NewPct );
	//overrides
	void Update(UPDATE_DEF);
	void Import( SectionData Section );
	void HandleEvent( uiEvent Event );
	UBOOL IsEnabledI(){ return bOn; }

	void Serialize( FArchive &Ar )
	{
		Super::Serialize(Ar);
		Ar << Rate << Timer << Index << Styles << Elements << LocalEvents << GlobalEvents << StartEvent << StopEvent;
	}

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
		Size=FDimension(640,480);
		Rate=24.f;
	}
};


//------------------------------------------------------------------------------
class UI_API UMaterialContainer : public UFixedSizeContainer
{
public:
	DECLARE_UI_CLASS(UMaterialContainer,UFixedSizeContainer)

	//overrides
	void Update(UPDATE_DEF);

	DEFAULT_PROPERTIES
	{
		SUPER_PROPS
	}
};


#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
