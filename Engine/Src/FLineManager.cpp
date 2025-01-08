//#if 1 //NEW (mdf) FLineManager
/*=============================================================================
FLineManager.cpp

See FLineManager.h.
=============================================================================*/

#include "EnginePrivate.h"
#include "FLineManager.h"
#include "UnRender.h"
#include <float.h>

/* class static data */
// class is only instantiated if it is used through GetInstance()
FLineManager* FLineManager::_Instance = NULL;

//-----------------------------------------------------------------------------

/*static*/FLineManager* FLineManager::GetInstance()
{
	guard(FLineManager::GetInstance)

	if( !_Instance )
	{
		_Instance = new FLineManager;

		// make sure generic line lists allocated
		check( _Instance->NextKey == InitialKeyVal );
		for( INT ii=0; ii<NumGenericLists; ii++ )
			_Instance->AllocateLineList();
		check( _Instance->NextKey == NumGenericLists );
	}

	return _Instance;

	unguard;
}

//-----------------------------------------------------------------------------

INT FLineManager::AllocateLineList( UBOOL bClip, UBOOL bHide, UBOOL bOnceOnly )
{
	guard(FLineManager::AllocateLineList)

	LineLists.Set( NextKey, LineListS( bClip, bHide, bOnceOnly) );

	// valid keys are in 0..MAXINT (NextKey never decrements so having more than 2GB lists a no go <g>...)
	return NextKey++;

	unguard;
}

//-----------------------------------------------------------------------------

