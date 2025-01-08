//=============================================================================
// Legend.cpp
//=============================================================================

#include "Legend.h"
#include "FLineManager.h"

// Register names.

#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) LEGEND_API FName LEGEND_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "LegendClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY


//-----------------------------------------------------------------------------
// package implementation
//-----------------------------------------------------------------------------

IMPLEMENT_PACKAGE_EX(Legend)
{

	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name) LEGEND_##name = FName(TEXT(#name),FNAME_Intrinsic);
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#include "LegendClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY

}

//-----------------------------------------------------------------------------
// intrinsic class implementations
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS(ABreakableItem);
IMPLEMENT_CLASS(ADynamicBlockingVolume);

/*-----------------------------------------------------------------------------
Dynamic blocking volume support.

Most of the code for setting up the bsp nodes for the blocking volume was taken
directly out of the editor code. This includes

	bspBuild
	bspRefresh
	bspBuildBounds
	
and code for functions called by these. At this point I'm leaning towards *not*
refactoring the editor code (e.g. move UEditorEngine functions into UEngine 
and/or export some of the non-class member functions) so we don't have duplicate
code because a) we're running out of time on this project and b) if we do this
future merges will be more painful c) some stuff would have to be generalized,
e.g. we don't want some checks and logs when the code is called in-game as
opposed to in the editor.
-----------------------------------------------------------------------------*/

//#define DBVDEBUGMODE 1

#ifdef DBVDEBUGMODE
#define DBVDEBUG(func) func
#define DBVCHECK( x ) check( x )
#else
#define DBVDEBUG(func)
#define DBVCHECK( x )
#endif

//#define DEBUGFACES 1

#ifdef DEBUGFACES
static void DrawFace( FFace& F, FPlane Clr )
{
	debugf( L"  drawing face with %d points", F.Points.Num() );
	for( int ii=0; ii<F.Points.Num()-1; ii++ )
		ADDLINE( F.Points( ii ), F.Points( ii+1 ), Clr );

	ADDLINE( F.Points( F.Points.Num()-1 ), F.Points( 0 ), Clr );
}

static void DrawFaces( TArray<FFace>* Faces, FPlane Clr )
{
	debugf( L"Drawing %d faces", Faces->Num() );
	for( int ii=0; ii<Faces->Num(); ii++ )
		DrawFace( (*Faces)( ii ), Clr );
}
#endif

//-----------------------------------------------------------------------------
// BEGIN CODE COPIED FROM UnBSP, UnVisi, UnEdCsg etc.

enum ENodePlace 
{
	NODE_Back		= 0, // Node is in back of parent              -> Bsp[iParent].iBack.
	NODE_Front		= 1, // Node is in front of parent             -> Bsp[iParent].iFront.
	NODE_Plane		= 2, // Node is coplanar with parent           -> Bsp[iParent].iPlane.
	NODE_Root		= 3, // Node is the Bsp root and has no parent -> Bsp[0].
};

static QWORD BuildZoneMasks( UModel* Model, INT iNode )
{
	guard(FEditorVisibility::BuildZoneMasks);

	FBspNode& Node = Model->Nodes(iNode);
	QWORD ZoneMask = 0;

	if( Node.iZone[0]!=0 ) ZoneMask |= ((QWORD)1) << Node.iZone[0];
	if( Node.iZone[1]!=0 ) ZoneMask |= ((QWORD)1) << Node.iZone[1];

	if( Node.iFront != INDEX_NONE )	ZoneMask |= BuildZoneMasks( Model, Node.iFront );
	if( Node.iBack  != INDEX_NONE )	ZoneMask |= BuildZoneMasks( Model, Node.iBack );
	if( Node.iPlane != INDEX_NONE )	ZoneMask |= BuildZoneMasks( Model, Node.iPlane );

	Node.ZoneMask = ZoneMask;

	return ZoneMask;
	unguard;
}

static FPoly BuildInfiniteFPoly( UModel* Model, INT iNode )
{
	guard(BuildInfiniteFPoly);

	FBspNode &Node   = Model->Nodes  (iNode       );
	FBspSurf &Poly   = Model->Surfs  (Node.iSurf  );
	FVector  Base    = Poly.Plane * Poly.Plane.W;
	FVector  Normal  = Poly.Plane;
	FVector	 Axis1,Axis2;

	// Find two non-problematic axis vectors.
	Normal.FindBestAxisVectors( Axis1, Axis2 );

	// Set up the FPoly.
	FPoly EdPoly;
	EdPoly.Init();
	EdPoly.NumVertices = 4;
	EdPoly.Normal      = Normal;
	EdPoly.Base        = Base;
	EdPoly.Vertex[0]   = Base + Axis1*WORLD_MAX + Axis2*WORLD_MAX;
	EdPoly.Vertex[1]   = Base - Axis1*WORLD_MAX + Axis2*WORLD_MAX;
	EdPoly.Vertex[2]   = Base - Axis1*WORLD_MAX - Axis2*WORLD_MAX;
	EdPoly.Vertex[3]   = Base + Axis1*WORLD_MAX - Axis2*WORLD_MAX;

	return EdPoly;
	unguard;
}

