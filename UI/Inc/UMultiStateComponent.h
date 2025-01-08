
////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
//UComponent*	GetState();			// Returns the current state of this component.
UBOOL			SetState( INT i );	// Sets the current state of this component.
INT				GetStateIndex();	// Gets the state index.

//------------------------------------------------------------------------------
// Appends the state to the List. If you want to replace/remove a given State,
// you need to manually remove it from the States array yourself.
UBOOL AddState( UComponent* C );

//------------------------------------------------------------------------------
// Removes the state at the given index and returns the component.  
// The component is *not* deleted, but since it is returned, the caller can 
// choose to delete it themself.
UComponent* RemoveState( INT i );

//------------------------------------------------------------------------------
void TweenLocation	( UBOOL bNewState ){ bTweenLocation	= bNewState; }
void TweenSize		( UBOOL bNewState ){ bTweenSize		= bNewState; }
void TweenColor		( UBOOL bNewState ){ bTweenColor	= bNewState; }


///////////////
// Overrides //
///////////////

void Serialize( FArchive &Ar );
void Import( SectionData Section );


/////////////////
// Transitions //
/////////////////

//------------------------------------------------------------------------------
//void	HandleEvent( uiEvent Event );
void	Transition( uiEvent Event );
void	AddTransition( uiEvent Event, INT Start, INT End, FLOAT Time, UComponent* Template, UBOOL bNoOverride=false, uiEvent AdditionalEvent1=EVENT_None, uiEvent AdditionalEvent2=EVENT_None, uiEvent AdditionalEvent3=EVENT_None );
void	AddTriggerEvent( INT iTransition, ActionEvent* Event, FLOAT Time );


/////////////////////////
// Transitioning state //
/////////////////////////

// Fix ARL: This is a bit messy since there's no native support for states in C++, but this simple hack should get us by for now rather than implementing a full fledge state pattern.
void GotoState_Transitioning()	{ bTransitioning=true; TransitionTag++; Transitioning_BeginState(); }
void GotoState_None()			{ if(bTransitioning){ Transitioning_EndState(); bTransitioning=false; } }

protected:
	void Transitioning_BeginState();
	void Transitioning_EndState();

	UComponent*	Transitioning_GetState();
	void		Transitioning_HandleEvent( uiEvent Event );
	void		Transitioning_Update(UPDATE_DEF);

	UComponent*	Global_GetState();
	void		Global_HandleEvent( uiEvent Event );
	void		Global_Update(UPDATE_DEF);

public:
	UComponent*	GetState()						{ if( bTransitioning ) return Transitioning_GetState();		else return Global_GetState();		}
	void		HandleEvent( uiEvent Event )	{ if( bTransitioning ) Transitioning_HandleEvent( Event );	else Global_HandleEvent( Event );	}
	void		Update(UPDATE_DEF)				{ if( bTransitioning ) Transitioning_Update(UPDATE_PARMS);	else Global_Update(UPDATE_PARMS);	}


/////////////
// Helpers //
/////////////

protected:
	virtual UBOOL SendTriggers( FLOAT Time );			// Returns true if at least one trigger is actually fired.


///////////////////////////////////
// Component interface redirects //
///////////////////////////////////

// NOTE[aleiby]: SetLocation and GetLocation purposely NOT overriden.

public:
	void			SetSizeI( const FDimension& D );
	FDimension		GetSizeI();
	ERenderStyle	GetStyleI();
	void			SetStyleI( ERenderStyle S );
	FColor			GetColorI();
	void			SetColorI( const FColor& C );
	FRectangle		GetBoundsI();
	FRectangle		GetScreenBoundsI();
	UComponent*		GetComponentAtI( const FPoint& P );
	void			SetVisibilityI( UBOOL bVisible );
	UBOOL			IsVisibleI();
	UBOOL			IsShowingI();
	UBOOL			IsDisplayableI();
	void			SetEnabledI( UBOOL bEnabled );
	UBOOL			IsEnabledI();
	void			ValidateI();
	void			InvalidateI();
	UBOOL			IsValidI();

	UComponent*		FindLooseComponentI( uiName Name );

	void			Tick( FLOAT DeltaSeconds );
//	void			Update(UPDATE_DEF);
	void			SetTween( FLOAT Pct );


