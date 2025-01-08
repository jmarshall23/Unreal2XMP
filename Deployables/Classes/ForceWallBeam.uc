//=============================================================================
// ForceWallBeam.uc
// $Author: Sbrown $
// $Date: 3/04/02 4:14a $
// $Revision: 2 $
//=============================================================================

class ForceWallBeam extends PulseLineGenerator;

defaultproperties
{	
	NumBeams=(A=1,B=1)
	BeamSegLength=(A=4000.000000,B=4000.000000)
	BeamTexture=Texture'ParticleSystems.Pulse.GlowBeam'
	BeamTextureWidth=10.000000
	SpriteJointSize=(A=15.000000,B=15.000000)
	SpriteJointTexture=Texture'ParticleSystems.Glass.GlassFog'
}
