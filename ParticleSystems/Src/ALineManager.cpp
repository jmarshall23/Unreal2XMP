#include "ParticleSystems.h"
#include "FLineManager.h"

//------------------------------------------------------------------------------
void ALineManager::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI )
{
	guard(ALineManager::Render);
	NOTE(debugf(TEXT("ALineManager::Render")));

	if( ParticleTemplates.Num()==0 )
		AddTemplate( CastChecked<ULineManagerTemplate>(StaticConstructObject( ULineManagerTemplate::StaticClass(), this )) );

	Super::Render(Owner,Frame,Lights,RI);

	unguard;
}

//------------------------------------------------------------------------------
UParticle* ALineManager::AddLine
(	FVector	Start
,	FVector	End
,	FVector	Color
,	UBOOL	bDirected
,	FLOAT	ArrowOffset
,	FLOAT	ArrowWidth
,	FLOAT	ArrowHeight
,	FVector	ArrowColor
)
{
	guard(ALineManager::AddLine);
	NOTE(debugf(TEXT("ALineManager::AddLine")));

	FLineManager::GetInstance()->AddLine(0,Start,End,Color);
	return NULL;

	ULineManagerTemplate* Template = CastChecked<ULineManagerTemplate>(ParticleTemplates(0));
	ULineParticle* Line = ParticleCastChecked<ULineParticle>(Template->GetParticle());

	Line->Location		= Start;
	Line->EndPoint		= End;
	Line->Color			= FPlane(Color.X,Color.Y,Color.Z,0.f);
	Line->bDirected		= bDirected;
	Line->ArrowOffset	= ArrowOffset;
	Line->ArrowWidth	= ArrowWidth;
	Line->ArrowHeight	= ArrowHeight;
	Line->ArrowColor	= FPlane(ArrowColor.X,ArrowColor.Y,ArrowColor.Z,0.f);

	return Line;

	unguard;
}

//------------------------------------------------------------------------------
void ALineManager::execAddLinePS( FFrame& Stack, RESULT_DECL )
{
	guard(ALineManager::execAddLinePS);
	NOTE(debugf(TEXT("ALineManager::execAddLinePS")));

	P_GET_VECTOR(Start);
	P_GET_VECTOR(End);
	P_GET_VECTOR_OPTX(Color,FVector(0,1,0));
	P_GET_UBOOL_OPTX(bDirected,false);
	P_GET_FLOAT_OPTX(ArrowOffset,64.f);
	P_GET_FLOAT_OPTX(ArrowWidth,2.f);
	P_GET_FLOAT_OPTX(ArrowHeight,8.f);
	P_GET_VECTOR_OPTX(ArrowColor,FVector(0,1,0));

	P_FINISH;

	AddLine( Start, End, Color, bDirected, ArrowOffset, ArrowWidth, ArrowHeight, ArrowColor );
	
	unguard;
}

IMPLEMENT_CLASS(ALineManager);

