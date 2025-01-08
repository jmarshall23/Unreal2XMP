#include "UI.h"
#include "ImportUI.h"

#define NOTIFY_ALL 1
#define MouseToggle 0

#if MouseToggle
static UMouseCursor* StaticMouse=NULL;
EXECFUNC(ToggleMouse){ if( StaticMouse ) StaticMouse->SetEnabled( !StaticMouse->IsEnabled() ); }
#endif

FPoint UMouseCursor::MousePos=FPoint(250,250);


// Fix ARL: Instead of storing absolute mouse coordinates in Location, use a relative percentage within MinX/MaxX/MinY/MaxY?


///////////////////////
// UObject overrides //
///////////////////////

//------------------------------------------------------------------------------
void UMouseCursor::Constructed()
{
	guard(UMouseCursor::Constructed);
	NOTE(debugf(TEXT("UMouseCursor::Constructed")));

	Super::Constructed();
	MouseIndex = GetGMouseList().Num();
	GetGMouseList().AddUniqueItem(this);

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::Destroy()
{
	guard(UMouseCursor::Destroy);
	NOTE(debugf(TEXT("UMouseCursor::Destroy")));

	UUIConsole* C = CastChecked<UUIConsole>(GetConsole());
	C->RemoveInputActionListener( IST_Axis, this, UI_NATIVE_CALLBACK(None,this,UMouseCursor,MouseMove_Event) );			// Fix ARL: Memory leak.
	C->RemoveInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UMouseCursor,MousePress_Event) );		// Fix ARL: Memory leak.
	C->RemoveInputActionListener( IST_Release, this, UI_NATIVE_CALLBACK(None,this,UMouseCursor,MouseRelease_Event) );	// Fix ARL: Memory leak.
	C->RemoveTickListener( this );

	if( MouseActionListenerTable )
	{
		delete MouseActionListenerTable;
		MouseActionListenerTable=NULL;
	}

	if( LastRelevantComponents )
	{
		delete LastRelevantComponents;
		LastRelevantComponents=NULL;
	}

	if( GetGMouse() == this )
		GetGMouse() = NULL;
	GetGMouseList()(MouseIndex) = NULL;

	Super::Destroy();

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::Serialize( FArchive &Ar )
{
	guard(UMouseCursor::Serialize);
	NOTE(debugf(TEXT("(%s)UMouseCursor::Serialize"), *GetFullName() ));

	Super::Serialize( Ar );
	
	Ar << MouseIndex;
	SERIALIZE_BOOL(bAutoConstrain)
	Ar << MinX << MaxX << MinY << MaxY;
	Ar << MoveTime << MoveTimer;
	// the remaining variables are all transient.

	// Fixup GMouseList.
	GetGMouseList().Reserve(MouseIndex+1);
	GetGMouseList()(MouseIndex)=this;

	Ar << *(DWORD*)&MouseActionListenerTable;
	if( MouseActionListenerTable )
	{
		if( Ar.IsLoading() )
			MouseActionListenerTable = new TMap<EMouseAction,NotifyList>();
		Ar << *(TMap<BYTE,NotifyList>*)MouseActionListenerTable;
	}

	if( LastRelevantComponents )
	{
		delete LastRelevantComponents;
		LastRelevantComponents = NULL;
	}

	unguard;
}


