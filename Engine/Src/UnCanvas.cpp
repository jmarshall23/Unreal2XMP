/*=============================================================================
	UnCanvas.cpp: Unreal canvas rendering.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney
	* 31/3/99 Updated revision history - Jack Porter
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	UCanvas scaled sprites.
-----------------------------------------------------------------------------*/

//
// Draw arbitrary aligned rectangle.
//
void UCanvas::DrawTile
(
	UMaterial*		Material,
	FLOAT			X,
	FLOAT			Y,
	FLOAT			XL,
	FLOAT			YL,
	FLOAT			U,
	FLOAT			V,
	FLOAT			UL,
	FLOAT			VL,
	FLOAT			Z,
	FPlane			Color,
	FPlane			Fog
)
{
	guard(UCanvas::DrawTile);

    if ( !pCanvasUtil || !Material )
        return;

	// Don't let empty textures get drawn (see TextureArray).
	if( !Material->MaterialUSize() || !Material->MaterialVSize() )
		return;

    UMaterial* PrevMaterial = Material; 

	if( Cast<UTexture>(Material) )
	{
		DECLARE_STATIC_UOBJECT( UFinalBlend, FinalBlend, {} );

        static BYTE LastStyle = STY_None;
        static UMaterial *LastMaterial = NULL;

        if( (Style != LastStyle) || (Material != LastMaterial) )
        {
            pCanvasUtil->Flush();
            LastStyle = Style;
            LastMaterial = Material;
        }

		FinalBlend->Material = Material;
		FinalBlend->TwoSided = 0;
		FinalBlend->ZWrite = 0;
		FinalBlend->ZTest = 0;

		switch( Style )
		{
		case STY_Masked:
			FinalBlend->FrameBufferBlending = FB_Overwrite;
			FinalBlend->AlphaTest = 1;
			FinalBlend->AlphaRef = 127;
			Material = FinalBlend;				
			break;
		case STY_Translucent:
			FinalBlend->FrameBufferBlending = FB_Translucent;
			FinalBlend->AlphaTest = 0;
			Material = FinalBlend;				
			break;
		case STY_Modulated:
			FinalBlend->FrameBufferBlending = FB_Modulate;
			FinalBlend->AlphaTest = 0;
			Material = FinalBlend;				
			break;
		case STY_Alpha:
			FinalBlend->FrameBufferBlending = FB_AlphaBlend;
			FinalBlend->AlphaTest = 1;
			FinalBlend->AlphaRef = 0;
			Material = FinalBlend;				
			break;
		case STY_Additive:
			FinalBlend->FrameBufferBlending = FB_Brighten;
			FinalBlend->AlphaTest = 1;
			FinalBlend->AlphaRef = 0;
			Material = FinalBlend;				
			break;
		case STY_Subtractive:
			FinalBlend->FrameBufferBlending = FB_Darken;
			FinalBlend->AlphaTest = 0;
			Material = FinalBlend;				
			break;
		}

        if( Style == 255 || PrevMaterial->IsA(UCombiner::StaticClass()) || PrevMaterial->IsA(UShader::StaticClass()) || PrevMaterial->IsA(UFinalBlend::StaticClass()) ) 
            Material = PrevMaterial;
	}
	
	pCanvasUtil->DrawTile(X,Y,X + XL,Y + YL,U,V,U + UL,V + VL,0.0f,Material,Color);

	unguard;
}

//
// Draw titling pattern.
//
void UCanvas::DrawPattern
(
	UMaterial*		Material,
	FLOAT			X,
	FLOAT			Y,
	FLOAT			XL,
	FLOAT			YL,
	FLOAT			Scale,
	FLOAT			OrgX,
	FLOAT			OrgY,
	FLOAT			Z,
	FPlane			Color,
	FPlane			Fog
)
{
	guard(UCanvas::DrawPattern);
	DrawTile( Material, X, Y, XL, YL, (X-OrgX)*Scale + Material->MaterialUSize(), (Y-OrgY)*Scale + Material->MaterialVSize(), XL*Scale, YL*Scale, Z, Color, Fog );
	unguard;
}

//
// Draw a scaled sprite.  Takes care of clipping.
// XSize and YSize are in pixels.
//
void UCanvas::DrawIcon
(
	UMaterial*			Material,
	FLOAT				ScreenX, 
	FLOAT				ScreenY, 
	FLOAT				XSize, 
	FLOAT				YSize,
	FLOAT				Z,
	FPlane				Color,
	FPlane				Fog
)
{
	guard(UCanvas::DrawIcon);
	DrawTile( Material, ScreenX, ScreenY, XSize, YSize, 0, 0, Material->MaterialUSize(), Material->MaterialVSize(), Z, Color, Fog );
	unguard;
}

/*-----------------------------------------------------------------------------
	Clip window.
-----------------------------------------------------------------------------*/

void UCanvas::SetClip( INT X, INT Y, INT XL, INT YL )
{
	guard(UCanvas::SetClip);

	CurX  = 0;
	CurY  = 0;
	OrgX  = X;
	OrgY  = Y;
	ClipX = XL;
	ClipY = YL;

	unguard;
}

/*-----------------------------------------------------------------------------
	UCanvas basic text functions.
-----------------------------------------------------------------------------*/

