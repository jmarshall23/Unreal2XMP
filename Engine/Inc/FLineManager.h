//#if 1 //NEW (mdf) FLineManager
/*=============================================================================
FLineManager.h

Class used for managing collections of lines which can be rendered in-game or
in-editor.

Multiple users of this class can each allocate their own list of lines which
they manage.

Enforced singleton class which is only instantiated if/when a line list is
requested. Rendering code calls FLineManager::Draw() which only draws lines
if the class has been instantiated. See Design Patterns singleton pattern for
more info.

To use this class:

+ allocate line list:
	INT myKey = FLineManager::GetInstance()->AllocateLineList();

+ add lines to line list:
	FLineManager::GetInstance->AddLine( myKey, Start, End, COLOR_GREEN );

+ can toggle clipping, hiding etc. if desired
	FLineManager::GetInstance->SetClipLines( myKey, true );
	FLineManager::GetInstance->SetHideLines( myKey, true );
	etc.

+ a line list can be removed (destroyed) using:
	FLineManager::GetInstance->FreeLineList( myKey ));

The AddArrow, AddBox etc. functions are helpers for creating arrows, bounding
boxes etc.

Any visible lines in the (unique) FLineManager will be rendered in game and 
in the editor. 

Engine exit routine will clean up the FLineManager on engine shutdown.

By default, lines aren't cleared between level loads.

You can use bOnceOnly to specify that lines should be cleared immediately 
after the last rendering pass. e.g. see collision cylinder rendering code -- 
adds lines in one part of the rendering pass and the FLineManager clears 
them immediately after rendering them (in case the target actor moves or is 
destroyed).

Some commands affect all lines (see ULevel::Exec):

	lines clear:	clears (removes) all lines from all lists
	lines clip:		toggles clipping for all lines in all lists
	lines info:		reports # lines and lists (won't catch bOnceOnly lines)

=============================================================================*/

#ifndef _INC_FLineManager_
#define _INC_FLineManager_

// line-drawing colors
#define COLOR_BLUE 			(FPlane(0.000f, 0.000f, 1.000f, 0.000f))
#define COLOR_GREEN			(FPlane(0.000f, 1.000f, 0.000f, 0.000f))
#define COLOR_CYAN			(FPlane(0.000f, 1.000f, 1.000f, 0.000f))
#define COLOR_RED			(FPlane(1.000f, 0.000f, 0.000f, 0.000f))
#define COLOR_MAGENTA		(FPlane(1.000f, 0.000f, 1.000f, 0.000f))
#define COLOR_YELLOW		(FPlane(1.000f, 1.000f, 0.000f, 0.000f))

#define COLOR_DKBLUE 		(FPlane(0.000f, 0.000f, 0.500f, 0.000f))
#define COLOR_DKGREEN		(FPlane(0.000f, 0.500f, 0.000f, 0.000f))
#define COLOR_DKCYAN		(FPlane(0.000f, 0.500f, 0.500f, 0.000f))
#define COLOR_DKRED			(FPlane(0.500f, 0.000f, 0.000f, 0.000f))
#define COLOR_DKMAGENTA		(FPlane(0.500f, 0.000f, 0.500f, 0.000f))
#define COLOR_DKYELLOW		(FPlane(0.500f, 0.500f, 0.000f, 0.000f))

#define COLOR_BLACK			(FPlane(0.000f, 0.000f, 0.000f, 0.000f))
#define COLOR_DKGREY		(FPlane(0.250f, 0.250f, 0.250f, 0.000f))
#define COLOR_GREY			(FPlane(0.500f, 0.500f, 0.500f, 0.000f))
#define COLOR_LTGREY		(FPlane(0.750f, 0.750f, 0.750f, 0.000f))
#define COLOR_WHITE			(FPlane(1.000f, 1.000f, 1.000f, 0.000f))
																
#define COLOR_BROWN			(FPlane(0.500f, 0.250f, 0.000f, 0.000f))
#define COLOR_ORANGE		(FPlane(1.000f, 0.500f, 0.000f, 0.000f))
#define COLOR_PINK			(FPlane(1.000f, 0.000f, 0.500f, 0.000f))

