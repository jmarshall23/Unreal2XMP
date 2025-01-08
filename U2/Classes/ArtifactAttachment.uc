
class ArtifactAttachment extends InventoryAttachment;

#exec OBJ LOAD File=..\Particles\Artifact_Carry_FX.u

var int Team;
var ParticleGenerator PigPen;

replication
{
	reliable if( bNetDirty && bNetInitial && Role==ROLE_Authority )
		Team;
}

function InitFor(Inventory I)
{
	Super.InitFor(I);
	SetTeam( I.GetTeam() );
}

function SetTeam( int NewTeam )
{
	Team = NewTeam;
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );
}

auto state WaitForBase
{
	simulated function BeginState()
	{
		SetTimer(1.0,false);
	}
	simulated function Timer()
	{
	    if (bPendingDelete || Level.NetMode == NM_DedicatedServer)
		{
			GotoState('');
			return;
		}

		if (Base?)
		{
			if (Team == 0)
				PigPen = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Artifact_Carry_FX.ParticleSalamander1', Base.Location );
			else
				PigPen = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Artifact_Carry_FX.ParticleSalamander2', Base.Location );
			PigPen.IdleTime=0;
			PigPen.bOwnerNoSee=true;
			PigPen.SetOwner(Base);
			PigPen.bHardAttach=true;
			PigPen.SetBase(Base);
			PigPen.Trigger(Self,Instigator);

			GotoState('');
			return;
		}

		SetTimer(1.0,false);
	}
}

simulated function Destroyed()
{
	if (PigPen?)
	{
		PigPen.ParticleDestroy();
		PigPen = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	DrawType=DT_StaticMesh
	bUnlit=true
}

