#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>

class CGameControllerMOD : public IGameController
{
private:
	// extended char information
	struct CCharExt
	{
		bool m_Valid;          // will be redetermined on every tick, true means active player with alive char
		int m_OwnedHill;       // the hill the player currently owns, -1 for none.
		bool m_IsLosing;       // are we currently losing our m_OwnedHill?
		int m_LoseCooldown;    // counts from sv_lose_delay towards zero, when losing a hill. on zero the hill is lost.
		int m_AcquiringHill;   // what hill are we gonna acquire? needed for maps with no gaps between different hills
		int m_AcquireCooldown; // same as m_LoseCooldown, just for acquisition
		int m_EnterTick;       // when did we last enter (step into, that is, not acquire) a hill
		int m_LastScoreTick;   // when did we last gain some score for our ownage
	};

	struct CCharExt m_aCharExt[MAX_CLIENTS];
	struct CCharExt m_aCharExtPrev[MAX_CLIENTS];

	int m_LastBroadcast;

	void DoHillOwnage();
	void DoStars();
	void DoBroadcasts();
	void DoScoring();
	void DoEmotes();
	void Announce(const char *pFormat, ...);
	int GetScoreForHill(int Hid);
public:
	CGameControllerMOD(class CGameContext *pGameServer);
	virtual ~CGameControllerMOD();

	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

	virtual void Tick();
};

#endif
