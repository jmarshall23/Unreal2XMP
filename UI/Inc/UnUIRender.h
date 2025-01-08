
#ifndef _INC_UIRender
#define _INC_UIRender

#ifndef NOTE
#define NOTE(a) {}
#endif

#ifndef NOTE0
#define NOTE0(a) {}
#endif

#ifndef NOTE1
#define NOTE1(a) a
#endif

#include "UnUIRenderTypes.h"

// Fix ARL: Dynamically tweak sizes based on usage?

typedef FUIVertexStream<FUIVertex,512>	SpriteVB;
typedef FUIIndexBuffer<768>				SpriteIB;

class /*UI_API*/ FDefaultUIRenderer: public FUIRenderer
{
	typedef FVertexStreamChunk<SpriteVB::SIZE,SpriteIB::SIZE> VertexStreamChunk;

public:
	FDefaultUIRenderer(UViewport* InViewport)
	:	Viewport(InViewport), RI(InViewport->RI)
//!!MERGE	,	Stats(InViewport->Actor->GetLevel()->Engine->Stats)
	,	bNeedsRender(false), bReleased(false)
	{
		guard(FDefaultUIRenderer::FDefaultUIRenderer);
		NOTE(debugf(TEXT("FDefaultUIRenderer::FDefaultUIRenderer")));

		RI->PushState();

		Reset();

		FMatrix LocalToWorld	= FTranslationMatrix	(	FVector	(	-Viewport->SizeX / 2.0f - 0.5f
																	,	-Viewport->SizeY / 2.0f - 0.5f
																	,	0.0f))
								* FScaleMatrix			(	FVector	(	2.0f / Viewport->SizeX
																	,	-2.0f / Viewport->SizeY
																	,	1.0f));
		RI->SetTransform(TT_LocalToWorld,LocalToWorld);
		RI->SetTransform(TT_WorldToCamera,FMatrix::Identity);
		RI->SetTransform(TT_CameraToScreen,FMatrix::Identity);

		new(ClipStack)FRectangle(0,0,Viewport->SizeX,Viewport->SizeY);
		Clip = &ClipStack.Last();

		ScaleStack.AddItem(FMatrix::Identity);

		unguard;
	}

	~FDefaultUIRenderer()
	{
		guard(FDefaultUIRenderer::~FDefaultUIRenderer);
		NOTE(debugf(TEXT("FDefaultUIRenderer::~FDefaultUIRenderer")));

		Release();

		unguard;
	}

	void Release()
	{
		guard(FDefaultUIRenderer::Release);
		NOTE(debugf(TEXT("FDefaultUIRenderer::Release")));

		if(bReleased) return;

		Render();
		RI->PopState();
		bReleased = true;

		unguard;
	}

	void Flush()
	{
		guard(FDefaultUIRenderer::Flush);
		NOTE(debugf(TEXT("FDefaultUIRenderer::Flush")));
		
//!!MERGE		Stats->UI_NumFlushes++;

		Render();
		Reset();

		unguard;
	}
	
