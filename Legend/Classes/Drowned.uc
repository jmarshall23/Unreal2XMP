class Drowned extends DamageType
	abstract;

static function class<Effects> GetPawnDamageEffect( vector HitLocation, float Damage, vector Momentum, Pawn Victim, bool bLowDetail )
{
	return Default.PawnDamageEffect;
}

defaultproperties
{
     DeathString="%o forgot to come up for air."
	 FemaleSuicide="%o forgot to come up for air."
	 MaleSuicide="%o forgot to come up for air."

	 bArmorStops=false
	 FlashFog=(X=312.500000,Y=468.7500000,Z=468.7500000)
    bNoSpecificLocation=true
    bCausesBlood=false

	ArmorHealthRatio=0.1	//0.5 is normal, 0 is all health and 1 is all armor
}
