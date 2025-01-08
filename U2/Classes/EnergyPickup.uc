class EnergyPickup extends PowerUpPickup;

//-----------------------------------------------------------------------------

function AnnouncePickup( Pawn Receiver )
{
	Super.AnnouncePickup( Receiver );
	if( Receiver? )
		XMPGame(Level.Game).NotifyRepairTeammateWithPack( Controller(Owner), Receiver.Controller );
}

//-----------------------------------------------------------------------------

auto state Pickup
{
	function bool ValidTouch( actor Other )
	{
		local U2Pawn U2P;
		local EnergyPowerUp	EP;

		U2P = U2Pawn(Other);
		if( U2P != None )
		{
			if( U2P.PowerSuitPower < U2P.PowerSuitMax )
			{
				EP = EnergyPowerUp(class'UtilGame'.static.GetInventoryItem( U2P, class'EnergyPowerUp' ));
				if( EP != None )
				{
					// Can only pickup if the PowerSuit is not at max and if any current EnergyPowerups
					// will not bring PowerSuit energy up to max.
					//DM( "### (" $ U2P.PowerSuitPower $ " + " $ float(EP.EnergyUnits) / 100.0 * U2P.PowerSuitMax $ ") < " $ U2P.PowerSuitMax $ " = " $ (U2P.PowerSuitPower + float(EP.EnergyUnits) / 100.0 * U2P.PowerSuitMax) < U2P.PowerSuitMax );
					if( (U2P.PowerSuitPower + float(EP.EnergyUnits) / 100.0 * U2P.PowerSuitMax) < U2P.PowerSuitMax )
					{
						if( EnergyUnits > 0 )
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
	Skins(0)=Shader'XMPWorldItemsT.Items.Metl_Pickup_TDSD_001_Energy'
	ParticleEffectA=ParticleSalamander'Pickup_FX.ParticleSalamander7'
	ParticleEffectB=ParticleSalamander'Pickup_FX.ParticleSalamander9'
	EnergyUnits=25
	TransferRate=25.0
	RespawnTime=0.0
	Description="Energy Pickup"
	PickupMessage="You got an Energy Pickup."
	InventoryType=class'EnergyPowerUp'
	PickupSound=Sound'U2A.PowerUps.EnergyPowerUp'
}