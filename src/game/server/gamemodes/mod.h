/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H
#include <game/server/gamecontroller.h>

namespace tour
{
    class Tournament;
    class Lineup;
}

class CGameControllerMOD: public IGameController
{
    private:
        class tour::Tournament *_tour;
        class tour::Lineup *_lineUp;
        int _announceOverride;
        char *_announceOverrideMsg;
    public:
	CGameControllerMOD(class CGameContext *pGameServer);

        virtual ~CGameControllerMOD();

	virtual void Tick();

        void forceAnnounce(const char *msg, int ticks);
        void doAnnounce() const;
        void onEnter(int cid) const;
        void onLeave(int cid) const;
        void onFreeze(int cid) const;
        void onUnfreeze(int cid) const;
        void onFail(int failpid, int killpid) const;
        bool onTeamSelect(int cid, int team) const;
        void onNoobStyle(int cid) const;

        bool isTourRunning() const;
        bool isTeamSelect() const;

        void dump() const;
};
#endif

