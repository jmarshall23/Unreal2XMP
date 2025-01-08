
#ifndef _INC_ParticleRender
#define _INC_ParticleRender

#define PROJECTIONS \
	FVector ProjRight	= (Frame->CameraX*1000).SafeNormal(); \
	FVector ProjUp		= (Frame->CameraY*1000).SafeNormal(); \
	FVector ProjFront	= ProjRight ^ ProjUp;

/*NOTE[aleiby]: Scaling by 1000 so SafeNormal doesn't return vect(0,0,0) at really low FOV(5).
	This might be a better way -- if we cached it in the SceneNode:
		FVector ProjBase	= SceneNode->Deproject(FPlane(0,0,0,1));
		FVector ProjUp		= SceneNode->Deproject(FPlane(0,-1,0,1)) - ProjBase;
		FVector ProjRight	= SceneNode->Deproject(FPlane(1,0,0,1)) - ProjBase;
		FVector ProjFront	= ProjRight ^ ProjUp;
*/

#define Sty2Style(DrawSty) \
		(	(DrawSty == STY_Translucent)	? STYLE_Translucent \
		:	(DrawSty == STY_Modulated)		? STYLE_Modulated \
		:	(DrawSty == STY_Alpha)			? STYLE_Alpha \
		:									  STYLE_Normal )

// Fix ARL: Dynamically tweak size based on usage?

#define SPRITE_BUFFER_SIZE 512
typedef FU2ParticleVertexStream<FU2SpriteParticleVertex,SPRITE_BUFFER_SIZE>	SpriteVB;
typedef FU2RawIndexBuffer<SPRITE_BUFFER_SIZE*3>								SpriteIB;

template<UBOOL TPointList=0>
class PARTICLESYSTEMS_API FSpriteRenderer
{
public:
	FSpriteRenderer
	(	FLevelSceneNode* InFrame, TList<FDynamicLight*>* InLights, FRenderInterface* InRI,
		FDynamicActor* InOwner, UParticleTemplate* InTemplate,
		UMaterial* Texture, BYTE DrawStyle, UBOOL InZTest = 1
	)
	:	Frame(InFrame), Lights(InLights), RI(InRI)
	,	Owner(InOwner), Template(InTemplate)
	,	NumPointsDrawn(0), TotalPointsDrawn(0)
	{
		SpriteVertices = new SpriteVB(RI);
		SpriteIndices = new SpriteIB(RI);

		RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);
		RI->EnableLighting(0,1);

		UMaterial* Material = Texture;

		DECLARE_STATIC_UOBJECT( UFinalBlend, FinalBlend, {} );

		switch( DrawStyle )
		{
		case STYLE_Alpha:
			FinalBlend->FrameBufferBlending = FB_AlphaBlend;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 1;
			FinalBlend->AlphaRef			= 0;
			break;
		case STYLE_Translucent:
			FinalBlend->FrameBufferBlending = FB_ShadowBlend;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 0;
			FinalBlend->AlphaRef			= 0;
			break;
		case STYLE_Modulated:
			FinalBlend->FrameBufferBlending = FB_Modulate;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 0;
			FinalBlend->AlphaRef			= 0;
			break;
		case STYLE_AlphaModulate:
			FinalBlend->FrameBufferBlending = FB_AlphaModulate_MightNotFogCorrectly;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 1;
			FinalBlend->AlphaRef			= 0;
			break;
		case STYLE_Brighten:
			FinalBlend->FrameBufferBlending = FB_Brighten;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 0;
			FinalBlend->AlphaRef			= 0;
			break;
		case STYLE_Darken:
			FinalBlend->FrameBufferBlending = FB_Darken;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 0;
			FinalBlend->AlphaRef			= 0;
			break;
		case STYLE_TrueTranslucent:
			FinalBlend->FrameBufferBlending = FB_Translucent;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 0;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 0;
			FinalBlend->AlphaRef			= 0;
			break;
		case STYLE_Masked:
			FinalBlend->FrameBufferBlending = FB_Overwrite;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 1;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 1;
			FinalBlend->AlphaRef			= 127;
			break;
		default:
			FinalBlend->FrameBufferBlending = FB_Overwrite;
			FinalBlend->Material			= Material;
			FinalBlend->TwoSided			= 1;
			FinalBlend->ZWrite				= 1;
			FinalBlend->ZTest				= InZTest;
			FinalBlend->AlphaTest			= 0;
			FinalBlend->AlphaRef			= 0;
			break;
		}

