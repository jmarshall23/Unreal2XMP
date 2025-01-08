
//////////////////////
// Native variables //
//////////////////////

protected:

TOrderedArray<ActionEvent*> Accessors;	// Ordered according to AccessorMasks.	// Fix ARL: Change to pointer to save a bit of space?  (most components never use this anyway)

DWORD AccessFlags;

DWORD LockFlags;

typedef TArray<UComponent*> EventComponents;
typedef TMap< uiEvent, EventComponents* > EventComponentsMap;

friend FArchive& operator<<( FArchive& Ar, EventComponents* &A );
friend FArchive& operator<<( FArchive& Ar, EventComponentsMap* &A );

public:	// should be protected (or even private), but exposed for serialization purposes.
EventComponentsMap* EventTable;


////////////////////
// Access support //
////////////////////

public:
	void AddAccessor( ActionEvent* Accessor );

protected:
	ActionEvent* GetAccessor( DWORD Type ){ return Accessors(GetAccessorIndex(Type)); }
	ActionEvent* GetAccessorSafe( DWORD Type ){ return (AccessFlags & Type) ? Accessors(GetAccessorIndex(Type)) : NULL; }

// We store a bit mask to keep track of which accessors are being used on a per component basis.
// Those accessors are then stored in a tarray ordered according to their associated mask value.
// This allows us to calculate the accessor's index by counting the number of set bits to the right of the one we're looking for.
INT GetAccessorIndex( DWORD Type )
{
	DWORD bits = AccessFlags & (Type-1);	// mask off all the bits to the left including the given one.
	INT count=0; for(;bits;count++)			// count the remaining bits...
		bits &= (bits-1);					// by repeatedly masking off the least significant bit until there are none left.
	return count;							// this gives us the accessor's index since there are 'count' other accessors before this one.
}