//
// Draw a string of characters.
// - returns pixels drawn
//
static INT DrawString
(
	UCanvas*		Canvas, 
	UFont*			Font, 
    FLOAT           ScaleX, 
    FLOAT           ScaleY, 
	INT				DrawX, 
	INT				DrawY,
	const TCHAR*	Text, 
	FPlane			Color, 
	UBOOL			bClip, 
	UBOOL			bHandleApersand
)
{
	guardSlow(DrawString);

    if ( !Canvas->pCanvasUtil ) 
        return 0;

    int i;

	// Draw all characters in string.
	INT LineX = 0;
	INT bDrawUnderline = 0;
	INT UnderlineWidth = 0;
	for( i=0; Text[i]; i++ )
	{
		INT bUnderlineNext = 0;
		INT Ch = (TCHARU)Font->RemapChar(Text[i]);

		// Handle ampersand underlining.
		if( bHandleApersand )
		{
			if( bDrawUnderline )
				Ch = (TCHARU)Font->RemapChar('_');
			if( Text[i]=='&' )
			{
				if( !Text[i+1] )
					break; 
				if( Text[i+1]!='&' )
				{
					bUnderlineNext = 1;
					Ch = (TCHARU)Font->RemapChar(Text[i+1]);
				}
			}
		}

		// Process character if it's valid.
		if( Ch < Font->Characters.Num() )
		{
			FFontCharacter& Char = Font->Characters(Ch);
		UTexture* Tex;
			if( Char.TextureIndex < Font->Textures.Num() && (Tex=Font->Textures(Char.TextureIndex))!=NULL )
			{
				// Compute character width.
				INT CharWidth;
				if( bDrawUnderline )
					CharWidth = Min(UnderlineWidth, Char.USize);
				else
					CharWidth = Char.USize;

				// Prepare for clipping.
				INT X      = LineX + DrawX;
				INT Y      = DrawY;
				INT CU     = Char.StartU;
				INT CV     = Char.StartV;
				INT CUSize = CharWidth;
				INT CVSize = Char.VSize;

				// Draw if it passes clip test.
				if
				(	(!bClip)
				||	(X+CUSize>0 && X<=Canvas->ClipX && Y+CVSize>0 && Y<=Canvas->ClipY) )
				{
					if( bClip )
					{
						if( X        < 0.f           ) { CU-=X; CUSize+=X; X=0;  }
						if( Y        < 0.f           ) { CV-=Y; CVSize+=Y; Y=0;  }
						if( X+CUSize > Canvas->ClipX ) { CUSize=(INT) (Canvas->ClipX-X); }
						if( Y+CVSize > Canvas->ClipY ) { CVSize=(INT) (Canvas->ClipY-Y); } 
					}

					FLOAT		TileX = Canvas->OrgX + X,
								TileY = Canvas->OrgY + Y;

                    Canvas->DrawTile( Tex, appRound(TileX), appRound(TileY), appRound(CUSize * ScaleX), appRound(CVSize * ScaleY),
                                      appRound(CU), appRound(CV), appRound(CUSize), appRound(CVSize), 0.0F, Color, FPlane(0,0,0,0) );
				}
				// Update underline status.
				if( bDrawUnderline )
					CharWidth = UnderlineWidth;

				if( !bUnderlineNext )
					LineX += (INT)((FLOAT)(CharWidth + Canvas->SpaceX + Font->Kerning) * ScaleX); 
				else
					UnderlineWidth = Char.USize;

				bDrawUnderline = bUnderlineNext;
			}
		}
	}

	return LineX;

	unguardSlow;
}

//
// Get a character's dimensions.
//
static inline void GetCharSize( UFont* Font, TCHAR InCh, INT& Width, INT& Height )
{
	guardSlow(GetCharSize);
	Width = 0;
	Height = 0;
	INT Ch    = (TCHARU)Font->RemapChar(InCh);

	if( Ch < Font->Characters.Num() )
	{
		FFontCharacter& Char = Font->Characters(Ch);
		Width = Char.USize;
		Height = Char.VSize;
	}
	unguardSlow;
}

// Notice: This will actually append to the array for continous wrapping, does not support tabs yet
void UCanvas::WrapStringToArray( const TCHAR *Text, TArray<FString> *OutArray, float Width, UFont *pFont, TCHAR EOL)
{
	check(Text != NULL);
	check(OutArray != NULL);

	const TCHAR *LastText;

	if (*Text == '\0')
		return;

	if (pFont == NULL)
		pFont = this->Font;

	TCHAR CurrColour[5] = { 0, 0, 0, 0, 0 }; // default to black
	UBOOL UseColour = 0;

	do
	{
		while (*Text == EOL)
		{
		INT iAdded = OutArray->AddZeroed();

			(*OutArray)(iAdded) = FString(TEXT(""));
			Text++;
		}
		if (*Text==0)
			break;

		LastText = Text;

		FString ColourStart;
		if(UseColour)
			ColourStart = FString( CurrColour );
		else
			ColourStart = FString(TEXT(""));

		INT iCleanWordEnd=0, iTestWord;
		INT TestXL=0, CleanXL=0, Skip=0;
		UBOOL GotWord=0;
		for( iTestWord=0; Text[iTestWord]!=0 && Text[iTestWord]!=EOL; )
		{
			INT ChW, ChH;
			
			if ( int(Text[iTestWord])==27 )		// Skip over color codes
				Skip=4;

			if (!Skip)
			{
			GetCharSize(pFont, Text[iTestWord], ChW, ChH);
				TestXL              += (INT) ((FLOAT)(ChW + SpaceX + pFont->Kerning)); // gam
		
			if( TestXL>Width )
				break;
			}
			else
			{
				// Read colour code.
				CurrColour[4-Skip] = Text[iTestWord];
				UseColour = 1;

				Skip--;
			}


			iTestWord++;
			UBOOL WordBreak = Text[iTestWord]==' ' || Text[iTestWord]==EOL || Text[iTestWord]==0;
			if( WordBreak || !GotWord )
			{
				iCleanWordEnd = iTestWord;
				CleanXL       = TestXL;
				GotWord       = GotWord || WordBreak;
			}
		}

		if( iCleanWordEnd==0 )
		{
			if (*Text != 0)
			{
			INT iAdded = OutArray->AddZeroed();

				if (Text == LastText)
				{
					(*OutArray)(iAdded) = ColourStart + FString::Printf(TEXT("%c"), Text);
					Text++;
				}
				else
					(*OutArray)(iAdded) = FString(TEXT(""));
			}
		}
		else
		{
		FString TextLine(Text);
		INT iAdded = OutArray->AddZeroed();
			(*OutArray)(iAdded) = ColourStart + TextLine.Left(iCleanWordEnd);
		}
		Text += iCleanWordEnd;

		// Skip whitespace after word wrap.
		while( *Text==' ' )
			Text++;

		if (*Text == EOL)
			Text++;
	}
	while( *Text );
}

