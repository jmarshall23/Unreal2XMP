class U2WeaponAttachment extends WeaponAttachment;

// player animation specification
var() bool bHeavy;
var() bool bRapidFire;
var() bool bAltRapidFire;
var vector mHitNormal;
var actor mHitActor;
var Weapon LitWeapon;

/*tbd
simulated function GetHitInfo()
{
	local vector HitLocation, Offset;
	
	// if standalone, already have valid HitActor and HitNormal
	if ( Level.NetMode == NM_Standalone )
		return;
	Offset = 20 * Normal(Instigator.Location - mHitLocation);
	mHitActor = Trace(HitLocation,mHitNormal,mHitLocation-Offset,mHitLocation+Offset, false);
}

simulated event ThirdPersonEffects()
{
    if (Level.NetMode != NM_DedicatedServer)
    {
		if ( xPawn(Instigator) == None )
			return;
        if (FlashCount == 0)
        {
            xPawn(Instigator).StopFiring();
        }
        else if (FiringMode == 0)
        {
            xPawn(Instigator).StartFiring(bHeavy, bRapidFire);
        }
        else
        {
            xPawn(Instigator).StartFiring(bHeavy, bAltRapidFire);
        }
    }
}
*/

simulated function PostNetBeginPlay()
{
    if( U2Pawn(Instigator) != None )
        U2Pawn(Instigator).SetWeaponAttachment( Self );
}

/*tbd:
simulated function vector GetTipLocation()
{
    local Coords C;
    C = GetBoneCoords('tip');
    return C.Origin;
}

simulated function WeaponLight()
{
    if ( (FlashCount > 0) && !Level.bDropDetail && (Instigator != None) 
		&& ((Level.TimeSeconds - LastRenderTime < 0.2) || (PlayerController(Instigator.Controller) != None)) )
    {
		if ( (Instigator != None) && (Instigator.Weapon != None) )
		{
			LitWeapon = Instigator.Weapon;
			LitWeapon.bDynamicLight = true;
		}
		else
			bDynamicLight = true;
        SetTimer(0.15, false);
    }
    else 
		Timer();
}

simulated function Timer()
{
	if ( LitWeapon != None )
	{
		LitWeapon.bDynamicLight = false;
		LitWeapon = None;
	}
    bDynamicLight = false;
}
*/

defaultproperties
{
    DrawScale=0.4
    bHeavy=false
    bRapidFire=false
    bAltRapidFire=false
}
