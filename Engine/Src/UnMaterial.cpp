/*=============================================================================
	UnMaterial.cpp: Unreal texture / material related classes.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Jack Porter
=============================================================================*/


#include "EnginePrivate.h"

/*----------------------------------------------------------------------------
	UMaterial.
----------------------------------------------------------------------------*/

UBOOL UMaterial::CheckCircularReferences( TArray<UMaterial*>& History )
{
	guard(UMaterial::CheckCircularReferences);
	if( History.FindItemIndex(this) != INDEX_NONE )
		return 0;
	if( FallbackMaterial )
	{
		INT NewItem = History.Add();
		History(NewItem) = this;
		if( !FallbackMaterial->CheckCircularReferences(History)	)
			return 0;
		History.Remove(NewItem);
	}
	return 1;
	unguard;
}

UMaterial::UMaterial()
{
}

void UMaterial::Serialize( FArchive& Ar )
{
	//!!vogel: material duplication code hack.
	if( !(GUglyHackFlags & 8) )
		Super::Serialize( Ar );

	if( Ar.LicenseeVer() > 0x00 )
	{
		BYTE TextureType;
		Ar << TextureType;
	}
}

UMaterial* UMaterial::CheckFallback()
{
	guard(UMaterial::CheckFallback);
	if( FallbackMaterial && UseFallback ) 
		return FallbackMaterial->CheckFallback();
	return this;
	unguard;
}

//
// Clear all fallback memories
//
void UMaterial::ClearFallbacks()
{
	guard(UMaterial::ClearFallbacks);
	for( TObjectIterator<UMaterial> It; It; ++It )
	{
		It->UseFallback = 0;
		It->Validated = 0;
	}
	unguard;
}

IMPLEMENT_CLASS(UMaterial);

/*----------------------------------------------------------------------------
	Misc material stuff
----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UBitmapMaterial);
IMPLEMENT_CLASS(UProxyBitmapMaterial);
IMPLEMENT_CLASS(URenderedMaterial);
IMPLEMENT_CLASS(UTexCoordMaterial);
IMPLEMENT_CLASS(UColorModifier);

/*----------------------------------------------------------------------------
	Shaders
----------------------------------------------------------------------------*/

//
// UShader
//
UBOOL UShader::CheckCircularReferences( TArray<UMaterial*>& History )
{
	guard(UShader::CheckCircularReferences);
	if( !Super::CheckCircularReferences(History))
		return 0;
	
	INT NewItem = History.Add();
	History(NewItem) = this;

	if(	( Diffuse				&& !Diffuse->CheckCircularReferences(History)				) ||
		( Opacity				&& !Opacity->CheckCircularReferences(History)				) ||
		( Specular				&& !Specular->CheckCircularReferences(History)				) ||
		( SpecularityMask		&& !SpecularityMask->CheckCircularReferences(History)		) ||
		( SelfIllumination		&& !SelfIllumination->CheckCircularReferences(History)		) ||
		( SelfIlluminationMask	&& !SelfIlluminationMask->CheckCircularReferences(History)	) ||
		( Detail				&& !Detail->CheckCircularReferences(History)				) )
		return 0;

	History.Remove(NewItem);
	return 1;
	unguard;
}

BYTE UShader::RequiredUVStreams()
{
	BYTE	UVStreamMask = 1;

	if(Diffuse)
		UVStreamMask |= Diffuse->RequiredUVStreams();

	if(Opacity)
		UVStreamMask |= Opacity->RequiredUVStreams();

	if(Specular)
		UVStreamMask |= Specular->RequiredUVStreams();

	if(SpecularityMask)
		UVStreamMask |= SpecularityMask->RequiredUVStreams();

	if(SelfIllumination)
		UVStreamMask |= SelfIllumination->RequiredUVStreams();

	if(SelfIlluminationMask)
		UVStreamMask |= SelfIlluminationMask->RequiredUVStreams();

	if(Detail)
		UVStreamMask |= Detail->RequiredUVStreams();

	return UVStreamMask;
}

UBOOL UShader::RequiresSorting()
{
	return 
		(OutputBlending == OB_Normal && Opacity)	||
		 OutputBlending == OB_Modulate				||
		 OutputBlending == OB_Brighten				||
		 OutputBlending == OB_Darken				||
		 OutputBlending == OB_Translucent;
}

UBOOL UShader::RequiresNormal()
{
	if(Diffuse && Diffuse->RequiresNormal())
		return 1;

	if(Opacity && Opacity->RequiresNormal())
		return 1;

	if(Specular && Specular->RequiresNormal())
		return 1;

	if(SpecularityMask && SpecularityMask->RequiresNormal())
		return 1;

	if(SelfIllumination && SelfIllumination->RequiresNormal())
		return 1;

	if(SelfIlluminationMask && SelfIlluminationMask->RequiresNormal())
		return 1;

	if(Detail && Detail->RequiresNormal())
		return 1;

	return 0;
}

UBOOL UShader::IsTransparent()
{
	return RequiresSorting();
}

INT UShader::MaterialUSize()
{
	if( Diffuse )
		return Diffuse->MaterialUSize();
	if( SelfIllumination )
		return SelfIllumination->MaterialUSize();
	return 0;
}

