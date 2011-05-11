#include <list>

#include <cstdlib>
#include <cstring>

#include <base/system.h>

#include <engine/shared/config.h>

#include <game/generated/protocol.h>

#include <game/server/gamecontext.h>

#include <game/server/entities/door.h>

#include "mod.h"

#define MAX_FIGHTBC 64
#define BCLEN 50

#define IDLE_MSG "waiting for players..."
#define PREPARE_MSG_RED "get in position! game commencing in %ds"
#define PREPARE_MSG_BLUE "get ready! door will open in %ds"

#define forlist(L,T,I) for(std::list<T>::const_iterator I = (L)->begin(); \
		I != (L)->end(); ++I)
#define forset(S,T,I) for(std::set<T>::const_iterator I = (S)->begin(); \
		I != (S)->end(); ++I)

#define PUSH_TEAM 0
#define DEF_TEAM 1

#define REACH_THRESHOLD 5.0f
#define TS Server()->TickSpeed()
#define GS GameServer()
#define CFG(A) g_Config.m_SvPay ## A
#define TICK Server()->Tick()
#define D(F, ARGS...) dbg_msg("mod", "%s:%i:%s(): " F, __FILE__, __LINE__,\
		 __func__,##ARGS)

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer) :
	CGameControllerTDM(pGameServer), m_LastNode(NULL)
{
	m_pGameType = "PAYLOAD";
	memset(m_apBroadcasts, 0, MAX_CLIENTS * sizeof(char*));
	Reset();
}

void CGameControllerMOD::Broadcast()
{
	if (!CFG(BroadcastFreq) 
			|| m_LastBroadcast + CFG(BroadcastFreq) * TS > TICK)
		return;

	for (int z = 1; z < MAX_CLIENTS; ++z)
		if (GS->m_apPlayers[z] && m_apBroadcasts[z])
			GS->SendBroadcast(m_apBroadcasts[z], z);

	m_LastBroadcast = TICK;
}

void CGameControllerMOD::UpdateBroadcast(int cid, const char *str, 
		size_t len)
{
	if (!str)
		return;
	if (cid >= 0) {
		free(m_apBroadcasts[cid]);
		m_apBroadcasts[cid] = strndup(str, len ? len : strlen(str));
	} else {
		for (int z = 1; z < MAX_CLIENTS; ++z)
			UpdateBroadcast(z, str, len);
	}
}

void CGameControllerMOD::IdleTick()
{
	D("IdleTick()");
	ChangeState(GAMESTATE_PREPARE);//XXX
}
void CGameControllerMOD::PrepTick()
{
	D("PrepTick()");
	ChangeState(GAMESTATE_FIGHT);//XXX
}

