/*=============================================================================
	UnScript.cpp: UnrealScript engine support code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Description:
	UnrealScript execution and support code.

Revision history:
	* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"
#include "UnNet.h"
#include "UnMovie.h" // Added by Demiurge Studios (Movie)

#include "xForceFeedback.h" 

#if 1 //NEW (mdf) line rendering
#include "FLineManager.h"
#define DEFAULTSCRIPTLINECOLOR FColor(255,0,0)
#endif

void AActor::execDebugClock( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::execDebugClock);
	P_FINISH;
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptDebugTime));
	unguard;
}

void AActor::execDebugUnclock( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::execDebugUnclock);
	P_FINISH;
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptDebugTime));
	unguard;
}

// 
// Generalize animation retrieval to work for both skeletal meshes (animation sits in Actor->SkelAnim->AnimSeqs) and
// classic meshes (Mesh->AnimSeqs) For backwards compatibility....
//


/*-----------------------------------------------------------------------------
	Tim's physics modes.
-----------------------------------------------------------------------------*/

/* unused - vogel
static FLOAT Splerp( FLOAT F )
{
	FLOAT S = Square(F);
	return (1.0f/16.0f)*S*S - (1.0f/2.0f)*S + 1;
}
*/
//
// Moving brush.
//
void AMover::physMovingBrush( FLOAT DeltaTime )
{
	guard(AMover::physMovingBrush);

	FVector OldLocation = Location;

	INT NewKeyNum     = Clamp( (INT)KeyNum, (INT)0, (INT)ARRAY_COUNT(KeyPos) );
	FLOAT TotalTime = DeltaTime;

	if( bInterpolating )
	{
		// If we are moving, turn off all attached anti-portals

		for (INT AP=0;AP<AntiPortals.Num();AP++)
		{
			if(AntiPortals(AP)->DrawType != DT_None)
			{
				AntiPortals(AP)->SetDrawType(DT_None);
				AntiPortals(AP)->ClearRenderData();
			}
		}
	}

	while( bInterpolating && DeltaTime>0.0f )
	{
		// We are moving.
		FLOAT NewAlpha = PhysAlpha + DeltaTime * PhysRate;
		if( NewAlpha > 1.0f )
		{
			DeltaTime *= (NewAlpha - 1.0f) / (NewAlpha - PhysAlpha);
			NewAlpha   = 1.0f;
		}
		else DeltaTime = 0.0f;

		// Compute alpha.
		FLOAT RenderAlpha;
		if( MoverGlideType == MV_GlideByTime )
		{
			// Make alpha time-smooth and time-continuous.
			// f(0)=0, f(1)=1, f'(0)=f'(1)=0.
			RenderAlpha = 3.0f*NewAlpha*NewAlpha - 2.0f*NewAlpha*NewAlpha*NewAlpha;
		}
		else RenderAlpha = NewAlpha;

		// Move.
		FCheckResult Hit(1.0f);
		if( GetLevel()->MoveActor
		(
			this,
			OldPos + ((BasePos + KeyPos[NewKeyNum]) - OldPos) * RenderAlpha - Location,
			OldRot + ((BaseRot + KeyRot[NewKeyNum]) - OldRot) * RenderAlpha,
			Hit
		) )
		{
			// Successfully moved.
			PhysAlpha = NewAlpha;
			if( NewAlpha == 1.0f )
			{
				// Just finished moving.
				bInterpolating = 0;
				eventKeyFrameReached();

				if (NewKeyNum==0)
				{
					// If we have stopped at the intial frame, flag it

					for (INT AP=0;AP<AntiPortals.Num();AP++)
					{
						if(AntiPortals(AP)->DrawType != DT_AntiPortal)
						{
							AntiPortals(AP)->SetDrawType(DT_AntiPortal);
							AntiPortals(AP)->ClearRenderData();
						}
					}
				}
			}
		}
	}
	Velocity = (Location - OldLocation)/TotalTime;
	unguard;
}

//
// Initialize execution.
//
void AActor::InitExecution()
{
	guard(AActor::InitExecution);

	UObject::InitExecution();

	check(GetStateFrame());
	check(GetStateFrame()->Object==this);
	check(GetLevel()!=NULL);
	check(GetLevel()->Actors(0)!=NULL);
	check(GetLevel()->Actors(0)==Level);
	check(Level!=NULL);

	unguardobj;
}

/*-----------------------------------------------------------------------------
	Natives.
-----------------------------------------------------------------------------*/

//////////////////////
// Console Commands //
//////////////////////

void AActor::execConsoleCommand( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execConsoleCommand);

	P_GET_STR(Command);
	P_FINISH;

	FStringOutputDevice StrOut;
	GetLevel()->Engine->Exec( *Command, StrOut );
	*(FString*)Result = *StrOut;

	unguard;
}

/////////////////////////////
// Log and error functions //
/////////////////////////////

void AActor::execError( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execError);

	P_GET_STR(S);
	P_FINISH;

	Stack.Log( *S );
	GetLevel()->DestroyActor( this );

	unguardexecSlow;
}

//////////////////////////
// Clientside functions //
//////////////////////////

void APlayerController::execClientTravel( FFrame& Stack, RESULT_DECL )
{
	guardSlow(APlayerController::execClientTravel);

	P_GET_STR(URL);
	P_GET_BYTE(TravelType);
	P_GET_UBOOL(bItems);
	P_FINISH;

	if( Player )
	{
		// Warn the client.
		eventPreClientTravel();

		// Do the travel.
		GetLevel()->Engine->SetClientTravel( Player, *URL, bItems, (ETravelType)TravelType );
	}

	unguardexecSlow;
}

void APlayerController::execGetPlayerNetworkAddress( FFrame& Stack, RESULT_DECL )
{
	guard(APlayerController::execGetPlayerNetworkAddress);
	P_FINISH;

	if( Player && Player->IsA(UNetConnection::StaticClass()) )
		*(FString*)Result = Cast<UNetConnection>(Player)->LowLevelGetRemoteAddress();
	else
		*(FString*)Result = TEXT("");
	unguard;
}

void APlayerController::execGetServerNetworkAddress( FFrame& Stack, RESULT_DECL )
{
	guard(APlayerController::execGetPlayerNetworkAddress);
	P_FINISH;

	if( GetLevel() && GetLevel()->NetDriver && GetLevel()->NetDriver->ServerConnection )
		*(FString*)Result = GetLevel()->NetDriver->ServerConnection->LowLevelGetRemoteAddress();
	else
		*(FString*)Result = TEXT("");
	unguard;
}

void APlayerController::execCopyToClipboard( FFrame& Stack, RESULT_DECL )
{
	guard(APlayerController::execCopyToClipboard);
	P_GET_STR(Text);
	P_FINISH;
	appClipboardCopy(*Text);
	unguard;
}

void APlayerController::execPasteFromClipboard( FFrame& Stack, RESULT_DECL )
{
	guard(APlayerController::execCopyToClipboard);
	P_GET_STR(Text);
	P_FINISH;
	*(FString*)Result = appClipboardPaste();
	unguard;
}

void ALevelInfo::execDetailChange( FFrame& Stack, RESULT_DECL )
{
	guard(ALevelInfo::execDetailChange);

	P_GET_BYTE(NewDetailMode);
	P_FINISH;

	XLevel->DetailChange((EDetailMode)NewDetailMode);

	unguard;
}

void ALevelInfo::execGetLocalURL( FFrame& Stack, RESULT_DECL )
{
	guardSlow(ALevelInfo::execGetLocalURL);

	P_FINISH;

	*(FString*)Result = GetLevel()->URL.String();

	unguardexecSlow;
}

void ALevelInfo::execIsDemoBuild( FFrame& Stack, RESULT_DECL )
{
	guardSlow(ALevelInfo::execIsDemoBuild);
	P_FINISH;

#if DEMOVERSION
	*(UBOOL*)Result = 1;
#else
	*(UBOOL*)Result = 0;
#endif

	unguardexecSlow;
}

void ALevelInfo::execGetAddressURL( FFrame& Stack, RESULT_DECL )
{
	guardSlow(ALevelInfo::execGetAddressURL);

	P_FINISH;

	*(FString*)Result = FString::Printf( TEXT("%s:%i"), *GetLevel()->URL.Host, GetLevel()->URL.Port );

	unguardexecSlow;
}

void ALevelInfo::execIsEntry( FFrame& Stack, RESULT_DECL )
{
	guard(ALevelInfo::execIsEntry);
	P_FINISH;

	*(UBOOL*)Result = 0;

	if(!XLevel || !XLevel->Engine)
		return;

	UGameEngine* GameEngine = Cast<UGameEngine>(XLevel->Engine);

	if( GameEngine &&  ( GameEngine->GLevel == GameEngine->GEntry ) )
		*(UBOOL*)Result = 1;

	unguard;
}

///////////////////////////
// Client-side functions //
///////////////////////////

void APlayerController::execClientHearSound( FFrame& Stack, RESULT_DECL )
{
	guard(APlayerController::execClientHearSound);

	P_GET_OBJECT(AActor,Actor);
	P_GET_INT(Id);
	P_GET_OBJECT(USound,Sound);
	P_GET_VECTOR(SoundLocation);
	P_GET_VECTOR(Parameters);
	P_GET_UBOOL(Attenuate);
	P_FINISH;

	FLOAT Volume = 0.001f * Parameters.X;
	FLOAT Radius = Parameters.Y;
	FLOAT Pitch  = 0.001f * Parameters.Z;
	if(	LocalPlayerController()	&& GetLevel()->Engine->Audio && Sound )
	{
		if( Actor && Actor->bDeleteMe )
			Actor = NULL;

		// First person sound attenuation hack.
		INT Flags = 0;
		if ( !Attenuate && Actor )
		{
			UViewport* Viewport = NULL;
			if( GetLevel()->Engine->Audio )
				Viewport = GetLevel()->Engine->Audio->GetViewport();
			if( Viewport && Actor->IsOwnedBy( Viewport->Actor->GetViewTarget() ) )
				Flags |= SF_No3D;
		}
		GetLevel()->Engine->Audio->PlaySound( Actor, Id, Sound, SoundLocation, Volume, Radius ? Radius : GAudioDefaultRadius, Pitch, Flags, 0.f );
	}
	unguardexec;
}

////////////////////////////////
// Latent function initiators //
////////////////////////////////

void AActor::execSleep( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSleep);

	P_GET_FLOAT(Seconds);
	P_FINISH;

	GetStateFrame()->LatentAction = EPOLL_Sleep;
	LatentFloat  = Seconds;

	unguardexecSlow;
}

void AActor::execFinishAnim( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execFinishAnim);

	P_GET_INT_OPTX(Channel, 0);
	P_FINISH;

	LatentFloat = Channel;
	StartAnimPoll();
	unguardexecSlow;
}

void AActor::StartAnimPoll()
{
	// Part of ending the zero-channel main animation.
	if( Mesh )
	{				
		Mesh->MeshGetInstance(this);
		if( MeshInstance->IsAnimLooping(LatentFloat) ) 
			MeshInstance->AnimStopLooping(LatentFloat);

		// If animation is (re)playing, wait for it to finish.
		if( IsAnimating(LatentFloat) && !MeshInstance->IsAnimPastLastFrame(LatentFloat) )
			GetStateFrame()->LatentAction = EPOLL_FinishAnim;
	}	
}

