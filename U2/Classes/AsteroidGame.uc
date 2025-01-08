//=============================================================================
// AsteroidGame.uc
//=============================================================================

class AsteroidGame extends XMPGame ;

//-----------------------------------------------------------------------------

event PreBeginPlay()
{
	Super.PreBeginPlay();
	class'U2Pawn'.default.MaxFallSpeed = 99999.0;
	class'U2Pawn'.default.MaxFallingMantleSpeed = -1.0;
	class'U2Pawn'.default.MaxRisingMantleSpeed = -1.0;
}

//-----------------------------------------------------------------------------

defaultproperties
{
}
