
class RocketLauncherEx extends RocketLauncher;

var float CheckTargetTimer;
var() float CheckTargetInterval;
var int RocketCounter;

var bool bTargetting;

var() sound PaintSound;

var Actor PaintedTargets[4];	// also used for painted targets before rockets are fired.

var float RocketContinueNotifyTimer;

replication
{
	reliable if( Role<ROLE_Authority )
		StopTargetting;
	unreliable if( Role==ROLE_Authority )
		ClientSetPaintedTargetA,
		ClientSetPaintedTargetB,
		ClientSetPaintedTargetC,
		ClientSetPaintedTargetD;
}

function bool PreSetAimingParameters( bool bAltFire, bool bInstant, float FireSpread, class<projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	if( bAltFire )
		return true;
	else
		return Super.PreSetAimingParameters( bAltFire, bInstant, FireSpread, ProjClass, bWarnTarget, bTrySplash );
}

function PostSetAimingParameters( bool bAltFire, bool bInstant, float FireSpread, class<projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	if( bAltFire )
		Super.PreSetAimingParameters( bAltFire, bInstant, FireSpread, ProjClass, bWarnTarget, bTrySplash );
}

function AuthorityFire(){ if(AmmoIndex==0) Super.AuthorityFire(); }
simulated function EverywhereFire()
{
	if(AmmoIndex==0)
		Super.EverywhereFire();
	else
		GotoState('Targetting');
}

function ServerFire(optional bool bAltFire)
{
	Super.ServerFire(bAltFire);
	if( IsInState('Targetting') )
		bTargetting=true;
}
function StopTargetting()
{
	bTargetting=false;
}

simulated function ClientSetPaintedTargetA(Actor A){ PaintedTargets[0]=A; PlayOwnedSound(PaintSound,SLOT_Interface); }
simulated function ClientSetPaintedTargetB(Actor A){ PaintedTargets[1]=A; PlayOwnedSound(PaintSound,SLOT_Interface); }
simulated function ClientSetPaintedTargetC(Actor A){ PaintedTargets[2]=A; PlayOwnedSound(PaintSound,SLOT_Interface); }
simulated function ClientSetPaintedTargetD(Actor A){ PaintedTargets[3]=A; PlayOwnedSound(PaintSound,SLOT_Interface); }