void AController::StartAnimPoll()
{
	if ( !Pawn )
		return;

	if( Pawn->Mesh )
	{				
		Pawn->Mesh->MeshGetInstance(this);
		if( Pawn->MeshInstance->IsAnimLooping(LatentFloat) ) 
			Pawn->MeshInstance->AnimStopLooping(LatentFloat);

		// If animation is (re)playing, wait for it to finish.
		if( Pawn->IsAnimating(LatentFloat) && !Pawn->MeshInstance->IsAnimPastLastFrame(LatentFloat) )
			GetStateFrame()->LatentAction = EPOLL_FinishAnim;
	}	
}


UBOOL AActor::CheckAnimFinished(INT Channel)
{
	if ( !Mesh )
		return true;
	Mesh->MeshGetInstance(this);
	return ( !IsAnimating(Channel) || MeshInstance->IsAnimPastLastFrame(Channel) );
}

UBOOL AController::CheckAnimFinished(INT Channel)
{
	if ( !Pawn || !Pawn->Mesh )
		return true;
	Pawn->Mesh->MeshGetInstance(this);
	return ( !Pawn->IsAnimating(Channel) || Pawn->MeshInstance->IsAnimPastLastFrame(Channel) );
}

void AActor::execFinishInterpolation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execFinishInterpolation);

	P_FINISH;

	GetStateFrame()->LatentAction = EPOLL_FinishInterpolation;

	unguardexecSlow;
}

///////////////////////////
// Slow function pollers //
///////////////////////////

void AActor::execPollSleep( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPollSleep);

	FLOAT DeltaSeconds = *(FLOAT*)Result;
	if( (LatentFloat-=DeltaSeconds) < 0.5 * DeltaSeconds )
	{
		// Awaken.
		GetStateFrame()->LatentAction = 0;
	}
	unguardexecSlow;
}
IMPLEMENT_FUNCTION( AActor, EPOLL_Sleep, execPollSleep );

void AActor::execPollFinishAnim( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPollFinishAnim);

	if( CheckAnimFinished(LatentFloat) )
		GetStateFrame()->LatentAction = 0;

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( AActor, EPOLL_FinishAnim, execPollFinishAnim );

void AActor::execPollFinishInterpolation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPollFinishInterpolation);

	if( !bInterpolating )
		GetStateFrame()->LatentAction = 0;

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( AActor, EPOLL_FinishInterpolation, execPollFinishInterpolation );

/////////////////////////
// Animation functions //
/////////////////////////
void AActor::execGetMeshName( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::GetMeshName);
	P_FINISH;


	if(Mesh)
		*(FString*)Result = FString(Mesh->GetName());
	else
		*(FString*)Result = FString(TEXT(""));

	unguard;
}

UBOOL AActor::PlayAnim(INT Channel,FName SequenceName, FLOAT PlayAnimRate, FLOAT TweenTime, INT Loop)
{
	guard(AActor::PlayAnim);

	if( !Mesh )
	{
		if ( !bHidden )
			debugf( NAME_Warning, TEXT("%s playanim (%s) with no mesh!"), GetName(), *SequenceName );
		return 0;
	}
	Mesh->MeshGetInstance(this);
	
	//debugf( L"%7.3f %s::PlayAnim playing %s on channel %d for Mesh %s", TIMESECONDS, GetName(), *SequenceName, Channel, Mesh->GetName() );
	
	return MeshInstance->PlayAnim( Channel, SequenceName, PlayAnimRate, TweenTime, Loop );
	unguard;
}

void AActor::execPlayAnim( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPlayAnim);

	P_GET_NAME(SequenceName);
	P_GET_FLOAT_OPTX(PlayAnimRate,1.f);
	P_GET_FLOAT_OPTX(TweenTime,0.f); // NOTE: universal default was "-1.0f" for a long time... - Erik
	P_GET_INT_OPTX(Channel, 0);
	P_FINISH;

#if 1 //NEW (mdf)
	*(DWORD*)Result = PlayAnim(Channel,SequenceName, PlayAnimRate, TweenTime,0);
#else
	PlayAnim(Channel,SequenceName, PlayAnimRate, TweenTime,0);
#endif

	unguardexecSlow;
}


void AActor::execLoopAnim( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execLoopAnim);

	P_GET_NAME(SequenceName);
	P_GET_FLOAT_OPTX(PlayAnimRate,1.f);
	P_GET_FLOAT_OPTX(TweenTime,0.f);
	P_GET_INT_OPTX(Channel, 0);
	P_FINISH;

#if 1 //NEW (mdf)
	*(DWORD*)Result = PlayAnim(Channel,SequenceName,PlayAnimRate,TweenTime,1);
#else
	PlayAnim(Channel,SequenceName,PlayAnimRate,TweenTime,1);
#endif
	unguardexecSlow;
}

void AActor::execTweenAnim( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execTweenAnim);

	P_GET_NAME(SequenceName);
	P_GET_FLOAT(TweenTime);
	P_GET_INT_OPTX(Channel, 0);
	P_FINISH;

#if 1 //NEW (mdf)
	*(DWORD*)Result = PlayAnim(Channel,SequenceName,0.f,TweenTime,0);
#else
	PlayAnim(Channel,SequenceName,0.f,TweenTime,0);
#endif

	unguardexecSlow;
}

void AActor::execIsAnimating( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execIsAnimating);

	P_GET_INT_OPTX(Channel, 0);
	P_FINISH;

	*(DWORD*)Result = IsAnimating(Channel);

	unguardexecSlow;
}

void AActor::execStopAnimating( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execStopAnimating);

	P_GET_UBOOL_OPTX(ClearAllButBase, 0);
	P_FINISH;

	// make all channels freeze in current pose
	if( Mesh )
	{
		Mesh->MeshGetInstance(this);
		
		USkeletalMeshInstance* SkelInstance = Cast<USkeletalMeshInstance>(MeshInstance);
		if(SkelInstance)
			SkelInstance->StopAnimating(ClearAllButBase);
		else
		MeshInstance->StopAnimating();
	}

	unguardexecSlow;
}

// GDC-hackish..
void AActor::execFreezeAnimAt( FFrame& Stack, RESULT_DECL )
{
	guardSlow( AActor::execFreezeAtAnimFrame);
	P_GET_FLOAT( Time );
	P_GET_INT_OPTX( Channel, 0 )
	P_FINISH;

	if( Mesh) 
	{
		Mesh->MeshGetInstance(this);
		MeshInstance->FreezeAnimAt( Time, Channel );
	}

	unguardexecSlow;
}

void AActor::execSetAnimFrame( FFrame& Stack, RESULT_DECL )
{
	guardSlow( AActor::execSetAnimFrame);
	P_GET_FLOAT( Frame );
	P_GET_INT_OPTX( Channel, 0 )
	P_GET_INT_OPTX( UnitFlag, 0 )
	P_FINISH;

	if( Mesh) 
	{
		Mesh->MeshGetInstance(this);
		MeshInstance->SetAnimFrame( Channel, Frame, UnitFlag );
	}

	unguardexecSlow;
}

// return true if than channel is animating, and animframe < 0
void AActor::execIsTweening( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execIsTweening);

	P_GET_INT(Channel);
	P_FINISH;

	*(DWORD*)Result = 0;

	if( Mesh )
	{
		Mesh->MeshGetInstance(this);
		*(DWORD*)Result = ( MeshInstance->IsAnimTweening( Channel ) ) ? 1 : 0;
	}
	
	unguardexecSlow;
}

void AActor::execHasAnim( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execHasAnim);

	P_GET_NAME(SequenceName);
	P_FINISH;

	// Check for a certain anim sequence.
	if( Mesh )
	{
		Mesh->MeshGetInstance(this);
		HMeshAnim HSeq = MeshInstance->GetAnimNamed( SequenceName );
		if( HSeq )
		{
			*(DWORD*)Result = 1;
		} else
			*(DWORD*)Result = 0;
	} else Stack.Logf( TEXT("HasAnim: No mesh") );
	unguardexecSlow;
}

void AActor::execLinkSkelAnim( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execLinkSkelAnim);	
	P_GET_OBJECT(UMeshAnimation, NewAnim);
	P_GET_OBJECT_OPTX(USkeletalMesh, NewMesh, NULL);
	P_FINISH;	

	// Set main skel anim for this instance.
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{
		if(! ((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->SetSkelAnim( NewAnim, NewMesh ) ) 
			debugf(TEXT("Trying to assign invalid animation object or into a bad channel for MeshInstance [%s]"), Mesh->MeshGetInstance(this)->GetName());
	}
	unguardexecSlow;
}

#ifdef WITH_KARMA

static UBOOL MeshIsCompatible(UMesh* Mesh, UMesh* newMesh)
{
	guard(MeshIsCompatible);

	// Check we have two skeletal meshes - otherwise deemed 'incompatible'
	if(!newMesh || !Mesh)
		return 0;

	USkeletalMesh* skelMesh = Cast<USkeletalMesh>(Mesh);
	USkeletalMesh* newSkelMesh = Cast<USkeletalMesh>(newMesh);
	if(!skelMesh || !newSkelMesh)
		return 0;

	// Must have same number of bones
	if( newSkelMesh->RefSkeleton.Num() != skelMesh->RefSkeleton.Num() )
		return 0;

	// Check heirarchy
	for(INT i=0; i<skelMesh->RefSkeleton.Num(); i++)
	{
		if( skelMesh->RefSkeleton(i).ParentIndex != newSkelMesh->RefSkeleton(i).ParentIndex )
			return 0;
	}

	return 1;

	unguard;
}

#endif // WITH_KARMA

void AActor::execLinkMesh( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execLinkMesh);	
	P_GET_OBJECT(UMesh, NewMesh);
	P_GET_UBOOL_OPTX( bKeepAnim, 0);
	P_FINISH;	

	if ( Mesh != NewMesh )
	{
		if( NewMesh && NewMesh->IsA(USkeletalMesh::StaticClass()) )
		{
			// If we are currently in ragdoll, check the mesh has the same heirarchy before replacing it.
			if(Mesh)
			{
#ifdef WITH_KARMA
				USkeletalMeshInstance* inst = (USkeletalMeshInstance*)Mesh->MeshGetInstance(this);

				// If we are currently running ragdoll, skeletons have to exactly match
				if(Physics == PHYS_KarmaRagDoll && inst && inst->KSkelIsInitialised)
				{
					if( !MeshIsCompatible( Mesh, NewMesh ) )
						return;
				}
#endif

				//#Skel - Other subclasses need specific setmesh to update meshinstances properly.
				if( bKeepAnim )
					((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->SetMesh( NewMesh ); 
			}
		}
		Mesh = NewMesh;
		bNetDirty=true;

		// FIXME temp hack - make this a script event
		APawn *P = Cast<APawn>(this);
		if ( P && (P->HeadScale != 1.f) )
			P->eventSetHeadScale(P->HeadScale);
	}
	unguardexecSlow;
}

void AActor::execBoneRefresh( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execBoneRefresh);	
	P_FINISH;		
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{
		((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->ForceBoneRefresh();
	}		
	else
	{
		debugf( NAME_Warning, TEXT("Actor %s has no valid skeletal mesh yet !!!"),this->GetName()); //#SKEL
	}
	unguardexecSlow;
}

void AActor::execGetBoneCoords( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetBoneCoords);
	P_GET_NAME(BoneName);
	P_FINISH;
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		INT BoneIdx = ((USkeletalMeshInstance*)MeshInstance)->MatchRefBone( BoneName );
		if( BoneIdx >= 0)
		{
			*(FCoords*)Result = ((USkeletalMeshInstance*)MeshInstance)->GetBoneCoords(BoneIdx);
		}
		else
		{
			*(FCoords*)Result = ((USkeletalMeshInstance*)MeshInstance)->GetBoneCoords(0);
			//debugf(TEXT("Warning: Invalid bone index (%i) for GetBoneRotation."),BoneIdx);
			//*(FCoords*)Result = GMath.UnitCoords;
		}
	}	
	unguardexecSlow;
}

void AActor::execGetBoneRotation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetBoneRotation);
	P_GET_NAME(BoneName);
	P_GET_INT_OPTX( Space, 0 ); // 0 local, 1 global, 2 relative reference pose (?) 
	P_FINISH;
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		INT BoneIdx = ((USkeletalMeshInstance*)MeshInstance)->MatchRefBone( BoneName );
		if( BoneIdx >= 0)
		{
			*(FRotator*)Result = ((USkeletalMeshInstance*)MeshInstance)->GetBoneRotation(BoneIdx,Space);
		}
		else
		{
			debugf(TEXT("Warning: Invalid bone index (%i) for GetBoneRotation."),BoneIdx);
			*(FRotator*)Result = FRotator(0,0,0);
		}
	}	
	unguardexecSlow;
}