void UCanvas::execWrapStringToArray( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execWrapStringToArray);
	P_GET_STR(InText);
	P_GET_TARRAY_REF(OutArray, FString);
	P_GET_FLOAT(Width);
	P_GET_STR_OPTX(EOL, TEXT("|"));
	P_FINISH;

	WrapStringToArray(*InText, OutArray, Width, NULL, EOL[0]);

	unguard;
}
IMPLEMENT_FUNCTION( UCanvas, -1, execWrapStringToArray );

//
// Compute size and optionally print text with word wrap.
//!!For the next generation, redesign to ignore CurX,CurY.
//
void UCanvas::WrappedPrint( ERenderStyle Style, INT& XL, INT& YL, UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center, const TCHAR* Text ) 
{
	guard(UCanvas::WrappedPrint);

	if( ClipX<0 || ClipY<0 )
		return;
	check(Font);
	FPlane DrawColor = Color.Plane();

	// Process each word until the current line overflows.
	XL = YL = 0;
	do
	{
		INT iCleanWordEnd=0, iTestWord;
		INT TestXL=(INT)CurX, CleanXL=0;
		INT TestYL=0,    CleanYL=0;
		UBOOL GotWord=0;
		for( iTestWord=0; Text[iTestWord]!=0 && Text[iTestWord]!='\n'; )
		{
			INT ChW, ChH;
			GetCharSize(Font, Text[iTestWord], ChW, ChH);
			TestXL              += (INT) ((FLOAT)(ChW + SpaceX + Font->Kerning) * ScaleX); 
			TestYL               = (INT) ((FLOAT)(Max( TestYL, ChH + (INT)SpaceY)) * ScaleY); 
			if( TestXL>ClipX )
				break;
			iTestWord++;
			UBOOL WordBreak = Text[iTestWord]==' ' || Text[iTestWord]=='\n' || Text[iTestWord]==0;
			if( WordBreak || !GotWord )
			{
				iCleanWordEnd = iTestWord;
				CleanXL       = TestXL;
				CleanYL       = TestYL;
				GotWord       = GotWord || WordBreak;
			}
		}
		if( iCleanWordEnd==0 )
			break;

		// Sucessfully split this line, now draw it.
		if( Style!=STY_None && OrgY+CurY<Viewport->SizeY && OrgY+CurY+CleanYL>0 )
		{
			FString TextLine(Text);
			INT LineX = Center ? (INT) (CurX+(ClipX-CleanXL)/2) : (INT) (CurX);
			LineX += DrawString( this, Font, ScaleX, ScaleY, LineX, (INT) CurY, *(TextLine.Left(iCleanWordEnd)), DrawColor, 0, 0 );
			CurX = LineX;
		}

		// Update position.
		CurX  = 0;
		CurY += CleanYL;
		YL   += CleanYL;
		XL    = Max(XL,CleanXL);
		Text += iCleanWordEnd;

		// Skip whitespace after word wrap.
		while( *Text==' ' )
			Text++;
	}
	while( *Text );

	unguardf(( TEXT("(%s)"), Text ));
}

/*-----------------------------------------------------------------------------
	UCanvas derived text functions.
-----------------------------------------------------------------------------*/

//
// Calculate the size of a string built from a font, word wrapped
// to a specified region.
//
void VARARGS UCanvas::WrappedStrLenf( UFont* Font, INT& XL, INT& YL, const TCHAR* Fmt, ... )
{
	TCHAR Text[4096];
	GET_VARARGS( Text, ARRAY_COUNT(Text), Fmt, Fmt );

	guard(UCanvas::WrappedStrLenf);
	WrappedPrint( STY_None, XL, YL, Font, 1.0F, 1.0F, 0, Text ); 
	unguard;
}

//
// Wrapped printf.
//
void VARARGS UCanvas::WrappedPrintf( UFont* Font, UBOOL Center, const TCHAR* Fmt, ... )
{
	TCHAR Text[4096];
	GET_VARARGS( Text, ARRAY_COUNT(Text), Fmt, Fmt );

	guard(UCanvas::WrappedPrintf);
	INT XL=0, YL=0;
	WrappedPrint( STY_Normal, XL, YL, Font, 1.0F, 1.0F, Center, Text ); 
	unguard;
}

//
// Calculate the size of a string built from a font, word wrapped
// to a specified region.
//
void VARARGS UCanvas::WrappedStrLenf( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL, const TCHAR* Fmt, ... ) 
{
	TCHAR Text[4096];
	GET_VARARGS( Text, ARRAY_COUNT(Text), Fmt, Fmt );

	guard(UCanvas::WrappedStrLenf);
	WrappedPrint( STY_None, XL, YL, Font, ScaleX, ScaleY, 0, Text ); 
	unguard;
}

