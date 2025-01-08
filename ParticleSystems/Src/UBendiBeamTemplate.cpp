#include "ParticleSystems.h"
#include "UnParticleRender.h"

// NOTE[aleiby]: This code was *not* written to be expandable.  
// You're better off just rewriting your own effect using this as a reference.
// NOTE[mjl]: Hahahaha. I have made it expandable. I RUL3Z0R!!!!
// NOTE[aleiby]: Maybe so, but now it's brok0r3d!!!   /me cleans house

//------------------------------------------------------------------------------
INT UBendiBeamTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UBendiBeamTemplate::Render);

	ABendiBeamGenerator* Parent = CastChecked<ABendiBeamGenerator>(GetOuter());

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

	INT NumParticlesDrawn = 0;

	// the main beam renderer
	FDefaultSpriteRenderer<> SP( Frame, Lights, RI, Owner, this, Parent->BeamTexture, Sty2Style(Parent->BeamStyle) );

	PROJECTIONS

	#define AxisOffset LastLocation
	#define Percentage Velocity.X
	#define UpdateTime LifeSpan

	// Update render bounds.
	Extents = FBox(0);
	Extents += CInfo->Start;
	Extents += CInfo->End;

	FVector Diff = CInfo->End - CInfo->Start;
	CInfo->Length = Diff.Size();
	FVector Dir = Diff / CInfo->Length; //normalize

	FColor BeamColor = Parent->GetBeamColor();

	//temp: hack to avoid overflowing VB/IB limit.
	if( CInfo->Length > 2400.f )
	{
		CInfo->Length = 2400.f;
		CInfo->End = CInfo->Start + CInfo->Length * Dir;
	}

	// Update meta beam periodically.
	for( INT i=0; i<ParticleList.Num()-1; i++ )
	{
		UParticle* Particle = ParticleList(i);
		if( Particle->UpdateTime && Frame->Viewport->CurrentTime >= Particle->UpdateTime )
		{
			Particle->AxisOffset = (VRand() ^ Dir) * Parent->MetaBeamWidth.GetRand();
			Particle->UpdateTime = Frame->Viewport->CurrentTime + Parent->MetaBeamRefreshTime.GetRand();
		}
	}

	// Initialize meta beam.
	if( ParticleList.Num()==0 )
	{
		bReverseDrawOrder = true;	// so percentages below are in ascending order.

		FLOAT Length = 0;
		while( Length <= CInfo->Length )
		{
			UParticle* Particle = GetParticle();
			Particle->Percentage = Length / CInfo->Length;
			if( 0 < Particle->Percentage && Particle->Percentage < 1 )
			{
				Particle->AxisOffset = (VRand() ^ Dir) * Parent->MetaBeamWidth.GetRand();	// Store a random offset.
				Particle->UpdateTime = Frame->Viewport->CurrentTime + appFrand() * Parent->MetaBeamRefreshTime.GetMax();
			}
			else
			{
				Particle->AxisOffset = FVector(0,0,0);
				Particle->UpdateTime = 0;
			}

			if( Length == CInfo->Length )
				break;
			Length = Min( Length + Parent->MetaBeamSegLength.GetRand(), CInfo->Length );
		}
	}

	// Ensure we have enough joints to continue.
	if( ParticleList.Num()<2 )
		return 0;

	// Update meta beam joint locations relative to new (current) Start and End points.
	ParticleList(0)->Location = CInfo->Start;
	ParticleList.Last()->Location = CInfo->End;
	for( INT i=1; i<ParticleList.Num()-1; i++ )
	{
		UParticle* Particle = ParticleList(i);
		Particle->Location = CInfo->Start + Particle->Percentage * Diff + Particle->AxisOffset;
	}

	TArray<JointItem> Joints;

	INT      NumBeams     = appRandRange( (INT)Parent->NumBeams.A, (INT)Parent->NumBeams.B );
	for( INT iBeams=0; iBeams<NumBeams; iBeams++ )
	{
		INT NumQuadPts = 0;
		
		QuadPts[NumQuadPts].Location = CInfo->Start;
		QuadPts[NumQuadPts].Color = BeamColor;
		QuadPts[NumQuadPts].Width = Parent->BeamTextureWidth;
		NumQuadPts++;

		FVector LastLocation = CInfo->Start;
		
		for( INT i=1; i<ParticleList.Num(); i++ )
		{
			UParticle* Particle = ParticleList(i);

			FVector  SegDiff    = Particle->Location - LastLocation;
			FLOAT    SegLength  = SegDiff.Size();
			FVector  SegDir     = SegDiff / SegLength; //normalize

			FLOAT Length = Parent->BeamSegLength.GetRand();

			while( Length < SegLength )
			{
				// Set up this point for the quadstrip
				QuadPts[NumQuadPts].Location = LastLocation + (Length * SegDir);
				QuadPts[NumQuadPts].Location += (VRand() ^ SegDir) * Parent->BeamWidth.GetRand();
				QuadPts[NumQuadPts].Color = BeamColor;
				QuadPts[NumQuadPts].Width = Parent->BeamTextureWidth;

				// Set up the next point with LOD
				FLOAT SegDistSq = (QuadPts[NumQuadPts].Location - Frame->ViewOrigin).SizeSquared();
				//disabled lod for bendibeams
				//FLOAT LOD = Clamp( SegDistSq * 0.00025f, 1.f, 1.f );

				// Store data for sprite joints.
				if( SegDistSq < 1000*1000 )
				{
					INT Index = Joints.Add();
					Joints(Index).QuadPtsIndex = NumQuadPts;
					Joints(Index).SegDistSq = SegDistSq;
				}

				//disabled lod for bendibeams
				//Length += Parent->BeamSegLength.GetRand() * LOD;
				Length += Parent->BeamSegLength.GetRand();

				NumQuadPts++;
			}
			LastLocation = QuadPts[NumQuadPts-1].Location;
		}

		QuadPts[NumQuadPts].Location = CInfo->End;
		QuadPts[NumQuadPts].Color = BeamColor;
		QuadPts[NumQuadPts].Width = Parent->BeamTextureWidth;
		NumQuadPts++;

		SP.AddQuadStrip( NumQuadPts, 0, 0 );
		NumParticlesDrawn++;
	}

	SP.Release();

	// the join renderer
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


IMPLEMENT_CLASS(UBendiBeamTemplate);

