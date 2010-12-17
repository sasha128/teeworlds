/*
 * Player.cpp
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#include <cstdio>

#include "Team.h"
#include "Player.h"

namespace tour
{

    Player::Player(int id) :
        _id(id), _team(0), _failflag(false)
    {
    }

    Player::~Player()
    {
    }

    void Player::dump()
    {
        fprintf(stderr, "\tplayer id %i, team %i, failflag: %i\n", _id, _team ? _team->id() : -1, _failflag);
    }

}
