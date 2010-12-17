/*
 * Lineup.cpp
 *
 *  Created on: Oct 13, 2010
 *      Author: fisted
 */

#include <set>
#include "Lineup.h"

namespace tour
{

    Lineup::Lineup()
    {
        _numDTeams = -1;
        _numPlayers = 0;
        for (int z = 0; z < MAX_CLIENTS; ++z) {
            _teamSelection[z] = TEAMSELECT_INVALID;
        }
    }

    Lineup::~Lineup()
    {
    }

    void Lineup::teamselect(int cid, int tid)
    {
        if (_teamSelection[cid] == tid) return;
        if (tid == TEAMSELECT_INVALID) {
            _numDTeams = -1;
            D("player: %i left",cid);
            --_numPlayers;
        } else {
            if (_teamSelection[cid] == TEAMSELECT_INVALID) {
                D("player: %i entered",cid);
                ++_numPlayers;
            } else {
                D("player: %i selected team %i",cid,tid);
            }
            _numDTeams = -1;
        }
        _teamSelection[cid] = tid;
    }
    int Lineup::countDistinctTeams() const
    {
        return (_numDTeams < 0) ? _numDTeams = actuallyCountDistinctTeams() : _numDTeams;
    }
    int Lineup::actuallyCountDistinctTeams() const
    {
        static std::set<int> tidused;
        tidused.clear();
        int solo = 0;
        for (int z = 0; z < MAX_CLIENTS; ++z) {
            if (_teamSelection[z] >= 0) {
                tidused.insert(_teamSelection[z]);
            } else if (_teamSelection[z] == TEAMSELECT_SOLO) {
                ++solo;
            }
        }
        return tidused.size() + solo;
    }
}
