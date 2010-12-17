/*
 * Lineup.h
 *
 *  Created on: Oct 13, 2010
 *      Author: fisted
 */

#ifndef LINEUP_H_
#define LINEUP_H_

#include <engine/shared/protocol.h>

#include "common.h"

namespace tour
{

    class Lineup
    {
        private:
            int _teamSelection[MAX_CLIENTS];
            int _numPlayers;
            mutable int _numDTeams;

            int actuallyCountDistinctTeams() const;

        public:
            Lineup();
            virtual ~Lineup();

            void teamselect(int cid, int tid);
            int getTeamSelection(int cid) const {return _teamSelection[cid];}
            int countDistinctTeams() const;
            int countPlayers() const            {return _numPlayers;}
    };

}

#endif /* LINEUP_H_ */
