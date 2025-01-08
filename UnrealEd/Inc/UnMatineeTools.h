/*=============================================================================
	UnMatineeTools.h: Tools for the Matinee system
	Copyright 1997-2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall
=============================================================================*/

class UNREALED_API FCameraPath
{
public:
	// Constructor.
	FCameraPath();
	virtual ~FCameraPath();

	void SetPathName( FString InPathName );
	void RebuildPointList( ULevel* InLevel );
	void DrawPointList( UViewport* Viewport );
	FLOAT GetPathLengthInPixels();
	TArray<AInterpolationPoint*>* GetPointList() { return &PointList; }
	void DeletePoint( AInterpolationPoint* InIP );

	inline FString GetPathName() { return PathName; }

private:
	void InsertPoint( AInterpolationPoint* InPoint );

	TArray<AInterpolationPoint*> PointList;		// The list of IPs that make up this path (sorted by position number)
	FString PathName;							// The name of the path we're associated with
	FLOAT LastScale;							// Holds the last scaling value that was using when rendering this path into a matinee viewport
};

class UNREALED_API FMatineeTools
{
public:
	UBOOL bAlwaysShowPath,		// Always draw the path, even if no interpolation points are selected?
		bShowPathOrientation;	// Show camera orientation markers along path?

	// Constructor.
	FMatineeTools();
	virtual ~FMatineeTools();

	void Init();
	FCameraPath* NewCameraPath();
	UBOOL DeleteCameraPath( FCameraPath* InPath );
	FCameraPath* GetCameraPathFromName( FString InPathName );
	void RebuildAllLists( ULevel* InLevel );
	FCameraPath* GetCurrentCameraPath();
	void SelectCameraPath( FString InPathName );
	void LinkPoints( TArray<AInterpolationPoint*>* InPointList );
	void GetSamplePoints( AInterpolationPoint* InIP, FLOAT InResolution, TArray<FPosition>* InPositions, UBOOL InEmptyPosList = 1 );
	void InsertPointAtPct( FCameraPath* InPath, FLOAT InPct );
	void RebuildAllPointLists( ULevel* InLevel );

	TArray<FCameraPath> CameraPaths;	// A list of all camera paths that exist in the matinee editing interface
	FCameraPath* CurCameraPath;
	UTexture *IntPoint, *IntPointSel, *TimeMarker;
	FLOAT TimeSliderPct;
};

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
