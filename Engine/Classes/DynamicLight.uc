//=============================================================================
// The dynamic light class.
// Used to create lights that need to move or should be treated as moveable
//=============================================================================
class DynamicLight extends Light
	placeable;

defaultproperties
{
     bStatic=False
     bDynamicLight=True
	 bNoDelete=False
}