static void SplitPartitioner
(
	UModel*	Model,
	FPoly**	PolyList,
	FPoly**	FrontList,
	FPoly**	BackList,
	INT		n,
	INT		nPolys,
	INT&	nFront, 
	INT&	nBack, 
	FPoly	InfiniteEdPoly
)
{
	FPoly FrontPoly,BackPoly;
	while( n < nPolys )
	{
		if( InfiniteEdPoly.NumVertices >= FPoly::VERTEX_THRESHOLD )
		{
			FPoly Half;
			InfiniteEdPoly.SplitInHalf(&Half);
			SplitPartitioner(Model,PolyList,FrontList,BackList,n,nPolys,nFront,nBack,Half);
		}
		FPoly* Poly = PolyList[n];
		switch( InfiniteEdPoly.SplitWithPlane(Poly->Vertex[0],Poly->Normal,&FrontPoly,&BackPoly,0) )
		{
			case SP_Coplanar:
				// May occasionally happen.
				DBVDEBUG( debugf( NAME_Log, TEXT("FilterBound: Got inficoplanar") ) );
				break;

			case SP_Front:
				// Shouldn't happen if hull is correct.
				DBVDEBUG( debugf( NAME_Log, TEXT("FilterBound: Got infifront") ) );
				return;

			case SP_Split:
				InfiniteEdPoly = BackPoly;
				break;

			case SP_Back:
				break;
		}
		n++;
	}

	FPoly* New = new(GMem)FPoly;
	*New = InfiniteEdPoly;
	New->Reverse();
	New->iBrushPoly |= 0x40000000;
	FrontList[nFront++] = New;

	New = new(GMem)FPoly;
	*New = InfiniteEdPoly;
	BackList[nBack++] = New;
}

static void UpdateBoundWithPolys( FBox& Bound, FPoly** PolyList, INT nPolys )
{
	guard(UpdateBoundWithPolys);
	for( INT i=0; i<nPolys; i++ )
		for( INT j=0; j<PolyList[i]->NumVertices; j++ )
			Bound += PolyList[i]->Vertex[j];
	unguard;
}

static void UpdateConvolutionWithPolys( UModel *Model, INT iNode, FPoly **PolyList, int nPolys )
{
	guard(UpdateConvolutionWithPolys);
	FBox Box(0);

	FBspNode &Node = Model->Nodes(iNode);
	Node.iCollisionBound = Model->LeafHulls.Num();
	for( int i=0; i<nPolys; i++ )
	{
		if( PolyList[i]->iBrushPoly != INDEX_NONE )
		{
			int j;
			for( j=0; j<i; j++ )
				if( PolyList[j]->iBrushPoly == PolyList[i]->iBrushPoly )
					break;
			if( j >= i )
				Model->LeafHulls.AddItem(PolyList[i]->iBrushPoly);
		}
		for( int j=0; j<PolyList[i]->NumVertices; j++ )
			Box += PolyList[i]->Vertex[j];
	}
	Model->LeafHulls.AddItem(INDEX_NONE);

	// Add bounds.
	Model->LeafHulls.AddItem( *(INT*)&Box.Min.X );
	Model->LeafHulls.AddItem( *(INT*)&Box.Min.Y );
	Model->LeafHulls.AddItem( *(INT*)&Box.Min.Z );
	Model->LeafHulls.AddItem( *(INT*)&Box.Max.X );
	Model->LeafHulls.AddItem( *(INT*)&Box.Max.Y );
	Model->LeafHulls.AddItem( *(INT*)&Box.Max.Z );

	unguard;
}

static void FilterBound
(
	UModel*			Model,
	FBox*			ParentBound,
	INT				iNode,
	FPoly**			PolyList,
	INT				nPolys,
	INT				Outside
)
{
	FMemMark Mark(GMem);
	FBspNode&		Node	= Model->Nodes  (iNode);
	FBspSurf&		Surf	= Model->Surfs  (Node.iSurf);
	//BEGIN CONTROVERSIAL CODE

	// OLD: HIDEOUS BUG.
	//FVector&		Base	= Model->Points(Model->Verts(Node.iVertPool).pVertex);

	// NEW: FIXED. Sort of.
	//FVector&		Base	= Model->Points(Surf.pBase);

	// NEWER. A less holy workaround.
	//FVector&		Base	= Node.NumVertices>0? Model->Points(Model->Verts(Node.iVertPool).pVertex): Model->Points(Surf.pBase);

	// NEWEST:

	FVector	Base = Surf.Plane * Surf.Plane.W;

	//END CONTROVERSIAL CODE
	FVector&		Normal	= Model->Vectors(Surf.vNormal);
	FBox			Bound(0);

	Bound.Min.X = Bound.Min.Y = Bound.Min.Z = +WORLD_MAX;
	Bound.Max.X = Bound.Max.Y = Bound.Max.Z = -WORLD_MAX;

	// Split bound into front half and back half.
	FPoly** FrontList = new(GMem,nPolys*2+16)FPoly*; int nFront=0;
	FPoly** BackList  = new(GMem,nPolys*2+16)FPoly*; int nBack=0;

	FPoly* FrontPoly  = new(GMem)FPoly;
	FPoly* BackPoly   = new(GMem)FPoly;

	for( INT i=0; i<nPolys; i++ )
	{
		FPoly *Poly = PolyList[i];
		switch( Poly->SplitWithPlane( Base, Normal, FrontPoly, BackPoly, 0 ) )
		{
			case SP_Coplanar:
				DBVDEBUG( debugf( NAME_Log, TEXT("FilterBound: Got coplanar") ) );
				FrontList[nFront++] = Poly;
				BackList[nBack++] = Poly;
				break;
			
			case SP_Front:
				FrontList[nFront++] = Poly;
				break;
			
			case SP_Back:
				BackList[nBack++] = Poly;
				break;
			
			case SP_Split:
				if( FrontPoly->NumVertices >= FPoly::VERTEX_THRESHOLD )
				{
					FPoly *Half = new(GMem)FPoly;
					FrontPoly->SplitInHalf(Half);
					FrontList[nFront++] = Half;
				}
				FrontList[nFront++] = FrontPoly;

				if( BackPoly->NumVertices >= FPoly::VERTEX_THRESHOLD )
				{
					FPoly *Half = new(GMem)FPoly;
					BackPoly->SplitInHalf(Half);
					BackList[nBack++] = Half;
				}
				BackList [nBack++] = BackPoly;

				FrontPoly = new(GMem)FPoly;
				BackPoly  = new(GMem)FPoly;
				break;

			default:
				appErrorf( TEXT("FZoneFilter::FilterToLeaf: Unknown split code") );
		}
	}
	if( nFront && nBack )
	{
		// Add partitioner plane to front and back.
		FPoly InfiniteEdPoly = BuildInfiniteFPoly( Model, iNode );
		InfiniteEdPoly.iBrushPoly = iNode;

		SplitPartitioner(Model,PolyList,FrontList,BackList,0,nPolys,nFront,nBack,InfiniteEdPoly);
	}
#ifdef DBVDEBUGMODE
	else
	{
		if( !nFront ) debugf( NAME_Log, TEXT("FilterBound: Empty fronthull") );
		if( !nBack  ) debugf( NAME_Log, TEXT("FilterBound: Empty backhull") );
	}
#endif
	// Recursively update all our childrens' bounding volumes.
	if( nFront > 0 )
	{
		if( Node.iFront != INDEX_NONE )
			FilterBound( Model, &Bound, Node.iFront, FrontList, nFront, Outside || Node.IsCsg() );
		else if( Outside || Node.IsCsg() )
			UpdateBoundWithPolys( Bound, FrontList, nFront );
		else
			UpdateConvolutionWithPolys( Model, iNode, FrontList, nFront );
	}
	if( nBack > 0 )
	{
		if( Node.iBack != INDEX_NONE)
			FilterBound( Model, &Bound,Node.iBack, BackList, nBack, Outside && !Node.IsCsg() );
		else if( Outside && !Node.IsCsg() )
			UpdateBoundWithPolys( Bound, BackList, nBack );
		else
			UpdateConvolutionWithPolys( Model, iNode, BackList, nBack );
	}

	// Apply this bound to this node if it's not a leaf.
	if( Node.iRenderBound==INDEX_NONE && (Node.iFront!=INDEX_NONE || Node.iBack!=INDEX_NONE) )
	{
		Node.iRenderBound = Model->Bounds.Add();
		Model->Bounds(Node.iRenderBound) = Bound;
		//Node.InclusiveSphereBound = FSphere(&Bound.Min,2);
	}

	// Update parent bound to enclose this bound.
	if( ParentBound )
		*ParentBound += Bound;

	Mark.Pop();
}

