#include "UI.h"


////////////////////
// Initialization //
////////////////////

//------------------------------------------------------------------------------
void UMultiStateComponent::Serialize( FArchive &Ar )
{
	guard(UMultiStateComponent::Serialize);

	Super::Serialize(Ar);

	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Serialize"), *GetFullName());
	for(INT i=0;i<Components.Num();i++)
		debugf(TEXT("Components[%d]: %s"),i,Components(i)?Components(i)->GetFullName():TEXT("None")));

	// Save out the number of states separately because there may be situations where there are more components than states.
	INT NumStates;
	if( Ar.IsSaving() )
		NumStates = States.Num();
	Ar << AR_INDEX(NumStates);
	if( Ar.IsLoading() )
	{
		States.Empty();
		States.Add(NumStates);
	}
	for( INT i=0; i<NumStates; i++ )
	{
		INT Index;
		if( Ar.IsSaving() )
			Index = GetComponentIndex(States(i));
		Ar << AR_INDEX(Index);
		if( Ar.IsLoading() )
			States(i) = (Index>=0) ? Components(Index) : NULL;
		if(!States(i)) appErrorf(TEXT("Error loading state[%d]! Index=%d"),i,Index);
	}

	Ar << CurrentState;

	Ar << Transitions;
	for( INT i=0; i<Transitions.Num(); i++ )
	{
		INT Index;
		if( Ar.IsSaving() )
			Index = GetComponentIndex(Transitions(i).Template);
		Ar << AR_INDEX(Index);
		if( Ar.IsLoading() )
			Transitions(i).Template = (Index>=0) ? Components(Index) : NULL;
		if(!Transitions(i).Template) appErrorf(TEXT("Error loading transition[%d]! Index=%d"),i,Index);
	}

	Ar << NextTransitionEvent << PrevTransitionEvent << NextStateEvent << PrevStateEvent;
	Ar << TransitionIndex << TransitionTime << LastPct;

	SERIALIZE_BOOL(bTweenLocation);
	SERIALIZE_BOOL(bTweenSize);
	SERIALIZE_BOOL(bTweenColor);
	SERIALIZE_BOOL(bTweenAlpha);
	SERIALIZE_BOOL(bTweenLinear);
	SERIALIZE_BOOL(bTweenReverseEase);
	SERIALIZE_BOOL(bTweenReverseEaseOverride);

	SERIALIZE_BOOL(bTransitioning);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