INT UShader::MaterialVSize()
{
	if( Diffuse )
		return Diffuse->MaterialVSize();
	if( SelfIllumination )
		return SelfIllumination->MaterialVSize();
	return 0;
}

void UShader::PostEditChange()
{
	guard(UShader::PostEditChange);
	Super::PostEditChange();

	// Mark the package as dirty
	UObject* Pkg = this;
	while( Pkg->GetOuter() )	Pkg = Pkg->GetOuter();
	if( Pkg && Pkg->IsA(UPackage::StaticClass() ) )
		((UPackage*)Pkg)->bDirty = 1;
	
	unguard;
}

// UShader automatically falls back to its diffuse if there's nothing better.
UBOOL UShader::HasFallback()
{
	return FallbackMaterial != NULL || Diffuse != NULL;
}

UMaterial* UShader::CheckFallback()
{
	guard(UShader::CheckFallback);
	if( UseFallback ) 
	{
		if( FallbackMaterial )
			return FallbackMaterial->CheckFallback();
		else
		if( Diffuse )
			return Diffuse->CheckFallback();
		else
			return NULL; // we should never get here.
	}
	return this;
	unguard;
}

void UShader::PreSetMaterial(FLOAT TimeSeconds)
{
	guard(UShader::PreSetMaterial);
	if(Diffuse)
		Diffuse->CheckFallback()->PreSetMaterial(TimeSeconds);
	if(Opacity)
		Opacity->CheckFallback()->PreSetMaterial(TimeSeconds);
	if(Specular)
		Specular->CheckFallback()->PreSetMaterial(TimeSeconds);
	if(SpecularityMask)
		SpecularityMask->CheckFallback()->PreSetMaterial(TimeSeconds);
	if(SelfIllumination)
		SelfIllumination->CheckFallback()->PreSetMaterial(TimeSeconds);
	if(SelfIlluminationMask)
		SelfIlluminationMask->CheckFallback()->PreSetMaterial(TimeSeconds);
	if(Detail)
		Detail->CheckFallback()->PreSetMaterial(TimeSeconds);
	unguard;
}

IMPLEMENT_CLASS(UShader);

//
// UCombiner
//
UBOOL UCombiner::CheckCircularReferences( TArray<UMaterial*>& History )
{
	guard(UCombiner::CheckCircularReferences);
	if( !Super::CheckCircularReferences(History))
		return 0;
	
	INT NewItem = History.Add();
	History(NewItem) = this;

	if(	( Material1	&& !Material1->CheckCircularReferences(History) ) ||
		( Material2	&& !Material2->CheckCircularReferences(History) ) ||
		( Mask		&& !Mask->CheckCircularReferences(History) ) )
		return 0;

	History.Remove(NewItem);
	return 1;
	unguard;
}

void UCombiner::PostEditChange()
{
	guard(UCombiner::PostEditChange);
	Super::PostEditChange();

	// Mark the package as dirty
	UObject* Pkg = this;
	while( Pkg->GetOuter() )	Pkg = Pkg->GetOuter();
	if( Pkg && Pkg->IsA(UPackage::StaticClass() ) )
		((UPackage*)Pkg)->bDirty = 1;
	
	unguard;
}

void UCombiner::PreSetMaterial(FLOAT TimeSeconds)
{
	guard(UCombiner::PreSetMaterial);
	if( Material1 )
		Material1->CheckFallback()->PreSetMaterial(TimeSeconds);
	if( Material2 )
		Material2->CheckFallback()->PreSetMaterial(TimeSeconds);	
	if( Mask && Mask!=Material1 && Mask!=Material2 )
		Mask->CheckFallback()->PreSetMaterial(TimeSeconds);
	unguard;
}

IMPLEMENT_CLASS(UCombiner);


/*----------------------------------------------------------------------------
	Constant Materials.
----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UConstantMaterial);
IMPLEMENT_CLASS(UConstantColor);


FColor UFadeColor::GetColor(FLOAT TimeSeconds)
{
	guard(UFadeColor::GetColor);

	FLOAT	Time = (TimeSeconds + FadePhase) / FadePeriod;

	if(ColorFadeType == FC_Sinusoidal)
	{
		FLOAT	Percent = 0.5f + appCos(Time * PI * 0.5f) * 0.5f;

		return FColor(Color1.Plane() * Percent + Color2.Plane() * (1.0f - Percent));
	}
	else if(ColorFadeType == FC_Linear)
	{
		INT		Cycle = appFloor(Time);
		FLOAT	Percent = (Cycle % 2) ? (Time - Cycle) : 1.0f - (Time - Cycle);

		return FColor(Color1.Plane() * Percent + Color2.Plane() * (1.0f - Percent));
	}
	else
		return Color1;

	unguard;
}

IMPLEMENT_CLASS(UFadeColor);


/*----------------------------------------------------------------------------
	Modifiers.
----------------------------------------------------------------------------*/

//
// UModifier
//
UBOOL UModifier::CheckCircularReferences( TArray<UMaterial*>& History )
{
	guard(UModifier::CheckCircularReferences);
	if( !Super::CheckCircularReferences(History))
		return 0;
	
	INT NewItem = History.Add();
	History(NewItem) = this;

	if( Material && !Material->CheckCircularReferences(History) )
		return 0;

	History.Remove(NewItem);
	return 1;
	unguard;
}