//
// Wrapped printf.
//
void VARARGS UCanvas::WrappedPrintf( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center, const TCHAR* Fmt, ... ) 
{
	TCHAR Text[4096];
	GET_VARARGS( Text, ARRAY_COUNT(Text), Fmt, Fmt );

	guard(UCanvas::WrappedPrintf);
	INT XL=0, YL=0;
	WrappedPrint( STY_Normal, XL, YL, Font, ScaleX, ScaleY, Center, Text ); 
	unguard;
}

void StaticClippedStrLen( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, FLOAT SpaceX, INT& XL, INT& YL, const TCHAR* Text )
{
	XL = 0;
	YL = 0;

	if (Font == NULL)
		return;

	for( INT i=0; Text[i]; i++)
	{
    	INT W, H;

		GetCharSize( Font, Text[i], W, H );

		if( Text[i + 1] )
	        W = (INT)((FLOAT)(W + SpaceX + Font->Kerning) * ScaleX);
		else
			W = (INT)((FLOAT)(W) * ScaleX);

        H = (INT)((FLOAT)(H) * ScaleY);
		
		XL += W;
		if(YL < H)
			YL = H;	
	}
}

void UCanvas::ClippedStrLen( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL, const TCHAR* Text )
{
	if (Font == NULL)
		Font = this->Font;

	StaticClippedStrLen( Font, ScaleX, ScaleY, SpaceX, XL, YL, Text );
}

void UCanvas::ClippedPrint( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center, const TCHAR* Text )
{
	FPlane DrawColor = Color.Plane();
	DrawString( this, Font, ScaleX, ScaleY, (INT) CurX, (INT) CurY, Text, DrawColor, 1, 0 );
}

void UCanvas::TextSize( UFont* Font, const TCHAR* Text, FLOAT &XL, FLOAT &YL )
{
	INT XLi, YLi;
	StaticClippedStrLen( Font, 1.f, 1.f, 0.f, XLi, YLi, Text );
	XL = XLi;
	YL = YLi;
}

void UConsole::TextSize( UFont* Font, const TCHAR* Text, FLOAT &XL, FLOAT &YL )
{
	INT XLi, YLi;
	StaticClippedStrLen( Font, 1.f, 1.f, 0.f, XLi, YLi, Text );
	XL = XLi;
	YL = YLi;
}

/*-----------------------------------------------------------------------------
	UCanvas object functions.
-----------------------------------------------------------------------------*/

void UCanvas::Init( UViewport* InViewport )
{
	guard(UCanvas::UCanvas);
	Viewport = InViewport;

	// Load default fonts
	if( !TinyFont )
		TinyFont	= Cast<UFont>( StaticLoadObject( UFont::StaticClass(), NULL, *TinyFontName, NULL, LOAD_NoWarn, NULL ) );
	if( !TinyFont )
	{
		GWarn->Logf(TEXT("Could not load stock Tiny font %s"), *TinyFontName);
		TinyFont=FindObjectChecked<UFont>(ANY_PACKAGE,TEXT("DefaultFont"));
	}
	if( !SmallFont )
		SmallFont	= Cast<UFont>( StaticLoadObject( UFont::StaticClass(), NULL, *SmallFontName, NULL, LOAD_NoWarn, NULL ) );
	if( !SmallFont )
	{
		GWarn->Logf(TEXT("Could not load stock Small font %s"), *SmallFontName);
		SmallFont=FindObjectChecked<UFont>(ANY_PACKAGE,TEXT("DefaultFont"));
	}
	if( !MedFont )
		MedFont		= Cast<UFont>( StaticLoadObject( UFont::StaticClass(), NULL, *MedFontName, NULL, LOAD_NoWarn, NULL ) );
	if( !MedFont )
	{
		GWarn->Logf(TEXT("Could not load stock Medium font %s"), *MedFontName);
		MedFont=FindObjectChecked<UFont>(ANY_PACKAGE,TEXT("DefaultFont"));
	}
	unguard;
}
void UCanvas::Update()
{
	guard(UCanvas::Update);

	// Call UnrealScript to reset.
	eventReset();

	// Copy size parameters from viewport.
	ClipX = Viewport->SizeX;
	SizeX = (INT) ClipX;
	ClipY = Viewport->SizeY;
	SizeY = (INT) ClipY;

    if ( !pCanvasUtil )
        pCanvasUtil = new FCanvasUtil(&Viewport->RenderTarget,Viewport->RI);

	if( GIsOpenGL )
		pCanvasUtil->CanvasToScreen = FTranslationMatrix(FVector(-Viewport->SizeX / 2.0f,-Viewport->SizeY / 2.0f,0.0f)) * FScaleMatrix(FVector(2.0f / Viewport->SizeX,-2.0f / Viewport->SizeY,1.0f));
	else
		pCanvasUtil->CanvasToScreen = FTranslationMatrix(FVector(-Viewport->SizeX / 2.0f - 0.5f,-Viewport->SizeY / 2.0f - 0.5f,0.0f)) * FScaleMatrix(FVector(2.0f / Viewport->SizeX,-2.0f / Viewport->SizeY,1.0f));
	pCanvasUtil->ScreenToCanvas = pCanvasUtil->CanvasToScreen.Inverse();
    pCanvasUtil->RI = Viewport->RI;

	unguard;
}

/*-----------------------------------------------------------------------------
	UCanvas natives.
-----------------------------------------------------------------------------*/