#define DEFAULTLINECOLOR	COLOR_RED

// defaults
const INT DefaultArrowBaseOffset		= 0;
const INT DefaultArrowBaseHalfLength	= 2;
const INT DefaultArrowLength			= 8;
const INT DefaultCylinderSegments		= 16;

const INT InitialKeyVal					= 0;
const INT NumGenericLists				= 3;

//-----------------------------------------------------------------------------
// handy macros

#define ADDACTOR		FLineManager::GetInstance()->AddActor0
#define ADDACTORAT		FLineManager::GetInstance()->AddActorAt0
#define ADDARROW		FLineManager::GetInstance()->AddArrow0
#define ADDARROWS		FLineManager::GetInstance()->AddArrows0
#define ADDBOX			FLineManager::GetInstance()->AddBox0
#define ADDCYLINDER		FLineManager::GetInstance()->AddCylinder0
#define ADDFBOX			FLineManager::GetInstance()->AddFBox0
#define ADDLINE			FLineManager::GetInstance()->AddLine0
#define ADDLINES		FLineManager::GetInstance()->AddLines0

//-----------------------------------------------------------------------------

class FLineBatcher;

//-----------------------------------------------------------------------------

class ENGINE_API FLineManager
{
private:
	//-------------------------------------------------------------------------
	// Note: arrow information needed here because these are oriented toward
	// the viewpoint (not known until render time -- so we can't just store
	// arrows as a set of lines).

	class LineInfoS
	{
	public:
		FVector		Start;
		FVector		End;
		FPlane		Color;
		FPlane		ArrowColor;
		FLOAT		ArrowBaseHalfLength;
		UBOOL		bIsArrow;
		FVector		Normal;
		FVector		BaseLocation;
		FVector		HeadLocation;
	
		LineInfoS( const FVector& StartVal, const FVector& EndVal, const FPlane ColorVal )
			: Start(StartVal), End(EndVal), Color(ColorVal) 
		{
			bIsArrow = false;
		};
	
		LineInfoS( const FVector& StartVal, const FVector& EndVal, const FPlane ColorVal,
				   const FPlane&  ArrowColorVal, 
				   FLOAT ArrowBaseHalfLengthVal=DefaultArrowBaseHalfLength, 
				   FVector NormalVal=FVector(0,0,0),
				   FVector BaseLocationVal=FVector(0,0,0),
				   FVector HeadLocationVal=FVector(0,0,0) )
			: Start(StartVal), End(EndVal), Color(ColorVal), 
			  ArrowColor(ArrowColorVal), ArrowBaseHalfLength(ArrowBaseHalfLengthVal),
			  Normal(NormalVal), BaseLocation(BaseLocationVal), HeadLocation(HeadLocationVal)
		{
			bIsArrow = true;
		};
	};
	
	//-------------------------------------------------------------------------

	class LineListS
	{
	public:
		UBOOL bClip;				// whether to clip all lines in list (depends on h/w too...)
		UBOOL bHide;				// whether any lines in list drawn
		UBOOL bOnceOnly;			// whether to clear lines (key stays valid) after rendering
		TArray<LineInfoS> Lines;	// the actual lines
	
		LineListS( UBOOL bClipVal=true, UBOOL bHideVal=false, UBOOL bOnceOnlyVal=false ) 
			: bClip(bClipVal), bHide(bHideVal), bOnceOnly(bOnceOnlyVal) {};
	};

	//-------------------------------------------------------------------------

private:
	INT NextKey;											// next key returned by AllocateLineList
	TMap<INT,LineListS> LineLists;							// the list of lists of lines
	static FLineManager* _Instance;							// singleton instance pointer

	FLineManager() : NextKey(InitialKeyVal) {};				// ctor -- can't be instantiated outside of this class -- use GetInstance
	~FLineManager() { LineLists.Empty(); };					// dtor -- can't be destroyed outside of this class -- use FreeInstance

public:
	static FLineManager* GetInstance();						// class must be used through this function, except for other static methods
	static void FreeInstance();								// cleanup -- should call if GetInstance ever called.

