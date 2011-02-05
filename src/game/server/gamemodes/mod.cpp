#include <cstdio>
#include <cstdarg>

#include <engine/shared/config.h>

#include <game/server/gamecontext.h>

#include "mod.h"

#define MAX_ANNOUNCE 256
#define MAX_BROADCAST 256
#define MAX_HILLS (TILE_HILL_LAST - TILE_HILL_FIRST + 1)

#define KOTH_MSGPFX "## KotH ## - "

//originally just for ease of devel, but come handy
#define TICK Server()->Tick()
#define TICKSPEED Server()->TickSpeed()
#define CFG(X) g_Config.m_Sv ## X
#define CHAR(CID) GameServer()->GetPlayerChar(CID)
#define forhills(I) for(int I = 0; I < MAX_HILLS; ++I)
#define forcids(I) for(int I = 0; I < MAX_CLIENTS; ++I) if (!m_aCharExt[I].m_Valid) {} else

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "KotH";
	m_LastBroadcast = 0;
}
	
CGameControllerMOD::~CGameControllerMOD()
{
}

void CGameControllerMOD::OnCharacterSpawn(class CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
	int Cid = pChr->GetPlayer()->GetCID();
	
	m_aCharExt[Cid].m_Valid = false; // will be true on next tick
	m_aCharExt[Cid].m_OwnedHill = m_aCharExt[Cid].m_AcquiringHill = -1;
	m_aCharExt[Cid].m_IsLosing = false;

	m_aCharExtPrev[Cid] = m_aCharExt[Cid];
	pChr->GetPlayer()->m_Score = 0;
}

int CGameControllerMOD::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	int res = IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);
	int Cid = pVictim->GetPlayer()->GetCID();
	if (m_aCharExt[Cid].m_OwnedHill != -1)
	{
		Announce("%s has given up hill %c.", Server()->ClientName(Cid), 'A' + m_aCharExt[Cid].m_OwnedHill);
		m_aCharExt[Cid].m_OwnedHill = -1;
	}
	return res;
}

void CGameControllerMOD::Tick()
{
	IGameController::Tick();

	for(int Cid = 0; Cid < MAX_CLIENTS; ++Cid)
		m_aCharExt[Cid].m_Valid = CHAR(Cid);

	if (m_Warmup || m_GameOverTick != -1)
		return;

	DoHillOwnage();
	DoScoring();
	DoBroadcasts();
	DoEmotes();

	mem_copy(m_aCharExtPrev, m_aCharExt, sizeof m_aCharExt);
}

void CGameControllerMOD::DoScoring()
{
	forcids(Cid)
		if (m_aCharExt[Cid].m_OwnedHill != -1 && m_aCharExt[Cid].m_LastScoreTick + CFG(ScoreInterval) * TICKSPEED < TICK)
		{
			m_aCharExt[Cid].m_LastScoreTick = TICK;
			CHAR(Cid)->GetPlayer()->m_Score += GetScoreForHill(m_aCharExt[Cid].m_OwnedHill);
		}
}

