/*============================================================================
	MasterServerClient.cpp - master server client

	Revision history:
		* Created by Jack Porter
============================================================================*/

#include "UnIpDrv.h"

/*-----------------------------------------------------------------------------
	FServerQueryLink
-----------------------------------------------------------------------------*/

#define PINGTIMEOUT 1.5f

struct FOutstandingPingInfo
{
	INT ServerID;
	BYTE PingCause;
	BYTE PingType;
	FIpAddr QueryAddr;
	DOUBLE PingSendTime;
	FServerResponseLine CurrentState;
	UBOOL GotReply;
	INT PingCount;
};

class FServerQueryLink : public FUdpLink
{
	AServerQueryClient* Actor;
	TArray<FOutstandingPingInfo> OutstandingPings;
public:
	FServerQueryLink( AServerQueryClient* InActor, INT InPort )
	:	FUdpLink()
	,	Actor(InActor)
	{
		guard(FServerQueryLink::FServerQueryLink);
		BindPort(InPort);
		unguard;
	}
	void Poll()
	{
		// check for ping timeouts.
		for( INT i=0;i<OutstandingPings.Num();i++ )
		{
			if( appSeconds() > OutstandingPings(i).PingSendTime + PINGTIMEOUT )
			{
				if( !OutstandingPings(i).GotReply && OutstandingPings(i).QueryAddr.Addr != INADDR_BROADCAST )
				{
					if( OutstandingPings(i).PingCause==PC_AutoPing && OutstandingPings(i).PingCount<1 )
					{
						// re-ping
						OutstandingPings(i).PingCount++;
						OutstandingPings(i).PingSendTime = appSeconds();
						BYTE Command = OutstandingPings(i).PingType;
                   		FArchiveUdpWriter ArSend( this, OutstandingPings(i).QueryAddr );
						ArSend << Command;
						ArSend.Flush();
					}
					else
					{
						Actor->delegateOnPingTimeout( OutstandingPings(i).ServerID, OutstandingPings(i).PingCause );
						OutstandingPings.Remove(i);
						i--;
					}
				}
				else
				{
					OutstandingPings.Remove(i);
					i--;
				}
			}
		}

		FUdpLink::Poll();
	}
	void PingServer( INT ServerID, BYTE PingCause, FIpAddr Addr, BYTE Command, FServerResponseLine& CurrentState )
	{
		guard(FServerQueryLink::PingServer);

		// find existing ping.
		INT i;
		for( i=0;i<OutstandingPings.Num();i++ )
			if( Addr == OutstandingPings(i).QueryAddr )
				break;
		if( i == OutstandingPings.Num() )
			i = OutstandingPings.AddZeroed();

		OutstandingPings(i).PingCount = 0;
		OutstandingPings(i).PingSendTime = appSeconds();
		OutstandingPings(i).QueryAddr = Addr;
		OutstandingPings(i).CurrentState = CurrentState;
		OutstandingPings(i).ServerID = ServerID;
		OutstandingPings(i).PingCause = PingCause;
		OutstandingPings(i).PingType  = Command;

//		GWarn->Logf(TEXT("Sending ping request to IP %s (%d)"), *Addr.GetString(1), Command);
		FArchiveUdpWriter ArSend( this, Addr );
		ArSend << Command;
		ArSend.Flush();

		// HACK: also ping the server when we request rules.
		if( Command == 3 )
		{
			Command = 0;
			ArSend << Command;
			ArSend.Flush();
		}
		unguard;
	}
	void CancelPings()
	{
		guard(FServerQueryLink::CancelPings);
		OutstandingPings.Empty();
		unguard;
	}
	void OnReceivedData( FIpAddr SrcAddr, BYTE* Data, INT Count )
	{
		guard(FServerQueryLink::OnReceivedData);
		FArchiveUdpReader ArRecv( Data, Count );

		// find server
		INT i;
		INT bc = -1;
		for( i=0;i<OutstandingPings.Num();i++ )
		{
			if( SrcAddr == OutstandingPings(i).QueryAddr )
				break;
			if( OutstandingPings(i).QueryAddr.Addr == INADDR_BROADCAST )
				bc = i;
		}

		FServerResponseLine* CurrentState=NULL;
		INT Ping;
		if( i < OutstandingPings.Num() )
		{
			Ping = appRound( (appSeconds() - OutstandingPings(i).PingSendTime) * 1000 );
			CurrentState = &OutstandingPings(i).CurrentState;
			OutstandingPings(i).GotReply = 1;
		}
		else
		if( bc >= 0 )
		{
			Ping = appRound( (appSeconds() - OutstandingPings(bc).PingSendTime) * 1000 );
		}
		else
			Ping = 9999;

		BYTE Command;
		ArRecv << Command;
		if( ArRecv.IsError() )
		{
			GWarn->Logf(TEXT("Received bad UDP ping data from %s (%d bytes)"), *SrcAddr.GetString(1), Count);
		}
		else
		{
			switch( Command )
			{
			case QI_Ping:
				{
					FServerResponseLine NewState;
					ArRecv << NewState;
						NewState.Ping = Ping;
					NewState.IP = SrcAddr.GetString(0);
					NewState.QueryPort = SrcAddr.Port;

					if( CurrentState )
					{
						// HACK don't update the data if you clicked on the server while we're pinging!
						if( OutstandingPings(i).PingType!=QI_Ping )
						{
							if( OutstandingPings.Num() > 5 )
							{
								NewState.CurrentPlayers = CurrentState->CurrentPlayers;
								NewState.MaxPlayers = CurrentState->MaxPlayers;
								NewState.MapName = CurrentState->MapName;
	                            NewState.Ping = CurrentState->Ping;
							}
							else
							if( CurrentState->Ping < 9999 )
	                            NewState.Ping = CurrentState->Ping;
						}

						// HACK keep master server's map capitalization
						if( NewState.MapName.Caps() == CurrentState->MapName.Caps() )
							NewState.MapName = CurrentState->MapName;

						NewState.PlayerInfo = CurrentState->PlayerInfo;
						NewState.ServerInfo = CurrentState->ServerInfo;
						Actor->delegateOnReceivedPingInfo(OutstandingPings(i).ServerID, OutstandingPings(i).PingCause, NewState );
					}
					else
						Actor->delegateOnReceivedPingInfo( -1, PC_Unknown, NewState );
				}
				break;
			case QI_Rules:
				if( CurrentState )
				{
					TArray<FKeyValuePair> NewInfo;
					while( !ArRecv.AtEnd() && !ArRecv.IsError() )
					{
						INT i = NewInfo.AddZeroed();
						ArRecv << NewInfo(i);
					}

					UBOOL HadIP = 0;
					for(INT ci=0;ci<CurrentState->ServerInfo.Num();ci++ )
					{
						for( INT ni=0;ni<NewInfo.Num();ni++ )
						{
							if( CurrentState->ServerInfo(ci).Key==NewInfo(ni).Key && CurrentState->ServerInfo(ci).Value==NewInfo(ni).Value )
							{
								NewInfo.Remove(ni);
								ni--;
							}
						}

						if( CurrentState->ServerInfo(ci).Key == TEXT("IP") )
							HadIP = 1;
					}

					for( INT ni=0;ni<NewInfo.Num();ni++ )
					{
						INT ci=CurrentState->ServerInfo.AddZeroed();
						CurrentState->ServerInfo(ci).Key = NewInfo(ni).Key;
						CurrentState->ServerInfo(ci).Value = NewInfo(ni).Value;
					}

					// Add IP:Port to rules
					if( !HadIP )
					{
						INT ci=CurrentState->ServerInfo.AddZeroed();
						CurrentState->ServerInfo(ci).Key = TEXT("IP");
						CurrentState->ServerInfo(ci).Value = FString::Printf(TEXT("%s:%d"), *CurrentState->IP, CurrentState->Port);
					}

					Actor->delegateOnReceivedPingInfo(OutstandingPings(i).ServerID, OutstandingPings(i).PingCause, *CurrentState );
				}
				else
					GWarn->Logf(TEXT("Couldn't find outstanding request for rules from %s"), *SrcAddr.GetString(1) );
				break;
			case QI_Players:
				if( CurrentState )
				{
					TArray<FPlayerResponseLine> NewInfo;
					while( !ArRecv.AtEnd() && !ArRecv.IsError() )
					{
						INT i = NewInfo.AddZeroed();
						ArRecv << NewInfo(i);
					}

					for( INT np=0;np<NewInfo.Num();np++ )
					{
						for(INT cp=0;cp<CurrentState->PlayerInfo.Num();cp++ )
						{
							if( CurrentState->PlayerInfo(cp).PlayerNum==NewInfo(np).PlayerNum )
							{
								CurrentState->PlayerInfo.Remove(cp);
								cp--;
							}
						}
					}

					for( INT np=0;np<NewInfo.Num();np++ )
					{
						INT cp=CurrentState->PlayerInfo.AddZeroed();
						CurrentState->PlayerInfo(cp) = NewInfo(np);
					}

					Actor->delegateOnReceivedPingInfo(OutstandingPings(i).ServerID, OutstandingPings(i).PingCause, *CurrentState );
				}
				else
					GWarn->Logf(TEXT("Couldn't find outstanding request for players from %s"), *SrcAddr.GetString(1) );
				break;
			}
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	AServerQueryClient
-----------------------------------------------------------------------------*/

void AServerQueryClient::Init()
{
	if( !LinkPtr )
    {
        if ((!bLANQuery) || (LANPort >= 0))
		    LinkPtr = (INT)(new FServerQueryLink(this, bLANQuery ? LANPort : 0));
    }
}

void AServerQueryClient::execPingServer( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(ServerID);
	P_GET_BYTE(PingCause)
	P_GET_STR(IP);
	P_GET_INT(Port)
	P_GET_BYTE(Command);
	P_GET_STRUCT(FServerResponseLine,CurrentState);
	P_FINISH;

	Init();

	FIpAddr Addr( *IP, Port );
	if( IP == TEXT("BROADCAST") )
		Addr.Addr = INADDR_BROADCAST;

	((FServerQueryLink*)LinkPtr)->PingServer( ServerID, PingCause, Addr, Command, CurrentState );
}

void AServerQueryClient::execCancelPings( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	Init();
	((FServerQueryLink*)LinkPtr)->CancelPings();
}

UBOOL AServerQueryClient::Poll( INT WaitTime )
{
	guard(AServerQueryClient::Poll);

	if( LinkPtr )
		((FServerQueryLink*)LinkPtr)->Poll();
	return 0;
	unguard;
}

void AServerQueryClient::Destroy()
{
	if( LinkPtr )
		delete ((FServerQueryLink*)LinkPtr);
	LinkPtr = 0;
	Super::Destroy();
}

void AServerQueryClient::PostScriptDestroyed()
{
	Super::PostScriptDestroyed();
	if( LinkPtr )
		delete ((FServerQueryLink*)LinkPtr);
	LinkPtr = 0;
}

IMPLEMENT_CLASS(AServerQueryClient);

/*-----------------------------------------------------------------------------
	FMasterServerClientLink
-----------------------------------------------------------------------------*/

#define DOWNLOADLISTFILE TEXT("AutoPatch.txt")

enum EMasterServerClientState
{
	MSCS_WaitingChallenge		= 0,
	MSCS_WaitingApproval		= 1,
	MSCS_SentQuery				= 2,
	MSCS_WaitQueryData			= 3,
};

class FMasterServerClientLink : public FTcpLink
{
	EMasterServerClientState ClientState;
	AMasterServerClient* Actor;
	INT ResultCount, ReceiveCount;
	BYTE ResultsCompressed;
	BYTE QueryType;

	FString						MasterServerName;
	INT							MasterServerPort;
public:
	FMasterServerClientLink( AMasterServerClient* InActor )
	:	FTcpLink()
	,	ClientState(MSCS_WaitingChallenge)
	,	Actor(InActor)
	,	ResultCount(-1)
	,	ReceiveCount(0)
	{
		SetLinkMode( TCPLINK_FArchive );
	}

	void StartQuery( BYTE InQueryType )
	{
		QueryType = InQueryType;
		Actor->eventGetMasterServer( MasterServerName, MasterServerPort );
		Resolve( *MasterServerName );
	}

	void OnResolved( FIpAddr a )
	{
		a.Port = MasterServerPort;
		GWarn->Logf(TEXT("Resolved %s -> %s"), *MasterServerName, *a.GetString(0) );
		Connect( a );
     }

	void OnConnectionSucceeded()
	{
		GWarn->Logf(TEXT("Connection established."));
	}

	void OnConnectionFailed()
	{
		GWarn->Logf(TEXT("Connection failed!!"));
		Actor->CurrentMasterServer++;
		Actor->SaveConfig();
		Actor->delegateOnQueryFinished(RI_ConnectionFailed,0);
	}

	void OnResolveFailed()
	{
		GWarn->Logf(TEXT("Failed to resolve '%s'"), *MasterServerName);
		Actor->CurrentMasterServer++;
		Actor->SaveConfig();
		Actor->delegateOnQueryFinished(RI_ConnectionFailed,0);
	}

	void OnClosed()
	{
		GWarn->Logf(TEXT("Connection closed!!"));
		if( ResultCount >= 0 && ReceiveCount != ResultCount )
			Actor->delegateOnQueryFinished(RI_ConnectionTimeout,0);
	}

	void SaveDownloadSites( TArray<FString>& DownloadSites )
	{
		FString s;
		for( INT i=0;i<DownloadSites.Num();i++ )
			s = s + DownloadSites(i) + TEXT("\n");

		// Save AutoPatch.txt
		GFileManager->Delete( DOWNLOADLISTFILE, 0, 1 );
		appSaveStringToFile( s, DOWNLOADLISTFILE );
	}

	void OnDataReceived()
	{
		while( DataAvailable() )
		{
			switch( ClientState )
			{
			case MSCS_WaitingChallenge:
				{
					FString Challenge;
					*ArRecv << Challenge;
					FString CDKeyHash, Response, ClientType;
					CDKeyHash = GetCDKeyHash();
					Response = GetCDKeyResponse( *Challenge ) ;
					ClientType = TEXT("CLIENT");
					INT Version = ENGINE_VERSION;
					BYTE Platform = GRunningOS;
					FString Language = UObject::GetLanguage();

					*ArSend << CDKeyHash << Response << ClientType << Version << Platform << Language;
					ArSend->Flush();
					ClientState = MSCS_WaitingApproval;
				}
				break;
			case MSCS_WaitingApproval:
				{
					FString Approval;
					*ArRecv << Approval;

					if( Approval == TEXT("APPROVED") )
					{
						*ArSend << QueryType;
						switch( QueryType )
						{
						case CTM_Query:
							*ArSend << Actor->Query;
							break;
						case CTM_GetMOTD:
							break;
						}
						ArSend->Flush();
						ClientState = MSCS_SentQuery;
					}
					else
					if( Approval == TEXT("UPGRADE") )
					{
						INT UpgradeVersion;
						TArray<FString> DownloadSites;
						FString MOTD;
						*ArRecv << UpgradeVersion << DownloadSites << MOTD;
						SaveDownloadSites(DownloadSites);
						Actor->delegateOnReceivedMOTDData( MR_MandatoryUpgrade, FString::Printf(TEXT("%d"), UpgradeVersion) );
						Actor->delegateOnReceivedMOTDData( MR_MOTD, MOTD );
						Actor->delegateOnQueryFinished(RI_MustUpgrade, UpgradeVersion);
					}
					else
					if( Approval == TEXT("MSLIST") )
					{
						TArray<FString> MasterServers;
						TArray<INT>		MasterServerPorts;
						*ArRecv << MasterServers << MasterServerPorts;
						for( INT i=0;i<5;i++ )
						{
							if( i < MasterServers.Num() )
								Actor->MasterServerAddress[i] = MasterServers(i);
							else
								Actor->MasterServerAddress[i] = TEXT("");

							if( i < MasterServerPorts.Num() )
								Actor->MasterServerPort[i] = MasterServerPorts(i);
							else
								Actor->MasterServerPort[i] = 0;
						}
						Actor->CurrentMasterServer = 0;
						Actor->SaveConfig();
						Actor->delegateOnQueryFinished(RI_ConnectionFailed,0);
					}
					else
					if( Approval == TEXT("DENIED") )
					{
						Actor->delegateOnQueryFinished(RI_AuthenticationFailed,0);
					}
					else
					{
						// master server busy, or i didn't understand its response
						Actor->CurrentMasterServer++;
						Actor->SaveConfig();
						Actor->delegateOnQueryFinished(RI_ConnectionFailed,0);
					}
				}
				break;
			case MSCS_SentQuery:
				{
					switch( QueryType )
					{
					case CTM_Query:
						*ArRecv << ResultCount << ResultsCompressed;
						Actor->ResultCount = ResultCount;
						if( ResultCount > 0 )
							ClientState = MSCS_WaitQueryData;
						else
						{
							Actor->delegateOnQueryFinished(RI_Success,0);
							Close();
						}
						break;
					case CTM_GetMOTD:
						{
							INT UpgradeVersion;
							FString MOTD;
							*ArRecv << MOTD << UpgradeVersion;
							Actor->delegateOnReceivedMOTDData( MR_MOTD, MOTD );
							if( UpgradeVersion )
							{
								TArray<FString> DownloadSites;
								*ArRecv << DownloadSites;
								SaveDownloadSites(DownloadSites);
								Actor->delegateOnReceivedMOTDData( MR_OptionalUpgrade, FString::Printf(TEXT("%d"), UpgradeVersion) );
							}
							Actor->delegateOnQueryFinished(RI_Success,0);
							Close();
						}
						break;
					}
				}
				break;
			case MSCS_WaitQueryData:
				{
					if( ReceiveCount < ResultCount )
					{
						FServerResponseLine Server;
						appMemzero( &Server, sizeof(FServerResponseLine) );
						ReceiveCount++;
						if( ResultsCompressed )
						{
                            // decode compressed results.
							DWORD IP;
							_WORD Port;
							_WORD QueryPort;
							BYTE CurrentPlayers;
							BYTE MaxPlayers;

							*ArRecv << IP << Port << QueryPort;
							if( ArRecv->IsError() )
							{
								Close();
								return;
							}
							*ArRecv << Server.ServerName;
							if( Server.ServerName.Len() == 25 )
								Server.ServerName = Server.ServerName + TEXT("....");

							if( ArRecv->IsError() )
							{
								Close();
								return;
							}
							*ArRecv << Server.MapName;

							if( ArRecv->IsError() )
							{
								Close();
								return;
							}
							*ArRecv << Server.GameType
									<< CurrentPlayers
									<< MaxPlayers;

							INT j = Server.MapName.InStr(TEXT("-"));
							UBOOL HasPrefix = (j==2 || j==3);
							if( Server.GameType == TEXT("0") )
							{
								Server.GameType = TEXT("xDeathMatch");
								if( !HasPrefix )
									Server.MapName = FString(TEXT("DM-")) + Server.MapName;
							}
							else
							if( Server.GameType == TEXT("1") )
							{
								Server.GameType = TEXT("xCTFGame");
								if( !HasPrefix )
									Server.MapName = FString(TEXT("CTF-")) + Server.MapName;
							}
							else
							if( Server.GameType == TEXT("2") )
							{
								Server.GameType = TEXT("xBombingRun");
								if( !HasPrefix )
									Server.MapName = FString(TEXT("BR-")) + Server.MapName;
							}
							else
							if( Server.GameType == TEXT("3") )
							{
								Server.GameType = TEXT("xTeamGame");
								if( !HasPrefix )
									Server.MapName = FString(TEXT("DM-")) + Server.MapName;
							}
							else
							if( Server.GameType == TEXT("4") )
							{
								Server.GameType = TEXT("xDoubleDom");
								if( !HasPrefix )
									Server.MapName = FString(TEXT("DOM-")) + Server.MapName;
							}

							FIpAddr Addr;
							Addr.Addr = ntohl(IP);
							Server.IP = Addr.GetString(0);
							Server.Port = Port;
							Server.QueryPort = QueryPort;
							Server.CurrentPlayers = CurrentPlayers;
							Server.MaxPlayers = MaxPlayers;
						}
						else
							*ArRecv << Server;

						Actor->delegateOnReceivedServer(Server);
					}

					if( ReceiveCount == ResultCount )
					{
						Actor->delegateOnQueryFinished(RI_Success,0);
						Close();
					}
				}
				break;
			}
		}
	}
};

/*-----------------------------------------------------------------------------
	AMasterServerLink
-----------------------------------------------------------------------------*/
void AMasterServerLink::execPoll( FFrame& Stack, RESULT_DECL )
{
	guard(AMasterServerLink::execPoll);
	P_GET_INT(WaitTime);
	P_FINISH;
	*(DWORD*)Result = Poll(WaitTime);
	unguard;
}

IMPLEMENT_CLASS(AMasterServerLink);

/*-----------------------------------------------------------------------------
	AMasterServerClient
-----------------------------------------------------------------------------*/
void AMasterServerClient::Init()
{
	Super::Init();

	// Make a new link to the master server
	if( MSLinkPtr )
		delete ((FMasterServerClientLink*)MSLinkPtr);
    MSLinkPtr = (INT)(new FMasterServerClientLink(this));
}

UBOOL AMasterServerClient::Poll( INT WaitTime )
{
	guard(AMasterServerClient::Poll);
	Super::Poll( WaitTime );
	if( MSLinkPtr )
		return ((FMasterServerClientLink*)MSLinkPtr)->Poll(WaitTime);
	return 0;
	unguard;
}

void AMasterServerClient::Destroy()
{
	if( MSLinkPtr )
		delete ((FMasterServerClientLink*)MSLinkPtr);
	MSLinkPtr = 0;
	Super::Destroy();
}

void AMasterServerClient::PostScriptDestroyed()
{
	Super::PostScriptDestroyed();
	if( MSLinkPtr )
		delete ((FMasterServerClientLink*)MSLinkPtr);
	MSLinkPtr = 0;
}

void AMasterServerClient::execStartQuery( FFrame& Stack, RESULT_DECL )
{
	P_GET_BYTE(QueryType)
	P_FINISH;
	Init();
    ResultCount = 0; // gam
	((FMasterServerClientLink*)MSLinkPtr)->StartQuery(QueryType);
}

void AMasterServerClient::execStop( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;
	if( MSLinkPtr )
		delete ((FMasterServerClientLink*)MSLinkPtr);
	MSLinkPtr = 0;
}

void AMasterServerClient::execLaunchAutoUpdate( FFrame& Stack, RESULT_DECL )
{
	guard(AMasterServerClient::execLaunchAutoUpdate);
	P_FINISH;

#if (defined WIN32)
    appLaunchURL( TEXT("Setup.exe"), TEXT("autopatch") );
#elif (defined __linux__)
    GUnixSpawnOnExit = TEXT("../updater/update");
#endif

	appRequestExit(0);
	unguard;
}

IMPLEMENT_CLASS(AMasterServerClient);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/