void AActor::execGetRootLocation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetRootLocation);
	P_FINISH;
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		*(FVector*)Result = ((USkeletalMeshInstance*)MeshInstance)->GetRootLocation();
	}
	unguardexecSlow;
}

void AActor::execGetRootRotation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetRootRotation);
	P_FINISH;
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		*(FRotator*)Result = ((USkeletalMeshInstance*)MeshInstance)->GetRootRotation();
	}
	unguardexecSlow;
}

void AActor::execGetRootLocationDelta( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetRootLocationDelta);
	P_FINISH;
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		*(FVector*)Result = ((USkeletalMeshInstance*)MeshInstance)->GetRootLocationDelta();
	}
	unguardexecSlow;
}

void AActor::execGetRootRotationDelta( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetRootRotationDelta);
	P_FINISH;
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		*(FRotator*)Result = ((USkeletalMeshInstance*)MeshInstance)->GetRootRotationDelta();
	}
	unguardexecSlow;
}


void AActor::execLockRootMotion( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execLockRootRotation);
	P_GET_INT_OPTX(Lock,1);	
	P_FINISH;

	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		((USkeletalMeshInstance*)MeshInstance)->LockRootMotion(Lock);
	}
	unguardexecSlow;
}

void AActor::execAnimBlendParams( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAnimBlendParams);
	P_GET_INT(Channel);
	P_GET_FLOAT_OPTX(BlendAlpha,0.0f); 
	P_GET_FLOAT_OPTX(InRate,0.0f);
	P_GET_FLOAT_OPTX(OutRate,0.0f);
	P_GET_NAME_OPTX(StartBoneName, NAME_None);	
	P_GET_UBOOL_OPTX(bGlobalPose, 0);
	P_FINISH;

	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		// Assign the blend parameters to the current AnimStage channel.
		((USkeletalMeshInstance*)MeshInstance)->SetBlendParams( Channel, BlendAlpha, InRate, OutRate, StartBoneName, bGlobalPose );
	}
	unguardexecSlow;
}

void AActor::execAnimBlendToAlpha( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAnimBlendToAlpha);
	P_GET_INT(Channel);
	P_GET_FLOAT(TargetAlpha); 
	P_GET_FLOAT(TimeInterval);	
	P_FINISH;

	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		Mesh->MeshGetInstance(this);
		// Assign the blend parameters to the current AnimStage channel.
		((USkeletalMeshInstance*)MeshInstance)->BlendToAlpha( Channel, TargetAlpha, TimeInterval);		
	}
	unguardexecSlow;
}

void AActor::execGetNotifyChannel( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetNotifyChannel);
	P_FINISH;

	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		*(DWORD*)Result = (DWORD)((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->LastNotifyStage;
	}
	else
	*(DWORD*)Result = 0; //Default stage.

	unguardexecSlow;	
}

void AActor::execEnableChannelNotify( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execEnableChannelNotify);
	P_GET_INT(Channel);	
	P_GET_INT_OPTX(Switch,1);	
	P_FINISH;

	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass()) )
	{				
		((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->EnableChannelNotify(Channel,Switch);
	}
	unguardexecSlow;	
}

void AActor::execSetBoneScale( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetBoneScale);
	P_GET_INT(Slot);
	P_GET_FLOAT_OPTX(BoneScale, 1.0f);
	P_GET_NAME_OPTX(BoneName,NAME_None);	
	P_FINISH;
	// Set or clear bone scaler.
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass())) 
	{
		((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->SetBoneScale( Slot, BoneScale, BoneName);
	}
	unguardexecSlow;
}

void AActor::execSetBoneLocation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetBoneLocation);
	//P_GET_INT(Slot);
	P_GET_NAME( BoneName );	
	P_GET_VECTOR_OPTX( BoneTrans, FVector(0.0f,0.0f,0.0f) );
	P_GET_FLOAT_OPTX( Alpha, 1.0f ); 
	P_FINISH;

	// Set or clear bone locator.
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass())) 
	{
		((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->SetBoneLocation( BoneName, BoneTrans, Alpha );
	}
	unguardexecSlow;
}

void AActor::execSetBoneRotation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetBoneRotation);
	P_GET_NAME( BoneName );	
	P_GET_ROTATOR_OPTX( BoneTurn, FRotator(0,0,0) );
	P_GET_INT_OPTX( Space, 0 );   // 0 local, 1 global
	P_GET_FLOAT_OPTX( Alpha, 1.0f ); 
	P_FINISH;

	// Set or clear bone rotator.
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass())) 
	{
		((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->SetBoneRotation( BoneName, BoneTurn, Space, Alpha );
	}
	unguardexecSlow;	
}


// native final function bool SetBoneDirection( name BoneName, rotator BoneTurn,  optional vector BoneTrans, optional float Alpha );

// 'Bend' bone direction (axis-angle style?) in world coordinates...
void AActor::execSetBoneDirection( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetBoneDirection);
	//P_GET_INT(Slot);
	P_GET_NAME( BoneName );	
	P_GET_ROTATOR( BoneTurn );
	P_GET_VECTOR_OPTX( BoneTrans, FVector(0.0f,0.0f,0.0f) );
	P_GET_FLOAT_OPTX( Alpha, 1.0f ); 
	P_GET_INT_OPTX( Space, 0 );   // 0 local, 1 global
	P_FINISH;	

	// Set or clear bone direction controller.
	if( Mesh && Mesh->IsA(USkeletalMesh::StaticClass())) 
	{
		((USkeletalMeshInstance*)Mesh->MeshGetInstance(this))->SetBoneDirection( BoneName, BoneTurn, BoneTrans, Alpha, Space);
	}
	unguardexecSlow;	
}

// native final function GetAnimParams( int Channel, out name Sequence, out float AnimFrame, out float AnimRate );

void AActor::execGetAnimParams( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execGetAnimParams);
	P_GET_INT(Channel);
	P_GET_NAME_REF( OutSeqName );
	P_GET_FLOAT_REF( OutAnimFrame );
	P_GET_FLOAT_REF( OutAnimRate );
	P_FINISH;	

	*OutSeqName = NAME_None;
	*OutAnimFrame = 0.0f;
	*OutAnimRate = 0.0f;

	if( Mesh )		
	{	
		Mesh->MeshGetInstance(this);

		*OutSeqName = MeshInstance->GetActiveAnimSequence( Channel );
		*OutAnimFrame = MeshInstance->GetActiveAnimFrame( Channel );
		*OutAnimRate = MeshInstance->GetActiveAnimRate( Channel );
	}


	unguardexecSlow;	
}


// native final function bool AnimIsInGroup( int Channel, name GroupName ); 
// Determine if currently playing animation of channel (default 0) is member of given group 

void AActor::execAnimIsInGroup( FFrame& Stack, RESULT_DECL )
{
	guardSlow( AActor::execAnimIsInGroup );
	P_GET_INT( Channel );
	P_GET_NAME( GroupName )
	P_FINISH;	

	*(DWORD*)Result = 0;
	if( Mesh )
	{		
		Mesh->MeshGetInstance(this);
		FName ThisAnimSeq = NAME_None;
		ThisAnimSeq = MeshInstance->GetActiveAnimSequence( Channel );
		if( ThisAnimSeq != NAME_None )
		{
			HMeshAnim HSeq = MeshInstance->GetAnimNamed( ThisAnimSeq );
			if( HSeq )
			{
				*(DWORD*)Result = MeshInstance->AnimIsInGroup(HSeq, GroupName)? 1 : 0 ;
			}
		}
	}
	unguardexecSlow;	
}

///////////////
// Collision //
///////////////

void AActor::execSetCollision( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetCollision);

	P_GET_UBOOL_OPTX(NewCollideActors,bCollideActors);
	P_GET_UBOOL_OPTX(NewBlockActors,  bBlockActors  );
	P_GET_UBOOL_OPTX(NewBlockPlayers, bBlockPlayers );
	P_FINISH;

	SetCollision( NewCollideActors, NewBlockActors, NewBlockPlayers );

	unguardexecSlow;
}

void AActor::execOnlyAffectPawns( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execOnlyAffectPawns);

	P_GET_UBOOL(bNewAffectPawns);
	P_FINISH;

	if( bCollideActors && (bNewAffectPawns != bOnlyAffectPawns))
	{
		SetCollision(false, bBlockActors, bBlockPlayers);
		bOnlyAffectPawns = bNewAffectPawns;
		SetCollision(true, bBlockActors, bBlockPlayers);
	}

	unguardexecSlow;
}

void AActor::execSetCollisionSize( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetCollisionSize);

	P_GET_FLOAT(NewRadius);
	P_GET_FLOAT(NewHeight);
	P_FINISH;

	SetCollisionSize( NewRadius, NewHeight );

	// Return boolean success or failure.
	*(DWORD*)Result = 1;

	unguardexecSlow;
}

void AActor::execSetBase( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetFloor);

	P_GET_OBJECT(AActor,NewBase);
	P_GET_VECTOR_OPTX(NewFloor, FVector(0,0,1) );
	P_FINISH;

	SetBase( NewBase,NewFloor );

	unguardexecSlow;
}

void AActor::execSetDrawScale( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetDrawScale);

	P_GET_FLOAT(NewScale);
	P_FINISH;

	SetDrawScale(NewScale);

	unguardexecSlow;
}

void AActor::execSetStaticMesh( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetStaticMesh);

	P_GET_OBJECT(UStaticMesh,NewStaticMesh);
	P_FINISH;

	SetStaticMesh(NewStaticMesh);

	unguardexecSlow;
}

