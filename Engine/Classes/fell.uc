class Fell extends DamageType
	abstract;

defaultproperties
{
	DeathString="%o left a small crater"
	MaleSuicide="%o left a small crater"
	FemaleSuicide="%o left a small crater"

    GibModifier=2.0
    bNoSpecificLocation=true

	ArmorHealthRatio=0.25	//0.5 is normal, 0 is all health and 1 is all armor
}