void UModifier::PostEditChange()
{
	guard(UModifier::PostEditChange);
	Super::PostEditChange();

	// Mark the package as dirty
	UObject* Pkg = this;
	while( Pkg->GetOuter() )	Pkg = Pkg->GetOuter();
	if( Pkg && Pkg->IsA(UPackage::StaticClass() ) )
		((UPackage*)Pkg)->bDirty = 1;
	
	unguard;
}

IMPLEMENT_CLASS(UModifier);

//
// UTexCoordModifier
//
INT UTexModifier::MaterialUSize() 
{
	INT USize = 0;
	if( Material )
		USize = Material->MaterialUSize();
	return USize;
}

INT UTexModifier::MaterialVSize() 
{
	INT VSize = 0;
	if( Material )
		VSize = Material->MaterialVSize();
	return VSize;
}

BYTE UTexModifier::RequiredUVStreams()
{
	if(TexCoordSource >= TCS_Stream0 && TexCoordSource <= TCS_Stream7)
		return (Material ? Material->RequiredUVStreams() : 1) | (1 << (TexCoordSource - TCS_Stream0));
	else
		return Material ? Material->RequiredUVStreams() : 1;
}

UBOOL UTexModifier::GetValidated()
{
	if( Material )
		return Material->GetValidated();
	return 1;
}

void UTexModifier::SetValidated( UBOOL InValidated )
{
	if( Material )
		Material->SetValidated(InValidated);
}

IMPLEMENT_CLASS(UTexModifier);

//
// UTexPannerTriggered
//
FMatrix* UTexPannerTriggered::GetMatrix(FLOAT TimeSeconds)
{
	guard(UTexPannerTriggered::GetMatrix);

	FLOAT FakeTime;
	if( TriggeredTime < 0.f )
		FakeTime = 0.f;
	else
	{
		FLOAT FakeTimeMax = StopAfterPeriod / PanRate;
		if( Reverse )
		{
			FakeTime = FakeTimeMax - (TimeSeconds - TriggeredTime);
			if( FakeTime < 0.f )
				FakeTime = 0.f;
		}
		else
		{
			FakeTime = (TimeSeconds - TriggeredTime);
			if( FakeTime > FakeTimeMax )
				FakeTime = FakeTimeMax;
		}
	}

	return UTexPanner::GetMatrix( GIsEditor ? TimeSeconds : FakeTime );

	unguard;
}
IMPLEMENT_CLASS(UTexPannerTriggered);

//
// UTexPanner
//
#define TEXPANNER_PANMAX 1024
FMatrix* UTexPanner::GetMatrix(FLOAT TimeSeconds)
{
	FVector D = PanDirection.Vector();
	FLOAT Su = TimeSeconds * PanRate * D.X / TEXPANNER_PANMAX;
	FLOAT Sv = TimeSeconds * PanRate * D.Y / TEXPANNER_PANMAX;
	FLOAT  du = (Su - appFloor(Su)) * TEXPANNER_PANMAX;
	FLOAT  dv = (Sv - appFloor(Sv)) * TEXPANNER_PANMAX;
	M = FMatrix (
					FPlane(1,0,0,0),
					FPlane(0,1,0,0),
					FPlane(du,dv,1,0),
					FPlane(0,0,0,1)
				);
	return &M; 
}
IMPLEMENT_CLASS(UTexPanner);

//
// UTexScaler
//
FMatrix* UTexScaler::GetMatrix(FLOAT TimeSeconds)
{
	M = FMatrix (
					FPlane(1.f/UScale,0,0,0),
					FPlane(0,1.f/VScale,0,0),
					FPlane(UOffset/MaterialUSize(),VOffset/MaterialVSize(),1,0),
					FPlane(0,0,0,1)
				);
	return &M; 
}
IMPLEMENT_CLASS(UTexScaler);

//
// UTexRotator
//
FMatrix* UTexRotator::GetMatrix(FLOAT TimeSeconds)
{
	switch( TexRotationType)
	{
	case TR_ConstantlyRotating:
		{
			FRotator R = Rotation * TimeSeconds;
			R.Pitch	&= 0xFFFF;
			R.Yaw	&= 0xFFFF;
			R.Roll	&= 0xFFFF;
			M = FRotationMatrix(R);
		}
		break;
	case TR_OscillatingRotation:
		{
			FLOAT S_Pitch = TimeSeconds * OscillationRate.Pitch / 0xFFFF;
			FLOAT d_Pitch = (FLOAT)OscillationAmplitude.Pitch * appSin( 2.f * PI * ((S_Pitch - appFloor(S_Pitch)) + OscillationPhase.Pitch) );
			FLOAT S_Yaw = TimeSeconds * OscillationRate.Yaw / 0xFFFF;
			FLOAT d_Yaw = (FLOAT)OscillationAmplitude.Yaw * appSin( 2.f * PI * ((S_Yaw - appFloor(S_Yaw)) + OscillationPhase.Yaw) );
			FLOAT S_Roll = TimeSeconds * OscillationRate.Roll / 0xFFFF;
			FLOAT d_Roll = (FLOAT)OscillationAmplitude.Roll * appSin( 2.f * PI * ((S_Roll - appFloor(S_Roll)) + OscillationPhase.Roll) );

			M = FRotationMatrix(Rotation + FRotator(d_Pitch, d_Yaw, d_Roll));
		}
		break;
	default:
		M = FRotationMatrix(Rotation);
		break;
	}
	
	if( UOffset != 0.f || VOffset != 0.f )
	{
		FLOAT du = UOffset/MaterialUSize();
		FLOAT dv = VOffset/MaterialVSize();
		
		M = FMatrix(
					FPlane(1,0,0,0),
					FPlane(0,1,0,0),
					FPlane(-du,-dv,1,0),
					FPlane(0,0,0,1)
			) *
			M *
			FMatrix(
					FPlane(1,0,0,0),
					FPlane(0,1,0,0),
					FPlane(du,dv,1,0),
					FPlane(0,0,0,1)
			);
	}

	return &M; 
}
IMPLEMENT_CLASS(UTexRotator);

