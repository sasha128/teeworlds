/*
 * Util.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#include <vector>
#include <map>

#include <base/vmath.h>

#ifndef UTIL_H_
#define UTIL_H_

namespace tour
{

    class Util
    {
        private:
            static class CGameContext *_gameCtx;
            static int _tickSpeed;

            //this is to be externalized
            static std::map<int, int> *_teamColors;

            static std::map<int, std::map<int, std::vector<vec2>*>*> *_arenae;
            static std::map<int, std::vector<vec2>*> *_ranks;
            static std::vector<vec2> *_linger;
            static std::vector<vec2> *_spawn;
            static std::vector<vec2> *_upcoming;
        public:
            static void init();
            static class CGameContext *getGameCtx();
            static void setGameCtx(class CGameContext *gctx);

            static void unfreezeTeam(class Team *t);
            static void freezeTeam(class Team *t, int ticks);

            static void teleportTeam(const class Team *t, vec2 dest);
            static void toArena(const Team *t, int arena, int order);/* this case needs special care for tee distribution*/
            static void teleportAll(vec2 dest);
            static void teleportPlayer(int cid, vec2 dest);

            static vec2 getPosArena(int arena, int order);
            static vec2 getPosRank(int rank);
            static vec2 getPosSpawn();
            static vec2 getPosLinger();
            static vec2 getPosUpcoming();
            static void regPosArena(int arena, int order, vec2 pos);
            static void regPosRank(int rank, vec2 pos);
            static void regPosSpawn(vec2 pos);
            static void regPosLinger(vec2 pos);
            static void regPosUpcoming(vec2 pos);

            static int getStartFreezeTime();
            static int getLMSMatchFailLimit();
            static int getLMSMatchMaxTime();

            static int getTickSpeed();

            static int getTeamColor(int tid);

            static void shuffleIV(std::vector<int> *vec);
    };

}

#endif /* UTIL_H_ */
