/*=============================================================================
	UIComponents.cpp: UI Component implementations.
	Copyright (C) 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron R Leiby
=============================================================================*/

#include "UI.h"
#include "ImportUI.h"


/*
	Class implementations.
*/

	IMPLEMENT_UI_CLASS(UComponent)
	IMPLEMENT_UI_CLASS(UContainer)
	IMPLEMENT_UI_CLASS(UFixedSizeContainer)
	IMPLEMENT_UI_CLASS(UScaleContainer)
	IMPLEMENT_UI_CLASS(UWindow)
	IMPLEMENT_UI_CLASS(UTextArea)
	IMPLEMENT_UI_CLASS(UTimedTextArea)
	IMPLEMENT_UI_CLASS(UScrollTextArea)
	IMPLEMENT_UI_CLASS(UScrollFrame)
	IMPLEMENT_UI_CLASS(UScrollBar)
	IMPLEMENT_UI_CLASS(UImageWipe)
	IMPLEMENT_UI_CLASS(UTileContainer)
	IMPLEMENT_UI_CLASS(UMultiStateComponent)
	IMPLEMENT_UI_CLASS(USelectorList)
	IMPLEMENT_UI_CLASS(USelectorItem)
	IMPLEMENT_UI_CLASS(USelector)
	IMPLEMENT_UI_CLASS(UPlayerID)
	IMPLEMENT_UI_CLASS(UButton)
	IMPLEMENT_UI_CLASS(UHoldButton)
	IMPLEMENT_UI_CLASS(UFrame)
	IMPLEMENT_UI_CLASS(URealComponent)
	IMPLEMENT_UI_CLASS(UTextComponentBase)
	IMPLEMENT_UI_CLASS(UVarWatcher)
	IMPLEMENT_UI_CLASS(ULabel)
	IMPLEMENT_UI_CLASS(ULabelShadow)
	IMPLEMENT_UI_CLASS(UTimedLabel)
	IMPLEMENT_UI_CLASS(UInputSelector)
	IMPLEMENT_UI_CLASS(UTextField)
	IMPLEMENT_UI_CLASS(UNumWatcher)
	IMPLEMENT_UI_CLASS(USoundComponent)
	IMPLEMENT_UI_CLASS(UImage)
	IMPLEMENT_UI_CLASS(UImageComponentBase)
	IMPLEMENT_UI_CLASS(UImageComponent)
	IMPLEMENT_UI_CLASS(UTimedImage)
	IMPLEMENT_UI_CLASS(USlider)
	IMPLEMENT_UI_CLASS(UMouseCursor)
	IMPLEMENT_UI_CLASS(UImageWedge)
	IMPLEMENT_UI_CLASS(UCheckBox)
	IMPLEMENT_UI_CLASS(UButtonImage)
	IMPLEMENT_UI_CLASS(UMultiImageComponent)
	IMPLEMENT_UI_CLASS(UAnimatedImageComponent)
	IMPLEMENT_UI_CLASS(UScriptComponent)
	IMPLEMENT_UI_CLASS(UKeyEvent)
	IMPLEMENT_UI_CLASS(UEditor)
	IMPLEMENT_UI_CLASS(UComponentHandle)
	IMPLEMENT_UI_CLASS(UMoveHandle)
	IMPLEMENT_UI_CLASS(UResizeHandle)
	IMPLEMENT_UI_CLASS(UResizeHandleN)
	IMPLEMENT_UI_CLASS(UResizeHandleS)
	IMPLEMENT_UI_CLASS(UResizeHandleE)
	IMPLEMENT_UI_CLASS(UResizeHandleW)
	IMPLEMENT_UI_CLASS(UResizeHandleNE)
	IMPLEMENT_UI_CLASS(UResizeHandleNW)
	IMPLEMENT_UI_CLASS(UResizeHandleSE)
	IMPLEMENT_UI_CLASS(UResizeHandleSW)
	IMPLEMENT_UI_CLASS(UCredits)
	IMPLEMENT_UI_CLASS(UMaterialContainer)


/*
	UAnimatedImageComponent
*/

