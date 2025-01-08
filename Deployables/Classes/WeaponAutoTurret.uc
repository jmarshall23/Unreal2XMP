class WeaponAutoTurret extends AssaultRifle;

//-----------------------------------------------------------------------------

var float TraceSpread;

//-----------------------------------------------------------------------------

simulated function SendEvent( string EventName );
simulated function UseAmmo();
simulated function SetupAttachments();
simulated function UpdateAttachments();
simulated function AttachToPawn(Pawn P);
simulated function PlayAnimEx(name Sequence);

//-----------------------------------------------------------------------------

defaultproperties
{
	PlayerViewOffset=(X=0.0,Y=0.0,Z=0.0)
	FireOffset=(X=0.0,Y=0.0,Z=-30.0)
	ReloadTime=0.0
	ReloadUnloadedTime=0.0
	SelectTime=0.0
	SelectUnloadedTime=0.0
	DownTime=0.0
	DownUnloadedTime=0.0
	bUpdateCrosshair=false
	bUpdateLocation=false
	bDrawWeapon=false
	bClearZ=false
	AttachmentClass=None

	AmmoName(0)=class'WeaponAutoTurretAmmo'
	AmmoName(1)=class'WeaponAutoTurretAmmo'
}

