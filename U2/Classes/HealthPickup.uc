class HealthPickup extends PowerUpPickup;

//-----------------------------------------------------------------------------

function AnnouncePickup( Pawn Receiver )
{
	Super.AnnouncePickup( Receiver );
	if( Receiver? )
		XMPGame(Level.Game).NotifyHealTeammateWithPack( Controller(Owner), Receiver.Controller );
}

//-----------------------------------------------------------------------------

auto state Pickup
{
	function bool ValidTouch( actor Other )
	{
		local Pawn P;
		local HealthPowerUp HP;

		P = Pawn(Other);
		if( P != None )
		{
			if( P.Health < P.default.Health )
			{
				HP = HealthPowerUp(class'UtilGame'.static.GetInventoryItem( P, class'HealthPowerUp' ));
				if( HP != None )
				{
					//DM( "### (" $ P.Health $ " + " $ HP.HealthUnits $ ") < " $ P.default.Health $ " = " $ (P.Health + HP.HealthUnits) < P.default.Health );
					// Can only pickup if owner is not at full health and if any current HealthPowerups
					// will not bring Health up to full.
					if( (P.Health + HP.HealthUnits) < P.default.Health )
					{
						if( HealthUnits > 0 )
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
	Skins(0)=Shader'XMPWorldItemsT.Items.Metl_Pickup_TDSD_001_Health'
	ParticleEffectA=ParticleSalamander'Pickup_FX.ParticleSalamander3'
	ParticleEffectB=ParticleSalamander'Pickup_FX.ParticleSalamander5'
	HealthUnits=25
	TransferRate=25.0
	RespawnTime=0.0
	Description="Health Pickup"
	PickupMessage="You got a Health Pickup."
	InventoryType=class'HealthPowerUp'
	PickupSound=Sound'U2A.PowerUps.HealthPowerUp'
}