void CGameControllerMOD::HandlePayload()
{
	static CCharacter *Proxim[MAX_CLIENTS];
	static float fluct = 1.0f;
	CCharacter *DummyChar = GS->GetPlayerChar(MAX_CLIENTS - 1);
	if (!DummyChar)
		return; //XXX
	vec2 DummyPos = DummyChar->m_Pos;
	//D("actual dummy pos is (%f,%f)",DummyPos.x,DummyPos.y);
	int Num = GS->m_World.FindEntities(DummyPos, CFG(Radius), (CEntity**)Proxim, (sizeof Proxim) / (sizeof(CCharacter*)), NETOBJTYPE_CHARACTER);

	DummyChar->SetCoreVel(vec2(0.0f, 0.0f));

	fluct *= -1.0f;

	//D("%i tees in proximity, current dir: %c, ht: %i", Num-1, (m_PayloadDir==FORE)?'F':((m_PayloadDir==BACK)?'B':'H'), m_HaltTicks);

	if (Num > 1) { /* payload itself is always part of the result set */
		switch (m_PayloadDir) {
			case FORE:
				break;
			case BACK:
			case HALT:
				m_HaltTicks = 0;
				m_PayloadDir = FORE;
				//D("set dir to F, ht to 0");
		}
	} else {
		switch (m_PayloadDir) {
			case BACK:
				break;
			case FORE:
				m_PayloadDir = HALT;
				m_HaltTicks = CFG(HaltDelay) * TS;
				//D("set dir to H, ht to %i",m_HaltTicks);
				break;
			case HALT:
				if (m_HaltTicks <= 0) {
					//D("set dir to B");
					m_PayloadDir = BACK;
				}
		}
	}

	if (m_PayloadDir == HALT) {
		DummyChar->SetPos(DummyChar->GetPos() + vec2(0.0, fluct));
		return;
	}

	CPathNode *TargetNode = (m_PayloadDir == BACK) ? m_LastNode : m_LastNode->Next();

	//D("targetnode is at (%i, %i)",TargetNode->LocX(),TargetNode->LocY());

	if (length(TargetNode->LocVec() - DummyPos) < REACH_THRESHOLD) {
		//D("we reached it");
		DummyChar->SetPos(TargetNode->LocVec());

		if (m_PayloadDir == FORE) {
			TargetNode = TargetNode->Next();
			m_LastNode = m_LastNode->Next();
			if (m_LastNode->IsCheckpoint()) {
				m_LastCheckNode = m_LastNode;
			}
			//D("updating target and last node to theirs nexts");
			if (!m_LastNode->Next()) { //we arrived at the very end
				//do win stuff XXX
				//D("we're at the end");
				m_GameState = GAMESTATE_END;
				return;
			}
		} else {
			if (TargetNode == m_LastCheckNode) {
				m_PayloadDir = HALT;
				//D("forcing payload to halt");
			} else {
				TargetNode = TargetNode->Prev();
				m_LastNode = m_LastNode->Prev();
				//D("updating target and last node to theirs prevs");
			}
		}
	}

	if (m_PayloadDir == HALT) {
		DummyChar->SetPos(DummyChar->GetPos() + vec2(0.0, fluct));
		return;
	}

	if (m_PayloadDir == FORE) {
		//D("we still want to move, forewards");
		float Accum = 0.0f;
		for (int z = 0; z < Num; ++z) {
			if (Proxim[z] == DummyChar || Proxim[z]->GetPlayer()->GetTeam() == DEF_TEAM)
				continue;
			float Dist = distance<float> (DummyPos, Proxim[z]->m_Pos);
			if (Dist < 32.0f)
				Dist = 32.0f;
			Accum += 32.0f / Dist;
			//D("dist is %f, accum: %f",Dist, Accum);
		}
		if (Accum > 0.0f) {
			vec2 NewVel = normalize(TargetNode->LocVec() - DummyPos) * (Accum * ((float)CFG(MoveSpeed)));
			//D("accum: %f, posdiff: %f, %f",Accum, NewVel.x,NewVel.y);
			DummyChar->SetPos(DummyChar->GetPos() + NewVel);
		}
	} else {
		//D("we still want to move, backwards");
		vec2 NewVel = normalize(TargetNode->LocVec() - DummyPos) * ((float)CFG(BackSpeed));
		//D("posdiff: %f, %f",NewVel.x,NewVel.y);
		DummyChar->SetPos(DummyChar->GetPos() + NewVel);
	}

}

void CGameControllerMOD::FightTick()
{
	
	//D("FightTick()");
	//wincheck

	HandlePayload();

	if (m_HaltTicks > 0)
		--m_HaltTicks;
	
	FightBroadcast();

}

void CGameControllerMOD::FightBroadcast()
{
//	char BCMsg[MAX_FIGHTBC];
//	if (m_LastFightBroadcast + Server()->TickSpeed() >= Server()->Tick()) 
//		return;
	
	

	//float f = 

	//m_LastFightBroadcast = Server()->Tick();

		
}

void CGameControllerMOD::EndTick()
{
	D("EndTick()");
}

void CGameControllerMOD::Tick()
{
	static bool VeryFirstTick = true;
	IGameController::Tick(); //bypass tdm game ctl, we dont want a score wincheck
	if (VeryFirstTick) {
		InitDummy();
		VeryFirstTick = false;
		//XXX door test
		CDoor *d = new CDoor(&GameServer()->m_World, vec2(1166, 1000), vec2(1166, 880), "A");
		GameServer()->m_World.InsertEntity(d);
		d->SetOpen(false);
		//XXX door test
		return;//XXX
	}
	Broadcast();
	(this->*m_TickSelect[m_GameState])();
}

vec2 CGameControllerMOD::GetMoveDirection()
{
	//if (!m_NextNode) return vec2(0.0,-1.0); //XXX
	//return normalize(m_NextNode->LocVec() - GS->GetPlayerChar(MAX_CLIENTS - 1)->m_Pos);
	return vec2(0.0f, 0.0f);
}

void CGameControllerMOD::InitDummy()
{
	int DummyID = MAX_CLIENTS - 1;
	D("init dummy called");
	CPlayer *p;

	GS->m_apPlayers[DummyID] = p = new (DummyID) CPlayer(GS, DummyID, 1);
	p->m_LastChangeInfo = TICK;

	//p->m_TeeInfos.m_UseCustomColor = 1;
	//p->m_TeeInfos.m_ColorBody = 35435;
	//p->m_TeeInfos.m_ColorFeet = 433454;

	Server()->SetClientName(DummyID, "PAYLOAD");
	str_copy(p->m_TeeInfos.m_SkinName, "redbopp", 10);
	p->Respawn();
}

