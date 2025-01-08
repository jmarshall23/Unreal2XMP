//===============================================================================
//  GTestB
//===============================================================================

class GTestB extends GunnerBlue;

event PostBeginPlay()
{
	Super.PostBeginPlay();
	LoopAnim( 'RunF', 1.0 );
}

defaultproperties
{
}