void CGameControllerMOD::DoHillOwnage()
{
	int aTouchedHill[MAX_CLIENTS];

	// iterate over players, update everything
	forcids(Cid)
	{
		int Col = GameServer()->Collision()->GetTile(round(CHAR(Cid)->m_Pos.x), round(CHAR(Cid)->m_Pos.y));
		aTouchedHill[Cid] = (Col >= TILE_HILL_FIRST && Col <= TILE_HILL_LAST) ? Col - TILE_HILL_FIRST : -1;

		if (m_aCharExt[Cid].m_OwnedHill == -1 && aTouchedHill[Cid] == -1) // we don't own a hill, we dont touch a hill
		{
			if (m_aCharExt[Cid].m_AcquiringHill != -1) // we left the hill we were gonna acquire
				m_aCharExt[Cid].m_AcquiringHill = -1;

			m_aCharExt[Cid].m_EnterTick = -1;
		}
		else if (m_aCharExt[Cid].m_OwnedHill == aTouchedHill[Cid]) // stay in, or re-enter owned hill
		{
			if (m_aCharExt[Cid].m_IsLosing) // re-enter
			{
				m_aCharExt[Cid].m_IsLosing = false;
				m_aCharExt[Cid].m_EnterTick = TICK;
			}
		}
		else // owned and touched hill differ (not implying we own one)
		{
			if (m_aCharExt[Cid].m_OwnedHill != -1) // we're outside of owned hill
			{
				if (!m_aCharExt[Cid].m_IsLosing) // we start losing our hill
				{
					m_aCharExt[Cid].m_IsLosing = true;
					m_aCharExt[Cid].m_LoseCooldown = CFG(LoseDelay) * TICKSPEED; // start lose cooldown
				}
				else if (m_aCharExt[Cid].m_LoseCooldown-- <= 0) // we eventually lost it
				{
					m_aCharExt[Cid].m_OwnedHill = -1;
					m_aCharExt[Cid].m_IsLosing = false;
				}
				else if (aTouchedHill[Cid] != -1 && CFG(EarlyLose)) //early lose, will lose on next tick
					m_aCharExt[Cid].m_LoseCooldown = 0;
			}
			else // we dont own a hill, but we're inside one.
			{
				if (m_aCharExt[Cid].m_EnterTick == -1) // we were outside before
					m_aCharExt[Cid].m_EnterTick = TICK;

				if (m_aCharExt[Cid].m_AcquiringHill == aTouchedHill[Cid]) // we're acquiring the hill we're in
				{
					if (m_aCharExt[Cid].m_AcquireCooldown-- <= 0) // we got it
					{
						m_aCharExt[Cid].m_OwnedHill = aTouchedHill[Cid];
						m_aCharExt[Cid].m_LastScoreTick = CFG(EarlyScore)?0:TICK;
						m_aCharExt[Cid].m_AcquiringHill = -1;
						m_aCharExt[Cid].m_IsLosing = false;
					}
				}
			}
		}
	}

	// iterate over hills, determine who's gonna acquire a hill
	forhills(Hid)
	{
		int NumOwn = 0;
		int MinEnterTick = -1, MinEnterCid = -1;

		// first count number of owners of this hill
		forcids(Cid)
			if (m_aCharExt[Cid].m_OwnedHill == Hid || m_aCharExt[Cid].m_AcquiringHill == Hid)
				NumOwn++;

		if (NumOwn >= CFG(MaxKingsPerHill)) // hill cannot take more kings, continue with next hill
			continue;

		forcids(Cid) // find the one having been on that hill for longest
			if ((aTouchedHill[Cid] == Hid && m_aCharExt[Cid].m_AcquiringHill != Hid && m_aCharExt[Cid].m_OwnedHill == -1)
					&& (MinEnterTick == -1 || m_aCharExt[Cid].m_EnterTick < MinEnterTick))
				MinEnterTick = m_aCharExt[MinEnterCid = Cid].m_EnterTick;

		// this one has been on the hill for longest, he will acquire it
		if (MinEnterCid != -1)
		{
			m_aCharExt[MinEnterCid].m_AcquiringHill = Hid;
			m_aCharExt[MinEnterCid].m_AcquireCooldown = CFG(AcquireDelay) * TICKSPEED; // start acquire cooldown
		}

		// if the hill can take even more players, and there are some remaining who want to have it, they will get selected on next tick.
	}

	// announce the changes in hill ownerships
	forcids(Cid)
		if (m_aCharExt[Cid].m_OwnedHill == -1 && m_aCharExtPrev[Cid].m_OwnedHill != -1)
			Announce("%s has lost hill %c!", Server()->ClientName(Cid), 'A' + m_aCharExtPrev[Cid].m_OwnedHill);
		else if (m_aCharExt[Cid].m_OwnedHill != -1 && m_aCharExtPrev[Cid].m_OwnedHill == -1)
			Announce("%s has acquired hill %c!", Server()->ClientName(Cid), 'A' + m_aCharExt[Cid].m_OwnedHill);
}