/*static*/void FLineManager::FreeInstance()
{
	guard(FLineManager::FreeInstance)

	if( _Instance )
	{
		delete _Instance;
		_Instance = NULL;
	}

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::FreeLineList( INT Key )
{
	guard(FLineManager::FreeLineList)

	// never free generic line lists
	if( Key > NumGenericLists )
		LineLists.Remove( Key );

	unguard;
}

//-----------------------------------------------------------------------------

/*static*/void FLineManager::ClearAllLines()
{
	guard(FLineManager::ClearAllLines)

	if( _Instance )
	{
		// render all lines in all lists
		for( TMap<INT,LineListS>::TIterator It(_Instance->LineLists); It; ++It )
		{
			It.Value().Lines.Empty();
		}	
	}

	unguard;
}

//-----------------------------------------------------------------------------

/*static*/void FLineManager::SetClipAllLines( UBOOL bVal )
{
	guard(FLineManager::SetClipAllLines)

	if( _Instance )
	{
		// render all lines in all lists
		for( TMap<INT,LineListS>::TIterator It(_Instance->LineLists); It; ++It )
		{
			It.Value().bClip = bVal;
		}	
	}

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::ClearLines( INT Key )
{
	guard(FLineManager::ClearLines)

	LineLists.Find( Key )->Lines.Empty();

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::SetClipLines( INT Key, UBOOL bVal )
{
	guard(FLineManager::SetClipLines)

	LineLists.Find( Key )->bClip = bVal;

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::SetHideLines( INT Key, UBOOL bVal )
{
	guard(FLineManager::SetHideLines)

	LineLists.Find( Key )->bHide = bVal;

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::ToggleClipLines( INT Key )
{
	guard(FLineManager::ToggleClipLines)

	LineLists.Find( Key )->bClip = !LineLists.Find( Key )->bClip;

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::ToggleHideLines( INT Key )
{
	guard(FLineManager::ToggleHideLines)

	LineLists.Find( Key )->bHide = !LineLists.Find( Key )->bHide;

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::SetLineColors( INT Key, const FPlane& Color )
{
	guard(FLineManager::SetLineColors)

	// render all lines in all lists
	for( TMap<INT,LineListS>::TIterator It(LineLists); It; ++It )
	{
		LineListS LineList = It.Value();
	
		for( INT ii=0; ii<LineList.Lines.Num(); ii++ )
			LineList.Lines(ii).Color = Color;
	}

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::OffsetLines( INT Key, const FVector& Offset )
{
	guard(FLineManager::OffsetLines)

	// render all lines in all lists
	for( TMap<INT,LineListS>::TIterator It(LineLists); It; ++It )
	{
		LineListS LineList = It.Value();
	
		for( INT ii=0; ii<LineList.Lines.Num(); ii++ )
		{
			LineList.Lines(ii).Start += Offset;
			LineList.Lines(ii).End += Offset;
		}
	}

	unguard;
}

//-----------------------------------------------------------------------------

UBOOL FLineManager::GetClipLines( INT Key ) const
{
	guard(FLineManager::GetClipLines)

	return LineLists.Find( Key )->bClip;

	unguard;
}

//-----------------------------------------------------------------------------

UBOOL FLineManager::GetHideLines( INT Key ) const
{
	guard(FLineManager::GetHideLines)

	return LineLists.Find( Key )->bHide;

	unguard;
}

//-----------------------------------------------------------------------------

FPlane FLineManager::GetLineColor( INT Key, INT Index ) const
{
	guard(FLineManager::GetLineColor)

	return LineLists.Find( Key )->Lines(Index).Color;

	unguard;
}

//-----------------------------------------------------------------------------

INT FLineManager::GetNumLists()
{
	guard(FLineManager::GetNumLists);

	INT Num=0;
	for( TMap<INT,LineListS>::TIterator It(_Instance->LineLists); It; ++It )
	{
		Num++;
	}

	return Num;

	unguard;
}

//-----------------------------------------------------------------------------

INT FLineManager::GetNumLines()
{
	guard(FLineManager::GetNumLines);

	INT Num=0;
	for( TMap<INT,LineListS>::TIterator It(_Instance->LineLists); It; ++It )
	{
		Num+=It.Value().Lines.Num();
	}

	return Num;

	unguard;
}

//-----------------------------------------------------------------------------


void FLineManager::AddLine( INT Key, const FVector& Start, const FVector& End, const FPlane& Color )
{
	guard(FLineManager::AddLine);

	LineLists.Find( Key )->Lines.AddItem( LineInfoS(Start, End, Color) );

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::AddLines( INT Key, const TArray<FVector>& EndPoints, const FPlane& Color )
{
	guard(FLineManager::AddLines)

	for( INT ii=0; (ii+1)<EndPoints.Num(); ii+=2 )
		AddLine( Key, EndPoints(ii), EndPoints(ii+1), Color );

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::AddArrow(
	INT Key, 
	const FVector& Start, 
	const FVector& End, 
	const FPlane& Color, 
	const FPlane& ArrowColor, 
	FLOAT ArrowBaseOffset, 
	FLOAT ArrowBaseHalfLength, 
	FLOAT ArrowLength )
{
	guard(FLineManager::AddArrow);

	// do as many calculations outside of the render loop as possible
	FVector Normal = (End - Start).SafeNormal();

	// if base offset is 0 (arrow should be at end of line), reduce length of line so
	// it just reaches arrowhead and arrowhead point corresponds to given end

	FVector AdjustedEnd = End;
	if( ArrowBaseOffset == 0.0 )
	{
		AdjustedEnd -= ArrowLength*Normal;
	}

	FVector BaseLocation = AdjustedEnd + (ArrowBaseOffset*Normal);
	FVector HeadLocation = BaseLocation + Normal*ArrowLength;

	LineLists.Find( Key )->Lines.AddItem( LineInfoS(Start, AdjustedEnd, Color, ArrowColor, ArrowBaseHalfLength, Normal, BaseLocation, HeadLocation) );

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::AddArrows( 
	INT Key, 
	const TArray<FVector>& EndPoints, 
	const FPlane& Color, 
	const FPlane& ArrowColor, 
	FLOAT ArrowBaseOffset, 
	FLOAT ArrowBaseLen, 
	FLOAT ArrowLen )
{
	guard(FLineManager::AddArrows);

	for( INT ii=0; (ii+1)<EndPoints.Num(); ii+=2 )
		AddArrow( Key, EndPoints(ii), EndPoints(ii+1), Color, ArrowColor, ArrowBaseOffset, ArrowBaseLen, ArrowLen );

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::AddBox( 
	INT Key,
	const FVector& Location,
	FLOAT XExtent, 
	FLOAT YExtent, 
	FLOAT ZExtent,
	const FPlane& Color,
	FLOAT GridTopBottomSpacing,
	FLOAT GridSidesSpacing ) // mdf-tbi
{
	guard(FLineManager::AddBox);

	FLOAT XOff, YOff, ZOff;

	if( XExtent < 0.1f )
		XExtent = 0.1f;
	if( YExtent < 0.1f )
		YExtent = 0.1f;
	if( ZExtent < 0.1f )
		ZExtent = 0.1f;

	// vertical edges
 	AddLine( Key, 
	   		 FVector( Location.X-XExtent, Location.Y-YExtent, Location.Z-ZExtent),
	   		 FVector( Location.X-XExtent, Location.Y-YExtent, Location.Z+ZExtent),
	   		 Color );
 	AddLine( Key, 
	   		 FVector( Location.X-XExtent, Location.Y+YExtent, Location.Z-ZExtent),
	   		 FVector( Location.X-XExtent, Location.Y+YExtent, Location.Z+ZExtent),
	   		 Color );
 	AddLine( Key, 
	   		 FVector( Location.X+XExtent, Location.Y-YExtent, Location.Z-ZExtent),
	   		 FVector( Location.X+XExtent, Location.Y-YExtent, Location.Z+ZExtent),
	   		 Color );
 	AddLine( Key, 
			 FVector( Location.X+XExtent, Location.Y+YExtent, Location.Z-ZExtent),
			 FVector( Location.X+XExtent, Location.Y+YExtent, Location.Z+ZExtent),
	   		 Color );

	// lower and upper edges
	for( ZOff=-ZExtent; ZOff<=ZExtent; ZOff+=2*ZExtent )
	{
	 	AddLine( Key, 
		   		 FVector( Location.X-XExtent, Location.Y-YExtent, Location.Z-ZOff),
		   		 FVector( Location.X-XExtent, Location.Y+YExtent, Location.Z-ZOff),
		   		 Color );
	 	AddLine( Key, 
		   		 FVector( Location.X-XExtent, Location.Y-YExtent, Location.Z-ZOff),
		   		 FVector( Location.X+XExtent, Location.Y-YExtent, Location.Z-ZOff),
		   		 Color );
	 	AddLine( Key, 
		   		 FVector( Location.X+XExtent, Location.Y+YExtent, Location.Z-ZOff),
		   		 FVector( Location.X+XExtent, Location.Y-YExtent, Location.Z-ZOff),
		   		 Color );
	 	AddLine( Key, 
				 FVector( Location.X+XExtent, Location.Y+YExtent, Location.Z-ZOff),
				 FVector( Location.X-XExtent, Location.Y+YExtent, Location.Z-ZOff),
		   		 Color );
	}

	if( GridTopBottomSpacing >= 0.0001f && GridTopBottomSpacing <= 1.0f )
	{
		for( ZOff=-ZExtent; ZOff<=ZExtent; ZOff+=2*ZExtent )
		{
			for( XOff=-XExtent; XOff<=XExtent; XOff+=XExtent*GridTopBottomSpacing )
			{
			 	AddLine( Key, 
				   		 FVector( Location.X-XOff, Location.Y-YExtent, Location.Z-ZOff),
				   		 FVector( Location.X-XOff, Location.Y+YExtent, Location.Z-ZOff),
				   		 Color );
			}
			for( YOff=-YExtent; YOff<=YExtent; YOff+=YExtent*GridTopBottomSpacing )
			{
			 	AddLine( Key, 
				   		 FVector( Location.X-XExtent, Location.Y-YOff, Location.Z-ZOff),
				   		 FVector( Location.X+XExtent, Location.Y-YOff, Location.Z-ZOff),
				   		 Color );
			}
		}
	}
	if( GridSidesSpacing >= 0.0001f && GridSidesSpacing <= 1.0f )
	{
		// mdf-tbi:
	}

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::AddBox( 
	INT Key,
	const FVector& Location,
	const FVector& Extent, 
	const FPlane& Color,
	FLOAT GridTopBottomSpacing,
	FLOAT GridSidesSpacing ) // mdf-tbi
{
	AddBox( Key, Location, Extent.X, Extent.Y, Extent.Z, Color, GridTopBottomSpacing, GridSidesSpacing);
}

//-----------------------------------------------------------------------------

void FLineManager::AddFBox( 
	INT Key,
	const FBox Box,
	const FPlane& Color,
	FLOAT GridTopBottomSpacing, 
	FLOAT GridSidesSpacing )
{
	AddBox( Key, Box.Min + 0.5*(Box.Max-Box.Min), 0.5*(Box.Max.X-Box.Min.X), 0.5*(Box.Max.Y-Box.Min.Y), 0.5*(Box.Max.Z-Box.Min.Z), Color, GridTopBottomSpacing, GridSidesSpacing );
}

//-----------------------------------------------------------------------------

void FLineManager::AddCylinder( 
	INT Key,
	const FVector& Location,
	FLOAT Radius, 
	FLOAT Height,
	const FPlane& Color,
	INT NumCylinderSegments,
	UBOOL bTopBottomGrid )
{
	guard(FLineManager::AddCylinder);

	if( Radius <= 0.1f )
		Radius = 0.1f;
	if( Height <= 0.1f )
		Height = 0.1f;

	FLOAT AngleInterval = (2*PI) / NumCylinderSegments;

	FLOAT PreviousXVal = Location.X + Radius * 1.0f;
	FLOAT PreviousYVal = Location.Y + Radius * 0.0f;
	for( FLOAT Angle=AngleInterval; Angle<=(2*PI+0.0001f); Angle+=AngleInterval )
	{
		// X/Y:
		FLOAT XVal = Location.X + Radius * appCos( Angle );
		FLOAT YVal = Location.Y + Radius * appSin( Angle );

		// draw line segment at bottom, middle, top	
		for( FLOAT CurHeight=Location.Z-Height; CurHeight<=Location.Z+Height; CurHeight+=Height )
		{
		 	AddLine( Key, 
	   				 FVector( PreviousXVal, PreviousYVal, CurHeight ),
	   				 FVector( XVal,         YVal,         CurHeight ),
		   			 Color );
	
		}
	
		// draw line segment from bottom to top
	 	AddLine( Key, 
				 FVector( XVal, YVal, Location.Z-Height ),
				 FVector( XVal, YVal, Location.Z+Height ),
	   			 Color );
	
		PreviousXVal = XVal;
		PreviousYVal = YVal;
	}

	if( bTopBottomGrid )
	{
		for( FLOAT CurHeight=Location.Z-Height; CurHeight<=Location.Z+Height; CurHeight+=Height )
		{
			// X=0,Y=R to X=0,Y=-R
		 	AddLine( Key, 
	   				 FVector( Location.X + 0.0f, Location.Y + Radius, CurHeight ),
	   				 FVector( Location.X + 0.0f, Location.Y - Radius, CurHeight ),
		   			 Color );
				
			// X=R,Y=0 to X=-R,Y=0
		 	AddLine( Key, 
	   				 FVector( Location.X + Radius, Location.Y + 0.0f, CurHeight ),
	   				 FVector( Location.X - Radius, Location.Y + 0.0f, CurHeight ),
		   			 Color );

  			for( FLOAT Angle=AngleInterval; Angle<PI/2-0.0001f; Angle+=AngleInterval )
			{
				FLOAT PosXVal = Location.X + Radius * appCos( Angle );
				FLOAT PosYVal = Location.Y + Radius * appSin( Angle );
				FLOAT NegXVal = Location.X - Radius * appCos( Angle );
				FLOAT NegYVal = Location.Y - Radius * appSin( Angle );
		
			 	AddLine( Key, 
		   				 FVector( PosXVal, PosYVal, CurHeight ),
		   				 FVector( NegXVal, PosYVal, CurHeight ),
			   			 Color );

			 	AddLine( Key, 
		   				 FVector( PosXVal, PosYVal, CurHeight ),
		   				 FVector( PosXVal, NegYVal, CurHeight ),
			   			 Color );

			 	AddLine( Key, 
		   				 FVector( NegXVal, NegYVal, CurHeight ),
		   				 FVector( PosXVal, NegYVal, CurHeight ),
			   			 Color );

			 	AddLine( Key, 
		   				 FVector( NegXVal, NegYVal, CurHeight ),
		   				 FVector( NegXVal, PosYVal, CurHeight ),
			   			 Color );
			}
		}
	}

	unguard;
}

//-----------------------------------------------------------------------------

void FLineManager::AddActor(
	INT Key,
	const AActor& A,
	const FPlane& Color,
	INT NumCylinderSegments,
	UBOOL bTopBottomGrid )
{
	AddCylinder( Key, A.Location, A.CollisionRadius, A.CollisionHeight, Color, NumCylinderSegments, bTopBottomGrid );
}

//-----------------------------------------------------------------------------

/*static*/void FLineManager::Draw( FSceneNode* Frame, UBOOL bForceEditorDraw )
{
	guard(FLineManager::Draw);

	if( _Instance && (!GIsEditor || bForceEditorDraw) )
	{
		// render all lines in all lists
		FLineBatcher	LineBatcherClipped( Frame->Viewport->RI, 1 );
		FLineBatcher	LineBatcherUnClipped( Frame->Viewport->RI, 0 );
		FLineBatcher*	LineBatcher = NULL;

		INT NumClippedLines = 0;
		INT NumUnClippedLines = 0;
		
		for( TMap<INT,LineListS>::TIterator It(_Instance->LineLists); It; ++It )
		{
			LineListS& LineList = It.Value();
	
			if( !LineList.bHide )
			{
				for( INT ii=0; ii<LineList.Lines.Num(); ii++ )
				{
					LineInfoS& Line = LineList.Lines(ii);

					if( LineList.bClip )
					{
						LineBatcher = &LineBatcherClipped;
						if( NumClippedLines++ > 32767 )
							LineBatcher->Flush();
					}
					else
					{
						LineBatcher = &LineBatcherUnClipped;
						if( NumUnClippedLines++ > 32767 )
							LineBatcher->Flush();
					}
					
					if( Line.bIsArrow )
					{
						LineBatcher->DrawLine( Line.Start, Line.End, Line.Color );
					
						// cross product of line with view vector --> vector perpendicular to line and view (ignore ortho views for speed -- distortion not significant)
						FVector BaseVect = Line.Normal ^ (Frame->ViewOrigin - Line.BaseLocation);
						BaseVect.Normalize();
						FVector BaseEnd1 = Line.BaseLocation + BaseVect*Line.ArrowBaseHalfLength;
						FVector BaseEnd2 = Line.BaseLocation - BaseVect*Line.ArrowBaseHalfLength;
					
						// base
					 	LineBatcher->DrawLine( BaseEnd1, BaseEnd2, Line.ArrowColor );
					
						// base end #1 to point
						LineBatcher->DrawLine( BaseEnd1,Line.HeadLocation, Line.ArrowColor );
					
						// base end #2 to point
						LineBatcher->DrawLine( BaseEnd2,Line.HeadLocation, Line.ArrowColor );
					}
					else
					{
						LineBatcher->DrawLine( Line.Start, Line.End, Line.Color );
					}
				}	
			}

			if( !Frame->Parent && LineList.bOnceOnly )
			{
				// done all render passes -- clear temporary lines
				LineList.Lines.Empty();
			}
		}

		LineBatcherClipped.Flush();
		LineBatcherUnClipped.Flush();
	}

	unguard;
}
//#endif