	// alloc/free line lists
	INT AllocateLineList( 
		UBOOL bClip=true, 
		UBOOL bHide=false, 
		UBOOL bOnceOnly=false );							// adds new entry to LineLists and returns key

	void FreeLineList( INT Key );							// delete specified line list

	// clearing lines
	static void ClearAllLines();							// empty all lists (keys remain valid)
	void ClearLines( INT Key );								// empty specified list (keys remain valid)

	// modifiers -- all line lists
	static void SetClipAllLines( UBOOL bVal );				// set clipping for all lines in all lists

	// modififers -- specified list
	void SetClipLines( INT Key, UBOOL bVal ); 				// set bClip flag for entire list
	void SetHideLines( INT Key, UBOOL bVal ); 				// set bHide flag for entire list
	void ToggleClipLines( INT Key );						// toggle bClip flag for entire list
	void ToggleHideLines( INT Key );						// toggle bHide flag for entire list

	void SetLineColors( INT Key, const FPlane& Color ); 	// change color of all lines in list

	// misc useful functions
	void OffsetLines( INT Key, const FVector& Offset );		// offset Start+End of all lines in specified list list

	// query functions
	UBOOL  GetClipLines( INT Key ) const; 					// fetch bClip flag for given list
	UBOOL  GetHideLines( INT Key ) const; 					// fetch bHide flag for given list

	FPlane GetLineColor( INT Key, INT Index ) const;		// fetch Color being used for given line in given list

	INT GetNumLists();										// return the number of active lists
	INT GetNumLines();										// return the total number of lines in all active lists

	// add lines
	void AddLine( 
		INT Key, 
		const FVector& Start, 
		const FVector& End, 
		const FPlane& Color=DEFAULTLINECOLOR );

	void AddLines( 
		INT Key, 
		const TArray<FVector>& EndPoints, 
		const FPlane& Color=DEFAULTLINECOLOR );

	// helpers for creating more complex objects
	void AddArrow( 
		INT Key, 
		const FVector& Start, 
		const FVector& End, 
		const FPlane& Color=DEFAULTLINECOLOR, 
		const FPlane& ArrowColor=DEFAULTLINECOLOR, 
		FLOAT ArrowBaseOffset=DefaultArrowBaseOffset, 
		FLOAT ArrowBaseHalfLength=DefaultArrowBaseHalfLength, 
		FLOAT ArrowLength=DefaultArrowLength );

	void AddArrows( 
		INT Key, 
		const TArray<FVector>& EndPoints, 
		const FPlane& Color=DEFAULTLINECOLOR, 
		const FPlane& ArrowColor=DEFAULTLINECOLOR, 
		FLOAT ArrowBaseOffset=DefaultArrowBaseOffset, 
		FLOAT ArrowBaseHalfLength=DefaultArrowBaseHalfLength, 
		FLOAT ArrowLength=DefaultArrowLength );

	void AddBox( 
		INT Key,
		const FVector& Location,
		FLOAT XExtent, 
		FLOAT YExtent, 
		FLOAT ZExtent,
		const FPlane& Color=DEFAULTLINECOLOR,
		FLOAT GridTopBottomSpacing=0.0f, 
		FLOAT GridSidesSpacing=0.0f );

	void AddBox( 
		INT Key,
		const FVector& Location,
		const FVector& Extent, 
		const FPlane& Color=DEFAULTLINECOLOR,
		FLOAT GridTopBottomSpacing=0.0f, 
		FLOAT GridSidesSpacing=0.0f );

	void AddFBox( 
		INT Key,
		const FBox Box,
		const FPlane& Color=DEFAULTLINECOLOR,
		FLOAT GridTopBottomSpacing=0.0f, 
		FLOAT GridSidesSpacing=0.0f );

	void AddCylinder( 
		INT Key,
		const FVector& Location,
		FLOAT Radius, 
		FLOAT Height,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true );

	void AddActor(
		INT Key,
		const AActor& A,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true );

	void AddActor(
		INT Key,
		const AActor* A,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true )
	{
		AddActor( Key, *A, Color, NumCylinderSegments, bTopBottomGrid );
	};

	// rendering -- called from game/editor renderers
	static void Draw( FSceneNode* Frame, UBOOL bForceEditorDraw=false );

