/*=============================================================================
	UnCanvas.h: Unreal canvas definition.
	Copyright 2001 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
=============================================================================*/

/*
	UCanvas
	A high-level rendering interface used to render objects on the HUD.
*/

class ENGINE_API UCanvas : public UObject
{
	DECLARE_CLASS(UCanvas,UObject,CLASS_Transient,Engine);
	NO_DEFAULT_CONSTRUCTOR(UCanvas);
public:

	// Variables.
	UFont*			Font;
	FLOAT			SpaceX, SpaceY;
	FLOAT			OrgX, OrgY;
	FLOAT			ClipX, ClipY;
	FLOAT			CurX, CurY;
	FLOAT			Z;
	BYTE			Style;
	FLOAT			CurYL;
	FColor			Color;
	BITFIELD		bCenter:1;
	BITFIELD		bNoSmooth:1;
	INT				SizeX, SizeY;
	UFont			*TinyFont, *SmallFont, *MedFont;
	UFont			*DebugFont;	//NEW (mdf) console debugging font
	FStringNoInit	TinyFontName, SmallFontName, MedFontName;
	UViewport*		Viewport;
    FCanvasUtil*    pCanvasUtil; // shared canvas util for batching successive canvas rendering funcs

	// UCanvas interface.
	virtual void Init( UViewport* InViewport );
	virtual void Update();
	virtual void DrawTile( UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FLOAT Z, FPlane Color, FPlane Fog );
	virtual void DrawIcon( UMaterial* Material, FLOAT ScreenX, FLOAT ScreenY, FLOAT XSize, FLOAT YSize, FLOAT Z, FPlane Color, FPlane Fog );
	virtual void DrawPattern( UMaterial* Material, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT Scale, FLOAT OrgX, FLOAT OrgY, FLOAT Z, FPlane Color, FPlane Fog );
	virtual void VARARGS WrappedStrLenf( UFont* Font, INT& XL, INT& YL, const TCHAR* Fmt, ... );
	virtual void VARARGS WrappedPrintf( UFont* Font, UBOOL Center, const TCHAR* Fmt, ... );

	virtual void VARARGS WrappedStrLenf( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL, const TCHAR* Fmt, ... );
	virtual void VARARGS WrappedPrintf( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center, const TCHAR* Fmt, ... );

	virtual void WrapStringToArray( const TCHAR* Text, TArray<FString> *OutArray, float Width, UFont *Font = NULL, const TCHAR EOL='\n' );
	virtual void ClippedStrLen( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, INT& XL, INT& YL, const TCHAR* Text );
	virtual void ClippedPrint( UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center, const TCHAR* Text );

	void DrawActor( AActor* Actor, UBOOL WireFrame, UBOOL ClearZ, FLOAT DisplayFOV );

	void virtual DrawTileStretched(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT AWidth, FLOAT AHeight);
	void virtual DrawTileScaled(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT NewXScale, FLOAT NewYScale);
	void virtual DrawTileBound(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT Width, FLOAT Height);
	void virtual DrawTileJustified(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT Width, FLOAT Height, BYTE Justification);
	void virtual DrawTileScaleBound(UMaterial* Mat, FLOAT Left, FLOAT Top, FLOAT Width, FLOAT Height);
	void virtual VARARGS DrawTextJustified(BYTE Justification, FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2, const TCHAR* Fmt, ... );


	virtual void SetClip( INT X, INT Y, INT XL, INT YL );

	void TextSize( UFont* Font, const TCHAR* Text, FLOAT &XL, FLOAT &YL );

	// Natives.
	DECLARE_FUNCTION(execStrLen)
	DECLARE_FUNCTION(execDrawText)
	DECLARE_FUNCTION(execDrawTile)
	DECLARE_FUNCTION(execDrawActor)
	DECLARE_FUNCTION(execDrawTileClipped)
	DECLARE_FUNCTION(execDrawTextClipped)
	DECLARE_FUNCTION(execTextSize)
	DECLARE_FUNCTION(execDrawPortal)
	DECLARE_FUNCTION(execWrapStringToArray) // mc

	// jmw

	DECLARE_FUNCTION(execDrawTileStretched);
	DECLARE_FUNCTION(execDrawTileJustified);
	DECLARE_FUNCTION(execDrawTileScaled);
	DECLARE_FUNCTION(execDrawTextJustified);

	// mb

	DECLARE_FUNCTION(execProject);
	DECLARE_FUNCTION(execDeProject);

    void eventReset()
    {
        ProcessEvent(FindFunctionChecked(TEXT("Reset")),NULL);
    }

private:
	// Internal functions.
	void WrappedPrint( ERenderStyle Style, INT& XL, INT& YL, UFont* Font, FLOAT ScaleX, FLOAT ScaleY, UBOOL Center, const TCHAR* Text ); 
};