//
// UTexOscillatorTriggered
//
FMatrix* UTexOscillatorTriggered::GetMatrix(FLOAT TimeSeconds)
{
	guard(UTexOscillatorTriggered::GetMatrix);

	FLOAT FakeTime;
	if( TriggeredTime < 0.f )
		FakeTime = 0.f;
	else
	{
		FLOAT FakeTimeMax = StopAfterPeriod / Max<FLOAT>( UOscillationRate, VOscillationRate );
		if( Reverse )
		{
			FakeTime = FakeTimeMax - (TimeSeconds - TriggeredTime);
			if( FakeTime < 0.f )
				FakeTime = 0.f;
		}
		else
		{
			FakeTime = (TimeSeconds - TriggeredTime);
			if( FakeTime > FakeTimeMax )
				FakeTime = FakeTimeMax;
		}
	}

	return UTexOscillator::GetMatrix( GIsEditor ? TimeSeconds : FakeTime );

	unguard;
}
IMPLEMENT_CLASS(UTexOscillatorTriggered);

//
// UTexOscillator
//
void UTexOscillator::PostEditChange()
{
	guard(UTexOscillator::PostEditChange);
	Super::PostEditChange();
	LastSu = LastSv = 0.f;
	unguard;
}
FMatrix* UTexOscillator::GetMatrix(FLOAT TimeSeconds)
{
	M = FMatrix::Identity;

	FLOAT Su = TimeSeconds * UOscillationRate;
	FLOAT Sv = TimeSeconds * VOscillationRate;

	switch( UOscillationType )
	{
	case OT_Jitter:
		{
			if( LastSu < 1.f || LastSu > Su + 1.f )
				LastSu = UOscillationPhase + appFloor(Su);
			if( Su-LastSu > 1.f )
			{
				CurrentUJitter = appFrand() * UOscillationAmplitude;
				LastSu = UOscillationPhase + appFloor(Su);
			}
			M.M[2][0] = CurrentUJitter;
		}
		break;
	case OT_Pan:
		{
			FLOAT  du = UOscillationAmplitude * appSin( 2.f * PI * (Su - appFloor(Su)) + 2.f * PI * UOscillationPhase );
			M.M[2][0] = du;
		}
		break;
	case OT_Stretch:
		{
			FLOAT  du = UOscillationAmplitude * appSin( 2.f * PI * (Su - appFloor(Su)) + 2.f * PI * UOscillationPhase );
			M.M[0][0] = 1.f+du;
		}
		break;
	case OT_StretchRepeat:
		{
			FLOAT  du = UOscillationAmplitude * appSin( 0.5f * PI * (Su - appFloor(Su)) + 2.f * PI * UOscillationPhase );
			M.M[0][0] = 1.f+du;
		}
		break;
	}

	switch( VOscillationType )
	{
	case OT_Jitter:
		{
			if( LastSv < 1.f || LastSu > Sv + 1.f )
				LastSv = VOscillationPhase + appFloor(Sv);
			if( Sv-LastSv > 1.f )
			{
				CurrentVJitter = appFrand() * VOscillationAmplitude;
				LastSv = VOscillationPhase + appFloor(Sv);
			}
			M.M[2][1] = CurrentVJitter;
		}
		break;
	case OT_Pan:
		{
			FLOAT  dv = VOscillationAmplitude * appSin( 2.f * PI * (Sv - appFloor(Sv)) + 2.f * PI * VOscillationPhase );
			M.M[2][1] = dv;
		}
		break;
	case OT_Stretch:
		{
			FLOAT  dv = VOscillationAmplitude * appSin( 2.f * PI * (Sv - appFloor(Sv)) + 2.f * PI * VOscillationPhase );
			M.M[1][1] = 1.f+dv;
		}
		break;
	case OT_StretchRepeat:
		{
			FLOAT  dv = VOscillationAmplitude * appSin( 0.5f * PI * (Sv - appFloor(Sv)) + 2.f * PI * VOscillationPhase );
			M.M[1][1] = 1.f+dv;
		}
		break;
	}

	if( UOffset != 0.f || VOffset != 0.f )
	{
		FLOAT du = UOffset/MaterialUSize();
		FLOAT dv = VOffset/MaterialVSize();
		
		M = FMatrix(
					FPlane(1,0,0,0),
					FPlane(0,1,0,0),
					FPlane(-du,-dv,1,0),
					FPlane(0,0,0,1)
			) *
			M *
			FMatrix(
					FPlane(1,0,0,0),
					FPlane(0,1,0,0),
					FPlane(du,dv,1,0),
					FPlane(0,0,0,1)
			);
	}
	
	return &M;
}
IMPLEMENT_CLASS(UTexOscillator);