		Material = FinalBlend;
		RI->SetMaterial( Material );

//		Template->SetupLighting( RI, Lights );
	}

//	~FSpriteRenderer()
	void Release()
	{
		Render();
		delete SpriteVertices;
		delete SpriteIndices;
//		Template->TeardownLighting( RI );
	}

protected:
	virtual void Reset()
	{
		SpriteVertices = new SpriteVB(RI);
		SpriteIndices = new SpriteIB(RI);
		NumPointsDrawn = 0;
	}

	UBOOL Flush( INT RequiredMinVB, INT RequiredMinIB )
	{
		//check( RequiredMinIB <= SpriteIB::SIZE );
		//check( RequiredMinVB <= SpriteVB::SIZE );
		if( RequiredMinIB > SpriteIB::SIZE || RequiredMinVB > SpriteVB::SIZE )
		{
			debugf(NAME_Warning,TEXT("FSpriteRenderer overrun!! IB: %i[%i] VB: %i[%i]"),RequiredMinIB,SpriteIB::SIZE,RequiredMinVB,SpriteVB::SIZE);
			return false;
		}

		if
		(	GetIndexCount() > (SpriteIB::SIZE - RequiredMinIB)
		||	GetVertexCount() > (SpriteVB::SIZE - RequiredMinVB)
		)
		{
			NOTE(debugf(TEXT("Flushing: %d %d %d %d"),RequiredMinIB,GetIndexCount(),RequiredMinVB,GetVertexCount()));
			GStats.DWORDStats(GEngineStats.STATS_Particle_Flushes)++;
			Render();
			delete SpriteVertices;
			delete SpriteIndices;
			Reset();
		}

		return true;
	}
	
	void Render()
	{
		guard(FSpriteRenderer::Render);

		RI->UnlockDynamicStream(SpriteVertices);
		RI->UnlockDynamicIndexBuffer(SpriteIndices,SpriteVertices->GetBaseIndex());
		RI->SetVertexShader(VS_FixedFunction,SpriteVertices);

		INT BaseIndex = SpriteIndices->GetBaseIndex();

		INT VertexCount = GetVertexCount();
		if( VertexCount == 0 )
			return;

		INT IndexCount = GetIndexCount();
		if( IndexCount == 0 )
			return;

		TotalPointsDrawn += NumPointsDrawn;

		if(TPointList)
		{
			RI->DrawPrimitive(
				PT_PointList,
				BaseIndex,
				VertexCount,
				0,
				VertexCount-1
				);
		}
		else
		{
			INT NumTriangles = IndexCount/3;
			RI->DrawPrimitive(
				PT_TriangleList,
				BaseIndex,
				NumTriangles,
				0,
				VertexCount-1
				);
/*!!ARL
			//Begin Projector
			if( Owner->Actor->Projectors.Num() )
			{
				RI->PushState();
				RI->SetZBias(15);
				for( INT i=0;i<Owner->Actor->Projectors.Num();i++ )
				{
					FProjectorRenderInfo* P = Owner->Actor->Projectors(i);
					if( !P->Render() )
					{
						Owner->Actor->Projectors.Remove(i--);
						continue;
					}
					UTexture* Tex = P->Texture->Get(Frame->Viewport->CurrentTime);
					RI->SetBlending( Tex->PolyFlags|PF_TwoSided );
					RI->SetTexture( 0, Tex->GetRenderInterface() );
					//!!HACK - vogel
					if( Tex->PolyFlags & PF_Modulated )
					{
						RI->EnableLighting( false );
						RI->SetDistanceFog( -1, 0, 0, FColor(127,127,127,255) );
					}
					RI->SetTextureTransform( 0, P->Matrix, TC_WorldCoords, P->ProjectorFlags&PRF_Projected );
					RI->DrawPrimitive(
						PT_TriangleList,
						BaseIndex,
						NumTriangles,
						0,
						IndexCount-1
						);
					Stats->Particle_Projectors++;
				}
				RI->PopState();
			}
			//End Projector
*/
		}

		unguard;
	}