UComponent* UMultiStateComponent::Global_GetState()
{
	guard(UMultiStateComponent::Global_GetState);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Global_GetState NumStates=%d"), *GetFullName(), States.Num()));

	IMPLEMENT_ACCESSOR_ENABLED(State,INT)

	return States(CurrentState);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::SetState( INT i )
{
	guard(UMultiStateComponent::SetState);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SetState( %d )"), *GetFullName(),i));

	if( States(i) )
	{
		if( States(CurrentState) )
			States(CurrentState)->SetEnabled(false);

		CurrentState=i;

		States(CurrentState)->SetEnabled(true);

		return true;
	}

	return false;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT UMultiStateComponent::GetStateIndex()
{
	guard(UMultiStateComponent::GetStateIndex);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::GetStateIndex"), *GetFullName()));

	return CurrentState;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::AddState( UComponent* C )
{
	guard(UMultiStateComponent::AddState);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::AddState( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("None")));

	if( AddComponent(C) )
	{
		States.AddItem(C);

		if(NumComponents()==1)
			SetState(0);
		else
			C->SetEnabled(false);

		return true;
	}
	else debugf(NAME_Warning,TEXT("(%s)UMultiStateComponent::AddState: Could not add component %s!"),C?C->GetFullName():TEXT("None"));

	return false;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent* UMultiStateComponent::RemoveState( INT i )
{
	guard(UMultiStateComponent::RemoveState);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::RemoveState( %d )"), *GetFullName(), i));

	if( States(i) && RemoveComponent(States(i)) )
	{
		UComponent* C=States(i);
		States(i)=NULL;
		return C;
	}

	return NULL;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////////
// Transitions //
/////////////////

//------------------------------------------------------------------------------
void UMultiStateComponent::Global_HandleEvent( uiEvent Event )
{
	guard(UMultiStateComponent::Global_HandleEvent);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Global_HandleEvent( %s )"), *GetFullName(), *LookupEventName(Event)));

	if( Event==NextTransitionEvent )
	{
		GotoState_None();	// Make sure we stop transitioning first.
		TransitionIndex++; if( TransitionIndex>=Transitions.Num() ) TransitionIndex = 0;
		while( !Transitions(TransitionIndex).Template )					// Fix ARL: Possible infinite loop!!!
			{ TransitionIndex++; if( TransitionIndex>=Transitions.Num() ) TransitionIndex = 0; }
		GotoState_Transitioning();
	}
	else if( Event==NextStateEvent )
	{
		INT iState = GetStateIndex();
		GotoState_None();
		SetState( WrappedIncrement( iState, States.Num() ) );
	}
	else if( Event==PrevTransitionEvent )
	{
		GotoState_None();	// Make sure we stop transitioning first.
		TransitionIndex--; if( TransitionIndex<0 ) TransitionIndex = Transitions.Num()-1;
		while( !Transitions(TransitionIndex).Template )					// Fix ARL: Possible infinite loop!!!
			{ TransitionIndex--; if( TransitionIndex<0 ) TransitionIndex = Transitions.Num()-1; }
		GotoState_Transitioning();
	}
	else if( Event==PrevStateEvent )
	{
		INT iState = GetStateIndex();
		GotoState_None();
		SetState( WrappedDecrement( iState, States.Num() ) );
	}
	else
	{
		Transition( Event );
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::Transition( uiEvent Event )
{
	guard(UMultiStateComponent::Transition);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Transition( %s )"), *GetFullName(), *LookupEventName(Event)));

	for( INT i=0; i<Transitions.Num(); i++ )
	{
		for( INT iEvent=0; iEvent<ARRAY_COUNT(Transitions(i).Event); iEvent++ )
		{
			if( Transitions(i).Event[iEvent] == Event )
			{
				GotoState_None();	// Make sure we stop transitioning first.
				TransitionIndex = i;
				GotoState_Transitioning();
				return;
			}
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::AddTransition
(
	uiEvent			Event
,	INT				Start
,	INT				End
,	FLOAT			Time
,	UComponent*		Template
,	UBOOL			bNoOverride
,	uiEvent			AdditionalEvent1
,	uiEvent			AdditionalEvent2
,	uiEvent			AdditionalEvent3
)
{
	guard(UMultiStateComponent::SetTransition);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::AddTransition( %s, %d, %d, %f, %s, %s, %s, %s, %s )"), *GetFullName(), *LookupEventName(Event), Start, End, Time, Template ? *Template->GetFullName() : TEXT("None"), bNoOverride ? TEXT("True") : TEXT("False"), *LookupEventName(AdditionalEvent1), *LookupEventName(AdditionalEvent2), *LookupEventName(AdditionalEvent3) ));

	INT Index = Transitions.AddZeroed();
	Transitions(Index).Event[0]		= Event;
	Transitions(Index).Event[1]		= AdditionalEvent1;
	Transitions(Index).Event[2]		= AdditionalEvent2;
	Transitions(Index).Event[3]		= AdditionalEvent3;
	Transitions(Index).Start		= Start;
	Transitions(Index).End			= End;
	Transitions(Index).Time			= Abs(Time);
	Transitions(Index).Template		= Template;
	Transitions(Index).bNoOverride	= bNoOverride;
	Transitions(Index).bReverseEase	= (Time<0.f);
	if( Template ){ AddComponent(Template); Template->SetEnabled(false); }

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::AddTriggerEvent( INT iTransition, ActionEvent* Event, FLOAT Time )
{
	guard(UMultiStateComponent::AddTriggerEvent);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::AddTriggerEvent( %d, %d, %f )"), *GetFullName(), iTransition, Event, Time));

	new(Transitions(iTransition).Triggers)FSTRUCT_TriggerDef( Time, Event );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////////////////
// Transitioning state //
/////////////////////////

//------------------------------------------------------------------------------
void UMultiStateComponent::Transitioning_BeginState()
{
	guard(UMultiStateComponent::Transitioning_BeginState);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Transitioning_BeginState[%d]"), *GetFullName(),TransitionIndex));

	TransitionTime = 0.0f;
	LastPct = -1.0f;

	// Handle transition times of zero.
	if( Transitions(TransitionIndex).Time <= 0.0f )
	{
		// See matching hack below for explaination.
		INT Tag=TransitionTag;
			SendTriggers( 1.0f );
		if (Tag!=TransitionTag)
			return;

		GotoState_None();
		return;
	}

	SetState( Transitions(TransitionIndex).Start );

	Global_GetState()->SetEnabled(false);
	GetState()->SetEnabled(true);

	// Initialize the tween state.
	// Fix ARL: This should be a component interface so subclasses can override like Tween -- that way they could initialize their own tween properties (like FrameSize - currently being set every frame due to lack of initialization).
	if( bTweenLocation ) GetState()->SetLocation( Global_GetState()->GetLocation() );
	if( bTweenSize     ) GetState()->SetSize    ( Global_GetState()->GetSize()     );
	if( bTweenColor    ) GetState()->SetColor   ( Global_GetState()->GetColor()    );
	if( bTweenAlpha    ) GetState()->SetAlpha   ( Global_GetState()->GetAlpha()    );

	bTweenReverseEaseOverride = Transitions(TransitionIndex).bReverseEase;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::Transitioning_EndState()
{
	guard(UMultiStateComponent::Transitioning_EndState);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Transitioning_EndState[%d]"), *GetFullName(),TransitionIndex));

	GetState()->SetEnabled(false);

	SetState( Transitions(TransitionIndex).End );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent* UMultiStateComponent::Transitioning_GetState()
{
	guard(UMultiStateComponent::Transitioning_GetState);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Transitioning_GetState"), *GetFullName()));

	return Transitions(TransitionIndex).Template;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::Transitioning_HandleEvent( uiEvent Event )
{
	guard(UMultiStateComponent::Transitioning_HandleEvent);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Transitioning_HandleEvent( %s )"), *GetFullName(), *LookupEventName(Event)));

	if( !Transitions(TransitionIndex).bNoOverride )
		Global_HandleEvent( Event );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::Transitioning_Update(UPDATE_DEF)
{
	guard(UMultiStateComponent::Transitioning_Update);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Transitioning_Update[%d]: TransitionTime=%f "), *GetFullName(), TransitionIndex, TransitionTime));

	if( TransitionTime > Transitions(TransitionIndex).Time )
		TransitionTime = Transitions(TransitionIndex).Time;

	FLOAT Pct = TransitionTime / Transitions(TransitionIndex).Time;

	// Allow the Component to do any special tween handling.
	GetState()->SetTween( Pct );

	// Update tween vars.
	GetState()->Tween
		(	this
		,	States( Transitions(TransitionIndex).Start )
		,	States( Transitions(TransitionIndex).End )
		,	Pct
		);

	// Draw the Component.
	Global_Update(UPDATE_PARMS);

	// Fix ARL: Here's a good example of where a Microsoft-style Result return value would come in handy.  Have it normally return SUCCESS, but sometimes return CHANGED_STATE, and abort accordingly.
	INT Tag=TransitionTag;
		SendTriggers( Pct );	// NOTE[aleiby]: Theoretically, this should cause problems if it causes us to change state.  However, in practice everything I've tried to account for that seems to break it, so I'll leave it as it for now since it seems to work fine.
	if (Tag!=TransitionTag)		// NOTE[aleiby]: In an attempt to fix the above issue, I have a tag which lets me know if we've triggered any transitions... if so, abort early so as not to screw things up further.
		return;

	if( TransitionTime >= Transitions(TransitionIndex).Time )
		GotoState_None();

	// These need to always be updated last since other code uses them.
	LastPct = Pct;
	TransitionTime += GRealDeltaTime;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::SendTriggers( FLOAT Time )
{
	guard(UMultiStateComponent::SendTriggers);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SendTriggers( %f ) -- TransitionIndex=%d LastPct=%f "), *GetFullName(),Time,TransitionIndex,LastPct));

	UBOOL bSentEvent=false;

	FSTRUCT_Transition &Transition = Transitions(TransitionIndex);
	for( INT i=0; i<Transition.Triggers.Num(); i++ )
	{
		// Fix ARL: We should ensure that if multiple triggers get fired that
		// they get sent in correct chronological order.
		if
		(	 Transition.Triggers(i).Event
		&&   Transition.Triggers(i).Time > LastPct
		&&	 Transition.Triggers(i).Time <= Time
		)
		{
			NOTE(debugf(TEXT("[%s] Sent TriggerEvent(%d|%d) at Time: %f"), *GetName(), TransitionIndex, i, Time ));
			Transition.Triggers(i).Event->OnEvent();
			bSentEvent=true;
		}
	}

	return bSentEvent;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////////////////////
// Component interface redirects //
///////////////////////////////////

//------------------------------------------------------------------------------
void UMultiStateComponent::SetSizeI( const FDimension& D )
{
	guard(UMultiStateComponent::SetSizeI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SetSizeI( %s )"), *GetFullName(), D.String()));

	GetState()->SetSize(D);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FDimension UMultiStateComponent::GetSizeI()
{
	guard(UMultiStateComponent::GetSizeI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::GetSizeI"), *GetFullName()));

	return GetState()->GetSize();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
ERenderStyle UMultiStateComponent::GetStyleI()
{
	guard(UMultiStateComponent::GetStyleI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::GetStyleI"), *GetFullName()));

	return GetState()->GetStyle();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::SetStyleI( ERenderStyle S )
{
	guard(UMultiStateComponent::SetStyleI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SetStyleI( %s )"), *GetFullName(), GetEnumEx(Engine.ERenderStyle,S,4)));

	GetState()->SetStyle(S);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FColor UMultiStateComponent::GetColorI()
{
	guard(UMultiStateComponent::GetColorI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::GetColorI"), *GetFullName()));

	return GetState()->GetColor();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::SetColorI( const FColor& C )
{
	guard(UMultiStateComponent::SetColorI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SetColorI( %s )"), *GetFullName(), C.String()));

	GetState()->SetColor(C);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle UMultiStateComponent::GetBoundsI()
{
	guard(UMultiStateComponent::GetBoundsI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::GetBoundsI"), *GetFullName()));

	return GetState()->GetBounds() + GetLocation();		// NOTE[aleiby]: We need to add our location because SetLocation takes effect on the MultiStateComponent instead of passing it off to the State.  We may want to make it pass SetLocation/GetLocation on to the current state, but that means that we can't move the entire MultiStateComponent as a unit (we'd have to set location on each of the individual states).

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle UMultiStateComponent::GetScreenBoundsI()
{
	guard(UMultiStateComponent::GetScreenBoundsI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::GetScreenBoundsI"), *GetFullName()));

	return GetState()->GetScreenBounds();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent* UMultiStateComponent::GetComponentAtI( const FPoint& P )
{
	guard(UMultiStateComponent::GetComponentAtI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::GetComponentAtI( %s )"), *GetFullName(), P.String()));

	return GetState()->GetComponentAt(P);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void  UMultiStateComponent::SetVisibilityI( UBOOL bVisible )
{
	guard(UMultiStateComponent::SetVisibilityI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SetVisibilityI( %s )"), *GetFullName(), bVisible ? TEXT("True") : TEXT("False")));

	GetState()->SetVisibility(bVisible);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::IsVisibleI()
{
	guard(UMultiStateComponent::IsVisibleI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::IsVisibleI"), *GetFullName()));

	return GetState()->IsVisible();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::IsShowingI()
{
	guard(UMultiStateComponent::IsShowingI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::IsShowingI"), *GetFullName()));

	return GetState()->IsShowing();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::IsDisplayableI()
{
	guard(UMultiStateComponent::IsDisplayableI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::IsDisplayableI"), *GetFullName()));

	return GetState()->IsDisplayable();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::SetEnabledI( UBOOL bEnabled )
{
	guard(UMultiStateComponent::SetEnabledI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SetEnabledI( %s )"), *GetFullName(), bEnabled ? TEXT("True") : TEXT("False")));

	GetState()->SetEnabled(bEnabled);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::IsEnabledI()
{
	guard(UMultiStateComponent::IsEnabledI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::IsEnabledI"), *GetFullName()));

	return GetState()->IsEnabled();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::ValidateI()
{
	guard(UMultiStateComponent::ValidateI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::ValidateI"), *GetFullName()));

	GetState()->Validate();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::InvalidateI()
{
	guard(UMultiStateComponent::InvalidateI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::InvalidateI"), *GetFullName()));

	GetState()->Invalidate();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UMultiStateComponent::IsValidI()
{
	guard(UMultiStateComponent::IsValidI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::IsValidI"), *GetFullName()));

	return GetState()->IsValid();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent*	UMultiStateComponent::FindLooseComponentI( uiName Name )
{
	guard(UMultiStateComponent::FindLooseComponentI);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::FindLooseComponentI( %s )"), *GetFullName(), **Name ));

	UComponent* C=GetState()->FindLooseComponent(Name);	// prefer current state.
	if(!C) C=Super::FindLooseComponentI(Name);
	return C;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::Tick( FLOAT DeltaSeconds )
{
	guard(UMultiStateComponent::Tick);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::Tick( %f )"), *GetFullName(), DeltaSeconds));

	GetState()->Tick(DeltaSeconds);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::Global_Update(UPDATE_DEF)
{
	guard(UMultiStateComponent::Global_Update);
	NOTE0(debugf(TEXT("(%s)UMultiStateComponent::Global_Update( %s )"), *GetFullName(), Canvas ? Canvas->GetFullName() : TEXT("None")));

	GetState()->Update(UPDATE_PARMS);
	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiStateComponent::SetTween( FLOAT Pct )
{
	guard(UMultiStateComponent::SetTween);
	NOTE(debugf(TEXT("(%s)UMultiStateComponent::SetTween( %f )"), *GetFullName(), Pct));

	GetState()->SetTween(Pct);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