//------------------------------------------------------------------------------
void UAnimatedImageComponent::SetImage( FImage* _Image )
{
	guard(UAnimatedImageComponent::SetImage);
	NOTE(debugf(TEXT("(%s)UAnimatedImageComponent::SetImage"), *GetFullName() ));

	Images.AddItem(_Image);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UAnimatedImageComponent::SetTween( FLOAT Pct )
{
	guard(UAnimatedImageComponent::SetTween);
	NOTE(debugf(TEXT("(%s)UAnimatedImageComponent::SetTween"), *GetFullName() ));

	Tween = Pct;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FImage* UAnimatedImageComponent::GetImage()
{
	guard(UAnimatedImageComponent::GetImage);
	NOTE(debugf(TEXT("(%s)UAnimatedImageComponent::GetImage"), *GetFullName() ));

	SIMPLE_ACCESSORX(AnimatedImageTween,&Tween)

	INT NumFrames = Images.Num(); if(NumFrames==0) return NULL;
	INT Index = Clamp( appFloor(NumFrames*Tween), 0, NumFrames-1 );
	return Images(Index);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UButton
*/

////////////////////
// Initialization //
////////////////////

//------------------------------------------------------------------------------
void UButton::Constructed()
{
	guard(UButton::Constructed);
	NOTE(debugf(TEXT("(%s)UButton::Constructed"), *GetFullName() ));

	Super::Constructed();

	Image = NEW_SUBCOMPONENT(UButtonImage);
	AddComponent( Image );

	Text = NEW_SUBCOMPONENT(ULabel);
	Text->SetAlignment( FAlignment(ALMT_Center,ALMT_Center) );
	AddComponent( Text );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UButton::Serialize( FArchive &Ar )
{
	guard(UButton::Serialize);
	NOTE(debugf(TEXT("(%s)UButton::Serialize"), *GetFullName()));

	Super::Serialize(Ar);

	SERIALIZE_INDEXED_COMPONENT(UButtonImage,Image);
	SERIALIZE_INDEXED_COMPONENT(ULabel,Text);

	Ar << Border;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UButton::SetImages( FImage* Up, FImage* Down )
{
	guard(UButton::SetImages);
	NOTE(debugf(TEXT("(%s)UButton::SetImages"), *GetFullName() ));

	Image->Image = Up;
	Image->DownImage = Down;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UButton::Init()
{
	guard(UButton::Init);
	NOTE(debugf(TEXT("(%s)UButton::Init"), *GetFullName() ));

	Image->Init();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UButton::SetFontSize( EFontSize NewSize )
{
	guard(UButton::SetFontSize);
	NOTE(debugf(TEXT("(%s)UButton::SetFontSize"), *GetFullName() ));

	Text->SetFontSize( NewSize );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UButton::SetFont( UFont* NewFont )
{
	guard(UButton::SetFont);
	NOTE(debugf(TEXT("(%s)UButton::SetFont"), *GetFullName() ));

	Text->SetFont( NewFont );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UFont* UButton::GetFont( UCanvas* C )
{
	guard(UButton::GetFont);
	NOTE(debugf(TEXT("(%s)UButton::GetFont"), *GetFullName() ));

	return Text->GetFont(C);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UButton::SetText( FString NewText, UBOOL bNoUpdateSize )
{
	guard(UButton::SetText);
	NOTE(debugf(TEXT("(%s)UButton::SetText"), *GetFullName() ));

	Text->SetText( NewText, bNoUpdateSize );

	if( !bNoUpdateSize )
	{
		Image->SetSize( Text->GetSize() + FDimension(Border*2,Border*2) );
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FString UButton::GetText()
{
	guard(UButton::GetText);
	NOTE(debugf(TEXT("(%s)UButton::GetText"), *GetFullName() ));

	return Text->GetText();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////////////////
// Component overrides //
/////////////////////////

//------------------------------------------------------------------------------
void UButton::SetSizeI( const FDimension& D )
{
	guard(UButton::SetSizeI);
	NOTE(debugf(TEXT("(%s)UButton::SetSizeI"), *GetFullName() ));

	Image->SetSize(D);
	Text->SetSize(D);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
// Setup mouse click event.
//------------------------------------------------------------------------------
void UButton::SetMouse( UMouseCursor* M )
{
	guard(UButton::SetMouse);
	NOTE(debugf(TEXT("(%s)UButton::SetMouse"), *GetFullName() ));

	Image->SetMouse( M );
	SetMouseClickEvent( M, UI_NATIVE_CALLBACK(MouseClick,this,UButton,Click) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UButton::Click(void*)
{
	guard(UButton::Click);
	NOTE(debugf(TEXT("(%s)UButton::Click"), *GetFullName() ));

	SIMPLE_ACCESSOR(ButtonAction)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UButtonImage
*/

////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UButtonImage::SetMouse( UMouseCursor* M )
{
	guard(UButtonImage::SetMouse);
	NOTE(debugf(TEXT("(%s)UButtonImage::SetMouse"), *GetFullName() ));

	SetMouseClickEvent( M, new ActionEvent(ACCESSOR_MouseClick) );	// only used so we can use bMouseClick_Over and bMouseClick_PressedWhileOver.  Fix ARL: this feels like a hack.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UButtonImage::SetImage( FImage* _Image, FImage* _DownImage )
{
	guard(UButtonImage::SetImage);
	NOTE(debugf(TEXT("(%s)UButtonImage::SetImage"), *GetFullName()));

	Super::SetImage( _Image );
	DownImage = _DownImage;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FImage* UButtonImage::GetImage()
{
	guard(UButtonImage::GetImage);
	NOTE(debugf(TEXT("(%s)UButtonImage::GetImage"), *GetFullName() ));

	if( bMouseClick_Over && bMouseClick_PressedWhileOver )
		return DownImage;
	else
		return Super::GetImage();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UCheckBox
*/


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UCheckBox::SetChecked( UBOOL B )
{
	guard(UCheckBox::SetChecked);
	NOTE(debugf(TEXT("(%s)UCheckBox::SetChecked"), *GetFullName() ));

	bChecked = B;

	SIMPLE_ACCESSOR_SET(CheckboxModify,UBOOL,bChecked)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UCheckBox::GetChecked()
{
	guard(UCheckBox::GetChecked);
	NOTE(debugf(TEXT("(%s)UCheckBox::GetChecked"), *GetFullName() ));

	IMPLEMENT_ACCESSOR_BEGIN(CheckboxAccess,UBOOL)
		bChecked = Result;
	IMPLEMENT_ACCESSOR_END(CheckboxAccess)

	return bChecked;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UCheckBox::Toggle(void*)
{
	guard(UCheckBox::Toggle);
	NOTE(debugf(TEXT("(%s)UCheckBox::Toggle"), *GetFullName() ));

	SetChecked( !bChecked );	// NOTE[aleiby]: We're not calling GetChecked() here because bChecked should already be set to whatever state the image was drawn at last.  If we called the function, the actual value may already have changed, so it would look like nothing had happened.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UCheckBox::SetMouse( UMouseCursor* M )
{
	guard(UCheckBox::SetMouse);
	NOTE(debugf(TEXT("(%s)UCheckBox::SetMouse"), *GetFullName() ));

	SetMouseClickEvent( M, UI_NATIVE_CALLBACK(MouseClick,this,UCheckBox,Toggle) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UCheckBox::SetImage( FImage* _Image, FImage* _Check )
{
	guard(UCheckBox::SetImage);
	NOTE(debugf(TEXT("(%s)UCheckBox::SetImage"), *GetFullName() ));

	Super::SetImage( _Image );
	Check = _Check;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
// NOTE[aleiby]: Completely overriden to avoid unnecessary extra work (like 
// getting the screen coords, size, pushing the canvas, etc, etc.)  We should
// occasionally check ImageComponentBase's version of the function to make sure
// we have all the latest changes since we're not calling Super.Update here.
//------------------------------------------------------------------------------
void UCheckBox::Update(UPDATE_DEF)
{
	guard(UCheckBox::Update);
	NOTE(debugf(TEXT("(%s)UCheckBox::Update"), *GetFullName() ));

	FImage* I=GetImage();
	if(!I) return;

	FPoint P = GetScreenCoords();	// Fix ARL: Wouldn't it make more sense for Update to pass in the relative screen coords (so we don't have to be constantly calculating them all the way up to the root for every component)?
	FDimension D = GetSize();

	Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width,D.Height), 0, GetPolyFlags(), GetColor() );

	// Fix ARL: Gray-out if Locked.
	if( GetChecked() )
		Ren->DrawTile( Check, FRectangle(P.X,P.Y,D.Width,D.Height), 0, GetPolyFlags(), GetColor() );

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

/*
	UFixedSizeContainer
*/

///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UFixedSizeContainer::SetSizeI( const FDimension& D )
{
	guard(UFixedSizeContainer::SetSizeI);
	NOTE(debugf(TEXT("(%s)UFixedSizeContainer::SetSizeI( %s )"), *GetFullName(), D.String() ));

	Size.Width = D.Width;
	Size.Height = D.Height;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FDimension UFixedSizeContainer::GetSizeI()
{
	guard(UFixedSizeContainer::GetSizeI);
	NOTE(debugf(TEXT("(%s)UFixedSizeContainer::GetSizeI"), *GetFullName() ));

	return Size;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle UFixedSizeContainer::GetBoundsI()
{
	guard(UFixedSizeContainer::GetBoundsI);
	NOTE(debugf(TEXT("(%s)UFixedSizeContainer::GetBoundsI"), *GetFullName()));

	FPoint P = GetLocation();
	FDimension D = GetSize();

	return FRectangle(P.X,P.Y,D.Width,D.Height);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle UFixedSizeContainer::GetScreenBoundsI()
{
	guard(UFixedSizeContainer::GetScreenBoundsI);
	NOTE(debugf(TEXT("(%s)UFixedSizeContainer::GetScreenBoundsI"), *GetFullName()));

	FPoint P = GetScreenCoords();
	FDimension D = GetSize();

	return FRectangle(P.X,P.Y,D.Width,D.Height);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}




/*
	UFrame
*/

/* Fix ARL: Implement the following:
	ACCESSOR_FrameXAbsPct		- Percent of Width relative to Parent's Location	[	0.f + FrameXPct*GetWidth()		]
	ACCESSOR_FrameYAbsPct		- Percent of Height relative to Parent's Location	[	0.f + FrameYPct*GetHeight()		]
*/

//------------------------------------------------------------------------------
void UFrame::SetFrameLocation( const FPoint& P )
{
	guard(UFrame::SetFrameLocation);
	NOTE(debugf(TEXT("(%s)UFrame::SetFrameLocation(%s)"), *GetFullName(), *P.String() ));

	FrameLocation = P;

	SIMPLE_ACCESSORX(FrameLocationModify,&FrameLocation)
	SIMPLE_ACCESSOR_SET(FrameXPctModify,FLOAT,(FrameLocation.X-GetX())/GetWidth())	// Fix ARL: Need to disable other accessor (if set) to prevent infinite recursion.
	SIMPLE_ACCESSOR_SET(FrameYPctModify,FLOAT,(FrameLocation.Y-GetY())/GetHeight())

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FPoint UFrame::GetFrameLocation()
{
	guard(UFrame::GetFrameLocation);
	NOTE(debugf(TEXT("(%s)UFrame::GetFrameLocation"), *GetFullName() ));

	SIMPLE_ACCESSORX(FrameLocationAccess,&FrameLocation)

	IMPLEMENT_ACCESSOR_BEGIN(FrameXPctAccess,FLOAT)	// Fix ARL: Need to disable other accessor (if set) to prevent infinite recursion.
		FrameLocation.X = GetX() + GetWidth()*Result;
	IMPLEMENT_ACCESSOR_END(FrameXPctAccess)

	IMPLEMENT_ACCESSOR_BEGIN(FrameYPctAccess,FLOAT)
		FrameLocation.Y = GetY() + GetHeight()*Result;
	IMPLEMENT_ACCESSOR_END(FrameYPctAccess)

	return FrameLocation;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UFrame::SetFrameSize( const FDimension& D )
{
	guard(UFrame::SetFrameSize);
	NOTE(debugf(TEXT("(%s)UFrame::SetFrameSize(%s)"), *GetFullName(), *D.String() ));

	FrameSize = D;

	SIMPLE_ACCESSORX(FrameSizeModify,&FrameSize)
	SIMPLE_ACCESSOR_SET(FrameWidthPctModify,FLOAT,FrameSize.Width/GetWidth())	// Fix ARL: Need to disable other accessor (if set) to prevent infinite recursion.
	SIMPLE_ACCESSOR_SET(FrameHeightPctModify,FLOAT,FrameSize.Height/GetHeight())

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FDimension UFrame::GetFrameSize()
{
	guard(UFrame::GetFrameSize);
	NOTE(debugf(TEXT("(%s)UFrame::GetFrameSize"), *GetFullName() ));

	SIMPLE_ACCESSORX(FrameSizeAccess,&FrameSize)

	IMPLEMENT_ACCESSOR_BEGIN(FrameWidthPctAccess,FLOAT)	// Fix ARL: Need to disable other accessor (if set) to prevent infinite recursion.
		FrameSize.Width = GetWidth()*Result;
	IMPLEMENT_ACCESSOR_END(FrameWidthPctAccess)

	IMPLEMENT_ACCESSOR_BEGIN(FrameHeightPctAccess,FLOAT)
		FrameSize.Height = GetHeight()*Result;
	IMPLEMENT_ACCESSOR_END(FrameHeightPctAccess)

	return FrameSize;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
void UFrame::SetFrameWidthPct( FLOAT Pct )
{
	guard(UFrame::SetFrameWidthPct);
	NOTE(debugf(TEXT("(%s)UFrame::SetFrameWidthPct(%f)"), *GetFullName(), Pct ));

	SetFrameSize( FDimension(GetWidth()*Pct,GetFrameSize().Height) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UFrame::GetFrameWidthPct()
{
	guard(UFrame::GetFrameWidthPct);
	NOTE(debugf(TEXT("(%s)UFrame::GetFrameWidthPct"), *GetFullName() ));

	return GetFrameSize().Width / GetWidth();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UFrame::SetFrameHeightPct( FLOAT Pct )
{
	guard(UFrame::SetFrameHeightPct);
	NOTE(debugf(TEXT("(%s)UFrame::SetFrameHeightPct(%f)"), *GetFullName(), Pct ));

	SetFrameSize( FDimension(GetFrameSize().Width,GetHeight()*Pct) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UFrame::GetFrameHeightPct()
{
	guard(UFrame::GetFrameHeightPct);
	NOTE(debugf(TEXT("(%s)UFrame::GetFrameHeightPct"), *GetFullName() ));

	return GetFrameSize().Height / GetHeight();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UFrame::SetFrameXPct( FLOAT Pct )
{
	guard(UFrame::SetFrameXPct);
	NOTE(debugf(TEXT("(%s)UFrame::SetFrameXPct(%f)"), *GetFullName(), Pct ));

	SetFrameLocation( FPoint(GetWidth()*Pct,GetFrameLocation().Y) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UFrame::GetFrameXPct()
{
	guard(UFrame::GetFrameXPct);
	NOTE(debugf(TEXT("(%s)UFrame::GetFrameXPct"), *GetFullName() ));

	return GetFrameLocation().X / GetWidth();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UFrame::SetFrameYPct( FLOAT Pct )
{
	guard(UFrame::SetFrameYPct);
	NOTE(debugf(TEXT("(%s)UFrame::SetFrameYPct(%f)"), *GetFullName(), Pct ));

	SetFrameLocation( FPoint(GetFrameLocation().X,GetHeight()*Pct) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UFrame::GetFrameYPct()
{
	guard(UFrame::GetFrameYPct);
	NOTE(debugf(TEXT("(%s)UFrame::GetFrameYPct"), *GetFullName() ));

	return GetFrameLocation().Y / GetHeight();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
// Uses drawing bounds for alignment and such.
FRectangle UFrame::GetBoundsI()
{
	guard(UFrame::GetBoundsI);
	NOTE(debugf(TEXT("(%s)UFrame::GetBoundsI"), *GetFullName() ));

	FPoint P = GetLocation() + GetFrameLocation();
	FDimension D = GetFrameSize();

	return FRectangle(P.X,P.Y,D.Width,D.Height);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
// Uses drawing bounds for mouse selection and such.
FRectangle UFrame::GetScreenBoundsI()
{
	guard(UFrame::GetScreenBoundsI);
	NOTE(debugf(TEXT("(%s)UFrame::GetScreenBoundsI"), *GetFullName() ));

	FPoint P = GetScreenCoords() + GetFrameLocation();
	FDimension D = GetFrameSize();

	return FRectangle(P.X,P.Y,D.Width,D.Height);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
// Preserves normal behavior for GetSize (for Tweening, etc.).
FDimension UFrame::GetSizeI()
{
	guard(UFrame::GetSizeI);
	NOTE(debugf(TEXT("(%s)UFrame::GetSizeI"), *GetFullName() ));

	FRectangle R = Super::GetBoundsI();
	return FDimension(R.Width,R.Height);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UFrame::Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct )
{
	guard(UFrame::Tween);
	NOTE(debugf(TEXT("(%s)UFrame::Tween"), *GetFullName() ));

	Super::Tween( Source, Start, End, Pct );

	Pct = AdjustTweenPct( Pct, Source );	// Fix ARL: It would be nice to not have to calculate this twice.

	UFrame* StartP = DynamicCastChecked<UFrame>(Start);
	UFrame* EndP = DynamicCastChecked<UFrame>(End);

	FPoint P1 = StartP->GetFrameLocation(); FPoint P2 = EndP->GetFrameLocation(); /*if( P1!=P2 )*/ SetFrameLocation( P1.Blend(P2,Pct) );
	FDimension D1 = StartP->GetFrameSize(); FDimension D2 = EndP->GetFrameSize(); /*if( D1!=D2 )*/ SetFrameSize    ( D1.Blend(D2,Pct) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UFrame::Update(UPDATE_DEF)
{
	guard(UFrame::Update);
	NOTE0(debugf(TEXT("(%s)UFrame::Update"), *GetFullName() ));

	FDimension D = GetFrameSize();

	//optimization
	if( D.Width<=1.0f || D.Height<=1.0f )
		return;

	FPoint P = GetScreenCoords() + GetFrameLocation();

	Ren->PushClip(FRectangle(P.X,P.Y,D.Width,D.Height));

	Super::Update(UPDATE_PARMS);

	Ren->PopClip();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UHoldButton
*/

///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UHoldButton::SetMouse( UMouseCursor* M )
{
	guard(UHoldButton::SetMouse);
	NOTE(debugf(TEXT("(%s)UHoldButton::SetMouse"), *GetFullName() ));

	Image->SetMouse(M);
	SetMousePressEvent(M, UI_NATIVE_CALLBACK(MousePress,this,UHoldButton,MousePress));
	SetMouseReleaseEvent(M, UI_NATIVE_CALLBACK(MouseRelease,this,UHoldButton,MouseRelease));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UHoldButton::Update(UPDATE_DEF)
{
	guard(UHoldButton::Update);
	NOTE0(debugf(TEXT("(%s)UHoldButton::Update"), *GetFullName() ));

	// Make Image always think we're down while pressed.
	if( Mouse )
	{
		Image->bMouseClick_Over=true;
		Image->bMouseClick_PressedWhileOver=true;
	}

	// Do repeats if necessary.
	if( NextRepeatTime>0 && GetTimeSeconds()>=NextRepeatTime )
	{
		Click(Mouse);
		NextRepeatTime = GetTimeSeconds() + RepeatRate;
	}

	Super::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
void UHoldButton::MousePress( void* Parms )
{
	guard(UHoldButton::MousePress);
	NOTE(debugf(TEXT("(%s)UHoldButton::MousePress"), *GetFullName() ));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;
	UMouseCursor* M = P->Mouse;

	if( ContainsAbs( M->GetScreenCoords() ) )
	{
		Mouse = M;

		Click(Mouse);

		if( RepeatDelay>0 )
			NextRepeatTime = GetTimeSeconds() + RepeatDelay;

		P->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UHoldButton::MouseRelease( void* Parms )
{
	guard(UHoldButton::MouseRelease);
	NOTE(debugf(TEXT("(%s)UHoldButton::MouseRelease"), *GetFullName() ));

	if( Mouse )
	{
		Mouse = NULL;
		NextRepeatTime = 0;
		Image->bMouseClick_Over=false;
		Image->bMouseClick_PressedWhileOver=false;
		((MouseNotifyParms*)Parms)->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UHoldButton::Serialize( FArchive &Ar )
{
	guard(UHoldButton::Serialize);
	NOTE(debugf(TEXT("(%s)UHoldButton::Serialize"), *GetFullName() ));

	Super::Serialize(Ar);

	Ar << RepeatDelay << RepeatRate << NextRepeatTime;

	SERIALIZE_MOUSE(UHoldButton);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UImageComponent
*/

//------------------------------------------------------------------------------
void UImageComponent::SetImage( FImage* _Image )
{
	guard(UImageComponent::SetImage);
	NOTE(debugf(TEXT("(%s)UImageComponent::SetImage"), *GetFullName() ));

	Image = _Image;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FImage* UImageComponent::GetImage()
{
	guard(UImageComponent::GetImage);
	NOTE(debugf(TEXT("(%s)UImageComponent::GetImage"), *GetFullName() ));

	if( !Image )	// performance hack: let's just assume that Image never needs to change once set.
	{
		// NOTE[aleiby]: Remember to set your size since this won't be done automatically.
		SIMPLE_ACCESSORX(ImageComponent,&Image)
	}

	return Image;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UImage
*/

//------------------------------------------------------------------------------
FImage* UImage::AddImage(FString Name,FLOAT X,FLOAT Y,FLOAT W,FLOAT H,UBOOL bTile)
{
	guard(UImage::AddImage);
	NOTE(debugf(TEXT("(%s)UImage::AddImage( %s, %3.1f, %3.1f, %3.1f, %3.1f, %i )"), *GetFullName(), *Name, X, Y, W, H, bTile ));

	FImage* Image = new FImage(Name,Material,X,Y,W,H,bTile);
	Images.AddItem(Image);
	GetFImageMap().Set( Name, Image );
	return Image;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FImage* UImage::AddFullImage(FString Name,UBOOL bTile)
{
	guard(UImage::AddFullImage);
	NOTE(debugf(TEXT("(%s)UImage::AddFullImage( %s, %i )"), *GetFullName(), *Name, bTile ));

	if(!Material)
	{
		debugf(NAME_Init,TEXT("AddFullImage: Material not set yet! '%s'"),*Name);
		return NULL;
	}

	return AddImage(Name,0,0,Material->MaterialUSize(),Material->MaterialVSize(),bTile);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FImage* UImage::FindImage(FString Name)
{
	guard(UImage::AddImage);
	NOTE(debugf(TEXT("(%s)UImage::FindImage( %s )"), *GetFullName(), *Name ));

	for(INT i=0;i<Images.Num();i++)
		if(Images(i)->Name==Name)
			return Images(i);

	return NULL;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UImageComponentBase
*/


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UImageComponentBase::Init()
{
	guard(UImageComponentBase::Init);
	NOTE(debugf(TEXT("(%s)UImageComponentBase::Init"), *GetFullName() ));

	FImage* I = GetImage();
	if( I && GetSize()==FDimension(0,0) )
		SetSize( FDimension(I->W,I->H) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UImageComponentBase::Update(UPDATE_DEF)
{
	guard(UImageComponentBase::Update);
	NOTE0(debugf(TEXT("%f: (%s)UImageComponentBase::Update( %s, %s, %s )"), GetTimeSeconds(), *GetFullName(), *GetScreenCoords().String(), *GetSize().String(), GetImage() ? *GetImage()->Name : TEXT("Image not set") ));

	FImage* I = GetImage();
	if(!I) return;

	FPoint P = GetScreenCoords();	// Fix ARL: Wouldn't it make more sense for Update to pass in the relative screen coords (so we don't have to be constantly calculating them all the way up to the root for every component)?
	FDimension D = GetSize();

	Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width,D.Height), 0, GetPolyFlags(), GetColor() );

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UImageWedge
*/


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
FLOAT UImageWedge::GetRadius()
{
	guard(UImageWedge::GetRadius);
	NOTE(debugf(TEXT("(%s)UImageWedge::GetRadius"),*GetFullName()));

	SIMPLE_ACCESSORX(ImageWedgeRadius,&Radius)

	return Radius;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UImageWedge::GetAngle()
{
	guard(UImageWedge::GetAngle);
	NOTE(debugf(TEXT("(%s)UImageWedge::GetAngle"),*GetFullName()));

	SIMPLE_ACCESSORX(ImageWedgeAngle,&Angle)

	return Angle;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UImageWedge::Update(UPDATE_DEF)
{
	guard(UImageWedge::Update);
	NOTE(debugf(TEXT("(%s)UImageWedge::Update( %s, %s, %s )"), *GetFullName(), *GetScreenCoords().String(), *GetSize().String(), GetImage() ? *GetImage()->Name : TEXT("Image not set") ));

	// Fix ARL: This could be greatly simplified (and optimized) using FMatrix, etc.

	FImage* I=GetImage();
	if(!I) return;

	UMaterial* M=I->Material;
	if(!M) return;

	FLOAT Radius = GetRadius();
	FLOAT Angle = GetAngle();

	FDimension D = GetSize();

	SetSizeC(0,0);	//ignore size for screen coord calcs.
	FPoint P = GetScreenCoords();
	SetSize(D);

	// Calc rotated vertices.
	FLOAT theta = Angle * PI / 180.f;
	FPoint Dir = FPoint( appCos(theta), appSin(theta) );
	FPoint Inv = FPoint( -Dir.Y, Dir.X );

	FPoint InnerY = Dir * Radius;
	FPoint OuterY = Dir * (Radius+D.Height);
	FPoint PerpWidth = Inv * (D.Width / 2.0f);

	FPoint P1 = P + OuterY - PerpWidth;
	FPoint P2 = P + OuterY + PerpWidth;
	FPoint P3 = P + InnerY + PerpWidth;
	FPoint P4 = P + InnerY - PerpWidth;

	FLOAT	U0=I->X,
			V0=I->Y,
			U1=I->X+I->W,
			V1=I->Y+I->H;

	U0/=M->MaterialUSize();
	V0/=M->MaterialVSize();
	U1/=M->MaterialUSize();
	V1/=M->MaterialVSize();

	NOTE(debugf(TEXT("%s %s %s %s"), *P1.String(), *P2.String(), *P3.String(), *P4.String() ));

	FQuadInfo Quad;

	Quad.Pts[0].X=P1.X;
	Quad.Pts[0].Y=P1.Y;
	Quad.Pts[0].U=U0;
	Quad.Pts[0].V=V0;

	Quad.Pts[1].X=P2.X;
	Quad.Pts[1].Y=P2.Y;
	Quad.Pts[1].U=U1;
	Quad.Pts[1].V=V0;

	Quad.Pts[2].X=P3.X;
	Quad.Pts[2].Y=P3.Y;
	Quad.Pts[2].U=U1;
	Quad.Pts[2].V=V1;

	Quad.Pts[3].X=P4.X;
	Quad.Pts[3].Y=P4.Y;
	Quad.Pts[3].U=U0;
	Quad.Pts[3].V=V1;

	Ren->DrawQuad( M, Quad, 0, GetPolyFlags(), GetColor() );

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UImageWedge::Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct )
{
	guard(UImageWedge::Tween);
	NOTE(debugf(TEXT("(%s)UImageWedge::Tween"), *GetFullName() ));

	Super::Tween( Source, Start, End, Pct );

	UImageWedge* StartP = DynamicCast<UImageWedge>(Start);
	UImageWedge* EndP = DynamicCast<UImageWedge>(End);

	if( StartP && EndP )
	{
		Pct = AdjustTweenPct( Pct, Source );	// Fix ARL: It would be nice to not have to calculate this twice.
		Radius = appBlend( StartP->GetRadius(), EndP->GetRadius(), Pct );
		Angle = appBlend( StartP->GetAngle(), EndP->GetAngle(), Pct );
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UImageWipe
*/


//------------------------------------------------------------------------------
void UImageWipe::SetTopImage( FImage* _Image )
{
	guard(UImageWipe::SetTopImage);
	NOTE(debugf(TEXT("(%s)UImageWipe::SetTopImage"), *GetFullName() ));

	check(_Image!=NULL);
	TopImage = NEW_SUBCOMPONENT(UImageComponent);
	AddComponent(TopImage);
	TopImage->Image = _Image;
	TopImage->Init();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UImageWipe::SetBottomImage( FImage* _Image )
{
	guard(UImageWipe::SetBottomImage);
	NOTE(debugf(TEXT("(%s)UImageWipe::SetBottomImage"), *GetFullName() ));

	check(_Image!=NULL);
	BottomImage = NEW_SUBCOMPONENT(UImageComponent);
	AddComponent(BottomImage);
	BottomImage->Image = _Image;
	BottomImage->Init();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UImageWipe::SetTopImageComponent( UImageComponent* C )
{
	guard(UImageWipe::SetTopImageComponent);
	NOTE(debugf(TEXT("(%s)UImageWipe::SetTopImageComponent"), *GetFullName() ));

	TopImageComponent = C;
	AddComponent(C);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


//------------------------------------------------------------------------------
void UImageWipe::SetBottomImageComponent( UImageComponent* C )
{
	guard(UImageWipe::SetBottomImageComponent);
	NOTE(debugf(TEXT("(%s)UImageWipe::SetBottomImageComponent"), *GetFullName() ));

	BottomImageComponent = C;
	AddComponent(C);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UImageWipe::Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT InPct )
{
	guard(UImageWipe::Tween);
	NOTE(debugf(TEXT("(%s)UImageWipe::Tween"), *GetFullName() ));

	Super::Tween( Source, Start, End, InPct );

	InPct = AdjustTweenPct(InPct,Source);	// Fix ARL: It would be nice to not have to calculate this twice.

	UImageWipe* StartIW = DynamicCastChecked<UImageWipe>(Start);
	UImageWipe* EndIW = DynamicCastChecked<UImageWipe>(End);

	FLOAT Pct1 = StartIW->Pct;
	FLOAT Pct2 = EndIW->Pct;
	Pct = appBlend(Pct1,Pct2,InPct);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UImageWipe::Update(UPDATE_DEF)
{
	guard(UImageWipe::Update);
	NOTE(debugf(TEXT("(%s)UImageWipe::Update"), *GetFullName() ));

	SIMPLE_ACCESSORX(ImageWipe,&Pct)
	Pct = Clamp(Pct,0.f,1.f);

	if( BottomImageComponent )
	{
		BottomImageComponent->Update(UPDATE_PARMS);
	}
	else if( BottomImage )
	{
		FPoint P = GetScreenCoords();
		FDimension D = GetSize();
		FImage* I = BottomImage->GetImage();
		Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width,D.Height), 0, GetPolyFlags(), GetColor() );
	}

	if( TopImageComponent )
	{
		FPoint P = TopImageComponent->GetScreenCoords();
		FDimension D = TopImageComponent->GetSize();
		FImage* I = TopImageComponent->GetImage();
		FLOAT W=I->W, H=I->H;
		if(bInvert)
		{
			FLOAT _X=I->X, _Y=I->Y;
			if(bVertical)	{ FLOAT Y=I->Y+I->H; I->H*=Pct; I->Y=Y-I->H; FLOAT H=D.Height*Pct; Ren->DrawTile( I, FRectangle(P.X,P.Y+D.Height-H,D.Width,H), 0, GetPolyFlags(), GetColor() ); }
			else			{ FLOAT X=I->X+I->W; I->W*=Pct; I->X=X-I->W; FLOAT W=D.Width*Pct;  Ren->DrawTile( I, FRectangle(P.X+D.Width-W,P.Y,W,D.Height), 0, GetPolyFlags(), GetColor() ); }
			I->X=_X; I->Y=_Y;
		}
		else
		{
			if(bVertical)	{ I->H*=Pct; Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width,D.Height*Pct), 0, GetPolyFlags(), GetColor() ); }
			else			{ I->W*=Pct; Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width*Pct,D.Height), 0, GetPolyFlags(), GetColor() ); }
		}
		I->W=W; I->H=H;
	}
	else if( TopImage )
	{
		FPoint P = GetScreenCoords();
		FDimension D = GetSize();
		FImage* I = TopImage->GetImage();
		FLOAT W=I->W, H=I->H;
		if(bInvert)
		{
			FLOAT _X=I->X, _Y=I->Y;
			if(bVertical)	{ FLOAT Y=I->Y+I->H; I->H*=Pct; I->Y=Y-I->H; FLOAT H=D.Height*Pct; Ren->DrawTile( I, FRectangle(P.X,P.Y+D.Height-H,D.Width,H), 0, GetPolyFlags(), GetColor() ); }
			else			{ FLOAT X=I->X+I->W; I->W*=Pct; I->X=X-I->W; FLOAT W=D.Width*Pct;  Ren->DrawTile( I, FRectangle(P.X+D.Width-W,P.Y,W,D.Height), 0, GetPolyFlags(), GetColor() ); }
			I->X=_X; I->Y=_Y;
		}
		else
		{
			if(bVertical)	{ I->H*=Pct; Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width,D.Height*Pct), 0, GetPolyFlags(), GetColor() ); }
			else			{ I->W*=Pct; Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width*Pct,D.Height), 0, GetPolyFlags(), GetColor() ); }
		}
		I->W=W; I->H=H;
	}

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UImageWipe::Serialize( FArchive &Ar )
{
	guard(UImageWipe::Serialize);
	NOTE(debugf(TEXT("(%s)UImageWipe::Serialize"), *GetFullName()));

	Super::Serialize(Ar);

	Ar << Pct;

	SERIALIZE_BOOL(bVertical);

	SERIALIZE_INDEXED_COMPONENT(UImageComponent,TopImage);
	SERIALIZE_INDEXED_COMPONENT(UImageComponent,BottomImage);
	SERIALIZE_INDEXED_COMPONENT(UImageComponent,TopImageComponent);
	SERIALIZE_INDEXED_COMPONENT(UImageComponent,BottomImageComponent);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UInputSelector
*/


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
// Setup click notification.
//------------------------------------------------------------------------------
void UInputSelector::SetMouse( UMouseCursor* M )
{
	guard(UInputSelector::SetMouse);
	NOTE(debugf(TEXT("(%s)UInputSelector::SetMouse"), *GetFullName() ));

	SetMouseClickEvent( M, UI_NATIVE_CALLBACK(MouseClick,this,UInputSelector,Click) );
	RegisterEvent( UI_EVENT(Reset), this );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UInputSelector::Update(UPDATE_DEF)
{
	guard(UInputSelector::Update);
	NOTE0(debugf(TEXT("%f: (%s)UInputSelector::Update"), GetTimeSeconds(), *GetFullName() ));

	IMPLEMENT_ACCESSOR_BEGIN(InputSelectorDisplay,FString)
		SetText( Result, true );
	IMPLEMENT_ACCESSOR_END(InputSelectorDisplay)

	Super::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UInputSelector::HandleEvent( uiEvent Event )
{
	guard(UInputSelector::HandleEvent);
	NOTE(debugf(TEXT("(%s)UInputSelector::HandleEvent( %s )"), *GetFullName(), *LookupEventName(Event) ));

	if( Event==UI_EVENT(Reset) )
	{
		FString KeyName=GetText();
		SIMPLE_ACCESSORX(InputSelectorReset,&KeyName)
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UInputSelector::Global_Click()
{
	guard(UInputSelector::Global_Click);
	NOTE(debugf(TEXT("(%s)UInputSelector::Global_Click"), *GetFullName() ));

	StartListening();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

// Fix ARL: Grey-out if locked.

//------------------------------------------------------------------------------
void UInputSelector::Global_StartListening()
{
	guard(UInputSelector::Global_StartListening);
	NOTE(debugf(TEXT("(%s)UInputSelector::Global_StartListening"), *GetFullName() ));

	GotoState_Listening();	// called first to ensure KeyEvent is in scope.
	CastChecked<UUIConsole>(GetConsole())->AddInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UInputSelector,KeyEvent) );		// Fix ARL: Memory leak.
	SIMPLE_ACCESSOR(StartListening)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UInputSelector::Listening_KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
{
	guard(UInputSelector::Listening_KeyEvent);
	NOTE(debugf(TEXT("(%s)UInputSelector::Listening_KeyEvent"), *GetFullName() ));

	ProcessKey( Key, GetEnumEx(Engine.EInputKey,Key,3) );
	return true;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UInputSelector::Listening_ProcessKey( INT Key, FString KeyName )
{
	guard(UInputSelector::Listening_ProcessKey);
	NOTE(debugf(TEXT("(%s)UInputSelector::Listening_ProcessKey"), *GetFullName() ));

/* Fix ARL: Replace with TArray<Range> IgnoredRanges;
	if
	(	((Key>=0x70) && (Key<=0x81))	// function keys
	||	((Key>=0x30) && (Key<=0x39))	// number keys
	)	return;
*/
	if(	KeyName!=TEXT("") && KeyName!=TEXT("Escape") )
	{
		SetText( KeyName, true );
		SIMPLE_ACCESSORX(InputSelector,&KeyName)
	}

	StopListening();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UInputSelector::Listening_StopListening()
{
	guard(UInputSelector::Listening_StopListening);
	NOTE(debugf(TEXT("(%s)UInputSelector::Listening_StopListening"), *GetFullName() ));

	SIMPLE_ACCESSOR(StopListening)
	CastChecked<UUIConsole>(GetConsole())->RemoveInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UInputSelector,KeyEvent) );		// Fix ARL: Memory leak. : We should store a pointer to the one that was orignally passed in.
	GotoState_None();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FColor UInputSelector::Listening_GetColor()
{
	guard(UInputSelector::Listening_GetColor);
	NOTE(debugf(TEXT("(%s)UInputSelector::Listening_GetColor"), *GetFullName() ));

	return SelectedColor;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UKeyEvent
*/


//////////////////////
// Keyboard support //
//////////////////////

//------------------------------------------------------------------------------
void UKeyEvent::SetConsole( UUIConsole* C )
{
	guard(UKeyEvent::SetConsole);
	NOTE(debugf(TEXT("(%s)UKeyEvent::SetConsole"), *GetFullName() ));

	TArray<EInputAction> Actions;
	for( INT i=0; i<Events.Num(); i++ )
		Actions.AddUniqueItem(Events(i).Action);
	for( INT i=0; i<Actions.Num(); i++ )
		C->AddInputActionListener( Actions(i), this, UI_NATIVE_CALLBACK(None,this,UKeyEvent,KeyEvent) );		// Fix ARL: Memory leak.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UKeyEvent::Destroy()
{
	guard(UKeyEvent::Destroy);
	NOTE(debugf(TEXT("UKeyEvent::Destroy")));

	UUIConsole* C = CastChecked<UUIConsole>(GetConsole());

	TArray<EInputAction> Actions;
	for( INT i=0; i<Events.Num(); i++ )
		Actions.AddUniqueItem(Events(i).Action);
	for( INT i=0; i<Actions.Num(); i++ )
		C->RemoveInputActionListener( Actions(i), this, UI_NATIVE_CALLBACK(None,this,UKeyEvent,KeyEvent) );		// Fix ARL: Memory leak.

	Super::Destroy();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UKeyEvent::KeyEvent( void* Parms )
{
	guard(UKeyEvent::KeyEvent);
	NOTE(debugf(TEXT("(%s)UKeyEvent::KeyEvent: Enabled=%d"), *GetFullName(), IsEnabled() ));

	KeyEventParms* P = (KeyEventParms*)Parms;

	EInputKey		Key		= P->Key;
	EInputAction	Action	= P->Action;
//	FLOAT			Delta	= P->Delta;

	// NOTE[aleiby]:
	// Send all events for the matching key, but only propogate to other KeyEvents if
	// we are disabled, or there are no matching keys.  This prevents a cascading effect
	// where the first keyevent enables the next, and then that one gets handled prematurely.
	// (This prevents us having multiple keyevents that handle the same event simultaneously active,
	// but that's generally a bad idea anyway -- just put multiple events in a single keyevent instead.)

	if( !IsEnabled() ){ P->bHandled=false; return; }

	NOTE(debugf(TEXT("KeyEvent is enabled: %s"), *GetFullName()));

	for( INT i=0; i<Events.Num(); i++ )
	{
		if( Key==Events(i).Key && Action==Events(i).Action )
		{
			Events(i).Event->OnEvent();
			P->bHandled=true;
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UKeyEvent::AddEvent( EInputKey Key, EInputAction Action, ActionEvent* Event )
{
	guard(UKeyEvent::AddEvent);
	NOTE(debugf(TEXT("(%s)UKeyEvent::AddEvent"), *GetFullName() ));

	new(Events)FKeyEvent(Key,Action,Event);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UKeyEvent::SetEnabledI( UBOOL _bEnabled )
{
	guard(UKeyEvent::SetEnabledI);
	NOTE(debugf(TEXT("(%s)UKeyEvent::SetEnabledI(%s)"), *GetFullName(), _bEnabled ? TEXT("True") : TEXT("False") ));

	bEnabled = _bEnabled;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL UKeyEvent::IsEnabledI()
{
	guard(UKeyEvent::IsEnabledI);
	NOTE(debugf(TEXT("(%s)UKeyEvent::IsEnabledI"), *GetFullName() ));

	return bEnabled;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	ULabel
*/


//------------------------------------------------------------------------------
void ULabel::SetText( const FString& NewText, UBOOL bNoUpdateSize )
{
	guard(ULabel::SetText);
	NOTE(debugf(TEXT("(%s)ULabel::SetText"), *GetFullName() ));

	Text = NewText;

	if( bWrap )
	{
		FDimension OldD=GetSize();
		AdjustSizeFromText( Text );
		FDimension NewD=GetSize();
		INT NumWraps = appFloor(NewD.Width / OldD.Width);
		OldD.Height *= NumWraps+1;
		SetSize(OldD);
		return;
	}

	if( !bNoUpdateSize )
		AdjustSizeFromText( Text );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FString ULabel::GetText()
{
	guard(ULabel::GetText);
	NOTE(debugf(TEXT("(%s)ULabel::GetText"), *GetFullName() ));

	IMPLEMENT_ACCESSOR_BEGIN(Label,FString)
		SetText( Result );
	IMPLEMENT_ACCESSOR_END(Label)

	IMPLEMENT_ACCESSOR_BEGIN(LabelFloatAccess,FLOAT)
		SetText( FString::Printf(*Format,Result) );
	IMPLEMENT_ACCESSOR_END(LabelFloatAccess)

	return Text;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FDimension ULabel::GetSizeI()
{
	guard(ULabel::GetSizeI);
	NOTE(debugf(TEXT("(%s)ULabel::GetSizeI"), *GetFullName() ));

	// Easiest way to update the size using the latest text.
	GetText();

	return Super::GetSizeI();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void ULabel::Update(UPDATE_DEF)
{
	guard(ULabel::Update);
	NOTE0(debugf(TEXT("%f: (%s)ULabel::Update"), GetTimeSeconds(), *GetFullName() ));

	//NOTE[aleiby]: Clipping doesn't work properly (like when centered and tweened in a frame).  Commented out part corrects this, but screws up frames in general.  Need better global solution.

	UCanvas* C=Frame->Viewport->Canvas;

	FPoint P=GetScreenCoords();
	FDimension D=GetSize();

	UFont* Font = GetFont(C);

	Ren->PushClip(FRectangle(P.X,P.Y,D.Width,D.Height));

	FLOAT PX=0, PY=0;

	if( bWrap )
	{
		// Fix ARL: This seems like it might be extra slow.

		FLOAT Width = D.Width;
		FLOAT Height = GetFontHeight(Font);

		PX=P.X; PY=P.Y;

		struct THelper
		{
			FString GetNextWord( FString &Text )
			{
				FString S = Text;
				INT InPos = S.InStr(TEXT(" "));
				if( InPos<0 )
				{
					Text=FString(TEXT(""));
					return S;
				}
				else
				{
					Text=S.Mid(InPos+1);
					return S.Left(InPos+1);
				}
			}

			INT GetStrWidth( const FString &S, UCanvas* C, UFont* F )
			{
				FLOAT W,H;
				C->TextSize(F,*S,W,H);
				return W;
			}

		} Helper;

		FString Text = GetText();
		FString ClippedLine = Helper.GetNextWord(Text);
		while( ClippedLine!=TEXT("") )
		{
			FString NextWord = Helper.GetNextWord(Text);
			while( NextWord!=TEXT("") && Helper.GetStrWidth( ClippedLine + NextWord, C, Font ) <= Width )
			{
				ClippedLine += NextWord;
				NextWord = Helper.GetNextWord(Text);
			}

			Ren->DrawString(*ClippedLine,PX,PY,Font,0,GetPolyFlags(),GetColor());

			ClippedLine = NextWord;
			PY += Height;

			// Break out early if we go beyond the clip plane.
			if(PY>C->ClipY) break;	//!!ARL
		}

		// Update new size based on text wrapping.
		SetSizeC(Width,PY-P.Y);
	}
	else
	{
		FString Text = GetText();

		// Align text within our bounds.
		FAlignment A=GetAlignment();
		if( (A.XAxis==ALMT_Right) || (A.XAxis==ALMT_Center) || (A.YAxis==ALMT_Bottom) || (A.YAxis==ALMT_Center) )
		{
			FLOAT Width=0, Height=0;
			C->TextSize( Font, *Text, Width, Height );

			switch( A.XAxis )
			{
			case ALMT_Right:	PX = D.Width - Width;			break;
			case ALMT_Center:	PX = D.Width/2 - Width/2;		break;
			}

			switch( A.YAxis )
			{
			case ALMT_Bottom:	PY = D.Height - Height;			break;
			case ALMT_Center:	PY = D.Height/2 - Height/2;		break;
			}
		}

		Ren->DrawString(*Text,P.X+PX,P.Y+PY,Font,0,GetPolyFlags(),GetColor());
	}

	Ren->PopClip();

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}



/*
	ULabelShadow
*/

//------------------------------------------------------------------------------
void ULabelShadow::SetAlphaI( BYTE A )
{
	guard(ULabelShadow::SetAlphaI);
	NOTE(debugf(TEXT("ULabelShadow::SetAlphaI")));

	FLOAT Alpha = GetAlpha();
	ShadowColor.A = Alpha ? (FLOAT)ShadowColor.A * (FLOAT)A / Alpha : A;
	Super::SetAlphaI(A);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void ULabelShadow::Update(UPDATE_DEF)
{
	guard(ULabelShadow::Update);
	NOTE0(debugf(TEXT("%f: (%s)ULabelShadow::Update"), GetTimeSeconds(), *GetFullName() ));

	// Fix ARL: Factor out more superclass update code to make this more optimized.

	// Draw shadow first.
	FColor OldColor = GetColor();
	FPoint OldLocation = GetLocation();
	SetColor(ShadowColor);
	SetLocation(OldLocation+ShadowOffset);
	Super::Update(UPDATE_PARMS);
	SetColor(OldColor);
	SetLocation(OldLocation);

	// Draw normally.
	DWORD OldAccessFlags = AccessFlags;
	AccessFlags &= ~(ACCESSOR_Label | ACCESSOR_LabelFloatAccess);	// no need to update accessors twice.
	Super::Update(UPDATE_PARMS);
	AccessFlags = OldAccessFlags;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}



/*
	UMultiImageComponent
*/

//------------------------------------------------------------------------------
INT UMultiImageComponent::GetIndex()
{
	guard(UMultiImageComponent::GetIndex);
	NOTE(debugf(TEXT("(%s)UMultiImageComponent::GetIndex"), *GetFullName() ));

	// Fix ARL: This could use some smart caching.
	SIMPLE_ACCESSORX(MultiImageComponent,&Index)

	check(Index>=0);

	return Index;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FDimension UMultiImageComponent::GetSizeI()
{
	guard(UMultiImageComponent::GetSizeI);
	NOTE(debugf(TEXT("(%s)UMultiImageComponent::GetSizeI"), *GetFullName() ));

	if( bAutoResize )
	{
		FImage* I = GetImage();
		FDimension D = Super::GetSizeI();
		if( D.Width!=I->W || D.Height!=I->H )
			SetSize( FDimension(I->W,I->H) );
	}

	return Super::GetSizeI();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiImageComponent::Init()
{
	guard(UMultiImageComponent::Init);
	NOTE(debugf(TEXT("(%s)UMultiImageComponent::Init"), *GetFullName() ));

	if( !bAutoResize )
		Super::Init();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMultiImageComponent::SetImage( FImage* _Image )
{
	guard(UMultiImageComponent::SetImage);
	NOTE(debugf(TEXT("(%s)UMultiImageComponent::SetImage"), *GetFullName() ));

	Images.AddItem(_Image);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FImage* UMultiImageComponent::GetImage()
{
	guard(UMultiImageComponent::GetImage);
	NOTE(debugf(TEXT("(%s)UMultiImageComponent::GetImage: Index=%d Num=%d"), *GetFullName(), GetIndex(), Images.Num() ));

	return Images( GetIndex() );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}



/*
	UNumWatcher
*/


//------------------------------------------------------------------------------
void UNumWatcher::Update(UPDATE_DEF)
{
	guard(UNumWatcher::Update);
	NOTE0(debugf(TEXT("(%s)UNumWatcher::Update"), *GetFullName() ));

	UCanvas* C=Frame->Viewport->Canvas;

	FPoint P = GetScreenCoords();

	if( bFloat )
	{
		IMPLEMENT_ACCESSOR_BEGIN(NumWatcherWatch,FLOAT)
			Ren->DrawString(*FString::Printf(TEXT("%f"),Result),P.X,P.Y,GetFont(C),0,GetPolyFlags(),GetColor());
		IMPLEMENT_ACCESSOR_END(NumWatcherWatch)
	}
	else
	{
		IMPLEMENT_ACCESSOR_BEGIN(NumWatcherWatch,INT)
			Ren->DrawString(*FString::Printf(TEXT("%d"),Result),P.X,P.Y,GetFont(C),0,GetPolyFlags(),GetColor());
		IMPLEMENT_ACCESSOR_END(NumWatcherWatch)
	}

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UPlayerID
*/


//------------------------------------------------------------------------------

void UPlayerID::Update(UPDATE_DEF)
{
	guard(UPlayerID::Update);
	NOTE0(debugf(TEXT("(%s)UPlayerID::Update"),*GetFullName()));

#if 1 

	check(Frame);
	check(Frame->Viewport);
	check(GetPlayerOwner());

	struct Local
	{
		static UBOOL GetPawnTeam( APawn* P, ATeamInfo*& PawnTeam )
		{
			PawnTeam = NULL;
			if( P && P->PlayerReplicationInfo )
				PawnTeam = P->PlayerReplicationInfo->Team;
			return( PawnTeam != NULL );
		}
	};

	ATeamInfo* OwnerTeam;
	APawn*     OwnerPawn = GetPlayerOwner()->Pawn;

	if( !Local::GetPawnTeam( OwnerPawn, OwnerTeam ) )
		return;

	FCanvasUtil CanvasUtil(&Frame->Viewport->RenderTarget, Frame->Viewport->RI);

	ATeamInfo* CurrentTeam;

	// Add new components for each of the known pawns.
	for( int i=0; i < Frame->PawnList.Num(); i++ )
	{
		CurrentPawn = Frame->PawnList(i);

		if( Local::GetPawnTeam( CurrentPawn, CurrentTeam ) &&
			CurrentPawn != OwnerPawn &&
			CurrentTeam == OwnerTeam )
		{
			FVector Loc = CurrentPawn->Location;
			FBox box = CurrentPawn->GetCollisionBoundingBox();
			Loc.Z += box.Max.Z - box.Min.Z;				// info on top of pawns bounding box
			if( ((Loc - Frame->ViewOrigin) | Frame->CameraToWorld.TransformNormal(FVector(0,0,1))) > 0 )
			{
				FVector Screen = CanvasUtil.ScreenToCanvas.TransformFVector(Frame->Project(Loc));
				SetLocationC( Screen.X, Screen.Y );
				// TBI: Get pawn's bounding box size and scale height accordingly
				//SetSizeC( NewSize.Width, NewSize.Height );
				Super::Update(UPDATE_PARMS);
			}
		}
	}

#elif 1 //NEW: Caching

	INT iComponent=0;
	TArray<FPoint> OriginalLocs;

	// Add new components for each of the known pawns.
	AController* PlayerOwner = GetPlayerOwner();
	for( AController* IterP=PlayerOwner->GetLevel()->GetLevelInfo()->ControllerList; IterP; IterP=IterP->nextController )
	{
 		if( IterP!=PlayerOwner && IterP->Pawn )
		{
			FVector Loc = IterP->Pawn->Location;
			Loc.Z += AdditionalHeight;
			if( ((Loc - Frame->ViewOrigin) | Frame->CameraToWorld.TransformNormal(FVector(0,0,1))) > 0 )
			{
				UComponent* C;
				if( iComponent < Components.Num() )
				{
					C = Components(iComponent);
				}
				else
				{
					C = GetUIConsole()->LoadComponent( Filename, ComponentName );
					AddComponent(C);
				}
				iComponent++;

//!!			C->Owner = IterP->Pawn;
				C->SetEnabled(1);
				OriginalLocs.AddItem(C->GetLocation());
/*!!MERGE
				FCanvasUtil CanvasUtil(Frame->Viewport,Frame->Viewport->RI);
				FVector Screen = CanvasUtil.ScreenToCanvas.TransformFVector(Frame->Project(Loc));
				C->SetLocation( FPoint(Screen.X,Screen.Y) + C->GetLocation() - GetScreenCoords() );
*/
			}
		}
	}

	// Disabled remaining unused components.
	while( iComponent<Components.Num() )
		Components(iComponent++)->SetEnabled(0);

	Super::Update(UPDATE_PARMS);

	// Restore original locations for reuse.
	for( INT i=0;i<OriginalLocs.Num();i++ )
		Components(i)->SetLocation(OriginalLocs(i));

#else

	// Fix ARL: This seems like an awfully wasteful way of doing things.  We should be caching this stuff.

	// Remove existing components.
	while( Components.Num() )
		Components(0)->DeleteUObject();	// This will cause it to remove itself from our list.

	// Add new components for each of the known pawns.
	AController* PlayerOwner = GetPlayerOwner();
	for( AController* IterP=PlayerOwner->GetLevel()->GetLevelInfo()->ControllerList; IterP; IterP=IterP->nextController )
	{
 		if( IterP!=PlayerOwner && IterP->Pawn )
		{
			FVector Loc = IterP->Pawn->Location;
			Loc.Z += AdditionalHeight;
			if( ((Loc - Frame->ViewOrigin) | Frame->CameraToWorld.TransformNormal(FVector(0,0,1))) > 0 )
			{
				GetObjMap().Set( TEXT("Pawn"), IterP->Pawn );
				UComponent* C = GetUIConsole()->LoadComponent( Filename, ComponentName );
				AddComponent(C);
				FCanvasUtil CanvasUtil(Frame->Viewport,Frame->Viewport->RI);
				FVector Screen = CanvasUtil.ScreenToCanvas.TransformFVector(Frame->Project(Loc));
				C->SetLocation( FPoint(Screen.X,Screen.Y) + C->GetLocation() - GetScreenCoords() );
			}
		}
	}

	Super::Update(UPDATE_PARMS);

#endif

	unguardf(( TEXT("(%s)"), *GetFullName() ));

}



/*
	URealComponent
*/



///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
FDimension URealComponent::GetSizeI()
{
	guard(URealComponent::GetSizeI);
	NOTE(debugf(TEXT("(%s)URealComponent::GetSizeI( %s )"), *GetFullName(), *Size.String() ));

	return Size;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::SetSizeI( const FDimension& D )
{
	guard(URealComponent::SetSizeI);
	NOTE(debugf(TEXT("(%s)URealComponent::SetSizeI( %s )"), *GetFullName(), D.String() ));

	Size.Width = D.Width;
	Size.Height = D.Height;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
ERenderStyle URealComponent::GetStyleI()
{
	guard(URealComponent::GetStyleI);
	NOTE(debugf(TEXT("(%s)URealComponent::GetStyleI"), *GetFullName() ));

	return (ERenderStyle)Style;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::SetStyleI( ERenderStyle S )
{
	guard(URealComponent::SetStyleI);
	NOTE(debugf(TEXT("(%s)URealComponent::SetStyleI( %s )"), *GetFullName(), GetEnumEx(Engine.ERenderStyle,S,4) ));

	Style = S;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FColor URealComponent::GetColorI()
{
	guard(URealComponent::GetColorI);
	NOTE(debugf(TEXT("(%s)URealComponent::GetColorI: %s"), *GetFullName(), DrawColor.String() ));

	return DrawColor;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::SetColorI( const FColor& C )
{
	guard(URealComponent::SetColorI);
	NOTE(debugf(TEXT("(%s)URealComponent::SetColorI( %s )"), *GetFullName(), C.String() ));

	DrawColor = C;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
BYTE URealComponent::GetAlphaI()
{
	guard(URealComponent::GetAlphaI);
	NOTE(debugf(TEXT("(%s)URealComponent::GetAlphaI"), *GetFullName() ));

	return DrawColor.A;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::SetAlphaI( BYTE A )
{
	guard(URealComponent::SetAlphaI);
	NOTE(debugf(TEXT("(%s)URealComponent::SetAlphaI( %b )"), *GetFullName(), A ));

	DrawColor.A = A;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::SetVisibilityI( UBOOL bVisible )
{
	guard(URealComponent::SetVisibilityI);
	NOTE(debugf(TEXT("(%s)URealComponent::SetVisibilityI( %s )"), *GetFullName(), bVisible ? TEXT("True") : TEXT("False") ));

	this->bVisible = bVisible;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL URealComponent::IsVisibleI()
{
	guard(URealComponent::IsVisibleI);
	NOTE(debugf(TEXT("(%s)URealComponent::IsVisibleI"), *GetFullName() ));

	return bVisible;	// Fix ARL: Do more stuff?

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL URealComponent::IsShowingI()
{
	guard(URealComponent::IsShowingI);
	NOTE(debugf(TEXT("(%s)URealComponent::IsShowingI"), *GetFullName() ));

	return bVisible;	// Fix ARL: Do more stuff?

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL URealComponent::IsDisplayableI()
{
	guard(URealComponent::IsDisplayableI);
	NOTE(debugf(TEXT("(%s)URealComponent::IsDisplayableI"), *GetFullName() ));

	return bVisible;	// Fix ARL: Do more stuff?

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::SetEnabledI( UBOOL _bEnabled )
{
	guard(URealComponent::SetEnabledI);
	NOTE(debugf(TEXT("(%s)URealComponent::SetEnabledI"), *GetFullName(), _bEnabled ? TEXT("True") : TEXT("False") ));

	bEnabled = _bEnabled;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL URealComponent::IsEnabledI()
{
	guard(URealComponent::IsEnabledI);
	NOTE(debugf(TEXT("(%s)URealComponent::IsEnabledI"), *GetFullName() ));

	return bEnabled;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::ValidateI()
{
	guard(URealComponent::ValidateI);
	NOTE(debugf(TEXT("(%s)URealComponent::ValidateI"), *GetFullName() ));

	bValid = true;	// Fix ARL: Do more stuf

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void URealComponent::InvalidateI()
{
	guard(URealComponent::InvalidateI);
	NOTE(debugf(TEXT("(%s)URealComponent::InvalidateI"), *GetFullName() ));

	bValid = false;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL URealComponent::IsValidI()
{
	guard(URealComponent::IsValidI);
	NOTE(debugf(TEXT("(%s)URealComponent::IsValidI"), *GetFullName() ));

	return bValid;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UScaleContainer
*/

///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UScaleContainer::Update(UPDATE_DEF)
{
	guard(UScaleContainer::Update);
	NOTE0(debugf(TEXT("(%s)UScaleContainer::Update"), *GetFullName() ));

	FDimension Scale(	Frame->Viewport->SizeX/RelativeSize.Width,
						Frame->Viewport->SizeY/RelativeSize.Height	);

	NOTE(debugf(TEXT("(%s)UScaleContainer::Update: Scale=%s RelativeSize=%s"),*GetFullName(),*Scale.String(),*RelativeSize.String()));

	Ren->PushScale(Scale);

	Super::Update(UPDATE_PARMS);

	Ren->PopScale();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UScriptComponent
*/

///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UScriptComponent::SendScriptEvent(UCanvas* C)
{
	guard(UScriptComponent::SendScriptEvent);
	NOTE(debugf(TEXT("(%s)UScriptComponent::SendScriptEvent"), *GetFullName() ));

	SIMPLE_ACCESSORX(ScriptComponentTarget,&C)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UScrollBar
*/


// !! NOTE: This class went through some evolutionary development.
// As such, many variables and function names don't make too much sense.
// I'm too lazy to go back and clean it all up now.  Feel free to do so yourself.
// !! YOU HAVE BEEN WARNED !!


///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
FLOAT UScrollBar::GetValue()
{
	guard(UScrollBar::GetValue);
	NOTE(debugf(TEXT("(%s)UScrollBar::GetValue"), *GetFullName() ));

	FLOAT Min, Max;
	GetRange(Min,Max);

	IMPLEMENT_ACCESSOR_BEGIN(ScrollBarGetValue,FLOAT)
		Value = appAblend(Min,Max,Clamp(Result,Min,Max));
	IMPLEMENT_ACCESSOR_END(ScrollBarGetValue)

	return appBlend(Min,Max,Value);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::GetRange( FLOAT &Min, FLOAT &Max )
{
	guard(UScrollBar::GetRange);
	NOTE(debugf(TEXT("(%s)UScrollBar::GetRange"), *GetFullName() ));

	SIMPLE_ACCESSOR_BEGIN(ScrollBarGetRange)
		struct TParms{ FLOAT Min, Max; } Parms;
		Accessor->OnEvent(&Parms);
		_Min=Parms.Min; _Max=Parms.Max;
	SIMPLE_ACCESSOR_END

	Min = _Min;
	Max = _Max;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UScrollBar::GetVisibleRange()
{
	guard(UScrollBar::GetVisibleRange);
	NOTE(debugf(TEXT("(%s)UScrollBar::GetVisibleRange"), *GetFullName() ));

	SIMPLE_ACCESSOR_BEGIN(ScrollBarGetVisibleRange)
		struct TParms{ FLOAT Range; } Parms;
		Accessor->OnEvent(&Parms);
		FLOAT Min, Max; GetRange(Min,Max);
		_Range = Clamp( Parms.Range, Min, Max );
	SIMPLE_ACCESSOR_END

	return _Range;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::SetValue( FLOAT NewValue )
{
	guard(UScrollBar::SetValue);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetValue"), *GetFullName() ));

	FLOAT Min, Max;
	GetRange(Min,Max);

	NewValue = Clamp( NewValue, Min, Max - GetVisibleRange() );
	Value = (NewValue - Min) / (Max - Min);

	SIMPLE_ACCESSORX(ScrollBarSetValue,&NewValue)

	// Update slider position.
	if( Slider )
		SetPos( Slider, GetDim(UpButton) + appBlend( 0.0f, GetDim(Background), Value ) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::SetRange( FLOAT NewMin, FLOAT NewMax )
{
	guard(UScrollBar::SetRange);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetRange"), *GetFullName() ));

	_Min = NewMin;
	_Max = NewMax;

	SIMPLE_ACCESSOR_BEGIN(ScrollBarSetRange)
		struct TParms{ FLOAT Min, Max; } Parms;
		Parms.Min=_Min; Parms.Max=_Max;
		Accessor->OnEvent(&Parms);
	SIMPLE_ACCESSOR_END

	// Update slider height.
	if( Slider )
		SetDim( Slider, ::Max<FLOAT>( GetDim(Background) * GetVisibleRange() / (_Max-_Min), UCONST_MIN_SLIDER_SIZE ) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::SetVisibleRange( FLOAT NewRange )
{
	guard(UScrollBar::SetVisibleRange);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetVisibleRange"), *GetFullName() ));

	FLOAT Min, Max;
	GetRange(Min,Max);

	_Range = Clamp( NewRange, Min, Max );

	SIMPLE_ACCESSORX(ScrollBarSetVisibleRange,&_Range)

	// Update slider height.
	if( Slider )
		SetDim( Slider, ::Max<FLOAT>( GetDim(Background) * _Range / (Max-Min), UCONST_MIN_SLIDER_SIZE ) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::ScrollUp()
{
	guard(UScrollBar::ScrollUp);
	NOTE(debugf(TEXT("(%s)UScrollBar::ScrollUp"), *GetFullName() ));

	SetValue( GetValue() - ScrollAmt );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::ScrollDown()
{
	guard(UScrollBar::ScrollDown);
	NOTE(debugf(TEXT("(%s)UScrollBar::ScrollDown"), *GetFullName() ));

	SetValue( GetValue() + ScrollAmt );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::PageUp()
{
	guard(UScrollBar::PageUp);
	NOTE(debugf(TEXT("(%s)UScrollBar::PageUp"), *GetFullName() ));

	SetValue( GetValue() - PageAmt );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::PageDown()
{
	guard(UScrollBar::PageDown);
	NOTE(debugf(TEXT("(%s)UScrollBar::PageDown"), *GetFullName() ));

	SetValue( GetValue() + PageAmt );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::Init()
{
	guard(UScrollBar::Init);
	NOTE(debugf(TEXT("(%s)UScrollBar::Init"), *GetFullName() ));

#if 1 //HACK
	UpButtonUp		= GetFImageMap().FindRef(TEXT("SB_UpButtonUp"));
	UpButtonDown	= GetFImageMap().FindRef(TEXT("SB_UpButtonDown"));
	DownButtonUp	= GetFImageMap().FindRef(TEXT("SB_DownButtonUp"));
	DownButtonDown	= GetFImageMap().FindRef(TEXT("SB_DownButtonDown"));
	BackgroundImage	= GetFImageMap().FindRef(TEXT("SB_Background"));
	SliderImage		= GetFImageMap().FindRef(TEXT("SB_Slider"));
#endif

	UpButton = NEW_SUBCOMPONENT(UHoldButton);
	AddComponent(UpButton);
	UpButton->SetImages( UpButtonUp, UpButtonDown );
	UpButton->Init();
	UpButton->AddAccessor( UI_NATIVE_CALLBACK(ButtonAction,this,UScrollBar,OnUpClick) );
	UpButton->SetAlignment( bVertical ? FAlignment(ALMT_Center,ALMT_Top) : FAlignment(ALMT_Left,ALMT_Center) );
	UpButton->RepeatDelay = RepeatDelay;
	UpButton->RepeatRate = RepeatRate;

	DownButton = NEW_SUBCOMPONENT(UHoldButton);
	AddComponent(DownButton);
	DownButton->SetImages( DownButtonUp, DownButtonDown );
	DownButton->Init();
	DownButton->AddAccessor( UI_NATIVE_CALLBACK(ButtonAction,this,UScrollBar,OnDownClick) );
	DownButton->SetAlignment( bVertical ? FAlignment(ALMT_Center,ALMT_Bottom) : FAlignment(ALMT_Right,ALMT_Center) );
	DownButton->RepeatDelay = RepeatDelay;
	DownButton->RepeatRate = RepeatRate;
	
	Background = NEW_SUBCOMPONENT(UImageComponent);
	AddComponent(Background);
	Background->Image = BackgroundImage;
	Background->Init();
	Background->SetAlignment( FAlignment(ALMT_Center,ALMT_Center) );
	
	Slider = NEW_SUBCOMPONENT(UImageComponent);
	AddComponent(Slider);
	Slider->Image = SliderImage;
	Slider->Init();
	Slider->SetAlignment( bVertical ? FAlignment(ALMT_Center,ALMT_None) : FAlignment(ALMT_None,ALMT_Center) );

	SetSize( GetSize() );
	SetValue( GetValue() );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::SetMouse( UMouseCursor* M )
{
	guard(UScrollBar::SetMouse);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetMouse"), *GetFullName() ));

	SetMousePressEvent(M, UI_NATIVE_CALLBACK(MousePress,this,UScrollBar,MousePress));
	SetMouseReleaseEvent(M, UI_NATIVE_CALLBACK(MouseRelease,this,UScrollBar,MouseRelease));

	UpButton->SetMouse(M);
	DownButton->SetMouse(M);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

// Capture the mouse for updating the slider's location.
// See Update() for actual update code.

//------------------------------------------------------------------------------
void UScrollBar::MousePress( void* Parms )
{
	guard(UScrollBar::MousePress);
	NOTE(debugf(TEXT("(%s)UScrollBar::MousePress"), *GetFullName() ));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;
	UMouseCursor* M = P->Mouse;

	FPoint Pt=M->GetScreenCoords();
	FLOAT MouseY = bVertical ? Pt.Y : Pt.X;

	if( Slider->ContainsAbs(Pt) )
	{
		SliderMouse = M;
		LastMouseY = MouseY;
		P->bHandled=true;
	}
	else if( Background->ContainsAbs(Pt) )
	{
		FLOAT SliderY = bVertical ? Slider->GetScreenCoords().Y : Slider->GetScreenCoords().X;

		if( MouseY < SliderY )
		{
			PageUpMouse = M;
			PageUp();
		}
		else
		{
			PageDownMouse = M;
			PageDown();
		}

		if( RepeatDelay>0 )
			NextRepeatTime = GetTimeSeconds() + RepeatDelay;

		P->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::MouseRelease( void* Parms )
{
	guard(UScrollBar::MouseRelease);
	NOTE(debugf(TEXT("(%s)UScrollBar::MouseRelease"), *GetFullName() ));

	if( SliderMouse || PageUpMouse || PageDownMouse )
	{
		SliderMouse = NULL;
		PageUpMouse = NULL;
		PageDownMouse = NULL;

		NextRepeatTime = 0;

		((MouseNotifyParms*)Parms)->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UScrollBar::SetSizeI( const FDimension& D )
{
	guard(UScrollBar::SetSizeI);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetSizeI( %s )"), *GetFullName(), D.String() ));

	// Just propogate to the super if we've not yet been initialized.
	if(!UpButton){ Super::SetSizeI(D); return; }

	FLOAT TotalButtonHeight = GetDim(UpButton) + GetDim(DownButton);

	// Clamp height.
	FLOAT Height = ::Max<FLOAT>( bVertical ? D.Height : D.Width, TotalButtonHeight + UCONST_MIN_SLIDER_SIZE );

	// Update background height.
	SetDim( Background, Height - TotalButtonHeight );

	// Update slider size and position.
	FLOAT Min, Max; GetRange(Min,Max); GetValue();
	FLOAT BackgroundHeight = GetDim(Background);
	FLOAT UpButtonHeight = GetDim(UpButton);
	FLOAT SliderHeight = ::Max<FLOAT>( BackgroundHeight * GetVisibleRange() / (Max-Min), UCONST_MIN_SLIDER_SIZE );
	FLOAT SliderY = UpButtonHeight + appBlend( 0.0f, BackgroundHeight, Value );

	SetDim( Slider, SliderHeight );

	// Adjust slider in case it extends past its limits.
	FLOAT Overflow = (SliderY + SliderHeight) - (BackgroundHeight + UpButtonHeight);
	if( Overflow > 0.0f )
	{
		Value = appAblend( 0.0f, BackgroundHeight, SliderY - Overflow );
		SetValue( Min + Value * (Max - Min) );	// enforce value rules.
	}
	else
	{
		SetPos( Slider, SliderY );
	}

	// Propogate width to subcomponents.
	for( INT i=0; i<Components.Num(); i++ )
		SetOtherDim( Components(i), bVertical ? D.Width : D.Height );

	Super::SetSizeI( bVertical ? FDimension(D.Width,Height) : FDimension(Height,D.Height) );
	
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::Update(UPDATE_DEF)
{
	guard(UScrollBar::Update);
	NOTE0(debugf(TEXT("(%s)UScrollBar::Update"), *GetFullName() ));

	// Update Slider position.
	if( SliderMouse )
	{
		FLOAT Y = bVertical ? SliderMouse->GetScreenCoords().Y : SliderMouse->GetScreenCoords().X;
		FLOAT NewY = GetPos(Slider) + (Y - LastMouseY);
		LastMouseY = Y;
		
		Value = appAblend( 0.0f, GetDim(Background), NewY - GetDim(UpButton) );

		FLOAT Min, Max; GetRange(Min,Max);
		SetValue( Min + Value * (Max - Min) );	// enforce value rules.
	}

	// Do repeats if necessary.
	if( NextRepeatTime>0 )
	{
		if( PageUpMouse && GetTimeSeconds()>=NextRepeatTime )
		{
			PageUp();
			NextRepeatTime = GetTimeSeconds() + RepeatRate;
		}
		else if( PageDownMouse && GetTimeSeconds()>=NextRepeatTime )
		{
			PageDown();
			NextRepeatTime = GetTimeSeconds() + RepeatRate;
		}
	}

	Super::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


//////////////////////////////////////////
// Horizontal/Vertical switch functions //
//////////////////////////////////////////

// Fix ARL: Rather than doing it this way it might make more sense to use a single function that does something like this:
//     FPoint FixP( FPoint P ) return FPoint(P.Y,P.X);

//------------------------------------------------------------------------------
void UScrollBar::SetPos( UComponent* C, FLOAT Pos )
{
	guard(UScrollBar::SetPos);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetPos"), *GetFullName() ));

	if(bVertical)	C->SetY(Pos);
	else			C->SetX(Pos);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UScrollBar::GetPos( UComponent* C )
{
	guard(UScrollBar::GetPos);
	NOTE(debugf(TEXT("(%s)UScrollBar::GetPos"), *GetFullName() ));

	if(bVertical)	return C->GetY();
	else			return C->GetX();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::SetDim( UComponent* C, FLOAT Dim )
{
	guard(UScrollBar::SetDim);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetDim"), *GetFullName() ));

	if(bVertical)	C->SetHeight(Dim);
	else			C->SetWidth(Dim);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UScrollBar::GetDim( UComponent* C )
{
	guard(UScrollBar::GetDim);
	NOTE(debugf(TEXT("(%s)UScrollBar::GetDim"), *GetFullName() ));

	if(bVertical)	return C->GetHeight();
	else			return C->GetWidth();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollBar::SetOtherDim( UComponent* C, FLOAT Dim )
{
	guard(UScrollBar::SetOtherDim);
	NOTE(debugf(TEXT("(%s)UScrollBar::SetOtherDim"), *GetFullName() ));

	if(bVertical)	C->SetWidth(Dim);
	else			C->SetHeight(Dim);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UScrollFrame
*/


//////////////////////
// Frame redirects //
//////////////////////

//------------------------------------------------------------------------------
void UScrollFrame::Tween( class UMultiStateComponent* Source, UComponent* Start, UComponent* End, FLOAT Pct )
	{ Super::Tween( Source, Start, End, Pct ); Frame->Tween( Source, Start, End, Pct ); }

//------------------------------------------------------------------------------
FPoint		UScrollFrame::GetFrameLocation()	{ return Frame->GetFrameLocation();	}
FDimension	UScrollFrame::GetFrameSize()		{ return Frame->GetFrameSize();		}

//------------------------------------------------------------------------------
void UScrollFrame::SetFrameLocation( const FPoint& P )
{
	guard(UScrollFrame::SetFrameLocation);
	NOTE(debugf(TEXT("(%s)UScrollFrame::SetFrameLocation"), *GetFullName() ));

	Frame->SetFrameLocation(P);

	FPoint A=Frame->GetFrameLocation();
	FRectangle B=Frame->GetBounds();

	if( SBVertical )
		SBVertical->SetValue( A.Y-B.Y );

	if( SBHorizontal )
		SBHorizontal->SetValue( A.X-B.X );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::SetFrameSize( const FDimension& D )
{
	guard(UScrollFrame::SetFrameSize);
	NOTE(debugf(TEXT("(%s)UScrollFrame::SetFrameSize"), *GetFullName() ));

	Frame->SetFrameSize(D);

	FDimension A=Frame->GetFrameSize();

	if( SBVertical )
		SBVertical->SetVisibleRange( A.Height );

	if( SBHorizontal )
		SBHorizontal->SetVisibleRange( A.Width );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::AddItem( UComponent* C )
{
	guard(UScrollFrame::AddItem);
	NOTE(debugf(TEXT("(%s)UScrollFrame::AddItem"), *GetFullName() ));

	Frame->AddComponent(C);

	FPoint A=Frame->GetLocation();
	FDimension B=Frame->GetSize();

	if( SBVertical )
		SBVertical->SetRange( A.Y, A.Y+B.Height );

	if( SBHorizontal )
		SBHorizontal->SetRange( A.X, A.X+B.Width );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UScrollFrame::Init()
{
	guard(UScrollFrame::Init);
	NOTE(debugf(TEXT("(%s)UScrollFrame::Init"), *GetFullName() ));

	//
	Frame = NEW_SUBCOMPONENT(UFrame);
	AddComponent(Frame);

	//
	SBVertical = NEW_SUBCOMPONENT(UScrollBar);
	Super::AddComponent(SBVertical);

	SBVertical->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetValue			,this,UScrollFrame,OnGetValueV));
	SBVertical->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetRange			,this,UScrollFrame,OnGetRangeV));
	SBVertical->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetVisibleRange	,this,UScrollFrame,OnGetVisibleRangeV));
	SBVertical->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarSetValue			,this,UScrollFrame,OnSetValueV));

	SBVertical->Init();
	SBVertical->SetAlignmentC( ALMT_Right, ALMT_None );

	//
	SBHorizontal = NEW_SUBCOMPONENT(UScrollBar);
	SBHorizontal->bVertical = false;
	Super::AddComponent(SBHorizontal);

	SBHorizontal->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetValue			,this,UScrollFrame,OnGetValueH));
	SBHorizontal->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetRange			,this,UScrollFrame,OnGetRangeH));
	SBHorizontal->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetVisibleRange	,this,UScrollFrame,OnGetVisibleRangeH));
	SBHorizontal->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarSetValue			,this,UScrollFrame,OnSetValueH));

	SBHorizontal->Init();
	SBHorizontal->SetAlignmentC( ALMT_None, ALMT_Bottom );

	//
	SizeHandle = NEW_SUBCOMPONENT(UResizeHandleSE);
	Super::AddComponent(SizeHandle);
	SizeHandle->Init();
	SizeHandle->Target = this;
	SizeHandle->SetAlignmentC( ALMT_Right, ALMT_Bottom );
	SizeHandle->SetLocationC(0,0);

	SetSize( GetSize() );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::SetMouse( UMouseCursor* M )
{
	guard(UScrollFrame::SetMouse);
	NOTE(debugf(TEXT("(%s)UScrollFrame::SetMouse"), *GetFullName() ));

	SBVertical->SetMouse(M);
	SBHorizontal->SetMouse(M);
	SizeHandle->SetMouse(M);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////////////
// ScrollBar hooks //
/////////////////////

//------------------------------------------------------------------------------
void UScrollFrame::OnGetValueV(void* Parms)
{
	guard(UScrollFrame::OnGetValueV);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnGetValueV"), *GetFullName() ));

	struct TParms{ FLOAT Value; };
	TParms* P=(TParms*)Parms;

	P->Value = Frame->GetFrameLocation().Y;

	NOTE(debugf(TEXT("OnGetValueV: %f"),P->Value));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::OnGetRangeV(void* Parms)
{
	guard(UScrollFrame::OnGetRangeV);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnGetRangeV"), *GetFullName() ));

	struct TParms{ FLOAT Min, Max; };
	TParms* P=(TParms*)Parms;

	P->Min = 0;
	P->Max = Frame->GetHeight();

	NOTE(debugf(TEXT("OnGetRangeV: %f %f"),P->Min,P->Max));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::OnGetVisibleRangeV(void* Parms)
{
	guard(UScrollFrame::OnGetVisibleRangeV);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnGetVisibleRangeV"), *GetFullName() ));

	struct TParms{ FLOAT Range; };
	TParms* P=(TParms*)Parms;

	P->Range = Frame->GetFrameSize().Height;

	NOTE(debugf(TEXT("OnGetVisibleRangeV: %f"),P->Range));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::OnSetValueV(void* Parms)
{
	guard(UScrollFrame::OnSetValueV);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnSetValueV"), *GetFullName() ));

	struct TParms{ FLOAT Value; };
	TParms* P=(TParms*)Parms;

	FPoint A=Frame->GetFrameLocation();
	A.Y=P->Value;

	Frame->SetY( -P->Value );
	Frame->SetFrameLocation(A);

	NOTE(debugf(TEXT("OnSetValueV: %f"),P->Value));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::OnGetValueH(void* Parms)
{
	guard(UScrollFrame::OnGetValueH);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnGetValueH"), *GetFullName() ));

	struct TParms{ FLOAT Value; };
	TParms* P=(TParms*)Parms;

	P->Value = Frame->GetFrameLocation().X;

	NOTE(debugf(TEXT("OnGetValueH: %f"),P->Value));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::OnGetRangeH(void* Parms)
{
	guard(UScrollFrame::OnGetRangeH);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnGetRangeH"), *GetFullName() ));

	struct TParms{ FLOAT Min, Max; };
	TParms* P=(TParms*)Parms;

	P->Min = 0;
	P->Max = Frame->GetWidth();

	NOTE(debugf(TEXT("OnGetRangeV: %f %f"),P->Min,P->Max));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::OnGetVisibleRangeH(void* Parms)
{
	guard(UScrollFrame::OnGetVisibleRangeH);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnGetVisibleRangeH"), *GetFullName() ));

	struct TParms{ FLOAT Range; };
	TParms* P=(TParms*)Parms;

	P->Range = Frame->GetFrameSize().Width;

	NOTE(debugf(TEXT("OnGetVisibleRangeH: %f"),P->Range));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::OnSetValueH(void* Parms)
{
	guard(UScrollFrame::OnSetValueH);
	NOTE(debugf(TEXT("(%s)UScrollFrame::OnSetValueH"), *GetFullName() ));

	struct TParms{ FLOAT Value; };
	TParms* P=(TParms*)Parms;

	FPoint A=Frame->GetFrameLocation();
	A.X=P->Value;

	Frame->SetX( -P->Value );
	Frame->SetFrameLocation(A);

	NOTE(debugf(TEXT("OnSetValueH: %f"),P->Value));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UScrollFrame::Serialize( FArchive &Ar )
{
	guard(UScrollFrame::Serialize);
	NOTE(debugf(TEXT("(%s)UScrollFrame::Serialize"), *GetFullName()));

	Super::Serialize(Ar);

	SERIALIZE_INDEXED_COMPONENT(UFrame,Frame);
	SERIALIZE_INDEXED_COMPONENT(UScrollBar,SBVertical);
	SERIALIZE_INDEXED_COMPONENT(UScrollBar,SBHorizontal);
	SERIALIZE_INDEXED_COMPONENT(UResizeHandleSE,SizeHandle);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollFrame::SetSizeI( const FDimension& Dim )
{
	guard(UScrollFrame::SetSizeI);
	NOTE(debugf(TEXT("(%s)UScrollFrame::SetSizeI"), *GetFullName() ));

	// Clamp size to size of Frame contents.
	FDimension D;
	FDimension PSize=Frame->GetSize();
	D.Width = Min( Dim.Width, PSize.Width );
	D.Height = Min( Dim.Height, PSize.Height );

	Super::SetSizeI(D);

	FDimension HandleSize = SizeHandle->GetSize();

	SBVertical->SetHeight( D.Height - HandleSize.Height );
	SBHorizontal->SetWidth( D.Width - HandleSize.Width );
	SetFrameSize(D);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}





/*
	UScrollTextArea
*/


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UScrollTextArea::Init()
{
	guard(UScrollTextArea::Init);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::Init"), *GetFullName() ));

	ScrollBar = NEW_SUBCOMPONENT(UScrollBar);
	ScrollBar->DrawOrder = -9998.0f;	// make sure it stays in slot 1.
	Super::AddComponent(ScrollBar);

	ScrollBar->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetValue		,this,UScrollTextArea,OnGetValue));
	ScrollBar->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetRange		,this,UScrollTextArea,OnGetRange));
	ScrollBar->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarGetVisibleRange	,this,UScrollTextArea,OnGetVisibleRange));
	ScrollBar->AddAccessor( UI_NATIVE_CALLBACK(ScrollBarSetValue		,this,UScrollTextArea,OnSetValue));

	ScrollBar->Init();
	ScrollBar->SetAlignmentC( ALMT_Right, ALMT_None );

	SetSize( GetSize() );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollTextArea::SetMouse( UMouseCursor* M )
{
	guard(UScrollTextArea::SetMouse);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::SetMouse"), *GetFullName() ));

	ScrollBar->SetMouse(M);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////////////
// ScrollBar hooks //
/////////////////////

//------------------------------------------------------------------------------
void UScrollTextArea::OnGetValue(void* Parms)
{
	guard(UScrollTextArea::OnGetValue);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::OnGetValue"), *GetFullName() ));

	struct TParms{ FLOAT Value; };
	TParms* P=(TParms*)Parms;

	P->Value = StartLineIndex;

	NOTE(debugf(TEXT("OnGetValue: %f"),P->Value));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollTextArea::OnGetRange(void* Parms)
{
	guard(UScrollTextArea::OnGetRange);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::OnGetRange"), *GetFullName() ));

	struct TParms{ FLOAT Min, Max; };
	TParms* P=(TParms*)Parms;

	P->Min = 0;
	P->Max = Components.Num()-1 - SN;

	NOTE(debugf(TEXT("OnGetRange: %f %f"),P->Min,P->Max));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollTextArea::OnGetVisibleRange(void* Parms)
{
	guard(UScrollTextArea::OnGetVisibleRange);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::OnGetVisibleRange"), *GetFullName() ));

	struct TParms{ FLOAT Range; };
	TParms* P=(TParms*)Parms;

	FLOAT W,H;
	GetConsole()->TextSize( GetFont(NULL), TEXT("Test"), W, H );

	P->Range = GetHeight() / H;

	NOTE(debugf(TEXT("OnGetVisibleRange: %f"),P->Range));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollTextArea::OnSetValue(void* Parms)
{
	guard(UScrollTextArea::OnSetValue);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::OnSetValue"), *GetFullName() ));

	struct TParms{ FLOAT Value; };
	TParms* P=(TParms*)Parms;

	StartLineIndex = P->Value;

	NOTE(debugf(TEXT("OnSetValue: %f"),P->Value));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UScrollTextArea::Serialize( FArchive &Ar )
{
	guard(UScrollTextArea::Serialize);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::Serialize"), *GetFullName()));

	Super::Serialize(Ar);

	SERIALIZE_INDEXED_COMPONENT(UScrollBar,ScrollBar);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollTextArea::AddText( FString NewText, UBOOL bWrap )
{
	guard(UScrollTextArea::SetSize);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::SetSize"), *GetFullName() ));

	Super::AddText(NewText,bWrap);

	UScrollBar* ScrollBar=DynamicCastChecked<UScrollBar>(Components(1));

	{
		struct TParms{ FLOAT Min, Max; } Parms;
		OnGetRange(&Parms);
		ScrollBar->SetRange(Parms.Min,Parms.Max);
	}

	{
		struct TParms{ FLOAT Value; } Parms;
		OnGetValue(&Parms);
		ScrollBar->SetValue(Parms.Value);
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollTextArea::SetSizeI( const FDimension& D )
{
	guard(UScrollTextArea::SetSizeI);
	NOTE(debugf(TEXT("(%s)UScrollTextArea::SetSizeI"), *GetFullName() ));

	Super::SetSizeI(D);
	ScrollBar->SetHeight( D.Height );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UScrollTextArea::Update(UPDATE_DEF)
{
	guard(UScrollTextArea::Update);
	NOTE0(debugf(TEXT("(%s)UScrollTextArea::Update"), *GetFullName() ));

	Super::Update(UPDATE_PARMS);
	Components(1)->Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}



/*
	USelector
*/



////////////////////
// Initialization //
////////////////////

//------------------------------------------------------------------------------
void USelector::Constructed()
{
	guard(USelector::Constructed);
	NOTE(debugf(TEXT("(%s)USelector::Constructed"), *GetFullName() ));

	Super::Constructed();

	List				= NEW_SUBCOMPONENT(USelectorList);		List            ->SetAlignment( FAlignment(ALMT_Center,ALMT_None  ) );	AddComponent( List             );
	TopFrameImage		= NEW_SUBCOMPONENT(UImageComponent);	TopFrameImage   ->SetAlignment( FAlignment(ALMT_Center,ALMT_Top   ) );	AddComponent( TopFrameImage    );
	BottomFrameImage	= NEW_SUBCOMPONENT(UImageComponent);	BottomFrameImage->SetAlignment( FAlignment(ALMT_Center,ALMT_Bottom) );	AddComponent( BottomFrameImage );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::Serialize( FArchive &Ar )
{
	guard(USelector::Serialize);
	NOTE(debugf(TEXT("(%s)USelector::Serialize"), *GetFullName()));

	Super::Serialize(Ar);

//!!ARL	Ar << TopFrame << BottomFrame;

	SERIALIZE_INDEXED_COMPONENT(USelectorList,List);
	SERIALIZE_INDEXED_COMPONENT(UImageComponent,TopFrameImage);
	SERIALIZE_INDEXED_COMPONENT(UImageComponent,BottomFrameImage);

	// Fix ARL: We can assume that this component - Group - has already been serialized (search up instead of down from the root to be a lot faster).
	// Remember though that our uplink connections haven't been fixed up yet (iow: parent hasn't been set yet).
	Ar << *(DWORD*)&Group;
	if( Group )
	{
		SERIALIZE_PATHED_COMPONENT(USelector,Group);
	}

	SERIALIZE_BOOL(bHideBorders);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
// SelectorList
void	USelector::AddItem( FString Item, ActionEvent* Action ){ List->AddItem( Item, Action ? Action : GetAccessorSafe(ACCESSOR_SelectorModify), GetAccessorSafe(ACCESSOR_SelectorModifyIndex) ); }
void	USelector::RemoveItem( FString Item )		{ List->RemoveItem( Item );			}
FString	USelector::GetItem( INT i )					{ return List->GetItem(i);			}
INT		USelector::NumItems()						{ return List->NumItems();			}
void	USelector::Empty()							{ List->Empty();					}
void	USelector::Select( INT i )					{ List->Select(i);					}
void	USelector::SelectItem( FString Item )		{ List->SelectItem( Item );			}
UBOOL	USelector::HasItem( FString Item )			{ return List->HasItem( Item );		}
FString	USelector::GetCurrentItem()					{ return List->GetCurrentItem();	}
INT		USelector::GetCurrentItemIndex()			{ return List->GetCurrentItemIndex();}
void	USelector::SetHighlight( const FColor& C )	{ List->SetHighlight(C);			}
void	USelector::SetUnHighlight( const FColor& C ){ List->SetUnHighlight(C);			}

//------------------------------------------------------------------------------
// ImageComponent
void USelector::SetBackground( FImage* _Image )		{ List->Image = _Image;				}
void USelector::Init()
{
	guard(USelector::Init);
	NOTE(debugf(TEXT("(%s)USelector::Init"), *GetFullName() ));

	FLOAT W,H;
	GetConsole()->TextSize( GetFont(), TEXT("Test"), W, H );
	FLOAT HalfHeight = H/2.0f;

	TopFrameImage->Image=TopFrame;
	TopFrameImage->Init();
	TopFrameImage->SetY(-(TopFrameImage->GetHeight()-HalfHeight));
//	TopFrameImage->Lock(NAME_Height);	// Fix ARL
	
	BottomFrameImage->Image=BottomFrame;
	BottomFrameImage->Init();
	BottomFrameImage->SetY(BottomFrameImage->GetHeight()-HalfHeight);
//	BottomFrameImage->Lock(NAME_Height);	// Fix ARL

	RegisterEvent( UI_EVENT(Refresh), this );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
// TextComponentBase
void	USelector::SetFontSize( EFontSize NewSize )	{ List->SetFontSize( NewSize );	}
void	USelector::SetFont( UFont* NewFont )		{ List->SetFont( NewFont );		}
UFont*	USelector::GetFont( UCanvas* C )			{ return List->GetFont(C);		}

//------------------------------------------------------------------------------
void USelector::Update(UPDATE_DEF)
{
	guard(USelector::Update);
	NOTE0(debugf(TEXT("(%s)USelector::Update"), *GetFullName() ));

	if( NumItems() == 0 )
		Refresh();

	IMPLEMENT_ACCESSOR_BEGIN(SelectorText,FString)
		SelectItem(Result);
	IMPLEMENT_ACCESSOR_END(SelectorText)

	IMPLEMENT_ACCESSOR_BEGIN(SelectorCurrent,INT)
		Select(Result);
	IMPLEMENT_ACCESSOR_END(SelectorCurrent)

	Super::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}
	
//------------------------------------------------------------------------------
void USelector::Refresh()
{
	guard(USelector::Refresh);
	NOTE(debugf(TEXT("(%s)USelector::Refresh()"), *GetFullName() ));

	SIMPLE_ACCESSOR_BEGIN(SelectorAccess)

		//FString Current = GetCurrentItem();
		INT iCurrent = GetCurrentItemIndex();

		Empty();

		TArray<FString> List;
		Accessor->OnEvent(&List);
		for( INT i=0; i<List.Num(); i++ )
			{ AddItem( List(i) ); NOTE(debugf(TEXT("SelectorAccess: %s"),*List(i))); }

		//SelectItem( Current );
		Select(iCurrent);

	SIMPLE_ACCESSOR_END

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::SetGroup( UContainer* C )
{
	guard(USelector::SetGroup);
	NOTE(debugf(TEXT("(%s)USelector::SetGroup( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	Group = C;

	if( Group )
	{
		Group->RegisterEvent( UI_EVENT(HideBorder), this );
		Group->SendEvent(UI_EVENT(HideBorder));	// start out all hidden (mouse events will take care of rest).
		RegisterEvent( UI_EVENT(ShowBorder), this );
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
void USelector::SetMouse( UMouseCursor* M )
{
	guard(USelector::SetMouse);
	NOTE(debugf(TEXT("(%s)USelector::SetMouse( %s )"), *GetFullName(), M ? *M->GetFullName() : TEXT("None") ));

	List->SetMouse(M);
	SetMousePressEvent  (M, UI_NATIVE_CALLBACK(MousePress  ,this,USelector,MousePress  ));
	SetMouseReleaseEvent(M, UI_NATIVE_CALLBACK(MouseRelease,this,USelector,MouseRelease));
	SetMouseEnterEvent  (M, UI_NATIVE_CALLBACK(MouseEnter  ,this,USelector,MouseEnter  ));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::MousePress( void* Parms )
{
	guard(USelector::MousePress);
	NOTE(debugf(TEXT("(%s)USelector::MousePress"), *GetFullName() ));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;

	if( ContainsAbs( P->Mouse->GetScreenCoords() ) )
	{
		UBOOL bLocked=0;

		IMPLEMENT_ACCESSOR_BEGIN(SelectorLocked,UBOOL)
			bLocked=Result;
			NOTE(debugf(TEXT("(%s)Locked=%d"),*GetFullName(),bLocked));
		IMPLEMENT_ACCESSOR_END(SelectorLocked)

		if( !bLocked )	// Fix ARL: Should add more generalized disabled support to all components -- gray-out, etc.
		{
			Refresh();
			BringToFront();

			List->Expand();		// Fix ARL: Restrict Mouse movement to List's Bounds?

			// Fade other (but not ourself).
			Lock(LOCK_AlphaPct);
			if( Group )
				Group->SetAlphaPct(0.25f);

			// Don't allow borders to change while open.
			if( Group )
				Group->Lock(LOCK_Borders);
		}

		P->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::MouseRelease( void* Parms )
{
	guard(USelector::MouseRelease);
	NOTE(debugf(TEXT("(%s)USelector::MouseRelease"), *GetFullName() ));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;

	if( List->bOpen )
	{ 
		List->Close();

		// Unfade others (but not ourself).
		if( Group )
			Group->SetAlphaPct(4.0f);
		Unlock(LOCK_AlphaPct);

		// Re-enable border changing within the group.
		if( Group )
			Group->Unlock(LOCK_Borders);

		P->bHandled = true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::PreSelect()
{
	guard(USelector::PreSelect);
	NOTE(debugf(TEXT("(%s)USelector::PreSelect"), *GetFullName()));

	SIMPLE_ACCESSOR(SelectorPreSelect)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::PostSelect()
{
	guard(USelector::PostSelect);
	NOTE(debugf(TEXT("(%s)USelector::PostSelect"), *GetFullName()));

	SIMPLE_ACCESSOR(SelectorPostSelect)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::MouseEnter( void* Parms )
{
	guard(USelector::MouseEnter);
	NOTE(debugf(TEXT("(%s)USelector::MouseEnter"), *GetFullName() ));

	if( Group && Group->Locked(LOCK_Borders) )
		return;

	if( Group )
	{
		Group->SendEvent(UI_EVENT(HideBorder));	// Hide others.
		SendEvent(UI_EVENT(ShowBorder));		// Show self.
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
FRectangle USelector::GetBoundsI()
{
	guard(USelector::GetBoundsI);
	NOTE(debugf(TEXT("(%s)USelector::GetBoundsI"), *GetFullName() ));

	FRectangle R = List->GetBounds() + GetLocation();
	R.Width = TopFrameImage->GetWidth();
	return R;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle USelector::GetScreenBoundsI()
{
	guard(USelector::GetScreenBoundsI);
	NOTE(debugf(TEXT("(%s)USelector::GetScreenBoundsI"), *GetFullName() ));

	FRectangle R = List->GetScreenBounds();
	R.Width = TopFrameImage->GetWidth();
	return R;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelector::HandleEvent( uiEvent Event )
{
	guard(USelector::HandleEvent);
	NOTE(debugf(TEXT("(%s)USelector::HandleEvent( %s )"), *GetFullName(), *LookupEventName(Event) ));

	if( Event==UI_EVENT(Refresh) ) Refresh();

	else if( Event==UI_EVENT(HideBorder) && bHideBorders )
	{
		// Fix ARL: Hack to show/hide borders.  Should instead use MultiStateComponents.  May want to reconsider rebuilding Selectors totally in script.
		TopFrameImage->SetAlpha(0);
		BottomFrameImage->SetAlpha(0);
	}
	else if( Event==UI_EVENT(ShowBorder) && bHideBorders )
	{
		TopFrameImage->SetAlpha(255);
		BottomFrameImage->SetAlpha(255);
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	USelectorItem
*/


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
void USelectorItem::SetMouse( UMouseCursor* M )
{
	guard(USelectorItem::SetMouse);
	NOTE(debugf(TEXT("(%s)USelectorItem::SetMouse"), *GetFullName()));

	SetMouseEnterEvent  (M, UI_NATIVE_CALLBACK(MouseEnter  ,this,USelectorItem,MouseEnter  ));
	SetMouseExitEvent   (M, UI_NATIVE_CALLBACK(MouseExit   ,this,USelectorItem,MouseExit   ));
	SetMouseReleaseEvent(M, UI_NATIVE_CALLBACK(MouseRelease,this,USelectorItem,MouseRelease));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorItem::MouseEnter( void* Parms )
{
	guard(USelectorItem::MouseEnter);
	NOTE(debugf(TEXT("(%s)USelectorItem::MouseEnter"), *GetFullName() ));

	SetColor(Highlight);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorItem::MouseExit( void* Parms )
{
	guard(USelectorItem::MouseExit);
	NOTE(debugf(TEXT("(%s)USelectorItem::MouseExit"), *GetFullName() ));

	SetColor(UnHighlight);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorItem::MouseRelease( void* Parms )
{
	guard(USelectorItem::MouseRelease);
	NOTE(debugf(TEXT("(%s)USelectorItem::MouseRelease: IsEnabled=%d"), *GetFullName(), IsEnabled()));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;

	USelectorList* List = DynamicCastChecked<USelectorList>(GetParent(USelectorList::StaticClass()));
	if( !List->bOpen )
		return;

	if( ContainsAbs( P->Mouse->GetScreenCoords() ) )
	{
		NOTE(debugf(TEXT("(%s)USelectorItem::MouseRelease: IsEnabled=%d DrawOrder=%f"), *GetFullName(), IsEnabled(), GetAbsDrawOrder()));

		P->bHandled = true;

		//HACK: DrawOrderAbs doesn't handle embedded components very well -- ideally we'd leave the call unhandled,
		//and then hope that our parent selector was the next highest priority component.  But in practice, all the
		//leaves of the other selectors in our group will have a higher abs draw order, so that kind of screws things
		//up -- since we don't want them to get the notification.  So instead, we'll just notify our selector manually.
		USelector* ParentSelector = DynamicCastChecked<USelector>(GetParent(USelector::StaticClass()));
		ParentSelector->MouseRelease(Parms);

		// Don't allow user to reselect the current item (this way if the user just quickly clicks on selector, nothing happens).
		INT i=List->GetItemIndex(this);
		if( ParentSelector->GetCurrentItemIndex()!=i )	// Fix ARL: Maybe make this optional?
		{
			ParentSelector->PreSelect();

			//List->SelectItem( GetText() );	// Fix ARL: Store index so we can call Select directly instead (otherwise we'll run into problems with duplicate text) -- note: GetParent()->GetComponentIndex doesn't work due to mixin with other components (like Images).
			List->Select(i);

			SIMPLE_ACCESSOR_SET(SelectorModify,FString,List->GetCurrentItem())
			SIMPLE_ACCESSOR_SET(SelectorModifyIndex,INT,i)

			ParentSelector->PostSelect();
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	USelectorList
*/



////////////////////
// Initialization //
////////////////////

//------------------------------------------------------------------------------
void USelectorList::Constructed()
{
	guard(USelectorList::Constructed);
	NOTE(debugf(TEXT("(%s)USelectorList::Constructed"), *GetFullName() ));

	Super::Constructed();

	TextInfo = NEW_SUBCOMPONENT(UTextComponentBase);

	OpenState		= NEW_SUBCOMPONENT(UFrame);
	ClosedState		= NEW_SUBCOMPONENT(UFrame);
	TransitionState	= NEW_SUBCOMPONENT(UFrame);

	AddState( ClosedState );
	AddState( OpenState );

	AddTransition( UI_EVENT(Open), 0, 1, 0.1f, TransitionState );
	AddTransition( UI_EVENT(Close), 1, 0, 0.2f, TransitionState );

	RegisterEvent( UI_EVENT(Open), this );
	RegisterEvent( UI_EVENT(Close), this );

	AddComponent( TextInfo );	// NOTE[aleiby]: Potentially dangerous, but will keep us from having memory leaks for now.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::Serialize( FArchive &Ar )
{
	guard(USelectorList::Serialize);
	NOTE(debugf(TEXT("(%s)USelectorList::Serialize"), *GetFullName()));

	Super::Serialize(Ar);

	Ar << CurrentItem;

	SERIALIZE_INDEXED_COMPONENT(UTextComponentBase,TextInfo);
	SERIALIZE_INDEXED_COMPONENT(UFrame,OpenState);
	SERIALIZE_INDEXED_COMPONENT(UFrame,ClosedState);
	SERIALIZE_INDEXED_COMPONENT(UFrame,TransitionState);

	SERIALIZE_MOUSE(USelectorList);

	Ar << Highlight << UnHighlight;
//!!ARL	Ar << Image;

	SERIALIZE_BOOL(bOpen);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
// TextComponentBase
void	USelectorList::SetFontSize( EFontSize NewSize )		{ TextInfo->SetFontSize( NewSize );	}
void	USelectorList::SetFont( UFont* NewFont )			{ TextInfo->SetFont( NewFont );		}
UFont*	USelectorList::GetFont( UCanvas* C )				{ return TextInfo->GetFont(C);		}

//------------------------------------------------------------------------------
void USelectorList::AddItem( FString Text, ActionEvent* Action, ActionEvent* Action2 )
{
	guard(USelectorList::AddItem);
	NOTE(debugf(TEXT("(%s)USelectorList::AddItem( %s )"), *GetFullName(), *Text ));

	AddSelectorItem( Text, OpenState      , OpenState      ->GetSize().Height, Action, Action2 );
	AddLabel       ( Text, ClosedState    , ClosedState    ->GetSize().Height );
	AddLabel       ( Text, TransitionState, TransitionState->GetSize().Height );

	OpenState->SetFrameSize( OpenState->GetSize() );

	if( NumItems()==1 )
		Select(0);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::RemoveItem( FString Text )
{
	guard(USelectorList::RemoveItem);
	NOTE(debugf(TEXT("(%s)USelectorList::RemoveItem"), *GetFullName() ));

	for( INT i=0; i<NumItems(); i++ )
	{
		if( GetItem(i)==Text )
		{
			OpenState      ->RemoveComponent( OpenState      ->GetComponent(i) );
			ClosedState    ->RemoveComponent( ClosedState    ->GetComponent(i) );
			TransitionState->RemoveComponent( TransitionState->GetComponent(i) );

			OpenState->SetFrameSize( OpenState->GetSize() );
			
			if( CurrentItem==i )
				Select(i);

			break;
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FString USelectorList::GetItem( INT i )
{
	guard(USelectorList::GetItem);
	NOTE(debugf(TEXT("(%s)USelectorList::GetItem"), *GetFullName() ));

	// Fix ARL: Is all this coupling really necessary?
	return NumItems() ? DynamicCastChecked<ULabel>(DynamicCastChecked<UContainer>(OpenState->GetComponent(i))->Components(1))->GetText() : FString(TEXT(""));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT USelectorList::GetItemIndex( USelectorItem* Item )
{
	guard(USelectorList::GetItemIndex);
	NOTE(debugf(TEXT("(%s)USelectorList::GetItemIndex( %s )"), *GetFullName(), Item ? *Item->GetFullName() : TEXT("NA") ));

	for( INT i=0; i<NumItems(); i++ )
		if( DynamicCastChecked<USelectorItem>(DynamicCastChecked<UContainer>(OpenState->GetComponent(i))->Components(1))==Item )
			return i;
	return -1;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT USelectorList::NumItems()
{
	guard(USelectorList::NumItems);
	NOTE(debugf(TEXT("(%s)USelectorList::NumItems"), *GetFullName() ));

	return OpenState->NumComponents();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::Empty()
{
	guard(USelectorList::Empty);
	NOTE(debugf(TEXT("(%s)USelectorList::Empty"), *GetFullName() ));

	while( NumItems() )
	{
		OpenState		->GetComponent(0)->DeleteUObject();
		ClosedState		->GetComponent(0)->DeleteUObject();
		TransitionState	->GetComponent(0)->DeleteUObject();
	}

	OpenState->SetFrameSize( OpenState->GetSize() );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::Select( INT i )
{
	guard(USelectorList::Select);
	NOTE(debugf(TEXT("(%s)USelectorList::Select( %d )  NumItems=%d"), *GetFullName(), i, NumItems() ));

	if( NumItems()<=i || i<0 )
		return;

	CurrentItem = i;
	ClosedState->SetLocation( -ClosedState->GetComponent(i)->GetLocation() );
	ClosedState->SetFrameSize( ClosedState->GetComponent(i)->GetSize() );
	ClosedState->SetFrameLocation( ClosedState->GetComponent(i)->GetAlignedLocation() );

	// Update selection color.
	for( INT j=0; j<OpenState->NumComponents(); j++ )
	{
		UComponent* A=DynamicCastChecked<UContainer>(ClosedState    ->GetComponent(j))->GetComponent(1);
		UComponent* B=DynamicCastChecked<UContainer>(TransitionState->GetComponent(j))->GetComponent(1);

		A->SetColor( j==i ? Highlight : UnHighlight );
		B->SetColor( j==i ? Highlight : UnHighlight );

		// psuedo-hack to maintain alpha pct -- see USelector::MousePress.
		UComponent* Group = DynamicCastChecked<USelector>(Parent)->Group;
		if( Group && Group->Locked(LOCK_Borders) )
			A->SetAlphaPct(0.25f);
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::SelectItem( FString Text )
{
	guard(USelectorList::SelectItem);
	NOTE(debugf(TEXT("(%s)USelectorList::SelectItem"), *GetFullName() ));

	for( INT i=0; i<NumItems(); i++ )
		if( GetItem(i)==Text )
			{ Select(i); break; }

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UBOOL USelectorList::HasItem( FString Item )
{
	guard(USelectorList::HasItem);
	NOTE(debugf(TEXT("(%s)USelectorList::HasItem"), *GetFullName() ));

	for( INT i=0; i<NumItems(); i++ )
		if( GetItem(i)==Item )
			return true;

	return false;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FString USelectorList::GetCurrentItem()
{
	guard(USelectorList::GetCurrentItem);
	NOTE(debugf(TEXT("(%s)USelectorList::GetCurrentItem"), *GetFullName() ));

	return GetItem( CurrentItem );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT USelectorList::GetCurrentItemIndex()
{
	guard(USelectorList::GetCurrentItemIndex);
	NOTE(debugf(TEXT("(%s)USelectorList::GetCurrentItemIndex"), *GetFullName() ));

	return CurrentItem;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::SetMouse( UMouseCursor* M )
{
	guard(USelectorList::SetMouse);
	NOTE(debugf(TEXT("(%s)USelectorList::SetMouse"), *GetFullName() ));

	Mouse = M;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::SetHighlight( const FColor& C )
{
	guard(USelectorList::SetHighlight);
	NOTE(debugf(TEXT("(%s)USelectorList::SetHighlight"), *GetFullName() ));

	Highlight = C;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::SetUnHighlight( const FColor& C )
{
	guard(USelectorList::SetUnHighlight);
	NOTE(debugf(TEXT("(%s)USelectorList::SetUnHighlight"), *GetFullName() ));

	UnHighlight = C;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::Expand()
{
	guard(USelectorList::Expand);
	NOTE(debugf(TEXT("(%s)USelectorList::Expand"), *GetFullName() ));

	if( !bOpen )
	{
		OpenState->SetLocation( ClosedState->GetLocation() );
		SendEvent(UI_EVENT(Open));
		bOpen=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::Close()
{
	guard(USelectorList::Close);
	NOTE(debugf(TEXT("(%s)USelectorList::Close"), *GetFullName() ));

	if( bOpen )
	{
		SendEvent(UI_EVENT(Close));
		bOpen=false;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
void USelectorList::AddSelectorItem( FString Text, UContainer* C, FLOAT Y, ActionEvent* Action, ActionEvent* Action2 )
{
	guard(USelectorList::AddSelectorItem);
	NOTE(debugf(TEXT("(%s)USelectorList::AddSelectorItem"), *GetFullName() ));

	UContainer* Container = NEW_SUBCOMPONENT(UContainer);
	C->AddComponent( Container );
	Container->SetY( Y );

	UImageComponent* ImageComponent = NEW_SUBCOMPONENT(UImageComponent);
	Container->AddComponent( ImageComponent );
	ImageComponent->DrawOrder = 0.0f;
	ImageComponent->Image = Image;
	ImageComponent->Init();
	ImageComponent->SetWidth( Parent->GetWidth() );

	USelectorItem* Item = NEW_SUBCOMPONENT(USelectorItem);
	Container->AddComponent( Item );
	Item->SetMouse( Mouse );
	Item->AddAccessor( Action );	if(Action2) Item->AddAccessor(Action2);
	Item->Highlight = Highlight;
	Item->UnHighlight = UnHighlight;
	Item->SetFont( GetFont() );
	Item->SetText( Text );
	Item->SetColor( UnHighlight );
	Item->SetWidth( Parent->GetWidth() );
	Item->SetAlignmentX( ALMT_Center );

	ImageComponent->SetHeight( Item->GetHeight() );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USelectorList::AddLabel( FString Text, UContainer* C, FLOAT Y )
{
	guard(USelectorList::AddLabel);
	NOTE(debugf(TEXT("(%s)USelectorList::AddLabel"), *GetFullName() ));

	UContainer* Container = NEW_SUBCOMPONENT(UContainer);
	C->AddComponent( Container );
	Container->SetY( Y );

	UImageComponent* ImageComponent = NEW_SUBCOMPONENT(UImageComponent);
	Container->AddComponent( ImageComponent );
	ImageComponent->DrawOrder = 0.0f;
	ImageComponent->Image = Image;
	ImageComponent->Init();
	ImageComponent->SetWidth( Parent->GetWidth() );

	ULabel* Item = NEW_SUBCOMPONENT(ULabel);
	Container->AddComponent( Item );
	Item->SetFont( GetFont() );
	Item->SetText( Text );
	Item->SetColor( UnHighlight );
	Item->SetWidth( Parent->GetWidth() );
	Item->SetAlignmentX( ALMT_Center );

	ImageComponent->SetHeight( Item->GetHeight() );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}



/*
	USlider
*/

///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void USlider::SetValue( FLOAT NewValue )
{
	guard(USlider::SetValue);
	NOTE(debugf(TEXT("(%s)USlider::SetValue(%f)"), *GetFullName(), NewValue ));

	if( Step > 0 )
		NewValue = appRound( ( NewValue - Min ) / Step ) * Step + Min;
	NewValue = Clamp( NewValue, Min, Max );
	Value = appAblend( Min, Max, NewValue );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT USlider::GetValue()
{
	guard(USlider::GetValue);
	NOTE(debugf(TEXT("(%s)USlider::GetValue"), *GetFullName() ));

	return appBlend( Min, Max, Value );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT USlider::GetIntValue()
{
	guard(USlider::GetIntValue);
	NOTE(debugf(TEXT("(%s)USlider::GetIntValue"), *GetFullName() ));

	return GetValue();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FString USlider::GetStringValue()
{
	guard(USlider::GetStringValue);
	NOTE(debugf(TEXT("(%s)USlider::GetStringValue: Val=%f Str=%s"), *GetFullName(), GetValue(), *FString::Printf(*Format,GetValue()) ));

	return FString::Printf(*Format,GetValue());

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USlider::SetRange( FLOAT NewMin, FLOAT NewMax )
{
	guard(USlider::SetRange);
	NOTE(debugf(TEXT("(%s)USlider::SetRange"), *GetFullName() ));

	Min = NewMin;
	Max = NewMax;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USlider::SetMouse( UMouseCursor* M )
{
	guard(USlider::SetMouse);
	NOTE(debugf(TEXT("(%s)USlider::SetMouse"), *GetFullName() ));

	SetMousePressEvent(M, UI_NATIVE_CALLBACK(MousePress,this,USlider,MousePress));
	SetMouseReleaseEvent(M, UI_NATIVE_CALLBACK(MouseRelease,this,USlider,MouseRelease));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

// Capture the mouse for updating the slider's location.
// See Update() for actual update code.

//------------------------------------------------------------------------------
void USlider::MousePress( void* Parms )
{
	guard(USlider::MousePress);
	NOTE(debugf(TEXT("(%s)USlider::MousePress"), *GetFullName() ));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;
	UMouseCursor* M = P->Mouse;

	if( ContainsAbs( M->GetScreenCoords() ) )
	{
		Mouse = M;
		P->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USlider::MouseRelease( void* Parms )
{
	guard(USlider::MouseRelease);
	NOTE(debugf(TEXT("(%s)USlider::MouseRelease"), *GetFullName() ));

	if( Mouse )
	{
		SetTargetValue();
		Mouse = NULL;
		((MouseNotifyParms*)Parms)->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USlider::SetTargetValue()
{
	guard(USlider::SetTargetValue);
	NOTE(debugf(TEXT("(%s)USlider::SetTargetValue"), *GetFullName() ));

	SIMPLE_ACCESSOR_SET(SliderModify,FLOAT,GetValue())

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USlider::GetTargetValue()
{
	guard(USlider::GetTargetValue);
	NOTE(debugf(TEXT("(%s)USlider::GetTargetValue"), *GetFullName() ));

	IMPLEMENT_ACCESSOR_BEGIN(SliderAccess,FLOAT)
		SetValue( Result );
	IMPLEMENT_ACCESSOR_END(SliderAccess)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void USlider::SetImage( FImage* _Image, FImage* _Handle )
{
	guard(USlider::SetImage);
	NOTE(debugf(TEXT("(%s)USlider::SetImage"), *GetFullName() ));

	Super::SetImage( _Image );
	Handle = _Handle;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
// NOTE[aleiby]: Completely overriden to avoid unnecessary extra work (like 
// getting the screen coords, size, pushing the canvas, etc, etc.)  We should
// occasionally check ImageComponentBase's version of the function to make sure
// we have all the latest changes since we're not calling Super.Update here.
//------------------------------------------------------------------------------
void USlider::Update(UPDATE_DEF)
{
	guard(USlider::Update);
	NOTE0(debugf(TEXT("(%s)USlider::Update"), *GetFullName() ));

	FPoint P = GetScreenCoords();	// Fix ARL: Wouldn't it make more sense for Update to pass in the relative screen coords (so we don't have to be constantly calculating them all the way up to the root for every component)?
	FDimension D = GetSize();

	// Update slider position.
	if( Mouse )
	{
		FPoint MPos = Mouse->GetScreenCoords();
		if( MPos.X < P.X + SideBuffer )
			Value = 0.0f;
		else if( MPos.X > P.X + D.Width - SideBuffer )
			Value = 1.0f;
		else
			Value = (MPos.X - (P.X + SideBuffer)) / (D.Width - 2*SideBuffer);
		SetValue( GetValue() );	// enforce value rules (like step, range, etc.)
	}
	else
	{
		GetTargetValue();	// Fix ARL: Potential performance hazzard - there should be no need to call this function every frame.
	}

	FImage* I = GetImage();
	Ren->DrawTile( I, FRectangle(P.X,P.Y,D.Width,D.Height), 0, GetPolyFlags(), GetColor() );
	P = FPoint( P.X + SideBuffer + Value * (D.Width - 2*SideBuffer) - Handle->W/2, P.Y + D.Height/2 - Handle->H/2 );	// trust me.
	Ren->DrawTile( Handle, FRectangle(P.X,P.Y,Handle->W,Handle->H), 0, GetPolyFlags(), GetColor() );

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USlider::Serialize( FArchive &Ar )
{
	guard(USlider::Serialize);
	NOTE(debugf(TEXT("(%s)USlider::Serialize"), *GetFullName() ));

	Super::Serialize(Ar);

//!!ARL	Ar << Handle;
	Ar << Format << SideBuffer << Step << Value << Min << Max;

	SERIALIZE_MOUSE(USlider);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	USoundComponent
*/


//------------------------------------------------------------------------------
void USoundComponent::Init()
{
	guard(USoundComponent::Init);
	NOTE(debugf(TEXT("(%s)USoundComponent::Init"), *GetFullName() ));

	LoadSound();
	RegisterEvent( UI_EVENT(PlaySound), this );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USoundComponent::LoadSound()
{
	guard(USoundComponent::LoadSound);
	NOTE(debugf(TEXT("(%s)USoundComponent::LoadSound"), *GetFullName() ));

	MySound = Cast<USound>(UObject::StaticLoadObject( USound::StaticClass(), NULL, *Sound, NULL, LOAD_NoWarn, NULL ));
	if(MySound) GetGConsole()->CollectAsset(MySound);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USoundComponent::PlaySound()
{
	guard(USoundComponent::PlaySound);
	NOTE(debugf(TEXT("(%s)USoundComponent::PlaySound: %s"), *GetFullName(), GetFullNameSafe(MySound) ));

	if( UObject::PurgingGarbage() )
		return;

	if( MySound && USound::Audio )
		USound::Audio->PlaySound( NULL, SLOT_None, MySound, GetConsole()->Viewport->Actor->Location, Volume, 4096.0, 1.0, (SF_NoUpdates | SF_No3D), 0.f );

//	if(MySound) GetConsole()->Viewport->Actor->PlaySound( MySound, SLOT_None, Volume, bInterrupt );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void USoundComponent::HandleEvent( uiEvent Event )
{
	guard(USoundComponent::HandleEvent);
	NOTE(debugf(TEXT("(%s)USoundComponent::HandleEvent: %s"), *GetFullName(), *LookupEventName(Event) ));

	// We don't care what event gets sent... just play the sound.
	// Leave the responsibility up to the user to ensure that we get registered with some component and some event.
	PlaySound();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UTextArea
*/


//------------------------------------------------------------------------------
void UTextArea::Constructed()
{
	guard(UTextArea::Constructed);
	NOTE(debugf(TEXT("(%s)UTextArea::Constructed"), *GetFullName() ));

	Super::Constructed();

	// Add a sentinel label to store default properties.
	UTextComponentBase* C=NEW_SUBCOMPONENT(UTextComponentBase);
	C->DrawOrder = -9999.0f;	// make sure it stays in slot 0.
	AddComponent(C);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
ULabel* UTextArea::CreateLabel()
{
	guard(UTextArea::CreateLabel);
	NOTE(debugf(TEXT("(%s)UTextArea::CreateLabel"), *GetFullName() ));

	return NEW_SUBCOMPONENT(ULabel);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextArea::SetFont( UFont* NewFont )
{
	guard(UTextArea::SetFont);
	NOTE(debugf(TEXT("(%s)UTextArea::SetFont"), *GetFullName() ));

	DynamicCastChecked<UTextComponentBase>(Components(0))->SetFont( NewFont );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UFont* UTextArea::GetFont( UCanvas* C )
{
	guard(UTextArea::GetFont);
	NOTE(debugf(TEXT("(%s)UTextArea::GetFont"), *GetFullName() ));

	return DynamicCastChecked<UTextComponentBase>(Components(0))->GetFont(C);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextArea::AddText( FString NewText, UBOOL bWrap )
{
	guard(UTextArea::AddText);
	NOTE(debugf(TEXT("(%s)UTextArea::AddText( %s, %s ) [%d]"), *GetFullName(), *NewText, bWrap ? TEXT("True") : TEXT("False"), NumComponents() ));

	if(bForceWrap) bWrap=true;
	if(bForceUnWrap) bWrap=false;

	// Fix ARL: Keep an index and treat our component list as a circular array so we're not reshuffling components so much.
	if( MaxLines )
	{
		INT NumExtra = (Components.Num()-SN - MaxLines)+1;
		for( INT i=0; i<NumExtra; i++ )
			GetUIConsole()->Incinerate(Components(i+SN));
		GetUIConsole()->MaybeFlushIncinerator();
	}

	ULabel* C = CreateLabel();
	AddComponent(C);
	C->bWrap = bWrap;
	C->SetFont( GetFont() );
	C->SetAlignment( Components(0)->GetAlignment() );
	C->SetStyle( Components(0)->GetStyle() );
	C->SetColor( Components(0)->GetColor() );
	C->SetText( NewText );

	if( StartLineIndex>0 )
		StartLineIndex++;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextArea::Empty()
{
	guard(UTextArea::Empty);
	NOTE(debugf(TEXT("(%s)UTextArea::Empty"), *GetFullName() ));

	for( INT i=SN; i<Components.Num(); i++ )
		GetUIConsole()->Incinerate(Components(i));
	GetUIConsole()->MaybeFlushIncinerator();

	StartLineIndex=0;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextArea::Update(UPDATE_DEF)
{
	guard(UTextArea::Update);
	NOTE0(debugf(TEXT("(%s)UTextArea::Update"), *GetFullName() ));

	IMPLEMENT_ACCESSOR_BEGIN(TextAreaAccess,TArray<FString>)
		while( SN<Components.Num() )	// Manually empty -- since incineration won't do, and it's perfectly safe at this point.
			Components.Last()->DeleteUObject();
		for(INT i=0;i<Result.Num();i++)
			AddText(Result(i));
	IMPLEMENT_ACCESSOR_END(TextAreaAccess)

	FPoint P = GetScreenCoords();

	FLOAT W = GetWidth();
	FLOAT H = GetHeight();
	FLOAT Y = bTopDown ? 0.0f : H;

	Ren->PushClip(FRectangle(P.X,P.Y,W,H));

	INT StartIndex,EndIndex; GetIndexBounds(StartIndex,EndIndex);
	for
	(	INT i =	bFIFO ? StartIndex + StartLineIndex	: EndIndex - StartLineIndex
	;			bFIFO ? i<=EndIndex					: i>=StartIndex
	;			bFIFO ? i++							: --i
	)
	{
		ULabel*	C = DynamicCastChecked<ULabel>(Components(i));

		if(!bTopDown) Y -= C->GetHeight();

		if(C->bWrap)	// set bottom wrapping bounds.
			C->SetSizeC(W,H-Y);

		C->SetLocationC(0,Y);
//		C->bDebug=bDebug;

		C->Update(UPDATE_PARMS);

		if(bTopDown) Y += C->GetHeight();

		// break out if out of bounds.
		if(bTopDown){ if(Y>H) break; }
		else        { if(Y<0) break; }
	}

	Ren->PopClip();

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UTextComponentBase
*/


//------------------------------------------------------------------------------
void UTextComponentBase::Constructed()
{
	guard(UTextComponentBase::Constructed);
	NOTE(debugf(TEXT("(%s)UTextComponentBase::Constructed"), *GetFullName() ));

	Super::Constructed();

	// Set default font.
	SetFont( CastChecked<UFont>(UObject::StaticLoadObject( UFont::StaticClass(), NULL, LocalizeGeneral(TEXT("DefaultFont"),TEXT("UI")), NULL, LOAD_NoWarn, NULL )) );	// Fix ARL: Unhardcode this.
	if(Font) GetGConsole()->CollectAsset(Font);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextComponentBase::SetFontSize( EFontSize NewSize )
{
	guard(UTextComponentBase::SetFontSize);
	NOTE(debugf(TEXT("(%s)UTextComponentBase::SetFontSize"), *GetFullName() ));

	FontSize = NewSize;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextComponentBase::SetFont( UFont* NewFont )
{
	guard(UTextComponentBase::SetFont);
	NOTE(debugf(TEXT("(%s)UTextComponentBase::SetFont"), *GetFullName() ));

	Font = NewFont;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UFont* UTextComponentBase::GetFont( UCanvas* C )
{
	guard(UTextComponentBase::GetFont);
	NOTE(debugf(TEXT("(%s)UTextComponentBase::GetFont"), *GetFullName() ));

	if( Font )		return Font;
//	else if( C )	return StaticGetScaledFont( C, FontSize );	// Fix ARL: Do we need support for FontSizes?
	else			return NULL;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FLOAT UTextComponentBase::GetFontHeight( UFont* F )
{
	guard(UTextComponentBase::GetFontHeight);
	NOTE(debugf(TEXT("(%s)UTextComponentBase::GetFontHeight"), *GetFullName() ));

	FLOAT W,H;
	GetConsole()->TextSize( F, TEXT("Test"), W, H );
	return H;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextComponentBase::AdjustSizeFromText( const FString &S )
{
	guard(UTextComponentBase::AdjustSizeFromText);
	NOTE(debugf(TEXT("(%s)UTextComponentBase::AdjustSizeFromText( %s )"), *GetFullName(), S ));

	UConsole* C=GetConsole(); check(C!=NULL);	// NOTE[aleiby]: You need to either add this component to a container that has a console, or use new(GetConsole()) class'Label'; before calling this function.
	FLOAT W=0.0f; FLOAT H=0.0f;
	C->TextSize( GetFont(), *S, W, H );
	SetSizeC(W,H);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}



/*
	UTextField
*/

// Fix ARL: This is a mess... clean it up!!

////////////////////
// Initialization //
////////////////////

//------------------------------------------------------------------------------
void UTextField::Constructed()
{
	guard(UTextField::Constructed);
	NOTE(debugf(TEXT("(%s)UTextField::Constructed"), *GetFullName() ));

	Super::Constructed();

//	AddIgnoredKey( **FString(TEXT("~")) );	// Fix ARL: This should probably be moved out of this class (it restricts reusablity).
//	AddIgnoredKey( **FString(TEXT("`")) );	// Fix ARL: This should probably be moved out of this class (it restricts reusablity).

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::Destroy()
{
	guard(UTextField::Destroy);
	NOTE(debugf(TEXT("UTextField::Destroy")));

	UUIConsole* C = CastChecked<UUIConsole>(GetConsole());
	C->RemoveInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UTextField,KeyEvent) );		// Fix ARL: Memory leak.
	C->RemoveKeyListener( UI_NATIVE_CALLBACK(None,this,UTextField,KeyType) );	// Fix ARL: Memory leak.

	if( Cursor )
		Cursor->DeleteUObject();

	Super::Destroy();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
UComponent* UTextField::GetComponentI( uiPath Path )
{
	guard(UTextField::GetComponentI);
	NOTE(debugf(TEXT("(%s)UTextField::GetComponentI( %s )"), *GetFullName(), *GetPathString(Path) ));

	// Check leaves and upstream connections.
	UComponent* C=Super::GetComponentI(Path);
	if(C) return C;

	// Check our cursor if we're next in the path.
	if( Cursor )
	{
		uiName N = Path.Pop();
		if( N==Name )
		{
			if( Path.Last()==Cursor->Name )
				return Cursor->GetComponent(Path);
		}
	}

	return NULL;
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}



////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
UBOOL UTextField::IsKeyIgnored( INT AsciiVal )
{
	guard(UTextField::IsKeyIgnored);
	NOTE(debugf(TEXT("(%s)UTextField::IsKeyIgnored( %d )"), *GetFullName(), AsciiVal ));

	for( INT i=0; i<IgnoredKeys.Num(); i++ )
		if( AsciiVal == IgnoredKeys(i) )
			return true;

	return false;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::AddIgnoredKey( INT AsciiVal )
{
	guard(UTextField::AddIgnoredKey);
	NOTE(debugf(TEXT("(%s)UTextField::AddIgnoredKey( %d )"), *GetFullName(), AsciiVal ));

	IgnoredKeys.AddItem( AsciiVal );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::RemoveIgnoredKey( INT AsciiVal )
{
	guard(UTextField::RemoveIgnoredKey);
	NOTE(debugf(TEXT("(%s)UTextField::RemoveIgnoredKey( %d )"), *GetFullName(), AsciiVal ));

	IgnoredKeys.RemoveItem( AsciiVal );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::SetInputRange( INT IMin, INT IMax )
{
	guard(UTextField::SetInputRange);
	NOTE(debugf(TEXT("(%s)UTextField::SetInputRange"), *GetFullName() ));

	InputRangeMax = Max(IMin,IMax);
	InputRangeMin = Min(IMin,IMax);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::BeginType(void*)
{
	guard(UTextField::BeginType);
	NOTE(debugf(TEXT("(%s)UTextField::BeginType"), *GetFullName() ));

	if( !bHasFocus )
	{
		bHasFocus = true;
		bWaitForDraw = true;	// ignores any input until we are drawn - this protects us from grabbing the keystroke that enabled us.
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::Deselect()
{
	guard(UTextField::Deselect);
	NOTE(debugf(TEXT("(%s)UTextField::Deselect"), *GetFullName() ));

	if( bHasFocus )
	{
		bHasFocus = false;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::SetCursor( UComponent* C )
{
	guard(UTextField::SetCursor);
	NOTE(debugf(TEXT("(%s)UTextField::SetCursor( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("None") ));

	if( Cursor )
		Cursor->DeleteUObject();

	Cursor = C;

	if( Cursor )
		Cursor->Parent = this;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::SetCursorPosition( INT i )
{
	guard(UTextField::SetCursorPosition);
	NOTE(debugf(TEXT("(%s)UTextField::SetCursorPosition"), *GetFullName() ));

	i = Clamp( i, 0, TypedStr.Len() );

	if( GetUIConsole()->bShift )
	{
		CursorOffset = i - CursorPos;
	}
	else
	{
		CursorPos = i;
		CursorOffset = 0;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT UTextField::GetCursorPosition()
{
	guard(UTextField::GetCursorPosition);
	NOTE(debugf(TEXT("(%s)UTextField::GetCursorPosition"), *GetFullName() ));

	return CursorPos + CursorOffset;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT UTextField::GetSelectionLeft()
{
	guard(UTextField::GetSelectionLeft);
	NOTE(debugf(TEXT("(%s)UTextField::GetSelectionLeft"), *GetFullName() ));

	return Min( CursorPos, CursorPos + CursorOffset );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT UTextField::GetSelectionRight()
{
	guard(UTextField::GetSelectionRight);
	NOTE(debugf(TEXT("(%s)UTextField::GetSelectionRight"), *GetFullName() ));

	return Max( CursorPos, CursorPos + CursorOffset );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
INT UTextField::GetSelectionSize()
{
	guard(UTextField::GetSelectionRight);
	NOTE(debugf(TEXT("(%s)UTextField::GetSelectionRight"), *GetFullName() ));

	return GetSelectionRight() - GetSelectionLeft();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::SetEnabledI( UBOOL bEnabled )
{
	guard(UTextField::SetEnabledI);
	NOTE(debugf(TEXT("(%s)UTextField::SetEnabledI"), *GetFullName() ));

	Super::SetEnabledI( bEnabled );

	if( IsEnabled() )
	{
		if( bAutoSelect )
			BeginType(NULL);
	}
	else
	{
		Deselect();
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
// Setup mouse click event.
//------------------------------------------------------------------------------
void UTextField::SetMouse( UMouseCursor* M )
{
	guard(UTextField::SetMouse);
	NOTE(debugf(TEXT("(%s)UTextField::SetMouse"), *GetFullName() ));

	SetMouseClickEvent( M, UI_NATIVE_CALLBACK(MouseClick,this,UTextField,BeginType) );
	SetMousePressEvent( M, UI_NATIVE_CALLBACK(MousePress,this,UTextField,MousePress) );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::MousePress( void* Parms )
{
	guard(UTextField::MousePress);
	NOTE(debugf(TEXT("(%s)UTextField::MousePress"), *GetFullName() ));

	if( bGreedyFocus )
		return;

	if( bHasFocus )
	{
		MouseNotifyParms* P = (MouseNotifyParms*)Parms;
		UMouseCursor* M = P->Mouse;

		if( !ContainsAbs( M->GetScreenCoords() ) )
		{
			Deselect();
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


//////////////////////
// Keyboard support //
//////////////////////

//------------------------------------------------------------------------------
void UTextField::SetConsole( UUIConsole* C )
{
	guard(UTextField::SetConsole);
	NOTE(debugf(TEXT("(%s)UTextField::SetConsole"), *GetFullName() ));

	C->AddInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UTextField,KeyEvent) );		// Fix ARL: Memory leak.
	C->AddKeyListener( UI_NATIVE_CALLBACK(None,this,UTextField,KeyType) );	// Fix ARL: Memory leak.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::KeyType( void* Parms )
{
	guard(UTextField::KeyType);
	NOTE(debugf(TEXT("(%s)UTextField::KeyType"), *GetFullName() ));

	if( bWaitForDraw ) return;

	KeyTypeParms* P = (KeyTypeParms*)Parms;
	EInputKey Key = P->Key;

	if( bHasFocus && Key>=InputRangeMin && Key<InputRangeMax && !IsKeyIgnored( Key ) )
	{
		// overwrite selected text.
		if( GetSelectionSize()>0 )
		{
			TypedStr.Delete( GetSelectionLeft(), GetSelectionSize() );
			CursorPos = GetSelectionLeft();
			CursorOffset = 0;
		}

		TypedStr.Insert( Chr(Key), CursorPos );
		CursorPos++;	// don't go through SetCursorPosition because that will select things when typing capital letters (due to use of shift key).
		bDirtyHist=true;

		P->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::KeyEvent( void* Parms )
{
	guard(UTextField::KeyEvent);
	NOTE(debugf(TEXT("(%s)UTextField::KeyEvent"), *GetFullName() ));

	if( bWaitForDraw ) return;

	KeyEventParms* P = (KeyEventParms*)Parms;

	EInputKey		Key		= P->Key;
//	EInputAction	Action	= P->Action;
//	FLOAT			Delta	= P->Delta;

	if( !bHasFocus ){ P->bHandled=false; return; }

	P->bHandled=true;	// eat all input - so keys don't get passed on to movement, etc bindings.

	if( Key==IK_Escape )
	{
		if( TypedStr!=TEXT("") )
		{
			TypedStr=TEXT("");
		}
		else
		{
			Deselect();
			SIMPLE_ACCESSOR(TextFieldEscapeEvent)
		}
	}
	else if( Key==IK_Enter )
	{
		if( TypedStr!=TEXT("") )
		{
			if( bDeselectOnEnter )
				Deselect();

			LastStr = TypedStr;
			
			// Record history.
			if( bDirtyHist )	// don't add history items back into the list.
			{
				HistoryNode* Node;
				Node = new HistoryNode(TypedStr);
				Node->Link(History);
			}
			bDirtyHist=true;

			// Fix ARL: Add validation function.  Example: MaxPlayers should only be a number between 0 and 16.  If someone types in 99, it should be changed to 16.
			FString Command = Prefix + TypedStr + Postfix;
			TypedStr=TEXT("");
			SIMPLE_ACCESSORX(TextFieldCommand,&Command)
			SIMPLE_ACCESSOR(TextFieldEnterEvent)
		}
	}
	else if( Key==IK_Backspace )
	{
		if( GetSelectionSize()>0 )
		{
			TypedStr.Delete( GetSelectionLeft(), GetSelectionSize() );
			SetCursorPosition( GetSelectionLeft() );
		}
		else if( GetCursorPosition()>0 )
		{
			SetCursorPosition( GetCursorPosition()-1 );
			TypedStr.Delete( GetCursorPosition() );
		}
	}
	else if( Key==IK_Delete )
	{
		if( GetSelectionSize()>0 )
		{
			TypedStr.Delete( GetSelectionLeft(), GetSelectionSize() );
			SetCursorPosition( GetSelectionLeft() );
		}
		else if( GetCursorPosition()<TypedStr.Len() )
		{
			TypedStr.Delete( GetCursorPosition() );
		}
	}
	else if( GetUIConsole()->bCtrl && Key==IK_Left )	// ctrl-left
	{
		// Fix ARL: Generalize to search for more than just spaces.
		SetCursorPosition( TypedStr.Left(GetCursorPosition()-1).InStr(TEXT(" "),1)+1 );
	}
	else if( GetUIConsole()->bCtrl && Key==IK_Right )	// ctrl-right
	{
		// Fix ARL: Generalize to search for more than just spaces.
		INT Pos = TypedStr.Mid(GetCursorPosition()).InStr(TEXT(" "));
		SetCursorPosition( (Pos != -1) ? GetCursorPosition()+Pos+1 : TypedStr.Len() );
	}
	else if( Key==IK_Left )
	{
		SetCursorPosition( GetCursorPosition()-1 );
	}
	else if( Key==IK_Right )
	{
		SetCursorPosition( GetCursorPosition()+1 );
	}
	else if( Key==IK_End )
	{
		SetCursorPosition( TypedStr.Len() );
	}
	else if( Key==IK_Home )
	{
		SetCursorPosition( 0 );
	}
	else if( Key==IK_Up )
	{
		if( History )
		{
			if( !bDirtyHist && History->Next )
				History = History->Next;
			bDirtyHist = false;
			TypedStr = *History;
		}
	}
	else if( Key==IK_Down )
	{
		if( History )
		{
			if( History->Prev )
				History = History->Prev;
			bDirtyHist = false;
			TypedStr = *History;
		}
	}
	// Fix ARL: Tab completion for MRU commands.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTextField::Update(UPDATE_DEF)
{
	guard(UTextField::Update);
	NOTE0(debugf(TEXT("(%s)UTextField::Update"), *GetFullName() ));

	bWaitForDraw = false;

	UCanvas* C=Frame->Viewport->Canvas;

	if( bHasFocus || bPersistent || AccessFlags & ACCESSOR_TextFieldAccess )
	{
		// Fix ARL: Only do this when TypedStr is actually changed?
		if( bAutoSize )
			AdjustSizeFromText( bHasFocus ? TypedStr : LastStr );

		FPoint P = GetScreenCoords();
		FDimension D = GetSize();

		// Draw cursor.
		if( Cursor && bHasFocus )
		{
			FLOAT W=0.0f; FLOAT H=0.0f;
			GetConsole()->TextSize( GetFont(), GetSelectionSize()>0 ? *TypedStr.Mid(GetSelectionLeft(),GetSelectionSize()) : GetCursorPosition()<TypedStr.Len() ? *TypedStr.Mid(CursorPos,1) : TEXT(" "), W, H );
			Cursor->SetSizeC(W,H);
			GetConsole()->TextSize( GetFont(), *TypedStr.Left(GetSelectionLeft()), W, H );
			Cursor->SetX(W);
			Cursor->Update(UPDATE_PARMS);
		}

		Ren->PushClip(FRectangle(P.X,P.Y,D.Width,D.Height));
//		C->DrawRectangle(P.X,P.Y,D.Width,D.Height);

		if( bHasFocus )
		{
			Ren->DrawString(*TypedStr,P.X,P.Y,GetFont(C),0,GetPolyFlags(),GetColor());
		}
		else if( bPersistent )
		{
			Ren->DrawString(*LastStr,P.X,P.Y,GetFont(C),0,GetPolyFlags(),TextColor);
		}
		else
		{
			SIMPLE_ACCESSOR_BEGIN(TextFieldAccess)
				FString S=TEXT("");
				Accessor->OnEvent(&S);
				Ren->DrawString(*S,P.X,P.Y,GetFont(C),0,GetPolyFlags(),TextColor);
			SIMPLE_ACCESSOR_END
		}

		Ren->PopClip();
	}

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UTileContainer
*/


//------------------------------------------------------------------------------
void UTileContainer::Update(UPDATE_DEF)
{
	guard(UTileContainer::Update);
	NOTE0(debugf(TEXT("(%s)UTileContainer::Update"), *GetFullName() ));

	FPoint P = bReverseJustify ? bVertical ? FPoint(GetWidth(),0) : FPoint(0,GetHeight()) : FPoint(0,0);

	if( bReverseOrder )
		for( INT i=Components.Num()-1; i>=0; --i )
		{
			UComponent* C=Components(i);
			if(!C->IsEnabled()){ if(bShortCircuit) break; else continue; }
			if(bReverseJustify)	C->SetLocation(P - (bVertical ? FDimension(C->GetWidth(),0) : FDimension(0,C->GetHeight())));
			else				C->SetLocation(P);
			C->Update(UPDATE_PARMS);
			if (bVertical)	P.Y += (FixedSpacing<0) ? C->GetHeight() : FixedSpacing;
			else			P.X += (FixedSpacing<0) ? C->GetWidth()  : FixedSpacing;
		}
	else	// Fix ARL: Maintanance hazzard!!  Move into a helper struct to remove code duplication?
		for( INT i=0; i<Components.Num(); i++ )
		{
			UComponent* C=Components(i);
			if(!C->IsEnabled()){ if(bShortCircuit) break; else continue; }
			if(bReverseJustify)	C->SetLocation(P - (bVertical ? FDimension(C->GetWidth(),0) : FDimension(0,C->GetHeight())));
			else				C->SetLocation(P);
			C->Update(UPDATE_PARMS);
			if (bVertical)	P.Y += (FixedSpacing<0) ? C->GetHeight() : FixedSpacing;
			else			P.X += (FixedSpacing<0) ? C->GetWidth()  : FixedSpacing;
		}

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
FRectangle UTileContainer::GetBoundsI()
{
	guard(UTileContainer::GetBoundsI);
	NOTE(debugf(TEXT("(%s)UTileContainer::GetBoundsI"), *GetFullName()));

	FRectangle Bounds(0,0,0,0);

	if(FixedSpacing<0)
	{
		if(bVertical)
		{
			for( INT i=0; i<Components.Num(); i++ )
			{
				UComponent* C = Components(i);
				if(!C->IsEnabled()) continue;
				FDimension D = C->GetSize();
				Bounds.Height += D.Height;
				Bounds.Width = Max( Bounds.Width, D.Width );
				NOTE(debugf(TEXT("   [v]Bounds: %s C.Size: %s (%s)"), *Bounds.String(), *D.String(), *C->GetFullName() ));
			}
		}
		else
		{
			for( INT i=0; i<Components.Num(); i++ )
			{
				UComponent* C = Components(i);
				if(!C->IsEnabled()) continue;
				FDimension D = C->GetSize();
				Bounds.Width += D.Width;
				Bounds.Height = Max( Bounds.Height, D.Height );
				NOTE(debugf(TEXT("   [h]Bounds: %s C.Size: %s (%s)"), *Bounds.String(), *D.String(), *C->GetFullName() ));
			}
		}
	}
	else
	{
		if(bVertical)
		{
			Bounds.Height = FixedSpacing * Components.Num();
			for( INT i=0; i<Components.Num(); i++ )
			{
				UComponent* C = Components(i);
				if(!C->IsEnabled()) continue;
				Bounds.Width = Max( Bounds.Width, C->GetWidth() );
				NOTE0(debugf(TEXT("   [vf]Bounds: %s C.Size: %s (%s)"), *Bounds.String(), *D.String(), *C->GetFullName() ));
			}
		}
		else
		{
			Bounds.Width = FixedSpacing * Components.Num();
			for( INT i=0; i<Components.Num(); i++ )
			{
				UComponent* C = Components(i);
				if(!C->IsEnabled()) continue;
				Bounds.Height = Max( Bounds.Height, C->GetHeight() );
				NOTE0(debugf(TEXT("   [hf]Bounds: %s C.Size: %s (%s)"), *Bounds.String(), *D.String(), *C->GetFullName() ));
			}
		}
	}

	return Bounds + GetLocation();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UTimedImage
*/


//------------------------------------------------------------------------------
void UTimedImage::Update(UPDATE_DEF)
{
	guard(UTimedImage::Update);
	NOTE(debugf(TEXT("(%s)UTimedImage::Update"), *GetFullName() ));

	// Update Lifespan.
	Lifespan -= GRealDeltaTime;

	// Adjust fade.
	FLOAT FadePct = 1.0f;
	if( FadeinPct )
	{
		FLOAT FadeStart = appBlend( InitialLifespan, 0.0f, FadeinPct );		// backwards because Lifespan counts down.
		if( Lifespan > FadeStart )
			FadePct *= appAblend( InitialLifespan, FadeStart, Lifespan );
	}
	if( FadeoutPct )
	{
		FLOAT FadeStart = appBlend( InitialLifespan, 0.0f, FadeoutPct );	// backwards because Lifespan counts down.
		if( Lifespan < FadeStart )
			FadePct *= appAblend( 0.0f, FadeStart, Lifespan );
	}
	if( FadePct<1.0f )
		SetAlpha( Clamp( FadePct * 256.0f, 0.0f, 255.0f ) );

	Super::Update(UPDATE_PARMS);

	if( Lifespan<0.0f )
		GetUIConsole()->Incinerate(this);	// queue for destruction.  (can't destroy immediately because that would screw up our parent's iteration)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTimedImage::SetLifespan( FLOAT Seconds )
{
	guard(UTimedImage::SetLifespan);
	NOTE(debugf(TEXT("(%s)UTimedImage::SetLifespan( %f )"), *GetFullName(), Seconds ));

	Lifespan = Seconds;
	InitialLifespan = Lifespan;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTimedImage::Reset()
{
	guard(UTimedImage::Reset);
	NOTE(debugf(TEXT("(%s)UTimedImage::Reset"), *GetFullName() ));

	GetUIConsole()->Incinerate(this);	// queue for destruction.  (can't destroy immediately because that would screw up our parent's iteration)

	Super::Reset();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UTimedLabel
*/


//------------------------------------------------------------------------------
void UTimedLabel::Update(UPDATE_DEF)
{
	guard(UTimedLabel::Update);
	NOTE(debugf(TEXT("(%s)UTimedLabel::Update"), *GetFullName() ));

	// Update Lifespan.
	Lifespan -= GRealDeltaTime;

	// Adjust fade.
	FLOAT FadePct = 1.0f;
	if( FadeinPct )
	{
		FLOAT FadeStart = appBlend( InitialLifespan, 0.0f, FadeinPct );		// backwards because Lifespan counts down.
		if( Lifespan > FadeStart )
			FadePct *= appAblend( InitialLifespan, FadeStart, Lifespan );
	}
	if( FadeoutPct )
	{
		FLOAT FadeStart = appBlend( InitialLifespan, 0.0f, FadeoutPct );	// backwards because Lifespan counts down.
		if( Lifespan < FadeStart )
			FadePct *= appAblend( 0.0f, FadeStart, Lifespan );
	}
//	if( FadePct<1.0f )
		SetAlpha( Clamp( FadePct * 256.0f, 0.0f, 255.0f ) );

	Super::Update(UPDATE_PARMS);

	if( Lifespan<0.0f )
		GetUIConsole()->Incinerate(this);	// queue for destruction.  (can't destroy immediately because that would screw up our parent's iteration)

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTimedLabel::SetLifespan( FLOAT Seconds )
{
	guard(UTimedLabel::SetLifespan);
	NOTE(debugf(TEXT("(%s)UTimedLabel::SetLifespan( %f )"), *GetFullName(), Seconds ));

	Lifespan = Seconds;
	InitialLifespan = Lifespan;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTimedLabel::Reset()
{
	guard(UTimedLabel::Reset);
	NOTE(debugf(TEXT("(%s)UTimedLabel::Reset"), *GetFullName() ));

	GetUIConsole()->Incinerate(this);	// queue for destruction.  (can't destroy immediately because that would screw up our parent's iteration)

	Super::Reset();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

/*
	UTimedTextArea
*/

//------------------------------------------------------------------------------
ULabel* UTimedTextArea::CreateLabel()
{
	guard(UTimedTextArea::CreateLabel);
	NOTE(debugf(TEXT("(%s)UTimedTextArea::CreateLabel"), *GetFullName() ));

	return NEW_SUBCOMPONENT(UTimedLabel);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UTimedTextArea::AddText( FString NewText, UBOOL bWrap )
{
	guard(UTimedTextArea::AddText);
	NOTE(debugf(TEXT("(%s)UTimedTextArea::AddText( %s, %s )"), *GetFullName(), *NewText, bWrap ? TEXT("True") : TEXT("False") ));

	if( MaxLines<=0 ) return;

	Super::AddText( NewText, bWrap );

	// Set new component's Lifespan.
	UTimedLabel* C=DynamicCastChecked<UTimedLabel>(Components.Last());
	C->SetLifespan( TextLifespan );
	C->FadeoutPct = FadeoutPct;
	
	// If there is more than one component - not counting the sentinel - then make sure
	// our Lifespan is at least MinInterval seconds longer than the previous.
	if( MinInterval && Components.Num()>(SN+1) )
	{
		UTimedLabel* Prev=DynamicCastChecked<UTimedLabel>(Components(Components.Num()-2));
		C->SetLifespan( Max( TextLifespan, Prev->Lifespan + MinInterval ) );
	}

	// Fix-up Lifespans due to possible removeage.
	INT StartIndex,EndIndex; GetIndexBounds(StartIndex,EndIndex);
	FLOAT Overflow	= DynamicCastChecked<UTimedLabel>(Components(StartIndex))->Lifespan - TextLifespan;
	if( Overflow>0.0f )
		for( INT i=1; i<Components.Num(); i++ )
			DynamicCastChecked<UTimedLabel>(Components(i))->Lifespan -= Overflow;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UVarWatcher
*/


//------------------------------------------------------------------------------
void UVarWatcher::SetWatch( UObject* WatchObj, FName PropName, INT ArrayIndex )
{
	guard(UVarWatcher::SetWatch);
	NOTE(debugf(TEXT("(%s)UVarWatcher::SetWatch( %s, %s )"), *GetFullName(), WatchObj ? WatchObj->GetFullName() : TEXT("NULL"), *PropName ));

	if(!WatchObj)
	{
		debugf( NAME_Warning, TEXT("(%s) SetWatch: WatchObj is NULL!"), *GetFullName() );
		return;
	}

	Obj = WatchObj;
	Prop = FindField<UProperty>( Obj->GetClass(), *PropName );
	Index = ArrayIndex;

	if(!Prop)
	{
		debugf( NAME_Warning, TEXT("(%s) SetWatch: Could not find property %s [%d] in %s!"), *GetFullName(), *PropName, ArrayIndex, Obj->GetClass()->GetName() );
		return;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UVarWatcher::Update(UPDATE_DEF)
{
	guard(UVarWatcher::Update);
	NOTE0(debugf(TEXT("(%s)UVarWatcher::Update"), *GetFullName() ));

	UCanvas* C=Frame->Viewport->Canvas;

	if( !Obj || !Prop ) return;

	TCHAR Temp[256];
	TCHAR* Str=&Temp[0];
	Prop->ExportText( Index, Str, (BYTE*)Obj, (BYTE*)Obj, PPF_Localized );

	FPoint P = GetScreenCoords();

	if(!bFullDesc)
		Ren->DrawString(Str,P.X,P.Y,GetFont(C),0,GetPolyFlags(),GetColor());
	if(Index)
		Ren->DrawString(*FString::Printf(TEXT("%s.%s[%d]: %s"),Obj->GetFullName(),Prop->GetName(),Index,Str),P.X,P.Y,GetFont(C),0,GetPolyFlags(),GetColor());
	else
		Ren->DrawString(*FString::Printf(TEXT("%s.%s: %s"),Obj->GetFullName(),Prop->GetName(),Str),P.X,P.Y,GetFont(C),0,GetPolyFlags(),GetColor());

	UComponent::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

/*
	UWindow
*/

////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UWindow::SetImages
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
)
{
	guard(UWindow::SetImages);
	NOTE(debugf(TEXT("(%s)UWindow::SetImages"), *GetFullName() ));

	if( Components.Num() )
		RemoveComponentType( UImageComponent::StaticClass(), true, true );

	UImageComponent* Image;
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=Top;			Image->Init(); Image->SetAlignment( FAlignment(ALMT_Center,ALMT_Top)	);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=Bottom;		Image->Init(); Image->SetAlignment( FAlignment(ALMT_Center,ALMT_Bottom)	);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=Left;		Image->Init(); Image->SetAlignment( FAlignment(ALMT_Left,ALMT_Center)	);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=Right;		Image->Init(); Image->SetAlignment( FAlignment(ALMT_Right,ALMT_Center)	);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=NE;			Image->Init(); Image->SetAlignment( FAlignment(ALMT_Right,ALMT_Top)		);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=NW;			Image->Init(); Image->SetAlignment( FAlignment(ALMT_Left,ALMT_Top)		);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=SE;			Image->Init(); Image->SetAlignment( FAlignment(ALMT_Right,ALMT_Bottom)	);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=SW;			Image->Init(); Image->SetAlignment( FAlignment(ALMT_Left,ALMT_Bottom)	);
	Image = NEW_SUBCOMPONENT(UImageComponent); AddComponent( Image ); Image->Image=Background;	Image->Init(); Image->SetAlignment( FAlignment(ALMT_Center,ALMT_Center)	);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

/*
	UEditor
*/

///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UEditor::SetMouse( UMouseCursor* M )
{
	guard(UEditor::SetMouse);
	NOTE(debugf(TEXT("(%s)UEditor::SetMouse"), GetFullName() ));

	SetMousePressEvent(M, UI_NATIVE_CALLBACK(MousePress,this,UEditor,MousePress));
	SetMouseReleaseEvent(M, UI_NATIVE_CALLBACK(MouseRelease,this,UEditor,MouseRelease));

	Mover		= NEW_COMPONENT(UMoveHandle);		AddComponent(Mover);		Mover->Init();		Mover->SetMouse(M);
	ResizerN	= NEW_COMPONENT(UResizeHandleN);	AddComponent(ResizerN);		ResizerN->Init();	ResizerN->SetMouse(M);
	ResizerS	= NEW_COMPONENT(UResizeHandleS);	AddComponent(ResizerS);		ResizerS->Init();	ResizerS->SetMouse(M);
	ResizerE	= NEW_COMPONENT(UResizeHandleE);	AddComponent(ResizerE);		ResizerE->Init();	ResizerE->SetMouse(M);
	ResizerW	= NEW_COMPONENT(UResizeHandleW);	AddComponent(ResizerW);		ResizerW->Init();	ResizerW->SetMouse(M);
	ResizerNE	= NEW_COMPONENT(UResizeHandleNE);	AddComponent(ResizerNE);	ResizerNE->Init();	ResizerNE->SetMouse(M);
	ResizerNW	= NEW_COMPONENT(UResizeHandleNW);	AddComponent(ResizerNW);	ResizerNW->Init();	ResizerNW->SetMouse(M);
	ResizerSE	= NEW_COMPONENT(UResizeHandleSE);	AddComponent(ResizerSE);	ResizerSE->Init();	ResizerSE->SetMouse(M);
	ResizerSW	= NEW_COMPONENT(UResizeHandleSW);	AddComponent(ResizerSW);	ResizerSW->Init();	ResizerSW->SetMouse(M);

#if 1 //HACK
	Mover		->Image = GetFImageMap().FindRef(TEXT("Red")); Mover		->SetSize(FDimension(8,8));
	ResizerN	->Image = GetFImageMap().FindRef(TEXT("Blue")); ResizerN	->SetSize(FDimension(8,8));
	ResizerS	->Image = GetFImageMap().FindRef(TEXT("Blue")); ResizerS	->SetSize(FDimension(8,8));
	ResizerE	->Image = GetFImageMap().FindRef(TEXT("Blue")); ResizerE	->SetSize(FDimension(8,8));
	ResizerW	->Image = GetFImageMap().FindRef(TEXT("Blue")); ResizerW	->SetSize(FDimension(8,8));
	ResizerNE	->Image = GetFImageMap().FindRef(TEXT("Yellow")); ResizerNE	->SetSize(FDimension(8,8));
	ResizerNW	->Image = GetFImageMap().FindRef(TEXT("Yellow")); ResizerNW	->SetSize(FDimension(8,8));
	ResizerSE	->Image = GetFImageMap().FindRef(TEXT("Yellow")); ResizerSE	->SetSize(FDimension(8,8));
	ResizerSW	->Image = GetFImageMap().FindRef(TEXT("Yellow")); ResizerSW	->SetSize(FDimension(8,8));
#endif

	unguard;
}

//------------------------------------------------------------------------------
void UEditor::SetConsole( UUIConsole* C )
{
	guard(UEditor::SetConsole);
	NOTE(debugf(TEXT("(%s)UEditor::SetConsole"), GetFullName() ));

	C->AddInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UEditor,KeyEvent) );		// Fix ARL: Memory leak.

	unguard;
}


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
void UEditor::MousePress( void* Parms )
{
	guard(UEditor::MousePress);
	NOTE(debugf(TEXT("(%s)UEditor::MousePress"), GetFullName() ));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;

	if( IsEnabled() )
	{
		Current = GetRoot()->GetComponentAt( P->Mouse->GetScreenCoords() );
		NOTE(debugf(TEXT("Editor got %s"), Current ? Current->GetFullName() : TEXT("NULL") ));
	}

	unguard;
}

//------------------------------------------------------------------------------
void UEditor::MouseRelease( void* Parms )
{
	guard(UEditor::MouseRelease);
	NOTE(debugf(TEXT("(%s)UEditor::MouseRelease"), GetFullName() ));

	MouseNotifyParms* P = (MouseNotifyParms*)Parms;

	if( IsEnabled() )
	{
		if( GetRoot()->GetComponentAt( P->Mouse->GetScreenCoords() ) == Current )
		{
			Select( Current );
		}
		else
		{
			Select( NULL );
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
void UEditor::KeyEvent( void* Parms )
{
	guard(UEditor::KeyEvent);
	NOTE(debugf(TEXT("(%s)UEditor::KeyEvent"), GetFullName() ));

	KeyEventParms* P = (KeyEventParms*)Parms;

	EInputKey		Key		= P->Key;
//	EInputAction	Action	= P->Action;
//	FLOAT			Delta	= P->Delta;

	if( IsEnabled() && Mover->Target )
	{
		// Extra hacky since FDimensions only like being created with positive values -- I wonder if that's screwing up calculations elsewhere.
		#define DEF_DIR(w,h) FDimension Dir; Dir.Width=w; Dir.Height=h;

		switch( Key )
		{

		// Nudge component.
		case IK_Up:{	NOTE(debugf(TEXT("Up")));			DEF_DIR( 0,-1)
			Mover->UpdateComponent( Mover->Target,			Dir );
			P->bHandled=true; return;}
		case IK_Down:{	NOTE(debugf(TEXT("Down")));			DEF_DIR( 0, 1)
			Mover->UpdateComponent( Mover->Target,			Dir );
			P->bHandled=true; return;}
		case IK_Left:{	NOTE(debugf(TEXT("Left")));			DEF_DIR(-1, 0)
			Mover->UpdateComponent( Mover->Target,			Dir );
			P->bHandled=true; return;}
		case IK_Right:{	NOTE(debugf(TEXT("Right")));		DEF_DIR( 1, 0)
			Mover->UpdateComponent( Mover->Target,			Dir );
			P->bHandled=true; return;}

		// Resize component.
		case IK_Z:{		NOTE(debugf(TEXT("Z")));			DEF_DIR( 0,-1)
			ResizerS->UpdateComponent( ResizerS->Target,	Dir );
			P->bHandled=true; return;}
		case IK_A:{		NOTE(debugf(TEXT("A")));			DEF_DIR( 0, 1)
			ResizerS->UpdateComponent( ResizerS->Target,	Dir );
			P->bHandled=true; return;}
		case IK_X:{		NOTE(debugf(TEXT("X")));			DEF_DIR(-1, 0)
			ResizerE->UpdateComponent( ResizerS->Target,	Dir );
			P->bHandled=true; return;}
		case IK_S:{		NOTE(debugf(TEXT("S")));			DEF_DIR( 1, 0)
			ResizerE->UpdateComponent( ResizerS->Target,	Dir );
			P->bHandled=true; return;}

		// Special ImageWedge hack.
		case IK_C:{
			UImageWedge* Wedge=DynamicCast<UImageWedge>(Mover->Target);
			if(Wedge) Wedge->Angle-=1.0f;
			P->bHandled=true; return;}
		case IK_D:{
			UImageWedge* Wedge=DynamicCast<UImageWedge>(Mover->Target);
			if(Wedge) Wedge->Angle+=1.0f;
			P->bHandled=true; return;}
		case IK_V:{
			UImageWedge* Wedge=DynamicCast<UImageWedge>(Mover->Target);
			if(Wedge) Wedge->Angle-=0.1f;
			P->bHandled=true; return;}
		case IK_F:{
			UImageWedge* Wedge=DynamicCast<UImageWedge>(Mover->Target);
			if(Wedge) Wedge->Angle+=0.1f;
			P->bHandled=true; return;}
		case IK_B:{
			UImageWedge* Wedge=DynamicCast<UImageWedge>(Mover->Target);
			if(Wedge) Wedge->Radius-=1.0f;
			P->bHandled=true; return;}
		case IK_G:{
			UImageWedge* Wedge=DynamicCast<UImageWedge>(Mover->Target);
			if(Wedge) Wedge->Radius+=1.0f;
			P->bHandled=true; return;}

		// Select parent.
		case IK_P:		NOTE(debugf(TEXT("P")));
			{
			UComponent* NewComponent = Mover->Target->Parent;
			if( NewComponent )
				Select( NewComponent );
			P->bHandled=true; return;
			}

		// Select child.
		case IK_O:		NOTE(debugf(TEXT("O")));
			{
			UContainer* Target = DynamicCast<UContainer>(Mover->Target);
			if( Target && Target->NumComponents() )
				Select( Target->GetComponent(0) );
			P->bHandled=true; return;
			}

		// Select sibling.
		case IK_I:		NOTE(debugf(TEXT("I")));
			{
			UComponent* Target = Mover->Target;
			UContainer* TargetParent = DynamicCast<UContainer>(Target->Parent);
			if( TargetParent && TargetParent->NumComponents()>1 )
				Select( TargetParent->GetComponent( (TargetParent->GetComponentIndex(Target)+1) % TargetParent->NumComponents() ) );
			P->bHandled=true; return;
			}
		};
	}

	P->bHandled=false; return;

	unguard;
}

//------------------------------------------------------------------------------
void UEditor::Select( UComponent* NewComponent )
{
	guard(UEditor::Select);
	NOTE(debugf(TEXT("(%s)UEditor::Select( %s )"), GetFullName(), NewComponent ? NewComponent->GetFullName() : TEXT("NULL") ));

	Mover->Target		= NewComponent;		Mover->Parent		= NewComponent;
	ResizerN->Target	= NewComponent;		ResizerN->Parent	= NewComponent;
	ResizerS->Target	= NewComponent;		ResizerS->Parent	= NewComponent;
	ResizerE->Target	= NewComponent;		ResizerE->Parent	= NewComponent;
	ResizerW->Target	= NewComponent;		ResizerW->Parent	= NewComponent;
	ResizerNE->Target	= NewComponent;		ResizerNE->Parent	= NewComponent;
	ResizerNW->Target	= NewComponent;		ResizerNW->Parent	= NewComponent;
	ResizerSE->Target	= NewComponent;		ResizerSE->Parent	= NewComponent;
	ResizerSW->Target	= NewComponent;		ResizerSW->Parent	= NewComponent;

	BringToFront();

	unguard;
}

//------------------------------------------------------------------------------
void UEditor::Destroy()
{
	guard(UEditor::Destroy);
	NOTE(debugf(TEXT("(%s)UEditor::Destroy"), GetFullName() ));

	Mover->Parent		= this;
	ResizerN->Parent	= this;
	ResizerS->Parent	= this;
	ResizerE->Parent	= this;
	ResizerW->Parent	= this;
	ResizerNE->Parent	= this;
	ResizerNW->Parent	= this;
	ResizerSE->Parent	= this;
	ResizerSW->Parent	= this;

	UUIConsole* C = CastChecked<UUIConsole>(GetConsole());
	C->RemoveInputActionListener( IST_Press, this, UI_NATIVE_CALLBACK(None,this,UEditor,KeyEvent) );		// Fix ARL: Memory leak.

	Super::Destroy();

	unguard;
}

/*
	UComponentHandle
*/

///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UComponentHandle::SetMouse( UMouseCursor* M )
{
	guard(UComponentHandle::SetMouse);
	NOTE(debugf(TEXT("(%s)UComponentHandle::SetMouse"), *GetFullName() ));

	SetMousePressEvent(M, UI_NATIVE_CALLBACK(MousePress,this,UComponentHandle,MousePress));
	SetMouseReleaseEvent(M, UI_NATIVE_CALLBACK(MouseRelease,this,UComponentHandle,MouseRelease));

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////////
// Mouse support //
///////////////////

//------------------------------------------------------------------------------
void UComponentHandle::MousePress( void* Parms )
{
	guard(UComponentHandle::MousePress);
	NOTE(debugf(TEXT("(%s)UComponentHandle::MousePress"), *GetFullName() ));

	if( Target )
	{
		MouseNotifyParms* P = (MouseNotifyParms*)Parms;
		FPoint MousePos = P->Mouse->GetScreenCoords();

		if( ContainsAbs( MousePos ) )
		{
			Mouse = P->Mouse;
			LastMousePos = MousePos;
			P->bHandled=true;
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponentHandle::MouseRelease( void* Parms )
{
	guard(UComponentHandle::MouseRelease);
	NOTE(debugf(TEXT("(%s)UComponentHandle::MouseRelease"), *GetFullName() ));

	if( Mouse )
	{
		Mouse = NULL;
		((MouseNotifyParms*)Parms)->bHandled=true;
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UComponentHandle::Serialize( FArchive &Ar )
{
	guard(UComponentHandle::Serialize);
	NOTE(debugf(TEXT("(%s)UComponentHandle::Serialize"), *GetFullName() ));

	Super::Serialize(Ar);

	SERIALIZE_PATHED_COMPONENT(UComponentHandle,Target);

	SERIALIZE_MOUSE(UComponentHandle);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UComponentHandle::Update(UPDATE_DEF)
{
	guard(UComponentHandle::Update);
	NOTE(debugf(TEXT("(%s)UComponentHandle::Update"), *GetFullName() ));

	if( Target )
	{
		Super::Update(UPDATE_PARMS);

		if( Mouse )
		{
			FPoint MousePos = Mouse->GetScreenCoords();
			UpdateComponent( Target, MousePos - LastMousePos );
			LastMousePos = MousePos;
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMoveHandle::Update(UPDATE_DEF)
{
	guard(UMoveHandle::Update);
	NOTE(debugf(TEXT("(%s)UMoveHandle::Update"), *GetFullName() ));

	Super::Update(UPDATE_PARMS);

	ULabel* Text = NEW_TRANSCOMPONENT(ULabel);
	DynamicCastChecked<UContainer>(GetGConsole()->Root)->AddComponent(Text);
	UImageWedge* Wedge=DynamicCast<UImageWedge>(Target);
	if(Wedge)	Text->SetText( FString::Printf( TEXT("%s: %i %i %i %i %3.1f %3.1f"),*Target->GetFullName(),(INT)Target->GetX(),(INT)Target->GetY(),(INT)Target->GetWidth(),(INT)Target->GetHeight(),Wedge->Radius,Wedge->Angle ) ); else
	if(Target)	Text->SetText( FString::Printf( TEXT("%s: %i %i %i %i"),*Target->GetFullName(),(INT)Target->GetX(),(INT)Target->GetY(),(INT)Target->GetWidth(),(INT)Target->GetHeight() ) );
	else		Text->SetText(TEXT("None"));
	Text->SetAlignment(FAlignment(ALMT_HardCenter,ALMT_HardTop));
	Text->Update(UPDATE_PARMS);
	Text->DeleteUObject();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UMoveHandle::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UMoveHandle::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UMoveHandle::UpdateComponent( %s, %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL"), *Delta.String() ));

	C->SetLocation( C->GetLocation() + Delta );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleN::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleN::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleN::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	C->SetHeight( C->GetHeight() - Delta.Height );
	C->SetY( C->GetY() + Delta.Height );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleS::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleS::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleS::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	C->SetHeight( C->GetHeight() + Delta.Height );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleE::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleE::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleE::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	C->SetWidth( C->GetWidth() + Delta.Width );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleW::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleW::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleW::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	C->SetWidth( C->GetWidth() - Delta.Width );
	C->SetX( C->GetX() + Delta.Width );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleNE::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleNE::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleNE::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	Delta.Width = -Delta.Width;
	C->SetSize( C->GetSize() - Delta );
	C->SetY( C->GetY() + Delta.Height );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleNW::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleNW::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleNW::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	C->SetSize( C->GetSize() - Delta );
	C->SetLocation( C->GetLocation() + Delta );
	
	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleSE::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleSE::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleSE::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	C->SetSize( C->GetSize() + Delta );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UResizeHandleSW::UpdateComponent( UComponent* C, FDimension Delta )
{
	guard(UResizeHandleSW::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UResizeHandleSW::UpdateComponent( %s )"), *GetFullName(), C ? *C->GetFullName() : TEXT("NULL") ));

	Delta.Height = -Delta.Height;
	C->SetSize( C->GetSize() - Delta );
	C->SetX( C->GetX() + Delta.Width );

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UCredits
*/

///////////////
// Interface //
///////////////

//------------------------------------------------------------------------------
void UCredits::Start()
{
	guard(UCredits::Start);
	NOTE(debugf(TEXT("(%s)UCredits::Start"), *GetFullName() ));

	// reset if necessary.
	if( bOn )
		Stop();

	Index = 0;
	Timer = 0.f;

	bOn = true;

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UCredits::Stop()
{
	guard(UCredits::Stop);
	NOTE(debugf(TEXT("(%s)UCredits::Stop"), *GetFullName() ));

	if( bOn )
	{
		for( INT i=0; i<Components.Num(); i++ )
			GetUIConsole()->Incinerate( Components(i) );

		bOn = false;

		SendGlobalEvents( ((FLOAT)Index)/((FLOAT)Elements.Num()), 1000.f );
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
void UCredits::UpdateComponent( UComponent* C, FLOAT DeltaTime )
{
	guard(UCredits::UpdateComponent);
	NOTE(debugf(TEXT("(%s)UCredits::UpdateComponent"), *GetFullName() ));

	// error check.
	if(!C){ debugf(NAME_Warning,TEXT("UCredits::UpdateComponent called with NULL component!")); return; }

	// precalc.
	FLOAT OldY = C->GetY();
	FLOAT NewY = OldY - (Rate * DeltaTime);
	FLOAT UpperLimit = 0.f - C->GetHeight();

	// kill finished components.
	if( NewY <= UpperLimit )
		{ GetUIConsole()->Incinerate(C); return; }

	// update location.
	C->SetY( NewY );

	// send events.
	FLOAT LowerLimit = GetHeight();
	FLOAT OldPct = appAblend( LowerLimit, UpperLimit, OldY );
	FLOAT NewPct = appAblend( LowerLimit, UpperLimit, NewY );
	for( INT i=0; i<LocalEvents.Num(); i++ )
	{
		FCreditEventLocal &Event=LocalEvents(i);
		if( OldPct <= Event.Pct && Event.Pct < NewPct )
		{
			NOTE(debugf(TEXT("sending event: %s"),*LookupEventName(Event.Event)));
			C->HandleEvent( Event.Event );
		}
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UCredits::SendGlobalEvents( FLOAT OldPct, FLOAT NewPct )
{
	guard(UCredits::SendGlobalEvents);
	NOTE(debugf(TEXT("(%s)UCredits::SendGlobalEvents"), *GetFullName() ));

	for( INT i=0; i<GlobalEvents.Num(); i++ )
	{
		FCreditEventGlobal &Event=GlobalEvents(i);
		if( OldPct <= Event.Pct && Event.Pct < NewPct )
			Event.Event->OnEvent();
	}

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

///////////////
// Overrides //
///////////////

//------------------------------------------------------------------------------
void UCredits::Update(UPDATE_DEF)
{
	guard(UCredits::Update);
	NOTE0(debugf(TEXT("(%s)UCredits::Update"), *GetFullName() ));

	// update existing components.
	for( INT i=0; i<Components.Num(); i++ )
		UpdateComponent( Components(i), GRealDeltaTime );

	// create new components as necessary.
	Timer += GRealDeltaTime;
	while( Timer > 0.f && Index < Elements.Num() )
	{
		FString Tag=TEXT("");
		FString Text=TEXT("");
		if( !Elements(Index++).Split(TEXT("|"),&Tag,&Text) )
			{ debugf(NAME_Warning,TEXT("Credits: No Tag delimitor in Element[%i]"),Index-1); continue; }
		//NOTE[aleiby]: Skipping delimitless entries above will potentially cause us to skip global events, however that itself is a bug in any case that would need to be fixed regardless.
		//NOTE[aleiby]: We could make the sending of global events more accurate by taking Timer value into account, but that would require knowing the total time for all credits which we won't know until loading them all and adding up their heights.  That seems unnecessary.
		// Fix ARL: Maybe factor in the time it takes to get from the bottom of the screen to the top somehow?  (GetHeight()/Rate)
		SendGlobalEvents( ((FLOAT)(Index-1))/((FLOAT)Elements.Num()), ((FLOAT)Index)/((FLOAT)Elements.Num()) );
		FString* ComponentName = Styles.Find(Tag);
		if( !ComponentName )
			{ debugf(NAME_Warning,TEXT("Credits: No Style matching Tag '%s' in Element[%i]"),*Tag,Index-1); continue; }
		UComponent* C = ImportComponent( *ComponentName, PARENT, NULL );
		if( !C )
			{ debugf(NAME_Warning,TEXT("Credits: Component '%s' not found for Style Tag '%s' in Element[%i]"),*ComponentName,*Tag,Index-1); continue; }
		TArray<UComponent*> Labels;
		static uiName LabelName = uiName(TEXT("CreditLabel"));
		C->GetLooseComponents( LabelName, Labels );
		if( !Labels.Num() )
			{ debugf(NAME_Warning,TEXT("Credits: No 'CreditLabel' component found for Style Tag '%s' in Element[%i]"),*ComponentName,*Tag,Index-1); continue; }
		for( INT i=0; i<Labels.Num(); i++ )
			DynamicCastChecked<ULabel>(Labels(i))->SetText( Text );
		AddComponent( C );
		C->SetY( GetHeight() );
		UpdateComponent( C, Timer );
		Timer -= C->GetHeight() / Rate;
	}

	// automatically shutdown when finished.
	if( !Components.Num() )
		Stop();

	Super::Update(UPDATE_PARMS);

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

//------------------------------------------------------------------------------
void UCredits::HandleEvent( uiEvent Event )
{
	guard(UCredits::HandleEvent);
	NOTE(debugf(TEXT("(%s)UCredits::HandleEvent( %s )"), *GetFullName(), *LookupEventName(Event) ));

	if( Event==StartEvent )
		Start();

	if( Event==StopEvent )
		Stop();

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}


/*
	UMaterialContainer
*/

////////////////
// UTexUIhook //
////////////////

//
// UTexUIhook
//
FMatrix* UTexUIhook::GetMatrix(FLOAT TimeSeconds)
{
	NOTE(debugf(TEXT("UTexUIhook::GetMatrix: ComponentName=%s bRelative=%d bPan=%d bScale=%d C=%s"),*ComponentName,bRelative,bPan,bScale,C?*C->GetFullName():TEXT("na")));
	FLOAT x=0.f,y=0.f;
	FLOAT w=1.f,h=1.f;
	if (C)
	{
		if (bRelative)
		{
			FRectangle B=C->GetScreenBounds();
			if (bPan)
			{
				x = Ri.X - B.X;
				y = Ri.Y - B.Y;
			}
			if (bScale)
			{
				if (B.Width)
				{
					FLOAT UScale=B.Width/Ri.Width;
					w = 1.f/UScale;
					FLOAT dx = Ri.X - Rf.X;
					x += dx - dx/UScale;
				}
				if (B.Height)
				{
					FLOAT VScale=B.Height/Ri.Height;
					h = 1.f/VScale;
					FLOAT dy = Ri.Y - Rf.Y;
					y += dy - dy/VScale;
				}
			}
			x /= Rf.Width;
			y /= Rf.Height;
		}
		else
		{
			if (bPan)
			{
				FPoint P=C->GetLocation();
				x = -P.X;
				y = -P.Y;
			}
			if (bScale)
			{
				FDimension D=C->GetSize();
				if (D.Width)
				{
					w = 1.f/D.Width;
					FLOAT dx = (Ri.X - Rf.X)/Rf.Width;
					x += dx - dx/D.Width;
				}
				if (D.Height)
				{
					h = 1.f/D.Height;
					FLOAT dy = (Ri.Y - Rf.Y)/Rf.Height;
					y += dy - dy/D.Height;
				}
			}
		}
	}
	M = FMatrix	(
					FPlane(w,0,0,0),
					FPlane(0,h,0,0),
					FPlane(x,y,1,0),
					FPlane(0,0,0,1)
				);
	// Fix ARL: Maybe add rotation?		M *= FRotationMatrix(R);
	return &M;
}
IMPLEMENT_CLASS(UTexUIhook);

///////////////
// Overrides //
///////////////

void UMaterialContainer::Update(UPDATE_DEF)
{
	guard(UMaterialContainer::Update);
	NOTE0(debugf(TEXT("(%s)UMaterialContainer::Update"), *GetFullName() ));

	// helper minion for iterating submaterials and linking up TexUIhooks with their appropriate ui components.
	struct Recursive
	{
		static void Exec( UMaterialContainer* Owner, UObject* M, const FRectangle& Ri, const FRectangle& Rf )
		{
			// actual work is done here.
			UTexUIhook* H=Cast<UTexUIhook>(M); if(H)
			{
				H->Ri=Ri;
				H->Rf=Rf;
				H->C=Owner->FindLooseComponent(uiName(H->ComponentName));	// Fix ARL: Cache name lookup. (don't cache uicomponent since multiple MaterialContainers may share same material)
				if (!H->C) debugf(NAME_Warning,TEXT("Unable to find '%s' in '%s' for '%s'!"),*H->ComponentName,*Owner->GetFullName(),H->GetFullName());
			}
			// the rest of this is for iterating submaterials.
			for( TFieldIterator<UObjectProperty> It(M->GetClass()); It; ++It )
				if( It->PropertyClass->IsChildOf( UMaterial::StaticClass() ) )
					for( INT i=0; i<It->ArrayDim; i++ )
					{
						UMaterial* Material = *(UMaterial**)((BYTE*)M + It->Offset + i*It->ElementSize);
						if (Material && Material!=M) Recursive::Exec( Owner, Material, Ri, Rf );
					}
		}
		static void ExecClear( UMaterialContainer* Owner, UObject* M )
		{
			// actual work is done here.
			UTexUIhook* H=Cast<UTexUIhook>(M); if(H)
			{
				H->C=NULL;
			}
			// the rest of this is for iterating submaterials.
			for( TFieldIterator<UObjectProperty> It(M->GetClass()); It; ++It )
				if( It->PropertyClass->IsChildOf( UMaterial::StaticClass() ) )
					for( INT i=0; i<It->ArrayDim; i++ )
					{
						UMaterial* Material = *(UMaterial**)((BYTE*)M + It->Offset + i*It->ElementSize);
						if (Material && Material!=M) Recursive::ExecClear( Owner, Material );
					}
		}
	};

	for( INT i=0; i<Components.Num(); i++ )
	{
		UComponent* C=Components(i);
		if (!C->IsEnabled()) continue;
		UImageComponentBase* I=DynamicCast<UImageComponentBase>(C); if(I)
		{
			FImage* Image=I->GetImage();
			UMaterial* Material=Image->Material;
			// get screen extents.
			FRectangle Ri=I->GetScreenBounds();
			// expand to full material size.
			FRectangle Rf;
			FLOAT UScale=Ri.Width/Image->W;
			FLOAT VScale=Ri.Height/Image->H;
			Rf.X = Ri.X - (Image->X * UScale);
			Rf.Y = Ri.Y - (Image->Y * VScale);
			Rf.Width = Material->MaterialUSize()*UScale;
			Rf.Height = Material->MaterialVSize()*VScale;
			// recursively hook up all TexUIhooks.
			Recursive::Exec( this, Material, Ri, Rf );
			// draw the image.
			I->Update(UPDATE_PARMS);
			// force polys to be drawn using current hook bindings (in case multiple image components share the same material).
			Ren->Flush();
			// clear references to avoid crashes later on -- like during precaching.
			Recursive::ExecClear( this, Material );
		}
		else C->Update(UPDATE_PARMS);
	}

	UComponent::Update(UPDATE_PARMS);	// don't call Super since we've already done all the work.

	unguardf(( TEXT("(%s)"), *GetFullName() ));
}

