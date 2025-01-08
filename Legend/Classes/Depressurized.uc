class Depressurized extends DamageType
	abstract;

defaultproperties
{
     DeathString="%o was depressurized by %k."
	 FemaleSuicide="%o was depressurized."
	 MaleSuicide="%o was depressurized."
	 bArmorStops=false
	 GibModifier=+100.0
	 ArmorHealthRatio=0.0	//0.5 is normal, 0 is all health and 1 is all armor
}