void AActor::execSetDrawType( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetDrawType);

	P_GET_BYTE(NewDrawType);
	P_FINISH;

	SetDrawType(EDrawType(NewDrawType));

	unguardexecSlow;
}

void AActor::execSetDrawScale3D( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetCollisionSize3D);

	P_GET_VECTOR(NewScale3D);
	P_FINISH;

	SetDrawScale3D(NewScale3D);

	unguardexecSlow;
}

#if 1 //NEW (mdf)
void AActor::execSetNetDirty( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetNetDirty);

	P_GET_UBOOL(bVal);
	P_FINISH;

	bNetDirty = bVal;

	unguardexecSlow;
}
#endif

void AActor::execGetClient( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::execGetClient);
	P_FINISH;
	*(UClient**)Result = GetLevel()->Engine->Client;
	unguardexec;
}

///////////
// Audio //
///////////
void AController::CheckHearSound(AActor* SoundMaker, INT Id, USound* Sound, FVector Parameters, FLOAT Radius, UBOOL Attenuate)
{
	guardSlow(AController::CheckHearSound);

	if ( Pawn && IsProbing(NAME_AIHearSound) && CanHearSound(Pawn->Location, SoundMaker, Radius) )
		eventAIHearSound( SoundMaker, Id, Sound, Pawn->Location, Parameters, Attenuate );

	unguardSlow;
}

// Can hear audio sound being played
UBOOL AController::CanHearSound(FVector HearSource, AActor* SoundMaker, FLOAT Radius)
{
	guardSlow(AController::CanHearSound);

	FVector SoundSourceLocation = SoundMaker->Location;
	if ( SoundMaker->IsA(AWeapon::StaticClass()) && SoundMaker->Instigator )
		SoundSourceLocation = SoundMaker->Instigator->Location;
	if( !GetLevel()->IsAudibleAt( SoundSourceLocation, HearSource, SoundMaker, (ESoundOcclusion) SoundMaker->SoundOcclusion ) )
	{
		FPointRegion Region1 = GetLevel()->Model->PointRegion( Level, HearSource );
		FPointRegion Region2 = GetLevel()->Model->PointRegion( Level, SoundSourceLocation );
		Radius *= GetLevel()->CalculateRadiusMultiplier( Region1.ZoneNumber, Region2.ZoneNumber );
	}

	if(	(HearSource - SoundSourceLocation).SizeSquared() < Square( Radius * GAudioMaxRadiusMultiplier ) )
		return true;
	else 
		return false;

	unguardSlow;
}

void APlayerController::CheckHearSound(AActor* SoundMaker, INT Id, USound* Sound, FVector Parameters, FLOAT Radius, UBOOL Attenuate )
{
	guardSlow(APlayerController::CheckHearSound);

	FVector SoundSourceLocation = SoundMaker->Location;
	if ( SoundMaker->IsA(AWeapon::StaticClass()) && SoundMaker->Instigator )
		SoundSourceLocation = SoundMaker->Instigator->Location;
	GetViewTarget();
	FVector ParamModifier(1000.f,1.f,1000.f);
	if ( CanHearSound(ViewTarget->Location, SoundMaker, Radius) || !Attenuate )
		eventClientHearSound( SoundMaker, Id, Sound, SoundSourceLocation, Parameters * ParamModifier, Attenuate );

	unguardSlow;
}

void AActor::execDemoPlaySound( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execDemoPlaySound);

	// Get parameters.
	P_GET_OBJECT(USound,Sound);
	P_GET_BYTE_OPTX(Slot,SLOT_Misc);
	P_GET_FLOAT_OPTX(Volume,TransientSoundVolume);
	P_GET_UBOOL_OPTX(bNoOverride, 0);
	P_GET_FLOAT_OPTX(Radius,TransientSoundRadius);
	P_GET_FLOAT_OPTX(Pitch,1.0f);
	P_GET_UBOOL_OPTX(Attenuate,1);
	P_FINISH;

	if( !Sound )
		return;

	if ( Instigator && ((Instigator == this) || (Instigator == Owner)) )
		Volume *= Instigator->SoundDampening;

	// Play the sound locally
	INT Id = GetIndex()*16 + Slot*2 + bNoOverride;
	FVector Parameters = FVector(Volume, Radius, Pitch);

	UClient* Client = GetLevel()->Engine->Client;
	if( Client )
	{
		for( INT i=0; i<Client->Viewports.Num(); i++ )
		{
			AController* Hearer = Client->Viewports(i)->Actor;
			if( Hearer && Hearer->GetLevel()==GetLevel() )
				Hearer->CheckHearSound(this, Id, Sound, Parameters, Radius ? Radius : GAudioDefaultRadius, Attenuate );
		}
	}
	unguardexecSlow;
}

void AActor::execPlayMusic( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPlayMusic);
	P_GET_STR(Song);
	P_GET_FLOAT(FadeInTime);
	P_FINISH;

	if( ParseParam( appCmdLine(), TEXT("nomusic") ) )
    {
    	*(INT*)Result = 0;
        return;
    }    

	INT SongHandle = 0;

	if( GetLevel()->Engine->Audio )
		SongHandle = GetLevel()->Engine->Audio->PlayMusic( Song, FadeInTime );

	*(INT*)Result  = SongHandle;

	unguardexecSlow;
}

void AActor::execStopMusic( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execStopMusic);
	P_GET_INT( SongHandle );
	P_GET_FLOAT( FadeOutTime );
	P_FINISH;	

	if( GetLevel()->Engine->Audio )
		GetLevel()->Engine->Audio->StopMusic( SongHandle, FadeOutTime );

	unguardexecSlow;
}

void AActor::execStopAllMusic( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execStopAllMusic);
	P_GET_FLOAT( FadeOutTime );
	P_FINISH;	

	if( GetLevel()->Engine->Audio )
		GetLevel()->Engine->Audio->StopAllMusic( FadeOutTime );

	unguardexecSlow;
}

void AActor::execPlaySound( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPlaySound);

	// Get parameters.
	P_GET_OBJECT(USound,Sound);
	P_GET_BYTE_OPTX(Slot,SLOT_Misc);
	P_GET_FLOAT_OPTX(Volume,TransientSoundVolume);
	P_GET_UBOOL_OPTX(bNoOverride, 0);
	P_GET_FLOAT_OPTX(Radius,TransientSoundRadius);
	P_GET_FLOAT_OPTX(Pitch,1.0f);
	P_GET_UBOOL_OPTX(Attenuate,1);
	P_FINISH;

	if( !Sound )
		return;

	if ( Instigator && ((Instigator == this) || (Instigator == Owner)) )
		Volume *= Instigator->SoundDampening;

	// Server-side demo needs a call to execDemoPlaySound for the DemoRecSpectator
	if(		GetLevel() && GetLevel()->DemoRecDriver
		&&	!GetLevel()->DemoRecDriver->ServerConnection
		&&	Level->NetMode != NM_Client )
		eventDemoPlaySound(Sound, Slot, Volume, bNoOverride, Radius, Pitch, Attenuate);

	INT Id = GetIndex()*16 + Slot*2 + bNoOverride;
	FVector Parameters = FVector(Volume, Radius, Pitch);

	// See if the function is simulated.
	UFunction* Caller = Cast<UFunction>( Stack.Node,CLASS_IsAUFunction );
	if( (Level->NetMode == NM_Client) || (Caller && (Caller->FunctionFlags & FUNC_Simulated)) )
	{
		// Called from a simulated function, so propagate locally only.
		UClient* Client = GetLevel()->Engine->Client;
		if( Client )
		{
			for( INT i=0; i<Client->Viewports.Num(); i++ )
			{
				AController* Hearer = Client->Viewports(i)->Actor;
				if( Hearer && Hearer->GetLevel()==GetLevel() )
					Hearer->CheckHearSound(this, Id, Sound, Parameters,Radius,Attenuate);
			}
		}
	}
	else
	{
		// Propagate to all player actors.
		for( AController* Hearer=Level->ControllerList; Hearer; Hearer=Hearer->nextController )
		{
			if( Hearer->bIsPlayer )
				Hearer->CheckHearSound(this, Id, Sound, Parameters, Radius ? Radius : GAudioDefaultRadius, Attenuate );
		}
	}
	unguardexecSlow;
}

void AActor::execPlayOwnedSound( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPlayOwnedSound);

	// Get parameters.
	P_GET_OBJECT(USound,Sound);
	P_GET_BYTE_OPTX(Slot,SLOT_Misc);
	P_GET_FLOAT_OPTX(Volume,TransientSoundVolume);
	P_GET_UBOOL_OPTX(bNoOverride, 0);
	P_GET_FLOAT_OPTX(Radius,TransientSoundRadius);
	P_GET_FLOAT_OPTX(Pitch,1.0f);
	P_GET_UBOOL_OPTX(Attenuate,0);
	P_FINISH;

	if( !Sound )
		return;

	if ( Instigator && ((Instigator == this) || (Instigator == Owner)) )
		Volume *= Instigator->SoundDampening;

	// if we're recording a demo, make a call to execDemoPlaySound()
	if( (GetLevel() && GetLevel()->DemoRecDriver && !GetLevel()->DemoRecDriver->ServerConnection) )
		eventDemoPlaySound(Sound, Slot, Volume, bNoOverride, Radius ? Radius : GAudioDefaultRadius, Pitch, Attenuate);

	INT Id = GetIndex()*16 + Slot*2 + bNoOverride;
	FVector Parameters = FVector(Volume, Radius, Pitch);

	if( Level->NetMode == NM_Client )
	{
		UClient* Client = GetLevel()->Engine->Client;
		if( Client )
		{
			for( INT i=0; i<Client->Viewports.Num(); i++ )
			{
				APlayerController* Hearer = Client->Viewports(i)->Actor;
				if( Hearer && Hearer->GetLevel()==GetLevel() )
					Hearer->CheckHearSound(this, Id, Sound, Parameters, Radius ? Radius : GAudioDefaultRadius, Attenuate );
			}
		}
	}
	else
	{
		AActor *RemoteOwner = NULL;
		if( Level->NetMode != NM_Standalone )
		{
			APawn* NoisePawn = NULL;
			if ( IsAPawn() )
				NoisePawn = Cast<APawn>(this);
			else if ( Owner && Owner->IsAPawn() )
				NoisePawn = Cast<APawn>(Owner);

			if ( NoisePawn && !NoisePawn->IsLocallyControlled() )
					RemoteOwner = NoisePawn;
		}

		for( AController* Hearer=Level->ControllerList; Hearer; Hearer=Hearer->nextController )
		{
			if( Hearer->bIsPlayer && ((Hearer->Pawn != RemoteOwner) || (Hearer->Pawn == NULL)) )
				Hearer->CheckHearSound(this, Id, Sound, Parameters, Radius ? Radius : GAudioDefaultRadius, Attenuate );
		}
	}
	unguardexecSlow;
}

void AActor::execGetSoundDuration( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::execGetSoundDuration);

	// Get parameters.
	P_GET_OBJECT(USound,Sound);
	P_FINISH;

	if ( Sound )
		*(FLOAT*)Result = Sound->GetDuration();
	else
		*(FLOAT*)Result = 0.f;
	unguardexec;
}