#define SIMPLE_ACCESSOR_NOCACHE_BEGIN(name) \
	if( AccessFlags & ACCESSOR_##name ){ \
		ActionEvent* Accessor = GetAccessor(ACCESSOR_##name); \
		check(Accessor!=NULL);/*checkSlow*/ \
		check(Accessor->Type==ACCESSOR_##name);/*checkSlow*/ \

#define SIMPLE_ACCESSOR_NOCACHE_END \
	}

#define SIMPLE_ACCESSOR_BEGIN(name) \
	SIMPLE_ACCESSOR_NOCACHE_BEGIN(name) \
		if( Accessor->NeedsUpdate() ){

#define SIMPLE_ACCESSOR_BEGIN_ENABLED(name) \
	SIMPLE_ACCESSOR_NOCACHE_BEGIN(name) \
		if( Accessor->NeedsUpdate() && IsEnabled() ){

#define SIMPLE_ACCESSOR_END \
	SIMPLE_ACCESSOR_NOCACHE_END \
		}

#define SIMPLE_ACCESSOR(name) \
	SIMPLE_ACCESSOR_BEGIN(name) \
		Accessor->OnEvent(); \
	SIMPLE_ACCESSOR_END

#define SIMPLE_ACCESSORX(name,parms) \
	SIMPLE_ACCESSOR_BEGIN(name) \
		Accessor->OnEvent(parms); \
	SIMPLE_ACCESSOR_END

#define SIMPLE_ACCESSORX_NOCACHE(name,parms) \
	SIMPLE_ACCESSOR_NOCACHE_BEGIN(name) \
		Accessor->OnEvent(parms); \
	SIMPLE_ACCESSOR_NOCACHE_END

#define SIMPLE_ACCESSOR_SET(name,type,value) \
	SIMPLE_ACCESSOR_BEGIN(name) \
		type P = value; \
		Accessor->OnEvent(&P); \
	SIMPLE_ACCESSOR_END

#define IMPLEMENT_ACCESSOR_BEGIN(name,type) \
	SIMPLE_ACCESSOR_BEGIN(name) \
		struct FuncParms { type ReturnValue; } Parms; \
		Accessor->OnEvent(&Parms); \
		type &Result = Parms.ReturnValue; \
		/* to prevent infinite recursion */ \
		AccessFlags &= ~ACCESSOR_##name;

#define IMPLEMENT_ACCESSOR_BEGIN_ENABLED(name,type) \
	SIMPLE_ACCESSOR_BEGIN_ENABLED(name) \
		struct FuncParms { type ReturnValue; } Parms; \
		Accessor->OnEvent(&Parms); \
		type &Result = Parms.ReturnValue; \
		/* to prevent infinite recursion */ \
		AccessFlags &= ~ACCESSOR_##name;

#define IMPLEMENT_ACCESSOR_BEGINX(name,type,init) \
	SIMPLE_ACCESSOR_BEGIN(name) \
		type Result = init; \
		Accessor->OnEvent(&Result); \
		/* to prevent infinite recursion */ \
		AccessFlags &= ~ACCESSOR_##name;

#define IMPLEMENT_ACCESSOR_END(name) \
		/* restore flag */ \
		AccessFlags |= ACCESSOR_##name; \
	SIMPLE_ACCESSOR_END

#define IMPLEMENT_ACCESSOR(name,type) \
	IMPLEMENT_ACCESSOR_BEGIN(name,type) \
		Set##name(Result); \
	IMPLEMENT_ACCESSOR_END(name)

#define IMPLEMENT_ACCESSOR_ENABLED(name,type) \
	IMPLEMENT_ACCESSOR_BEGIN_ENABLED(name,type) \
		Set##name(Result); \
	IMPLEMENT_ACCESSOR_END(name)

#define IMPLEMENT_ACCESSOR_C(name,type,other) \
	IMPLEMENT_ACCESSOR_BEGIN(name,type) \
		DWORD OldAccessFlags = AccessFlags; \
		AccessFlags &= ~ACCESSOR_##other; \
		Set##name(Result); \
		AccessFlags = OldAccessFlags; \
	IMPLEMENT_ACCESSOR_END(name)

#define IMPLEMENT_ACCESSOR_C3(name,type,other1,other2) \
	IMPLEMENT_ACCESSOR_BEGIN(name,type) \
		DWORD OldAccessFlags = AccessFlags; \
		AccessFlags &= ~(ACCESSOR_##other1 | ACCESSOR_##other2); \
		Set##name(Result); \
		AccessFlags = OldAccessFlags; \
	IMPLEMENT_ACCESSOR_END(name)


////////////////////
// Lock mechanism //
////////////////////

public:
	
enum LockMasks
{
	//component
	LOCK_Location	=BIT(0),
	LOCK_Size		=BIT(1),
	LOCK_Style		=BIT(2),
	LOCK_Color		=BIT(3),
	LOCK_Alpha		=BIT(4),
	LOCK_AlphaPct	=BIT(5),
	LOCK_Alignment	=BIT(6),
	LOCK_SendEvent	=BIT(7),
	//not implemented yet.
	LOCK_X			=BIT(8),
	LOCK_Y			=BIT(9),
	LOCK_Width		=BIT(10),
	LOCK_Height		=BIT(11),
	LOCK_R			=BIT(12),
	LOCK_G			=BIT(13),
	LOCK_B			=BIT(14),
	LOCK_AlignmentX	=BIT(15),
	LOCK_AlignmentY	=BIT(16),
	//selector
	LOCK_Borders	=BIT(17),
};

void Lock(DWORD Func){ LockFlags |= Func; }
void Unlock(DWORD Func){ LockFlags &= ~Func; }
UBOOL Locked(DWORD Func){ return LockFlags & Func; }


/////////////////
// Static data //
/////////////////

public:

#define UI_TEXTURE_HASH_SIZE 7 /*128*/
static THashTable< FString, UTexture*, UI_TEXTURE_HASH_SIZE > GTextureCache;


//////////////////////
// UObject overides //
//////////////////////

public:

void DeleteUObject()
{
	Destroy();
	delete this;
}

virtual void Constructed(){}
virtual void Destroy();
virtual UBOOL IsLessThan( const UComponent* A ) const { return DrawOrder > A->DrawOrder; }	// Higher numbers get drawn on top.

FString GetName(){ return *Name; }
FString GetFullName(){ return FString::Printf(TEXT("%s::%s"),GetClass()->GetName(),**Name); }
UBOOL IsTransient(){ return Name.GetIndex() & UI_TransientMask; }

UBOOL IsA(uiClass* Class)
{
	for(uiClass* C=GetClass();C;C=C->GetSuper())
		if(C==Class)
			return 1;
	return 0;
}


//////////////////////
// Listener support //
//////////////////////

public:

void	AddListener( UObject* L );									// Registers the given Listener to recieve the appropriate events from this Component.
UBOOL	RemoveListener( UObject* L );								// Removes the given Listener from this component.  Returns false if the given listener wasn't registered to begin with.
INT		RemoveListenerType( FName Class, UBOOL bAndSubclasses=0 );	// Removes all Listeners of the given class and (optionally) subclasses.  Returns the number of actual Listeners removed.

void	RegisterEvent( uiEvent Event, UComponent* C );				// Registers component 'C' so it recieves all 'Event' events generated by this Component.
void	RemoveEventListener( UComponent* C );						// Removes all entries of 'C' from our event table.  This is usually only called when a component is destroyed.
void	DisableEvent( uiEvent Event );								// Don't recieve HandleEvent notifications for this event.
void	EnableEvent( uiEvent Event );								// Reenable recieving of HandleEvent notifications for this event.
UBOOL	IsEventEnabled( uiEvent Event );							// Check if this event is not disabled.

void	SendNotification( FString FuncName );						// Calls the specified function on all our listeners.
void	SendEvent( uiEvent Event );									// Sends the given Event to all listening Components via HandleEvent.


/////////////
// Helpers //
/////////////

public:

UContainer*			GetParent(uiClass* Type=NULL);
UComponent*			GetRoot();
UConsole*			GetConsole();
UUIConsole*			GetUIConsole();
APlayerController*	GetPlayerOwner();
DOUBLE				GetTimeSeconds();
UClient*			GetClient();

virtual UContainer*	GetGroup(){ return GetParent(); }

void ConsoleCommand( FString Command );

static uiEvent GetEvent( FString EventName ){ return ::GetEvent(EventName); }

template<class T> T* GetParentType()
{
	return (T*)GetParent( T::StaticClass() );
}

///////////////////
// Notifications //
///////////////////

public:

// I'm getting tired of changing this in a million places.
#define UPDATE_DEF FSceneNode* Frame, FUIRenderer* Ren
#define UPDATE_PARMS Frame, Ren

virtual void HandleEvent( uiEvent Event ){}	// Override to handle specific listener events.
virtual void Tick( FLOAT DeltaTime ){}		// Time update (called once per Frame - must register with console to receive notifications).
virtual void Update(UPDATE_DEF){}			// Updates this component.
virtual void Reset();						// Called between level loads (used for resetting timers, etc.).
virtual void Shrink(){}						// Interface for cleaning up TArray slack.
virtual void SetTween( FLOAT Pct ){}		// Called while tweening between two states (0.0->1.0).  Used for animated images, etc.

virtual void Precache(UPDATE_DEF){ Update(UPDATE_PARMS); }	// override to disable culling (like IsEnabled checks, etc.)


///////////////////////
// Animation helpers //
///////////////////////

public:

// Fix ARL: Should we get rid of SetTween?
virtual void Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct );
virtual FLOAT AdjustTweenPct( FLOAT TweenPct, UMultiStateComponent* Source );	// Play with this to affect ease in/out.


////////////////
// Interfaces //
////////////////

public:

virtual void Import( SectionData Section );

void			SetLocation			( const FPoint& P		);	// Moves this component to a new location.
FPoint			GetLocation			( void					);	// Gets the location of this component in the form of a point specifying the component's top-left corner.
FPoint			GetScreenCoords		( void					);	// Gets the location of this component relative to the root component (which should match the screen).
FPoint			GetAlignedLocation	( void					);	// Gets the locations of this component relative to its parent component and accounting for alignment.
void			SetSize				( const FDimension& D	);	// Resizes this component so that it has width 'd.width' and height 'd.height'.
FDimension		GetSize				( void					);	// Returns the size of this component.
void			SetStyle			( ERenderStyle S		);	// Sets the drawing style of this component.
ERenderStyle	GetStyle			( void					);	// Gets the drawing style of this component.
void			SetColor			( const FColor& C		);	// Sets the color of this component.
FColor			GetColor			( void					);	// Gets the color of this component.
void			SetAlpha			( BYTE A				);	// Independantly set the alpha of this component (redirects through SetColor by default).
void			SetAlphaPct			( FLOAT Pct				);	// Modify the existing alpha of this component by the specified percentage (0..1).
BYTE			GetAlpha			( void					);	// Independantly get the alpha of this component (redirects through GetColor by default).
FRectangle		GetBounds			( void					);	// Gets the bounds of this component in the form of a Rectangle object.
FRectangle		GetScreenBounds		( void					);	// Gets the screen bounds of this component in the form of a Rectangle object.
void			SetAlignment		( const FAlignment& A	);	// Change the alignment of this component.
FAlignment		GetAlignment		( void					);	// Returns the alignment.
UBOOL			Contains			( const FPoint& P		);	// Checks whether this component "contains" the specified point, where the point's x and y coordinates are defined to be relative to the coordinate system of this component.
UBOOL			ContainsAbs			( const FPoint& P		);	// Checks whether this component "contains" the specified point, where the point's x and y coordinates are defined to be relative to the screen.
UComponent*		GetComponentAt		( const FPoint& P		);	// Returns the component or subcomponent that contains the specified point.
FLOAT			GetAbsDrawOrder		( void					);	// Returns the absolute draw order of this component (assumes all DrawOrders are in the range 0 to 10).
void			SetParent			( class UContainer* P	);	// Notification for when this component's Parent is changed.
void			SetVisibility		( UBOOL bVisible		);	// Shows or hides this component as specified.
UBOOL			IsVisible			( void					);	// Determines whether this component should be visible when its parent is visible.
UBOOL			IsShowing			( void					);	// Determines whether this component is showing on screen.
UBOOL			IsDisplayable		( void					);	// Determines whether this component is displayable.
void			SetEnabled			( UBOOL bEnabled		);	// Enables or disables this component as specified.
UBOOL			IsEnabled			( void					);	// Determines whether this component is enabled.
void			Validate			( void					);	// Ensures that this component has a valid layout.
void			Invalidate			( void					);	// Invalidates this component.
UBOOL			IsValid				( void					);	// Determines whether this component is valid.
void			BringToFront		( void					);	// Render component on top of all others within its group.
void			SendToBack			( void					);	// Render component behind all others within its group.
void			BringForward		( INT NumLevels=1		);	// Draw x layers higher than currently drawn within its group (defaults to 1 layer).
void			SendBack			( INT NumLevels=1		);	// Draw x layers lower than currently drawn within its group (defaults to 1 layer).
void			RequestFocus		( void					);	// Requests that this component get the input focus.
UBOOL			HasFocus			( void					);	// Returns true if this Component has the keyboard focus.

UComponent*		GetComponent		( uiName Name			);	// Return the contained component that matches the given name.
UComponent*		FindLooseComponent	( uiName Name			);	// Return the first contained component that loosely matches the given name (loose = ignore postfix numbers).
void			GetLooseComponents	( uiName Name, TArray<UComponent*> &Components );	// Return the contained component that loosely matches the given name (loose = ignore postfix numbers).

uiPath			GetFullPath			( void					);	// Return the full path of this component from the Root.
UComponent*		GetComponent		( uiPath Path			);	// Return the subcomponent as specified by the given path relative to this component.


///////////////////////////
// Convenience functions //
///////////////////////////

public:

// Fix ARL: Make some of these overloaded functions?
// These functions need not be overridable since their implementations all depend upon other overridable functions.
// Suffix of 'C' indicates the "component" version of that function - useful if the caller doesn't know about Points, Rectangles, etc.

FLOAT		GetX()																	{ return GetLocation().X; }
FLOAT		GetY()																	{ return GetLocation().Y; }
FPoint		GetLocationOut( FPoint& P )												{ P=GetLocation(); return P; }
FPoint		GetLocationOutC( FLOAT& X, FLOAT& Y )									{ FPoint P=GetLocation(); X=P.X; Y=P.Y; return P; }
void		SetX( FLOAT X )															{ SetLocation( FPoint(X,GetY()) ); }
void		SetY( FLOAT Y )															{ SetLocation( FPoint(GetX(),Y) ); }
void		SetLocationC( FLOAT X, FLOAT Y )										{ SetLocation( FPoint(X,Y) ); }
FLOAT		GetHeight()																{ return GetSize().Height; }
FLOAT		GetWidth()																{ return GetSize().Width; }
FDimension	GetSizeOut( FDimension& D )												{ D=GetSize(); return D; }
FDimension	GetSizeOutC( FLOAT& Width, FLOAT& Height )								{ FDimension D=GetSize(); Width=D.Width; Height=D.Height; return D; }
void		SetWidth( FLOAT Width )													{ SetSize( FDimension(Width,GetHeight()) ); }
void		SetHeight( FLOAT Height )												{ SetSize( FDimension(GetWidth(),Height) ); }
void		SetSizeC( FLOAT Width, FLOAT Height )									{ SetSize( FDimension(Width,Height) ); }
BYTE		GetR()																	{ return GetColor().R; }
BYTE		GetG()																	{ return GetColor().G; }
BYTE		GetB()																	{ return GetColor().B; }
FColor		GetColorOut( FColor& C )												{ C=GetColor(); return C; }
FColor		GetColorOutC( BYTE& R, BYTE& G, BYTE& B, BYTE& A )						{ FColor C=GetColor(); R=C.R; G=C.G; B=C.B; A=C.A; return C; }
void		SetR( BYTE R )															{ FColor C=GetColor(); C.R=R; SetColor(C); }
void		SetG( BYTE G )															{ FColor C=GetColor(); C.G=G; SetColor(C); }
void		SetB( BYTE B )															{ FColor C=GetColor(); C.B=B; SetColor(C); }
void		SetColorC( BYTE R=0, BYTE G=0, BYTE B=0, BYTE A=255 )					{ SetColor( FColor(R,G,B,A) ); }
FRectangle	GetBoundsOut( FRectangle& R )											{ R=GetBounds(); return R; }
FRectangle	GetBoundsOutC( FLOAT& X, FLOAT& Y, FLOAT& Width, FLOAT& Height )		{ FRectangle R=GetBounds(); X=R.X; Y=R.Y; Width=R.Width; Height=R.Height; return R; }
FRectangle	GetScreenBoundsOut( FRectangle& R )										{ R=GetScreenBounds(); return R; }
FRectangle	GetScreenBoundsOutC( FLOAT& X, FLOAT& Y, FLOAT& Width, FLOAT& Height )	{ FRectangle R=GetScreenBounds(); X=R.X; Y=R.Y; Width=R.Width; Height=R.Height; return R; }
void		SetAlignmentX( EAlignment XAxis )										{ SetAlignment( FAlignment(XAxis,GetAlignmentY()) ); }
void		SetAlignmentY( EAlignment YAxis )										{ SetAlignment( FAlignment(GetAlignmentX(),YAxis) ); }
void		SetAlignmentC( EAlignment XAxis, EAlignment YAxis )						{ SetAlignment( FAlignment(XAxis,YAxis) ); }
EAlignment	GetAlignmentX()															{ return (EAlignment)GetAlignment().XAxis; }
EAlignment	GetAlignmentY()															{ return (EAlignment)GetAlignment().YAxis; }
FAlignment	GetAlignmentOut( FAlignment& A )										{ A=GetAlignment(); return A; }
FAlignment	GetAlignmentOutC( EAlignment& XAxis, EAlignment& YAxis )				{ FAlignment A=GetAlignment(); XAxis=(EAlignment)A.XAxis; YAxis=(EAlignment)A.YAxis; return A; }
UComponent*	GetComponentAtC( FLOAT X, FLOAT Y )										{ return GetComponentAt( FPoint(X,Y) ); }
UBOOL		ContainsC( FLOAT X, FLOAT Y )											{ return Contains( FPoint(X,Y) ); }


//////////////////
// Overridables //
//////////////////

protected:
// Fix ARL: Does this really need to be protected?  If no functions call their Super implementation, make it private.
// (C++ needs an overridable, but not callable keyword.)

// Breaking these out separates interface from implementation.
// It also allows the interface functions to enforce pre/post-conditions.
// Further, it allows the interface functions to always call the accessor/modifier ActionEvent.
// If overridden here, we don't need to worry about remembering to or when to call the accessor/modifier.
// Any subclasses (especially composites) should implement the majority (if not all) of the following overrides.
// Protected so we can call the Super's implementation
// The 'I' suffix indentifies these functions as "implementation".  (Interface functions require no special identifiers.)

virtual void			SetLocationI		( const FPoint& P		);
virtual FPoint			GetLocationI		( void					);
virtual FPoint			GetScreenCoordsI	( void					);
virtual FPoint			GetAlignedLocationI	( void					);
virtual void			SetSizeI			( const FDimension& D	)/*=0*/	{ return;									};
virtual FDimension		GetSizeI			( void					)/*=0*/	{ return FDimension(0.f,0.f);				};
virtual void			SetStyleI			( ERenderStyle S		)/*=0*/	{ return;									};
virtual ERenderStyle	GetStyleI			( void					)/*=0*/	{ return STY_Normal;						};
virtual void			SetColorI			( const FColor& C		)/*=0*/	{ return;									};
virtual FColor			GetColorI			( void					)/*=0*/	{ return FColor(0,0,0,0);					};
virtual FRectangle		GetBoundsI			( void					);
virtual FRectangle		GetScreenBoundsI	( void					);
virtual void			SetAlignmentI		( const FAlignment& A	);
virtual FAlignment		GetAlignmentI		( void					);
virtual UBOOL			ContainsI			( const FPoint& P		);
virtual UBOOL			ContainsAbsI		( const FPoint& P		);
virtual UComponent*		GetComponentAtI		( const FPoint& P		);
virtual FLOAT			GetAbsDrawOrderI	( void					);
virtual void			SetParentI			( class UContainer* P	);
virtual void			SetVisibilityI		( UBOOL bVisible		)/*=0*/	{ return;									};
virtual UBOOL			IsVisibleI			( void					)/*=0*/	{ return true;								};
virtual UBOOL			IsShowingI			( void					)/*=0*/	{ return true;								};
virtual UBOOL			IsDisplayableI		( void					)/*=0*/	{ return true;								};
virtual void			SetEnabledI			( UBOOL bEnabled		)/*=0*/	{ return;									};
virtual UBOOL			IsEnabledI			( void					)/*=0*/	{ return true;								};
virtual void			ValidateI			( void					)/*=0*/	{ return;									};
virtual void			InvalidateI			( void					)/*=0*/	{ return;									};
virtual UBOOL			IsValidI			( void					)/*=0*/	{ return true;								};
virtual void			BringToFrontI		( void					);
virtual void			SendToBackI			( void					);
virtual void			BringForwardI		( INT NumLevels			);
virtual void			SendBackI			( INT NumLevels			);
virtual void			RequestFocusI		( void					);
virtual UBOOL			HasFocusI			( void					);

virtual UComponent*		GetComponentI		( uiName Name			);
virtual UComponent*		FindLooseComponentI	( uiName Name			);
virtual void			GetLooseComponentsI	( uiName Name, TArray<UComponent*> &Components );
virtual uiPath			GetFullPathI		( void					);
virtual UComponent*		GetComponentI		( uiPath Path			);

// These two(three now) functions are really "Convenience functions" (see above), but it's very useful to allow them to be separately overridable outside of GetColor/SetColor.
virtual BYTE			GetAlphaI			( void					)		{ return GetColor().A;						};
virtual void			SetAlphaI			( BYTE A				)		{ FColor C=GetColor(); C.A=A; SetColor(C);	};
virtual void			SetAlphaPctI		( FLOAT Pct				)		{ SetAlpha( GetAlpha() * Pct );				};
// Fix ARL: Generalize Pct setting support.  Any interface function should be able to be modified using a specified percentage.
// NOTE[aleiby]: The problem is composites.  If the blending is done the top level you get a "blurring" effect.  The tweening needs to be done at the leaves.


DWORD GetPolyFlags()
{
	switch(GetStyle())
	{
	case STY_None:			return PF_Invisible;
	case STY_Translucent:	return PF_Translucent;
	case STY_Modulated:		return PF_Modulated;
	case STY_Masked:		return PF_Masked;
	case STY_Alpha:			return PF_AlphaTexture;
	default:				return 0;
	}
}


///////////////////
// Mouse support //
///////////////////

public:

void SetMouseOverEvent		( UMouseCursor* M, ActionEvent* EnterEvent, ActionEvent* ExitEvent );
void SetMouseEnterEvent		( UMouseCursor* M, ActionEvent* Event );
void SetMouseExitEvent		( UMouseCursor* M, ActionEvent* Event );
void SetMousePressEvent		( UMouseCursor* M, ActionEvent* Event );
void SetMouseReleaseEvent	( UMouseCursor* M, ActionEvent* Event );
void SetMouseClickEvent		( UMouseCursor* M, ActionEvent* Event );

// MouseClick helper event callbacks.
void UI_CALLBACK MouseClick_Enter  (void*){ bMouseClick_Over=true; }
void UI_CALLBACK MouseClick_Exit   (void*){ bMouseClick_Over=false; }
void UI_CALLBACK MouseClick_Press  (void* P)
{
	if( bMouseClick_Over )
	{
		bMouseClick_PressedWhileOver=true;
//?		((MouseNotifyParms*)P)->bHandled=true;
	}
}
void UI_CALLBACK MouseClick_Release(void* P)
{
	if( bMouseClick_Over && bMouseClick_PressedWhileOver )
	{
		SIMPLE_ACCESSORX(MouseClick,P)
//?		((MouseNotifyParms*)P)->bHandled=true;
	}
	bMouseClick_PressedWhileOver=false;
}


