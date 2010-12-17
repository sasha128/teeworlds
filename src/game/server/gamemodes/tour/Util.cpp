/*
 * Util.cpp
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */
#include <set>

#include <cstdlib>

#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "common.h"

#include "Team.h"
#include "Player.h"

#include "Util.h"

namespace tour
{
    CGameContext *Util::_gameCtx = NULL;
    int Util::_tickSpeed = -1;
    std::map<int, int> *Util::_teamColors = NULL;
    std::map<int, std::map<int, std::vector<vec2>*>*> *Util::_arenae = NULL;
    std::map<int, std::vector<vec2>*> *Util::_ranks = NULL;
    std::vector<vec2> *Util::_linger = NULL;
    std::vector<vec2> *Util::_spawn = NULL;
    std::vector<vec2> *Util::_upcoming = NULL;

    void Util::init()
    {
        /*TODO make colors adjustable via config, i leave it as a hardcode for now*/
        _teamColors = new std::map<int, int>;
        (*_teamColors)[0] = 0xa5e500;//blue
        (*_teamColors)[1] = 0xff00;//red
        (*_teamColors)[2] = 0x51ff4d;//green
        (*_teamColors)[3] = 0x2bff00;//yellow
        (*_teamColors)[4] = 0xc3c65b;//purp
        (*_teamColors)[5] = 0x1;//grey
        (*_teamColors)[6] = 0xffffff;//white
        (*_teamColors)[7] = 0x15a618;//brown
    }

    class CGameContext *Util::getGameCtx()
    {
        return _gameCtx;
    }
    void Util::setGameCtx(class CGameContext *gctx)
    {
        _gameCtx = gctx;
    }
    void Util::unfreezeTeam(Team *t)
    {
        for (std::map<int, Player*>::const_iterator it = t->getMembIter(true); it != t->getMembIter(false); ++it) {
            CCharacter *c = _gameCtx->GetPlayerChar(it->second->id());
            if (c) c->Unfreeze();
        }
    }

    void Util::freezeTeam(Team *t, int ticks)
    {
        for (std::map<int, Player*>::const_iterator it = t->getMembIter(true); it != t->getMembIter(false); ++it) {
            CCharacter *c = _gameCtx->GetPlayerChar(it->second->id());
            if (c) c->Freeze(ticks);
        }
    }
    void Util::teleportTeam(const Team *t, vec2 dest)
    {
        for (std::map<int, Player*>::const_iterator it = t->getMembIter(true); it != t->getMembIter(false); ++it)
            teleportPlayer(it->second->id(), dest);
    }

    void Util::toArena(const Team *t, int arena, int order)
    {
        DV("porting team %i (%i membs) to arena %i (order %i)",t->id(), t->countMembs(), arena, order);
        for (std::map<int, Player*>::const_iterator it = t->getMembIter(true); it != t->getMembIter(false); ++it) {
            vec2 dest=getPosArena(arena,order);
            teleportPlayer(it->second->id(), dest);
        }
    }
    void Util::teleportAll(vec2 dest)
    {
        CCharacter *c;
        for (int z = 0; z < MAX_CLIENTS; ++z) {
            if ((c = _gameCtx->GetPlayerChar(z))) c->Teleport(dest);
        }
    }

    void Util::teleportPlayer(int cid, vec2 dest)
    {
        CCharacter *c = _gameCtx->GetPlayerChar(cid);
        if (c) {
            DV("porting player %i to (%f,%f)",cid,dest.x,dest.y);
            c->Teleport(dest);
        }
        else D("teleport player: no char for player id %i",cid);
    }

    vec2 Util::getPosArena(int arena, int order)
    {
        if (_arenae && _arenae->count(arena) && (*_arenae)[arena]->count(order)) {
            int num = (*(*_arenae)[arena])[order]->size();
            return (*(*(*_arenae)[arena])[order])[rand() % num];
        }
        return vec2(0, 0);
    }
    vec2 Util::getPosRank(int rank)
    {
        if (_ranks && _ranks->count(rank)) {
            int num = (*_ranks)[rank]->size();
            return (*(*_ranks)[rank])[rand() % num];
        }
        return vec2(0, 0);
    }
    vec2 Util::getPosSpawn()
    {
        if (_spawn) {
            int num = _spawn->size();
            return (*_spawn)[rand() % num];
        }
        return vec2(0, 0);
    }
    vec2 Util::getPosLinger()
    {
        if (_linger) {
            int num = _linger->size();
            return (*_linger)[rand() % num];
        }
        return vec2(0, 0);
    }
    vec2 Util::getPosUpcoming()
    {
        if (_upcoming) {
            int num = _upcoming->size();
            return (*_upcoming)[rand() % num];
        }
        return vec2(0, 0);
    }

    void Util::regPosArena(int arena, int order, vec2 pos)
    {
        if (!_arenae) _arenae = new std::map<int, std::map<int, std::vector<vec2>*>*>;
        if (!_arenae->count(arena)) (*_arenae)[arena] = new std::map<int, std::vector<vec2>*>;
        if (!((*_arenae)[arena]->count(order))) (*(*_arenae)[arena])[order] = new std::vector<vec2>;
        (*(*_arenae)[arena])[order]->push_back(pos);
    }
    void Util::regPosRank(int rank, vec2 pos)//ranks start with 1
    {
        if (!_ranks) _ranks = new std::map<int, std::vector<vec2>*>;
        if (!_ranks->count(rank)) (*_ranks)[rank] = new std::vector<vec2>;
        (*_ranks)[rank]->push_back(pos);
    }
    void Util::regPosSpawn(vec2 pos)
    {
        if (!_spawn) _spawn = new std::vector<vec2>;
        _spawn->push_back(pos);
    }
    void Util::regPosLinger(vec2 pos)
    {
        if (!_linger) _linger = new std::vector<vec2>;
        _linger->push_back(pos);
    }

    void Util::regPosUpcoming(vec2 pos)
    {
        if (!_upcoming) _upcoming = new std::vector<vec2>;
        _upcoming->push_back(pos);
    }

    int Util::getStartFreezeTime()
    {
        return g_Config.m_SvSpawnFreezeTime * (getTickSpeed() >> 1);
    }
    int Util::getLMSMatchFailLimit()
    {//generalize it baby
        return g_Config.m_SvLMSMatchFailLimit;
    }
    int Util::getLMSMatchMaxTime()
    {
        return g_Config.m_SvLMSMatchMaxTime * getTickSpeed();
    }
    int Util::getTickSpeed()
    {
        if (_tickSpeed < 0) _tickSpeed = _gameCtx->Server()->TickSpeed();
        return _tickSpeed;
    }
    int Util::getTeamColor(int tid)
    {
        return (*_teamColors)[tid];
    }
    /*shuffle an int vector, no templates version :o)*/
    void Util::shuffleIV(std::vector<int> *vec)
    {
        static std::vector<int> workvec;
        static std::set<int> workset;
        workvec.clear();
        workset.clear();
        int num = vec->size();
        int i;

        for (int z = 0; z < num; ++z) {
            while (workset.count(i = rand() % num))
                ;
            workvec.push_back((*vec)[i]);
            workset.insert(i);
        }
        for (int z = 0; z < num; ++z) {
            (*vec)[z] = workvec[z];
        }
    }
}