void APlayerController::execLeaveVoiceChat( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::execLeaveVoiceChat);

	P_FINISH;
	
	if( GetLevel()->Engine->Audio )
		GetLevel()->Engine->Audio->LeaveVoiceChat();
		
	unguardexec;
}

void APlayerController::execChangeVoiceChatter( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::execLeaveVoiceChat);

	P_GET_INT(IpAddr);
	P_GET_INT(Handle);
	P_GET_UBOOL(Add);

	P_FINISH;
	
	if( GetLevel()->Engine->Audio )
		GetLevel()->Engine->Audio->ChangeVoiceChatter( IpAddr, Handle, Add );
		
	unguardexec;
}

//////////////
// Movement //
//////////////

void AActor::execMove( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execMove);

	P_GET_VECTOR(Delta);
	P_GET_UBOOL_OPTX(bTest,0);
	P_FINISH;

	*(DWORD*)Result = Move( Delta, bTest );

	unguardexecSlow;
}

void AActor::execSetLocation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetLocation);

	P_GET_VECTOR(NewLocation);
	P_FINISH;

	*(DWORD*)Result = SetLocation( NewLocation );
	unguardexecSlow;
}

void AActor::execTestLocation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execTestLocation);

	P_GET_VECTOR(TestLocation);
	P_FINISH;

	*(DWORD*)Result = GetLevel()->FarMoveActor( this, TestLocation, 1 );
	unguardexecSlow;
}

void AActor::execSetRelativeLocation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetLocation);

	P_GET_VECTOR(NewLocation);
	P_FINISH;

	if ( AttachmentBone != NAME_None )
	{
		RelativeLocation = NewLocation;
		*(DWORD*)Result = 1;
	}
	else if ( Base )
	{
		FCoords Coords = GMath.UnitCoords / Base->Rotation;
		NewLocation = Base->Location + NewLocation.X * Coords.XAxis + NewLocation.Y * Coords.YAxis + NewLocation.Z * Coords.ZAxis;
		*(DWORD*)Result = GetLevel()->FarMoveActor( this, NewLocation,false,false,true );
		if ( Base )
			RelativeLocation = Location - Base->Location;
	}

	unguardexecSlow;
}

void AActor::execSetRotation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetRotation);

	P_GET_ROTATOR(NewRotation);
	P_FINISH;

	*(DWORD*)Result = SetRotation( NewRotation );

	unguardexecSlow;
}

void AActor::execSetRelativeRotation( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetRelativeRotation);

	P_GET_ROTATOR(NewRotation);
	P_FINISH;

	if ( AttachmentBone != NAME_None )
	{
		RelativeRotation = NewRotation;
		*(DWORD*)Result = 1;
	}
	else
	{
		if ( Base )
		{
			if(bHardAttach && !bBlockActors && !bBlockPlayers)
			{
				FVector currentRelLoc;
				currentRelLoc.X = HardRelMatrix.M[3][0];
				currentRelLoc.Y = HardRelMatrix.M[3][1];
				currentRelLoc.Z = HardRelMatrix.M[3][2];
	
				// We make a new HardRelMatrix using the new rotation and the existing position.
				HardRelMatrix = FRotationMatrix(NewRotation) * FTranslationMatrix(currentRelLoc);

				// Work out what the new child rotation is
				FMatrix BaseTM = FRotationMatrix(Base->Rotation) * FTranslationMatrix(Base->Location);
				FMatrix NewWorldTM = HardRelMatrix * BaseTM;
				NewRotation = (GMath.UnitCoords * NewWorldTM.Coords()).OrthoRotation();
			}
			else
			{
			FCoords Coords = GMath.UnitCoords / Base->Rotation;
			FCoords PartCoords = GMath.UnitCoords / NewRotation;
			PartCoords = PartCoords * Coords.Transpose();
			NewRotation = PartCoords.OrthoRotation();
			}
		}
		FCheckResult Hit(1.0f);
		*(DWORD*)Result = GetLevel()->MoveActor( this, FVector(0,0,0), NewRotation, Hit );
	}
	unguardexecSlow;
}


// 
// Explicitly attach an object to / detach from an actor, thereby setting it as the base.
//
void AActor::execAttachToBone( FFrame& Stack, RESULT_DECL )
{
	//native final function bool AttachToBone( actor Attachment, name BoneName );
	guardSlow(AActor::execAttachToActorBone);	
	P_GET_OBJECT(AActor,Attachment);
	P_GET_NAME(BoneName);
	P_FINISH;
	
	*(DWORD*)Result = AttachToBone( Attachment, BoneName );

	unguardexecSlow;
}

void AActor::execDetachFromBone( FFrame& Stack, RESULT_DECL )
{
	//native final function bool DetachFromBone( actor Attachment );
	guardSlow(AActor::execDetachFromBone);
	P_GET_OBJECT(AActor,Attachment);
	P_FINISH;

	*(DWORD*)Result = DetachFromBone( Attachment );

	unguardexecSlow;
}



///////////////
// Relations //
///////////////

void AActor::execSetOwner( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetOwner);

	P_GET_ACTOR(NewOwner);
	P_FINISH;

	SetOwner( NewOwner );

	unguardexecSlow;
}

//////////////////
// Line tracing //
//////////////////

void AActor::execTrace( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execTrace);

	P_GET_VECTOR_REF(HitLocation);
	P_GET_VECTOR_REF(HitNormal);
	P_GET_VECTOR(TraceEnd);
	P_GET_VECTOR_OPTX(TraceStart,Location);
	P_GET_UBOOL_OPTX(bTraceActors,bCollideActors);
	P_GET_VECTOR_OPTX(TraceExtent,FVector(0,0,0));
	P_GET_OBJECT_REF(UMaterial, Material);
	P_FINISH;

	// Trace the line.
	FCheckResult Hit(1.0f);
	DWORD TraceFlags;
	if( bTraceActors )
		TraceFlags = TRACE_ProjTargets;
	else
		TraceFlags = TRACE_World;

	if( Material )
		TraceFlags |= TRACE_Material;

	AActor *TraceActor = this;
	AController *C = Cast<AController>(this);
	if ( C && C->Pawn )
		TraceActor = C->Pawn;
	GetLevel()->SingleLineCheck( Hit, TraceActor, TraceEnd, TraceStart, TraceFlags, TraceExtent );

#if 0 //DEBUG
	UFunction* Caller = Cast<UFunction>( Stack.Node,CLASS_IsAUFunction );
	debugf(TEXT("==> Trace by %s (%s)"),GetFullNameSafe(Caller),GetFullName());
	GTempLineBatcher->AddLine( TraceStart, TraceEnd, FColor(0,255,0) );
	if( Hit.Actor )
		GTempLineBatcher->AddLine( TraceStart, Hit.Location, FColor(255,0,0) );
//	GTempLineBatcher->AddLine( TraceStart, Hit.Location, FColor(0,255,0) );
//	GTempLineBatcher->AddLine( Hit.Location, TraceEnd, FColor(255,0,0) );
#endif

	*(AActor**)Result = Hit.Actor;
	*HitLocation      = Hit.Location;
	*HitNormal        = Hit.Normal;
	if( Material )
		*Material = Hit.Material;
	unguardexecSlow;
}

void AActor::execFastTrace( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execFastTrace);

	P_GET_VECTOR(TraceEnd);
	P_GET_VECTOR_OPTX(TraceStart,Location);
	P_FINISH;

	// Trace the line.
	FCheckResult Hit(1.f);
	GetLevel()->SingleLineCheck( Hit, this, TraceEnd, TraceStart, TRACE_World|TRACE_StopAtFirstHit );

	*(DWORD*)Result = !Hit.Actor;

	unguardexecSlow;
}

///////////////////////
// Spawn and Destroy //
///////////////////////

void AActor::execSpawn( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSpawn);

	P_GET_OBJECT(UClass,SpawnClass);
	P_GET_OBJECT_OPTX(AActor,SpawnOwner,NULL); 
	P_GET_NAME_OPTX(SpawnName,NAME_None);
	P_GET_VECTOR_OPTX(SpawnLocation,Location);
	P_GET_ROTATOR_OPTX(SpawnRotation,Rotation);
	P_GET_ACTOR_OPTX(SpawnTemplate,NULL);
	P_FINISH;

	// SCRIPTTIME FLOAT OldClock = GStats.DWORDStats(GEngineStats.STATS_Game_SpawningCycles);
	clock(GStats.DWORDStats(GEngineStats.STATS_Game_SpawningCycles));
	// Spawn and return actor.
	AActor* Spawned = SpawnClass ? GetLevel()->SpawnActor
	(
		SpawnClass,
		NAME_None,
		SpawnLocation,
		SpawnRotation,
		SpawnTemplate,
		0,
		0,
		SpawnOwner,
		Instigator
	) : NULL;
	if( Spawned && (SpawnName != NAME_None) )
		Spawned->Tag = SpawnName;
	*(AActor**)Result = Spawned;
	unclock(GStats.DWORDStats(GEngineStats.STATS_Game_SpawningCycles));
	/* SCRIPTTIME
	if ( Spawned )
		debugf(TEXT("%s Spawned %s took %f"),GetName(),Spawned->GetName(), (GStats.DWORDStats(GEngineStats.STATS_Game_SpawningCycles) - OldClock) * GSecondsPerCycle * 1000.0f);
	else
		debugf(TEXT("%s Failed spawn took %f"),GetName(),(GStats.DWORDStats(GEngineStats.STATS_Game_SpawningCycles) - OldClock) * GSecondsPerCycle * 1000.0f);
	*/
	unguardexecSlow;
}

void AActor::execDestroy( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execDestroy);

	P_FINISH;
	
	*(DWORD*)Result = GetLevel()->DestroyActor( this );

	unguardexecSlow;
}

////////////
// Timing //
////////////

void AActor::execSetTimer( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetTimer);

	P_GET_FLOAT(NewTimerRate);
	P_GET_UBOOL(bLoop);
	P_FINISH;

	TimerCounter = 0.0f;
	TimerRate    = NewTimerRate;
	bTimerLoop   = bLoop;

	unguardexecSlow;
}

////////////////
// Warp zones //
////////////////

void AWarpZoneInfo::execWarp( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AWarpZoneInfo::execWarp);

	P_GET_VECTOR_REF(WarpLocation);
	P_GET_VECTOR_REF(WarpVelocity);
	P_GET_ROTATOR_REF(WarpRotation);
	P_FINISH;

	// Perform warping.
	*WarpLocation = (*WarpLocation).TransformPointBy ( WarpCoords.Transpose() );
	*WarpVelocity = (*WarpVelocity).TransformVectorBy( WarpCoords.Transpose() );
	*WarpRotation = (GMath.UnitCoords / *WarpRotation * WarpCoords.Transpose()).OrthoRotation();

	unguardexecSlow;
}

void AWarpZoneInfo::execUnWarp( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AWarpZoneInfo::execUnWarp);

	P_GET_VECTOR_REF(WarpLocation);
	P_GET_VECTOR_REF(WarpVelocity);
	P_GET_ROTATOR_REF(WarpRotation);
	P_FINISH;

	// Perform unwarping.
	*WarpLocation = (*WarpLocation).TransformPointBy ( WarpCoords );
	*WarpVelocity = (*WarpVelocity).TransformVectorBy( WarpCoords );
	*WarpRotation = (GMath.UnitCoords / *WarpRotation * WarpCoords).OrthoRotation();

	unguardexecSlow;
}