//
// UTexEnvMap.
//
FMatrix* UTexEnvMap::GetMatrix(FLOAT TimeSeconds)
{
	switch(EnvMapType)
	{
	case EM_WorldSpace:
		TexCoordSource = TCS_WorldEnvMapCoords;
		break;
	case EM_CameraSpace:
		TexCoordSource = TCS_CameraEnvMapCoords;
		break;
	}

	return NULL;
}
IMPLEMENT_CLASS(UTexEnvMap);

//
// UTexMatrix
//
IMPLEMENT_CLASS(UTexMatrix);

//
// UTexCoordSource
//

void UTexCoordSource::PostEditChange()
{
	Super::PostEditChange();

	SourceChannel = Clamp(SourceChannel,0,7);
	TexCoordSource = (ETexCoordSource) (TCS_Stream0 + SourceChannel);
}

IMPLEMENT_CLASS(UTexCoordSource);

//
// UVertexColor
//

IMPLEMENT_CLASS(UVertexColor);

//
// UFinalBlend.
//

UBOOL UFinalBlend::RequiresSorting()
{
	return 
		FrameBufferBlending == FB_Modulate		||
		FrameBufferBlending == FB_AlphaBlend	||
		FrameBufferBlending == FB_AlphaModulate_MightNotFogCorrectly ||
		FrameBufferBlending == FB_Translucent	||
		FrameBufferBlending == FB_Darken		||
		FrameBufferBlending == FB_Brighten;
}

UBOOL UFinalBlend::IsTransparent()
{
	return RequiresSorting();
}

void UFinalBlend::PostEditChange()
{
	guard(UFinalBlend::PostEditChange);
	Super::PostEditChange();

	// Mark the package as dirty
	UObject* Pkg = this;
	while( Pkg->GetOuter() )	Pkg = Pkg->GetOuter();
	if( Pkg && Pkg->IsA(UPackage::StaticClass() ) )
		((UPackage*)Pkg)->bDirty = 1;
	
	unguard;
}

UBOOL UFinalBlend::GetValidated()
{
	if( Material )
		return Material->GetValidated();
	return 1;
}

void UFinalBlend::SetValidated( UBOOL InValidated )
{
	if( Material )
		Material->SetValidated(InValidated);
}
IMPLEMENT_CLASS(UFinalBlend);

//
// UMaterialSwitch
//
UBOOL UMaterialSwitch::CheckCircularReferences( TArray<UMaterial*>& History )
{
	guard(UMaterialSwitch::CheckCircularReferences);
	if( !Super::CheckCircularReferences(History))
		return 0;
	
	INT NewItem = History.Add();
	History(NewItem) = this;

	for( INT i=0;i<Materials.Num();i++ )
		if( Materials(i) && !Materials(i)->CheckCircularReferences(History) )
			return 0;

	History.Remove(NewItem);
	return 1;
	unguard;
}

void UMaterialSwitch::PostEditChange()
{
	guard(UMaterialSwitch::PostEditChange);
	Super::PostEditChange();
	Material = Current >=0 && Current < Materials.Num() ? Materials(Current) : NULL;
	unguard;
}

IMPLEMENT_CLASS(UMaterialSwitch)

//
// UMaterialSequence
//
void UMaterialSequence::PostEditChange()
{
	guard(UMaterialSequence::PostEditChange);
	Super::PostEditChange();
	CurrentTime = 0.f;
	LastTime = 0.f;
	TotalTime = 0.f;
	for( INT i=0;i<SequenceItems.Num();i++ )
		TotalTime += SequenceItems(i).Time;
	unguard;
}

void UMaterialSequence::Serialize(FArchive& Ar)
{
	guard(UMaterialSequence::Serialize);
	// Make sure we don't store a reference to our internal Combiner.
	if( Ar.IsSaving() )
		Material = NULL;
	Super::Serialize(Ar);
	unguard;
}