void UCanvas::execStrLen( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execStrLen);

	P_GET_STR(InText);
	P_GET_FLOAT_REF(XL);
	P_GET_FLOAT_REF(YL);
	P_FINISH;

	INT XLi, YLi;
	INT OldCurX, OldCurY;
	OldCurX = (INT) CurX;
	OldCurY = (INT) CurY;
	CurX = 0;
	CurY = 0;
	WrappedStrLenf( Font, 1.f, 1.f, XLi, YLi, TEXT("%s"), *InText ); 
	CurY = OldCurY;
	CurX = OldCurX;
	*XL = XLi;
	*YL = YLi;

	unguard;
}
IMPLEMENT_FUNCTION( UCanvas, 464, execStrLen );

void UCanvas::execDrawText( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawText);
	P_GET_STR(InText);
	P_GET_UBOOL_OPTX(CR,1);
	P_FINISH;

	if( Style == STY_None ) 
        return;

	if( !Font )
	{
		Stack.Logf( NAME_Warning, TEXT("DrawText: No font") ); 
		return;
	}
	INT XL=0, YL=0;
//	unclock(GScriptCycles);
//	clock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
	if( Style!=STY_None )
		WrappedPrint( (ERenderStyle)Style, XL, YL, Font, 1.f, 1.f, bCenter, *InText ); 
    
//    unclock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
//	clock(GScriptCycles);
	CurX += XL;
	CurYL = Max(CurYL,(FLOAT)YL);
	if( CR )
	{
		CurX  = 0;
		CurY += CurYL;
		CurYL = 0;
	}

	unguardexec;
}
IMPLEMENT_FUNCTION( UCanvas, 465, execDrawText );

void UCanvas::execDrawTile( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawTile);
	P_GET_OBJECT(UMaterial,Mat);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_FINISH;
	if( !Mat )
	{
		Stack.Logf( TEXT("DrawTile: Missing Material") );
		return;
	}
//	unclock(GScriptCycles);
//	clock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
	if( Style!=STY_None ) DrawTile
	(
		Mat,
		OrgX+CurX,
		OrgY+CurY,
		XL,
		YL,
		U,
		V,
		UL,
		VL,
		Z,
		Color.Plane(),
		FPlane(0,0,0,0)
	);
//	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
//	clock(GScriptCycles);
	CurX += XL + SpaceX;
	CurYL = Max(CurYL,YL);
	unguardexec;
}
IMPLEMENT_FUNCTION( UCanvas, 466, execDrawTile );

void UCanvas::DrawActor( AActor* Actor, UBOOL WireFrame, UBOOL ClearZ, FLOAT DisplayFOV )
{
    guard(UCanvas::DrawActor)

    if ( pCanvasUtil ) 
        pCanvasUtil->Flush();

    if ( !Actor ) 
		return;
	AActor*		CameraActor = Viewport->Actor;
	FVector		CameraLocation = Viewport->Actor->Location;
	FRotator	CameraRotation = Viewport->Actor->Rotation;

	Viewport->Actor->eventPlayerCalcView(CameraActor,CameraLocation,CameraRotation);

//	unclock(GScriptCycles);
//	clock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
	INT OldRendMap;
	OldRendMap = Viewport->Actor->RendMap;
	if( WireFrame )
		Viewport->Actor->RendMap = REN_Wire;
	if( ClearZ )
		Viewport->RI->Clear(0,FColor(0,0,0),1,1.0f,0);
    FActorSceneNode(Viewport,&Viewport->RenderTarget,Actor,CameraActor,CameraLocation,CameraRotation,DisplayFOV).Render(Viewport->RI);
	Viewport->Actor->RendMap = OldRendMap;
//	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
//	clock(GScriptCycles);

    unguard;
}

void UCanvas::execDrawActor( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawActor);

	P_GET_OBJECT(AActor, Actor);
	P_GET_UBOOL(WireFrame);
	P_GET_UBOOL_OPTX(ClearZ, 0);
	P_GET_FLOAT_OPTX(DisplayFOV,Viewport->Actor->FovAngle);
	P_FINISH;

    DrawActor( Actor, WireFrame, ClearZ, DisplayFOV ); 

	unguardexec;
}
IMPLEMENT_FUNCTION( UCanvas, 467, execDrawActor );

void UCanvas::execDrawTileClipped( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawTileClipped);
	P_GET_OBJECT(UMaterial,Mat);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_FINISH;

	if( !Mat )
	{
		Stack.Logf( TEXT("DrawTileClipped: Missing Material") );
		return;
	}


	// Clip to ClipX and ClipY
	if( XL > 0 && YL > 0 )
	{		
		if( CurX<0 )
			{FLOAT C=CurX*UL/XL; U-=C; UL+=C; XL+=CurX; CurX=0;}
		if( CurY<0 )
			{FLOAT C=CurY*VL/YL; V-=C; VL+=C; YL+=CurY; CurY=0;}
		if( XL>ClipX-CurX )
			{UL+=(ClipX-CurX-XL)*UL/XL; XL=ClipX-CurX;}
		if( YL>ClipY-CurY )
			{VL+=(ClipY-CurY-YL)*VL/YL; YL=ClipY-CurY;}
	
//		unclock(GScriptCycles);
//		clock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
		if( Style!=STY_None ) 
			DrawTile
			(
				Mat,
				OrgX+CurX,
				OrgY+CurY,
				XL,
				YL,
				U,
				V,
				UL,
				VL,
				Z,
				Color.Plane(),
				FPlane(0,0,0,0)
			);
//		unclock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
//		clock(GScriptCycles);

		CurX += XL + SpaceX;
		CurYL = Max(CurYL,YL);
	}

	unguardexec;
}
IMPLEMENT_FUNCTION( UCanvas, 468, execDrawTileClipped );