/*-----------------------------------------------------------------------------
	Native iterator functions.
-----------------------------------------------------------------------------*/

void AActor::execAllActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAllActors);

	// Get the parms.
	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_NAME_OPTX(TagName,NAME_None);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<GetLevel()->Actors.Num() && *OutActor==NULL )
		{
			AActor* TestActor = GetLevel()->Actors(iActor++);
			if(	TestActor && TestActor->IsA(BaseClass) && (TagName==NAME_None || TestActor->Tag==TagName) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

void AActor::execDynamicActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execDynamicActors);

	// Get the parms.
	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_NAME_OPTX(TagName,NAME_None);
	P_FINISH;
	
	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iActor = GetLevel()->iFirstDynamicActor;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<GetLevel()->Actors.Num() && *OutActor==NULL )
		{
			AActor* TestActor = GetLevel()->Actors(iActor++);
			if(	TestActor && TestActor->IsA(BaseClass) && (TagName==NAME_None || TestActor->Tag==TagName) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

void AActor::execChildActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execChildActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<GetLevel()->Actors.Num() && *OutActor==NULL )
		{
			AActor* TestActor = GetLevel()->Actors(iActor++);
			if(	TestActor && TestActor->IsA(BaseClass) && TestActor->IsOwnedBy( this ) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

void AActor::execBasedActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execBasedActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<GetLevel()->Actors.Num() && *OutActor==NULL )
		{
			AActor* TestActor = GetLevel()->Actors(iActor++);
			if(	TestActor && TestActor->IsA(BaseClass) && TestActor->Base==this )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

void AActor::execTouchingActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execTouchingActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iTouching=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		for( iTouching; iTouching<Touching.Num() && *OutActor==NULL; iTouching++ )
		{
			AActor* TestActor = Touching(iTouching);
			if(	TestActor && TestActor->IsA(BaseClass) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

void AActor::execTraceActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execTraceActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_VECTOR_REF(HitLocation);
	P_GET_VECTOR_REF(HitNormal);
	P_GET_VECTOR(End);
	P_GET_VECTOR_OPTX(Start,Location);
	P_GET_VECTOR_OPTX(TraceExtent,FVector(0,0,0));
	P_FINISH;

	FMemMark Mark(GMem);
	BaseClass         = BaseClass ? BaseClass : AActor::StaticClass();
	FCheckResult* Hit = GetLevel()->MultiLineCheck( GMem, End, Start, TraceExtent, Level, TRACE_AllColliding, this );

	PRE_ITERATOR;
		if( Hit )
		{
			if ( Hit->Actor && Hit->Actor->IsA(BaseClass) )
			{
				*OutActor    = Hit->Actor;
				*HitLocation = Hit->Location;
				*HitNormal   = Hit->Normal;
			}
			Hit = Hit->GetNext();
		}
		else
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			*OutActor = NULL;
			break;
		}
	POST_ITERATOR;
	Mark.Pop();

	unguardexecSlow;
}

void AActor::execRadiusActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execRadiusActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_FLOAT(Radius);
	P_GET_VECTOR_OPTX(TraceLocation,Location);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<GetLevel()->Actors.Num() && *OutActor==NULL )
		{
			AActor* TestActor = GetLevel()->Actors(iActor++);
			if
			(	TestActor
			&&	TestActor->IsA(BaseClass) 
			&&	(TestActor->Location - TraceLocation).SizeSquared() < Square(Radius + TestActor->CollisionRadius) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

void AActor::execVisibleActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execVisibleActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_FLOAT_OPTX(Radius,0.0f);
	P_GET_VECTOR_OPTX(TraceLocation,Location);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iActor=0;
	FCheckResult Hit(1.f);

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<GetLevel()->Actors.Num() && *OutActor==NULL )
		{
			AActor* TestActor = GetLevel()->Actors(iActor++);
			if
			(	TestActor
			&& !TestActor->bHidden
			&&	TestActor->IsA(BaseClass)
			&&	(Radius==0.0f || (TestActor->Location-TraceLocation).SizeSquared() < Square(Radius)) )
			{
				GetLevel()->SingleLineCheck( Hit, this, TestActor->Location, TraceLocation, TRACE_World|TRACE_StopAtFirstHit );
				if ( !Hit.Actor || (Hit.Actor == TestActor) )
					*OutActor = TestActor;
			}
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

void AActor::execVisibleCollidingActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execVisibleCollidingActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_FLOAT(Radius);
	P_GET_VECTOR_OPTX(TraceLocation,Location);
	P_GET_UBOOL_OPTX(bIgnoreHidden, 0); 
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	FMemMark Mark(GMem);
	FCheckResult* Link=GetLevel()->Hash->ActorRadiusCheck( GMem, TraceLocation, Radius, 0 );
	
	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		FCheckResult Hit(1.f);
		if ( Link )
		{
			while ( Link )
			{
				if( !Link->Actor
					||	!Link->Actor->IsA(BaseClass) 
					||  (bIgnoreHidden && Link->Actor->bHidden) )
				{
					Link=Link->GetNext();
				}
				else
				{
					GetLevel()->SingleLineCheck( Hit, this, Link->Actor->Location, TraceLocation, TRACE_World );
					if ( Hit.Actor && (Hit.Actor != Link->Actor) )
						Link=Link->GetNext();
					else
						break;
				}
			}

			if ( Link )
			{
				*OutActor = Link->Actor;
				Link=Link->GetNext();
			}
		}
		if ( *OutActor == NULL ) 
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	Mark.Pop();
	unguardexecSlow;
}

void AActor::execCollidingActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execCollidingActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_FLOAT(Radius);
	P_GET_VECTOR_OPTX(TraceLocation,Location);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	FMemMark Mark(GMem);
	FCheckResult* Link=GetLevel()->Hash->ActorRadiusCheck( GMem, TraceLocation, Radius, 0 );
	
	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		FCheckResult Hit(1.f);
		if ( Link )
		{
			while ( Link )
			{
				if( !Link->Actor
					||	!Link->Actor->IsA(BaseClass) )
				{
					Link=Link->GetNext();
				}
				else
					break;
			}

			if ( Link )
			{
				*OutActor = Link->Actor;
				Link=Link->GetNext();
			}
		}
		if ( *OutActor == NULL ) 
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	Mark.Pop();
	unguardexecSlow;
}

void AZoneInfo::execZoneActors( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AZoneInfo::execZoneActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<GetLevel()->Actors.Num() && *OutActor==NULL )
		{
			AActor* TestActor = GetLevel()->Actors(iActor++);
			if
			(	TestActor
			&&	TestActor->IsA(BaseClass)
			&&	TestActor->IsInZone(this) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}

/*-----------------------------------------------------------------------------
	Script processing function.
-----------------------------------------------------------------------------*/

//
// Execute the state code of the actor.
//
void AActor::ProcessState( FLOAT DeltaSeconds )
{
	if
	(	GetStateFrame()
	&&	GetStateFrame()->Code
	&&	(Role>=ROLE_Authority || (GetStateFrame()->StateNode->StateFlags & STATE_Simulated))
	&&	!IsPendingKill() )
	{
		UState* OldStateNode = GetStateFrame()->StateNode;
		guard(AActor::ProcessState);
		// SCRIPTTIME FLOAT StartTime = GScriptCycles;
		if( ++GScriptEntryTag==1 )
			clock(GScriptCycles);

		// If a latent action is in progress, update it.
		if( GetStateFrame()->LatentAction )
			(this->*GNatives[GetStateFrame()->LatentAction])( *GetStateFrame(), (BYTE*)&DeltaSeconds );

		// Execute code.
		INT NumStates=0;
		while( !bDeleteMe && GetStateFrame()->Code && !GetStateFrame()->LatentAction )
		{
			BYTE Buffer[MAX_CONST_SIZE];
			GetStateFrame()->Step( this, Buffer );
			if( GetStateFrame()->StateNode!=OldStateNode )
			{
				OldStateNode = GetStateFrame()->StateNode;
				if( ++NumStates > 4 )
				{
					//GetStateFrame().Logf( "Pause going from %s to %s", xx, yy );
					break;
				}
			}
		}
		if( --GScriptEntryTag==0 )
		{
			unclock(GScriptCycles);
			/* SCRIPTTIME
			FLOAT PT = (GScriptCycles - StartTime) * GSecondsPerCycle * 1000.0f;
			if ( PT > 0.01f )
			{
				debugf(TEXT("%s processSTATE"),GetName());
				if ( PT > 0.1f )
					debugf(TEXT("***STATE processing time %f"),PT);
				else
					debugf(TEXT("   STATE processing time %f"),PT);
			}
			*/
		}
		unguardf(( TEXT("Object %s, Old State %s, New State %s"), GetFullName(), OldStateNode->GetFullName(), GetStateFrame()->StateNode->GetFullName() ));
	}
}

//
// Internal RPC calling.
//
static inline void InternalProcessRemoteFunction
(
	AActor*			Actor,
	UNetConnection*	Connection,
	UFunction*		Function,
	void*			Parms,
	FFrame*			Stack,
	UBOOL			IsServer
)
{
	guardSlow(InternalProcessRemoteFunction);
	
	GStats.DWORDStats(GEngineStats.STATS_Net_NumRPC)++;
	
	// Make sure this function exists for both parties.
	FClassNetCache* ClassCache = Connection->PackageMap->GetClassNetCache( Actor->GetClass() );
	if( !ClassCache )
		return;
	FFieldNetCache* FieldCache = ClassCache->GetFromField( Function );
	if( !FieldCache )
		return;

	// Get the actor channel.
	UActorChannel* Ch = Connection->ActorChannels.FindRef(Actor);
	if( !Ch )
	{
		if( IsServer )
			Ch = (UActorChannel *)Connection->CreateChannel( CHTYPE_Actor, 1 );
		if( !Ch )
			return;
		if( IsServer )
			Ch->SetChannelActor( Actor );
	}

	// Make sure initial channel-opening replication has taken place.
	if( Ch->OpenPacketId==INDEX_NONE )
	{
		if( !IsServer )
			return;
		Ch->ReplicateActor();
	}

	// Form the RPC preamble.
	FOutBunch Bunch( Ch, 0 );
	//debugf(TEXT("   Call %s"),Function->GetFullName());
	Bunch.WriteInt( FieldCache->FieldNetIndex, ClassCache->GetMaxIndex() );

	// Form the RPC parameters.
	if( Stack )
	{
		appMemzero( Parms, Function->ParmsSize );
        for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
			Stack->Step( Stack->Object, (BYTE*)Parms + It->Offset );
		checkSlow(*Stack->Code==EX_EndFunctionParms);
	}
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
	{
		if( Connection->PackageMap->ObjectToIndex(*It)!=INDEX_NONE )
		{
			UBOOL Send = 1;
			if( !Cast<UBoolProperty>(*It,CLASS_IsAUBoolProperty) )
			{
				Send = !It->Matches(Parms,NULL,0);
				Bunch.WriteBit( Send );
			}
			if( Send )
				It->NetSerializeItem( Bunch, Connection->PackageMap, (BYTE*)Parms + It->Offset );
		}
	}

	// Reliability.
	//warning: RPC's might overflow, preventing reliable functions from getting thorough.
	if( Function->FunctionFlags & FUNC_NetReliable )
		Bunch.bReliable = 1;

	// Send the bunch.
	if( !Bunch.IsError() )
	{
		Ch->SendBunch( &Bunch, 1 );
	}
	else
		debugf( NAME_DevNet, TEXT("RPC bunch overflowed") );

	unguardSlow;
}

//
// Return whether a function should be executed remotely.
//
UBOOL AActor::ProcessRemoteFunction( UFunction* Function, void* Parms, FFrame* Stack )
{
	guard(AActor::ProcessRemoteFunction);

	// Quick reject.
	if( (Function->FunctionFlags & (FUNC_Static | FUNC_Interface)) || bDeleteMe )
		return 0;
	UBOOL Absorb = (Role<=ROLE_SimulatedProxy) && !(Function->FunctionFlags & (FUNC_Simulated | FUNC_Native));
	if( GetLevel()->DemoRecDriver )
	{
		if( GetLevel()->DemoRecDriver->ServerConnection )
			return Absorb;
		ProcessDemoRecFunction( Function, Parms, Stack );
	}
	if( Level->NetMode==NM_Standalone )
		return 0;
	if( !(Function->FunctionFlags & FUNC_Net) )
		return Absorb;

	// Check if the actor can potentially call remote functions.
	// FIXME - cleanup finding top - or make function
    APlayerController* Top = GetTopPlayerController();//Cast<APlayerController>(GetTopOwner());
	UNetConnection* ClientConnection = NULL;
	if
	(	(Role==ROLE_Authority)
	&&	(Top==NULL || (ClientConnection=Cast<UNetConnection>(Top->Player))==NULL) )
		return Absorb;

	// See if UnrealScript replication condition is met.
	while( Function->GetSuperFunction() )
		Function = Function->GetSuperFunction();
	UBOOL Val=0;
	FFrame( this, Function->GetOwnerClass(), Function->RepOffset, NULL ).Step( this, &Val );
	if( !Val )
		return Absorb;

	// Get the connection.
	UBOOL           IsServer   = Level->NetMode==NM_DedicatedServer || Level->NetMode==NM_ListenServer;
	UNetConnection* Connection = IsServer ? ClientConnection : GetLevel()->NetDriver->ServerConnection;
	check(Connection);

	// If saturated and function is unimportant, skip it.
	if( !(Function->FunctionFlags & FUNC_NetReliable) && !Connection->IsNetReady(0) )
		return 1;

	// Send function data to remote.
	InternalProcessRemoteFunction( this, Connection, Function, Parms, Stack, IsServer );
	return 1;

	unguardf(( TEXT("(%s)"), Function->GetFullName() ));
}

// Replicate a function call to a demo recording file
void AActor::ProcessDemoRecFunction( UFunction* Function, void* Parms, FFrame* Stack )
{
	guard(AActor::ProcessDemoRecFunction);

	// Check if the function is replicatable
	if( (Function->FunctionFlags & (FUNC_Static|FUNC_Net))!=FUNC_Net || bNetTemporary )
		return;

	UBOOL IsNetClient = (Level->NetMode == NM_Client);

	// Check if actor was spawned locally in a client-side demo 
	if(IsNetClient && Role == ROLE_Authority)
		return;

	// See if UnrealScript replication condition is met.
	while( Function->GetSuperFunction() )
		Function = Function->GetSuperFunction();

	UBOOL Val=0;
	if(IsNetClient)
		Exchange(RemoteRole, Role);
	bDemoRecording = 1;
	bClientDemoRecording = IsNetClient;
	FFrame( this, Function->GetOwnerClass(), Function->RepOffset, NULL ).Step( this, &Val );
	bDemoRecording = 0;
	bClientDemoRecording = 0;
	if(IsNetClient)
		Exchange(RemoteRole, Role);
	bClientDemoNetFunc = 0;
	if( !Val )
		return;

	// Get the connection.
	if( !GetLevel()->DemoRecDriver->ClientConnections.Num() )
		return;
	UNetConnection* Connection = GetLevel()->DemoRecDriver->ClientConnections(0);
	if(!Connection )
		return;

	// Send function data to remote.
	BYTE* SavedCode = Stack ? Stack->Code : NULL;
	InternalProcessRemoteFunction( this, Connection, Function, Parms, Stack, 1 );
	if( Stack )
		Stack->Code = SavedCode;

	unguardf(( TEXT("(%s/%s)"), GetName(), Function->GetFullName() ));
}

/*-----------------------------------------------------------------------------
	GameInfo
-----------------------------------------------------------------------------*/

//
// Network
//
void AGameInfo::execGetNetworkNumber( FFrame& Stack, RESULT_DECL )
{
	guard(AGameInfo::execNetworkNumber);
	P_FINISH;

	*(FString*)Result = XLevel->NetDriver ? XLevel->NetDriver->LowLevelGetNetworkNumber() : FString(TEXT(""));

	unguardexec;
}

//
// Deathmessage parsing.
//
void AGameInfo::execParseKillMessage( FFrame& Stack, RESULT_DECL )
{
	guard(AGameInfo::execParseKillMessage);
	P_GET_STR(KillerName);
	P_GET_STR(VictimName);
	P_GET_STR(KillMessage);
	P_FINISH;

	FString Message, Temp;
	INT Offset;

	Temp = KillMessage;

	Offset = Temp.InStr(TEXT("%k"));
	if (Offset != -1)
	{
		Message = Temp.Left(Offset);
		Message += KillerName;
		Message += Temp.Right(Temp.Len() - Offset - 2);
		Temp = Message;
	}

	Offset = Temp.InStr(TEXT("%o"));
	if (Offset != -1)
	{
		Message = Temp.Left(Offset);
		Message += VictimName;
		Message += Temp.Right(Temp.Len() - Offset - 2);
	}

	*(FString*)Result = Message;

	unguardexec;
}

// Color functions
#define P_GET_COLOR(var)            P_GET_STRUCT(FColor,var)

void AActor::execMultiply_ColorFloat( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execMultiply_ColorFloat);

	P_GET_COLOR(A);
	P_GET_FLOAT(B);
	P_FINISH;

	A.R = (BYTE) (A.R * B);
	A.G = (BYTE) (A.G * B);
	A.B = (BYTE) (A.B * B);
	*(FColor*)Result = A;

	unguardexecSlow;
}	

void AActor::execMultiply_FloatColor( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execMultiply_FloatColor);

	P_GET_FLOAT (A);
	P_GET_COLOR(B);
	P_FINISH;

	B.R = (BYTE) (B.R * A);
	B.G = (BYTE) (B.G * A);
	B.B = (BYTE) (B.B * A);
	*(FColor*)Result = B;

	unguardexecSlow;
}	

void AActor::execAdd_ColorColor( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAdd_ColorColor);

	P_GET_COLOR(A);
	P_GET_COLOR(B);
	P_FINISH;

	A.R = A.R + B.R;
	A.G = A.G + B.G;
	A.B = A.B + B.B;
	*(FColor*)Result = A;

	unguardexecSlow;
}

