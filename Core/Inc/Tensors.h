
/*=============================================================================
	Tensors.h: Various collected tensors.
	Copyright 2001 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Written by Aaron Leiby
=============================================================================*/

/*----------------------------------------------------------------------------
	LeviCivita.
		Eijk: Levi-Civita tensor where
		 0 for i=j, j=k, i=k
		+1 for even permutations
		-1 for odd	permutations
		(Parity: num swaps required to order ijk)
----------------------------------------------------------------------------*/

struct LeviCivita
{
	enum PARTIY
	{ SAME=0, EVEN=1, ODD=-1 };
	static inline const PARTIY E( const INT i, const INT j, const INT k )
	{
		if( i==j || j==k || i==k ) return SAME;	//111
		if( i<j  && j<k  && i<k  ) return EVEN;	//123
		if( i<j  && j>k  && i>k  ) return EVEN;	//231
		if( i>j  && j<k  && i>k  ) return EVEN;	//312
		if( i>j  && j>k  && i>k  ) return ODD;	//321
		if( i>j  && j<k  && i<k  ) return ODD;	//213
		if( i<j  && j>k  && i<k  ) return ODD;	//132
		appErrorf(TEXT("LeviCivita::E(%d,%d,%d): unreachable"),i,j,k);
		return SAME;
	}
};

