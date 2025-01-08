
class DamageFilterVehicles extends DamageFilter
	abstract;

defaultproperties
{
	Filter(0)=(DamageType=class'DamageTypeEMP',DamageMultiplier=2.4,MomentumMultiplier=1.0)
	Filter(1)=(DamageType=class'DamageTypeThermalExplosiveRound',DamageMultiplier=0.3,MomentumMultiplier=1.0)
	Filter(2)=(DamageType=class'DamageTypeThermal',DamageMultiplier=0.2,MomentumMultiplier=1.0)
	Filter(3)=(DamageType=class'DamageTypeBiologicalGas',DamageMultiplier=0.06,MomentumMultiplier=1.0)
	Filter(4)=(DamageType=class'RocketDamage',DamageMultiplier=1.2,MomentumMultiplier=1.0)
	Filter(5)=(DamageType=class'DamageTypePhysical',DamageMultiplier=1.2,MomentumMultiplier=1.0)
	Filter(6)=(DamageType=class'DamageType',DamageMultiplier=0.3,MomentumMultiplier=1.0)
}