void UCanvas::execDrawTextClipped( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawTextClipped);
	P_GET_STR(InText);
	P_GET_UBOOL_OPTX(CheckHotKey, 0);
	P_FINISH;

	if( !Font )
	{
		Stack.Logf( TEXT("DrawTextClipped: No font") ); 
		return;
	}

	check(Font);

	FPlane DrawColor = Color.Plane();
//	unclock(GScriptCycles);
//	clock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
	DrawString( this, Font, 1.f, 1.f, (INT) CurX, (INT) CurY, *InText, DrawColor, 1, CheckHotKey ); 
//	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
//	clock(GScriptCycles);

	unguardexec;
}
IMPLEMENT_FUNCTION( UCanvas, 469, execDrawTextClipped );

void UCanvas::execTextSize( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execTextSize);
	P_GET_STR(InText);
	P_GET_FLOAT_REF(XL);
	P_GET_FLOAT_REF(YL);
	P_FINISH;

	INT XLi, YLi;

	if( !Font )
	{
		Stack.Logf( TEXT("TextSize: No font") ); 
		return;
	}

	ClippedStrLen( Font, 1.f, 1.f, XLi, YLi, *InText );
	
	*XL = XLi;
	*YL = YLi;

	unguardexec;
}
IMPLEMENT_FUNCTION( UCanvas, 470, execTextSize );

void UConsole::execTextSize( FFrame& Stack, RESULT_DECL )
{
	guard(UConsole::execTextSize);
	P_GET_OBJECT(UFont,Font);
	P_GET_STR(Text);
	P_GET_FLOAT_REF(XL);
	P_GET_FLOAT_REF(YL);
	P_FINISH;

	if( Font )
		TextSize( Font, *Text, *XL, *YL );
	else
		Stack.Logf( TEXT("TextSize: No font") );

	unguardexec;
}
IMPLEMENT_FUNCTION( UConsole, INDEX_NONE, execTextSize );

// execDrawPortal
// Written by Andrew Scheidecker
// Edited by Brandon Reinhart
void UCanvas::execDrawPortal( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawPortal);
	P_GET_INT(X);
	P_GET_INT(Y);
	P_GET_INT(Width);
	P_GET_INT(Height);
	P_GET_OBJECT(AActor,CamActor);
	P_GET_VECTOR(CamLocation);
	P_GET_ROTATOR(CamRotation);
	P_GET_INT_OPTX(FOV, 90);
	P_GET_UBOOL_OPTX(ClearZ, 1);
	P_FINISH;

	// Save state.

//	unclock(GScriptCycles);
//	clock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
	Viewport->RI->PushState();

	FLOAT	SavedFovAngle = Viewport->Actor->FovAngle;

	// Clear the portal viewport.

	FCanvasUtil(&Viewport->RenderTarget,Viewport->RI).DrawTile(
		X,Y,
		X + Width,Y + Height,
		0,0,
		0,0,
		1,
		NULL,
//!!MAT		PF_NoZTest,
		FColor(0,0,0)
		);

	Viewport->RI->SetViewport(X,Y,Width,Height);

	// Render the scene.

	FCameraSceneNode	SceneNode(Viewport,&Viewport->RenderTarget,CamActor,CamLocation,CamRotation,FOV);

	Viewport->Actor->FovAngle = FOV;

	SceneNode.Render(Viewport->RI);

	// Restore state.

	Viewport->Actor->FovAngle = SavedFovAngle;
	Viewport->RI->PopState();
//	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_CanvasCycles));
//	clock(GScriptCycles);

	unguardexec;
}
IMPLEMENT_FUNCTION( UCanvas, 480, execDrawPortal );

void UCanvas::execDrawTileStretched( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawTileStretched);

	P_GET_OBJECT(UMaterial,Mat);
	P_GET_FLOAT(AWidth);
	P_GET_FLOAT(AHeight);
	P_FINISH;

	DrawTileStretched(Mat,CurX, CurY, AWidth, AHeight);

	unguard;

}
IMPLEMENT_FUNCTION( UCanvas, -1, execDrawTileStretched );

