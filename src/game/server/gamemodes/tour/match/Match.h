/*
 * Match.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#ifndef MATCH_H_
#define MATCH_H_

namespace tour
{

    class Match
    {
        public:
            Match();
            virtual ~Match();

            virtual void tick() = 0;

            //add a team to match, only before the match has begun
            virtual void addTeam(class Team *t) = 0;
            virtual int countTeams() const = 0;
            //drop a team from match, at any time
            virtual void dropTeam(int tid) = 0;
            virtual bool hasTeam(int tid) = 0;

            virtual bool hasStarted() const = 0;
            virtual bool hasFinished() const = 0;

            virtual bool start(int arena) = 0;

            //can in theory be called before match finishs (but not before it starts)
            virtual int getWinner(int *dest, int max) const = 0;
            virtual int getLoser(int *dest, int max) const = 0;

            virtual int getTeamMsg(char *dest, Team* t) const = 0;

            virtual void dump() = 0;
    };

}

#endif /* MATCH_H_ */
