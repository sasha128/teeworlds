/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <cstdlib>
#include <csignal>
#include <cstring>
#include <ctime>

#include <engine/shared/config.h>
#include <engine/server.h>

#include <game/server/gamecontext.h>

#include "tour/Util.h"
#include "tour/Factory.h"
#include "tour/tour/Tournament.h"
#include "tour/tour/impl/SingleElimTournament.h"
#include "tour/Lineup.h"

#include "mod.h"

CGameControllerMOD* modinst;
void segvhnd(int sig);
void init_once(CGameControllerMOD*m);

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer) :
    IGameController(pGameServer), _tour(NULL), _lineUp(NULL), _announceOverride(0), _announceOverrideMsg(NULL)
{
	m_pGameType = "Tournament";
	init_once(this);
	tour::Util::init();
	tour::Factory::init();
	_lineUp = new tour::Lineup;
	_tour = tour::Factory::allocTournament();
	srand(time(NULL));
}

CGameControllerMOD::~CGameControllerMOD()
{
	delete _lineUp;
}

void CGameControllerMOD::doAnnounce() const
{
    static char announcebuf[256];
    if (_announceOverride > 0) {
        strcpy(announcebuf, _announceOverrideMsg);
    }
    for (int z = 0; z < MAX_CLIENTS; ++z) {
        if (GameServer()->m_apPlayers[z]) {
            if (_announceOverride <= 0) {
                _tour->getPlayerMsg(announcebuf, z);
            }
            if (strlen(announcebuf) > 0) {
                GameServer()->SendBroadcast(announcebuf, z);
            }
        }
    }
}

void CGameControllerMOD::forceAnnounce(const char *msg, int ticks)
{
    _announceOverride = ticks;
    if (_announceOverrideMsg) free(_announceOverrideMsg);
    _announceOverrideMsg = strdup(msg);
}
       
void CGameControllerMOD::Tick()
{
    static int announcectr = Server()->TickSpeed();
    static bool rankport = false;
    static int tickdelay = 0;
    IGameController::Tick();
    --_announceOverride;
    if (--announcectr <= 0) {
        doAnnounce();
        announcectr = Server()->TickSpeed();
    }
    if (--tickdelay > 0) return;
    if (_tour->hasStarted()) {
        if (_tour->hasFinished()) {
            if (!rankport) {
                tour::Util::teleportAll(tour::Util::getPosUpcoming());
                _tour->rankPort();
                tickdelay = g_Config.m_SvTourFinDelay * Server()->TickSpeed();
                rankport = true;
            } else {
                _tour->reset();
                tickdelay = g_Config.m_SvTourStartDelay * Server()->TickSpeed();
                rankport = false;
                tour::Util::teleportAll(tour::Util::getPosSpawn());
            }
        } else {
            _tour->tick();
        }
    } else {
        if (!_tour->start(_lineUp)) {
            tickdelay = g_Config.m_SvTourStartDelay * Server()->TickSpeed();
        }
    }
}

void CGameControllerMOD::onEnter(int cid) const
{
    _lineUp->teamselect(cid, TEAMSELECT_SOLO);
}
void CGameControllerMOD::onLeave(int cid) const
{
    _lineUp->teamselect(cid, TEAMSELECT_INVALID);
    if (_tour->hasStarted() && !_tour->hasFinished()) {
        S("propagating leave event");
        _tour->onLeave(cid);
    }
}

bool CGameControllerMOD::onTeamSelect(int cid, int team) const
{
    _lineUp->teamselect(cid, team);
    return !_tour->hasStarted();
}

bool CGameControllerMOD::isTourRunning() const
{
    return _tour->hasStarted() && !_tour->hasFinished();
}
bool CGameControllerMOD::isTeamSelect() const
{
    return !_tour->hasStarted() && !_tour->hasFinished();
}
void CGameControllerMOD::onNoobStyle(int cid) const
{
    if (_tour->hasStarted() && !_tour->hasFinished()) _tour->onNoobStyle(cid);

}
void CGameControllerMOD::onFreeze(int cid) const
{
    if (_tour->hasStarted() && !_tour->hasFinished()) _tour->onFreeze(cid);
}
void CGameControllerMOD::onUnfreeze(int cid) const
{
    if (_tour->hasStarted() && !_tour->hasFinished()) _tour->onUnfreeze(cid);
}
void CGameControllerMOD::onFail(int failpid, int killpid) const
{
    if (_tour->hasStarted() && !_tour->hasFinished()) _tour->onFail(failpid, killpid);
}
void CGameControllerMOD::dump() const
{
    _tour->dump();
}

void init_once(CGameControllerMOD*m)
{
    static int i = 0;
    if (!i) {
        modinst = m;
        ++i;
        signal(SIGSEGV, segvhnd);
    }
}
void segvhnd(int sig)
{
    if (sig == SIGSEGV) {
        S("segfault, dumping");
        modinst->dump();
        sync();
        exit(EXIT_FAILURE);
    }
}