void UCanvas::DrawTileStretched(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT AWidth, FLOAT AHeight)
{

	guard(UCanvas::DrawTileStretched);

	CurX = Left;
	CurY = Top;

	if( !Mat )
	{
		debugf( TEXT("DrawTile: Missing Material") );
		return;
	}
	
	if (Style==NULL)
		return;

	// Get the size of the image

	FLOAT mW = Mat->MaterialUSize();
	FLOAT mH = Mat->MaterialVSize();

	// Get the midpoints of the image

	FLOAT MidX = appFloor(mW/2);
	FLOAT MidY = appFloor(mH/2);

	// Grab info about the scaled image

	FLOAT SmallTileW = AWidth - mW;
	FLOAT SmallTileH = AHeight - mH;
	FLOAT fX, fY;		// Used to shrink

	// Draw the spans first

		// Top and Bottom

	if (mW<AWidth)
	{
		fX = MidX;

		if (mH>AHeight)
			fY = AHeight/2;
		else
			fY = MidY;

		DrawTile(Mat, CurX+fX,	CurY,					SmallTileW,		fY,		MidX,	0,			1,		fY,	0, Color, FPlane(0,0,0,0));
		DrawTile(Mat, CurX+fX,	CurY+AHeight-fY,		SmallTileW,		fY,		MidX,	mH-fY,		1,		fY,	0, Color, FPlane(0,0,0,0));
	}
	else
		fX = AWidth / 2;

		// Left and Right

	if (mH<AHeight)
	{

		fY = MidY;

		DrawTile(Mat, CurX,				CurY+fY,	fX,	SmallTileH,		0,		fY, fX,	1,	0,	Color,	FPlane(0,0,0,0));
		DrawTile(Mat, CurX+AWidth-fX,	CurY+fY,	fX,	SmallTileH,		mW-fX,	fY, fX,	1,	0,	Color,	FPlane(0,0,0,0));

	}
	else
		fY = AHeight / 2; 

		// Center

	if ( (mH<AHeight) && (mW<AWidth) )
		DrawTile(Mat, CurX+fX, CurY+fY, SmallTileW, SmallTileH, fX, fY, 1, 1, 0, Color, FPlane(0,0,0,0));

	// Draw the 4 corners.

	DrawTile(Mat, CurX,				CurY,				fX, fY, 0,		0,		fX, fY, 0,Color,FPlane(0,0,0,0));
	DrawTile(Mat, CurX+AWidth-fX,	CurY,				fX, fY,	mW-fX,	0,		fX, fY, 0,Color,FPlane(0,0,0,0));
	DrawTile(Mat, CurX,				CurY+AHeight-fY,	fX, fY, 0,		mH-fY,	fX, fY, 0,Color,FPlane(0,0,0,0));
	DrawTile(Mat, CurX+AWidth-fX,	CurY+AHeight-fY,	fX, fY,	mW-fX,	mH-fY,	fX, fY, 0,Color,FPlane(0,0,0,0));

	unguard;
}
void UCanvas::execDrawTileScaled( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawTileStretched);

	P_GET_OBJECT(UMaterial,Mat);
	P_GET_FLOAT(NewXScale);
	P_GET_FLOAT(NewYScale);
	P_FINISH;

	DrawTileScaled(Mat,CurX,CurY,NewXScale,NewYScale);

	unguard;

}

IMPLEMENT_FUNCTION( UCanvas, -1, execDrawTileScaled );


void UCanvas::DrawTileBound(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT Width, FLOAT Height)
{
	guard(UCanvas::DrawTileBound);

	if (!Mat)
		debugf(TEXT("DrawTileBound: Missing Material"));

	if (Style==NULL)
		return;

	DrawTile( Mat, Left, Top, Width, Height, 0, 0, Width, Height, 0, Color, FPlane(0,0,0,0));

	unguard;
}

void UCanvas::DrawTileScaleBound(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT Width, FLOAT Height)
{
	guard(UCanvas::DrawTileScaleBound);

	if (!Mat)
		debugf(TEXT("DrawTileScaleBound: Missing Material"));

	if (Style==NULL)
		return;

	FLOAT mW = Mat->MaterialUSize();
	FLOAT mH = Mat->MaterialVSize();

	DrawTile( Mat, Left, Top, Width, Height, 0, 0, mW, mH, 0, Color, FPlane(0,0,0,0));

	unguard;
}

IMPLEMENT_FUNCTION( UCanvas, -1, execDrawTileJustified );

void UCanvas::execDrawTileJustified( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawTileJustified);
	P_GET_OBJECT(UMaterial,Mat);
	P_GET_BYTE(Just);
	P_GET_FLOAT(AWidth);
	P_GET_FLOAT(AHeight);
	P_FINISH;

	DrawTileJustified(Mat, CurX, CurY, AWidth, AHeight, Just);

	unguard;
}


// Justification is: 0 = left/top, 1 = Center, 2 = bottom/right
void UCanvas::DrawTileJustified(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT Width, FLOAT Height, BYTE Justification)
{
	guard(UCanvas::DrawTileScaleBound);

	if (!Mat)
		debugf(TEXT("DrawTileScaleBound: Missing Material"));

	if (Style==NULL)
		return;

	FLOAT mW = Mat->MaterialUSize();
	FLOAT mH = Mat->MaterialVSize();

	if (mW <= 0.0 || mH <= 0.0)
		return;

	// Find scaling proportions
	FLOAT MatProps = mH/mW;
	FLOAT BndProps = Height/Width;

	if (MatProps == BndProps)
		DrawTile( Mat, Left, Top, Width, Height, 0, 0, mW, mH, 0, Color, FPlane(0,0,0,0));
	else if (MatProps > BndProps)	// Stretch to fit Height
	{
	FLOAT NewWidth = Width * BndProps / MatProps;
	FLOAT NewLeft = Left;

		if (Justification == 1)			// Centered
			NewLeft += ((Width - NewWidth) / 2.0);
		else if (Justification == 2)	// RightBottom
			NewLeft += Width - NewWidth;

		DrawTile( Mat, NewLeft, Top, NewWidth, Height, 0, 0, mW, mH, 0, Color, FPlane(0,0,0,0));
	}
	else							// Stretch to fit Width
	{
	FLOAT NewHeight = Height * MatProps / BndProps;
	FLOAT NewTop = Top;

		if (Justification == 1)			// Centered
			NewTop += ((Height - NewHeight) / 2.0);
		else if (Justification == 2)	// RightBottom
			NewTop += Height - NewHeight;

		DrawTile( Mat, Left, NewTop, Width, NewHeight, 0, 0, mW, mH, 0, Color, FPlane(0,0,0,0));
	}
	unguard;
}

void UCanvas::DrawTileScaled(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT NewXScale, FLOAT NewYScale)
{
	guard(UCanvas::DrawTileScaled);

	if( !Mat )
	{
		debugf( TEXT("DrawTile: Missing Material") );
		return;
	}
	
	if (Style==NULL)
		return;

	FLOAT mW = Mat->MaterialUSize();
	FLOAT mH = Mat->MaterialVSize();

	DrawTile( Mat, Left, Top, mW*NewXScale, mH*NewYScale, 0, 0, mW, mH, 0, Color, FPlane(0,0,0,0));

	unguard;
}

