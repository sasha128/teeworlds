/*
 * Team.cpp
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#include <map>

#include <cstdio>

#include "Player.h"
#include "Team.h"

namespace tour
{

    Team::Team(int id) :
        _id(id), _membs(), _noobstyle(0)
    {
    }

    Team::~Team()
    {
        _membs.clear();
    }

    bool Team::addMemb(class Player *p)
    {
        if (_membs.count(p->id())) return false;
        return (_membs[p->id()] = p);
    }

    void Team::dump()
    {
        fprintf(stderr, "\tteam id %i, noobstyle: %i, membs: ", _id, _noobstyle);
        for (std::map<int, Player*>::const_iterator it = _membs.begin(); it != _membs.end(); ++it) {
            fprintf(stderr, "%i, ", it->first);
        }
        fprintf(stderr, "\n");
    }

}
