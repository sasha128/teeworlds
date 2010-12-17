/*
 * common.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#ifndef COMMON_H_
#define COMMON_H_


#include <cstdio>

#define VERBOSE 0

#define TEAMSELECT_INVALID -1
#define TEAMSELECT_SOLO -2

#define D(F,...) fprintf(stderr,"%s:%i:%s() - " F "\n",__FILE__,__LINE__,__func__,__VA_ARGS__)
#define S(F) fprintf(stderr,"%s:%i:%s() - " F "\n",__FILE__,__LINE__,__func__)
#define DV(F,...) do{if (VERBOSE) fprintf(stderr,"%s:%i:%s() - " F "\n",__FILE__,__LINE__,__func__,__VA_ARGS__); } while(0)
#define SV(F) do{if (VERBOSE) fprintf(stderr,"%s:%i:%s() - " F "\n",__FILE__,__LINE__,__func__); } while(0)


#endif /* COMMON_H_ */