IMPLEMENT_FUNCTION( UCanvas, -1, execDrawTextJustified );

void UCanvas::execDrawTextJustified( FFrame& Stack, RESULT_DECL )
{
	guard(UCanvas::execDrawTextJustified);
	P_GET_STR(InText);
	P_GET_BYTE(Just);
	P_GET_FLOAT(X1);
	P_GET_FLOAT(Y1);
	P_GET_FLOAT(X2);
	P_GET_FLOAT(Y2);
	P_FINISH;

	DrawTextJustified(Just,X1,Y1,X2,Y2, TEXT("%s"),*InText);

	unguard;
}


void VARARGS UCanvas::DrawTextJustified(BYTE Justification, FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, const TCHAR* Fmt, ... )
{
	guard(UCanvas::DrawTextJustified);
	
	TCHAR Text[4096];
	GET_VARARGS( Text, ARRAY_COUNT(Text), Fmt, Fmt );

	INT XL,YL;
	CurX = 0;
	CurY = 0;
	ClippedStrLen(  Font, 1.0f,1.0f, XL, YL, Text );

	// Adjust the Y so the Font is centered in the bounding box
	CurY = ((y2-y1) / 2) - (YL/2);

	if (Justification == 0)						// Left
		CurX = 0;
	else if (Justification == 1)				// Center
	{
		if( XL > x2-x1 ) // align left when there's no room
			CurX = 0;
		else
			CurX = ((x2-x1) / 2) - (XL/2);
	}
	else if (Justification == 2)				// Right
		CurX = (x2-x1) - XL;

	INT OldClipX = ClipX;
	INT OldClipY = ClipY;
	INT OldOrgX = OrgX;
	INT OldOrgY = OrgY;

	// Clip to box
	OrgX = x1;
	OrgY = y1;
	ClipX = x2-x1;
	ClipY = y2-y1;

	ClippedPrint(Font, 1.0f, 1.0f, false, Text);

	ClipX = OldClipX;
	ClipY = OldClipY;
	OrgX = OldOrgX;
	OrgY = OldOrgY;


	unguard;

}

// Converts a world location in to Screen X/Y coordinates
// NOTE: code taken from UIteraction::execWorldToScreen
void UCanvas::execProject(FFrame& Stack,RESULT_DECL)
{
	guard(UCanvas::execProject);
	P_GET_VECTOR(V);

	//NOTE: can we get the viewport from Engine->Client->Viewports array?
	if (!Viewport)
	{
		*(FVector*)Result = FVector();
		debugf(TEXT("UCanvas::execProject: Unable to get viewport"));
		return;
	}

	APlayerController*	CameraActor = Viewport->Actor;

	P_GET_VECTOR_OPTX(CameraLocation, CameraActor->Location);	// CameraLocation and CameraRotation can be used
	P_GET_ROTATOR_OPTX(CameraRotation, CameraActor->Rotation);	// to project to any viewpoint
	P_FINISH;

	AActor*		ViewActor = NULL;

	CameraActor->eventPlayerCalcView(ViewActor,CameraLocation,CameraRotation);

	FCameraSceneNode	SceneNode(Viewport, &Viewport->RenderTarget, ViewActor, CameraLocation, CameraRotation,CameraActor->FovAngle);
	FCanvasUtil			CanvasUtil(&Viewport->RenderTarget, Viewport->RI);

	*(FVector*)Result = CanvasUtil.ScreenToCanvas.TransformFVector(SceneNode.Project(V));

	unguard;
}
IMPLEMENT_FUNCTION( UCanvas, INDEX_NONE, execProject );

// Translate X/Y screen coordinates in to a world vector
// NOTE: code taken from UIteraction::execWorldToScreen
void UCanvas::execDeProject(FFrame& Stack,RESULT_DECL)
{
	guard(UCanvas::execDeProject);

	P_GET_VECTOR(V);

	//NOTE: can we get the viewport from Engine->Client->Viewports array?
	if (!Viewport)
	{
		*(FVector*)Result = FVector();
		debugf(TEXT("UCanvas::execProject: Unable to get viewport"));
		return;
	}


	APlayerController*	CameraActor = Viewport->Actor;


	P_GET_VECTOR_OPTX(CameraLocation, CameraActor->Location);	// CameraLocation and CameraRotation can be used
	P_GET_ROTATOR_OPTX(CameraRotation, CameraActor->Rotation);	// to deproject from any viewpoint
	P_FINISH;

	AActor*				ViewActor = NULL;

	CameraActor->eventPlayerCalcView(ViewActor,CameraLocation,CameraRotation);

	FCameraSceneNode	SceneNode(Viewport, &Viewport->RenderTarget, ViewActor, CameraLocation, CameraRotation,CameraActor->FovAngle);
	FCanvasUtil			CanvasUtil(&Viewport->RenderTarget, Viewport->RI);

	FPlane	P(CanvasUtil.CanvasToScreen.TransformFVector(V),NEAR_CLIPPING_PLANE);
	FVector Direction = SceneNode.Deproject(P);
	Direction = Direction - CameraLocation;
	Direction.SafeNormal();
	*(FVector*)Result = Direction;

	unguard;
}
IMPLEMENT_FUNCTION( UCanvas, INDEX_NONE, execDeProject );


IMPLEMENT_CLASS(UCanvas);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

