/*
 * Team.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#ifndef TEAM_H_
#define TEAM_H_

#include <map>

namespace tour
{

    class Team
    {
        private:
            const int _id;
            std::map<int, class Player*> _membs;
            int _noobstyle;
        public:
            Team(int id);
            virtual ~Team();

            int id() const                   {return _id;}

            bool addMemb(class Player *p);
            int countMembs() const           {return _membs.size();}
            void dropMemb(int id)            {if (_membs.count(id)) _membs.erase(id);}
            void setNoobStyle(int ns)        {_noobstyle=ns; }
            int incNoobStyle()               {return ++_noobstyle; }
            int getNoobStyle() const         {return _noobstyle; }

            std::map<int, class Player*>::const_iterator getMembIter(bool first) const
                {return first ? _membs.begin() : _membs.end();}

            virtual void dump();
    };

}

#endif /* TEAM_H_ */
