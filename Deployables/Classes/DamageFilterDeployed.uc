
class DamageFilterDeployed extends DamageFilter
	abstract;

defaultproperties
{
	Filter(0)=(DamageType=class'DamageTypeEMP',DamageMultiplier=4.0,MomentumMultiplier=1.0)
	Filter(1)=(DamageType=class'DamageTypeThermalExplosiveRound',DamageMultiplier=1.0,MomentumMultiplier=1.0)
	Filter(2)=(DamageType=class'DamageTypeThermal',DamageMultiplier=0.6,MomentumMultiplier=1.0)
	Filter(3)=(DamageType=class'DamageTypeBiologicalGas',DamageMultiplier=0.2,MomentumMultiplier=1.0)
	Filter(4)=(DamageType=class'RocketDamage',DamageMultiplier=2.0,MomentumMultiplier=1.0)
	Filter(5)=(DamageType=class'DamageTypePhysical',DamageMultiplier=2.0,MomentumMultiplier=1.0)
	Filter(6)=(DamageType=class'DamageType',DamageMultiplier=1.0,MomentumMultiplier=1.0)
}
