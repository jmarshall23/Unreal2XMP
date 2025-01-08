/*=============================================================================
	UnEdSrv.cpp: UUnrealEdEngine implementation, the Unreal editing server
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "UnrealEd.h"
#include "UnPath.h"

UModel* GBrush = NULL;
const TCHAR* GStream = NULL;
TCHAR TempStr[MAX_EDCMD], TempFname[MAX_EDCMD], TempName[MAX_EDCMD], Temp[MAX_EDCMD];
_WORD Word1, Word2, Word4;

extern FVector GPivotLocation, GSnappedLocation, GGridBase;
extern FString GMapExt;


UBOOL UUnrealEdEngine::Exec( const TCHAR* Stream, FOutputDevice& Ar )
{
	guard(UUnrealEdEngine::Exec);

	if( UEditorEngine::Exec( Stream, Ar ) )
		return 1;

	const TCHAR* Str = Stream;

	//----------------------------------------------------------------------------------
	// EDIT
	//
	if( ParseCommand(&Str,TEXT("EDIT")) )
	{
		if( Exec_Edit( Str, Ar ) )
			return 1;
	}
	//------------------------------------------------------------------------------------
	// ACTOR: Actor-related functions
	//
	else if (ParseCommand(&Str,TEXT("ACTOR")))
	{
		if( Exec_Actor( Str, Ar ) )
			return 1;
	}
	//------------------------------------------------------------------------------------
	// PREFAB management:
	//
	else if( ParseCommand(&Str,TEXT("Prefab")) )
	{
		if( Exec_Prefab( Str, Ar ) )
			return 1;
	}
	//------------------------------------------------------------------------------------
	// MODE management (Global EDITOR mode):
	//
	else if( ParseCommand(&Str,TEXT("MODE")) )
	{
		if( Exec_Mode( Str, Ar ) )
			return 1;
	}
	//------------------------------------------------------------------------------------
	// SCRIPT: script compiler
	//
	else if( ParseCommand(&Str,TEXT("SCRIPT")) )
	{
		if( Exec_Script( Str, Ar ) )
			return 1;
	}
	//----------------------------------------------------------------------------------
	// PIVOT
	//
	else if( ParseCommand(&Str,TEXT("PIVOT")) )
	{
		if( Exec_Pivot( Str, Ar ) )
			return 1;
	}
	else if( ParseCommand(&Str,TEXT("MAYBEAUTOSAVE")) )
	{
		if( AutoSave && ++AutoSaveCount>=AutosaveTimeMinutes )
		{
			AutoSaveIndex = (AutoSaveIndex+1)%10;
			SaveConfig();
			TCHAR Cmd[MAX_EDCMD];
			appSprintf( Cmd, TEXT("MAP SAVE AUTOSAVE=1 FILE=\"%s%s%s%s%s%sAuto%i.%s\""), appBaseDir(), PATH_SEPARATOR, TEXT(".."), PATH_SEPARATOR, TEXT("Maps"), PATH_SEPARATOR, AutoSaveIndex, *GMapExt );
			debugf( NAME_Log, TEXT("Autosaving '%s'"), Cmd );
			Exec( Cmd, Ar );
			AutoSaveCount=0;
		}
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("PREFERENCES")) )
	{
		ShowPreferences();
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("RENDEREMULATE")) )
	{
		if( ParseCommand(&Str,TEXT("gf1")) )
			GEditor->GRenDev->SetEmulationMode( HEM_GeForce1 );
		else
		if( ParseCommand(&Str,TEXT("gf2")) )
			GEditor->GRenDev->SetEmulationMode( HEM_GeForce1 );
		else
		if( ParseCommand(&Str,TEXT("xbox")) )
			GEditor->GRenDev->SetEmulationMode( HEM_XBox );
		else
			GEditor->GRenDev->SetEmulationMode( HEM_None );
		return 1;
	}

	return 0;
	unguard;
}

UBOOL UUnrealEdEngine::Exec_Edit( const TCHAR* Str, FOutputDevice& Ar )
{
	guard(UUnrealEdEngine::Exec_Edit);

	if( ParseCommand(&Str,TEXT("CUT")) )
	{
		Trans->Begin( TEXT("Cut") );
		edactCopySelected( Level );
		edactDeleteSelected( Level );
		Trans->End();
		RedrawLevel( Level );
	}
	else if( ParseCommand(&Str,TEXT("COPY")) )
	{
		edactCopySelected( Level );
	}
	else if( ParseCommand(&Str,TEXT("PASTE")) )
	{
		enum ePasteTo
		{
			ePASTETO_OriginalLocation	= 0,
			ePASTETO_Here				= 1,
			ePASTETO_WorldOrigin		= 2
		} PasteTo = ePASTETO_OriginalLocation;

		FString TransName = TEXT("Paste");
		if( Parse( Str, TEXT("TO="), TempStr, 15 ) )
		{
			if( !appStrcmp( TempStr, TEXT("HERE") ) )
			{
				PasteTo = ePASTETO_Here;
				TransName = TEXT("Paste Here");
			}
			else
				if( !appStrcmp( TempStr, TEXT("ORIGIN") ) )
				{
					PasteTo = ePASTETO_WorldOrigin;
					TransName = TEXT("Paste to World Origin");
				}
		}

		Trans->Begin( *TransName );

		GEditor->SelectNone( Level, 1, 0 );
		edactPasteSelected( Level, 0 );

		if( PasteTo != ePASTETO_OriginalLocation )
		{
			// Figure out which location to center the actors around.
			FVector Origin = FVector(0,0,0);
			if( PasteTo == ePASTETO_Here )
				Origin = GEditor->ClickLocation;

			// Get a bounding box for all the selected actors locations.
			FBox bbox(0);
			for( INT i=0; i<Level->Actors.Num(); i++ )
				if( Level->Actors(i) && Level->Actors(i)->bSelected )
					bbox += Level->Actors(i)->Location;

			// Compute how far the actors have to move.
			FVector Location = bbox.GetCenter(),
				Adjust = Origin - Location;

			// Move the actors
			for( INT i=0; i<Level->Actors.Num(); i++ )
				if( Level->Actors(i) && Level->Actors(i)->bSelected )
					Level->Actors(i)->Location += Adjust;
		}

		Trans->End();
		RedrawLevel( Level );
	}

	return 0;

	unguard;
}

UBOOL UUnrealEdEngine::Exec_Pivot( const TCHAR* Str, FOutputDevice& Ar )
{
	guard(UUnrealEdEngine::Exec_Pivot);

	if( ParseCommand(&Str,TEXT("HERE")) )
	{
		NoteActorMovement( Level );
		SetPivot( ClickLocation, 0, 0, 0 );
		FinishAllSnaps( Level );
		RedrawLevel( Level );
	}
	else if( ParseCommand(&Str,TEXT("SNAPPED")) )
	{
		NoteActorMovement( Level );
		SetPivot( ClickLocation, 1, 0, 0 );
		FinishAllSnaps( Level );
		RedrawLevel( Level );
	}

	return 0;

	unguard;
}


UBOOL UUnrealEdEngine::Exec_Actor( const TCHAR* Str, FOutputDevice& Ar )
{
	guard(UUnrealEdEngine::Exec_Actor);

	if( ParseCommand(&Str,TEXT("ADD")) )
	{
		UClass* Class;
		if( ParseObject<UClass>( Str, TEXT("CLASS="), Class, ANY_PACKAGE ) )
		{
			AActor* Default   = Class->GetDefaultActor();
			FVector Collision = FVector(Default->CollisionRadius,Default->CollisionRadius,Default->CollisionHeight);
			INT bSnap;
			Parse(Str,TEXT("SNAP="),bSnap);
			if( bSnap )		Constraints.Snap( ClickLocation, FVector(0, 0, 0) );
			FVector Location = ClickLocation + ClickPlane * (FBoxPushOut(ClickPlane,Collision) + 0.1);
			if( bSnap )		Constraints.Snap( Location, FVector(0, 0, 0) );
			AActor* Actor = AddActor( Level, Class, Location );
			UTexture* Texture;
			if( ParseObject<UTexture>( Str, TEXT("TEXTURE="), Texture, ANY_PACKAGE ) )
				Actor->Texture = Texture;
			RedrawLevel(Level);
			return 1;
		}
	}
	else if( ParseCommand(&Str,TEXT("MIRROR")) )
	{
		Trans->Begin( TEXT("Mirroring Actors") );

		FVector V( 1, 1, 1 );
		GetFVECTOR( Str, V );

		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && Actor->bSelected )
			{
				FCoords ToWorld = GMath.UnitCoords * Actor->Location * -(GPivotLocation + Actor->PrePivot);
				FCoords ToLocal = GMath.UnitCoords / -(GPivotLocation + Actor->PrePivot) / Actor->Location;

				ABrush* Brush = Cast<ABrush>(Actor);

				if( Brush )
				{
					Brush->Brush->Modify();

					for( INT poly = 0 ; poly < Brush->Brush->Polys->Element.Num() ; poly++ )
					{
						FPoly* Poly = &(Brush->Brush->Polys->Element(poly));
						Brush->Brush->Polys->Element.ModifyAllItems();

						Poly->TextureU *= V;
						Poly->TextureV *= V;

						Poly->Base = Poly->Base.TransformPointBy( ToWorld );
						Poly->Base *= V;
						Poly->Base = Poly->Base.TransformPointBy( ToLocal );

						for( INT vtx = 0 ; vtx < Poly->NumVertices ; vtx++ )
						{
							Poly->Vertex[vtx] = Poly->Vertex[vtx].TransformPointBy( ToWorld );
							Poly->Vertex[vtx] *= V;
							Poly->Vertex[vtx] = Poly->Vertex[vtx].TransformPointBy( ToLocal );
						}

						Poly->Reverse();
						Poly->CalcNormal();
					}

					Brush->Brush->BuildBound();
				}
				else
				{
					Actor->Modify();

					Actor->Location -= GPivotLocation - Actor->PrePivot;
					Actor->Location *= V;
					Actor->Location += GPivotLocation - Actor->PrePivot;

					// Adjust the rotation as best we can.
					if( V.X != 1 )	Actor->Rotation.Yaw += (32768 * V.X);
					if( V.Y != 1 )	Actor->Rotation.Yaw += (32768 * V.Y);
					if( V.Z != 1 )	Actor->Rotation.Pitch += (32768 * V.Z);
				}
			}
		}

		Trans->End();
		RedrawLevel(Level);
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("HIDE")) )
	{
		if( ParseCommand(&Str,TEXT("SELECTED")) ) // ACTOR HIDE SELECTED
		{
			Trans->Begin( TEXT("Hide Selected") );
			Level->Modify();
			edactHideSelected( Level );
			Trans->End();
			SelectNone( Level, 0 );
			NoteSelectionChange( Level );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("UNSELECTED")) ) // ACTOR HIDE UNSELECTEED
		{
			Trans->Begin( TEXT("Hide Unselected") );
			Level->Modify();
			edactHideUnselected( Level );
			Trans->End();
			SelectNone( Level, 0 );
			NoteSelectionChange( Level );
			return 1;
		}
	}
	else if( ParseCommand(&Str,TEXT("UNHIDE")) ) // ACTOR UNHIDE ALL
	{
		// "ACTOR UNHIDE ALL" = "Drawing Region: Off": also disables the far (Z) clipping plane
		ResetZClipping();
		Trans->Begin( TEXT("UnHide All") );
		Level->Modify();
		edactUnHideAll( Level );
		Trans->End();
		NoteSelectionChange( Level );
		return 1;
	}
	else if( ParseCommand(&Str, TEXT("APPLYTRANSFORM")) )
	{
		Trans->Begin( TEXT("Apply brush transform") );
		Level->Modify();
		edactApplyTransform( Level );
		Trans->End();
		RedrawLevel( Level );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("CLIP")) ) // ACTOR CLIP Z/XY/XYZ
	{
		if( ParseCommand(&Str,TEXT("Z")) )
		{
			SetZClipping();
			RedrawLevel( Level );
			return 1;
		}
	}
	else if( ParseCommand(&Str, TEXT("REPLACE")) )
	{
		UClass* Class;
		if( ParseCommand(&Str, TEXT("BRUSH")) ) // ACTOR REPLACE BRUSH
		{
			Trans->Begin( TEXT("Replace selected brush actors") );
			Level->Modify();
			edactReplaceSelectedBrush( Level );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
		else if( ParseObject<UClass>( Str, TEXT("CLASS="), Class, ANY_PACKAGE ) ) // ACTOR REPLACE CLASS=<class>
		{
			Trans->Begin( TEXT("Replace selected non-brush actors") );
			Level->Modify();
			edactReplaceSelectedWithClass( Level, Class );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
	}
	else if( ParseCommand(&Str,TEXT("SELECT")) )
	{
		if( ParseCommand(&Str,TEXT("NONE")) ) // ACTOR SELECT NONE
		{
			return Exec( TEXT("SELECT NONE") );
		}
		else if( ParseCommand(&Str,TEXT("ALL")) ) // ACTOR SELECT ALL
		{
			Trans->Begin( TEXT("Select All") );
			Level->Modify();
			edactSelectAll( Level );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("INSIDE") ) ) // ACTOR SELECT INSIDE
		{
			Trans->Begin( TEXT("Select Inside") );
			Level->Modify();
			edactSelectInside( Level );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("INVERT") ) ) // ACTOR SELECT INVERT
		{
			Trans->Begin( TEXT("Select Invert") );
			Level->Modify();
			edactSelectInvert( Level );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("OFCLASS")) ) // ACTOR SELECT OFCLASS CLASS=<class>
		{
			UClass* Class;
			if( ParseObject<UClass>(Str,TEXT("CLASS="),Class,ANY_PACKAGE) )
			{
				Trans->Begin( TEXT("Select of class") );
				Level->Modify();
				edactSelectOfClass( Level, Class );
				Trans->End();
				NoteSelectionChange( Level );
			}
			else Ar.Log( NAME_ExecWarning, TEXT("Missing class") );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("OFSUBCLASS")) ) // ACTOR SELECT OFSUBCLASS CLASS=<class>
		{
			UClass* Class;
			if( ParseObject<UClass>(Str,TEXT("CLASS="),Class,ANY_PACKAGE) )
			{
				Trans->Begin( TEXT("Select subclass of class") );
				Level->Modify();
				edactSelectSubclassOf( Level, Class );
				Trans->End();
				NoteSelectionChange( Level );
			}
			else Ar.Log( NAME_ExecWarning, TEXT("Missing class") );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("DELETED")) ) // ACTOR SELECT DELETED
		{
			Trans->Begin( TEXT("Select deleted") );
			Level->Modify();
			edactSelectDeleted( Level );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("MATCHINGSTATICMESH")) ) // ACTOR SELECT MATCHINGSTATICMESH
		{
			Trans->Begin( TEXT("Select Matching Static Mesh") );
			Level->Modify();
			edactSelectMatchingStaticMesh( Level );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("MATCHINGZONE")) ) // ACTOR SELECT MATCHINGZONE
		{
			Trans->Begin( TEXT("Select All Actors in Matching Zone") );
			Level->Modify();
			edactSelectMatchingZone( Level );
			Trans->End();
			NoteSelectionChange( Level );
			return 1;
		}
	}
	else if( ParseCommand(&Str,TEXT("DELETE")) )
	{
		Trans->Begin( TEXT("Delete Actors") );
		Level->Modify();
		edactDeleteSelected( Level );
		Trans->End();
		NoteSelectionChange( Level );
		GEditor->RedrawLevel( GEditor->Level );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("UPDATE")) )
	{
		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor=Level->Actors(i);
			if( Actor && Actor->bSelected )
				Actor->PostEditChange();
		}
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("SET")) )
	{
		Trans->Begin( TEXT("Set Actors") );

		UViewport* CurrentViewport = GetCurrentViewport();
		if( !CurrentViewport )
			debugf(TEXT("No current viewport."));
		else
		{
			for( INT i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor=Level->Actors(i);
				if( Actor && Actor->bSelected )
				{
					Actor->Modify();
					Actor->Location = CurrentViewport->Actor->Location;
					Actor->Rotation = FRotator( CurrentViewport->Actor->Rotation.Pitch, CurrentViewport->Actor->Rotation.Yaw, Actor->Rotation.Roll );
				}
			}
		}

		Trans->End();
		RedrawLevel( Level );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("RESET")) )
	{
		Trans->Begin( TEXT("Reset Actors") );
		Level->Modify();
		UBOOL Location=0;
		UBOOL Pivot=0;
		UBOOL Rotation=0;
		UBOOL Scale=0;
		if( ParseCommand(&Str,TEXT("LOCATION")) )
		{
			Location=1;
			ResetPivot();
		}
		else if( ParseCommand(&Str, TEXT("PIVOT")) )
		{
			Pivot=1;
			ResetPivot();
		}
		else if( ParseCommand(&Str,TEXT("ROTATION")) )
		{
			Rotation=1;
		}
		else if( ParseCommand(&Str,TEXT("SCALE")) )
		{
			Scale=1;
		}
		else if( ParseCommand(&Str,TEXT("ALL")) )
		{
			Location=Rotation=Scale=1;
			ResetPivot();
		}
		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor=Level->Actors(i);
			if( Actor && Actor->bSelected )
			{
				Actor->Modify();
				if( Location ) Actor->Location  = FVector(0.f,0.f,0.f);
				if( Location ) Actor->PrePivot  = FVector(0.f,0.f,0.f);
				if( Pivot && Cast<ABrush>(Actor) )
				{
					ABrush* Brush = Cast<ABrush>(Actor);
					FModelCoords Coords, Uncoords;
					Brush->BuildCoords( &Coords, &Uncoords );
					Brush->Location -= Brush->PrePivot.TransformVectorBy( Coords.PointXform );
					Brush->PrePivot = FVector(0.f,0.f,0.f);
					Brush->PostEditChange();
				}
				if( Scale    ) Actor->DrawScale = 1.0f;
			}
		}
		Trans->End();
		RedrawLevel( Level );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("DUPLICATE")) )
	{
		Trans->Begin( TEXT("Duplicate Actors") );
		edactCopySelected( Level );
		GEditor->SelectNone( Level, 1, 0 );
		edactPasteSelected( Level, 1 );
		Trans->End();
		RedrawLevel( Level );
		return 1;
	}
	else if( ParseCommand(&Str, TEXT("ALIGN")) )
	{
		if( ParseCommand(&Str,TEXT("SNAPTOFLOOR")) )
		{
			Trans->Begin( TEXT("Snap actors to floor") );

			UBOOL bAlign=0;
			ParseUBOOL( Str, TEXT("ALIGN="), bAlign );

			Level->Modify();
			for( INT i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor = Cast<AActor>(Level->Actors(i));
				if( Actor && Actor->bSelected )
				{
					Actor->Modify();

					FVector ActorExtent(Actor->CollisionRadius,Actor->CollisionRadius,Actor->CollisionHeight);

					// If there is no collision information for this actor, use it's bounding box.
					if( ActorExtent.IsZero() )
					{
						if( Actor->IsA( AStaticMeshActor::StaticClass() ) )
						{
							UStaticMesh* StaticMesh = Actor->StaticMesh;
							FBox bbox = StaticMesh->BoundingBox;

							FVector Diff = bbox.Max - bbox.Min;

							INT Height, Radius;

							Height = Diff.Z/2.f;
							if( Diff.X > Diff.Y )
								Radius = Diff.X/2.f;
							else
								Radius = Diff.Y/2.f;

							FLOAT Scale = Actor->DrawScale3D.GetAbsMax() * Abs(Actor->DrawScale) * 0.85f;
							Height *= Scale;
							Radius *= Scale;

							ActorExtent = FVector(Radius,Radius,Height);
						}
					}

					FMemMark Mark(GMem);
					FCheckResult* Hit = NULL;
					Hit = Level->MultiLineCheck
					(
						GMem,
						Actor->Location + FVector(0,0,-1)*WORLD_MAX,
						Actor->Location,
						ActorExtent,
						Level->GetLevelInfo(),
						TRACE_World,
						Actor
					);

					if( Hit )
					{
						Actor->Location = Hit->Location;
						if( bAlign )
						{
							Actor->Rotation = Hit->Normal.Rotation();
							Actor->Rotation.Pitch -= 16384;
						}

						Actor->PostEditChange();
					}
					Mark.Pop();
				}
			}

			Trans->End();
			RedrawLevel( Level );
			return 1;
		}
		else
		{
			Trans->Begin( TEXT("Align brush vertices") );
			Level->Modify();
			edactAlignVertices( Level );
			Trans->End();
			RedrawLevel( Level );
			return 1;
		}
	}
	else if( ParseCommand(&Str,TEXT("KEYFRAME")) )
	{
		INT Num=0;
		Parse(Str,TEXT("NUM="),Num);
		Trans->Begin( TEXT("Set mover keyframe") );
		Level->Modify();
		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AMover* Mover=Cast<AMover>(Level->Actors(i));
			if( Mover && Mover->bSelected )
			{
				Mover->Modify();
				Mover->KeyNum = Num;
				Mover->PostEditChange();
				SetPivot( Mover->Location, 0, 0, 1 );
			}
		}
		Trans->End();
		RedrawLevel( Level );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("ROTATERANGE")) )
	{
		FRange Pitch, Yaw, Roll;
		Parse(Str,TEXT("PITCHMIN="),Pitch.A);
		Parse(Str,TEXT("PITCHMAX="),Pitch.B);
		Parse(Str,TEXT("YAWMIN="),Yaw.A);
		Parse(Str,TEXT("YAWMAX="),Yaw.B);
		Parse(Str,TEXT("ROLLMIN="),Roll.A);
		Parse(Str,TEXT("ROLLMAX="),Roll.B);
		Trans->Begin( TEXT("Range rotate actors") );
		Level->Modify();
		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor=Cast<AActor>(Level->Actors(i));
			if( Actor && Actor->bSelected )
			{
				Actor->Modify();
				Actor->Rotation += FRotator( Pitch.GetRand(), Yaw.GetRand(), Roll.GetRand() );
			}
		}
		Trans->End();
		RedrawLevel( Level );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("TOGGLE")) )
	{
		if( ParseCommand(&Str,TEXT("LOCKMOVEMENT")) )			// ACTOR TOGGLE LOCKMOVEMENT
		{
			for( INT i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor = Cast<AActor>(Level->Actors(i));
				if( Actor && Actor->bSelected )
				{
					Actor->Modify();
					Actor->bLockLocation = !Actor->bLockLocation;
				}
			}
		}
		else if( ParseCommand(&Str,TEXT("AUTOALIGNTERRAIN")) )	// ACTOR TOGGLE AUTOALIGNTERRAIN
		{
			for( INT i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor = Cast<AActor>(Level->Actors(i));
				if( Actor && Actor->bSelected )
				{
					Actor->Modify();
					Actor->bAutoAlignToTerrain = !Actor->bAutoAlignToTerrain;
				}
			}
		}

		RedrawLevel( Level );
	}
	else if( ParseCommand(&Str,TEXT("PARTICLESYSTEMS")) )
	{
		if( ParseCommand(&Str, TEXT("NEW")) )
		{
			Trans->Begin( TEXT("Creating new particle system") );
			Level->Modify();
			
			AActor* Image;
			if( ParseObject<AActor>( Str, TEXT("IMAGE="), Image, ANY_PACKAGE ) && Image && Image->IsA(TEXT("ParticleGenerator")) )
			{
				edactCreateNewParticleSystem( Level, Image );
			}
			else
			{
				Ar.Log( NAME_ExecWarning, TEXT("Image particle system not correctly specified.") );
			}

			Trans->End();
			RedrawLevel( Level );
			return 1;
		}
		else if( ParseCommand(&Str, TEXT("CONFORM")) )
		{
			Trans->Begin( TEXT("Conforming selected particle systems") );
			Level->Modify();
			
			AActor* Image;
			if( ParseObject<AActor>( Str, TEXT("IMAGE="), Image, ANY_PACKAGE ) && Image && Image->IsA(TEXT("ParticleGenerator")) )
			{
				edactConformParticleSystems( Level, Image );
			}
			else
			{
				Ar.Log( NAME_ExecWarning, TEXT("Image particle system not correctly specified.") );
			}

			Trans->End();
			RedrawLevel( Level );
			return 1;
		}
		else if( ParseCommand(&Str, TEXT("CLEAN")) )
		{
//			Trans->Begin( TEXT("Destroying all particles in level.") );
//			Level->Modify();
			
			edactCleanParticleSystems( Level );

//			Trans->End();
			RedrawLevel( Level );
			return 1;
		}
		else if( ParseCommand(&Str, TEXT("VALIDATE")) )
		{
			debugf(TEXT("Validating all particle systems in %s:"),GetNameSafe(Level));
			for( INT i=0; i<Level->Actors.Num(); i++ )
				if( Level->Actors(i) && Level->Actors(i)->IsA(TEXT("ParticleGenerator")) )
				{
					UBOOL GIsScriptableSaved=1;
					Exchange(GIsScriptable,GIsScriptableSaved);
					FNotifyInfo(Level->Actors(i),TEXT("ValidateComponents")).SendNotification();
					Exchange(GIsScriptable,GIsScriptableSaved);
				}
			debugf(TEXT("Done!"));
			return 1;
		}
	}


	return 0;

	unguard;
}


UBOOL UUnrealEdEngine::Exec_Mode( const TCHAR* Str, FOutputDevice& Ar )
{
	guard(UUnrealEdEngine::Exec_Mode);

	Word1 = Mode;  // To see if we should redraw
	Word2 = Mode;  // Destination mode to set

	UBOOL DWord1;
	if( ParseUBOOL(Str,TEXT("GRID="), DWord1) )
	{
		FinishAllSnaps (Level);
		Constraints.GridEnabled = DWord1;
		Word1=MAXWORD;
	}
	if( ParseUBOOL(Str,TEXT("ROTGRID="), DWord1) )
	{
		FinishAllSnaps (Level);
		Constraints.RotGridEnabled=DWord1;
		Word1=MAXWORD;
	}
	if( ParseUBOOL(Str,TEXT("SNAPVERTEX="), DWord1) )
	{
		FinishAllSnaps (Level);
		Constraints.SnapVertices=DWord1;
		Word1=MAXWORD;
	}
	if( ParseUBOOL(Str,TEXT("ALWAYSSHOWTERRAIN="), DWord1) )
	{
		FinishAllSnaps (Level);
		AlwaysShowTerrain=DWord1;
		Word1=MAXWORD;
	}
	if( ParseUBOOL(Str,TEXT("USEACTORROTATIONGIZMO="), DWord1) )
	{
		FinishAllSnaps (Level);
		UseActorRotationGizmo=DWord1;
		Word1=MAXWORD;
	}
	if( Parse(Str,TEXT("SELECTIONLOCK="), DWord1) )
	{
		FinishAllSnaps (Level);
		// If -1 is passed in, treat it as a toggle.  Otherwise, use the value as a literal assignment.
		if( DWord1 == -1 )
			GEdSelectionLock=(GEdSelectionLock == 0) ? 1 : 0;
		else
			GEdSelectionLock=DWord1;
		Word1=MAXWORD;
	}
	Parse(Str,TEXT("MAPEXT="), GMapExt);
	if( Parse(Str,TEXT("USESIZINGBOX="), DWord1) )
	{
		FinishAllSnaps (Level);
		// If -1 is passed in, treat it as a toggle.  Otherwise, use the value as a literal assignment.
		if( DWord1 == -1 )
			UseSizingBox=(UseSizingBox == 0) ? 1 : 0;
		else
			UseSizingBox=DWord1;
		Word1=MAXWORD;
	}
	Parse( Str, TEXT("SPEED="), MovementSpeed );
	Parse( Str, TEXT("SNAPDIST="), Constraints.SnapDistance );
	//
	// Major modes:
	//
	if 		(ParseCommand(&Str,TEXT("CAMERAMOVE")))		Word2 = EM_ViewportMove;
	else if	(ParseCommand(&Str,TEXT("CAMERAZOOM")))		Word2 = EM_ViewportZoom;
	else if	(ParseCommand(&Str,TEXT("ACTORROTATE")))	Word2 = EM_ActorRotate;
	else if	(ParseCommand(&Str,TEXT("ACTORSCALE")))		Word2 = EM_ActorScale;
	else if	(ParseCommand(&Str,TEXT("ACTORSNAP"))) 		Word2 = EM_ActorSnapScale;
	else if	(ParseCommand(&Str,TEXT("TEXTUREPAN")))		Word2 = EM_TexturePan;
	else if	(ParseCommand(&Str,TEXT("TEXTUREROTATE")))	Word2 = EM_TextureRotate;
	else if	(ParseCommand(&Str,TEXT("TEXTURESCALE"))) 	Word2 = EM_TextureScale;
	else if	(ParseCommand(&Str,TEXT("BRUSHCLIP"))) 		Word2 = EM_BrushClip;
	else if	(ParseCommand(&Str,TEXT("FACEDRAG"))) 		Word2 = EM_FaceDrag;
	else if	(ParseCommand(&Str,TEXT("VERTEXEDIT"))) 	Word2 = EM_VertexEdit;
	else if	(ParseCommand(&Str,TEXT("POLYGON"))) 		Word2 = EM_Polygon;
	else if (ParseCommand(&Str,TEXT("TERRAINEDIT"))) 	Word2 = EM_TerrainEdit;
	else if (ParseCommand(&Str,TEXT("MATINEE"))) 		Word2 = EM_Matinee;
	else if (ParseCommand(&Str,TEXT("EYEDROPPER"))) 	Word2 = EM_EyeDropper;
	else if (ParseCommand(&Str,TEXT("FINDACTOR"))) 		Word2 = EM_FindActor;
	else if (ParseCommand(&Str,TEXT("NEWCAMERAMOVE"))) 	Word2 = EM_NewCameraMove;
	else if	(ParseCommand(&Str,TEXT("GEOMETRY"))) 		Word2 = EM_Geometry;

	if( Word2 != Word1 )
	{
		if( Word1 == EM_Polygon )
			polygonDeleteMarkers();
		if( Word1 == EM_BrushClip )
			brushclipDeleteMarkers();

		edcamSetMode( Word2 );
		RedrawLevel( Level );
	}

	// Reset the roll on all viewport cameras
	UViewport* Viewport = NULL;
	for( INT vp = 0 ; vp < dED_MAX_VIEWPORTS ; ++vp )
	{
		Viewport = FindObject<UViewport>( ANY_PACKAGE, *FString::Printf(TEXT("U2Viewport%d"), vp) );
		if( Viewport && !Viewport->IsOrtho() )
			Viewport->Actor->Rotation.Roll = 0;
	}

	EdCallback( EDC_RedrawAllViewports, 0, 0 );
	EdCallback( EDC_RefreshEditor, 1, ERefreshEditor_Misc );

	return 1;

	unguard;
}

UBOOL UUnrealEdEngine::Exec_Script( const TCHAR* Str, FOutputDevice& Ar )
{
	guard(UUnrealEdEngine::Exec_Script);

	if( ParseCommand(&Str,TEXT("MAKE")) )
	{
		GWarn->BeginSlowTask( TEXT("Compiling scripts"), 0);
		UBOOL All  = ParseCommand(&Str,TEXT("ALL"));
		UBOOL Boot = ParseCommand(&Str,TEXT("BOOT"));
		MakeScripts( UObject::StaticClass(), GWarn, All, Boot, true );
		GWarn->EndSlowTask();
		UpdatePropertiesWindows();
		return 1;
	}

	return 0;

	unguard;
}

UBOOL UUnrealEdEngine::Exec_Prefab( const TCHAR* Str, FOutputDevice& Ar )
{
	guard(UUnrealEdEngine::Exec_Prefab);

	if( ParseCommand(&Str,TEXT("New")) )
	{
		FName GroupName=NAME_None;
		FName PackageName;
		UClass* PrefabClass;

		DWORD T3DDataPtr;
		Parse( Str, TEXT("T3DData="), T3DDataPtr );
		TCHAR* T3DText = (TCHAR*)T3DDataPtr;

		if( Parse( Str, TEXT("NAME="), TempName, NAME_SIZE )
				&& ParseObject<UClass>( Str, TEXT("CLASS="), PrefabClass, ANY_PACKAGE )
				&& Parse( Str, TEXT("PACKAGE="), PackageName )
				&& PrefabClass->IsChildOf( UPrefab::StaticClass() ) 
				&& PackageName!=NAME_None )
		{
			UPackage* Pkg = CreatePackage(NULL,*PackageName);
			Pkg->bDirty = 1;
			if( Parse( Str, TEXT("GROUP="), GroupName ) && GroupName!=NAME_None )
				Pkg = CreatePackage(Pkg,*GroupName);
			if( !StaticFindObject( PrefabClass, Pkg, TempName ) )
			{
				// Create new prefab object.
				UPrefab* Result = (UPrefab*)StaticConstructObject( PrefabClass, Pkg, TempName, RF_Public|RF_Standalone );
				Result->PostLoad();

				GCurrentPrefab = Result;
				GCurrentPrefab->T3DText = T3DText;
			}
			else
				Ar.Logf( NAME_ExecWarning, TEXT("Prefab exists") );
		}
		else
			Ar.Logf( NAME_ExecWarning, TEXT("Bad PREFAB NEW") );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("ADD")) )		// PREFAB ADD NAME=<name> [ SNAP=<0/1> ]
	{
		GWarn->BeginSlowTask( TEXT("Inserting prefab"), 1);

		const TCHAR* Ptr = *GCurrentPrefab->T3DText;
		ULevelFactory* Factory;
		Factory = ConstructObject<ULevelFactory>(ULevelFactory::StaticClass());
		Factory->FactoryCreateText( Level,ULevel::StaticClass(), Level->GetOuter(), Level->GetFName(), RF_Transactional, NULL, TEXT("paste"), Ptr, Ptr+GCurrentPrefab->T3DText.Len(), GWarn );
		NoteSelectionChange( Level );

		// Now that we've added the prefab into the world figure out the bounding box, and move it
		// so it's centered on the last click location.
		FBox bbox(1);
		for( INT i=0; i<Level->Actors.Num(); ++i )
			if( Level->Actors(i) && Level->Actors(i)->bSelected )
				bbox += (Level->Actors(i)->Location - Level->Actors(i)->PrePivot);

		FVector NewLocation;
		FVector Location;
		if( GetFVECTOR( Str, NewLocation ) )
			Location = NewLocation + ClickPlane;
		else
			Location = ClickLocation + ClickPlane;

		FVector Diff = Location - bbox.GetCenter();
		
		for( INT i=0; i<Level->Actors.Num(); ++i )
			if( Level->Actors(i) && Level->Actors(i)->bSelected )
				Level->Actors(i)->Location += Diff;

		RedrawLevel( Level );

		GWarn->EndSlowTask();

		return 1;
	}

	return 0;

	unguard;
}
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