public:
	void AddPoint( FColor Color, FVector V1 )
	{
		Flush(1,1);

		AddVertex();
		CurrentVertex->Position	= V1;
		CurrentVertex->Color	= Color;
		AddIndex( NumPointsDrawn++ );
	}
	
	void AddTriangle( FColor Color, FVector V1, FVector V2, FVector V3 )
	{
		Flush(3,3);

		AddVertex();
		CurrentVertex->Position	= V1;
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= 0;
		CurrentVertex->V		= 0;

		AddVertex();
		CurrentVertex->Position	= V2;
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= 1;
		CurrentVertex->V		= 0;

		AddVertex();
		CurrentVertex->Position	= V3;
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= 0;
		CurrentVertex->V		= 1;

		AddIndex( NumPointsDrawn+0 );
		AddIndex( NumPointsDrawn+1 );
		AddIndex( NumPointsDrawn+2 );
		NumPointsDrawn+=3;
	}
	
	void AddQuad( FColor Color, FVector V1, FVector V2, FVector V3, FVector V4, FRectangle TexCoords )
	{
		NOTE(debugf(TEXT("FSpriteRenderer::AddQuad( %s, %s, %s, %s, %s, %s )"),*Color.String(),*V1.String(),*V2.String(),*V3.String(),*V4.String(),*TexCoords.String()));

		Flush(4,6);

		AddVertex();
		CurrentVertex->Position	= V1;
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= TexCoords.X;
		CurrentVertex->V		= TexCoords.Y;

		AddVertex();
		CurrentVertex->Position	= V2;
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= TexCoords.X+TexCoords.Width;
		CurrentVertex->V		= TexCoords.Y;

		AddVertex();
		CurrentVertex->Position	= V3;
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= TexCoords.X+TexCoords.Width;
		CurrentVertex->V		= TexCoords.Y+TexCoords.Height;

		AddVertex();
		CurrentVertex->Position	= V4;
		CurrentVertex->Color	= Color;
		CurrentVertex->U		= TexCoords.X;
		CurrentVertex->V		= TexCoords.Y+TexCoords.Height;

		AddIndex( NumPointsDrawn+0 );
		AddIndex( NumPointsDrawn+1 );
		AddIndex( NumPointsDrawn+2 );
		AddIndex( NumPointsDrawn+0 );
		AddIndex( NumPointsDrawn+2 );
		AddIndex( NumPointsDrawn+3 );
		NumPointsDrawn+=4;
	}
	
	void AddQuadStrip( INT NumPoints, FLOAT* InV, INT NumVs )
	{
		guard(FSpriteRenderer::AddQuadStrip);
		NOTE(debugf(TEXT("FSpriteRenderer::AddQuadStrip( %d, %d, %d, %s )"),NumPoints,InV,NumVs));

		//FLineBatcher LineBatcher(RI);

		if( NumPoints < 2 )
			return;

		if( !Flush(2*NumPoints,6*NumPoints) )
			return;

		INT i;
		
		FLOAT V[4];
		switch( NumVs )
		{
		case 0:
		case 1:
			V[0] = 0.0f;
			V[1] = 1.0f;
			V[2] = 0.0f;
			V[3] = 1.0f;
			break;
		case 2: //BendiBeams, PulseLineBeams, ViewVelocityScaled
			V[0] = InV[0];
			V[1] = InV[1];
			V[2] = InV[0];
			V[3] = InV[1];
			break;
		case 3:
			V[0] = InV[0];
			V[1] = InV[1];
			V[2] = InV[1];
			V[3] = InV[2];
			break;
		case 4: //TexturedStreaks
			V[0] = InV[0];
			V[1] = InV[1];
			V[2] = InV[2];
			V[3] = InV[3];
			break;
		}

		// Build and draw streak.
		#define QuadPts		(UParticleTemplate::QuadPts)
		#define VertAxes	(UParticleTemplate::Pts)

		// Ensure this doesn't cause us to overflow
		check( NumPoints < ARRAY_COUNT(QuadPts) );
		check( NumPoints < ARRAY_COUNT(VertAxes) );

		// Find the directional axis for each vertex of the streak
		// (first and last points aren't blended)
		{	/*i=0*/
			FVector SegAxis = QuadPts[1].Location - QuadPts[0].Location;
			FVector CameraAxis = Frame->ViewOrigin - QuadPts[0].Location;
			VertAxes[0] = (SegAxis ^ CameraAxis).SafeNormal();
			VertAxes[0] *= QuadPts[0].Width;
		}
		for( i=1; i<NumPoints-1; i++ )
		{
			FVector SegAxis0 = QuadPts[i].Location - QuadPts[i-1].Location;
			FVector SegAxis1 = QuadPts[i+1].Location - QuadPts[i].Location;
			FVector CameraAxis = Frame->ViewOrigin - QuadPts[i].Location;
			FVector VertAxis0 = (SegAxis0 ^ CameraAxis).SafeNormal();
			FVector VertAxis1 = (SegAxis1 ^ CameraAxis).SafeNormal();	// Fix ARL: Cache this for use in the next iteration as VertAxes0.
			VertAxes[i] = (VertAxis0 + VertAxis1) / 2.f;
			VertAxes[i] *= QuadPts[i].Width;
		}
		{	/*i=NumPoints-1*/
			FVector SegAxis = QuadPts[i].Location - QuadPts[i-1].Location;
			FVector CameraAxis = Frame->ViewOrigin - QuadPts[i].Location;
			VertAxes[i] = (SegAxis ^ CameraAxis).SafeNormal();
			VertAxes[i] *= QuadPts[i].Width;
		}

		// This is sort of a hack. It allows for the insertion of a new pair of vertices to 
		// keep the V coordinates in alignment. Without it, the code would be much more complex,
		// or at least it would be wasting twice as many vertices per particle
		INT pushahead = 0;

		// Quad numbers are:
		// 13
		// 02

		//for( i=0; i<NumPoints-1; i++ )
		//	LineBatcher.DrawLine( QuadPts[i].Location, QuadPts[i+1].Location, FColor(0,255,255) );

		// Create the various vertices for the streak
		for( i=0; i<NumPoints; i++ )
		{
			// BMME Explanation:
			// Begin/Middle0/Middle1/End == 0/1/2/3, for easy array lookup
			// Example BMME sequence: B M0 M1 M0 M1 ... M0 M1 E
			INT BMME = ( i == 0 ) ? 0 :  ( i == NumPoints - 1 ) ? 3 : (2 - (i + pushahead) % 2);

			FColor Color = QuadPts[i].Color;

			//LineBatcher.DrawLine( QuadPts[i].Location, QuadPts[i].Location+VertAxes[i], FColor(255,0,255) );
			//LineBatcher.DrawLine( QuadPts[i].Location, QuadPts[i].Location-VertAxes[i], FColor(255,255,0) );

			AddVertex();
			CurrentVertex->Position	= QuadPts[i].Location-VertAxes[i];
			CurrentVertex->Color	= Color;
			CurrentVertex->U		= 0;
			CurrentVertex->V		= V[BMME];

			AddVertex();
			CurrentVertex->Position	= QuadPts[i].Location+VertAxes[i];
			CurrentVertex->Color	= Color;
			CurrentVertex->U		= 1;
			CurrentVertex->V		= V[BMME];

			if( i == NumPoints-2	// if the next pair will be the last one
			&&	V[BMME] == V[1]		// if we currently have the wrong V coordinate to finish
			){
				// then put another set of vertex coordinates in the stream,
				// which are identical with the exception of the V coordinate
				// The new V coordinate should allow for a successful progression to the last set
				i--;
				pushahead = 1;
				continue;
			}
		}

		// Piece together the vertices to create the needed polygons
		for( i=0; i<NumPoints-1+pushahead; i++ )
		{
			AddIndex( NumPointsDrawn+0 );
			AddIndex( NumPointsDrawn+1 );
			AddIndex( NumPointsDrawn+3 );
			AddIndex( NumPointsDrawn+0 );
			AddIndex( NumPointsDrawn+3 );
			AddIndex( NumPointsDrawn+2 );
			NumPointsDrawn+=2;
		}
		NumPointsDrawn+=2; //since the last VertexPair is not the begining of a new polygon

		unguard;
	}