simulated state Targetting
{
ignores Fire, AltFire;

	simulated event BeginState()
	{
		RocketCounter=4;
		PaintedTargets[0]=None;
		PaintedTargets[1]=None;
		PaintedTargets[2]=None;
		PaintedTargets[3]=None;
		Enable('Tick');
	}

	simulated event EndState()
	{
		Disable('Tick');
	}

	simulated event Tick( float DeltaSeconds )
	{
		local PlayerController PC;
		local Pawn P;

		//Global.Tick(DeltaSeconds);
		if( (Role == ROLE_Authority && bTargetting) || Instigator.PressingAltFire() )
		{
			if( (Role == ROLE_Authority) && (RocketCounter > 0) && (Level.TimeSeconds >= CheckTargetTimer) )
			{
				CheckTargets();
			}
		}
		else
		{
			StopTargetting();

			if (Role == ROLE_Authority)
				Super.AuthorityFire();
			Super.EverywhereFire();
		}

		// Keep the triangle notifies up while we continue targetting.
		RocketContinueNotifyTimer += DeltaSeconds;
		if( RocketContinueNotifyTimer >= 1.0 )
		{
			RocketContinueNotifyTimer = 0.0;
			P = Pawn(PaintedTargets[0]);
			if (P?)
			{
				PC = PlayerController(P.Controller);
				if (PC?) PC.ContinueNotifyRocketLockOnA();
			}
			P = Pawn(PaintedTargets[1]);
			if (P?)
			{
				PC = PlayerController(P.Controller);
				if (PC?) PC.ContinueNotifyRocketLockOnB();
			}
			P = Pawn(PaintedTargets[2]);
			if (P?)
			{
				PC = PlayerController(P.Controller);
				if (PC?) PC.ContinueNotifyRocketLockOnC();
			}
			P = Pawn(PaintedTargets[3]);
			if (P?)
			{
				PC = PlayerController(P.Controller);
				if (PC?) PC.ContinueNotifyRocketLockOnD();
			}
		}
	}

	function CheckTargets()
	{
		local Pawn PaintedTarget;
		PaintedTarget = Pawn(PlayerController(Instigator.Controller).PlayerSees( TraceDist, 0.0 ));
		if( PaintedTarget? && PaintedTarget.Controller? && !PaintedTarget.Controller.SameTeamAs(Instigator.Controller) )
		{
			PaintTarget( PaintedTarget );
		}
	}

	function PaintTarget( Pawn PaintedTarget )
	{
		local PlayerController PC;
		CheckTargetTimer = Level.TimeSeconds + CheckTargetInterval;
		--RocketCounter;
		PaintedTargets[RocketCounter] = PaintedTarget;
		PC = PlayerController(PaintedTarget.Controller);
		switch(RocketCounter)
		{
		case 0: ClientSetPaintedTargetA(PaintedTarget); if (PC?) PC.NotifyRocketLockOnA(); break;
		case 1: ClientSetPaintedTargetB(PaintedTarget); if (PC?) PC.NotifyRocketLockOnB(); break;
		case 2: ClientSetPaintedTargetC(PaintedTarget); if (PC?) PC.NotifyRocketLockOnC(); break;
		case 3: ClientSetPaintedTargetD(PaintedTarget); if (PC?) PC.NotifyRocketLockOnD(); break;
		}
	}

	function Projectile ProjectileFire()
	{
		local Projectile P;
		local RocketProjectileAlt R;
		P = Global.ProjectileFire();
		R = RocketProjectileAlt(P);
		if (R?)
		{
			R.PaintedTargets[0] = PaintedTargets[0];
			R.PaintedTargets[1] = PaintedTargets[1];
			R.PaintedTargets[2] = PaintedTargets[2];
			R.PaintedTargets[3] = PaintedTargets[3];
		}
		return P;
	}
}
simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot )
{
	local point RocketLocation;
	local bool bAltRocket;
	local vector dummyloc;

	RocketLocation.X = 0;
	RocketLocation.Y = 0;

	bAltRocket = (GetRocketStatus(0) || GetRocketStatus(1) || GetRocketStatus(2) || GetRocketStatus(3));

	if( IsInState('Targetting') || bAltRocket )
	{
		if( FireMode!=FM_AltFire )
		{
			FireMode = FM_AltFire;
			SendEvent("RL_AltFire");
		}
	}
	else if( LastFiredRocket!=None && !LastFiredRocket.bPendingDelete )
	{
		if (class'UtilGame'.static.ActorLookingAt(Instigator,LastFiredRocket,cos(Instigator.Controller.FOVAngle/57.2958)))
		{RocketLocation = GetProjectedLocation( LastFiredRocket.Location, Canvas );} else {RocketLocation = GetProjectedLocation( dummyloc, Canvas, true );}
		if( FireMode!=FM_Fire )
		{
			FireMode = FM_Fire;
			SendEvent("RL_Fire");
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

	RocketLocation1.X = RocketLocation.X;
	RocketLocation2.X = RocketLocation.X;
	RocketLocation3.X = RocketLocation.X;
	RocketLocation4.X = RocketLocation.X;
	RocketLocation1.Y = RocketLocation.Y;
	RocketLocation2.Y = RocketLocation.Y;
	RocketLocation3.Y = RocketLocation.Y;
	RocketLocation4.Y = RocketLocation.Y;

	if( IsInState('Targetting') )
	{
		if( PaintedTargets[0]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[0],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation1 = GetProjectedLocation( PaintedTargets[0].Location, Canvas );} else {RocketLocation1 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( PaintedTargets[1]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[1],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation2 = GetProjectedLocation( PaintedTargets[1].Location, Canvas );}else {RocketLocation2 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( PaintedTargets[2]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[2],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation3 = GetProjectedLocation( PaintedTargets[2].Location, Canvas );}else {RocketLocation3 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( PaintedTargets[3]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[3],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation4 = GetProjectedLocation( PaintedTargets[3].Location, Canvas );}else {RocketLocation4 = GetProjectedLocation( dummyloc, Canvas, true );}
	}
	else if (bAltRocket)
	{
		if( (GetRocketStatus(0))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[0],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{RocketLocation1 = GetProjectedLocation( AltRockets[0].Location, Canvas );} 
		else {RocketLocation1 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( (GetRocketStatus(1))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[1],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{RocketLocation2 = GetProjectedLocation( AltRockets[1].Location, Canvas );} 
		else {RocketLocation2 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( (GetRocketStatus(2))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[2],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{RocketLocation3 = GetProjectedLocation( AltRockets[2].Location, Canvas );} 
		else {RocketLocation3 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( (GetRocketStatus(3))&&(class'UtilGame'.static.ActorLookingAt(Instigator,AltRockets[3],cos (Instigator.Controller.FOVAngle/57.2958))) ) 
		{RocketLocation4 = GetProjectedLocation( AltRockets[3].Location, Canvas );} 
		else {RocketLocation4 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( PaintedTargets[0]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[0],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation1 = GetProjectedLocation( PaintedTargets[0].Location, Canvas );} else {RocketLocation1 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( PaintedTargets[1]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[1],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation2 = GetProjectedLocation( PaintedTargets[1].Location, Canvas );}else {RocketLocation2 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( PaintedTargets[2]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[2],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation3 = GetProjectedLocation( PaintedTargets[2].Location, Canvas );}else {RocketLocation3 = GetProjectedLocation( dummyloc, Canvas, true );}
		if( PaintedTargets[3]!=None )
			if (class'UtilGame'.static.ActorLookingAt(Instigator,PaintedTargets[3],cos (Instigator.Controller.FOVAngle/57.2958))) 
			{RocketLocation4 = GetProjectedLocation( PaintedTargets[3].Location, Canvas );}else {RocketLocation4 = GetProjectedLocation( dummyloc, Canvas, true );}
	}

	RocketLocation1.X-=8;RocketLocation1.Y-=4;
	RocketLocation2.X+=8;RocketLocation2.Y-=4;
	RocketLocation3.X+=8;RocketLocation3.Y+=4;
	RocketLocation4.X-=8;RocketLocation4.Y+=4;
}

defaultproperties
{
	CheckTargetInterval=0.15
	PaintSound=sound'WeaponsA.RocketLauncher.RL_Paint'
	TraceDist=28800 // 600 yards
}