bool CGameControllerMOD::Init(CTile *pTiles, int Width, int Height)
{
	D("Init(%p, %d, %d) called", pTiles, Width, Height);
	if (!InitPath(pTiles, Width, Height))
		return false;

	m_TickSelect[GAMESTATE_IDLE] = &CGameControllerMOD::IdleTick;
	m_TickSelect[GAMESTATE_PREPARE] = &CGameControllerMOD::PrepTick;
	m_TickSelect[GAMESTATE_FIGHT] = &CGameControllerMOD::FightTick;
	m_TickSelect[GAMESTATE_END] = &CGameControllerMOD::EndTick;

	return true;
}

bool CGameControllerMOD::CanSpawnPl(class CPlayer *pP, vec2 *pPos)
{
	if (pP->GetCID() == MAX_CLIENTS - 1) {
		if (pPos && m_LastCheckNode)
			*pPos = m_LastCheckNode->LocVec();
		return m_LastCheckNode;
	} else {
		return IGameController::CanSpawn(pP->GetTeam(), pPos);
	}
	
}

CPathNode::CPathNode(int LocX, int LocY, bool IsCheck, CPathNode *pNext, CPathNode *pPrev) :
	m_Next(pNext), m_Prev(pPrev), m_Check(IsCheck), m_DoneFlag(false)
{
	m_Loc[0] = LocX;
	m_Loc[1] = LocY;
}

bool CGameControllerMOD::CollectNodes(CTile *pTiles, int Width, int Height, CPathNode **ppStartNode, CPathNode **ppFinNode, 
		std::set<CPathNode*>& rTweenerNodes, std::set<int*>& rNCHints)
{
	int *tmp;
	*ppStartNode = *ppFinNode = NULL;
	for (int y = 0; y < Height; ++y) {
		for (int x = 0; x < Width; ++x) {
			int Index = pTiles[y * Width + x].m_Index;
			//vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);

			if (Index == TILE_PATH_START) {
				if (ppStartNode)
					*ppStartNode = new CPathNode(x, y, true);
			} else if (Index == TILE_PATH_FIN) {
				if (ppFinNode)
					*ppFinNode = new CPathNode(x, y, true);
			} else if (Index == TILE_PATH_VERTEX || Index == TILE_PATH_CHECK) {
				rTweenerNodes.insert(new CPathNode(x, y, Index == TILE_PATH_CHECK));
			} else if (Index == TILE_PATH_NOTCON) {
				tmp = new int[2];
				tmp[0] = x;
				tmp[1] = y;
				rNCHints.insert(tmp);
			}

		}
	}
	return *ppStartNode && *ppFinNode;
}

#define NPN_NOREMAIN -1

