#include "ParticleSystems.h"
#include "UnParticleRender.h"

//------------------------------------------------------------------------------
INT UPulseLineTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UPulseLineTemplate::Render);
	NOTE(debugf(TEXT("UPulseLineTemplate::Render")));

	APulseLineGenerator* Parent = CastChecked<APulseLineGenerator>(Owner->Actor);

	INT iTemplate=0;
	while( iTemplate<Parent->ParticleTemplates.Num() && Parent->ParticleTemplates(iTemplate)!=this ) iTemplate++;
	if( iTemplate==Parent->ParticleTemplates.Num() )
	{
		debugf(NAME_Warning,TEXT("%s is not in parent %s"),GetFullName(),Parent->GetFullName());
		return 0;
	}

	FTConnectionInfo* CInfo = &Parent->Connections(iTemplate);

	if( !Parent || !Parent->bOn || !Parent->BeamTexture || CInfo->Start==CInfo->End )
		return 0;

#if 0 //DEBUG
	debugf(TEXT("### drawing ###"));
	RI->PushState();
	RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);
	FLineBatcher LineBatcher(RI);
	LineBatcher.DrawLine(CInfo->Start,CInfo->End,FColor(0,255,0));
	LineBatcher.Flush();
	RI->PopState();
#endif

	INT NumParticlesDrawn = 0;

	// the main beam renderer
	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, Parent->BeamTexture, Sty2Style(Parent->BeamStyle) );

	PROJECTIONS

	FVector  Diff         = CInfo->End - CInfo->Start;
	CInfo->Length         = Diff.Size();
	FVector  Dir          = Diff.SafeNormal();
	INT      NumBeams     = appRandRange( (INT)Parent->NumBeams.A, (INT)Parent->NumBeams.B );

	FColor BeamColor = Parent->GetBeamColor();
	NOTE(debugf( L"Color  %d, %d, %d %d", BeamColor.R, BeamColor.G, BeamColor.B, BeamColor.A ););
	
	// Update render bounds.
	Extents = FBox(0);
	Extents += CInfo->Start;
	Extents += CInfo->End;

	TArray<JointItem> Joints;

	for( INT iBeams=0; iBeams<NumBeams; iBeams++ )
	{
		INT NumQuadPts = 0;
		FLOAT Length = 0;
		while( Length <= CInfo->Length )
		{
			// Set up this point for the quadstrip
			QuadPts[NumQuadPts].Location = CInfo->Start + (Length * Dir);
			if( 0 < Length && Length < CInfo->Length )
				QuadPts[NumQuadPts].Location += ((VRand() ^ Dir).SafeNormal() * Parent->BeamWidth.GetRand());
			QuadPts[NumQuadPts].Color = BeamColor;
			QuadPts[NumQuadPts].Width = Parent->BeamTextureWidth;

			// Set up the next point with LOD
			FVector ParticleOffset = QuadPts[NumQuadPts].Location - Frame->ViewOrigin;
			FLOAT SegDistSq = ParticleOffset.SizeSquared();
			FLOAT LOD = Clamp( SegDistSq * 0.00025f, 1.f, 40.f );

			// Store data for sprite joints.
			if( SegDistSq < 1000*1000 )
			{
				INT Index = Joints.Add();
				Joints(Index).QuadPtsIndex = NumQuadPts;
				Joints(Index).SegDistSq = SegDistSq;
			}

			NumQuadPts++;
			if( Length == CInfo->Length )
				break;
			Length = Min<FLOAT>( Length + Parent->BeamSegLength.GetRand() * LOD,CInfo->Length );
//			Length = Min<FLOAT>( Length + Parent->BeamSegLength.GetRand(),CInfo->Length );
		}
		SP.AddQuadStrip( NumQuadPts, 0, 0 );
		NumParticlesDrawn++;
	}

	SP.Release();
	
	// the main beam renderer
	FDefaultSpriteRenderer<> SPJ( Frame, Lights, RI, Owner, this, Parent->SpriteJointTexture, Sty2Style(Parent->SpriteJointStyle) );

	FColor SpriteJointColor = Parent->GetSpriteColor();

	for( INT i=0; i<Joints.Num(); i++)
	{
		FLOAT SegDist = appSqrt( Joints(i).SegDistSq );

		FLOAT Size = Parent->SpriteJointSize.GetRand();

		FVector Up		= ProjUp	* Size;
		FVector Right	= ProjRight	* Size;

		SPJ.AddQuad(
			SpriteJointColor.Scale(1.f - (SegDist-1000.0f) / 1000.f,true),
			QuadPts[Joints(i).QuadPtsIndex].Location - Up - Right,
			QuadPts[Joints(i).QuadPtsIndex].Location + Up - Right,
			QuadPts[Joints(i).QuadPtsIndex].Location + Up + Right,
			QuadPts[Joints(i).QuadPtsIndex].Location - Up + Right,
			FRectangle(0,0,1,1)
			);
		NumParticlesDrawn++;
	}
	SPJ.Release();

	return NumParticlesDrawn;

	unguard;
}

IMPLEMENT_CLASS(UPulseLineTemplate);

