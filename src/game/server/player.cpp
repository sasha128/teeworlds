/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_pAccount = 0;
	blockScore = 0;
	m_LastAnnoyingMsg = 0;
	
	for (int i = 1;i < 16;i++)
	{
	    idMap[i] = -1;
	}
	idMap[0] = ClientID;
	m_ChatScore = 0;
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	if (GetAccount())
	{
		blockScore = GetAccount()->Payload()->blockScore;
	}

	Server()->SetClientScore(m_ClientID, blockScore);

	if (m_ChatScore > 0)
		m_ChatScore--;

	Server()->SetClientScore(m_ClientID, m_Score);

	// do latency stuff
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
		m_Spawning = true;

	if(m_pCharacter)
	{
		if(m_pCharacter->IsAlive())
		{
			m_ViewPos = m_pCharacter->m_Pos;
		}
		else
		{
			delete m_pCharacter;
			m_pCharacter = 0;
		}
	}
	else if(m_Spawning && m_RespawnTick <= Server()->Tick())
		TryRespawn();
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	int id = -1;
	int* idMap=GameServer()->m_apPlayers[SnappingClient]->idMap;
	for (int i = 0;i < 16;i++)
	{
	    if (idMap[i] == m_ClientID)
	    {
		id = i;
		break;
	    }
	}
	if (id == -1)
		return;
	
	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));

	if(!pClientInfo)
		return;

	const char* DbgStateChars = "fsizb";

	if (g_Config.m_SvScoringDebug && m_pCharacter)
	{
		char dbgName[200];
		str_format(dbgName, sizeof(dbgName), "%c%d::%d%s", DbgStateChars[m_pCharacter->State], m_pCharacter->lastInteractionPlayer, m_ClientID, Server()->ClientName(m_ClientID));
		StrToInts(&pClientInfo->m_Name0, 4, dbgName);
	}
	else
		StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));
	if (!g_Config.m_SvLoginClan || !GetAccount())
		StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	else StrToInts(&pClientInfo->m_Clan0, 3, GetAccount()->Name());
	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, id, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_Score = blockScore;
	pPlayerInfo->m_ClientID = id;
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if(m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void CPlayer::OverrideColors(int Color)
{
	bool Ch = false;
	
	if (Color < 0) // reset
	{
		Ch = (m_TeeInfos.m_UseCustomColor != m_OrigTeeInfos.m_UseCustomColor)
			|| (m_TeeInfos.m_ColorBody != m_OrigTeeInfos.m_ColorBody)
			|| (m_TeeInfos.m_ColorFeet != m_OrigTeeInfos.m_ColorFeet);
		m_EnforcedColors = false;
		m_TeeInfos.m_UseCustomColor = m_OrigTeeInfos.m_UseCustomColor;	
		m_TeeInfos.m_ColorBody = m_OrigTeeInfos.m_ColorBody;	
		m_TeeInfos.m_ColorFeet = m_OrigTeeInfos.m_ColorFeet;	
	}
	else
	{
		Ch = (m_TeeInfos.m_UseCustomColor != 1)
			|| (m_TeeInfos.m_ColorBody != Color)
			|| (m_TeeInfos.m_ColorFeet != Color);
		m_EnforcedColors = true;
		m_TeeInfos.m_UseCustomColor = 1;
		m_TeeInfos.m_ColorBody = m_TeeInfos.m_ColorFeet = Color;	
	}
	if (Ch)
		GameServer()->m_pController->OnPlayerInfoChange(this);
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	if(!m_pCharacter && m_Team == TEAM_SPECTATORS && m_SpectatorID == SPEC_FREEVIEW)
		m_ViewPos = vec2(NewInput->m_TargetX, NewInput->m_TargetY);

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

int CPlayer::BlockKillCheck()
{
	int killer = m_ClientID;
	if (m_pCharacter->State != BS_FROZEN) return killer;
	if (Server()->ClientIngame(m_pCharacter->lastInteractionPlayer))
	{
		char aBuf[16];//for loltext
		killer = m_pCharacter->lastInteractionPlayer;
		double scoreStolen = min(blockScore * g_Config.m_SvScoreSteal, GameServer()->m_apPlayers[killer]->blockScore * g_Config.m_SvScoreStealLimit) / 100;
		blockScore -= scoreStolen;
		if (GetAccount())
		{
			GetAccount()->Payload()->blockScore = blockScore;
			str_format(aBuf, sizeof aBuf, "-%.1f", scoreStolen);
			GameServer()->CreateLolText(m_pCharacter, false, vec2(0,-100), vec2(0,-1), 50, aBuf);
		}
		double minSteal = (double)g_Config.m_SvScoreCreep / 1000;
		if (scoreStolen < minSteal && GameServer()->m_apPlayers[killer]->GetAccount()) // if killer has account, give him score for unreg creeps
			scoreStolen = minSteal;
		GameServer()->m_apPlayers[killer]->blockScore += scoreStolen;
		if (GameServer()->m_apPlayers[killer]->GetAccount())
		{
			GameServer()->m_apPlayers[killer]->GetAccount()->Payload()->blockScore = GameServer()->m_apPlayers[killer]->blockScore;

			str_format(aBuf, sizeof aBuf, "+%.1f", scoreStolen);
			GameServer()->CreateLolText(GameServer()->GetPlayerChar(killer), false, vec2(0,-100), vec2(0,-1), 50, aBuf);
		}
		else
		{
			if (g_Config.m_SvRegisterMessageInterval != 0 && (m_LastAnnoyingMsg == 0 || Server()->Tick() - m_LastAnnoyingMsg > Server()->TickSpeed()*g_Config.m_SvRegisterMessageInterval))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s, say /reg in chat to register and gain score for killing %s", Server()->ClientName(killer), Server()->ClientName(m_ClientID));
				GameServer()->SendChatTarget(killer, aBuf);
				m_LastAnnoyingMsg = Server()->Tick();
			}
		}
	}
	return killer;
}

void CPlayer::BlockKill()
{
	if (!m_pCharacter) return;
	int killer = BlockKillCheck();
	if (killer == m_ClientID) return;
	m_pCharacter->SendKillMsg(killer, WEAPON_HAMMER, 0);
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		int killer = BlockKillCheck();
		m_pCharacter->Die(killer, killer == m_ClientID ? Weapon : WEAPON_NINJA);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team)
{
	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

	KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if(!GameServer()->m_pController->CanSpawn(m_Team, &SpawnPos))
		return;

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos);
}
