#ifndef DDMINCOMMON_H
#define DDMINCOMMON_H

#include <cstdio>

#define REFREEZE_INTERVAL_TICKS (Server()->TickSpeed()>>1)

#define D(DF, DA...) fprintf(stderr,"%s:%i:%s() - " DF "\n",__FILE__,__LINE__,__func__,##DA)
#define DVF "(%f, %f)"
#define DVI "(%d, %d)"

#define DV(V) (V).x, (V).y

#define FLAGCOL_ELAST 0.5f

#define TICK Server()->Tick()
#define TICKSPEED Server()->TickSpeed()
#define CFG(X) g_Config.m_Sv ## X

#define MAX_BROADCAST 128

#endif
#ifndef DDMINCOMMON_H
#define DDMINCOMMON_H

#include <cstdio>

#define RACEBIT_TOUCH_FREEZE   0
#define RACEBIT_TOUCH_UNFREEZE 1
#define RACEBIT_TOUCH_START    2
#define RACEBIT_TOUCH_FIN      3

#define REFREEZE_INTERVAL_TICKS (Server()->TickSpeed()>>1)

#define D(DF, DA...) fprintf(stderr,"%s:%i:%s() - " DF "\n",__FILE__,__LINE__,__func__,##DA)
#define DVF "(%f, %f)"
#define DVI "(%d, %d)"

#define DV(V) (V).x, (V).y

#define FLAGCOL_ELAST 0.5f

#define TICK Server()->Tick()
#define TICKSPEED Server()->TickSpeed()
#define CFG(X) g_Config.m_Sv ## X

#define MAX_BROADCAST 128

#endif
