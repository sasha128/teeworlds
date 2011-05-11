#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <set>

#include <engine/shared/protocol.h>

#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/tdm.h>


class CPathNode;
class CGameControllerMOD;

typedef void (CGameControllerMOD::*tickfunc_t)();

class CGameControllerMOD : public CGameControllerTDM
{
	public:
		enum EGameState { GAMESTATE_IDLE, GAMESTATE_PREPARE, GAMESTATE_FIGHT, GAMESTATE_END };
		enum EPayloadDir { BACK = -1 , HALT,  FORE };




	private:
		EGameState m_GameState;

		int        m_LastBroadcast;
		char      *m_apBroadcasts[MAX_CLIENTS];

		CPathNode *m_LastCheckNode;
		CPathNode *m_LastNode;

		EPayloadDir m_PayloadDir;

		int m_HaltTicks;

		void (CGameControllerMOD::*m_TickSelect[4])();

		bool ChangeState(int NewState);
		void Reset();
		bool StartPrepare();
		bool StartFight();
		void UpdateBroadcast(int cid, const char *str, size_t len = 0); // 0 meaning no limit
		void Broadcast();
		void FightBroadcast();

		bool InitPath(CTile *pTiles, int Width, int Height);
		bool CollectNodes(CTile *pTiles, int Width, int Height, CPathNode **ppStartNode, CPathNode **ppFinNode,
				std::set<CPathNode*>& rTweenerNodes, std::set<int*>& rNCHints);


		bool FindNextPathNode(CPathNode **Dest, const CPathNode *CurNode,
				const std::set<CPathNode*>& AllNodes, const std::set<int*>& NCHints);

		bool NodesConnectable(const CPathNode *NodeA, const CPathNode *NodeB, const std::set<int*>& NCHints);

		void InitDummy();
		vec2 GetMoveDirection();

		int Distance(const CPathNode *NodeA, const CPathNode *NodeB);
		int ShiftVal(const CPathNode *NodeA, const CPathNode *NodeB);

	public:
	CGameControllerMOD(class CGameContext *pGameServer);
	virtual void Tick();
		virtual void IdleTick();
		virtual void PrepTick();
		virtual void HandlePayload();
		virtual void FightTick();
		virtual void EndTick();

		virtual bool Init(CTile *pTiles, int Width, int Height);

		virtual bool CanSpawn(class CPlayer *pP, vec2 *pPos);

};

class CPathNode {
	public:
		enum EPathType { PT_START, PT_POINT, PT_CHECKPOINT, PT_FIN };
	private:
		int m_Loc[2]; // unit is tiles
		class CPathNode *m_Next;
		class CPathNode *m_Prev;
		bool m_Check;
		mutable bool m_DoneFlag;// used only while constructing the path



	public:
		CPathNode(int LocX, int LocY, bool IsCheck, CPathNode *pNext = NULL, CPathNode *pPrev = NULL);
		CPathNode *Next() const            { return m_Next; }
		CPathNode *Prev() const            { return m_Prev; }
		bool HasNext() const               { return m_Next; }
		bool HasPrev() const               { return m_Prev; }
		void SetNext(CPathNode *Next)      { m_Next = Next; }
		void SetPrev(CPathNode *Prev)      { m_Prev = Prev; }
		bool IsCheckpoint() const          { return m_Check; }
		void SetCheckpoint(bool CanHazChk) { m_Check = CanHazChk; }
		int  LocX() const                  { return m_Loc[0]; }
		int  LocY() const                  { return m_Loc[1]; }
		vec2 LocVec() const                { return vec2(m_Loc[0]*32.0f+16.0f, m_Loc[1]*32.0f+16.0f); }

		bool IsDone() const                { return m_DoneFlag; }
		void MarkDone(bool d) const        { m_DoneFlag = d; }

	};
#endif