bool CGameControllerMOD::InitPath(CTile *pTiles, int Width, int Height)
{
	CPathNode *StartNode, *FinNode;
	std::set<CPathNode*> AllNodes;
	std::set<int*> NoConnectHints; /* holds pairs of ints */

	if (!CollectNodes(pTiles, Width, Height, &StartNode, &FinNode, AllNodes, NoConnectHints)) {
		D("CollectNodes failed, will cause InitPath to fail as well");
		return false;
	}
	D("we got start-, end-, and %d tweener nodes; %d nc hints", AllNodes.size(),
			NoConnectHints.size());

	/*add start and fin node to node set to have really all nodes in there*/
	AllNodes.insert(StartNode);
	AllNodes.insert(FinNode);

	/* clear all doneflags, FindNextPathNode() */
	forset(&AllNodes, CPathNode*, it)
		(*it)->MarkDone(false);

	CPathNode *CurNode = StartNode, *PrevNode = NULL;
	size_t NodeCount = 1;

	D("start: (%d, %d)", CurNode->LocX(), CurNode->LocY());
	while (CurNode != FinNode) {
		CurNode->SetPrev(PrevNode);
		if (PrevNode)
			PrevNode->SetNext(CurNode);

		if (!FindNextPathNode(&CurNode, PrevNode = CurNode, AllNodes, NoConnectHints)) {
			D("failed to find the next path vertex, you might probably have to RTFM");
			return false; //dont care about allocated mem, we fail to start anyway
		}
		D("next: (%d, %d)%s", CurNode->LocX(), CurNode->LocY(), CurNode->IsCheckpoint()?" (chk)":"");
		++NodeCount;
	}
	CurNode->SetPrev(PrevNode);
	PrevNode->SetNext(CurNode);

	D("done (%d/%d)", NodeCount, AllNodes.size());

	if (NodeCount < AllNodes.size())
		D("warning: not all path vertices are actually being used, fix your payload path");

	m_LastNode = m_LastCheckNode = StartNode;
/*
	int CheckCount = 0;

	CurNode = StartNode;
	do {
		if (CurNode->IsCheckpoint()) 
			++CheckCount;
	} while((CurNode = CurNode->Next()));

	int *CheckDists = new int[CheckCount]; // 0: distance from 0 to A, 1: distance from A to B, ..., last: distance from last checkpoint to dest

	CurNode = StartNode;
	CheckCount = 0;
	int Index = 0;
	while((CurNode = CurNode->Next())) {
		if (CurNode->IsCheckpoint()) 
			break;
	}
	if (!CurNode || !CurNode->HasNext()) {
		D("payload path needs at least one checkpoint. cannot start like this.");
		return false;
	}

	//CurNode is now at first checkpoint aka '0'
	int DistAccum = 0;
	CPathNode *LastNode = CurNode;
	while((CurNode = CurNode->Next())) {
		DistAccum += Distance(LastNode, CurNode);
		
		if (CurNode->IsCheckpoint() || !CurNode->HasNext()) {
			CheckDists[Index++] = DistAccum;
			D("CheckDists[%d] := %d (node: %d,%d)", Index-1, DistAccum, CurNode->LocX(), CurNode->LocY());
		}
		
	}
*/
	return true;
}

int CGameControllerMOD::Distance(const CPathNode *NodeA, const CPathNode *NodeB)
{
	return max<int> (absolute<int> (NodeA->LocX() - NodeB->LocX()), absolute<int> (NodeA->LocY() - NodeB->LocY()));
}

int CGameControllerMOD::ShiftVal(const CPathNode *NodeA, const CPathNode *NodeB)
{
	return min<int> (absolute<int> (NodeA->LocX() - NodeB->LocX()), absolute<int> (NodeA->LocY() - NodeB->LocY()));
}

bool CGameControllerMOD::NodesConnectable(const CPathNode *NodeA, const CPathNode *NodeB, const std::set<int*>& NCHints)
{
	int Dist = Distance(NodeA, NodeB);
	int Shift = ShiftVal(NodeA, NodeB);

	//D("nodes (%d, %d) and (%d, %d) connectable? dist: %d, shift: %d",NodeA->LocX(),NodeA->LocY(),NodeB->LocX(),NodeB->LocY(),Dist,Shift);

	if (Shift != 0 && Shift != Dist) {
		//D("yes, always.");
		return true; /* always connectable since we dont have a 'real' straight or 45 deg line inbetween*/
	}

	int XStep = 0, YStep = 0;
	if (NodeA->LocX() != NodeB->LocX())
		XStep = (NodeA->LocX() > NodeB->LocX()) ? -1 : 1;
	if (NodeA->LocY() != NodeB->LocY())
		YStep = (NodeA->LocY() > NodeB->LocY()) ? -1 : 1;

	//D("xstep: %i, ystep: %i",XStep, YStep);


	/* this is not exactly the most performant approach, but it is only done once, and anyways,
	 * both loops, inner and outer, will probably have very few iterations for sane maps */
	for (int x = NodeA->LocX() + XStep, y = NodeA->LocY() + YStep; x != NodeB->LocX() || y != NodeB->LocY(); x += XStep, y += YStep) {
		//D("iter: x: %d, y: %d",x, y);
		forset(&NCHints,int*, it)
			if ((*it)[0] == x && (*it)[1] == y) {
				//D("nodes (%d, %d) and (%d, %d) [dist: %d, shift: %d] not connectable due to (%d, %d)",
						//NodeA->LocX(), NodeA->LocY(), NodeB->LocX(), NodeB->LocY(),Dist, Shift, x, y);
				return false;
			}
	}
	//D("not found a nc hint, nodes are connectable");
	return true;
}