	// for use with helper macros (these use default key 0)
	void AddLine0( 
		const FVector& Start, 
		const FVector& End, 
		const FPlane& Color=DEFAULTLINECOLOR )
	{
		AddLine( 0, Start, End, Color );
	}

	void AddLines0( 
		const TArray<FVector>& EndPoints, 
		const FPlane& Color=DEFAULTLINECOLOR )
	{
		AddLines( 0, EndPoints, Color );
	}

	void AddArrow0( 
		const FVector& Start, 
		const FVector& End, 
		const FPlane& Color=DEFAULTLINECOLOR, 
		const FPlane& ArrowColor=DEFAULTLINECOLOR, 
		FLOAT ArrowBaseOffset=DefaultArrowBaseOffset, 
		FLOAT ArrowBaseHalfLength=DefaultArrowBaseHalfLength, 
		FLOAT ArrowLength=DefaultArrowLength )
	{
		AddArrow( 0, Start, End, Color, ArrowColor, ArrowBaseOffset, ArrowBaseHalfLength, ArrowLength );
	}

	void AddArrows0( 
		const TArray<FVector>& EndPoints, 
		const FPlane& Color=DEFAULTLINECOLOR, 
		const FPlane& ArrowColor=DEFAULTLINECOLOR, 
		FLOAT ArrowBaseOffset=DefaultArrowBaseOffset, 
		FLOAT ArrowBaseHalfLength=DefaultArrowBaseHalfLength, 
		FLOAT ArrowLength=DefaultArrowLength )
	{
		AddArrows( 0, EndPoints, Color, ArrowColor, ArrowBaseOffset, ArrowBaseHalfLength, ArrowLength );
	}

	void AddBox0( 
		const FVector& Location,
		FLOAT XExtent, 
		FLOAT YExtent, 
		FLOAT ZExtent,
		const FPlane& Color=DEFAULTLINECOLOR,
		FLOAT GridTopBottomSpacing=0.0f, 
		FLOAT GridSidesSpacing=0.0f )
	{
		AddBox( 0, Location, XExtent, YExtent, ZExtent, Color, GridTopBottomSpacing, GridSidesSpacing );
	}

	void AddBox0( 
		const FVector& Location,
		const FVector& Extent, 
		const FPlane& Color=DEFAULTLINECOLOR,
		FLOAT GridTopBottomSpacing=0.0f, 
		FLOAT GridSidesSpacing=0.0f )
	{
		AddBox( 0, Location, Extent, Color, GridTopBottomSpacing, GridSidesSpacing );
	}

	void AddFBox0( 
		const FBox Box,
		const FPlane& Color=DEFAULTLINECOLOR,
		FLOAT GridTopBottomSpacing=0.0f, 
		FLOAT GridSidesSpacing=0.0f )
	{
		AddFBox( 0, Box, Color, GridTopBottomSpacing, GridSidesSpacing );
	}

	void AddCylinder0( 
		const FVector& Location,
		FLOAT Radius, 
		FLOAT Height,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true )
	{
		AddCylinder( 0, Location, Radius, Height, Color, NumCylinderSegments, bTopBottomGrid );
	}

	void AddActor0(
		const AActor& A,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true )
	{
		AddActor( 0, A, Color, NumCylinderSegments, bTopBottomGrid );
	}

	void AddActor0(
		const AActor* A,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true )
	{
		AddActor( 0, A, Color, NumCylinderSegments, bTopBottomGrid );
	}
	
	void AddActorAt0(
		const AActor& A,
		const FVector& Location,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true )
	{
		AddCylinder( 0, Location, A.CollisionRadius, A.CollisionHeight, Color, NumCylinderSegments, bTopBottomGrid );
	}

	void AddActorAt0(
		const AActor* A,
		const FVector& Location,
		const FPlane& Color=DEFAULTLINECOLOR,
		INT NumCylinderSegments=DefaultCylinderSegments,
		UBOOL bTopBottomGrid=true )
	{
		AddCylinder( 0, Location, A->CollisionRadius, A->CollisionHeight, Color, NumCylinderSegments, bTopBottomGrid );
	}
};

#endif //_INC_FLineManager_
//#endif