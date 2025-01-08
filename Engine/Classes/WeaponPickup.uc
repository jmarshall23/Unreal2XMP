class WeaponPickup extends Pickup
	abstract;

var   bool	  bWeaponStay;
var() bool	  bThrown; // true if deliberatly thrown, not dropped from kill

function PostBeginPlay()
{
	Super.PostBeginPlay();
	SetWeaponStay();
	MaxDesireability = 1.2 * class<Weapon>(InventoryType).Default.AIRating;
}

function SetWeaponStay()
{
	bWeaponStay = ( bWeaponStay && Level.Game.bWeaponStay );
}

function StartSleeping()
{
    if (bDropped)
        Destroy();
    else if (!bWeaponStay)
	    GotoState('Sleeping');
}

function bool AllowRepeatPickup()
{
    return (!bWeaponStay || (bDropped && !bThrown));
}

/* DetourWeight()
value of this path to take a quick detour (usually 0, used when on route to distant objective, but want to grab inventory for example)
*/
function float DetourWeight(Pawn Other,float PathWeight)
{
	local Weapon AlreadyHas;

	AlreadyHas = Weapon(Other.FindInventoryType(InventoryType)); 
	if ( (AlreadyHas != None)
		&& bWeaponStay )
		return 0;
	if ( AIController(Other.Controller).PriorityObjective()
		&& ((Other.Weapon.AIRating > 0.5) || (PathWeight > 400)) )
		return 0;
	if ( class<Weapon>(InventoryType).Default.AIRating > Other.Weapon.AIRating )
		return class<Weapon>(InventoryType).Default.AIRating/PathWeight;
	return class<Weapon>(InventoryType).Default.AIRating/PathWeight;
}

// tell the bot how much it wants this weapon pickup
// called when the bot is trying to decide which inventory pickup to go after next
function float BotDesireability(Pawn Bot)
{
	local Weapon AlreadyHas;
	local float desire;

	// bots adjust their desire for their favorite weapons
	desire = MaxDesireability + Bot.Controller.AdjustDesireFor(self);

	// see if bot already has a weapon of this type
	AlreadyHas = Weapon(Bot.FindInventoryType(InventoryType)); 
	if ( AlreadyHas != None )
	{
		if ( Bot.Controller.bHuntPlayer )
			return 0;

		// can't pick it up if weapon stay is on
		if ( !AllowRepeatPickup() )
			return 0;
		if ( (RespawnTime < 10) 
			&& ( bHidden || (AlreadyHas.AmmoType == None) 
				|| (AlreadyHas.AmmoType.AmmoAmount < AlreadyHas.AmmoType.MaxAmmo)) )
			return 0;


		// bot wants this weapon for the ammo it holds
		if ( AlreadyHas.HasAmmo() )
			return FMax( 0.25 * desire, 
					AlreadyHas.AmmoType.PickupClass.Default.MaxDesireability
					 * FMin(1, 0.15 * AlreadyHas.AmmoType.MaxAmmo/AlreadyHas.AmmoType.AmmoAmount) ); 
		else
			return 0.05;
	}
	if ( Bot.Controller.bHuntPlayer && (MaxDesireability * 0.833 < Bot.Weapon.AIRating - 0.1) )
		return 0;
	
	// incentivize bot to get this weapon if it doesn't have a good weapon already
	if ( (Bot.Weapon == None) || (Bot.Weapon.AIRating < 0.5) )
		return 2*desire;

	return desire;
}

function float GetRespawnTime()
{
	if ( (Level.NetMode != NM_Standalone) || (Level.Game.GameDifficulty > 3) )
		return ReSpawnTime;
	return RespawnTime * (0.33 + 0.22 * Level.Game.GameDifficulty); 
}

defaultproperties
{
    bAmbientGlow=true
	CollisionRadius=+36.0
	CollisionHeight=+30.0
    Physics=PHYS_Rotating 
    RotationRate=(Yaw=32768)
     PickupMessage="You got a weapon"
     RespawnTime=30.000000
     Texture=Texture'Engine.S_Weapon'
     MaxDesireability=0.5000
    AmbientGlow=255
    bPredictRespawns=true
    bWeaponStay=true
}