void AActor::execSubtract_ColorColor( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSubtract_ColorColor);

	P_GET_COLOR(A);
	P_GET_COLOR(B);
	P_FINISH;

	A.R = A.R - B.R;
	A.G = A.G - B.G;
	A.B = A.B - B.B;
	*(FColor*)Result = A;

	unguardexecSlow;
}

void AVolume::execEncompasses( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execEncompasses);

	P_GET_ACTOR(InActor);
	P_FINISH;

	*(DWORD*)Result = Encompasses(InActor->Location);
	unguardexecSlow;
}

/*---------------------------------------------------------------------------------------
	Projectors
---------------------------------------------------------------------------------------*/

void AActor::AttachProjector( AProjector* Projector )
{
	// Remove old projectors.

	for(INT ProjectorIndex = 0;ProjectorIndex < Projectors.Num();ProjectorIndex++)
		if(!Projectors(ProjectorIndex)->Render( Level->TimeSeconds ))
			Projectors.Remove(ProjectorIndex--);

	// Add the new projector.

	Projectors.AddUniqueItem( Projector->RenderInfo->AddReference() );
}

void AActor::DetachProjector( AProjector* Projector )
{
	Projector->RenderInfo->RemoveReference();
	Projectors.RemoveItem( Projector->RenderInfo );
}

//
// AActor::GetProjectorBase - who's projector list to use.
//
AActor* AActor::GetProjectorBase()
{
	if( bCollideActors || !bAcceptsProjectors )
		return this;

	if( Base )
		return Base->GetProjectorBase();

	return this;
}


//
// Get collision bounding box of actor's primitive
//
FBox AActor::GetCollisionBoundingBox()
{
	guard(AActor::GetRenderBoundingSphere);

	FMatrix	_LocalToWorld;

	if(DrawType == DT_Mesh && Mesh)
		_LocalToWorld = Mesh->MeshGetInstance(this)->MeshToWorld();
	else
		_LocalToWorld = LocalToWorld();

	return GetPrimitive()->GetCollisionBoundingBox(this).TransformBy(_LocalToWorld);

	unguard;
}


void AActor::execGetCollisionBoundingBox( FFrame& Stack, RESULT_DECL )
{
	guard(AActor::execGetRenderBoundingSphere);
	P_FINISH;

	*(FBox*)Result = GetCollisionBoundingBox();

	unguardexec;
}


//
//	AActor::execGetRenderBoundingSphere
//

void AActor::execGetRenderBoundingSphere(FFrame& Stack,RESULT_DECL)
{
	guard(AActor::execGetRenderBoundingSphere);

	P_FINISH;

	FMatrix	_LocalToWorld;

	if(DrawType == DT_Mesh && Mesh)
		_LocalToWorld = Mesh->MeshGetInstance(this)->MeshToWorld();
	else
		_LocalToWorld = LocalToWorld();

	*(FSphere*)Result = GetPrimitive()->GetRenderBoundingSphere(this).TransformBy(_LocalToWorld);

	unguardexec;
}

//
//	AActor::execDrawDebugLine
//

void AActor::execDrawDebugLine(FFrame& Stack,RESULT_DECL)
{
	guard(AActor::execDrawDebugLine);

	P_GET_VECTOR(LineStart);
	P_GET_VECTOR(LineEnd);
	P_GET_BYTE(R);
	P_GET_BYTE(G);
	P_GET_BYTE(B);
	P_FINISH;

	GTempLineBatcher->AddLine(LineStart, LineEnd, FColor(R, G, B));

	unguard;
}

// jdf ---

void AActor::execPlayFeedbackEffect( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execPlayFeedbackEffect);

	P_GET_STR(EffectName);
	P_FINISH;

    PlayFeedbackEffect( TCHAR_TO_ANSI(*EffectName) );

    unguardSlow;
}

void AActor::execStopFeedbackEffect( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execStopFeedbackEffect);

    P_GET_STR_OPTX(EffectName, TEXT("@#$%!"));
	P_FINISH;

    if( EffectName.Len() )
        StopFeedbackEffect( EffectName!=TEXT("@#$%!") ? TCHAR_TO_ANSI(*EffectName) : NULL );

    unguardSlow;
}

