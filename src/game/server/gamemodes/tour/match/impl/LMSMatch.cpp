/*
 * LMSMatch.cpp
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#include <cstring>

#include "../../common.h"
#include "../../Team.h"
#include "../../Player.h"
#include "../../Util.h"

#include "LMSMatch.h"

#define forvec(VEC,K,IT) for(std::vector<K>::const_iterator IT=(VEC)->begin();IT != (VEC)->end();)
#define forvec_auto(VEC,K,IT) for(std::vector<K>::const_iterator IT=(VEC)->begin();IT != (VEC)->end();++IT)
#define formap(MAP,K,V,IT) for(std::map<K,V>::const_iterator IT=(MAP)->begin();IT != (MAP)->end();)
#define formap_auto(MAP,K,V,IT) for(std::map<K,V>::const_iterator IT=(MAP)->begin();IT != (MAP)->end();++IT)

#define forteams(IT) formap(&_teams,int,Team*,IT)
#define forteams_auto(IT) formap_auto(&_teams,int,Team*,IT)

namespace tour
{

    LMSMatch::LMSMatch() :
        _teams(), _teaminfo(), _started(false), _finished(false), _arena(-1), _losers(), _winner(-1), _failLimit(
                Util::getLMSMatchFailLimit()), _lastLeadTid(-1), _tmp_orderc(0)
    {
    }

    LMSMatch::~LMSMatch()
    {
        _losers.clear();
        _teams.clear();
        _teaminfo.clear();
    }

    bool LMSMatch::start(int inarena)
    {
        if (_started) return false;
        _arena = inarena;
        _timeleft = Util::getLMSMatchMaxTime();
        allToSpawn();
        return _started = true;
    }

    void LMSMatch::tick()
    {
        if (!_started || _finished) return;
        if (countTeams() >= 2) {
            Player *failed_player = NULL;
            forteams_auto(it_teams) {
                Team *team = it_teams->second;
                for (std::map<int, Player*>::const_iterator it_players = team->getMembIter(true); it_players != team->getMembIter(false); ++it_players)
                    if (it_players->second->getFailFlag()) (failed_player = it_players->second)->setFailFlag(false);
            }
            --_timeleft;
            if (failed_player) {
                DV("player %i failed",failed_player->id());
                if (++(_teaminfo[failed_player->getTeam()->id()]->fail) >= _failLimit) {
                    SV("faillimit hit");
                    dropTeam(failed_player->getTeam()->id());
                    _losers.push_back(failed_player->getTeam()->id());
                    Util::unfreezeTeam(failed_player->getTeam());
                    Util::teleportTeam(failed_player->getTeam(), Util::getPosLinger());
                }
                int minfail = -1;
                forteams_auto(it_teams)
                    if (minfail < 0 || _teaminfo[it_teams->first]->fail < minfail)
                        minfail = _teaminfo[it_teams->first]->fail;
                int minfail_count = 0, minfail_tid=-1;
                forteams_auto(it_teams) {
                    if (_teaminfo[it_teams->first]->fail == minfail) {
                        minfail_tid = it_teams->first;
                        if (++minfail_count == 2) break;
                    }
                }
                if (minfail_count == 1) _lastLeadTid = minfail_tid;
                allToSpawn();
            } else if (_timeleft <= 0) {
                DV("match %p timeout, determining winner",this);
                int minfail = -1;
                forteams_auto(it_teams)
                    if (minfail < 0 || _teaminfo[it_teams->first]->fail < minfail)
                        minfail = _teaminfo[it_teams->first]->fail;
                std::vector<int> minfail_vec;
                forteams_auto(it_teams)
                    if (_teaminfo[it_teams->first]->fail == minfail) minfail_vec.push_back(it_teams->first);
                int win_tid = -1;
                if (minfail == 0) {
                    if (minfail_vec.size() >= 2) { //determine winner by comparing noobstyle
                        SV("by noobstyle");
                        int minnoob = -1;
                        int minnoob_tid = -1;
                        forvec_auto(&minfail_vec,int,it_minfail) {
                            Team *team = _teams[*it_minfail];
                            if (minnoob == -1 || team->getNoobStyle() < minnoob) {
                                minnoob = team->getNoobStyle();
                                minnoob_tid = team->id();
                            }
                        }
                        win_tid = minnoob_tid;
                    } else {
                        win_tid = minfail_vec.front();
                    }
                } else {
                    if (minfail_vec.size() >= 2 && _teams.count(_lastLeadTid)) { //determine winner by checking history
                        SV("by history");
                        win_tid = _lastLeadTid;
                    } else {
                        win_tid = minfail_vec.front();
                    }
                }
                DV("determined winner: %i",win_tid);
                forteams_auto(it_teams) {
                    if (it_teams->first != win_tid) {
                        _losers.push_back(it_teams->first);
                        Util::unfreezeTeam(it_teams->second);
                        Util::teleportTeam(it_teams->second, Util::getPosLinger());
                    }
                }
                forvec_auto(&_losers,int,it_losers) dropTeam(*it_losers);
            }
        }
        int num_teams;
        if ((num_teams = countTeams()) < 2) {
            DV("only %i teams remaining",num_teams);
            if (num_teams == 1) {
                _winner = _teams.begin()->first;
                DV("winner: %i",_winner);
                Util::unfreezeTeam(_teams.begin()->second);
                Util::teleportTeam(_teams.begin()->second, Util::getPosLinger());
                dropTeam(_winner);
            }
            SV("marking match as finished");
            _finished = true;
        }
    }

    void LMSMatch::addTeam(class Team *t)
    {
        _teams[t->id()] = t;
        _teaminfo[t->id()] = new TeamInfo(t, _tmp_orderc++);
    }

    int LMSMatch::countTeams() const
    {
        return _teams.size();
    }

    void LMSMatch::dropTeam(int tid)
    {
        if (_teams.count(tid)) {
            DV("dropping team %i from match %p",tid,this);
            _teams.erase(tid);
            delete _teaminfo[tid];
            _teaminfo.erase(tid);
        }
    }

    bool LMSMatch::hasTeam(int tid)
    {
        return _teams.count(tid);
    }

    bool LMSMatch::hasStarted() const
    {
        return _started;
    }

    bool LMSMatch::hasFinished() const
    {
        return _finished;
    }

    int LMSMatch::getWinner(int *dest, int max) const
    {
        if (max > 0) *dest = _winner;
        return max > 0 ? 1 : 0;
    }

    int LMSMatch::getLoser(int *dest, int max) const
    {
        int num = _losers.size();
        if (num > max) num = max;
        for (int z = 0; z < num; ++z) dest[z] = _losers[z];
        return num;
    }

    //just make sure its long enough
    int LMSMatch::getTeamMsg(char *dest, Team* t) const
    {
        static char tmp[64];
        if (!_started) {
            strcpy(dest, "waiting for opponent(s)");
        } else {
            if (!_finished) {
                sprintf(dest, "lives: %i", _failLimit - _teaminfo.find(t->id())->second->fail);
                forteams_auto(it_teams) {
                    if (it_teams->first != t->id()) {
                        sprintf(tmp, " - %i", _failLimit - _teaminfo.find(it_teams->first)->second->fail);
                        strcat(dest, tmp);
                    }
                }
                sprintf(tmp, ", time left: %i", _timeleft / Util::getTickSpeed());
                strcat(dest, tmp);
            } else {
                strcpy(dest, "match finished"); //?
            }
        }
        return strlen(dest);
    }

    void LMSMatch::dump()
    {
        fprintf(stderr, "\tmatch (addr %p), started: %i, finished: %i, arena: %i, teams:\n", this, _started, _finished, _arena);
        forteams_auto(it_teams) {
            TeamInfo *team_info = _teaminfo[it_teams->first];
            fprintf(stderr, "\t\t\tteam %i, order %i, fail %i\n", it_teams->first, team_info->order, team_info->fail);
        }
        fprintf(stderr, "\t\ttimeleft: %i, lltid: %i, winner: %i, faillimit: %i, losers: ", _timeleft, _lastLeadTid, _winner, _failLimit);
        forvec_auto(&_losers,int,it_losers)
            fprintf(stderr, "%i,", *it_losers);
        fprintf(stderr, "\n");
    }

    void LMSMatch::allToSpawn() const
    {
        DV("all to spawn (%p)",this);
        forteams_auto(it_teams) {
            Team *team = it_teams->second;
            Util::unfreezeTeam(team);
            Util::freezeTeam(team, Util::getStartFreezeTime());
            Util::toArena(team,_arena, _teaminfo.find(team->id())->second->order);
        }
    }

}
