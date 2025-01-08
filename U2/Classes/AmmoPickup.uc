class AmmoPickup extends PowerUpPickup;

//-----------------------------------------------------------------------------

function AnnouncePickup( Pawn Receiver )
{
	Super.AnnouncePickup( Receiver );
	if( Receiver? )
		XMPGame(Level.Game).NotifyResupplyTeammateWithPack( Controller(Owner), Receiver.Controller );
}

//-----------------------------------------------------------------------------

function bool IsMissingDefaultInventory( Pawn PlayerPawn )
{
	local array< class<Weapon> > WeaponClasses;
	local class<Weapon> W;
	local int i;

	// Spawn default weapon.
	WeaponClasses = PlayerPawn.MyDefaultWeapons();
	W = Level.Game.BaseMutator.GetDefaultWeapon();
	if( W!=None )
		WeaponClasses[WeaponClasses.length] = W;
	for( i=0; i<WeaponClasses.length; i++ )
	{
		if( WeaponClasses[i].static.IsValidDefaultInventory( PlayerPawn ) &&
			PlayerPawn.FindInventoryType(WeaponClasses[i])==None )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

auto state Pickup
{
	function bool ValidTouch( actor Other )
	{
		local Pawn P;
		local AmmoPowerUp AP;
		local float MinAmmoPct;
		local Inventory Inv;
		local Weapon W;

		P = Pawn(Other);
		if( P != None && P.PlayerReplicationInfo? )
		{
			if( P.PlayerReplicationInfo.AmmoEquipedPct < 1.0 || IsMissingDefaultInventory(P) )
			{
				AP = AmmoPowerUp(class'UtilGame'.static.GetInventoryItem( P, class'AmmoPowerUp' ));
				if( AP != None )
				{
					// Can only pickup if any ammo type is not at max and if any current AmmoPowerups
					// will not bring the most depleted ammo type up to max.
					MinAmmoPct = 100.0;
					for( Inv=P.Inventory; Inv!=None; Inv=Inv.Inventory )
					{
						W = Weapon(Inv);
						if( W? && W.AmmoType.AmmoUsed > 0 )
							MinAmmoPct = FMin( MinAmmoPct, W.ResupplyStatus() * 100.0 );
					}

					//DM( "### (" $ MinAmmoPct $ " + " $ AP.AmmoUnits $ ") < " $ 100.0 $ " = " $ (MinAmmoPct + AP.AmmoUnits) < 100.0 );
					if( MinAmmoPct + AP.AmmoUnits < 100.0 )
					{
						if( AmmoUnits > 0 )
							return Super.ValidTouch( Other );
						else
							return false;
					}
					else
						return false;
				}
				else
					return Super.ValidTouch( Other );
			}
			else
				return false;
		}
		return Super.ValidTouch( Other );
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	Skins(0)=Shader'XMPWorldItemsT.Items.Metl_Pickup_TDSD_001_Supply'
	ParticleEffectA=ParticleSalamander'Pickup_FX.ParticleSalamander8'
	ParticleEffectB=ParticleSalamander'Pickup_FX.ParticleSalamander10'
	AmmoUnits=25
	TransferRate=25.0
	RespawnTime=0.0
	Description="Ammo Pickup"
	PickupMessage="You got an Ammo Pickup."
	InventoryType=class'AmmoPowerUp'
	PickupSound=Sound'U2A.PowerUps.AmmoPowerUp'
}