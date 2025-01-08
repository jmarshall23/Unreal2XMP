#include "ParticleSystems.h"

//------------------------------------------------------------------------------
void UTarpBuilderTemplate::InitParticle( UParticle* &P )
{
	guard(UTarpBuilderTemplate::InitParticle);
	
	Super::InitParticle( P );

	UTarpParticle* Particle = ParticleCastChecked<UTarpParticle>(P);

	Particle->UTex = 0.f;
	Particle->VTex = 0.f;
	Particle->Normal = FVector(0,0,0);
	Particle->NextUParticle = NULL;
	Particle->NextVParticle = NULL;
	Particle->Anchor = NULL;
	Particle->SlipForces.Empty();

	unguard;
}

//------------------------------------------------------------------------------
void AdjustVerts( UTarpParticle* A, UTarpParticle* B, AParticleGenerator* System )
{
	guard(UTarpBuilderTemplate::AdjustVerts);

	if( A->Anchor && !B->Anchor )
	{
		FCheckResult Hit(1.0f);
		System->GetLevel()->SingleLineCheck( Hit, System, B->Location, A->Location, TRACE_ProjTargets );
		if( Hit.Actor )
		{
			A->Location = Hit.Location;
			A->Anchor->SetParticle( A, A->Anchor->RelativeActor );
		}
	}
	else if( B->Anchor && !A->Anchor )
	{
		FCheckResult Hit(1.0f);
		System->GetLevel()->SingleLineCheck( Hit, System, A->Location, B->Location, TRACE_ProjTargets );
		if( Hit.Actor )
		{
			B->Location = Hit.Location;
			B->Anchor->SetParticle( B, B->Anchor->RelativeActor );
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
INT UTarpBuilderTemplate::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI, AParticleGenerator* System )
{
	guard(UTarpBuilderTemplate::Render);
#if 0 //MERGEFIX
	STAT(clock(GStat.ParticleDrawTime));

	INT i;
	ATarpBuilder* Parent = CastChecked<ATarpBuilder>(GetOuter());

	UTarpParticle* ParticleA;
	UTarpParticle* ParticleB;
	UTarpParticle* ParticleC;
	UTarpParticle* Particle;

	if( Parent->Width > 0.f && Parent->Height > 0.f )
	{
		// 
		// Make sure the tarp it built.
		//
		
		if( NumParticles == 0 )
		{
			Build();
		}

		//
		// Check and adjust for geometry clipping.
		//
		//  + Trace from Anchored particles to non-anchored particles to check for geometry collision,
		//    and move Anchored particle to HitLocation.
		//

		if( Parent->bAdjustVerts )
		{
			for( ParticleA = Cast<UTarpParticle>(ParticleList); ParticleA->NextVParticle; ParticleA = ParticleA->NextVParticle )
			{
				for
				(	ParticleB = ParticleA
				,	ParticleC = ParticleA->NextVParticle
				;	ParticleB->NextUParticle
				&&	ParticleC->NextUParticle
				;	ParticleB = ParticleB->NextUParticle
				,	ParticleC = ParticleC->NextUParticle
				)
				{
					AdjustVerts( ParticleB					, ParticleB->NextUParticle	, System );
					AdjustVerts( ParticleC					, ParticleC->NextUParticle	, System );
					AdjustVerts( ParticleB					, ParticleC					, System );
					AdjustVerts( ParticleB->NextUParticle	, ParticleC->NextUParticle	, System );
					AdjustVerts( ParticleB					, ParticleC->NextUParticle	, System );
					AdjustVerts( ParticleB->NextUParticle	, ParticleC					, System );
				}
			}
		}

		//
		// Draw tarp.
		//

		if( Parent->bWireframe )
		{
			for( ParticleA = Cast<UTarpParticle>(ParticleList); ParticleA->NextVParticle; ParticleA = ParticleA->NextVParticle )
			{
				for
				(	ParticleB = ParticleA
				,	ParticleC = ParticleA->NextVParticle
				;	ParticleB->NextUParticle
				&&	ParticleC->NextUParticle
				;	ParticleB = ParticleB->NextUParticle
				,	ParticleC = ParticleC->NextUParticle
				)
				{
					i=0;
					Pts[i++] = ParticleB->Location;
					Pts[i++] = ParticleB->NextUParticle->Location;
					Pts[i++] = ParticleC->NextUParticle->Location;
					Pts[i++] = ParticleC->Location;
					Pts[i++] = ParticleB->Location;
					Pts[i++] = ParticleC->NextUParticle->Location;
					Frame->Viewport->RenDev->Draw3DLines( Frame, FPlane(0,1,0,0), LINE_Transparent, i, &Pts[0] );
				}
			}
		}
		else if( Parent->Skins[0] )
		{
#if 0
			// 5---0---1
			// |  /|\  |
			// | / | \ |
			// |/  |  \|
			// 4---3---2
#else
			// 0---1
			// |\  |
			// | \ |
			// |  \|
			// 3---2

			// Calculate each poly's normal.
			for( i=0, ParticleA = Cast<UTarpParticle>(ParticleList); ParticleA->NextVParticle; ParticleA = ParticleA->NextVParticle )
			{
				for
				(	ParticleB = ParticleA
				,	ParticleC = ParticleA->NextVParticle
				;	ParticleB->NextUParticle
//				&&	ParticleC->NextUParticle
				;	ParticleB = ParticleB->NextUParticle
				,	ParticleC = ParticleC->NextUParticle
				)
				{
					UTarpParticle* &P0 = ParticleB;
					UTarpParticle* &P1 = ParticleB->NextUParticle;
					UTarpParticle* &P2 = ParticleC->NextUParticle;
					UTarpParticle* &P3 = ParticleC;

					P0->Normal = FVector(0,0,0);
					P1->Normal = FVector(0,0,0);
					P2->Normal = FVector(0,0,0);
					P3->Normal = FVector(0,0,0);

					FVector& p0 = P0->Location;
					FVector& p1 = P1->Location;
					FVector& p2 = P2->Location;
					FVector& p3 = P3->Location;

					FVector p01 = p1-p0;
					FVector p02 = p2-p0;
					FVector p03 = p3-p0;

					Pts[i++] = (p02^p01).SafeNormal();
					Pts[i++] = (p03^p02).SafeNormal();
				}
			}

			// Calculate each verts normal.
			for( i=0, ParticleA = Cast<UTarpParticle>(ParticleList); ParticleA->NextVParticle; ParticleA = ParticleA->NextVParticle )
			{
				for
				(	ParticleB = ParticleA
				,	ParticleC = ParticleA->NextVParticle
				;	ParticleB->NextUParticle
//				&&	ParticleC->NextUParticle
				;	ParticleB = ParticleB->NextUParticle
				,	ParticleC = ParticleC->NextUParticle
				)
				{
					UTarpParticle* &P0 = ParticleB;
					UTarpParticle* &P1 = ParticleB->NextUParticle;
					UTarpParticle* &P2 = ParticleC->NextUParticle;
					UTarpParticle* &P3 = ParticleC;

					FVector& n0 = Pts[i++];
					FVector& n1 = Pts[i++];

					P0->Normal += n0+n1;
					P1->Normal += n0;
					P2->Normal += n0+n1;
					P3->Normal += n1;
				}
			}
			for( Particle = Cast<UTarpParticle>(ParticleList); Particle; Particle = Cast<UTarpParticle>(Particle->NextParticle) )
				Particle->Normal = Particle->Normal.SafeNormal();

			FTransTexture**	TriPoints = TransTexturePointers;

			FTextureInfo Info;
			UTexture* Texture = Parent->Skins[0]->Get( Frame->Viewport->CurrentTime );
			Texture->Lock( Info, Frame->Viewport->CurrentTime, -1, Frame->Viewport->RenDev );

			FLOAT USize = Texture->USize;
			FLOAT VSize = Texture->VSize;

			DWORD PolyFlags = (Parent->Style == STY_Translucent) ? PF_Translucent
							: (Parent->Style == STY_Modulated)   ? PF_Modulated
							: (Parent->Style == STY_Alpha)       ? PF_AlphaTexture
							:                                      PF_Occlude;

			// Queue up four transtextures.
			for( i=0; i<4; i++ )
			{
				TriPoints[i] = &TransTextures[i];
				TriPoints[i]->Fog = FPlane(0,0,0,0);
			}

			ParticleA = Cast<UTarpParticle>(ParticleList);

			while( ParticleA->NextVParticle )
			{
				ParticleB = ParticleA->NextVParticle;
				ParticleC = ParticleB;

				Pipe( *TriPoints[0], Frame, ParticleA->Location );
				Pipe( *TriPoints[3], Frame, ParticleB->Location );

				TriPoints[0]->Light = Light( ParticleA->Location, Parent->GetLevel(), ParticleA->Normal );
				TriPoints[3]->Light = Light( ParticleB->Location, Parent->GetLevel(), ParticleB->Normal );

				TriPoints[0]->U = ParticleA->UTex * USize;
				TriPoints[0]->V = ParticleA->VTex * VSize;
				TriPoints[3]->U = ParticleB->UTex * USize;
				TriPoints[3]->V = ParticleB->VTex * VSize;

				ParticleA = ParticleA->NextUParticle;
				ParticleB = ParticleB->NextUParticle;
				while( ParticleA )
				{
					Pipe( *TriPoints[1], Frame, ParticleA->Location );
					Pipe( *TriPoints[2], Frame, ParticleB->Location );

					TriPoints[1]->Light = Light( ParticleA->Location, Parent->GetLevel(), ParticleA->Normal ) * Parent->ScaleGlow;
					TriPoints[2]->Light = Light( ParticleB->Location, Parent->GetLevel(), ParticleB->Normal ) * Parent->ScaleGlow;

					TriPoints[1]->U = ParticleA->UTex * USize;
					TriPoints[1]->V = ParticleA->VTex * VSize;
					TriPoints[2]->U = ParticleB->UTex * USize;
					TriPoints[2]->V = ParticleB->VTex * VSize;

					// Verify texture coords.
					NOTE(for( i=0; i<4; i++ ) debugf(TEXT("(%d) U: %f V: %f"), i, TriPoints[i]->U, TriPoints[i]->V ));

					Frame->Viewport->RenDev->DrawGouraudPolygon( Frame, Info, TriPoints, 4, PolyFlags, NULL );
					STAT(GStat.NumParticles++);

					ParticleA = ParticleA->NextUParticle;
					ParticleB = ParticleB->NextUParticle;

					Exchange( TriPoints[0], TriPoints[1] );
					Exchange( TriPoints[3], TriPoints[2] );
				}

				ParticleA = ParticleC;
			}

			//
			// Draw environ overlay.
			//

			if( Parent->bShiny && Parent->Texture )
			{
				FTransTexture**	TriPoints = TransTexturePointers;
			
				FTextureInfo Info;
				UTexture* Texture = Parent->Texture->Get( Frame->Viewport->CurrentTime );
				Texture->Lock( Info, Frame->Viewport->CurrentTime, -1, Frame->Viewport->RenDev );

				FLOAT UScale = Info.UScale * Info.USize / 256.0;
				FLOAT VScale = Info.VScale * Info.VSize / 256.0;

				// Queue up four transtextures.
				for( i=0; i<4; i++ )
				{
					TriPoints[i] = &TransTextures[i];
					TriPoints[i]->Light = Parent->EnvBrightness.Plane();
					TriPoints[i]->Fog = FPlane(0,0,0,0);
				}

				ParticleA = Cast<UTarpParticle>(ParticleList);

				while( ParticleA->NextVParticle )
				{
					ParticleB = ParticleA->NextVParticle;
					ParticleC = ParticleB;

					Pipe( *TriPoints[0], Frame, ParticleA->Location );
					Pipe( *TriPoints[3], Frame, ParticleB->Location );

					TriPoints[0]->Normal = ParticleA->Normal;
					TriPoints[3]->Normal = ParticleB->Normal;

					ParticleA = ParticleA->NextUParticle;
					ParticleB = ParticleB->NextUParticle;
					while( ParticleA )
					{
						Pipe( *TriPoints[1], Frame, ParticleA->Location );
						Pipe( *TriPoints[2], Frame, ParticleB->Location );
						
						TriPoints[1]->Normal = ParticleA->Normal;
						TriPoints[2]->Normal = ParticleB->Normal;

						for( i=0; i<4; i++ )
							EnviroMap( Frame, *TriPoints[i], UScale, VScale );

						Frame->Viewport->RenDev->DrawGouraudPolygon( Frame, Info, TriPoints, 4, PF_Translucent, NULL );
						STAT(GStat.NumParticles++);

						ParticleA = ParticleA->NextUParticle;
						ParticleB = ParticleB->NextUParticle;

						Exchange( TriPoints[0], TriPoints[1] );
						Exchange( TriPoints[3], TriPoints[2] );
					}

					ParticleA = ParticleC;
				}
			}
#endif
			Texture->Unlock( Info );
		}
	}

	STAT(unclock(GStat.ParticleDrawTime));
#endif //MERGEFIX
	return 0;

	unguard;
}

//------------------------------------------------------------------------------
void UTarpBuilderTemplate::Build()
{
	guard(UTarpBuilderTemplate::Build);
	NOTE(debugf(TEXT("(%s)UTarpBuilderTemplate::Build"), GetFullName() ));
#if 0 //MERGEFIX
	INT i,j;
	ATarpBuilder* Parent = CastChecked<ATarpBuilder>(GetOuter());

	UTarpParticle* ParticleA;
	UTarpParticle* ParticleB;
	UTarpParticle* ParticleC;
	UTarpParticle* Particle;

	FVector Start = Parent->Location + FVector(-Parent->Width/2.f,Parent->Height/2.f,0.f);
	FLOAT USize = Parent->Width / (FLOAT)Parent->Columns;
	FLOAT VSize = Parent->Height / (FLOAT)Parent->Rows;

	// We get all the particle ahead of time so we can initialize them in the correct order.
	// Otherwise they'd end up backwards since they are added to the ParticleList at the head 
	// rather than the end of the list.
	for( j=0; j<=Parent->Rows; j++ )
	{
		for( i=0; i<=Parent->Columns; i++ )
		{
			UTarpParticle* Particle = CastChecked<UTarpParticle>(GetParticle());
			Particle->NextUParticle = NULL;
			Particle->NextVParticle = NULL;
			NOTE(debugf(TEXT("Created: %s"), Particle->GetFullName() ));
		}
	}

	Particle = Cast<UTarpParticle>(ParticleList);
	UTarpParticle* LastUParticle=NULL;
	UTarpParticle* LastVParticle=NULL;

	FLOAT UInterval = 1.f / Parent->Columns;
	FLOAT VInterval = 1.f / Parent->Rows;
	
	FLOAT V=0.f;
	FVector P = Start;
	for( j=0; j<=Parent->Rows; j++, P.Y -= VSize )
	{			
		if( LastVParticle )
			LastVParticle->NextVParticle = Particle;

		LastVParticle = Particle;
		LastUParticle = NULL;

		FLOAT U=0.f;
		P.X = Start.X;			
		for( i=0; i<=Parent->Columns; i++, P.X += USize )
		{
			Particle->Location = P;
			Particle->UTex = U;
			Particle->VTex = V;

			U += UInterval;

			if( LastUParticle )
				LastUParticle->NextUParticle = Particle;

			LastUParticle = Particle;
			Particle = Cast<UTarpParticle>(Particle->NextParticle);
		}

		V += VInterval;
	}

	// Make sure we linked everything up correctly.
	NOTE
	(
		check(Particle==NULL);	// Make sure we added the correct amount.
		for( UTarpParticle* DebugVParticle=Cast<UTarpParticle>(ParticleList); DebugVParticle; DebugVParticle = DebugVParticle->NextVParticle )
		{
			for( UTarpParticle* DebugUParticle=DebugVParticle; DebugUParticle; DebugUParticle = DebugUParticle->NextUParticle )
			{
				debugf(TEXT("(%f %f %f)%s"), DebugUParticle->Location.X, DebugUParticle->Location.Y, DebugUParticle->Location.Z, DebugUParticle->GetFullName() );
			}
			debugf(TEXT(""));
		}
	)

#if 0 // Debug.
	UAnchorForce* Anchor = (UAnchorForce*)StaticConstructObject( UAnchorForce::StaticClass(), System );
	Anchor->Priority = 9;
	Anchor->SetParticle( ParticleList, System );
	System->AddForce( Anchor );
#endif
	// Attach structural supports.
	USpringForce* Spring;
	USlipForce*   Slip;
	ParticleA = Cast<UTarpParticle>(ParticleList);
	while( ParticleA->NextVParticle )
	{
		ParticleB = ParticleA->NextVParticle;
		ParticleC = ParticleB;

		Slip = CastChecked<USlipForce>(StaticConstructObject( USlipForce::StaticClass(), Parent ));
		Slip->SetEndpoints( ParticleA, ParticleB, 0.f );
		ParticleA->SlipForces.AddItem( Slip );
		ParticleB->SlipForces.AddItem( Slip );
		Parent->AddForce( Slip );

		UTarpParticle* LastParticleA = ParticleA;
		UTarpParticle* LastParticleB = ParticleB;

		ParticleA = ParticleA->NextUParticle;
		ParticleB = ParticleB->NextUParticle;

		while( ParticleA )
		{
			Slip = CastChecked<USlipForce>(StaticConstructObject( USlipForce::StaticClass(), Parent ));
			Slip->SetEndpoints( ParticleA, LastParticleB );
			ParticleA->SlipForces.AddItem( Slip );
			LastParticleB->SlipForces.AddItem( Slip );
			Parent->AddForce( Slip );

			Slip = CastChecked<USlipForce>(StaticConstructObject( USlipForce::StaticClass(), Parent ));
			Slip->SetEndpoints( ParticleB, LastParticleA );
			ParticleB->SlipForces.AddItem( Slip );
			LastParticleA->SlipForces.AddItem( Slip );
			Parent->AddForce( Slip );

			Slip = CastChecked<USlipForce>(StaticConstructObject( USlipForce::StaticClass(), Parent ));
			Slip->SetEndpoints( ParticleA, ParticleB );
			ParticleA->SlipForces.AddItem( Slip );
			ParticleB->SlipForces.AddItem( Slip );
			Parent->AddForce( Slip );

			Slip = CastChecked<USlipForce>(StaticConstructObject( USlipForce::StaticClass(), Parent ));
			Slip->SetEndpoints( ParticleA, LastParticleA );
			ParticleA->SlipForces.AddItem( Slip );
			LastParticleA->SlipForces.AddItem( Slip );
			Parent->AddForce( Slip );

			Slip = CastChecked<USlipForce>(StaticConstructObject( USlipForce::StaticClass(), Parent ));
			Slip->SetEndpoints( ParticleB, LastParticleB );
			ParticleB->SlipForces.AddItem( Slip );
			LastParticleB->SlipForces.AddItem( Slip );
			Parent->AddForce( Slip );

			LastParticleA = ParticleA;
			LastParticleB = ParticleB;

			ParticleA = ParticleA->NextUParticle;
			ParticleB = ParticleB->NextUParticle;
		}

		ParticleA = ParticleC;
	}

	// Attach flexible supports (U-across).
	for( ParticleA = Cast<UTarpParticle>(ParticleList); ParticleA; ParticleA = ParticleA->NextVParticle )
	{
		for
		(	Particle = ParticleA
		;	Particle->NextUParticle
		&&	Particle->NextUParticle->NextUParticle
		;	Particle = Particle->NextUParticle
		)
		{
			Spring = CastChecked<USpringForce>(StaticConstructObject( USpringForce::StaticClass(), Parent ));
			Spring->SetEndpoints( Particle, Particle->NextUParticle->NextUParticle );
			Spring->SetStiffnessRef( Parent->Rigidity );
			Parent->AddForce( Spring );
		}
	}

	// Attach flexible supports (V-down).
	for
	(	ParticleA = Cast<UTarpParticle>(ParticleList)
	;	ParticleA
	&&	ParticleA->NextVParticle
	&&	ParticleA->NextVParticle->NextVParticle
	;	ParticleA = ParticleA->NextVParticle
	)
	{
		for
		(	ParticleB = ParticleA
		,	ParticleC = ParticleA->NextVParticle->NextVParticle
		;	ParticleB && ParticleC
		;	ParticleB = ParticleB->NextUParticle
		,	ParticleC = ParticleC->NextUParticle
		)
		{
			Spring = CastChecked<USpringForce>(StaticConstructObject( USpringForce::StaticClass(), Parent ));
			Spring->SetEndpoints( ParticleB, ParticleC );
			Spring->SetStiffnessRef( Parent->Rigidity );
			Parent->AddForce( Spring );
		}
	}
#endif
	unguard;
}

//------------------------------------------------------------------------------
void UTarpBuilderTemplate::AttachTo( AActor* Other, INT ID )
{
	guard(UTarpBuilderTemplate::AttachTo);
	NOTE(debugf(TEXT("(%s)UTarpBuilderTemplate::AttachTo( %s, %d )"), GetFullName(), Other ? Other->GetFullName() : TEXT("None"), ID ));

/* Fix ARL: Finish ClothAchor implemention.

	if( Other->Mesh )
	{
		ATarpBuilder* Parent = CastChecked<ATarpBuilder>(GetOuter());

		Other->Mesh->ClothAnchors.Load();	// Make sure lazy-arrays are loaded.
		for( INT i=0; i<Other->Mesh->ClothAnchors.Num(); i++ )
		{
			FAnchorPoint& Data = Other->Mesh->ClothAnchors(i);
			if( Data.ClothID == ID )
			{
				// Find the correct particle.
				UTarpParticle* Particle = Cast<UTarpParticle>(ParticleList);
				for( INT j=0; j<Data.YClothIndex; j++ )
					if( Particle )
						Particle = Particle->NextVParticle;
				for( j=0; j<Data.XClothIndex; j++ )
					if( Particle )
						Particle = Particle->NextUParticle;

				// Attach the AnchorForce.
				if( Particle )
				{
					UMeshAnchor* Anchor = CastChecked<UMeshAnchor>(StaticConstructObject( UMeshAnchor::StaticClass(), Parent ));
					Anchor->RelativeActor	= Other;
					Anchor->ParticleA		= Particle;
					Anchor->VertexIndex		= Data.MeshVertIndex;
					Parent->AddForce( Anchor );
					NOTE(debugf(TEXT("Attaching %s to %s.  %d"), Anchor->GetFullName(), Particle->GetFullName(), Anchor->VertexIndex ));

					// Bump up the priority on the slip forces for this particle.
					for( INT j=0; j<Particle->SlipForces.Num(); j++ )
						Particle->SlipForces(j)->Priority = GetDefault<USlipForce>()->Priority+0.5f;
				}
			}
		}

		Parent->Forces.Sort();
	}

*/
	unguard;
}

IMPLEMENT_CLASS(UTarpBuilderTemplate);


