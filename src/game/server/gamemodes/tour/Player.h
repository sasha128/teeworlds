/*
 * Player.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#ifndef PLAYER_H_
#define PLAYER_H_

namespace tour
{

    class Player
    {
        private:
            const int _id;
            class Team* _team;
            bool _failflag;
        public:
            Player(int id);
            virtual ~Player();

            int id() const              { return _id; }
            class Team* getTeam() const { return _team; }
            void setTeam(class Team* t) { _team=t; }
            bool getFailFlag() const    { return _failflag; }
            void setFailFlag(bool b)    { _failflag=b; }

            virtual void dump();
    };

}

#endif /* PLAYER_H_ */
