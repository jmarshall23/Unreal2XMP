class AnimNotify_FireWeapon extends AnimNotify_Scripted;

event Notify( Actor Owner )
{
/*!!MERGE
	// fake fire - play weapon effect, but no real shot
	Pawn(Owner).bIgnorePlayFiring = true;
	WeaponAttachment(Pawn(Owner).Weapon.ThirdPersonActor).ThirdPersonEffects();
	if ( Pawn(Owner).Weapon.FireSound != None )
		Pawn(Owner).Weapon.PlaySound(Pawn(Owner).Weapon.FireSound, SLOT_None, 1.0);
*/
}
