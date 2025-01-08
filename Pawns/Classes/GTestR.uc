//===============================================================================
//  GTestR
//===============================================================================

class GTestR extends GunnerRed;

event PostBeginPlay()
{
	Super.PostBeginPlay();
	LoopAnim( 'RunF', 1.0 );
}

defaultproperties
{
}

