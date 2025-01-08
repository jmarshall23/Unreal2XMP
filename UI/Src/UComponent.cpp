#include "UI.h"
#include "ImportUI.h"


///////////////////////////
// Static initialization //
///////////////////////////

THashTable< FString, UTexture*, UI_TEXTURE_HASH_SIZE > UComponent::GTextureCache=THashTable< FString, UTexture*, UI_TEXTURE_HASH_SIZE >();


/////////////////////
// Basic Interface //
/////////////////////

//------------------------------------------------------------------------------
void			UComponent::SetLocation			( const FPoint& P		)
{
	guard(UComponent::SetLocation);
	NOTE(debugf(TEXT("(%s)UComponent::SetLocation( %s )"), *GetFullName(), *P.String() ));
	if(!Locked(LOCK_Location))
		SetLocationI(P);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FPoint			UComponent::GetLocation			( void					)
{
	guard(UComponent::GetLocation);
	NOTE(debugf(TEXT("(%s)UComponent::GetLocation"), *GetFullName() ));

	IMPLEMENT_ACCESSOR(Location,FPoint)
	IMPLEMENT_ACCESSOR_C(X,FLOAT,Y)
	IMPLEMENT_ACCESSOR_C(Y,FLOAT,X)

	return GetLocationI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FPoint			UComponent::GetScreenCoords		( void					)
{
	guard(UComponent::GetScreenCoords);
	NOTE(debugf(TEXT("(%s)UComponent::GetScreenCoords"), *GetFullName() ));
	return GetScreenCoordsI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FPoint			UComponent::GetAlignedLocation	( void					)
{
	guard(UComponent::GetAlignedLocation);
	NOTE(debugf(TEXT("(%s)UComponent::GetAlignedLocation"), *GetFullName() ));
	return GetAlignedLocationI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetSize				( const FDimension& D	)
{
	guard(UComponent::SetSize);
	NOTE(debugf(TEXT("(%s)UComponent::SetSize( %s )"), *GetFullName(), *D.String() ));
	if(!Locked(LOCK_Size))
		SetSizeI(D);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FDimension		UComponent::GetSize				( void					)
{
	guard(UComponent::GetSize);
	NOTE(debugf(TEXT("(%s)UComponent::GetSize"), *GetFullName() ));

	IMPLEMENT_ACCESSOR(Size,FDimension)
	IMPLEMENT_ACCESSOR_C(Width,FLOAT,Height)
	IMPLEMENT_ACCESSOR_C(Height,FLOAT,Width)

	return GetSizeI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetStyle			( ERenderStyle S		)
{
	guard(UComponent::SetStyle);
	NOTE(debugf(TEXT("(%s)UComponent::SetStyle( %s )"), *GetFullName(), GetEnumEx(Engine.ERenderStyle,S,4) ));
	if(!Locked(LOCK_Style))
		SetStyleI(S);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
ERenderStyle	UComponent::GetStyle			( void					)
{
	guard(UComponent::GetStyle);
	NOTE(debugf(TEXT("(%s)UComponent::GetStyle"), *GetFullName() ));
	return GetStyleI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetColor			( const FColor& C		)
{
	guard(UComponent::SetColor);
	NOTE(debugf(TEXT("(%s)UComponent::SetColor( %s )"), *GetFullName(), *C.String() ));
	if(!Locked(LOCK_Color))
		SetColorI(C);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FColor			UComponent::GetColor			( void					)
{
	guard(UComponent::GetColor);
	NOTE(debugf(TEXT("(%s)UComponent::GetColor"), *GetFullName() ));

	IMPLEMENT_ACCESSOR(Color,FColor)
	IMPLEMENT_ACCESSOR(Alpha,FLOAT)
	IMPLEMENT_ACCESSOR_C3(R,FLOAT,G,B)
	IMPLEMENT_ACCESSOR_C3(G,FLOAT,R,B)
	IMPLEMENT_ACCESSOR_C3(B,FLOAT,R,G)

	return GetColorI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetAlpha			( BYTE A				)
{
	guard(UComponent::SetAlpha);
	NOTE(debugf(TEXT("(%s)UComponent::SetAlpha( %i )"), *GetFullName(), A ));
	if(!Locked(LOCK_Alpha))
		SetAlphaI(A);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetAlphaPct			( FLOAT Pct				)
{
	guard(UComponent::SetAlphaPct);
	NOTE(debugf(TEXT("(%s)UComponent::SetAlphaPct( %f )"), *GetFullName(), Pct ));
	if(!Locked(LOCK_AlphaPct))
		SetAlphaPctI(Pct);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
BYTE			UComponent::GetAlpha			( void					)
{
	guard(UComponent::GetAlpha);
	NOTE(debugf(TEXT("(%s)UComponent::GetAlpha"), *GetFullName() ));

	IMPLEMENT_ACCESSOR(Alpha,FLOAT)

	return GetAlphaI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FRectangle		UComponent::GetBounds			( void					)
{
	guard(UComponent::GetBounds);
	NOTE(debugf(TEXT("(%s)UComponent::GetBounds"), *GetFullName() ));
	return GetBoundsI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FRectangle		UComponent::GetScreenBounds		( void					)
{
	guard(UComponent::GetScreenBounds);
	NOTE(debugf(TEXT("(%s)UComponent::GetScreenBounds"), *GetFullName() ));
	return GetScreenBoundsI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetAlignment		( const FAlignment& A	)
{
	guard(UComponent::SetAlignment);
	NOTE(debugf(TEXT("(%s)UComponent::SetAlignment( %s )"), *GetFullName(), *A.String() ));
	if(!Locked(LOCK_Alignment))
		SetAlignmentI(A);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FAlignment		UComponent::GetAlignment		( void					)
{
	guard(UComponent::GetAlignment);
	NOTE(debugf(TEXT("(%s)UComponent::GetAlignment"), *GetFullName() ));
	return GetAlignmentI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::Contains			( const FPoint& P		)
{
	guard(UComponent::Contains);
	NOTE(debugf(TEXT("(%s)UComponent::Contains( %s )"), *GetFullName(), *P.String() ));
	return ContainsI(P);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::ContainsAbs			( const FPoint& P		)
{
	guard(UComponent::ContainsAbs);
	NOTE(debugf(TEXT("(%s)UComponent::ContainsAbs( %s )"), *GetFullName(), *P.String() ));
	return ContainsAbsI(P);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UComponent*		UComponent::GetComponentAt		( const FPoint& P		)
{
	guard(UComponent::GetComponentAt);
	NOTE(debugf(TEXT("(%s)UComponent::GetComponentAt( %s )"), *GetFullName(), *P.String() ));
	return GetComponentAtI(P);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
FLOAT			UComponent::GetAbsDrawOrder		( void					)
{
	guard(UComponent::GetAbsDrawOrder);
	NOTE(debugf(TEXT("(%s)UComponent::GetAbsDrawOrder"), *GetFullName() ));
	return GetAbsDrawOrderI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetParent			( class UContainer* P	)
{
	guard(UComponent::SetParent);
	NOTE(debugf(TEXT("(%s)UComponent::SetParent( %s )"), *GetFullName(), P ? *P->GetFullName() : TEXT("None") ));
	SetParentI(P);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetVisibility		( UBOOL bVisible		)
{
	guard(UComponent::SetVisibility);
	NOTE(debugf(TEXT("(%s)UComponent::SetVisibility( %s )"), *GetFullName(), bVisible ? TEXT("True") : TEXT("False") ));
	SetVisibilityI(bVisible);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::IsVisible			( void					)
{
	guard(UComponent::IsVisible);
	NOTE(debugf(TEXT("(%s)UComponent::IsVisible"), *GetFullName() ));
	return IsVisibleI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::IsShowing			( void					)
{
	guard(UComponent::IsShowing);
	NOTE(debugf(TEXT("(%s)UComponent::IsShowing"), *GetFullName() ));
	return IsShowingI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::IsDisplayable		( void					)
{
	guard(UComponent::IsDisplayable);
	NOTE(debugf(TEXT("(%s)UComponent::IsDisplayable"), *GetFullName() ));
	return IsDisplayableI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SetEnabled			( UBOOL bEnabled		)
{
	guard(UComponent::SetEnabled);
	NOTE(debugf(TEXT("(%s)UComponent::SetEnabled( %s )"), *GetFullName(), bEnabled ? TEXT("True") : TEXT("False") ));
	SetEnabledI(bEnabled);
	if(IsEnabled())
	{
		SIMPLE_ACCESSOR(OnEnabled)
	}
#if 0 //ARL This didn't fix what I thought it would, so I'm going to leave it out for now.
	else
	{
		// Fix ARL: Maybe just force a MouseRelease instead?
		// reset these, or we'll often send erroneous click notifications.
		bMouseClick_Over=false;
		bMouseClick_PressedWhileOver=false;
	}
#endif
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::IsEnabled			( void					)
{
	guard(UComponent::IsEnabled);
	NOTE(debugf(TEXT("(%s)UComponent::IsEnabled"), *GetFullName() ));
	UBOOL bEnabled = IsEnabledI();
	if( AccessFlags & ACCESSOR_Enabled && bEnabled )	//NOTE[aleiby]: I added the extra AccessFlags check here first, since most of the time that'll be false, and it'll save us doing two checks.  Also, by checking bEnabled outside of the IMPLEMENT_ACCESSOR block, we save ourself having to call any accessor function if we're already disabled.
		SIMPLE_ACCESSORX_NOCACHE(Enabled,&bEnabled)
	return bEnabled;
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::Validate			( void					)
{
	guard(UComponent::Validate);
	NOTE(debugf(TEXT("(%s)UComponent::Validate"), *GetFullName() ));
	ValidateI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::Invalidate			( void					)
{
	guard(UComponent::Invalidate);
	NOTE(debugf(TEXT("(%s)UComponent::Invalidate"), *GetFullName() ));
	InvalidateI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::IsValid				( void					)
{
	guard(UComponent::IsValid);
	NOTE(debugf(TEXT("(%s)UComponent::IsValid"), *GetFullName() ));
	return IsValidI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::BringToFront		( void					)
{
	guard(UComponent::BringToFront);
	NOTE(debugf(TEXT("(%s)UComponent::BringToFront"), *GetFullName() ));
	BringToFrontI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SendToBack			( void					)
{
	guard(UComponent::SendToBack);
	NOTE(debugf(TEXT("(%s)UComponent::SendToBack"), *GetFullName() ));
	SendToBackI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::BringForward		( INT NumLevels			)
{
	guard(UComponent::BringForward);
	NOTE(debugf(TEXT("(%s)UComponent::BringForward( %i )"), *GetFullName(), NumLevels ));
	BringForwardI(NumLevels);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::SendBack			( INT NumLevels			)
{
	guard(UComponent::SendBack);
	NOTE(debugf(TEXT("(%s)UComponent::SendBack( %i )"), *GetFullName(), NumLevels ));
	SendBackI(NumLevels);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::RequestFocus		( void					)
{
	guard(UComponent::RequestFocus);
	NOTE(debugf(TEXT("(%s)UComponent::RequestFocus"), *GetFullName() ));
	RequestFocusI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UBOOL			UComponent::HasFocus			( void					)
{
	guard(UComponent::HasFocus);
	NOTE(debugf(TEXT("(%s)UComponent::HasFocus"), *GetFullName() ));
	return HasFocusI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UComponent*		UComponent::GetComponent		( uiName Name			)
{
	guard(UComponent::GetComponent(name));
	NOTE(debugf(TEXT("(%s)UComponent::GetComponent( %s )"), *GetFullName(), **Name ));
	return GetComponentI(Name);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UComponent*		UComponent::FindLooseComponent	( uiName Name			)
{
	guard(UComponent::FindLooseComponent(name));
	NOTE(debugf(TEXT("(%s)UComponent::FindLooseComponent( %s )"), *GetFullName(), **Name ));
	return FindLooseComponentI(Name);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
void			UComponent::GetLooseComponents	( uiName Name, TArray<UComponent*> &Components )
{
	guard(UComponent::GetLooseComponents(string));
	NOTE(debugf(TEXT("(%s)UComponent::GetLooseComponents( %s )"), *GetFullName(), **Name ));
	GetLooseComponentsI(Name,Components);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
uiPath			UComponent::GetFullPath		( void						)
{
	guard(UComponent::GetFullPath);
	NOTE(debugf(TEXT("(%s)UComponent::GetFullPath"), *GetFullName() ));
	return GetFullPathI();
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
//------------------------------------------------------------------------------
UComponent*		UComponent::GetComponent	( uiPath Path				)
{
	guard(UComponent::GetComponent(path));
	NOTE(debugf(TEXT("(%s)UComponent::GetComponent( %s )"), *GetFullName(), *GetPathString(Path) ));

	// Error checking.
	if( Path.Num()==0 )
		return NULL;
	if( Path.Last()==uiName::None() )
		return NULL;

	return GetComponentI(Path);
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////////
// UObject overrides //
///////////////////////

// What's the point of typedefs if they refuse to carry across files?!
typedef TArray<UComponent*> EventComponents;
typedef TMap< uiEvent, EventComponents* > EventComponentsMap;

//------------------------------------------------------------------------------
void UComponent::Serialize( FArchive &Ar )
{
	guard(UComponent::Serialize);
	NOTE(debugf(TEXT("(%s)UComponent::Serialize"), *GetFullName() ));

	// Parent not serialized since that will be set when we are added to a container.
	Ar << Location << Align << DrawOrder << Name << Listeners << DisabledEvents;

	NOTE(debugf(TEXT("%s %s..."),Ar.IsLoading()?TEXT("Loading"):Ar.IsSaving()?TEXT("Saving"):TEXT("Neither saving nor loading"),**Name));
	
	// Senpai set through other's EventTable.
	if( Ar.IsLoading() )
		Senpai.Empty();

	Ar << Accessors << AccessFlags << LockFlags;

	// EventTable
	Ar << *(DWORD*)&EventTable;
	if( EventTable )
	{
		struct EventTableFixup { static void UI_CALLBACK Fixup( FArchive& Ar, void* _owner )
		{
			guard(EventTableFixup);

			UComponent* owner = (UComponent*)_owner;
			EventComponentsMap* &EventTable = owner->EventTable;

			// Fixup EventTable
			if( Ar.IsLoading() )
				EventTable = new EventComponentsMap();
			Ar << *EventTable;

			// Fixup Senpai
			if( Ar.IsLoading() )
				for( EventComponentsMap::TIterator It(*EventTable); It; ++It )
				{
					EventComponents& A = *It.Value();
					for( INT i=0; i<A.Num(); i++ )
						A(i)->Senpai.AddItem(owner);
				}

			unguard;

		}};	((FArchiveUI&)Ar).NeedsFixup(this,EventTableFixup::Fixup);
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FArchive& operator<<( FArchive& Ar, EventComponents* &A )
{
	guard(EventComponents<<);

	INT Size;
	if( Ar.IsSaving() )
		Size = A->Num();
	Ar << AR_INDEX(Size);
	if( Ar.IsLoading() )
	{
		A = new EventComponents();
		A->Empty(Size);
		A->Add(Size);
	}

	for( INT i=0; i<Size; i++ )
	{
		UComponent* &C = (*A)(i);
		uiPath Path;
		if( Ar.IsSaving() )
			Path = C->GetFullPath();
		Ar << Path;
		if( Ar.IsLoading() )
		{
			C = GetGConsole()->Root->GetComponent(Path);
			if(!C) appErrorf(TEXT("Component not found! (%s)"),*GetPathString(Path));
		}
	}

	return Ar;
	unguard;
}

//------------------------------------------------------------------------------
void UComponent::Destroy()
{
	guard(UComponent::Destroy);
	NOTE(debugf(TEXT("(%s)UComponent::Destroy"), *GetFullName() ));

	if( GetParent() )
	{
		GetParent()->RemoveComponent( this );
	}

	// Remove from all EventTables we are dependent upon.
	for( INT i=0; i<Senpai.Num(); i++ )
		Senpai(i)->RemoveEventListener(this);
	Senpai.Empty();

	Listeners.Empty();	// Fix ARL: If we add notifications to RemoveListener, then we'll need to use RemoveListener to remove items rather than Empty().

	if( EventTable )
	{
		for( TMap< uiEvent, TArray<UComponent*>* >::TIterator It(*EventTable); It; ++It )
		{
			TArray<UComponent*> &EventListeners = *It.Value();
			for( INT i=0; i<EventListeners.Num(); i++ )
				EventListeners(i)->Senpai.RemoveItem(this);
		}
		delete EventTable;
		EventTable=NULL;
	}

#define REMOVE_MOUSE_LISTENERS(type) \
	if( AccessFlags & (ACCESSOR_Mouse##type | ACCESSOR_MouseClick) ) \
		for( INT i=0; i<GetGMouseList().Num(); i++ )	/* we don't keep pointers to our mice so we'll go through all of them just to be safe. */ \
			if( GetGMouseList()(i) ) \
				GetGMouseList()(i)->RemoveAllMouseActionListeners(MA_##type, this);

	REMOVE_MOUSE_LISTENERS(Enter)
	REMOVE_MOUSE_LISTENERS(Exit)
	REMOVE_MOUSE_LISTENERS(Press)
	REMOVE_MOUSE_LISTENERS(Release)

#undef REMOVE_MOUSE_LISTENERS

	Reset();

/*!!arl -- crashing on RestartLevel in BR_01.un2
	guard(DeleteAccessors);
	for( INT i=0; i<Accessors.Num(); i++ )
		delete Accessors(i);	//!! this is bad since some accessors are shared - Selectors for instance.
	Accessors.Empty();
	unguard;
*/

#if 0 //DEBUG
	if( GetGConsole()->InputActionListenerTable )
	{
		for( TMap<EInputAction,NotifyList>::TIterator It(*GetGConsole()->InputActionListenerTable); It; ++It )
		{
			NotifyList ActionListeners = It.Value();
			if( ActionListeners )
			{
				for( INT i=0; i<ActionListeners->Num(); i++ )
				{
					if( (*ActionListeners)(i)->C == this )
						appErrorf(TEXT("Forgot to unregister action listeners!"));
				}
			}
		}
	}
#endif

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


////////////////////
// Access support //
////////////////////

//------------------------------------------------------------------------------
void UComponent::AddAccessor( ActionEvent* Accessor )
{
	guard(UComponent::AddAccessor);

	if( !Accessor || Accessor->Type==ACCESSOR_None ) return;

	if( AccessFlags & Accessor->Type )
	{
		INT i=GetAccessorIndex(Accessor->Type);
		// Insert at the head of the list (but after the first one).
		// NOTE[aleiby]: This will cause potentially weird behavior since they will all be executed
		// in the order they were added except for the first one which will always be executed last.
		// However, this keeps the pointer in the MouseNotification stuff from having to change all the time.
		// (See calls to AddMouseActionListener.)
		Accessor->NextAction = Accessors(i)->NextAction;
		Accessors(i)->NextAction = Accessor;
	}
	else
	{
		Accessors.AddItem(Accessor);
		AccessFlags |= Accessor->Type;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


//////////////////////
// Listener support //
//////////////////////

//------------------------------------------------------------------------------
void UComponent::AddListener( UObject* L )
{
	guard(UComponent::AddListener);
	NOTE(debugf(TEXT("(%s)UComponent::AddListener( %s )"), *GetFullName(), L ? L->GetFullName() : TEXT("NULL") ));

	if( L ) Listeners.AddItem( L );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UComponent::RemoveListener( UObject* L )
{
	guard(UComponent::RemoveListener);
	NOTE(debugf(TEXT("(%s)UComponent::RemoveListener( %s )"), *GetFullName(), L ? L->GetFullName() : TEXT("NULL") ));

	return Listeners.RemoveItem( L );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT UComponent::RemoveListenerType( FName Class, UBOOL bAndSubclasses )
{
	guard(UComponent::RemoveListenerType);
	NOTE(debugf(TEXT("(%s)UComponent::RemoveListenerType( %s, %d )"), *GetFullName(), *Class, bAndSubclasses ));

	INT NumListenersRemoved=0;

	for( INT i=0; i<Listeners.Num(); /*below*/ )
	{
		if( bAndSubclasses ? Listeners(i)->IsA(Class) : Listeners(i)->GetClass()->GetFName()==Class )
		{
			NumListenersRemoved += RemoveListener( Listeners(i) );
		}
		else i++;
	}

	return NumListenersRemoved;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SendNotification( FString FuncName )
{
	guard(UComponent::SendNotification);
	NOTE(debugf(TEXT("(%s)UComponent::SendNotification( %s )"), *GetFullName(), FuncName ));

	for( INT i=0; i<Listeners.Num(); i++ )
		FInterfaceNotifyInfo(Listeners(i),FuncName).SendNotification();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::RegisterEvent( uiEvent Event, UComponent* C )
{
	guard(UComponent::RegisterEvent);
	NOTE(debugf(TEXT("(%s)UComponent::RegisterEvent( %d, %s )"), *GetFullName(), Event, C ? *C->GetFullName() : TEXT("None") ));

	if( !EventTable )
		EventTable = new TMap< uiEvent, TArray<UComponent*>* >();

	TArray<UComponent*>* EventListeners = EventTable->FindRef( Event );

	if( !EventListeners )
	{
		EventListeners = new TArray<UComponent*>();
		EventTable->Set( Event, EventListeners );
	}
		
	EventListeners->AddItem( C );	// Fix ARL: Disallow duplicates?
	C->Senpai.AddItem(this);		// record dependence.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::RemoveEventListener( UComponent* C )
{
	guard(UComponent::RemoveEventListener);
	NOTE(debugf(TEXT("(%s)UComponent::RemoveEventListener( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("None") ));

	if( EventTable )
		for( TMap< uiEvent, TArray<UComponent*>* >::TIterator It(*EventTable); It; ++It )
			It.Value()->RemoveItem(C);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SendEvent( uiEvent Event )
{
	guard(UComponent::SendEvent);
	NOTE(debugf(TEXT("(%s)UComponent::SendEvent( %s )"), *GetFullName(), *LookupEventName(Event) ));

	if( !Locked(LOCK_SendEvent) && EventTable && Event!=NAME_None && IsEventEnabled( Event ) )
	{
		TArray<UComponent*>* EventListeners = EventTable->FindRef( Event );
		if( EventListeners )
		{
			TArray<UComponent*> &EventListenersRef = *EventListeners;
			for( INT i=0; i<EventListenersRef.Num(); i++ )
			{
				UComponent* C = DynamicCast<UComponent>(EventListenersRef(i));
				if(C) C->HandleEvent( Event );
			}
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::DisableEvent( uiEvent Event )
{
	guard(UComponent::DisableEvent);
	NOTE(debugf(TEXT("(%s)UComponent::DisableEvent( %s )"), *GetFullName(), *LookupEventName(Event)));
	DisabledEvents.AddItem( Event );
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::EnableEvent( uiEvent Event )
{
	guard(UComponent::EnableEvent);
	NOTE(debugf(TEXT("(%s)UComponent::EnableEvent( %s )"), *GetFullName(), *LookupEventName(Event)));
	DisabledEvents.RemoveItem( Event );
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UComponent::IsEventEnabled( uiEvent Event )
{
	guard(UComponent::IsEventEnabled);
	NOTE(debugf(TEXT("(%s)UComponent::IsEventEnabled"), *GetFullName()));

	for( INT i=0; i<DisabledEvents.Num(); i++ )
		if( DisabledEvents(i) == Event )
			return false;

	return true;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////////////
// Additional interfaces //
///////////////////////////

#if 0 //disabled because it slows everything down needlessly.
//------------------------------------------------------------------------------
void UComponent::Update(UPDATE_DEF)
{
	guard(UComponent::Update);
	NOTE(debugf(TEXT("(%s)UComponent::Update"), *GetFullName() ));

	if( bDebug )
	{
		FPoint P=GetScreenCoords();
		FDimension D=GetSize();
		FColor Color = FColor( *(DWORD*)this, *(DWORD*)this>>1, *(DWORD*)this>>2, 255 );	// generate object specific color.
		Frame->Viewport->Canvas->DrawRectangle( P.X, P.Y, D.Width, D.Height, Color );
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
#endif

//------------------------------------------------------------------------------
void UComponent::Reset()
{
	guard(UComponent::Reset);
	NOTE(debugf(TEXT("(%s)UComponent::Reset"), *GetFullName() ));

	for( INT i=0; i<Accessors.Num(); i++ )
		Accessors(i)->Reset();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

/////////////////////////////
// Default implementations //
/////////////////////////////

//------------------------------------------------------------------------------
void UComponent::SetLocationI( const FPoint& P )
{
	guard(UComponent::SetLocationI);
	NOTE(debugf(TEXT("(%s)UComponent::SetLocationI( %s )"), *GetFullName(), P.String()));
	
	Location = P;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FPoint UComponent::GetLocationI()
{
	guard(UComponent::GetLocationI);
	NOTE(debugf(TEXT("(%s)UComponent::GetLocationI"), *GetFullName()));
	
	return Location;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FPoint UComponent::GetScreenCoordsI()
{
	guard(UComponent::GetScreenCoordsI);
	NOTE(debugf(TEXT("(%s)UComponent::GetScreenCoordsI"), *GetFullName()));
	
	if(Parent)	return Parent->GetScreenCoords() + GetAlignedLocation();
	else		return GetAlignedLocation();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

// Fix ARL: Can this be unvirtualized?

//------------------------------------------------------------------------------
FPoint UComponent::GetAlignedLocationI()
{
	guard(UComponent::GetAlignedLocationI);
	NOTE(debugf(TEXT("(%s)UComponent::GetAlignedLocationI"), *GetFullName()));

	FPoint		L=GetLocation();
	FAlignment	A=GetAlignment();

	UBOOL bAlign				= (A.XAxis==ALMT_Right) || (A.XAxis==ALMT_Center) || (A.YAxis==ALMT_Bottom) || (A.YAxis==ALMT_Center);
	UBOOL bHardAlign			= (A.XAxis==ALMT_HardRight) || (A.XAxis==ALMT_HardCenter) || (A.YAxis==ALMT_HardBottom) || (A.YAxis==ALMT_HardCenter);

	UBOOL bAlignToParent		= bAlign || (A.XAxis==ALMT_Left) || (A.YAxis==ALMT_Top);
	UBOOL bHardAlignToParent	= bHardAlign || (A.XAxis==ALMT_HardLeft) || (A.YAxis==ALMT_HardTop);

	if( bAlign || bHardAlign )
	{
		FDimension D=GetSize();
		switch( A.XAxis )
		{
		case ALMT_HardRight:
		case ALMT_Right:	L.X -= D.Width;		break;

		case ALMT_HardCenter:
		case ALMT_Center:	L.X -= D.Width/2;	break;
		}

		switch( A.YAxis )
		{
		case ALMT_HardBottom:
		case ALMT_Bottom:	L.Y -= D.Height;	break;

		case ALMT_HardCenter:
		case ALMT_Center:	L.Y -= D.Height/2;	break;
		}
	}

	if( bAlignToParent )
	{
		if( Parent )
		{
			FRectangle R;
			R = Parent->GetBounds();	// NOTE[aleiby]: Use GetBounds because it accounts for containers whose upper-left most component does not line up at 0,0.
			R -= Parent->GetLocation();	// NOTE[aleiby]: Because GetAlignedLocation is supposed to return a location relative to the parent's location, but GetBounds already includes that location.
			switch( A.XAxis )
			{
			case ALMT_Left:		L.X += R.X;					break;
			case ALMT_Right:	L.X += R.X + R.Width;		break;
			case ALMT_Center:	L.X += R.X + R.Width/2;		break;
			}

			switch( A.YAxis )
			{
			case ALMT_Top:		L.Y += R.Y;					break;
			case ALMT_Bottom:	L.Y += R.Y + R.Height;		break;
			case ALMT_Center:	L.Y += R.Y + R.Height/2;	break;
			}
		}
	}

	if( bHardAlignToParent )
	{
		UComponent* P = GetRoot();
		if( P )
		{
			FRectangle R;
			R = P->GetBounds();	// NOTE[aleiby]: Use GetBounds because it accounts for containers whose upper-left most component does not line up at 0,0.
			R -= Parent->GetScreenCoords();	// NOTE[aleiby]: Because GetAlignedLocation is supposed to return a location relative to the parent's location, but GetBounds already includes that location.
			switch( A.XAxis )
			{
			case ALMT_HardLeft:		L.X += R.X;					break;
			case ALMT_HardRight:	L.X += R.X + R.Width;		break;
			case ALMT_HardCenter:	L.X += R.X + R.Width/2;		break;
			}

			switch( A.YAxis )
			{
			case ALMT_HardTop:		L.Y += R.Y;					break;
			case ALMT_HardBottom:	L.Y += R.Y + R.Height;		break;
			case ALMT_HardCenter:	L.Y += R.Y + R.Height/2;	break;
			}
		}
	}

	return L;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle UComponent::GetBoundsI()
{
	guard(UComponent::GetBoundsI);
	NOTE(debugf(TEXT("(%s)UComponent::GetBoundsI"), *GetFullName()));

	FPoint		P=GetLocation();
	FDimension	D=GetSize();

	return FRectangle(P.X,P.Y,D.Width,D.Height);
	
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle UComponent::GetScreenBoundsI()
{
	guard(UComponent::GetScreenBoundsI);
	NOTE(debugf(TEXT("(%s)UComponent::GetScreenBoundsI"), *GetFullName()));

	FPoint		P=GetScreenCoords();
	FDimension	D=GetSize();
	
	return FRectangle(P.X,P.Y,D.Width,D.Height);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SetAlignmentI( const FAlignment& A )
{
	guard(UComponent::SetAlignmentI);
	NOTE(debugf(TEXT("(%s)UComponent::SetAlignmentI( %s )"), *GetFullName(), A.String()));

	Align = A;
	
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FAlignment UComponent::GetAlignmentI()
{
	guard(UComponent::GetAlignmentI);
	NOTE(debugf(TEXT("(%s)UComponent::GetAlignmentI"), *GetFullName()));

	return Align;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UComponent::ContainsI( const FPoint& P )
{
	guard(UComponent::ContainsI);
	NOTE(debugf(TEXT("(%s)UComponent::ContainsI( %s )"), *GetFullName(), P.String()));

//	return P<=GetBounds();	// this allows you to select multiple adjacent items at once.
	return P.Contained(GetBounds());

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UComponent::ContainsAbsI( const FPoint& P )
{
	guard(UComponent::ContainsAbsI);
	NOTE(debugf(TEXT("(%s)UComponent::ContainsAbsI( %s )"), *GetFullName(), P.String()));

//	return P<=GetScreenBounds();	// this allows you to select multiple adjacent items at once.
	return P.Contained(GetScreenBounds());

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent* UComponent::GetComponentAtI( const FPoint& P )
{
	guard(UComponent::GetComponentAtI);
	NOTE(debugf(TEXT("(%s)UComponent::GetComponentAtI( %s )"), *GetFullName(), P.String()));

	if( ContainsAbs(P) )	return this;
	else					return NULL;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UComponent::GetAbsDrawOrderI()
{
	guard(UComponent::GetAbsDrawOrderI);
	NOTE(debugf(TEXT("(%s)UComponent::GetAbsDrawOrderI"), *GetFullName()));

	if(Parent)	return Parent->GetAbsDrawOrder() * 10.f + DrawOrder;
	else		return DrawOrder;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SetParentI( class UContainer* P )
{
	guard(UComponent::SetParentI);
	NOTE(debugf(TEXT("(%s)UComponent::SetParentI( %s )"), *GetFullName(), P ? *P->GetFullName() : TEXT("None") ));

	Parent = P;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::BringToFrontI()
{
	guard(UComponent::BringToFrontI);
	NOTE(debugf(TEXT("(%s)UComponent::BringToFrontI"), *GetFullName()));
	
	FLOAT MaxDrawOrder = -99999.f;

	UContainer* P=GetGroup();
	UContainer* Parent=NULL;

	for( INT i=0; i<P->Components.Num(); i++ )
	{
		UComponent* C=P->Components(i);
		UContainer* Container = DynamicCast<UContainer>(C);
		if( !C->IsA(UMouseCursor::StaticClass()) )	// Fix ARL: Un-hack this.
		{
			if(!Parent && Container && Container->HasComponent(this))
				Parent = Container;
			MaxDrawOrder = Max( C->DrawOrder, MaxDrawOrder );
		}
	}

	if (Parent)
		Parent->DrawOrder = MaxDrawOrder * 1.01f;
	else
		debugf(NAME_Warning,TEXT("BringToFront: %s is not in group %s"),*GetFullName(),*P->GetFullName());

	P->Components.Sort();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SendToBackI()
{
	guard(UComponent::SendToBackI);
	NOTE(debugf(TEXT("(%s)UComponent::SendToBackI"), *GetFullName()));

	FLOAT MinDrawOrder = 99999.f;

	UContainer* P=GetGroup();
	UContainer* Parent=NULL;

	for( INT i=0; i<P->Components.Num(); i++ )
	{
		UComponent* C=P->Components(i);
		UContainer* Container = DynamicCast<UContainer>(C);
		if(!Parent && Container && Container->HasComponent(this))
			Parent = Container;
		MinDrawOrder = Min( C->DrawOrder, MinDrawOrder );
	}

	if (Parent)
		Parent->DrawOrder = MinDrawOrder * 0.99f;
	else
		debugf(NAME_Warning,TEXT("SendToBack: %s is not in group %s"),*GetFullName(),*P->GetFullName());

	P->Components.Sort();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::BringForwardI( INT NumLevels )
{
	guard(UComponent::BringForwardI);
	NOTE(debugf(TEXT("(%s)UComponent::BringForwardI( %d )"), *GetFullName(), NumLevels));

	// Fix ARL: Maybe fix to support proper groups -- not currently done since it would be quite slow.

	UContainer* P=GetParent();

	while( NumLevels-- > 0 )
	{
		if(P) for( INT i=1; i<P->Components.Num(); i++ )
		{
			if( P->Components(i-1)==this )
			{
				Exchange( P->Components(i), P->Components(i-1) );
				Exchange( P->Components(i)->DrawOrder, P->Components(i-1)->DrawOrder );
				break;
			}
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SendBackI( INT NumLevels )
{
	guard(UComponent::SendBackI);
	NOTE(debugf(TEXT("(%s)UComponent::SendBackI( %d )"), *GetFullName(), NumLevels));

	// Fix ARL: Maybe fix to support proper groups -- not currently done since it would be quite slow.

	UContainer* P=GetParent();

	while( NumLevels-- > 0 )
	{
		if(P) for( INT i=1; i<P->Components.Num(); i++ )
		{
			if( P->Components(i)==this )
			{
				Exchange( P->Components(i), P->Components(i-1) );
				Exchange( P->Components(i)->DrawOrder, P->Components(i-1)->DrawOrder );
				break;
			}
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::RequestFocusI()
{
	guard(UComponent::RequestFocusI);
	NOTE(debugf(TEXT("(%s)UComponent::RequestFocusI"), *GetFullName()));
	// Fix ARL: Implement me.
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UComponent::HasFocusI()
{
	guard(UComponent::HasFocusI);
	NOTE(debugf(TEXT("(%s)UComponent::HasFocusI"), *GetFullName()));
	// Fix ARL: Implement me.
	return true;
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent*	UComponent::GetComponentI( uiName StrictName )
{
	guard(UComponent::GetComponentI name);
	NOTE(debugf(TEXT("(%s)UComponent::GetComponentI(name)( %s )"), *GetFullName(), **StrictName ));

	return Name==StrictName ? this : NULL;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent*	UComponent::FindLooseComponentI( uiName LooseName )
{
	guard(UComponent::FindLooseComponentI name);
	NOTE(debugf(TEXT("(%s)UComponent::FindLooseComponentI(name)( %s )"), *GetFullName(), **LooseName ));

	return Name.LooseCompare(LooseName) ? this : NULL;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::GetLooseComponentsI( uiName LooseName, TArray<UComponent*> &Components )
{
	guard(UComponent::GetLooseComponentsI string);
	NOTE(debugf(TEXT("(%s)UComponent::GetLooseComponentsI(string)( %s )"), *GetFullName(), **LooseName ));

	if( Name.LooseCompare(LooseName) )
		Components.AddItem(this);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
uiPath UComponent::GetFullPathI()
{
	guard(UComponent::GetFullPathI);
	NOTE(debugf(TEXT("(%s)UComponent::GetFullPathI"), *GetFullName() ));

	uiPath Path;
	for(UComponent* C=this;C;C=C->Parent)
		Path.AddItem(C->Name);	//push onto stack
	checkSlow(GetGConsole()->Root->GetComponent(Path)==this);
	return Path;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent* UComponent::GetComponentI( uiPath Path )
{
	guard(UComponent::GetComponentI path);
	NOTE(debugf(TEXT("(%s)UComponent::GetComponentI(path)( %s )"), *GetFullName(), *GetPathString(Path) ));

	// Check for leaves.
	if( Path.Num()==1 )
	{
		if( Path(0)==Name )
			return this;
		if( Path(0)==uiName::Parent() )
			return Parent;
		if( Parent && Path(0)==Parent->Name )
			return Parent;
	}

	// Propogate upstream.
	uiName N = Path.Pop();
	if( Parent && (N==Parent->Name || N==uiName::Parent()) )
		return Parent->GetComponent(Path);

	return NULL;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
void UComponent::SetMouseOverEvent( UMouseCursor* M, ActionEvent* EnterEvent, ActionEvent* ExitEvent )
{
	guard(UComponent::SetMouseOverEvent);
	NOTE(debugf(TEXT("(%s)UComponent::SetMouseOverEvent( %s )"), *GetFullName(), M ? *M->GetFullName() : TEXT("None")));

	SetMouseEnterEvent( M, EnterEvent );
	SetMouseExitEvent( M, ExitEvent );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SetMouseEnterEvent( UMouseCursor* M, ActionEvent* Event )
{
	guard(UComponent::SetMouseEnterEvent);
	NOTE(debugf(TEXT("(%s)UComponent::SetMouseEnterEvent( %s )"), *GetFullName(), M ? *M->GetFullName() : TEXT("None")));

	// Validate input.
	if(!M){ debugf(TEXT("Invalid MouseCursor: NULL")); return; }
	if(!Event || !(Event->Type & ACCESSOR_MouseEnter) ){ debugf(TEXT("Invalid Event: %d"),Event ? Event->Type : -1); return; }

	// Setup notifications.
	if( !(AccessFlags & ACCESSOR_MouseEnter) )
		M->AddMouseActionListener( MA_Enter, this, Event );

	AddAccessor(Event);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SetMouseExitEvent( UMouseCursor* M, ActionEvent* Event )
{
	guard(UComponent::SetMouseExitEvent);
	NOTE(debugf(TEXT("(%s)UComponent::SetMouseExitEvent( %s )"), *GetFullName(), M ? *M->GetFullName() : TEXT("None")));

	// Validate input.
	if(!M){ debugf(TEXT("Invalid MouseCursor: NULL")); return; }
	if(!Event || !(Event->Type & ACCESSOR_MouseExit) ){ debugf(TEXT("Invalid Event: %d"),Event ? Event->Type : -1); return; }

	// Setup notifications.
	if( !(AccessFlags & ACCESSOR_MouseExit) )
		M->AddMouseActionListener( MA_Exit, this, Event );

	AddAccessor(Event);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SetMousePressEvent( UMouseCursor* M, ActionEvent* Event )
{
	guard(UComponent::SetMousePressEvent);
	NOTE(debugf(TEXT("(%s)UComponent::SetMousePressEvent( %s )"), *GetFullName(), M ? *M->GetFullName() : TEXT("None")));

	// Validate input.
	if(!M){ debugf(TEXT("Invalid MouseCursor: NULL")); return; }
	if(!Event || !(Event->Type & ACCESSOR_MousePress) ){ debugf(TEXT("Invalid Event: %d"),Event ? Event->Type : -1); return; }

	// Setup notifications.
	if( !(AccessFlags & ACCESSOR_MousePress) )
		M->AddMouseActionListener( MA_Press, this, Event );

	AddAccessor(Event);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SetMouseReleaseEvent( UMouseCursor* M, ActionEvent* Event )
{
	guard(UComponent::SetMouseReleaseEvent);
	NOTE(debugf(TEXT("(%s)UComponent::SetMouseReleaseEvent( %s )"), *GetFullName(), M ? *M->GetFullName() : TEXT("None")));

	// Validate input.
	if(!M){ debugf(TEXT("Invalid MouseCursor: NULL")); return; }
	if(!Event || !(Event->Type & ACCESSOR_MouseRelease) ){ debugf(TEXT("Invalid Event: %d"),Event ? Event->Type : -1); return; }

	// Setup notifications.
	if( !(AccessFlags & ACCESSOR_MouseRelease) )
		M->AddMouseActionListener( MA_Release, this, Event );

	AddAccessor(Event);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::SetMouseClickEvent( UMouseCursor* M, ActionEvent* Event )
{
	guard(UComponent::SetMouseClickEvent);
	NOTE(debugf(TEXT("(%s)UComponent::SetMouseClickEvent( %s )"), *GetFullName(), M ? *M->GetFullName() : TEXT("None")));

	// Validate input.
	if(!M){ debugf(TEXT("Invalid MouseCursor: NULL")); return; }
	if(!Event || !(Event->Type & ACCESSOR_MouseClick) ){ debugf(TEXT("Invalid Event: %d"),Event ? Event->Type : -1); return; }

	// Setup notifications.
	if( !(AccessFlags & ACCESSOR_MouseClick) )
	{
		M->AddMouseActionListener( MA_Enter,   this, UI_NATIVE_CALLBACK(None,this,UComponent,MouseClick_Enter  ) );
		M->AddMouseActionListener( MA_Exit,    this, UI_NATIVE_CALLBACK(None,this,UComponent,MouseClick_Exit   ) );
		M->AddMouseActionListener( MA_Press,   this, UI_NATIVE_CALLBACK(None,this,UComponent,MouseClick_Press  ) );
		M->AddMouseActionListener( MA_Release, this, UI_NATIVE_CALLBACK(None,this,UComponent,MouseClick_Release) );
	}

	AddAccessor(Event);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////////
// Animation helpers //
///////////////////////

//------------------------------------------------------------------------------
void UComponent::Tween( UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct )
{
	guard(UComponent::Tween);
	NOTE(debugf(TEXT("(%s)UComponent::Tween( %s, %s, %s, %f )"), *GetFullName(), Source ? *Source->GetFullName() : TEXT("None"), Start ? *Start->GetFullName() : TEXT("None"), End ? *End->GetFullName() : TEXT("None"), Pct));

	FPoint     P1, P2;
	FDimension D1, D2;
	FColor     C1, C2;
	FLOAT      A1, A2;

	Pct = AdjustTweenPct( Pct, Source );

	// Fix ARL: Add ability to tween across Alignments by going off of the ScreenCoords instead.

	// Fix ARL: Get rid of need for source (and TweenLocation, etc) by simply locking the attributes you don't want to change instead.

	if( Source->bTweenLocation ){ P1=Start->GetLocation(); P2 = End->GetLocation(); if( P1!=P2 ) SetLocation( P1.Blend(P2,Pct) ); }
	if( Source->bTweenSize     ){ D1=Start->GetSize();     D2 = End->GetSize();     if( D1!=D2 ) SetSize    ( D1.Blend(D2,Pct) ); }
	if( Source->bTweenColor    ){ C1=Start->GetColor();    C2 = End->GetColor();    if( C1!=C2 ) SetColor   ( C1.Blend(C2,Pct) ); }
	if( Source->bTweenAlpha    ){ A1=Start->GetAlpha();    A2 = End->GetAlpha();    if( A1!=A2 ) SetAlpha   ( appBlend(A1,A2,Pct) ); }

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UComponent::AdjustTweenPct( FLOAT TweenPct, UMultiStateComponent* Source )
{
	guard(UComponent::AdjustTweenPct);
	NOTE(debugf(TEXT("(%s)UComponent::AdjustTweenPct( %f )"), *GetFullName(), TweenPct));

	return	Source->bTweenLinear												? TweenPct
		:	(Source->bTweenReverseEase || Source->bTweenReverseEaseOverride)	? appPow(TweenPct,3)
		:																		  1.0f-appPow((1.0f-TweenPct),3);
//	return TweenPct^2;
//	return Sin(TweenPct*PI/2);
//	return TweenPct;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
UContainer* UComponent::GetParent(uiClass* Type)
{
	guard(UComponent::GetParent);
	NOTE(debugf(TEXT("(%s)UComponent::GetParent(%s)"), *GetFullName(), Type?Type->GetName():TEXT("")));

	UContainer* C=DynamicCast<UContainer>(Parent);
	if(!Type || !C || C->IsA(Type))
		return C;
	else
		return C->GetParent(Type);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent* UComponent::GetRoot()
{
	guard(UComponent::GetRoot);
	NOTE(debugf(TEXT("(%s)UComponent::GetRoot"), *GetFullName()));

	if(Parent)	return Parent->GetRoot();
	else		return this;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UConsole* UComponent::GetConsole()
{
	guard(UComponent::GetConsole);
	NOTE(debugf(TEXT("(%s)UComponent::GetConsole"), *GetFullName()));

	return GetGConsole();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UUIConsole* UComponent::GetUIConsole()
{
	guard(UComponent::GetUIConsole);
	NOTE(debugf(TEXT("(%s)UComponent::GetUIConsole"), *GetFullName()));

	return GetGConsole();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
APlayerController* UComponent::GetPlayerOwner()
{
	guard(UComponent::GetPlayerOwner);
	NOTE(debugf(TEXT("(%s)UComponent::GetPlayerOwner"), *GetFullName()));

	return GetConsole()->Viewport->Actor;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
DOUBLE UComponent::GetTimeSeconds()
{
	guard(UComponent::GetTimeSeconds);
	NOTE(debugf(TEXT("(%s)UComponent::GetTimeSeconds"), *GetFullName()));

	return appSeconds();
/* the below code will be affected by Slomo, gametime, cause problems with level transitions, and so on
	// Fix ARL: There must be a better way.
	return GetPlayerOwner()->GetLevel()->TimeSeconds;
*/
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UClient* UComponent::GetClient()
{
	guard(UComponent::GetClient);
	NOTE(debugf(TEXT("(%s)UComponent::GetClient"), *GetFullName()));

	return GetPlayerOwner()->GetLevel()->Engine->Client;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponent::ConsoleCommand( FString Command )
{
	guard(UComponent::ConsoleCommand);
	NOTE(debugf(TEXT("(%s)UComponent::ConsoleCommand( %s )"), *GetFullName(), Command));

	GetConsole()->ConsoleCommand( *Command );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

