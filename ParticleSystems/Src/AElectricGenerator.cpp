#include "ParticleSystems.h"
//!!MERGE #include "UnLegendMesh.h"

#define RADIATOR_ERROR(err) RETURN;
//#define RADIATOR_ERROR(err) { debugf(TEXT("(%s) %s: %s"), GetFullName(), Mesh ? Mesh->GetFullName() : TEXT("NULL"), TEXT(err)); return; }

//!!MERGE static FVector GolemToUnreal( const FVector &V ){ return FVector(V.Z,-V.X,V.Y); }
//static FVector UnrealToGolem( const FVector &V ){ return FVector(-V.Y,V.Z,V.X); }

//------------------------------------------------------------------------------
INT AElectricGenerator::GetFacingTriIndex( FVector PointToFace )
{
	guard(AElectricGenerator::GetFacingTriIndex);
	NOTE(debugf(TEXT("AElectricGenerator::GetFacingTriIndex")));

#define RETURN return iTri;

	INT iTri = -1;
/*!!MERGE
	AActor* Owner = Source ? Source : this;

	ULegendMesh* Mesh = Cast<ULegendMesh>(Owner->Mesh);
	if(!Mesh) RADIATOR_ERROR("Mesh not set");
	
	ULegendMeshInstance* MeshInstance = CastChecked<ULegendMeshInstance>(Mesh->MeshGetInstance(Owner));
	if(!MeshInstance) RADIATOR_ERROR("MeshInstance not set");

	GLM::REntEntity* Entity = MeshInstance->GetEntity();
	if(!Entity) RADIATOR_ERROR("Entity not set");

	FLOAT MeshVertLOD = 0.f;	// use the full version.
	GLM::IEntModelLevel* ModelLevel = Entity->EntityGetModel()->ModelGetLevelForDetail(MeshVertLOD);
	if(!ModelLevel) RADIATOR_ERROR("ModelLevel not present");

	GLM::IEntModelTris* ModelTris = ModelLevel->LevelGetTris();
	if(!ModelTris) RADIATOR_ERROR("ModelTris don't exist");

	FCoords MeshCoords = GMath.UnitCoords * (Owner->Location + Owner->PrePivot) * Owner->Rotation * FScale(Owner->DrawScale3D * Owner->DrawScale, 0.f, SHEER_None);

	FMemMark Mark(GMem);
	FVector* Points = New<FVector>(GMem,Entity->EntityGetPointCount());

	GLM::SEntModelVertex* ModelVerts = Entity->EntityGetModel()->ModelGetVertices();

	GLM::CEntVertexEval Eval;
	Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, 0, Entity->EntityGetPointCount() );
	Entity->EntityVertexEval(Eval);

	for( INT iAttempts=0; iAttempts<64; iAttempts++ )
	{
		INT i = appRand() % ModelTris->TrisGetTriangleCount();
		GLM::SEntModelTrisTri* Tri = &ModelTris->TrisGetTriangles()[i];

		Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, ModelVerts[ Tri->mVerts[0] ].mPointIndex, 1 );
		Entity->EntityVertexEval(Eval);
		Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, ModelVerts[ Tri->mVerts[1] ].mPointIndex, 1 );
		Entity->EntityVertexEval(Eval);
		Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, ModelVerts[ Tri->mVerts[2] ].mPointIndex, 1 );
		Entity->EntityVertexEval(Eval);

		FVector A = Points[ ModelVerts[ Tri->mVerts[0] ].mPointIndex ];
		FVector B = Points[ ModelVerts[ Tri->mVerts[1] ].mPointIndex ];
		FVector C = Points[ ModelVerts[ Tri->mVerts[2] ].mPointIndex ];

		A = GolemToUnreal(A).TransformPointBy(MeshCoords);
		B = GolemToUnreal(B).TransformPointBy(MeshCoords);
		C = GolemToUnreal(C).TransformPointBy(MeshCoords);

		FVector Center = (A+B+C)/3.f;
		FVector Normal = ((A-B)^(C-A)).SafeNormal();

		// See if we face the given point.
		if( (Normal | (PointToFace - Center).SafeNormal()) > 0.9f )
		{
			iTri = i;
			break;
		}
	}
*/
	return iTri;

#undef RETURN

	unguard;
}

