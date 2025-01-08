
class RocketLauncher extends Weapon;

var Projectile LastFiredRocket;
var Projectile AltRockets[4];
var Point	RocketLocation1,
			RocketLocation2,
			RocketLocation3,
			RocketLocation4;

var enum EFireMode { FM_None, FM_Fire, FM_AltFire } FireMode;

var float AltFireMinIneffectiveRange;		// distance within which altfire will likely pass through/miss target
var float AltFireReallyIneffectiveRange;	// generally AltFireReallyIneffectiveRange/X preset for speed
var float PreferAltFireRange;				// distance at which skilled NPCs will prefer altfire

simulated function bool GetRocketStatus( int i )
{
	return (AltRockets[i] != None) && (!AltRockets[i].bPendingDelete);
}

simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot )
{
	local point RocketLocation;
	local bool bAltRocket;
	local vector dummyloc;

	RocketLocation.X = 0;
	RocketLocation.Y = 0;

	bAltRocket = (GetRocketStatus(0) || GetRocketStatus(1) || GetRocketStatus(2) || GetRocketStatus(3));

	if( LastFiredRocket!=None && !LastFiredRocket.bPendingDelete )
	{
		if (class'UtilGame'.static.ActorLookingAt(Instigator,LastFiredRocket,cos (Instigator.Controller.FOVAngle/57.2958)))
		{RocketLocation = GetProjectedLocation( LastFiredRocket.Location, Canvas );} else {RocketLocation = GetProjectedLocation( dummyloc, Canvas, true );}

		if( FireMode!=FM_Fire )
		{
			FireMode = FM_Fire;
			SendEvent("RL_Fire");
		}
	}
	else if( bAltRocket )
	{
		if( FireMode!=FM_AltFire )
		{
			FireMode = FM_AltFire;
			SendEvent("RL_AltFire");
		}
	}
	else
	{
		if( FireMode!=FM_None )
		{
			FireMode = FM_None;
			SendEvent("RL_Default");
		}
	}

	RocketLocation1.X = RocketLocation.X - 8; RocketLocation1.Y = RocketLocation.Y - 4;
	RocketLocation2.X = RocketLocation.X + 8; RocketLocation2.Y = RocketLocation.Y - 4;
	RocketLocation3.X = RocketLocation.X + 8; RocketLocation3.Y = RocketLocation.Y + 4;
	RocketLocation4.X = RocketLocation.X - 8; RocketLocation4.Y = RocketLocation.Y + 4;

	if( bAltRocket )
	{
		if( (GetRocketStatus(0))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[0],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{
			RocketLocation1 = GetProjectedLocation( AltRockets[0].Location, Canvas );} else {RocketLocation1 = GetProjectedLocation( dummyloc, Canvas, true );
		}
		if( (GetRocketStatus(1))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[1],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{
			RocketLocation2 = GetProjectedLocation( AltRockets[1].Location, Canvas );} else {RocketLocation2 = GetProjectedLocation( dummyloc, Canvas, true );
		}
		if( (GetRocketStatus(2))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[2],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{
			RocketLocation3 = GetProjectedLocation( AltRockets[2].Location, Canvas );} else {RocketLocation3 = GetProjectedLocation( dummyloc, Canvas, true );
		}
		if( (GetRocketStatus(3))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[3],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{
			RocketLocation4 = GetProjectedLocation( AltRockets[3].Location, Canvas );} else {RocketLocation4 = GetProjectedLocation( dummyloc, Canvas, true );
		}
	}
}

simulated function Point GetProjectedLocation( vector Loc, Canvas Canvas, optional bool returnbad)
{
	local vector RocLoc;
	local point Pt;

	Pt.X=Canvas.SizeX+8;
	Pt.Y=Canvas.SizeY+8;
	if (returnbad) 
		return Pt;

	RocLoc = Canvas.Project( Loc );
	if
	(	RocLoc.X >= 0  && RocLoc.X <= Canvas.SizeX
	&&	RocLoc.Y >= 0  && RocLoc.Y <= Canvas.SizeY
	)
	{
		Pt.X = RocLoc.X - Canvas.SizeX/2;
		Pt.Y = RocLoc.Y - Canvas.SizeY/2;
	}

	return Pt;
}

interface function Point GetRocketLocation01(){ return RocketLocation1; }
interface function Point GetRocketLocation02(){ return RocketLocation2; }
interface function Point GetRocketLocation03(){ return RocketLocation3; }
interface function Point GetRocketLocation04(){ return RocketLocation4; }

function float GetProjSpeed( bool bAlt )
{
	if( bAlt )
		return 1200;
	else
		return Super.GetProjSpeed( bAlt ); 
}


defaultproperties
{
	// text
    ItemName="Rocket Launcher"
	IconIndex=5
	ItemID="RL"
	Crosshair="Crosshair_RL"

	// weapon data
	AutoSwitchPriority=14
	InventoryGroup=1
	GroupOffset=1
	PickupAmmoCount=25

	// assets
	Mesh=SkeletalMesh'WeaponsK.RL_FP'
	AttachmentClass=class'RocketAttachment'
	//!! FireSkins(0)=material'WeaponFXT.RL_FP_Skin2FX'
	//!! FireSkinsTP(0)=material'WeaponFXT.RL_TP_Skin2FX'
	PlayerViewOffset=(X=15,Y=0,Z=-30)
	FireOffset=(X=10.0,Y=19.0,Z=4)
	SelectSound=sound'WeaponsA.RocketLauncher.RL_Select'
	ReloadSound=sound'WeaponsA.RocketLauncher.RL_Reload'
	ReloadUnloadedSound=sound'WeaponsA.RocketLauncher.RL_ReloadUnloaded'

	AmmoName(0)=class'RocketLauncherAmmo'
	AmmoName(1)=class'RocketLauncherAmmoAlt'

	// additional timing
	ReloadTime=2.0
	ReloadUnloadedTime=2.0
}