public:
	INT PointsDrawn()	{ return TotalPointsDrawn;   }
	INT TrisDrawn()		{ return TotalPointsDrawn/3; }
	INT QuadsDrawn()	{ return TotalPointsDrawn/4; }

protected:
	virtual void AddVertex()=0;
	virtual void AddIndex(INT Index)=0;
	virtual INT GetIndexCount()=0;
	virtual INT GetVertexCount()=0;

protected:
	FSpriteRenderer(){}

protected:
	FLevelSceneNode* Frame;
	TList<FDynamicLight*>* Lights;
	FRenderInterface* RI;

	FDynamicActor* Owner;
	UParticleTemplate* Template;

	FVertexStream* SpriteVertices;
	FIndexBuffer* SpriteIndices;
	
	FU2SpriteParticleVertex* CurrentVertex;
	INT NumPointsDrawn,TotalPointsDrawn;
};

template<UBOOL TPointList=0>
class PARTICLESYSTEMS_API FDefaultSpriteRenderer : public FSpriteRenderer<TPointList>
{
public:
	FDefaultSpriteRenderer
	(	FLevelSceneNode* InFrame, TList<FDynamicLight*>* InLights, FRenderInterface* InRI,
		FDynamicActor* InOwner, UParticleTemplate* InTemplate, UMaterial* InTexture, BYTE InDrawStyle, UBOOL InZTest = 1
	)
	:	FSpriteRenderer<TPointList>( InFrame, InLights, InRI, InOwner, InTemplate, InTexture, InDrawStyle, InZTest )
	,	VertexCount(0),IndexCount(0)
	{
		VBIndex = ((SpriteVB*)SpriteVertices)->VB;
		IBIndex = ((SpriteIB*)SpriteIndices)->IB;
	}

protected:
	virtual void Reset()
	{
		FSpriteRenderer<TPointList>::Reset();
		VBIndex = ((SpriteVB*)SpriteVertices)->VB;
		IBIndex = ((SpriteIB*)SpriteIndices)->IB;
		VertexCount = 0;
		IndexCount = 0;
	}

protected:
	virtual void AddVertex()
	{
		CurrentVertex = VBIndex++;
		VertexCount++;
	}
	virtual void AddIndex(INT Index)
	{
		*(IBIndex++) = Index;
		IndexCount++;
	}

	virtual INT GetIndexCount()
	{
//		return ((INT)IBIndex - (INT)IBStart) / (INT)sizeof(_WORD);
		return IndexCount;
	}
	virtual INT GetVertexCount()
	{
//		return ((INT)VBIndex - (INT)VBStart) / (INT)sizeof(FU2SpriteParticleVertex);
		return VertexCount;
	}

private:
	FU2SpriteParticleVertex*	VBIndex;
	_WORD*						IBIndex;

	INT VertexCount,IndexCount;
};

//used for BendiBeams and PulseLines
struct PARTICLESYSTEMS_API JointItem {
	INT QuadPtsIndex;
	FLOAT SegDistSq;
};

#endif