	void DrawQuad( UMaterial* Material, const FQuadInfo &Quad, FLOAT Z=0, DWORD PolyFlags=0, FColor Color=FColor(255,255,255,255) )
	{
		guardSlow(FDefaultUIRenderer::DrawTile);
		NOTE0(debugf(TEXT("FDefaultUIRenderer::DrawQuad( %s, %s, %f, %d, %s )"),Material?Material->GetFullName():TEXT("None"),*Quad.String(),Z,PolyFlags,*Color.String()));

//!!MERGE		FClock Clock(Stats->UI_DrawQuadCycles);

		if(bReleased) return;

		SetVertexStream(Material,PolyFlags,4,6);

		INT I1 = AddVertex();
		CurrentVertex->Position	= FVector(Quad.Pts[0].X,Quad.Pts[0].Y,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= Quad.Pts[0].U;
		CurrentVertex->V		= Quad.Pts[0].V;

		INT I2 = AddVertex();
		CurrentVertex->Position	= FVector(Quad.Pts[1].X,Quad.Pts[1].Y,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= Quad.Pts[1].U;
		CurrentVertex->V		= Quad.Pts[1].V;

		INT I3 = AddVertex();
		CurrentVertex->Position	= FVector(Quad.Pts[2].X,Quad.Pts[2].Y,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= Quad.Pts[2].U;
		CurrentVertex->V		= Quad.Pts[2].V;

		INT I4 = AddVertex();
		CurrentVertex->Position	= FVector(Quad.Pts[3].X,Quad.Pts[3].Y,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= Quad.Pts[3].U;
		CurrentVertex->V		= Quad.Pts[3].V;

		AddIndex(I1);
		AddIndex(I2);
		AddIndex(I3);

		AddIndex(I1);
		AddIndex(I3);
		AddIndex(I4);
		
		unguardSlow;
	}

	//FImage Image(TEXT("TEMP"),Material,Tex.X,Tex.Y,Tex.Width,Tex.Height);

	// NOTE[aleiby]: Broken out so it can be timed separately from DrawString.
	void DrawTile( FImage* Image, const FRectangle &Loc, FLOAT Z=0, DWORD PolyFlags=0, FColor Color=FColor(255,255,255,255) )
	{
//!!MERGE		FClock Clock(Stats->UI_DrawTileCycles);
		DrawTileEx(Image,Loc,Z,PolyFlags,Color);
	}
	void DrawTileEx( FImage* Image, const FRectangle &Loc, FLOAT Z=0, DWORD PolyFlags=0, FColor Color=FColor(255,255,255,255) )
	{
		guardSlow(FDefaultUIRenderer::DrawTile);
		NOTE(debugf(TEXT("DefaultUIRenderer::DrawTile( %s, %s, %s, %f, %d, %s )"),Image?*Image->String():TEXT("None"),*Loc.String(),*Tex.String(),Z,PolyFlags,*Color.String()));

		if(bReleased) return;

		// Clip tile.
		FRectangle R=Clip->Intersection(Loc);
		if(R==FRectangle(0,0,0,0))
			return;

		// Prefetch/calc components.
		FLOAT	X0 = R.X,
				Y0 = R.Y,
				X1 = R.X+R.Width,
				Y1 = R.Y+R.Height;

		FLOAT	LocX0 = Loc.X,
				LocY0 = Loc.Y,
				LocX1 = Loc.X+Loc.Width,
				LocY1 = Loc.Y+Loc.Height;

		FLOAT	W=Image->W,
				H=Image->H;

		if (Image->bTile){ W=R.Width; H=R.Height; }

		FLOAT	TexU0 = Image->X,
				TexV0 = Image->Y,
				TexU1 = Image->X+W,
				TexV1 = Image->Y+H;

		// Update clipped UVs.
		FLOAT	U0 = appBlend( TexU0, TexU1, appAblend( LocX0, LocX1, X0 ) ),
				V0 = appBlend( TexV0, TexV1, appAblend( LocY0, LocY1, Y0 ) ),
				U1 = appBlend( TexU0, TexU1, appAblend( LocX0, LocX1, X1 ) ),
				V1 = appBlend( TexV0, TexV1, appAblend( LocY0, LocY1, Y1 ) );

		UMaterial* Material = Image->Material;

		// Normalize UVs.
		U0 /= Material->MaterialUSize();
		V0 /= Material->MaterialVSize();
		U1 /= Material->MaterialUSize();
		V1 /= Material->MaterialVSize();

		//!!ARL
		X0 = (INT)X0;
		Y0 = (INT)Y0;
		X1 = (INT)X1;
		Y1 = (INT)Y1;

		SetVertexStream(Material,PolyFlags,4,6);

		INT I1 = AddVertex();
		CurrentVertex->Position	= FVector(X0,Y0,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= U0;
		CurrentVertex->V		= V0;

		INT I2 = AddVertex();
		CurrentVertex->Position	= FVector(X1,Y0,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= U1;
		CurrentVertex->V		= V0;

		INT I3 = AddVertex();
		CurrentVertex->Position	= FVector(X1,Y1,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= U1;
		CurrentVertex->V		= V1;

		INT I4 = AddVertex();
		CurrentVertex->Position	= FVector(X0,Y1,Z);
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= U0;
		CurrentVertex->V		= V1;

		AddIndex(I1);
		AddIndex(I2);
		AddIndex(I3);

		AddIndex(I1);
		AddIndex(I3);
		AddIndex(I4);

		unguardSlow;
	}

	INT DrawString( const TCHAR* Text, FLOAT DrawX, FLOAT DrawY, UFont* Font, FLOAT Z=0, DWORD PolyFlags=0, FColor Color=FColor(255,255,255,255), UBOOL bClip=false, UBOOL bHandleApersand=true )
	{
		guardSlow(FDefaultUIRenderer::DrawString);
		NOTE(debugf(TEXT("FDefaultUIRenderer::DrawString( %s, %f, %f, %s, %d, %s, %s, %s )"),Text,DrawX,DrawY,Font?Font->GetFullName():TEXT("None"),PolyFlags,*Color.String(),bClip?TEXT("True"):TEXT("False"),bHandleApersand?TEXT("True"):TEXT("False")));

//!!MERGE		FClock Clock(Stats->UI_DrawStringCycles);

		if(bReleased) return 0;

		// Fix ARL: Steal ampersand underlining functionality from Canvas?

		// Draw all characters in string.
		INT LineX = 0;
		for( INT i=0; Text[i]; i++ )
		{
			INT Ch = (TCHARU)Font->RemapChar(Text[i]);

			// Process character if it's valid.
			if( Ch < Font->Characters.Num() )
			{
				FFontCharacter& Char = Font->Characters(Ch);
				UTexture* PageMaterial;
				if( Char.TextureIndex < Font->Textures.Num() && (PageMaterial=Font->Textures(Char.TextureIndex))!=NULL )
				{
					INT	CharUSize=Char.USize+Font->XPad,
						CharVSize=Char.VSize+Font->YPad;
					FImage Image(TEXT("TEMP"),PageMaterial,Char.StartU,Char.StartV,CharUSize,CharVSize);
					DrawTileEx
						(	&Image
						,	FRectangle(DrawX+LineX,DrawY,CharUSize,CharVSize)
						,	Z,PolyFlags,Color
						);
					LineX += Char.USize;
				}
			}
		}

		return LineX;

		unguardSlow;
	}
	
	void PushClip(const FRectangle &R)
	{
		ClipStack.AddItem(Clip->Intersection(R));
		Clip = &ClipStack.Last();
	}
	void PopClip()
	{
		ClipStack.Pop();
		Clip = &ClipStack.Last();
	}

	void PushScale(const FDimension &Scale)
	{
		Flush();
		ScaleStack.AddItem
			(	FTranslationMatrix(FVector(-0.5f,-0.5f,0.0f))
			*	FScaleMatrix(FVector(Scale.Width,Scale.Height,1.0f))
			*	FTranslationMatrix(FVector(Scale.Width/2.0f,Scale.Height/2.0f,0.0f)));
		RI->SetTransform(TT_CameraToScreen,ScaleStack.Last());

		ClipStack.AddItem(FRectangle(-4096,-4096,16384,16384));	// Fix ARL: Properly scale instead (have to account for this in both PushClip and PushScale).
		Clip = &ClipStack.Last();
	}
	void PopScale()
	{
		Flush();
		ScaleStack.Pop();
		RI->SetTransform(TT_CameraToScreen,ScaleStack.Last());

		PopClip();
	}

private:
	void Reset()
	{
		guard(FDefaultUIRenderer::Reset);
		NOTE(debugf(TEXT("FDefaultUIRenderer::Reset")));

		SpriteVertices = new SpriteVB(RI);
		SpriteIndices = new SpriteIB(RI);

		VBTail = ((SpriteVB*)SpriteVertices)->VB;
		IBTail = ((SpriteIB*)SpriteIndices)->IB;

		VertexIndex = 0;
		CurrentChunk = NULL;

		bNeedsRender = true;

		unguard;
	}

	void Render()
	{
		guard(FDefaultUIRenderer::Render);
		NOTE(debugf(TEXT("FDefaultUIRenderer::Render")));

//!!MERGE		FClock Clock(Stats->UI_RenderCycles);

		if(!bNeedsRender) return;

		RI->UnlockDynamicStream(SpriteVertices);
		RI->UnlockDynamicIndexBuffer(SpriteIndices,SpriteVertices->GetBaseIndex());
		RI->SetVertexShader(VS_FixedFunction,SpriteVertices);

		INT BaseIndex = SpriteIndices->GetBaseIndex();

		if(CurrentChunk)
		{
			UMaterial* Material = CurrentChunk->Material;

			if( Material->IsA(UTexture::StaticClass()) )
			{
				DECLARE_STATIC_UOBJECT( UFinalBlend, FinalBlend, {} );

				switch( CurrentChunk->PolyFlags )
				{
				case PF_AlphaTexture:
					FinalBlend->FrameBufferBlending = FB_AlphaBlend;
					FinalBlend->Material			= Material;
					FinalBlend->TwoSided			= 1;
					FinalBlend->ZWrite				= 0;
					FinalBlend->ZTest				= 1;
					FinalBlend->AlphaTest			= 1;
					FinalBlend->AlphaRef			= 0;
					break;
				case PF_Translucent:
					FinalBlend->FrameBufferBlending = FB_Translucent;
					FinalBlend->Material			= Material;
					FinalBlend->TwoSided			= 1;
					FinalBlend->ZWrite				= 0;
					FinalBlend->ZTest				= 1;
					FinalBlend->AlphaTest			= 0;
					FinalBlend->AlphaRef			= 0;
					break;
				case PF_Modulated:
					FinalBlend->FrameBufferBlending = FB_Modulate;//FB_AlphaModulate_MightNotFogCorrectly;
					FinalBlend->Material			= Material;
					FinalBlend->TwoSided			= 1;
					FinalBlend->ZWrite				= 0;
					FinalBlend->ZTest				= 1;
					FinalBlend->AlphaTest			= 1;
					FinalBlend->AlphaRef			= 0;
					break;
				case PF_Masked:
					FinalBlend->FrameBufferBlending = FB_Overwrite;
					FinalBlend->Material			= Material;
					FinalBlend->TwoSided			= 1;
					FinalBlend->ZWrite				= 1;
					FinalBlend->ZTest				= 1;
					FinalBlend->AlphaTest			= 1;
					FinalBlend->AlphaRef			= 127;
					break;
				default:
					FinalBlend->FrameBufferBlending = FB_Overwrite;
					FinalBlend->Material			= Material;
					FinalBlend->TwoSided			= 1;
					FinalBlend->ZWrite				= 1;
					FinalBlend->ZTest				= 1;
					FinalBlend->AlphaTest			= 0;
					FinalBlend->AlphaRef			= 0;
					break;
				}

				Material = FinalBlend;
			}

			RI->SetMaterial( Material );

			INT	NumPrims = CurrentChunk->GetTriangleCount(),
				MaxIndex = CurrentChunk->GetVertexCount()-1;

			RI->DrawPrimitive(PT_TriangleList,BaseIndex,NumPrims,0,MaxIndex);
			NOTE(debugf(TEXT("DrawPrim %d %d %d %d"),BaseIndex,NumPrims,0,MaxIndex));

//!!MERGE			Stats->UI_Triangles += NumPrims;

			BaseIndex += VertexStreamChunk::INDICES;
		}

		// Cleanup.
		delete SpriteVertices;
		delete SpriteIndices;

		bNeedsRender=false;

		unguard;
	}

	void SetVertexStream( UMaterial* Material, DWORD PolyFlags, INT RequiredVertices, INT RequiredIndices )
	{
		NOTE(debugf(TEXT("FDefaultUIRenderer::SetVertexStream( %s, %d )"),Material?Material->GetFullName():TEXT("None"),PolyFlags));

		if(CurrentChunk && (CurrentChunk->Material!=Material || CurrentChunk->PolyFlags!=PolyFlags || !CurrentChunk->HasRoom(RequiredVertices,RequiredIndices)))
		{
			Flush();
			CurrentChunk = NULL;
		}

		if(!CurrentChunk)
		{
			NOTE(debugf(TEXT("Creating new chunk...")));
			VertexStreamChunks = VertexStreamChunk(Material,PolyFlags,VBTail,IBTail,VertexIndex);
			CurrentChunk = &VertexStreamChunks;

			// Update tail pointers.
			VertexIndex += VertexStreamChunk::VERTICES;
			VBTail += VertexStreamChunk::VERTICES;
			IBTail += VertexStreamChunk::INDICES;
		}
	}

	INT AddVertex()
	{
		return CurrentChunk->AddVertex(CurrentVertex);
	}
	void AddIndex(INT Index)
	{
		CurrentChunk->AddIndex(Index);
	}

private:
	FDefaultUIRenderer(){}

	UViewport* Viewport;
	FRenderInterface* RI;
//!!MERGE	FStats* Stats;

	TArray<FRectangle> ClipStack;
	FRectangle* Clip;

	TArray<FMatrix> ScaleStack;

	FVertexStream* SpriteVertices;
	FIndexBuffer* SpriteIndices;
	
	FUIVertex* CurrentVertex;

	FUIVertex*	VBTail;
	_WORD*		IBTail;

	INT VertexIndex;
	
	BITFIELD bNeedsRender:1	GCC_PACK(4);
	BITFIELD bReleased:1;

	VertexStreamChunk* CurrentChunk;
	VertexStreamChunk VertexStreamChunks;
};

#endif //_INC_UIRender

