//=============================================================================
// PowerSuit.uc
//=============================================================================

class PowerSuit extends Armor;

var float MaxPower;					// The Maximum level of the PowerSuit energy holding capacity
var float DamageResidue;			// Holds the fractional damage not dealt to the player immediately

var sound PowerSuitHitSound;
var float PowerSuitHitVolume;

var() float ArmorDistribution;		// Percentage of damage absorbed by armor.

//-----------------------------------------------------------------------------

function GiveTo( pawn Other )
{
	local U2Pawn U2P;
	Super.GiveTo( Other );
	U2P = U2Pawn(Owner);
	if( U2P? )
	{
		U2P.PowerSuitMax   = MaxPower;
		U2P.PowerSuitPower = MaxPower;
	}
}

//-----------------------------------------------------------------------------

function float GetPower() { return U2Pawn(Owner).PowerSuitPower; }
function SetPower( float NewPower ) { U2Pawn(Owner).PowerSuitPower = NewPower; }

//-----------------------------------------------------------------------------

function DisplayDamageTaken()
{
	local Pawn P;
	P = Pawn(Owner);
	if( P? && P.IsRealPlayer() )
	{
		if( PlayerController(P.Controller)? )
			PlayerController(P.Controller).ClientSendEvent( 1 );		// case 1: SendEvent( "ShieldFlash" )
		//P.PlaySound( PowerSuitHitSound, SLOT_None, PowerSuitHitVolume );
	}
}

/*-----------------------------------------------------------------------------
Determines the amount of damage to be dealt to the PowerSuit, applies that
damage and returns all carryover damage and health damage. */

// DamageType specific ratios:
// 0=all health, 0.25=armor piercing, 0.5=normal armor calculation(default), 1=armor only(emp).

function int ArmorAbsorbDamage( int Damage, class<DamageType> DamageType, vector HitLocation, out vector Momentum )
{
	local float ArmorDamage,HealthDamage;
	local float Power;
	Power = GetPower();

	// Do full damage to health if armor is empty.
	if(Power <= 0)
		return Damage;

	//log("ArmorAbsorbDamage["$ArmorDistribution$"]"@Damage@DamageType@"ratio:"@DamageType.default.ArmorHealthRatio);

	if(!DamageType)
		DamageType=class'DamageType';

	// Distribute Damage using DamageType's ArmorHealthRatio and our own ArmorDistribution.
	if(DamageType.default.ArmorHealthRatio > 0.5)
	{
		ArmorDamage = Damage * Blend( ArmorDistribution, 1.0, ABlend( 0.5, 1.0, DamageType.default.ArmorHealthRatio ) );
	}
	else if(DamageType.default.ArmorHealthRatio < 0.5)
	{
		ArmorDamage = Damage * Blend( 0.0, ArmorDistribution, ABlend( 0.0, 0.5, DamageType.default.ArmorHealthRatio ) );
	}
	else
	{
		ArmorDamage = Damage * ArmorDistribution;
	}

	HealthDamage = Damage - ArmorDamage;

	//log("  distributed - armor:"@ArmorDamage@"health:"@HealthDamage);

	// Take damage to EnergyShield.
	if(ArmorDamage > 0)
	{
		Power -= ArmorDamage;
		SetPower(FMax(Power,0));
		DisplayDamageTaken();
	}

	// Bleed extra damage to health.
	if(Power < 0)
	{
		HealthDamage -= Power;	// power is negative, so this is really addition.
	}

	//log("    health damage:"@HealthDamage@"power:"@GetPower());

	// Collect fractional errors (especially useful when splitting constant damages of 1).
	Damage = HealthDamage;
	DamageResidue += (HealthDamage - Damage);
	if( DamageResidue > 1 )
	{
		DamageResidue -= 1;
		Damage += 1;
	}

	//log("    final damage:"@Damage@"residue:"@DamageResidue);

	return Damage;
}

//-----------------------------------------------------------------------------

function armor PrioritizeArmor( int Damage, class<DamageType> DamageType, vector HitLocation )
{
	return Self;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	ItemName="Power Suit"
	PowerSuitHitSound=Sound'U2A.Suits.PowerSuitHit'
	PowerSuitHitVolume=1.0
	ArmorDistribution=0.5
}


