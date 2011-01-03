/*
 * CPayloadClass.h
 *
 *  Created on: Nov 23, 2010
 *      Author: xig
 */

#ifndef CPAYLOADCLASS_H_
#define CPAYLOADCLASS_H_

class CPayloadClass
{
	public:
		CPayloadClass();
		virtual ~CPayloadClass();
		virtual bool CanPush() const = 0;
};

#endif /* CPAYLOADCLASS_H_ */