//------------------------------------------------------------------------------
FVector AElectricGenerator::GetTriLocation( INT TriangleIndex )
{
	guard(AElectricGenerator::GetTriLocation);
	NOTE(debugf(TEXT("AElectricGenerator::GetTriLocation")));

#define RETURN return TriCenter;

	FVector TriCenter=FVector(0,0,0);
/*!!MERGE
	AActor* Owner = Source ? Source : this;

	ULegendMesh* Mesh = Cast<ULegendMesh>(Owner->Mesh);
	if(!Mesh) RADIATOR_ERROR("Mesh not set");
	
	ULegendMeshInstance* MeshInstance = CastChecked<ULegendMeshInstance>(Mesh->MeshGetInstance(Owner));
	if(!MeshInstance) RADIATOR_ERROR("MeshInstance not set");

	GLM::REntEntity* Entity = MeshInstance->GetEntity();
	if(!Entity) RADIATOR_ERROR("Entity not set");

	FLOAT MeshVertLOD = 0.f;	// use the full version.
	GLM::IEntModelLevel* ModelLevel = Entity->EntityGetModel()->ModelGetLevelForDetail(MeshVertLOD);
	if(!ModelLevel) RADIATOR_ERROR("ModelLevel not present");

	GLM::IEntModelTris* ModelTris = ModelLevel->LevelGetTris();
	if(!ModelTris) RADIATOR_ERROR("ModelTris don't exist");

	FCoords MeshCoords = GMath.UnitCoords * (Owner->Location + Owner->PrePivot) * Owner->Rotation * FScale(Owner->DrawScale3D * Owner->DrawScale, 0.f, SHEER_None);

	FMemMark Mark(GMem);
	FVector* Points = New<FVector>(GMem,Entity->EntityGetPointCount());

	GLM::SEntModelVertex* ModelVerts = Entity->EntityGetModel()->ModelGetVertices();

	GLM::CEntVertexEval Eval;
	Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, 0, Entity->EntityGetPointCount() );
	Entity->EntityVertexEval(Eval);

	GLM::SEntModelTrisTri* Tri = &ModelTris->TrisGetTriangles()[TriangleIndex];

	Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, ModelVerts[ Tri->mVerts[0] ].mPointIndex, 1 );
	Entity->EntityVertexEval(Eval);
	Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, ModelVerts[ Tri->mVerts[1] ].mPointIndex, 1 );
	Entity->EntityVertexEval(Eval);
	Eval.mPoints.Init( GLM::ENTVEBF_Evaluate, (GLM::VVec3*)Points, NULL, ModelVerts[ Tri->mVerts[2] ].mPointIndex, 1 );
	Entity->EntityVertexEval(Eval);

	FVector A = Points[ ModelVerts[ Tri->mVerts[0] ].mPointIndex ];
	FVector B = Points[ ModelVerts[ Tri->mVerts[1] ].mPointIndex ];
	FVector C = Points[ ModelVerts[ Tri->mVerts[2] ].mPointIndex ];

	A = GolemToUnreal(A).TransformPointBy(MeshCoords);
	B = GolemToUnreal(B).TransformPointBy(MeshCoords);
	C = GolemToUnreal(C).TransformPointBy(MeshCoords);

	TriCenter = (A+B+C)/3.f;
*/
	return TriCenter;

#undef RETURN

	unguard;
}

//------------------------------------------------------------------------------
void AElectricGenerator::execGetFacingTriIndex( FFrame& Stack, RESULT_DECL )
{
	guard(AElectricGenerator::execGetFacingTriIndex);
	NOTE(debugf(TEXT("AElectricGenerator::execGetFacingTriIndex")));

	P_GET_VECTOR(PointToFace);
	P_FINISH;

//!!MERGE	*(INT*)Result = GetFacingTriIndex( PointToFace );
	
	unguard;
}

//------------------------------------------------------------------------------
void AElectricGenerator::execGetTriLocation( FFrame& Stack, RESULT_DECL )
{
	guard(AElectricGenerator::execGetTriLocation);
	NOTE(debugf(TEXT("AElectricGenerator::execGetTriLocation")));

	P_GET_INT(TriangleIndex);
	P_FINISH;

//!!MERGE	*(FVector*)Result = GetTriLocation( TriangleIndex );
	
	unguard;
}

//------------------------------------------------------------------------------
void AElectricGenerator::Spawned()
{
	guard(AElectricGenerator::Spawned);
	NOTE(debugf(TEXT("AElectricGenerator::Spawned")));

	Super::Spawned();

	// Copy over default values.
	ABendiBeamGenerator* Defaults = CastChecked<ABendiBeamGenerator>(BeamType->GetDefaultActor());
	BeamTexture				= Defaults->BeamTexture;
	BeamSegLength			= Defaults->BeamSegLength;
	BeamColor				= Defaults->BeamColor;
	BeamWidth				= Defaults->BeamWidth;
	BeamTextureWidth.A		= Defaults->BeamTextureWidth;
	BeamTextureWidth.B		= Defaults->BeamTextureWidth;
	NumBeams				= Defaults->NumBeams;
	SpriteJointTexture		= Defaults->SpriteJointTexture;
	SpriteJointSize			= Defaults->SpriteJointSize;
	SpriteJointColor		= Defaults->SpriteJointColor;
	MetaBeamSegLength		= Defaults->MetaBeamSegLength;
	MetaBeamWidth			= Defaults->MetaBeamWidth;
	MetaBeamRefreshTime		= Defaults->MetaBeamRefreshTime;

	unguard;
}


IMPLEMENT_CLASS(AElectricGenerator);