bool CGameControllerMOD::FindNextPathNode(CPathNode **Dest, const CPathNode *CurNode, const std::set<CPathNode*>& AllNodes, 
		const std::set<int*>& NCHints)
{
	std::set<CPathNode*> Remain;

	/* exclude all already processed nodes, and those explicitly declared as not connected by NCHints */
	forset(&AllNodes,CPathNode*,it)
		if (!(*it)->IsDone() && (*it) != CurNode && NodesConnectable(CurNode, *it, NCHints))
			Remain.insert(*it);

	int CurX = CurNode->LocX(), CurY = CurNode->LocY();

	//D("want to find the next node for (%d, %d), considering %d remaining nodes",CurX,CurY, Remain.size());

	/* find the closest node(s) */
	std::set<CPathNode*> Proxim;
	int MinDist = -1, MinShift = -1;
	int Dist;

	forset(&Remain,CPathNode*,it) {
		if ((Dist = Distance(CurNode, *it)) < MinDist || MinDist < 0) {
			/* found a closer node */
			Proxim.clear();
			Proxim.insert(*it);
			MinShift = ShiftVal(CurNode, *it);
			//D("node (%d, %d) is closer than what we saw before (new dist/shift: %d/%d, prev minimum: %d) purging proxim set, inserting",
					//(*it)->LocX(),(*it)->LocY(), Dist, MinShift, MinDist);
			MinDist = Dist;
		} else if (Dist == MinDist) {
			int Shift = ShiftVal(CurNode, *it);
			if (Shift < MinShift) {
				/* found a equally close, but more straightly connectable node */

				//D("node (%d, %d) is equally close as we saw before, but has less shift (%d, prev: %d) purging proxim set, inserting",
						//(*it)->LocX(),(*it)->LocY(), Shift, MinShift);
				Proxim.clear();
				Proxim.insert(*it);
				MinShift = Shift;
			} else if (Shift == MinShift) {
				//D("node (%d, %d) is just equal to our best (dist/shift: %d/%d), inserting",
						//(*it)->LocX(),(*it)->LocY(), MinDist, MinShift);
				Proxim.insert(*it);
			}
		}
	}

	//D("%d proximity candidates",Proxim.size());

	if (Proxim.size() >= 2) {
		D("ambigous payload path: offending node: (%d, %d); %d candidates:", CurX, CurY,
				Proxim.size());
		forset(&Proxim,CPathNode*,it)
			D("\tcand: successor node at (%d, %d)", (*it)->LocX(), (*it)->LocY());
		/* we're gonna pick whichever our std::set implementation gives us, in this case */
	}

	if (Proxim.empty())
		return false;

	if (Proxim.size() >= 1)
		*Dest = *(Proxim.begin());

	Proxim.clear();

	CurNode->MarkDone(true);

	return true;
}

bool CGameControllerMOD::StartPrepare()
{
	if (m_GameState != GAMESTATE_IDLE)
		return false;
	m_GameState = GAMESTATE_PREPARE;
	StartRound();
	return true;
}
bool CGameControllerMOD::StartFight()
{
	if (m_GameState != GAMESTATE_PREPARE)
		return false;
	m_GameState = GAMESTATE_FIGHT;
	return true;
}

void CGameControllerMOD::Reset()
{
	m_GameState = GAMESTATE_IDLE;
	m_LastBroadcast = 0;
	m_PayloadDir = HALT;
	if (m_LastNode)
		while (m_LastNode->Prev())
			m_LastNode = m_LastNode->Prev();
	m_LastCheckNode = m_LastNode;
	m_HaltTicks = 0;
	UpdateBroadcast(-1, IDLE_MSG);
	ResetGame(); //XXX
}

/* these are the state transitions we're going to support
 ANY       -> IDLE
 IDLE, END -> PREPARE
 FIGHT     -> END
 PREPARE   -> FIGHT */
bool CGameControllerMOD::ChangeState(int NewState)
{
	if (m_GameState != GAMESTATE_IDLE && NewState == GAMESTATE_IDLE) {
		Reset();
	} else if (m_GameState == GAMESTATE_IDLE && NewState == GAMESTATE_PREPARE) {
		StartPrepare();
	} else if (m_GameState == GAMESTATE_PREPARE && NewState == GAMESTATE_FIGHT) {
		StartFight();
	} else if (m_GameState == GAMESTATE_FIGHT && NewState == GAMESTATE_END) {
		/* show scoreboard, give winner opportunity to taunt, w/e */
		D("you still haven't implemented anything for the fight->end transition dude");
		m_GameState = GAMESTATE_END;//XXX
	} else if (m_GameState == GAMESTATE_END && NewState == GAMESTATE_PREPARE) {
		/* maybe we should just enforce a idle transition here */
		Reset();
		StartPrepare();
	} else if (NewState != m_GameState) {
		D("insane state transition from %i to %i issued. won't.", m_GameState, NewState);
	}
	
	return m_GameState == NewState;
}