void AActor::execForceFeedbackSupported( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execFeedbackEffectSupported);

	P_GET_UBOOL_OPTX(Enable, 1);
	P_FINISH;
	void* hWnd = NULL;

	if (Enable)
	{
		TObjectIterator<UEngine> EngineIt;
		if
		(	EngineIt
		&&	EngineIt->Client
		&&	EngineIt->Client->Viewports.Num() )
			hWnd = EngineIt->Client->Viewports(0)->GetWindow();

		InitForceFeedback(hInstance, hWnd);
	}
	else
		ExitForceFeedback();

    *(UBOOL*)Result = (UBOOL)GForceFeedbackAvailable;

    unguardSlow;
}
// --- jdf

/*---------------------------------------------------------------------------------------
	CD Key Validation: MasterServer -> GameServer -> Client -> GameServer -> MasterServer
---------------------------------------------------------------------------------------*/

//
// Request a CD Key challenge response from the client.
//
void APlayerController::execClientValidate(FFrame& Stack,RESULT_DECL)
{
	guard(APlayerController::execClientValidate);
	P_GET_STR( Challenge );
	P_FINISH;
//	eventServerValidationResponse( GetCDKeyHash(),  GetCDKeyResponse(*Challenge) );
	unguard;
}

//
// CD Key Challenge response received from the client.
//
void APlayerController::execServerValidationResponse(FFrame& Stack,RESULT_DECL)
{
	guard(APlayerController::execServerValidationResponse);
	P_GET_STR( CDKeyHash );
	P_GET_STR( Response );
	P_FINISH;
	UNetConnection* Conn = Cast<UNetConnection>(Player);
	if( Conn )
	{
		// Store it in the connection.  The uplink will pick this up.
		Conn->CDKeyHash     = CDKeyHash;
		Conn->CDKeyResponse = Response;
	}
	unguard;
}

void AActor::execClock(FFrame& Stack,RESULT_DECL)
{
	guard(AActor::execClock);
	P_GET_FLOAT_REF( Time );
	P_FINISH;
	clock(*Time);
	unguard;
}

void AActor::execUnClock(FFrame& Stack,RESULT_DECL)
{
	guard(AActor::execUnClock);
	P_GET_FLOAT_REF( Time );
	P_FINISH;
	unclock(*Time);
	*Time = *Time * GSecondsPerCycle * 1000.f;
	unguard;
}

#if 1 //NEW: (mdf) make it possible to toggle bStatic for summoninng
void AActor::execSetClassStatic( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execSetClassStatic);

	P_GET_OBJECT(UClass,TargetClass);
	P_GET_UBOOL(bVal);
	P_FINISH;

	TargetClass->GetDefaultActor()->bStatic = bVal;

	unguardexecSlow;
}
#endif

//------------------------------------------------------------------------------
void ALevelInfo::execRegisterExternalForce( FFrame& Stack, RESULT_DECL )
{
	guard(ALevelInfo::execRegisterExternalForce);

	P_GET_OBJECT(UObject,Force);
	P_FINISH;

	RegisterExternalForce( Force );

	unguardexec;
}

//------------------------------------------------------------------------------
void ALevelInfo::execUnRegisterExternalForce( FFrame& Stack, RESULT_DECL )
{
	guard(ALevelInfo::execUnRegisterExternalForce);

	P_GET_OBJECT(UObject,Force);
	P_FINISH;

	UnRegisterExternalForce( Force );

	unguardexec;
}

//------------------------------------------------------------------------------
void ALevelInfo::execRegisterAffectedTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(ALevelInfo::execRegisterAffectedTemplate);

	P_GET_OBJECT(UObject,ParticleTemplate);
	P_FINISH;

	RegisterAffectedTemplate( ParticleTemplate );

	unguardexec;
}

//------------------------------------------------------------------------------
void ALevelInfo::execUnRegisterAffectedTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(ALevelInfo::execUnRegisterAffectedTemplate);

	P_GET_OBJECT(UObject,ParticleTemplate);
	P_FINISH;

	UnRegisterAffectedTemplate( ParticleTemplate );

	unguardexec;
}

//------------------------------------------------------------------------------
struct FuncParms{ class UObject* A; };
//------------------------------------------------------------------------------
void ALevelInfo::RegisterExternalForce( UObject* Force )
{
	guard(ALevelInfo::RegisterExternalForce);
	//debugf(TEXT("ALevelInfo::RegisterExternalForce( %s )"), Force ? Force->GetFullName() : TEXT("None") );

	if( !Force ) return;	// error checking.

	if( !Force->IsIn(GetOuter()) )
		//appErrorf(TEXT("Attempt to register external force!  '%s'"),Force->GetFullName());
		return;

	ExternalForces.AddItem( Force );

//	AActor* System = CastChecked<AActor>(Force->GetOuter());
//	UBOOL bParticleAffector = System->IsA(TEXT("ParticleAffector"));
//	FName AffectorTag = System->Event;

	// Add [to] all AffectedTemplates.
	for( INT i=0; i<AffectedTemplates.Num(); i++ )
	{
		UObject* Template = AffectedTemplates(i);
		FuncParms Parms;
		Parms.A = Template;
//		if( !bParticleAffector || AffectorTag==CastChecked<AActor>(Template->GetOuter())->Tag )
			Force->ProcessEvent(Force->FindFunctionChecked(FName(TEXT("AddExternalTemplate"),FNAME_Intrinsic)),&Parms);
	}

	unguard;
}

//------------------------------------------------------------------------------
void ALevelInfo::UnRegisterExternalForce( UObject* Force )
{
	guard(ALevelInfo::UnRegisterExternalForce);
	//debugf(TEXT("ALevelInfo::UnRegisterExternalForce( %s )"), Force ? Force->GetFullName() : TEXT("None") );

	if( !Force ) return;	// error checking.

	ExternalForces.RemoveItem( Force );

	// Note[aleiby]: Make sure you remove the Force from its System so the affected templates also get removed correctly.

  unguard;
}

//------------------------------------------------------------------------------
void ALevelInfo::RegisterAffectedTemplate( UObject* ParticleTemplate )
{
	guard(ALevelInfo::RegisterAffectedTemplate);
	//debugf(TEXT("ALevelInfo::RegisterAffectedTemplate( %s )"), ParticleTemplate ? ParticleTemplate->GetFullName() : TEXT("None") );

	if( !ParticleTemplate ) return;	// error checking.

	// Fix ARL: Should never happen, but it does -- see comments in ParticleGenerator.uc
	if( !ParticleTemplate->IsIn(GetOuter()) )
		//appErrorf(TEXT("Attempt to register external template!  '%s'"),ParticleTemplate->GetFullName());
		return;

	AffectedTemplates.AddItem( ParticleTemplate );

//	AActor* AffectedSystem = CastChecked<AActor>(ParticleTemplate->GetOuter());
//	FName AffectedTag = AffectedSystem->Tag;

	// Add [to] all external forces.
	for( INT i=0; i<ExternalForces.Num(); i++ )
	{
		UObject* Force = ExternalForces(i);
		FuncParms Parms;
		Parms.A = ParticleTemplate;

//		AActor* System = CastChecked<AActor>(Force->GetOuter());
//		UBOOL bParticleAffector = System->IsA(TEXT("ParticleAffector"));

//		if( !bParticleAffector || AffectedTag==System->Event )
			Force->ProcessEvent(Force->FindFunctionChecked(FName(TEXT("AddExternalTemplate"),FNAME_Intrinsic)),&Parms);
	}

	unguard;
}

//------------------------------------------------------------------------------
void ALevelInfo::UnRegisterAffectedTemplate( UObject* ParticleTemplate )
{
	guard(ALevelInfo::UnRegisterAffectedTemplate);
	//debugf(TEXT("ALevelInfo::UnRegisterAffectedTemplate( %s )"), ParticleTemplate ? ParticleTemplate->GetFullName() : TEXT("None") );

	if( !ParticleTemplate ) return;	// error checking.

	AffectedTemplates.RemoveItem( ParticleTemplate );

	// Remove [from] all external forces.
	for( INT i=0; i<ExternalForces.Num(); i++ )
	{
		UObject* Force = ExternalForces(i);
		FuncParms Parms;
		Parms.A = ParticleTemplate;
		Force->ProcessEvent(Force->FindFunctionChecked(FName(TEXT("RemoveExternalTemplate"),FNAME_Intrinsic)),&Parms);
	}

	unguard;
}

#if 1 //NEW (mdf) line rendering
static INT ScriptLinesKey = -1;

void AActor::execAddLine( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAddLine);

	P_GET_VECTOR(Start);
	P_GET_VECTOR(End);
	P_GET_STRUCT_OPTX(FColor,Color,DEFAULTSCRIPTLINECOLOR);
	P_FINISH;

	if( ScriptLinesKey == -1 )
		 ScriptLinesKey = FLineManager::GetInstance()->AllocateLineList( true );
	FLineManager::GetInstance()->AddLine( ScriptLinesKey, Start, End, Color.Plane() );

	unguardexecSlow;
}

void AActor::execAddArrow( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAddArrow);

	P_GET_VECTOR(Start);
	P_GET_VECTOR(End);
	P_GET_STRUCT_OPTX(FColor,Color,DEFAULTSCRIPTLINECOLOR);
	P_FINISH;

	if( ScriptLinesKey == -1 )
		 ScriptLinesKey = FLineManager::GetInstance()->AllocateLineList( true );
	FLineManager::GetInstance()->AddArrow( ScriptLinesKey, Start, End, Color.Plane(), Color.Plane() );

	unguardexecSlow;
}

void AActor::execAddBox( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAddBox);

	P_GET_VECTOR(Location);
	P_GET_VECTOR(Extent);
	P_GET_STRUCT_OPTX(FColor,Color,DEFAULTSCRIPTLINECOLOR);
	P_FINISH;

	if( ScriptLinesKey == -1 )
		 ScriptLinesKey = FLineManager::GetInstance()->AllocateLineList( true );
	FLineManager::GetInstance()->AddBox( ScriptLinesKey, Location, Extent, Color.Plane() );


	unguardexecSlow;
}

void AActor::execAddCylinder( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAddCylinder);

	P_GET_VECTOR(Location);
	P_GET_FLOAT(Radius);
	P_GET_FLOAT(Height);
	P_GET_STRUCT_OPTX(FColor,Color,DEFAULTSCRIPTLINECOLOR);
	P_FINISH;

	if( ScriptLinesKey == -1 )
		 ScriptLinesKey = FLineManager::GetInstance()->AllocateLineList( true );
	FLineManager::GetInstance()->AddCylinder( ScriptLinesKey, Location, Radius, Height, Color.Plane() );

	unguardexecSlow;
}

void AActor::execAddActor( FFrame& Stack, RESULT_DECL )
{
	guardSlow(AActor::execAddActor);

	P_GET_ACTOR(Actor);
	P_GET_STRUCT_OPTX(FColor,Color,DEFAULTSCRIPTLINECOLOR);
	P_FINISH;

	if( ScriptLinesKey == -1 )
		 ScriptLinesKey = FLineManager::GetInstance()->AllocateLineList( true );
	FLineManager::GetInstance()->AddActor( ScriptLinesKey, *Actor, Color.Plane() );

	unguardexecSlow;
}
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