void UMaterialSequence::PreSetMaterial(FLOAT TimeSeconds)
{
	guard(UMaterialSequence::PreSetMaterial);
	if( !Paused )
	{
		// Level change.
		if( LastTime > TimeSeconds )
		{
			LastTime = 0.f; 
			CurrentTime = 0.f;
		}
		else
		if( LastTime > 0.f )
			CurrentTime += TimeSeconds-LastTime;
		LastTime = TimeSeconds;

		// Handle Looping
		if( Loop && TotalTime > 0.f && CurrentTime > TotalTime )
		{
			CurrentTime -= appFloor(CurrentTime/TotalTime) * TotalTime; 
			while( CurrentTime > TotalTime )
				CurrentTime -= TotalTime;
		}

		FLOAT CountTime = 0.f;
		for( INT i=0;i<SequenceItems.Num();i++ )
		{
			if( CurrentTime >= CountTime && (CurrentTime < CountTime + SequenceItems(i).Time || (!Loop && i==SequenceItems.Num()-1) ) )
			{
                switch( SequenceItems(i).Action )
				{
				case MSA_ShowMaterial:
					Material = SequenceItems(i).Material;
					break;
				case MSA_FadeToMaterial:
					if( CurrentTime >= CountTime + SequenceItems(i).Time )
						Material = SequenceItems(i).Material;
					else
					{
						DECLARE_STATIC_UOBJECT( UConstantColor, ConstantColor, {} );
						DECLARE_STATIC_UOBJECT( UCombiner, Combiner,
							{
								Combiner->Mask = ConstantColor;
								Combiner->CombineOperation = CO_AlphaBlend_With_Mask;
							} 
						);
						ConstantColor->Color = FColor(255,255,255,Clamp<INT>( appRound(255.f * ((CurrentTime-CountTime)/SequenceItems(i).Time)), 0, 255) );
						INT prev = i-1;
						if( prev < 0 )
							prev = SequenceItems.Num()-1;
						Combiner->Material1 = SequenceItems(prev).Material;
						Combiner->Material2 = SequenceItems(i).Material;
						Material = Combiner;
					}
					break;

				}
				break;
			}
			CountTime += SequenceItems(i).Time;
		}
	}
	Super::PreSetMaterial(TimeSeconds);
	unguard;
}

UBOOL UMaterialSequence::CheckCircularReferences( TArray<class UMaterial*>& History )
{
	guard(UMaterialSequence::CheckCircularReferences);
	if( !Super::CheckCircularReferences(History))
		return 0;
	
	INT NewItem = History.Add();
	History(NewItem) = this;

	for( INT i=0;i<SequenceItems.Num();i++ )
		if( SequenceItems(i).Material && !SequenceItems(i).Material->CheckCircularReferences(History) )
			return 0;

	History.Remove(NewItem);
	return 1;

	unguard;
}

IMPLEMENT_CLASS(UMaterialSequence);

//
// UOpacityModifier
//
IMPLEMENT_CLASS(UOpacityModifier);


/*----------------------------------------------------------------------------
	ConvertPolyFlagsToMaterial
----------------------------------------------------------------------------*/

