#include "UI.h"


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
UBOOL UContainer::AddComponent( UComponent* C, INT Index )
{
	guard(UContainer::AddComponent);
	NOTE(debugf(TEXT("(%s)UContainer::AddComponent( %s, %d )"), *GetFullName(), C ? *C->GetFullName() : TEXT("None"), Index ));
	
	if
	(	C
//	&&	C!=this					// Fix ARL: Make sure Containers don't get directly
//	&&	!C->HasComponent(this)	// or indirectly added to themselves.
//	&&	!HasComponent(C)		// Fix ARL: Don't allow duplicates?
//	&&	Index>=0				// Fix ARL: Use Index.
	)
	{
		if( C->GetParent() )
			C->GetParent()->RemoveComponent(C);
		C->SetParent(this);
		Components.AddItem(C);

		// Fix DrawOrders so there's no duplicates.
		for( INT i=1; i>Components.Num(); i++ )
			if( Components(i)->DrawOrder <= Components(i-1)->DrawOrder )
				Components(i)->DrawOrder = Components(i-1)->DrawOrder+0.01f;

		return true;
	}

	return false;
	
	unguard;
}

//------------------------------------------------------------------------------
UBOOL UContainer::RemoveComponent( UComponent* C )
{
	guard(UContainer::RemoveComponent);
	NOTE(debugf(TEXT("(%s)UContainer::RemoveComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	// Fix ARL: Send notifications (to Component? to Container?) -- what if it messes with our array?
	return Components.RemoveItem(C);

	unguard;
}

//------------------------------------------------------------------------------
INT UContainer::RemoveComponentType( uiClass* ComponentType, UBOOL bAndSubclasses, UBOOL bDestroy )
{
	guard(UContainer::RemoveComponentType);
	NOTE(debugf(TEXT("(%s)UContainer::RemoveComponentType"), *GetFullName()));

	TArray<UComponent*> Removed;

	// Fix ARL: Implement bAndSubclasses

	for( INT i=0; i<Components.Num(); i++ )
		if( bAndSubclasses ? Components(i)->IsA(ComponentType) : Components(i)->GetClass()==ComponentType )
			Removed.AddUniqueItem( Components(i) );

	for( INT i=0; i<Removed.Num(); i++ )
		if(bDestroy)	Removed(i)->DeleteUObject();		// This will cause it to remove itself.
		else			RemoveComponent( Removed(i) );

	return Removed.Num();

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UContainer::HasComponent( UComponent* C )
{
	guard(UContainer::HasComponent);
	NOTE(debugf(TEXT("(%s)UContainer::HasComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("None") ));

	UComponent* iComponent;
	UContainer* iContainer;

	for( INT i=0; i<Components.Num(); i++ )
	{
		iComponent = Components(i);
		if( iComponent==C )
			return true;
		iContainer = DynamicCast<UContainer>(iComponent);
		if( iContainer && iContainer->HasComponent(C) )
			return true;
	}

	return false;

	unguard;
}

//------------------------------------------------------------------------------
INT UContainer::GetComponentIndex( UComponent* C )
{
	guard(UContainer::GetComponentIndex);
	NOTE(debugf(TEXT("(%s)UContainer::GetComponentIndex( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("None") ));

	for( INT i=0; i<Components.Num(); i++ )
		if( Components(i)==C )
			return i;

	return -1;

	unguard;
}


///////////////////////
// UObject overrides //
///////////////////////

//------------------------------------------------------------------------------
void UContainer::Destroy()
{
	guard(UContainer::Destroy);
	NOTE(debugf(TEXT("(%s)UContainer::Destroy"), *GetFullName()));

	while( Components.Num() )
		Components(0)->DeleteUObject();	// This will cause it to remove itself from our list.

	Super::Destroy();

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Serialize( FArchive &Ar )
{
	guard(UContainer::Serialize);
	NOTE(debugf(TEXT("(%s)UContainer::Serialize"), *GetFullName()));

	Super::Serialize(Ar);

	INT Size;
	if( Ar.IsSaving() )
		Size = Components.Num();
	Ar << AR_INDEX(Size);
	if( Ar.IsLoading() )
	{
		Components.Empty(Size);
		Components.Add(Size);
	}
	for( INT i=0; i<Size; i++ )
		(FArchiveUI&)Ar << Components(i);

	// Fixup parents.
	if( Ar.IsLoading() )
		for( INT i=0; i<Components.Num(); i++ )
			Components(i)->Parent = this;

	unguard;
}


////////////////////////////
// Composite calculations //
////////////////////////////

//------------------------------------------------------------------------------
FRectangle UContainer::GetBoundsI()
{
	guard(UContainer::GetBoundsI);
	NOTE(debugf(TEXT("(%s)UContainer::GetBoundsI"), *GetFullName()));

	FLOAT MinX=0.0f;
	FLOAT MinY=0.0f;
	FLOAT MaxX=0.0f;
	FLOAT MaxY=0.0f;

	for( INT i=0; i<Components.Num(); i++ )		// Fix ARL: Should we be going through accessor functions (NumComponents/GetComponent) instead of accessing Components directly?
	{
		UComponent* C = Components(i);
		FRectangle R = C->GetBounds() + GetLocation();	// Fix ARL: Why aren't we caching this?  Why not simply tack it on at the end?  Do we need it at all?

		if( i==0 )
		{
			MinX = R.X;
			MinY = R.Y;
			MaxX = R.X+R.Width;
			MaxY = R.Y+R.Height;
		}
		else
		{
			MinX = Min( R.X, MinX );
			MinY = Min( R.Y, MinY );
			MaxX = Max( R.X+R.Width, MaxX );
			MaxY = Max( R.Y+R.Height, MaxY );
		}
	}

	return FRectangle(MinX,MinY,MaxX-MinX,MaxY-MinY);

	unguard;
}

//------------------------------------------------------------------------------
FRectangle UContainer::GetScreenBoundsI()
{
	guard(UContainer::GetScreenBoundsI);
	NOTE(debugf(TEXT("(%s)UContainer::GetScreenBoundsI"), *GetFullName()));

	FLOAT MinX=0.0f;
	FLOAT MinY=0.0f;
	FLOAT MaxX=0.0f;
	FLOAT MaxY=0.0f;

	for( INT i=0; i<Components.Num(); i++ )
	{
		UComponent*	C = Components(i);
		FPoint      P = C->GetScreenCoords();
		FDimension  D = C->GetScreenBounds().Dimension();

		if( i==0 )
		{
			MinX = P.X;
			MinY = P.Y;
			MaxX = P.X+D.Width;
			MaxY = P.Y+D.Height;
		}
		else
		{
			MinX = Min( P.X, MinX );
			MinY = Min( P.Y, MinY );
			MaxX = Max( P.X+D.Width, MaxX );
			MaxY = Max( P.Y+D.Height, MaxY );
		}
	}

	return FRectangle(MinX,MinY,MaxX-MinX,MaxY-MinY);

	unguard;
}

//------------------------------------------------------------------------------
UComponent* UContainer::GetComponentAtI( const FPoint& P )
{
	guard(UContainer::GetComponentAtI);
	NOTE(debugf(TEXT("(%s)UContainer::GetComponentAtI( %s )"), *GetFullName(), P.String()));

	// Iterate through components backwards (to give higher draw order components higher priority).
	for( INT i=Components.Num()-1; i>=0; --i )
	{
		UComponent* C=Components(i)->GetComponentAt(P);
		if(C) return C;
	}

	return NULL;

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::SetSizeI( const FDimension& D )
{
	guard(UContainer::SetSizeI);
	NOTE(debugf(TEXT("(%s)UContainer::SetSizeI( %s )"), *GetFullName(), D.String()));

	FDimension ScaleDim = D / GetSize();
	FPoint     ScalePt  = FPoint(ScaleDim.Width,ScaleDim.Height);

	for( INT i=0; i<Components.Num(); i++ )
	{
		UComponent* C = Components(i);
		C->SetSize( C->GetSize() * ScaleDim );
		C->SetLocation( C->GetLocation() * ScalePt );
	}

	unguard;
}


/////////////////////////////////////
// Component interface dispatchers //
/////////////////////////////////////

//------------------------------------------------------------------------------
// NOTE[aleiby]: This unnecessarily gets the upper bounding location and throws it away, but it's better than duplicating potentially complicated code.
FDimension UContainer::GetSizeI()				
{
	guard(UContainer::GetSizeI);
	NOTE(debugf(TEXT("(%s)UContainer::GetSizeI"), *GetFullName()));

	FRectangle R=GetBounds();
	return FDimension(R.Width,R.Height);

	unguard;
}

//------------------------------------------------------------------------------
ERenderStyle UContainer::GetStyleI()
{
	guard(UContainer::GetStyleI);
	NOTE(debugf(TEXT("(%s)UContainer::GetStyleI"), *GetFullName()));

	return Components(0)->GetStyle();	// Fix ARL: Return max instead?

	unguard;
}	

//------------------------------------------------------------------------------
void UContainer::SetStyleI( ERenderStyle S )
{
	guard(UContainer::SetStyleI);
	NOTE(debugf(TEXT("(%s)UContainer::SetStyleI( %s )"), *GetFullName(), GetEnumEx(Engine.ERenderStyle,S,4)));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->SetStyle(S);

	unguard;
}

//------------------------------------------------------------------------------
FColor UContainer::GetColorI()
{	
	guard(UContainer::GetColorI);
	NOTE(debugf(TEXT("(%s)UContainer::GetColorI"), *GetFullName()));

	if( Components.Num()<=0 ) return FColor(0,0,0,0);

	FColor Color;
	INT R=0;
	INT G=0;
	INT B=0;
	INT A=0;
	
	INT i;
	for( i=0; i<Components.Num(); i++ )
	{
		Color=Components(i)->GetColor();
		R += Color.R;
		G += Color.G;
		B += Color.B;
		A += Color.A;
	}
	
	Color.R = R/i;
	Color.G = G/i;
	Color.B = B/i;
	Color.A = A/i;

	return Color;

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::SetColorI( const FColor& C )
{
	guard(UContainer::SetColorI);
	NOTE(debugf(TEXT("(%s)UContainer::SetColorI( %s )"), *GetFullName(), C.String() ));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->SetColor( C );

	unguard;
}

//------------------------------------------------------------------------------
BYTE UContainer::GetAlphaI()
{
	guard(UContainer::GetAlphaI);
	NOTE(debugf(TEXT("(%s)UContainer::GetAlphaI"), *GetFullName() ));

	INT A=0,i=0;
	for(; i<Components.Num(); i++ )
		A += Components(i)->GetAlpha();
	
	return A/i;

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::SetAlphaI( BYTE A )
{
	guard(UContainer::SetAlphaI);
	NOTE(debugf(TEXT("(%s)UContainer::SetAlphaI( %d )"), *GetFullName(), A ));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->SetAlpha(A);

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::SetAlphaPctI( FLOAT Pct )
{
	guard(UContainer::SetAlphaPctI);
	NOTE(debugf(TEXT("(%s)UContainer::SetAlphaPctI( %f )"), *GetFullName(), Pct ));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->SetAlphaPct(Pct);

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::SetVisibilityI( UBOOL bVisible )
{
	guard(UContainer::SetVisibilityI);
	NOTE(debugf(TEXT("(%s)UContainer::SetVisibilityI( %s )"), *GetFullName(), bVisible ? TEXT("True") : TEXT("False")));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->SetVisibility( bVisible );

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UContainer::IsVisibleI()
{
	guard(UContainer::IsVisibleI);
	NOTE(debugf(TEXT("(%s)UContainer::IsVisibleI"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		if( Components(i)->IsVisible() )
			return true;
		
	return false;

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UContainer::IsShowingI()
{
	guard(UContainer::IsShowingI);
	NOTE(debugf(TEXT("(%s)UContainer::IsShowingI"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		if( Components(i)->IsShowing() )
			return true;
		
	return false;

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UContainer::IsDisplayableI()
{
	guard(UContainer::IsDisplayableI);
	NOTE(debugf(TEXT("(%s)UContainer::IsDisplayableI"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		if( Components(i)->IsDisplayable() )
			return true;
		
	return false;

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::SetEnabledI( UBOOL bEnabled )
{
	guard(UContainer::SetEnabledI);
	NOTE(debugf(TEXT("(%s)UContainer::SetEnabledI( %s )"), *GetFullName(), bEnabled ? TEXT("True") : TEXT("False")));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->SetEnabled( bEnabled );

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UContainer::IsEnabledI()
{
	guard(UContainer::IsEnabledI);
	NOTE(debugf(TEXT("(%s)UContainer::IsEnabledI"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		if( Components(i)->IsEnabled() )
			return true;
	
	return false;

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::ValidateI()
{
	guard(UContainer::ValidateI);
	NOTE(debugf(TEXT("(%s)UContainer::ValidateI"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->Validate();

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::InvalidateI()
{
	guard(UContainer::InvalidateI);
	NOTE(debugf(TEXT("(%s)UContainer::InvalidateI"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->Invalidate();

	unguard;
}

//------------------------------------------------------------------------------
UBOOL UContainer::IsValidI()
{
	guard(UContainer::IsValidI);
	NOTE(debugf(TEXT("(%s)UContainer::IsValidI"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		if( !Components(i)->IsValid() )
			return false;
	
	return true;

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Tick( FLOAT DeltaSeconds )
{
	guard(UContainer::Tick);
	NOTE(debugf(TEXT("(%s)UContainer::Tick( %s )"), *GetFullName(), DeltaSeconds));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->Tick( DeltaSeconds );

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Precache(UPDATE_DEF)
{
	guard(UContainer::Precache);
	NOTE(debugf(TEXT("(%s)UContainer::Precache"), *GetFullName()));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->Precache(UPDATE_PARMS);

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Update(UPDATE_DEF)
{
	guard(UContainer::Update);
	NOTE0(debugf(TEXT("(%s)UContainer::Update( %s )"), *GetFullName(), Canvas ? Canvas->GetFullName() : TEXT("None")));

	for( INT i=0; i<Components.Num(); i++ )
		if( Components(i)->IsEnabled() )			// Fix ARL: Potentially slow?
			Components(i)->Update(UPDATE_PARMS);

	UComponent::Update(UPDATE_PARMS);

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Reset()
{
	guard(UContainer::Reset);
	NOTE(debugf(TEXT("(%s)UContainer::Reset"), *GetFullName() ));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->Reset();

	Super::Reset();

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Shrink()
{
	guard(UContainer::Shrink);
	NOTE(debugf(TEXT("(%s)UContainer::Shrink"), *GetFullName() ));

	Components.Shrink();

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->Shrink();

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::SetTween( FLOAT Pct )
{
	guard(UContainer::SetTween);
	NOTE(debugf(TEXT("(%s)UContainer::SetTween( %f )"), *GetFullName(), Pct));

	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->SetTween( Pct );

	unguard;
}

//------------------------------------------------------------------------------
void UContainer::Tween( UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct )
{
	guard(UContainer::Tween);
	NOTE(debugf(TEXT("(%s)UContainer::Tween"),*GetFullName()));

	Super::Tween( Source, Start, End, Pct );

	UContainer* StartC = DynamicCast<UContainer>(Start);
	UContainer* EndC = DynamicCast<UContainer>(End);

	if
	(	StartC && EndC
	&&	Components.Num()==StartC->Components.Num()
	&&	Components.Num()==EndC->Components.Num()
	)
	{
		for( INT i=0; i<Components.Num(); i++ )
			Components(i)->Tween( Source, StartC->Components(i), EndC->Components(i), Pct );
	}

	unguard;
}

//------------------------------------------------------------------------------
UComponent*	UContainer::GetComponentI( uiName Name )
{
	guard(UContainer::GetComponentI);
	NOTE(debugf(TEXT("(%s)UContainer::GetComponentI( %s )"), *GetFullName(), **Name ));

	UComponent* C=Super::GetComponentI(Name);
	for( INT i=0; !C && i<Components.Num(); C=Components(i++)->GetComponent(Name) );
	return C;

	unguard;
}

//------------------------------------------------------------------------------
UComponent* UContainer::GetComponentI( uiPath Path )
{
	guard(UContainer::GetComponentI path);
	NOTE(debugf(TEXT("(%s)UContainer::GetComponentI(path)( %s )"), *GetFullName(), *GetPathString(Path) ));

	// Check leaves and upstream connections.
	UComponent* C=Super::GetComponentI(Path);
	if(C) return C;

	// Check children if we're next in the path.
	uiName N = Path.Pop();
	if( N==Name )
	{
		for( INT i=0; i<Components.Num(); i++ )
			if( Path.Last()==Components(i)->Name )
				return Components(i)->GetComponent(Path);
	}

	return NULL;
	unguard;
}

//------------------------------------------------------------------------------
UComponent*	UContainer::FindLooseComponentI( uiName Name )
{
	guard(UContainer::FindLooseComponentI);
	NOTE(debugf(TEXT("(%s)UContainer::FindLooseComponentI( %s )"), *GetFullName(), **Name ));

	UComponent* C=Super::FindLooseComponentI(Name);
	for( INT i=0; !C && i<Components.Num(); C=Components(i++)->FindLooseComponent(Name) );
	return C;

	unguard;
}

//------------------------------------------------------------------------------
void		UContainer::GetLooseComponentsI( uiName Name, TArray<UComponent*> &List )
{
	guard(UContainer::GetLooseComponentsI);
	NOTE(debugf(TEXT("(%s)UContainer::GetLooseComponentsI( %s )"), *GetFullName(), **Name ));

	Super::GetLooseComponentsI(Name,List);
	for( INT i=0; i<Components.Num(); i++ )
		Components(i)->GetLooseComponents(Name,List);

	unguard;
}