void CGameControllerMOD::DoEmotes()
{
	forcids(Cid)
		if (m_aCharExt[Cid].m_OwnedHill != -1) // we own a hill
		{
			if (CFG(EyesOwn) >= 0 && TICK % TICKSPEED == 0) // set OWN-eyes every second
				CHAR(Cid)->SetEmote(CFG(EyesOwn), TICK + TICKSPEED);
			if (CFG(EmoticonLose) >= 0 && m_aCharExt[Cid].m_IsLosing && m_aCharExt[Cid].m_LoseCooldown % (TICKSPEED>>1) == 0)
				GameServer()->SendEmoticon(Cid, CFG(EmoticonLose)); // we're losing our hill, do lose-emoticon every 1/2 sec
		}
		else if (m_aCharExt[Cid].m_EnterTick != -1 && m_aCharExt[Cid].m_OwnedHill == -1 && m_aCharExt[Cid].m_AcquiringHill == -1) // want a hill, but not can haz.
		{
			if (CFG(EyesCannotAcquire) >= 0 && TICK % TICKSPEED == 0) // cannotacquire-eyes every sec
				CHAR(Cid)->SetEmote(CFG(EyesCannotAcquire), TICK + TICKSPEED);
		}
		else if (m_aCharExt[Cid].m_AcquiringHill != -1) // we're acquiring one
		{
			if (CFG(EmoticonAcquire) >= 0 && m_aCharExt[Cid].m_AcquireCooldown % (TICKSPEED>>1) == 0)
				GameServer()->SendEmoticon(Cid, CFG(EmoticonAcquire)); // do acquire emoticon every 1/2 sec
		}
}

