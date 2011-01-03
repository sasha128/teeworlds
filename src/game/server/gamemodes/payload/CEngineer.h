/*
 * CEngineer.h
 *
 *  Created on: Nov 23, 2010
 *      Author: xig
 */

#ifndef CENGINEER_H_
#define CENGINEER_H_

#include "CPayloadClass.h"

class CEngineer: public CPayloadClass
{
	public:
		CEngineer();
		virtual ~CEngineer();

		virtual bool CanPush() const { return true; }
};

#endif /* CENGINEER_H_ */
