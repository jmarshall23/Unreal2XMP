class DamTypeTelefragged extends DamageType
	abstract;

defaultproperties
{
	DeathString="%o was telefragged by %k"
	MaleSuicide="%o was telefragged by %k"
	FemaleSuicide="%o was telefragged by %k"

    bAlwaysGibs=true
    bNoSpecificLocation=true
    bArmorStops=false

	ArmorHealthRatio=0.0	//0.5 is normal, 0 is all health and 1 is all armor
}

