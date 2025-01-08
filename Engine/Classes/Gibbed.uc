class Gibbed extends DamageType
	abstract;

defaultproperties
{
	DeathString="%o exploded in a shower of body parts"
	MaleSuicide="%o exploded in a shower of body parts"
	FemaleSuicide="%o exploded in a shower of body parts"

    bAlwaysGibs=true
    bNoSpecificLocation=true
    bArmorStops=false

	ArmorHealthRatio=0.0	//0.5 is normal, 0 is all health and 1 is all armor
}