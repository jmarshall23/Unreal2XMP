/*=============================================================================
	UTextureArray.h: Combines a series of smaller textures into a single larger one.  
	Copyright 2002 Legend Entertainment. All Rights Reserved.
=============================================================================*/

//
// Rebuilds the entire texture from the individual SourceTextures.
//
void Rebuild();
void PostEditChange();

//
// Calculates the 'i'th element's UV coords.
//
FRectangle GetElementUV( INT i );

//
// Given x, calculates the smallest two dimensional
// array dimensions large enough to hold x elements.
// Returns the number of wasted elements.
//
INT CalcBestDimensions( INT x, INT &OutN, INT &OutM );

INT GetUSize();
INT GetVSize();

// UObject overrides.
void Serialize( FArchive &Ar );