/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
void UMouseCursor::FindRelevantListeners()
{
	guard(UMouseCursor::FindRelevantListeners);
	NOTE(debugf(TEXT("(%s)UMouseCursor::FindRelevantListeners"), *GetFullName() ));

	if( !MouseActionListenerTable ) return;

	// Make sure we don't get called more than once per tick.
	if( LastRelevancyUpdateTime>=TimeStamp )
		return;
	LastRelevancyUpdateTime=TimeStamp;

	INT i;
	// Fix ARL: What about non-component listeners?
	TArray<UComponent*>  ListeningComponents;	// All unique components that are listening for mouse events.
	TArray<UComponent*>* RelevantComponents		// All ListeningComponents that the mouse is currently over.
		= new TArray<UComponent*>();

	// Build a list of listening components that the mouse is currently over.
	for( TMap<EMouseAction,NotifyList>::TIterator It(*MouseActionListenerTable); It; ++It )
	{
#if NOTIFY_ALL 
		if( !(It.Key()==MA_Enter || It.Key()==MA_Exit) ) continue;	// No need to worry about other event types since these are the only ones that care about relevency.
#endif
		NotifyList NotifyInfo=It.Value();
		for( i=0; i<NotifyInfo->Num(); i++ )
		{
			UComponent* C = (*NotifyInfo)(i)->GetTarget();
			ListeningComponents.AddUniqueItem(C);
		}
	}
	for( i=0; i<ListeningComponents.Num(); i++ )
	{
		if( ListeningComponents(i)->IsEnabled() && ListeningComponents(i)->ContainsAbs( GetScreenCoords() ) )
		{
			RelevantComponents->AddItem( ListeningComponents(i) );
		}
	}

	NOTE(if(RelevantComponents) for( i=0; i<RelevantComponents->Num(); i++ ) debugf(TEXT("RelevantComponents(%d): %s"), i, *(*RelevantComponents)(i)->GetFullName() ));
	NOTE(if(LastRelevantComponents) for( i=0; i<LastRelevantComponents->Num(); i++ ) debugf(TEXT("LastRelevantComponents(%d): %s"), i, *(*LastRelevantComponents)(i)->GetFullName() ));

	// Filter out new and old components.
	NewComponents.Empty();
	OldComponents.Empty();
	for( i=0; i<ListeningComponents.Num(); i++ )
	{
		UComponent* C = ListeningComponents(i);

		UBOOL bIsRelevant = RelevantComponents->FindItemIndex(C)!=INDEX_NONE;
		UBOOL bWasRelevant = LastRelevantComponents ? LastRelevantComponents->FindItemIndex(C)!=INDEX_NONE : false;

		if( bIsRelevant && !bWasRelevant )
		{
			NOTE(debugf(TEXT("Adding %s to NewComponents list."), *C->GetFullName() ));
			NewComponents.AddItem(C);
		}
		else if( !bIsRelevant && bWasRelevant )
		{
			NOTE(debugf(TEXT("Adding %s to OldComponents list."), *C->GetFullName() ));
			OldComponents.AddItem(C);
		}
	}

	// Keep the list of RelevantComponents around so we have something to check against next tick.
	if( LastRelevantComponents )
		delete LastRelevantComponents;	// Fix ARL: Should we be recycling these instead?
	LastRelevantComponents = RelevantComponents;

	unguard;
}


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UMouseCursor::Tick( float DeltaTime )
{
	guard(UMouseCursor::Tick);
	NOTE(debugf(TEXT("(%s)UMouseCursor::Tick( %f )"), *GetFullName(), DeltaTime ));

	Super::Tick( DeltaTime );

	if( !IsEnabled() || !MouseActionListenerTable ) return;
	NOTE(debugf(TEXT("(%s)UMouseCursor::Tick"),*GetFullName()));

	MouseNotifyParms Parms;
	Parms.Mouse = this;
	Parms.bHandled = false;

	TimeStamp += DeltaTime;
	FindRelevantListeners();

#if 0 // Fix ARL: This could be a lot faster if each Component had flags telling which events they were listening for -- instead of having to check which lists they're in.

	for( INT i=0; i<NewComponent.Num(); i++ )
		if( NewComponents(i).EventFlags & MA_Enter )
			NewComponents(i).SendMouseEnterNotification();

	for( i=0; i<OldComponent.Num(); i++ )
		if( OldComponents(i).EventFlags & MA_Exit )
			OldComponents(i).SendMouseExitNotification();
#else

	if( NewComponents.Num() )
	{
		NotifyList Listeners = MouseActionListenerTable->FindRef( MA_Enter );
		if( Listeners )
			for( INT i=0; i<Listeners->Num(); i++ )
				if( NewComponents.FindItemIndex( (*Listeners)(i)->GetTarget() )!=INDEX_NONE )
					{ (*Listeners)(i)->SendNotification( &Parms ); /*if( Parms.bHandled ) break;*/ }
	}

	if( OldComponents.Num() )
	{
		NotifyList Listeners = MouseActionListenerTable->FindRef( MA_Exit );
		if( Listeners )
			for( INT i=0; i<Listeners->Num(); i++ )
				if( OldComponents.FindItemIndex( (*Listeners)(i)->GetTarget() )!=INDEX_NONE )
					{ (*Listeners)(i)->SendNotification( &Parms ); /*if( Parms.bHandled ) break;*/ }
	}

#endif

	if( MoveTimer>0.f )
	{
		MoveTimer -= DeltaTime;
		if( MoveTimer<=0.f )
			SendEvent(UI_EVENT(Off));
	}

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::DispatchMousePress()
{
	guard(UMouseCursor::DispatchMousePress);
	NOTE(debugf(TEXT("(%s)UMouseCursor::DispatchMousePress"), *GetFullName() ));

	if( !IsEnabled() || !MouseActionListenerTable ) return;

#if NOTIFY_ALL // Send events to all enabled listening components until the event is handled.

	MouseNotifyParms Parms;
	Parms.Mouse = this;
	Parms.bHandled = false;

	NotifyList Listeners = MouseActionListenerTable->FindRef( MA_Press );
#if 1 //CLEAN
	if( Listeners )
	{
		Listeners->Sort();
		for( INT i=0; i<Listeners->Num(); i++ )
			if( (*Listeners)(i)->GetTarget()->IsEnabled() )
				{ (*Listeners)(i)->SendNotification( &Parms ); if( Parms.bHandled ) break; }
	}
#else
	if( Listeners )
	{
		Listeners->Sort();
		NOTE(for( INT j=0; j<Listeners->Num(); j++ ) debugf(TEXT("Listeners(%d|%f)=%s"),j,(*Listeners)(j)->GetTarget()->GetAbsDrawOrder(),(*Listeners)(j)->GetTarget()->GetFullName()));
		for( INT i=0; i<Listeners->Num(); i++ )
		{
			if( (*Listeners)(i)->GetTarget()->IsEnabled() )
			{
				NOTE(debugf(TEXT("DispatchMousePress: %s"), *(*Listeners)(i)->GetTarget()->GetFullName() ));
				(*Listeners)(i)->SendNotification( &Parms );
				if( Parms.bHandled ) break;
			}
			NOTE(else debugf(TEXT("DispatchMousePress: Component not enabled (%s)"), (*Listeners)(i)->GetTarget()->GetFullName() ));
		}
		NOTE(while( ++i<Listeners->Num() ) debugf(TEXT("DispatchMousePress: No event sent (%s)"), (*Listeners)(i)->GetTarget()->GetFullName() ));
	}
#endif

#else // Send events only to relevant components.

	FindRelevantListeners();

	if( LastRelevantComponents && LastRelevantComponents->Num() )
	{
		NotifyList Listeners = MouseActionListenerTable->FindRef( MA_Press );
		if( Listeners )
			for( INT i=0; i<Listeners->Num(); i++ )
				if( LastRelevantComponents->FindItemIndex( (*Listeners)(i).GetTarget() )!=INDEX_NONE )
					{ (*Listeners)(i).SendNotification(); if( Parms.bHandled ) break; }
	}

#endif

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::DispatchMouseRelease()
{
	guard(UMouseCursor::DispatchMouseRelease);
	NOTE(debugf(TEXT("(%s)UMouseCursor::DispatchMouseRelease"), *GetFullName() ));

	if( !IsEnabled() || !MouseActionListenerTable ) return;

#if NOTIFY_ALL // Send events to all enabled listening components until the event is handled.

	MouseNotifyParms Parms;
	Parms.Mouse = this;
	Parms.bHandled = false;

	NotifyList Listeners = MouseActionListenerTable->FindRef( MA_Release );
#if 1 //CLEAN
	if( Listeners )
	{
		Listeners->Sort();
		for( INT i=0; i<Listeners->Num(); i++ )
			if( (*Listeners)(i)->GetTarget()->IsEnabled() )
				{ (*Listeners)(i)->SendNotification( &Parms ); if( Parms.bHandled ) break; }
	}
#else
	if( Listeners )
	{
		Listeners->Sort();
		NOTE(for( INT j=0; j<Listeners->Num(); j++ ) debugf(TEXT("Listeners(%d|%f)=%s"),j,(*Listeners)(j)->GetTarget()->GetAbsDrawOrder(),(*Listeners)(j)->GetTarget()->GetFullName()));
		for( INT i=0; i<Listeners->Num(); i++ )
		{
			if( (*Listeners)(i)->GetTarget()->IsEnabled() )
			{
				NOTE(debugf(TEXT("DispatchMouseRelease: %s"), (*Listeners)(i)->GetTarget()->GetFullName() ));
				(*Listeners)(i)->SendNotification( &Parms );
				if( Parms.bHandled ) break;
			}
			NOTE(else debugf(TEXT("DispatchMouseRelease: Component not enabled (%s)"), (*Listeners)(i)->GetTarget()->GetFullName() ));
		}
		NOTE(while( ++i<Listeners->Num() ) debugf(TEXT("DispatchMouseRelease: No event sent (%s)"), (*Listeners)(i)->GetTarget()->GetFullName() ));
	}
#endif

#else // Send events only to relevant components.

	FindRelevantListeners();

	if( LastRelevantComponents && LastRelevantComponents->Num() )
	{
		NotifyList Listeners = MouseActionListenerTable->FindRef( MA_Release );
		if( Listeners )
			for( INT i=0; i<Listeners->Num(); i++ )
				if( LastRelevantComponents->FindItemIndex( (*Listeners)(i).GetTarget() )!=INDEX_NONE )
					{ (*Listeners)(i).SendNotification(); if( Parms.bHandled ) break; }
	}

#endif

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::AddMouseActionListener( EMouseAction Action, UComponent* C, ActionEvent* NotifyObj )
{
	guard(UMouseCursor::AddMouseActionListener);
	NOTE(debugf(TEXT("(%s)UMouseCursor::AddMouseActionListener( %s, %s )"), *GetFullName(), GetEnumEx(UI.EMouseAction,Action,3), C ? *C->GetFullName() : TEXT("NULL") ));

	if( !MouseActionListenerTable )
		MouseActionListenerTable = new TMap<EMouseAction,NotifyList>();

	NotifyList ActionListeners = MouseActionListenerTable->FindRef( Action );

	if( !ActionListeners )
	{
		ActionListeners = new NotifyListType();
		MouseActionListenerTable->Set( Action, ActionListeners );
	}

#if 1 //NEW
	ActionListeners->AddItem( new NotifyTypeClass(C,NotifyObj) );		// Fix ARL: Memory leak.
#else
	new(*ActionListeners)NotifyType(new NotifyTypeClass(C,NotifyObj));	// Fix ARL: Memory leak.
	ActionListeners->Sort();	// Fix ARL: This would be faster if we used Percolate(ActionListeners->Num()), but Percolate is private.
#endif

	NOTE(debugf(TEXT("(AddMouseActionListener) MouseActionListenerTable: %d ActionListeners: %d Index: %d Action: %s Component: %s"), (DWORD)MouseActionListenerTable, (DWORD)ActionListeners, ActionListeners->Num(), GetEnumEx(UI.EMouseAction,Action,3), C ? *C->GetFullName() : TEXT("NULL") ));
	NOTE(if(Action==MA_Release) for( INT i=0; i<ActionListeners->Num(); i++ ) debugf(TEXT("ActionListeners(%d|%f)=%s"),i,(*ActionListeners)(i)->GetTarget()->GetAbsDrawOrder(),(*ActionListeners)(i)->GetTarget()->GetFullName()));

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::RemoveMouseActionListener( EMouseAction Action, UComponent* C, ActionEvent* NotifyObj )
{
	guard(UMouseCursor::RemoveMouseActionListener);
	NOTE(debugf(TEXT("(%s)UMouseCursor::RemoveMouseActionListener"), *GetFullName() ));

	NotifyTypeClass NotifyInfo = NotifyTypeClass(C,NotifyObj);

	if( MouseActionListenerTable )
	{
		NotifyList ActionListeners = MouseActionListenerTable->FindRef( Action );
		if( ActionListeners )
		{
			for( INT i=0; i<ActionListeners->Num(); /*below*/ )
			{
				if( *(*ActionListeners)(i) == NotifyInfo )
					ActionListeners->Remove(i);
				else i++;
			}
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::RemoveAllMouseActionListeners( EMouseAction Action, UComponent* C )
{
	guard(UMouseCursor::RemoveAllMouseActionListeners);
	NOTE(debugf(TEXT("(%s)UMouseCursor::RemoveAllMouseActionListeners"), *GetFullName() ));

	if( MouseActionListenerTable )
	{
		NotifyList ActionListeners = MouseActionListenerTable->FindRef( Action );
		if( ActionListeners )
		{
			for( INT i=0; i<ActionListeners->Num(); /*below*/ )
			{
				if( (*ActionListeners)(i)->GetTarget() == C )
					ActionListeners->Remove(i);
				else i++;
			}
		}
	}

	unguard;
}


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UMouseCursor::SetConsole( UUIConsole* C )
{
	guard(UMouseCursor::SetConsole);
	NOTE(debugf(TEXT("(%s)UMouseCursor::SetConsole"), *GetFullName() ));

	C->AddInputActionListener( IST_Axis, this, UI_NATIVE_CALLBACK(None,this,UMouseCursor,MouseMove_Event) );			// Fix ARL: Memory leak.
	C->AddInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UMouseCursor,MousePress_Event) );			// Fix ARL: Memory leak.
	C->AddInputActionListener( IST_Release, this, UI_NATIVE_CALLBACK(None,this,UMouseCursor,MouseRelease_Event) );		// Fix ARL: Memory leak.
	C->AddTickListener( this );

	if( bAutoConstrain )
	{
		MinX = 0;			MinY = 0;
		MaxX = C->FrameX;	MaxY = C->FrameY;
		NOTE(debugf(TEXT("%s) Auto-constraining to %dx%d"),*GetFullName(),(INT)MaxX,(INT)MaxY));
	}

#if MouseToggle
	StaticMouse = this;	// for grabbing notifications above.
#endif

	unguard;
}

//------------------------------------------------------------------------------
INT UMouseCursor::GetMouseIndex()
{
	guard(UMouseCursor::GetMouseIndex);
	NOTE(debugf(TEXT("UMouseCursor::GetMouseIndex")));
	check(GetGMouseList()(MouseIndex)==this);
	return MouseIndex;
	unguard;
}


/////////////////////////
// Component overrides //
/////////////////////////

//------------------------------------------------------------------------------
FPoint UMouseCursor::GetScreenCoordsI()
{
	guard(UMouseCursor::GetScreenCoordsI);
	NOTE(debugf(TEXT("(%s)UMouseCursor::GetScreenCoordsI"), *GetFullName() ));

	UConsole* C = GetConsole();
	UViewport* V = C->Viewport;

	if( bAutoConstrain )	// Fix ARL: Move this into a separate function.
	{
		MinX = 0;			MinY = 0;
		MaxX = C->FrameX;	MaxY = C->FrameY;
	}

	if( V->bWindowsMouseAvailable )
		MousePos = FPoint(V->WindowsMouseX,V->WindowsMouseY);

	return MousePos;

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::Update(UPDATE_DEF)
{
	guard(UMouseCursor::Update);
	NOTE(debugf(TEXT("(%s)UMouseCursor::Update"), *GetFullName() ));

	if( IsEnabled() )
	{
		Super::Update(UPDATE_PARMS);
#if MOUSECURSOR_PARTICLESYSTEMS
		if( MouseTrail )
			MouseTrail->SetLocation( CalcMouseTrailLocation() );
#endif
	}

	unguard;
}

//------------------------------------------------------------------------------
UComponent* UMouseCursor::GetComponentAtI( const FPoint& P )
{
	guard(UMouseCursor::GetComponentAtI);
	NOTE(debugf(TEXT("(%s)UMouseCursor::GetComponentAtI( %s )"), *GetFullName(), P.String()));

	// Mice are exempt from selection -- makes editing a headache otherwise.
	return NULL;

	unguard;
}


/////////////////
// MouseEvents //
/////////////////

//------------------------------------------------------------------------------
void UMouseCursor::MouseMove_Event( void* Parms )
{
	guard(UMouseCursor::MouseMove_Event);
	NOTE(debugf(TEXT("(%s)UMouseCursor::MouseMove_Event"), *GetFullName() ));

	KeyEventParms* P = (KeyEventParms*)Parms;

	EInputKey		Key		= P->Key;
//	EInputAction	Action	= P->Action;
	FLOAT			Delta	= P->Delta;

	NOTE(debugf(TEXT("%f - %s) Key: %s Delta: %f"), GetTimeSeconds(), *GetFullName(), GetEnumEx(Engine.EInputKey,Key,3), Delta));
	NOTE(debugf(TEXT("%f %f %f %f"),MinX,MaxX,MinY,MaxY));

	if( IsEnabled() )
	{
		switch( Key )
		{
		case IK_MouseX:
			MouseAction();
			MousePos.X += GetClient()->MouseSpeed * Delta;
			MousePos.X = Clamp( MousePos.X, MinX, MaxX );
			P->bHandled = true; return;
		case IK_MouseY:
			MouseAction();
			MousePos.Y -= GetClient()->MouseSpeed * Delta;
			MousePos.Y = Clamp( MousePos.Y, MinY, MaxY );
			P->bHandled = true; return;
		}
	}

	P->bHandled = false; return;

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::MousePress_Event( void* Parms )
{
	guard(UMouseCursor::MousePress_Event);
	NOTE(debugf(TEXT("(%s)UMouseCursor::MousePress_Event"), *GetFullName() ));

	KeyEventParms* P = (KeyEventParms*)Parms;

	EInputKey		Key		= P->Key;
//	EInputAction	Action	= P->Action;
//	FLOAT			Delta	= P->Delta;

	if( IsEnabled() )
	{
		switch( Key )
		{
		case IK_LeftMouse:
			MouseAction();
			DispatchMousePress();		// Fix ARL: How should we handle other mouse buttons?
			P->bHandled = true; return;
		case IK_RightMouse:
			MouseAction();
			P->bHandled = true; return;
		case IK_MiddleMouse:
			MouseAction();
			P->bHandled = true; return;
		}
	}

	P->bHandled = false; return;

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::MouseRelease_Event( void* Parms )
{
	guard(UMouseCursor::MouseRelease_Event);
	NOTE(debugf(TEXT("(%s)UMouseCursor::MouseRelease_Event"), *GetFullName() ));

	KeyEventParms* P = (KeyEventParms*)Parms;

	EInputKey		Key		= P->Key;
//	EInputAction	Action	= P->Action;
//	FLOAT			Delta	= P->Delta;

	if( IsEnabled() )
	{
		switch( Key )
		{
		case IK_LeftMouse:
			MouseAction();
			DispatchMouseRelease();		// Fix ARL: How should we handle other mouse buttons?
			P->bHandled = true; return;
		case IK_RightMouse:
			MouseAction();
			P->bHandled = true; return;
		case IK_MiddleMouse:
			MouseAction();
			P->bHandled = true; return;
		}
	}

	P->bHandled = false; return;

	unguard;
}

//------------------------------------------------------------------------------
void UMouseCursor::MouseAction()
{
	guard(UMouseCursor::MouseAction);
	NOTE(debugf(TEXT("(%s)UMouseCursor::MouseAction"), *GetFullName() ));

	if( MoveTime>0.f )
	{
		if( MoveTimer<=0.f )
			SendEvent(UI_EVENT(On));

		MoveTimer = MoveTime;
	}

	unguard;
}


#if MOUSECURSOR_PARTICLESYSTEMS

////////////////////////////
// ParticleSystem support //
////////////////////////////

//------------------------------------------------------------------------------
void UMouseCursor::SetEnabledI( UBOOL bEnabled )
{
	guard(UMouseCursor::SetEnabledI);
	NOTE(debugf(TEXT("(%s)UMouseCursor::SetEnabledI"), *GetFullName() ));

	Super::SetEnabledI(bEnabled);

	if( IsEnabled() )
	{
		if( !MouseTrail )
		{
			if( MouseTrailName!=TEXT("") )
				MouseTrail = class'ParticleGenerator'.static.CreateNew( GetConsole()->Viewport->Actor, class'ParticleSalamander', MouseTrailName, CalcMouseTrailLocation() );
		}
		else
		{
			MouseTrail->SetLocation( CalcMouseTrailLocation() );
			MouseTrail->bOn = true;
		}
	}
	else
	{
		MouseTrail->bOn = false;
	}

	unguard;
}

//------------------------------------------------------------------------------
FVector UMouseCursor::CalcMouseTrailLocation()
{
	guard(UMouseCursor::CalcMouseTrailLocation);
	NOTE(debugf(TEXT("(%s)UMouseCursor::CalcMouseTrailLocation"), *GetFullName() ));

	// Fix ARL: Unhardcode (account for FOV, screen size, etc.)
	FPoint S = GetScreenCoords();
	APlayerPawn* P = GetConsole().Viewport.Actor;
	FVector Loc = FVector( 64, 0.126*(S.X-512), -0.126*(S.Y-384) );
	Loc = Loc >> P->ViewRotation;
	Loc += P->Location + FVector(0,0,P->BaseEyeHeight);
	return Loc;

	unguard;
}

#endif