//!! OLDVER
UMaterial* UMaterial::ConvertPolyFlagsToMaterial( UMaterial* InMaterial, DWORD InPolyFlags )
{
	guard(UMaterial::ConvertPolyFlagsToMaterial);
	
	// Create material for shiny environment mapping
	UTexture* Texture;
	if( (Texture=Cast<UTexture>(InMaterial)) != NULL && Texture->OLDEnvironmentMap )
	{
		FString NewName = FString::Printf(TEXT("%s_%sShiny"), Texture->GetName(), (InPolyFlags&PF_AlphaTexture) ? TEXT("Const") : TEXT("") );
		FString NewFinalName = NewName;

		if( InPolyFlags&PF_AlphaTexture )
			NewFinalName += TEXT("_Alpha");

		UShader* NewMaterial = FindObject<UShader>( ANY_PACKAGE, *NewFinalName );
		if( !NewMaterial )
		{
			FString TexCoordName = FString::Printf(TEXT("%s_EnvMapCoords"), Texture->OLDEnvironmentMap->GetName() );
			UTexEnvMap* EnvMapTex = FindObject<UTexEnvMap>( ANY_PACKAGE, *TexCoordName );
			if( !EnvMapTex )
			{
				EnvMapTex = ConstructObject<UTexEnvMap>( UTexEnvMap::StaticClass(), Texture->OLDEnvironmentMap->GetOuter(), FName(*TexCoordName), RF_Public|RF_Standalone );
				switch( Texture->OLDEnvMapTransformType )
				{
				case EMTT_ViewSpace:	EnvMapTex->EnvMapType = EM_CameraSpace; break;
				case EMTT_WorldSpace:	EnvMapTex->EnvMapType = EM_WorldSpace; break;
				}
				EnvMapTex->Material = Texture->OLDEnvironmentMap;
			}

			NewMaterial = ConstructObject<UShader>( UShader::StaticClass(), InMaterial->GetOuter(), FName(*NewName), RF_Public|RF_Standalone );	
			NewMaterial->Diffuse = Texture;
			NewMaterial->Specular = EnvMapTex;

			if( InPolyFlags&PF_AlphaTexture )
			{
				NewMaterial->Opacity = Texture;
				//!!MAT
				//NewMaterial->SpecularityMask = FColor(0,0,0,255*Texture->OLDSpecular);
			}
			else
				NewMaterial->SpecularityMask = Texture;
		}

		return NewMaterial;
	}

	// Create material for alpha texture
	//!!MAT
	//!! should unlit be handled this way?
	if( InPolyFlags&PF_Unlit )
	{
		FString NewName = FString::Printf(TEXT("%s_Unlit%s"), InMaterial->GetName(), InPolyFlags&PF_TwoSided?TEXT("TwoSided"):TEXT(""));
		UShader* NewMaterial = FindObject<UShader>( ANY_PACKAGE, *NewName );
		if( !NewMaterial )
		{
			NewMaterial = ConstructObject<UShader>( UShader::StaticClass(), InMaterial->GetOuter(), FName(*NewName), RF_Public|RF_Standalone );
			NewMaterial->TwoSided = InPolyFlags&PF_TwoSided?1:0;
			NewMaterial->SelfIllumination = InMaterial;
		}
		return NewMaterial;
	}

	// Create material for alpha texture
	if( InPolyFlags&PF_AlphaTexture )
	{
		FString NewName = FString::Printf(TEXT("%s_Alpha%s"), InMaterial->GetName(), InPolyFlags&PF_TwoSided?TEXT("TwoSided"):TEXT(""));
		UShader* NewMaterial = FindObject<UShader>( ANY_PACKAGE, *NewName );
		if( !NewMaterial )
		{
			NewMaterial = ConstructObject<UShader>( UShader::StaticClass(), InMaterial->GetOuter(), FName(*NewName), RF_Public|RF_Standalone );
			NewMaterial->TwoSided = InPolyFlags&PF_TwoSided?1:0;
			NewMaterial->Diffuse = InMaterial;
			NewMaterial->Opacity = InMaterial;
		}
		return NewMaterial;
	}

	// Create material for masked texture
	if( InPolyFlags&PF_Masked )
	{
		FString NewName = FString::Printf(TEXT("%s_Masked%s"), InMaterial->GetName(), InPolyFlags&PF_TwoSided?TEXT("TwoSided"):TEXT(""));
		UShader* NewMaterial = FindObject<UShader>( ANY_PACKAGE, *NewName );
		if( !NewMaterial )
		{
			NewMaterial = ConstructObject<UShader>( UShader::StaticClass(), InMaterial->GetOuter(), FName(*NewName), RF_Public|RF_Standalone );
			NewMaterial->TwoSided = InPolyFlags&PF_TwoSided?1:0;
			NewMaterial->Diffuse = InMaterial;
			NewMaterial->Opacity = InMaterial;
			NewMaterial->OutputBlending = OB_Masked;
		}
		return NewMaterial;
	}

	// Create material for modulated texture
	if( InPolyFlags&PF_Modulated )
	{
		FString NewName = FString::Printf(TEXT("%s_Modulated%s"), InMaterial->GetName(), InPolyFlags&PF_TwoSided?TEXT("TwoSided"):TEXT(""));
		UShader* NewMaterial = FindObject<UShader>( ANY_PACKAGE, *NewName );
		if( !NewMaterial )
		{
			NewMaterial = ConstructObject<UShader>( UShader::StaticClass(), InMaterial->GetOuter(), FName(*NewName), RF_Public|RF_Standalone );
			NewMaterial->TwoSided = InPolyFlags&PF_TwoSided?1:0;
			NewMaterial->Diffuse = InMaterial;
			NewMaterial->OutputBlending = OB_Modulate;
		}
		return NewMaterial;
	}

	// Create material for translucent texture
	if( InPolyFlags&PF_Translucent )
	{
		FString NewName = FString::Printf(TEXT("%s_Translucent%s"), InMaterial->GetName(), InPolyFlags&PF_TwoSided?TEXT("TwoSided"):TEXT(""));
		UShader* NewMaterial = FindObject<UShader>( ANY_PACKAGE, *NewName );
		if( !NewMaterial )
		{
			NewMaterial = ConstructObject<UShader>( UShader::StaticClass(), InMaterial->GetOuter(), FName(*NewName), RF_Public|RF_Standalone );
			NewMaterial->TwoSided = InPolyFlags&PF_TwoSided?1:0;
			NewMaterial->Diffuse = InMaterial;
			NewMaterial->OutputBlending = OB_Translucent;
		}
		return NewMaterial;
	}

	// Create material for double-sided texture
	if( InPolyFlags&PF_TwoSided )
	{
		FString NewName = FString::Printf(TEXT("%s_TwoTwoSided"), InMaterial->GetName());
		UShader* NewMaterial = FindObject<UShader>( ANY_PACKAGE, *NewName );
		if( !NewMaterial )
		{
			NewMaterial = ConstructObject<UShader>( UShader::StaticClass(), InMaterial->GetOuter(), FName(*NewName), RF_Public|RF_Standalone );
			NewMaterial->TwoSided = InPolyFlags&PF_TwoSided?1:0;
			NewMaterial->Diffuse = InMaterial;
		}
		return NewMaterial;
	}

	return InMaterial;
	unguard;
}

IMPLEMENT_CLASS(UHardwareShader)

UHardwareShader::UHardwareShader()
{
	for(INT i = 0; i < MAX_PASSES; i++)
	{
		VertexShaders[i] = new(TEXT("FVertexShader")) FVertexShader(this, i);
		PixelShaders[i] = new(TEXT("FPixelShader")) FPixelShader(this, i);
	}
}

void UHardwareShader::PostLoad()
{
	Super::PostLoad();
}