// TODO: too much code duplication here
void CGameControllerMOD::DoBroadcasts()
{
	char aaMainLine[MAX_CLIENTS][MAX_BROADCAST];
	char aaSubLine[MAX_CLIENTS][MAX_BROADCAST];
	char aBuf[MAX_BROADCAST];

	// recreate/dispatch every BroadcastInterval ticks
	if (m_LastBroadcast + CFG(BroadcastInterval) < TICK)
	{
		m_LastBroadcast = TICK;
		forcids(Cid)
			aaMainLine[Cid][0] = aaSubLine[Cid][0] = '\0';

		int MaxOwnedHid = -1;
		int CurHid;

		// find the master hill
		for(CurHid = MAX_HILLS - 1; MaxOwnedHid == -1 && CurHid >= 0; --CurHid)
			forcids(Cid)
				if (m_aCharExt[Cid].m_OwnedHill == CurHid)
				{
					MaxOwnedHid = CurHid;
					break;//don't like cross break
				}

		if (MaxOwnedHid < 0)
			forcids(Cid)
				str_format(aaMainLine[Cid], MAX_BROADCAST, "No hill is currently being owned");
		else
		{
			char *pPos = aBuf;
			int Left = sizeof aBuf;
			int NumOwner = 0;

			// concatenate names of players owning master hill
			forcids(Cid)
			{
				if (m_aCharExt[Cid].m_OwnedHill == MaxOwnedHid)
				{
					if (Left > 0)
					{
						str_format(pPos, Left, "%s%s", NumOwner?", ":"", Server()->ClientName(Cid));
						int Len = str_length(pPos);
						Left -= Len;
						pPos += Len;
					}

					NumOwner++;
				}
			}

			// construct the headline
			forcids(Cid)
				str_format(aaMainLine[Cid], MAX_BROADCAST, "Master King%s (Hill %c): %s", NumOwner>1?"s":"", 'A' + MaxOwnedHid, aBuf);

			if (MAX_HILLS > 1 && MaxOwnedHid > 0) // (possibly) moar hills owned besides master hill?
			{
				static int SecHill = -1; // we rotate the second line to display one owned hill at a time

				int StartSecHill = -1;

				do {
					SecHill++;

					if (SecHill >= MaxOwnedHid) // no owned hills beyond master hill, start over
						SecHill = 0;

					if (StartSecHill == -1)
						StartSecHill = SecHill; // save first value for determining when we're through searching w/o results
					else if (StartSecHill == SecHill)
						break; // no other hill owned, besides master hill

					pPos = aBuf;
					Left = sizeof aBuf;
					NumOwner = 0;
					// concatenate names of players owning this hill
					forcids(Cid)
					{
						if (m_aCharExt[Cid].m_OwnedHill == SecHill)
						{
							if (Left > 0)
							{
								str_format(pPos, Left, "%s%s", NumOwner?", ":"", Server()->ClientName(Cid));
								int Len = str_length(pPos);
								Left -= Len;
								pPos += Len;
							}
							NumOwner++;
						}
					}
				} while(!NumOwner); // found something owned, done

				// construct the subline
				if (NumOwner)
					forcids(Cid)
						str_format(aaSubLine[Cid], MAX_BROADCAST, "(King%s of Hill %c: %s)", NumOwner>1?"s":"", 'A' + SecHill, aBuf);
			}
		}

		// create special broadcasts for acquiring/losing players
		forcids(Cid)
		{
			if (m_aCharExt[Cid].m_AcquiringHill != -1)
				str_format(aaSubLine[Cid], MAX_BROADCAST, "** Hill %c is yours in %ds! **", 'A' + m_aCharExt[Cid].m_AcquiringHill,
						m_aCharExt[Cid].m_AcquireCooldown / TICKSPEED);
			else if (m_aCharExt[Cid].m_IsLosing)
				str_format(aaSubLine[Cid], MAX_BROADCAST, "** Losing hill %c in %ds! **", 'A' + m_aCharExt[Cid].m_OwnedHill,
						m_aCharExt[Cid].m_LoseCooldown / TICKSPEED);
		}

		// dispatch broadcasts
		forcids(Cid)
		{
			str_format(aBuf, sizeof aBuf, "%s%s%s ", aaMainLine[Cid], aaSubLine[Cid][0]?"\n":"", aaSubLine[Cid][0]?aaSubLine[Cid]:"");

			GameServer()->SendBroadcast(aBuf, Cid);
		}
	}
}

void CGameControllerMOD::Announce(const char *pFormat, ...)
{
	char aBuf[MAX_ANNOUNCE];
	va_list vl;

	if (!pFormat || !*pFormat)
		return;

	str_copy(aBuf, KOTH_MSGPFX, str_length(KOTH_MSGPFX) + 1);

	va_start(vl, pFormat);
	vsnprintf(aBuf + str_length(KOTH_MSGPFX), MAX_ANNOUNCE - str_length(KOTH_MSGPFX), pFormat, vl);
	va_end(vl);

	aBuf[MAX_ANNOUNCE-1] = '\0';
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
}

int CGameControllerMOD::GetScoreForHill(int Hid)
{
	switch(Hid)
	{
		case 0: return CFG(ScoreHillA); // yeah this sucks hard
		case 1: return CFG(ScoreHillB);
		case 2: return CFG(ScoreHillC);
		case 3: return CFG(ScoreHillD);
		case 4: return CFG(ScoreHillE);
		case 5: return CFG(ScoreHillF);
		case 6: return CFG(ScoreHillG);
		case 7: return CFG(ScoreHillH);
		case 8: return CFG(ScoreHillI);
		case 9: return CFG(ScoreHillJ);
		case 10: return CFG(ScoreHillK);
		case 11: return CFG(ScoreHillL);
		case 12: return CFG(ScoreHillM);
		default : return 1;
	}
}