static void bspBuildBounds( UModel* Model )
{
	guard(UEditorEngine::bspBuildBounds);
	if( Model->Nodes.Num()==0 )
		return;

	BuildZoneMasks( Model, 0 );
	
	FPoly Polys[6], *PolyList[6];
	guard(Init);
	for( int i=0; i<6; i++ )
	{
		PolyList[i] = &Polys[i];
		PolyList[i]->Init();
		PolyList[i]->NumVertices=4;
		PolyList[i]->iBrushPoly = INDEX_NONE;
	}

	Polys[0].Vertex[0]=FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX,HALF_WORLD_MAX);
	Polys[0].Vertex[1]=FVector( HALF_WORLD_MAX,-HALF_WORLD_MAX,HALF_WORLD_MAX);
	Polys[0].Vertex[2]=FVector( HALF_WORLD_MAX, HALF_WORLD_MAX,HALF_WORLD_MAX);
	Polys[0].Vertex[3]=FVector(-HALF_WORLD_MAX, HALF_WORLD_MAX,HALF_WORLD_MAX);
	Polys[0].Normal   =FVector( 0.000000,  0.000000,  1.000000 );
	Polys[0].Base     =Polys[0].Vertex[0];

	Polys[1].Vertex[0]=FVector(-HALF_WORLD_MAX, HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[1].Vertex[1]=FVector( HALF_WORLD_MAX, HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[1].Vertex[2]=FVector( HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[1].Vertex[3]=FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[1].Normal   =FVector( 0.000000,  0.000000, -1.000000 );
	Polys[1].Base     =Polys[1].Vertex[0];

	Polys[2].Vertex[0]=FVector(-HALF_WORLD_MAX,HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[2].Vertex[1]=FVector(-HALF_WORLD_MAX,HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[2].Vertex[2]=FVector( HALF_WORLD_MAX,HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[2].Vertex[3]=FVector( HALF_WORLD_MAX,HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[2].Normal   =FVector( 0.000000,  1.000000,  0.000000 );
	Polys[2].Base     =Polys[2].Vertex[0];

	Polys[3].Vertex[0]=FVector( HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[3].Vertex[1]=FVector( HALF_WORLD_MAX,-HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[3].Vertex[2]=FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[3].Vertex[3]=FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[3].Normal   =FVector( 0.000000, -1.000000,  0.000000 );
	Polys[3].Base     =Polys[3].Vertex[0];

	Polys[4].Vertex[0]=FVector(HALF_WORLD_MAX, HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[4].Vertex[1]=FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[4].Vertex[2]=FVector(HALF_WORLD_MAX,-HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[4].Vertex[3]=FVector(HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[4].Normal   =FVector( 1.000000,  0.000000,  0.000000 );
	Polys[4].Base     =Polys[4].Vertex[0];

	Polys[5].Vertex[0]=FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[5].Vertex[1]=FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[5].Vertex[2]=FVector(-HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	Polys[5].Vertex[3]=FVector(-HALF_WORLD_MAX, HALF_WORLD_MAX,-HALF_WORLD_MAX);
	Polys[5].Normal   =FVector(-1.000000,  0.000000,  0.000000 );
	Polys[5].Base     =Polys[5].Vertex[0];
	unguard;

	// Empty bounds and hulls.
	guard(Empty);
	Model->Bounds.Empty();
	Model->LeafHulls.Empty();
	for( int i=0; i<Model->Nodes.Num(); i++ )
	{
		Model->Nodes(i).iRenderBound     = INDEX_NONE;
		Model->Nodes(i).iCollisionBound  = INDEX_NONE;
	}
	unguard;

	guard(Filter);
	FilterBound( Model, NULL, 0, PolyList, 6, Model->RootOutside );
	
	Model->Bounds.Shrink();
	unguard;

	DBVDEBUG( debugf( NAME_Log, TEXT("bspBuildBounds: Generated %i bounds, %i hulls"), Model->Bounds.Num(), Model->LeafHulls.Num() ) );
	unguard;
}

static void TagReferencedNodes( UModel *Model, INT *NodeRef, INT *PolyRef, INT iNode )
{
	FBspNode &Node = Model->Nodes(iNode);

	NodeRef[iNode     ] = 0;
	PolyRef[Node.iSurf] = 0;

	if( Node.iFront != INDEX_NONE ) TagReferencedNodes(Model,NodeRef,PolyRef,Node.iFront);
	if( Node.iBack  != INDEX_NONE ) TagReferencedNodes(Model,NodeRef,PolyRef,Node.iBack );
	if( Node.iPlane != INDEX_NONE ) TagReferencedNodes(Model,NodeRef,PolyRef,Node.iPlane);
}

static void bspRefresh( UModel *Model, int NoRemapSurfs )
{
	guard(UEditorEngine::bspRefresh);

	FMemMark Mark(GMem);
	INT *VectorRef, *PointRef, *NodeRef, *PolyRef, i;
	TArray<INT*>	VertexRef;
	BYTE  B;

	// Remove unreferenced Bsp surfs.
	NodeRef		= new(GMem,MEM_Oned,Model->Nodes.Num())INT;
	PolyRef		= new(GMem,MEM_Oned,Model->Surfs.Num())INT;
	if( Model->Nodes.Num() > 0 )
		TagReferencedNodes( Model, NodeRef, PolyRef, 0 );
	
	if( NoRemapSurfs )
		appMemzero(PolyRef,Model->Surfs.Num() * sizeof (INT));

	// Remap Bsp nodes and surfs.
	int n=0;
	for( i=0; i<Model->Surfs.Num(); i++ )
	{
		if( PolyRef[i]!=INDEX_NONE )
		{
			Model->Surfs(n) = Model->Surfs(i);
			PolyRef[i]=n++;
		}
	}
	DBVDEBUG( debugf( NAME_Log, TEXT("Polys: %i -> %i"), Model->Surfs.Num(), n ) );
	Model->Surfs.Remove( n, Model->Surfs.Num()-n );

	n=0;
	for( i=0; i<Model->Nodes.Num(); i++ ) if( NodeRef[i]!=INDEX_NONE )
	{
		Model->Nodes(n) = Model->Nodes(i);
		NodeRef[i]=n++;
	}
	DBVDEBUG( debugf( NAME_Log, TEXT("Nodes: %i -> %i"), Model->Nodes.Num(), n ) );
	Model->Nodes.Remove( n, Model->Nodes.Num()-n  );

	// Update Bsp nodes.
	for( i=0; i<Model->Nodes.Num(); i++ )
	{
		FBspNode *Node = &Model->Nodes(i);
		Node->iSurf = PolyRef[Node->iSurf];
		if (Node->iFront != INDEX_NONE) Node->iFront = NodeRef[Node->iFront];
		if (Node->iBack  != INDEX_NONE) Node->iBack  = NodeRef[Node->iBack];
		if (Node->iPlane != INDEX_NONE) Node->iPlane = NodeRef[Node->iPlane];
	}

	// Remove unreferenced points and vectors.
	VectorRef = new(GMem,MEM_Oned,Model->Vectors.Num())INT;
	PointRef  = new(GMem,MEM_Oned,Model->Points .Num ())INT;

	// Check Bsp surfs.
	for( i=0; i<Model->Surfs.Num(); i++ )
	{
		FBspSurf *Surf = &Model->Surfs(i);
		VectorRef [Surf->vNormal   ] = 0;
		VectorRef [Surf->vTextureU ] = 0;
		VectorRef [Surf->vTextureV ] = 0;
		PointRef  [Surf->pBase     ] = 0;
	}

	// Check Bsp nodes.
	for( i=0; i<Model->Nodes.Num(); i++ )
	{
		// Tag all points used by nodes.
		FBspNode*	Node		= &Model->Nodes(i);
		FVert*		VertPool	= &Model->Verts(Node->iVertPool);
		for( B=0; B<Node->NumVertices;  B++ )
		{
			PointRef[VertPool->pVertex] = 0;
			VertPool++;
		}
		Node++;
	}

	// Remap points.
	n=0; 
	for( i=0; i<Model->Points.Num(); i++ ) if( PointRef[i]!=INDEX_NONE )
	{
		Model->Points(n) = Model->Points(i);
		PointRef[i] = n++;
	}
	DBVDEBUG( debugf( NAME_Log, TEXT("Points: %i -> %i"), Model->Points.Num(), n ) );
	Model->Points.Remove( n, Model->Points.Num()-n );
	DBVCHECK(Model->Points.Num()==n);

	// Remap vectors.
	n=0; for (i=0; i<Model->Vectors.Num(); i++) if (VectorRef[i]!=INDEX_NONE)
	{
		Model->Vectors(n) = Model->Vectors(i);
		VectorRef[i] = n++;
	}
	DBVDEBUG( debugf( NAME_Log, TEXT("Vectors: %i -> %i"), Model->Vectors.Num(), n ) );
	Model->Vectors.Remove( n, Model->Vectors.Num()-n );

	// Update Bsp surfs.
	for( i=0; i<Model->Surfs.Num(); i++ )
	{
		FBspSurf *Surf	= &Model->Surfs(i);
		Surf->vNormal   = VectorRef [Surf->vNormal  ];
		Surf->vTextureU = VectorRef [Surf->vTextureU];
		Surf->vTextureV = VectorRef [Surf->vTextureV];
		Surf->pBase     = PointRef  [Surf->pBase    ];
	}

	// Update Bsp nodes.
	for( i=0; i<Model->Nodes.Num(); i++ )
	{
		FBspNode*	Node		= &Model->Nodes(i);
		FVert*		VertPool	= &Model->Verts(Node->iVertPool);
		for( B=0; B<Node->NumVertices;  B++ )
		{			
			VertPool->pVertex = PointRef [VertPool->pVertex];
			VertPool++;
		}

		Node++;
	}

	// Shrink the objects.
	Model->ShrinkModel();

	Mark.Pop();
	unguard;
}

static FPoly *FindBestSplit
(
	INT					NumPolys,
	FPoly**				PolyList,
	EBspOptimization	Opt,
	INT					Balance,
	INT					InPortalBias
)
{
	guard(FindBestSplit);
	DBVCHECK(NumPolys>0);

	// No need to test if only one poly.
	if( NumPolys==1 )
		return PolyList[0];

	FPoly   *Poly, *Best=NULL;
	FLOAT   Score, BestScore;
	int     i, Index, j, Inc;
	int     Splits, Front, Back, Coplanar, AllSemiSolids;

	//PortalBias -- added by Legend on 4/12/2000
	float	PortalBias = InPortalBias / 100.0f;
	Balance &= 0xFF;								// keep only the low byte to recover "Balance"
	//GLog->Logf( TEXT("Balance=%d PortalBias=%f"), Balance, PortalBias );

	if		(Opt==BSP_Optimal)  Inc = 1;					// Test lots of nodes.
	else if (Opt==BSP_Good)		Inc = Max(1,NumPolys/20);	// Test 20 nodes.
	else /* BSP_Lame */			Inc = Max(1,NumPolys/4);	// Test 4 nodes.

	// See if there are any non-semisolid polygons here.
	for( i=0; i<NumPolys; i++ )
		if( !(PolyList[i]->PolyFlags & PF_AddLast) )
			break;
	AllSemiSolids = (i>=NumPolys);

	// Search through all polygons in the pool and find:
	// A. The number of splits each poly would make.
	// B. The number of front and back nodes the polygon would create.
	// C. Number of coplanars.
	BestScore = 0;
	for( i=0; i<NumPolys; i+=Inc )
	{
		Splits = Front = Back = Coplanar = 0;
		Index = i-1;
		do
		{
			Index++;
			Poly = PolyList[Index];
		} while( Index<(i+Inc) && Index<NumPolys 
			&& ( (Poly->PolyFlags & PF_AddLast) && !(Poly->PolyFlags & PF_Portal) )
			&& !AllSemiSolids );
		if( Index>=i+Inc || Index>=NumPolys )
			continue;

		for( j=0; j<NumPolys; j+=Inc ) if( j != Index )
		{
			FPoly *OtherPoly = PolyList[j];
			switch( OtherPoly->SplitWithPlaneFast( FPlane( Poly->Vertex[0], Poly->Normal), NULL, NULL ) )
			{
				case SP_Coplanar:
					Coplanar++;
					break;

				case SP_Front:
					Front++;
					break;

				case SP_Back:
					Back++;
					break;

				case SP_Split:
					// Disfavor splitting polys that are zone portals.
					if( !(OtherPoly->PolyFlags & PF_Portal) )
						Splits++;
					else
						Splits += 16;
					break;
			}
		}
		// added by Legend 1/31/1999
		// Score optimization: minimize cuts vs. balance tree (as specified in BSP Rebuilder dialog)
		Score = ( 100.0 - float(Balance) ) * Splits + float(Balance) * Abs( Front - Back );
		if( Poly->PolyFlags & PF_Portal )
		{
			// PortalBias -- added by Legend on 4/12/2000
			//
			// PortalBias enables level designers to control the effect of Portals on the BSP.
			// This effect can range from 0.0 (ignore portals), to 1.0 (portals cut everything).
			//
			// In builds prior to this (since the 221 build dating back to 1/31/1999) the bias
			// has been 1.0 causing the portals to cut the BSP in ways that will potentially
			// degrade level performance, and increase the BSP complexity.
			// 
			// By setting the bias to a value between 0.3 and 0.7 the positive effects of 
			// the portals are preserved without giving them unreasonable priority in the BSP.
			//
			// Portals should be weighted high enough in the BSP to separate major parts of the
			// level from each other (pushing entire rooms down the branches of the BSP), but
			// should not be so high that portals cut through adjacent geometry in a way that
			// increases complexity of the room being (typically, accidentally) cut.
			//
			Score -= ( 100.0 - float(Balance) ) * Splits * PortalBias; // ignore PortalBias of the split polys -- bias toward portal selection for cutting planes!
		}
		//debugf( "  %4d: Score = %f (Front = %4d, Back = %4d, Splits = %4d, Flags = %08X)", Index, Score, Front, Back, Splits, Poly->PolyFlags ); //LEC

		if( Score<BestScore || !Best )
		{
			Best      = Poly;
			BestScore = Score;
		}
	}
	DBVCHECK(Best);
	return Best;
	unguard;
}

static INT AddThing( TArray<FVector>& Vectors, FVector& V, FLOAT Thresh, int Check )
{
	if( Check )
	{
		// See if this is very close to an existing point/vector.		
		for( INT i=0; i<Vectors.Num(); i++ )
		{
			const FVector &TableVect = Vectors(i);
			FLOAT Temp=(V.X - TableVect.X);
			if( (Temp > -Thresh) && (Temp < Thresh) )
			{
				Temp=(V.Y - TableVect.Y);
				if( (Temp > -Thresh) && (Temp < Thresh) )
				{
					Temp=(V.Z - TableVect.Z);
					if( (Temp > -Thresh) && (Temp < Thresh) )
					{
						// Found nearly-matching vector.
						return i;
					}
				}
			}
		}
	}
	return Vectors.AddItem( V );
}

static INT bspAddVector( UModel *Model, FVector *V, int Normal )
{
	guard(UEditorEngine::bspAddVector);
	return AddThing
	(
		Model->Vectors,
		*V,
		Normal ? THRESH_NORMALS_ARE_SAME : THRESH_VECTORS_ARE_NEAR,
		1
	);
	unguard;
}

static INT bspAddPoint( UModel *Model, FVector *V, int Exact )
{
	guard(UEditorEngine::bspAddPoint);
	FLOAT Thresh = Exact ? THRESH_POINTS_ARE_SAME : THRESH_POINTS_ARE_NEAR;

	// Try to find a match quickly from the Bsp. This finds all potential matches
	// except for any dissociated from nodes/surfaces during a rebuild.
	FVector Temp;
	INT pVertex;
	FLOAT NearestDist = Model->FindNearestVertex(*V,Temp,Thresh,pVertex);
	if( (NearestDist >= 0.0) && (NearestDist <= Thresh) )
	{
		// Found an existing point.
		return pVertex;
	}
	else
	{
		// No match found; add it slowly to find duplicates.
		//return AddThing( Model->Points, *V, Thresh, !FastRebuild );
		return AddThing( Model->Points, *V, Thresh, false );
	}
	unguard;
}

static INT bspAddNode
(
	UModel*		Model, 
	INT         iParent, 
	ENodePlace	NodePlace,
	DWORD		NodeFlags, 
	FPoly*		EdPoly
)
{
	guard(UEditorEngine::bspAddNode);
	
	if( NodePlace == NODE_Plane )
	{
		// Make sure coplanars are added at the end of the coplanar list so that 
		// we don't insert NF_IsNew nodes with non NF_IsNew coplanar children.
		while( Model->Nodes(iParent).iPlane != INDEX_NONE )
			iParent = Model->Nodes(iParent).iPlane;
	}
	FBspSurf* Surf = NULL;
	if( EdPoly->iLink == Model->Surfs.Num() )
	{
		INT NewIndex = Model->Surfs.AddZeroed();
		Surf = &Model->Surfs(NewIndex);

		// This node has a new polygon being added by bspBrushCSG; must set its properties here.
		Surf->pBase     	= bspAddPoint  (Model,&EdPoly->Base,1);
		Surf->vNormal   	= bspAddVector (Model,&EdPoly->Normal,1);
		Surf->vTextureU 	= bspAddVector (Model,&EdPoly->TextureU,0);
		Surf->vTextureV 	= bspAddVector (Model,&EdPoly->TextureV,0);
		Surf->Material  	= EdPoly->Material;
		Surf->Actor			= NULL;

		Surf->PolyFlags 	= EdPoly->PolyFlags & ~PF_NoAddToBSP;
		Surf->LightMapScale	= EdPoly->LightMapScale;

		Surf->Actor	 		= EdPoly->Actor;
		Surf->iBrushPoly	= EdPoly->iBrushPoly;

		Surf->Plane			= FPlane(EdPoly->Vertex[0],EdPoly->Normal);
	}
	else
	{
		DBVCHECK(EdPoly->iLink!=INDEX_NONE);
		DBVCHECK(EdPoly->iLink<Model->Surfs.Num());
		Surf = &Model->Surfs(EdPoly->iLink);
	}

	// Set NodeFlags.
	if( Surf->PolyFlags & PF_NotSolid              ) NodeFlags |= NF_NotCsg;
	if( Surf->PolyFlags & (PF_Invisible|PF_Portal) ) NodeFlags |= NF_NotVisBlocking;
	if( Surf->PolyFlags & PF_Masked                ) NodeFlags |= NF_ShootThrough;

	if( EdPoly->NumVertices > FBspNode::MAX_NODE_VERTICES )
	{
		// Split up into two coplanar sub-polygons (one with MAX_NODE_VERTICES vertices and
		// one with all the remaining vertices) and recursively add them.

		// Copy first bunch of verts.
		FMemMark Mark(GMem);
		FPoly *EdPoly1 = new(GMem)FPoly;
		*EdPoly1 = *EdPoly;
		EdPoly1->NumVertices = FBspNode::MAX_NODE_VERTICES;

		FPoly *EdPoly2 = new(GMem)FPoly;
		*EdPoly2 = *EdPoly;
		EdPoly2->NumVertices = EdPoly->NumVertices + 2 - FBspNode::MAX_NODE_VERTICES;

		// Copy first vertex then the remaining vertices.
		appMemmove
		(
			&EdPoly2->Vertex[1],
			&EdPoly->Vertex [FBspNode::MAX_NODE_VERTICES - 1],
			(EdPoly->NumVertices + 1 - FBspNode::MAX_NODE_VERTICES) * sizeof (FVector)
		);
		INT iNode = bspAddNode( Model, iParent, NodePlace, NodeFlags, EdPoly1 ); // Add this poly first.
		bspAddNode( Model, iNode,   NODE_Plane, NodeFlags, EdPoly2 ); // Then add other (may be bigger).

		Mark.Pop();
		return iNode; // Return coplanar "parent" node (not coplanar child)
	}
	else
	{
		// Add node.
		if( NodePlace!=NODE_Root )
			Model->Nodes.ModifyItem( iParent );
		INT iNode			 = Model->Nodes.AddZeroed();
		FBspNode& Node       = Model->Nodes(iNode);

		// Tell transaction tracking system that parent is about to be modified.
		FBspNode* Parent=NULL;
		if( NodePlace!=NODE_Root )
			Parent = &Model->Nodes(iParent);

		// Set node properties.
		Node.iSurf       	 = EdPoly->iLink;
		Node.NodeFlags   	 = NodeFlags;
		Node.iRenderBound    = INDEX_NONE;
		Node.iCollisionBound = INDEX_NONE;
		Node.ZoneMask		 = Parent ? Parent->ZoneMask : ~(QWORD)0;
		Node.Plane           = FPlane( EdPoly->Vertex[0], EdPoly->Normal );
		Node.iVertPool       = Model->Verts.Add(EdPoly->NumVertices);
		Node.iFront		     = INDEX_NONE;
		Node.iBack		     = INDEX_NONE;
		Node.iPlane		     = INDEX_NONE;
		if( NodePlace==NODE_Root )
		{
			Node.iLeaf[0]	 = INDEX_NONE;
			Node.iLeaf[1] 	 = INDEX_NONE;
			Node.iZone[0]	 = 0;
			Node.iZone[1]	 = 0;
		}
		else if( NodePlace==NODE_Front || NodePlace==NODE_Back )
		{
			INT ZoneFront=NodePlace==NODE_Front;
			Node.iLeaf[0]	 = Parent->iLeaf[ZoneFront];
			Node.iLeaf[1] 	 = Parent->iLeaf[ZoneFront];
			Node.iZone[0]	 = Parent->iZone[ZoneFront];
			Node.iZone[1]	 = Parent->iZone[ZoneFront];
		}
		else
		{
			INT IsFlipped    = (Node.Plane|Parent->Plane)<0.0;
			Node.iLeaf[0]    = Parent->iLeaf[IsFlipped  ];
			Node.iLeaf[1]    = Parent->iLeaf[1-IsFlipped];
			Node.iZone[0]    = Parent->iZone[IsFlipped  ];
			Node.iZone[1]    = Parent->iZone[1-IsFlipped];
		}

		// Link parent to this node.
		if     ( NodePlace==NODE_Front ) Parent->iFront = iNode;
		else if( NodePlace==NODE_Back  ) Parent->iBack  = iNode;
		else if( NodePlace==NODE_Plane ) Parent->iPlane = iNode;

		// Add all points to point table, merging nearly-overlapping polygon points
		// with other points in the poly to prevent criscrossing vertices from
		// being generated.

		// Must maintain Node->NumVertices on the fly so that bspAddPoint is always
		// called with the Bsp in a clean state.
		FVector	Points[FBspNode::MAX_NODE_VERTICES];
		BYTE	n           = EdPoly->NumVertices;
		Node.NumVertices = 0;
		FVert* VertPool	 = &Model->Verts( Node.iVertPool );
		for( BYTE i=0; i<n; i++ )
		{
			int pVertex = bspAddPoint(Model,&EdPoly->Vertex[i],0);
			if( Node.NumVertices==0 || VertPool[Node.NumVertices-1].pVertex!=pVertex )
			{
				Points[Node.NumVertices] = EdPoly->Vertex[i];
				VertPool[Node.NumVertices].iSide   = INDEX_NONE;
				VertPool[Node.NumVertices].pVertex = pVertex;
				Node.NumVertices++;
			}
		}
		if( Node.NumVertices>=2 && VertPool[0].pVertex==VertPool[Node.NumVertices-1].pVertex )
		{
			Node.NumVertices--;
		}
		if( Node.NumVertices < 3 )
		{
			//!!?? GErrors++;
			DBVDEBUG( debugf( NAME_Warning, TEXT("bspAddNode: Infinitesimal polygon %i (%i)"), Node.NumVertices, n ) );
			Node.NumVertices = 0;
		}

		Node.iSection = INDEX_NONE;
		Node.iLightMap = INDEX_NONE;

		// Calculate a bounding sphere for this node.

		Node.ExclusiveSphereBound = FSphere(Points,Node.NumVertices);

		return iNode;
	}
	unguard;
}

static void SplitPolyList
(
	UModel				*Model,
	INT                 iParent,
	ENodePlace			NodePlace,
	INT                 NumPolys,
	FPoly				**PolyList,
	EBspOptimization	Opt,
	INT					Balance,
	INT					PortalBias,
	INT					RebuildSimplePolys
)
{
	guard(SplitPolyList);
	FMemMark Mark(GMem);

	// To account for big EdPolys split up.
	int NumPolysToAlloc = NumPolys + 8 + NumPolys/4;
	int NumFront=0; FPoly **FrontList = new(GMem,NumPolysToAlloc)FPoly*;
	int NumBack =0; FPoly **BackList  = new(GMem,NumPolysToAlloc)FPoly*;

	FPoly *SplitPoly = FindBestSplit( NumPolys, PolyList, Opt, Balance, PortalBias );

	// Add the splitter poly to the Bsp with either a new BspSurf or an existing one.
	if( RebuildSimplePolys )
		SplitPoly->iLink = Model->Surfs.Num();

	INT iOurNode	 = bspAddNode(Model,iParent,NodePlace,0,SplitPoly);
	INT iPlaneNode = iOurNode;

	// Now divide all polygons in the pool into (A) polygons that are
	// in front of Poly, and (B) polygons that are in back of Poly.
	// Coplanar polys are inserted immediately, before recursing.

	// If any polygons are split by Poly, we ignrore the original poly,
	// split it into two polys, and add two new polys to the pool.

	FPoly *FrontEdPoly = new(GMem)FPoly;
	FPoly *BackEdPoly  = new(GMem)FPoly;
	for( INT i=0; i<NumPolys; i++ )
	{
		FPoly *EdPoly = PolyList[i];
		if( EdPoly == SplitPoly )
			continue;

		switch( EdPoly->SplitWithPlane( SplitPoly->Vertex[0], SplitPoly->Normal, FrontEdPoly, BackEdPoly, 0 ) )
		{
			case SP_Coplanar:
	            if( RebuildSimplePolys )
					EdPoly->iLink = Model->Surfs.Num()-1;
				iPlaneNode = bspAddNode( Model, iPlaneNode, NODE_Plane, 0, EdPoly );
				break;
			
			case SP_Front:
	            FrontList[NumFront++] = PolyList[i];
				break;
			
			case SP_Back:
	            BackList[NumBack++] = PolyList[i];
				break;
			
			case SP_Split:

				// Create front & back nodes.
				FrontList[NumFront++] = FrontEdPoly;
				BackList [NumBack ++] = BackEdPoly;

				// If newly-split polygons have too many vertices, break them up in half.
				if( FrontEdPoly->NumVertices >= FPoly::VERTEX_THRESHOLD )
				{
					FPoly *Temp = new(GMem)FPoly;
					FrontEdPoly->SplitInHalf(Temp);
					FrontList[NumFront++] = Temp;
				}
				if( BackEdPoly->NumVertices >= FPoly::VERTEX_THRESHOLD )
				{
					FPoly *Temp = new(GMem)FPoly;
					BackEdPoly->SplitInHalf(Temp);
					BackList[NumBack++] = Temp;
				}
				FrontEdPoly = new(GMem)FPoly;
				BackEdPoly  = new(GMem)FPoly;
				break;
		}
	}

	// Recursively split the front and back pools.
	if( NumFront > 0 ) SplitPolyList( Model, iOurNode, NODE_Front, NumFront, FrontList, Opt, Balance, PortalBias, RebuildSimplePolys );
	if( NumBack  > 0 ) SplitPolyList( Model, iOurNode, NODE_Back,  NumBack,  BackList,  Opt, Balance, PortalBias, RebuildSimplePolys );

	Mark.Pop();
	unguard;
}

static void bspBuild
(
	UModel*				Model, 
	EBspOptimization	Opt, 
	INT					Balance, 
	INT					PortalBias,
	INT					RebuildSimplePolys,
	INT					iNode
)
{
	guard(UEditorEngine::bspBuild);

#ifdef DBVDEBUGMODE
	INT OriginalPolys = Model->Polys->Element.Num();
#endif

	// Empty the model's tables.
	if( RebuildSimplePolys==1 )
	{
		// Empty everything but polys.
		Model->EmptyModel( 1, 0 );
	}
	else if( RebuildSimplePolys==0 )
	{
		// Empty node vertices.
		for( INT i=0; i<Model->Nodes.Num(); i++ )
			Model->Nodes(i).NumVertices = 0;

		// Refresh the Bsp.
		bspRefresh(Model,1);
		
		// Empty nodes.
		Model->EmptyModel( 0, 0 );
	}
	if( Model->Polys->Element.Num() )
	{
		// Allocate polygon pool.
		FMemMark Mark(GMem);
		FPoly** PolyList = new( GMem, Model->Polys->Element.Num() )FPoly*;

		// Add all FPolys to active list.
		for( int i=0; i<Model->Polys->Element.Num(); i++ )
			if( Model->Polys->Element(i).NumVertices )
				PolyList[i] = &Model->Polys->Element(i);

		// Now split the entire Bsp by splitting the list of all polygons.
		SplitPolyList
		(
			Model,
			INDEX_NONE,
			NODE_Root,
			Model->Polys->Element.Num(),
			PolyList,
			Opt,
			Balance,
			PortalBias,
			RebuildSimplePolys
		);
	
		// Now build the bounding boxes for all nodes.
		if( RebuildSimplePolys==0 )
		{
			// Remove unreferenced things.
			bspRefresh( Model, 1 );

			// Rebuild all bounding boxes.
			bspBuildBounds( Model );
		}

		Mark.Pop();
	}

	//!!?? Model->ClearRenderData(GRenDev);
	
#ifdef DBVDEBUGMODE
	debugf( NAME_Log, TEXT("bspBuild built %i convex polys into %i nodes"), OriginalPolys, Model->Nodes.Num() );
#endif
	unguard;
}

// END CODE COPIED FROM UnBSP, UnVisi, UnEdCsg etc.
//-----------------------------------------------------------------------------

void ADynamicBlockingVolume::execSetFaces( FFrame &Stack, RESULT_DECL )
{
	guard(ADynamicBlockingVolume::execSetFaces)

	P_GET_DYNARRAY_REF(FFace,Faces);
	P_FINISH;
	
#ifdef DEBUGFACES
	DrawFaces( Faces, COLOR_RED );
#endif

	delete Brush; 
	Brush = NULL;

	// pathological cases
	if( !Faces || Faces->Num() < 1 )
		return;

	// copy Faces into brush
	PolyFlags		= 0;
	Brush			= new( GetOuter(), NAME_None, 0) UModel( NULL, 1 );
	Brush->Polys	= new( GetOuter(), NAME_None, 0) UPolys;
	DBVCHECK( Brush->Polys->Element.GetOwner() == Brush->Polys );
	
	// copy polys from Faces list
	Brush->Polys->Element.Empty();
	Brush->Polys->Element.Add( Faces->Num() );
	
	DBVDEBUG( debugf( L"Adding %d faces", Brush->Polys->Element.Num() ) );
	for( INT ii=0; ii<Brush->Polys->Element.Num(); ii++ )
	{
		FPoly NewPoly;
		NewPoly.Init();
		DBVDEBUG( debugf( L"  Adding %d vertices", (*Faces)(ii).Points.Num() ) );
		for( INT jj=0; jj<(*Faces)(ii).Points.Num(); ++jj )
		{
			NewPoly.Vertex[ NewPoly.NumVertices ] = WorldToLocal().TransformFVector( ((*Faces)(ii).Points)(jj) );
			NewPoly.NumVertices++;
		}

		FVector SurfBase(0.0f, 0.0f, 0.0f);
		for( INT kk=0; kk<NewPoly.NumVertices; kk++ )
			SurfBase += NewPoly.Vertex[ kk ];
		SurfBase = SurfBase / NewPoly.NumVertices;
		
		NewPoly.Base = SurfBase;

		NewPoly.CalcNormal();
		
		Brush->Polys->Element(ii) = (*new FPoly( NewPoly ));
		Brush->Polys->Element(ii).iBrushPoly = INDEX_NONE;
	}
	DBVCHECK( Brush->Polys->Element.GetOwner() == Brush->Polys );

	// Update poly textures.
	for( INT ii=0; ii<Brush->Polys->Element.Num(); ii++ )
	{
		FPoly* Poly = &(Brush->Polys->Element(ii));
		Poly->iBrushPoly = INDEX_NONE;
		Poly->Material = NULL;
	}

	Brush->ModifyAllSurfs( true ); // all we need?

	// Build bounding box.
	Brush->BuildBound();

	// build BSP for the brush
	bspBuild( Brush, BSP_Good, 15, 70, 1, 0 );
	bspRefresh( Brush, 1 );
	bspBuildBounds( Brush );
		
	// re-hash
	SetCollision( true, true, true );

#ifdef WITH_KARMA
	// update karma collision
	if( bBlockKarma )
		InitActorKarma();
#endif
	
	unguard;
}

//-----------------------------------------------------------------------------
// end of Legend.cpp
//-----------------------------------------------------------------------------
