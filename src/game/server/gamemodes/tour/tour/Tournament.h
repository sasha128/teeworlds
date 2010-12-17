/*
 * Tournament.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#ifndef TOURNAMENT_H_
#define TOURNAMENT_H_

namespace tour
{

    class Tournament
    {
        public:
            Tournament();
            virtual ~Tournament();

            virtual void reset() = 0;
            virtual bool start(class Lineup*) = 0;
            virtual void tick() = 0;
            virtual void rankPort() const = 0;
            virtual bool hasStarted() const = 0;
            virtual bool hasFinished() const = 0;
            virtual int getPlayerMsg(char *dest, int cid) const = 0;
            virtual void onNoobStyle(int cid) = 0;
            virtual void onLeave(int cid) = 0;
            virtual void onFreeze(int cid) = 0;
            virtual void onUnfreeze(int cid) = 0;
            virtual void onFail(int failpid, int killpid) = 0;
            virtual void dump() = 0;
    };

}

#endif /* TOURNAMENT_H_ */
