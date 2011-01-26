#include <base/system.h>
#include <game/server/player.h>

#include "chatcommands.h"

bool CChatCommands::HandleMessage(class CPlayer *pPlayer, const char *pMsg)
{ // TODO make commands registerable
	bool got = false;
	if ((got = str_comp_nocase(pMsg, "/drop") == 0))
	{
		pPlayer->m_FlagDrop = true;
	}
	else if(pMsg[0] == '/')
	{
		got=true; //ignore for now
	}
	return got;
}
