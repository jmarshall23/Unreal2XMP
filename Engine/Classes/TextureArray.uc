//=============================================================================
// TextureArray: Combines a series of smaller textures into a single larger one.  
//=============================================================================
class TextureArray extends Texture
	native;

var() bool bRebuild;	// rebuilds the texture from the SourceTextures array.

var int N, M;	// Number of elements across and down.

var() transient array<Texture> SourceTextures;

defaultproperties
{
}