void UHardwareShader::Destroy()
{
	for(INT i = 0; i < MAX_PASSES; i++)
	{
		delete VertexShaders[i];
		delete PixelShaders[i];
		VertexShaders[i] = NULL;
		PixelShaders[i] = NULL;
	}
	Super::Destroy();
}

void UHardwareShader::Changed()
{
	for(INT i = 0; i < MAX_PASSES; i++)
	{
		VertexShaders[i]->IncRevision();
		PixelShaders[i]->IncRevision();
	}
}

void UHardwareShader::PostEditChange()
{
	Changed();
}

void UHardwareShader::InitDuplicate()
{
	for(INT i = 0; i < MAX_PASSES; i++)
	{
		VertexShaders[i] = new(TEXT("FVertexShader")) FVertexShader(this, i);
		PixelShaders[i] = new(TEXT("FPixelShader")) FPixelShader(this, i);
	}
}

//
// UTextureArray
//

void UTextureArray::PostEditChange()
{
	guard(UTextureArray::PostEditChange);
	Super::PostEditChange();
	if (bRebuild) Rebuild();
	unguard;
}

void UTextureArray::Rebuild()
{
	guard(UTextureArray::Rebuild);

	bRebuild=false;

	INT	ElementU=0,
		ElementV=0;

	// Copy valid textures into a new array.
	TArray<UTexture*> ValidTextures;
	for( INT i=0; i<SourceTextures.Num(); i++ )
	{
		UTexture* Texture = SourceTextures(i);
		if( Texture )
		{
			if(!ElementU) ElementU=Texture->USize;
			if(!ElementV) ElementV=Texture->VSize;

			if(Texture->USize!=ElementU)
				debugf(NAME_Warning,TEXT("UTextureArray::Rebuild(): Invalid USize.  %d instead of %d for %s"),Texture->USize,ElementU,Texture->GetFullName());
			else if(Texture->VSize!=ElementV)
				debugf(NAME_Warning,TEXT("VTextureArray::Rebuild(): Invalid VSize.  %d instead of %d for %s"),Texture->VSize,ElementV,Texture->GetFullName());
			else if(Texture->Format!=TEXF_RGBA8)
				debugf(NAME_Warning,TEXT("VTextureArray::Rebuild(): Invalid Format. '%s' instead of 'TEXF_RGBA8' for %s"),GetEnumEx(Engine.ETextureFormat,Texture->Format,0),Texture->GetFullName());
			else
				ValidTextures.AddItem( Texture );
		}
	}

	if( !ValidTextures.Num() )
		return;

	CalcBestDimensions( ValidTextures.Num(), N, M );

	Format = TEXF_RGBA8;
	Init(N*ElementU,M*ElementV);

	INT iTexture=0;
	for( INT m=0; m<M; m++ )
	{
		for( INT n=0; n<N; n++ )
		{
			if(iTexture==ValidTextures.Num())
				break;
			UTexture* Texture = ValidTextures(iTexture);
			iTexture++;
			if(!Texture)
				break;

			FColor* Src = (FColor*) &Texture->Mips(0).DataArray(0);
			FColor*	Dest = (FColor*) &this->Mips(0).DataArray(0) + (n*ElementU) + (m*ElementV*this->USize);

			for( INT V=0; V<ElementV; V++ )
			{
				for( INT U=0; U<ElementU; U++ )
				{
					Dest[U] = Src[U];
				}

				Src += Texture->USize;
				Dest += this->USize;
			}
		}
	}

	unguard;
}

FRectangle UTextureArray::GetElementUV( INT i )
{
	guard(UTextureArray::GetElementUV);

	i = i%(N*M);

	INT m = ((FLOAT)i)/N;	//row
	INT n = i-(m*N);		//column

	//debugf(TEXT("UTextureArray::GetElementUV i=%d n=%d m=%d N=%d M=%d"),i,n,m,N,M);

	return FRectangle( ((FLOAT)n)/N, ((FLOAT)m)/M, 1.f/N, 1.f/M );	//normalized
	unguard;
}

INT UTextureArray::CalcBestDimensions( INT x, INT &OutN, INT &OutM )
{
	guard(UTextureArray::CalcBestDimensions);
	INT min_waste=MAXINT;
	for(INT n=FNextPowerOfTwo(x/2); n>0; n/=2)
	{
		INT m = FNextPowerOfTwo(appCeil(((float)x)/n));
		INT waste = n + m + (n * m) - x;
		if(waste<min_waste)
		{
			OutN = n;
			OutM = m;
			min_waste = waste;
		}
	}
	//debugf(TEXT("CalcBestDimensions for %d using %dx%d with %d waste."),x,OutN,OutM,min_waste);
	return min_waste;
	unguard;
}

INT UTextureArray::GetUSize(){ return USize/N; }
INT UTextureArray::GetVSize(){ return VSize/M; }

void UTextureArray::Serialize( FArchive &Ar )
{
	guard(UTextureArray::Serialize);
	Super::Serialize(Ar);
/*!!MERGE
	if( Ar.LicenseeVer()<LVERSION_LEGACY_TEXTUREARRAY_SUPPORT )
	{
		TMap<UTexture*,INT> ReferenceTable;
		Ar << ReferenceTable;
	}
*/
	unguard;
}

IMPLEMENT_CLASS(UTextureArray);

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
