#include <cstring>

#include <base/system.h>
#include <base/math.h>

#include <game/server/player.h>

#include "door.h"

std::map<unsigned int, CDoor*> CDoor::s_DoorMap;

CDoor* CDoor::GetDoorByName(const char *name)
{
	unsigned int hash = str_quickhash(name);
	if (!s_DoorMap.count(hash))
		return NULL;

	return s_DoorMap[hash];
}

CDoor::CDoor(CGameWorld *pWorld, vec2 From, vec2 To, const char *Name)
: CEntity(pWorld, NETOBJTYPE_LASER), m_Name(strdup(Name))
{
	dbg_msg("door","CDoor(%p, (%f,%f), (%f,%f)", pWorld, 
			From.x, From.y, To.x, To.y);

	if (From.x != To.x && From.y != To.y)
		dbg_msg("door", "crappy door from (%f,%f) to (%f,%f) "
				"(neither horizontal nor vertical)",
				From.x, From.y, To.x, To.y);

	m_Id2 = Server()->SnapNewID();
	m_Open = false;
	m_Pos = From;
	m_DestPos = To;
	m_Vert = abs(From.x - To.x) < abs(From.y - To.y);
	for(int z = 0; z < MAX_CLIENTS; ++z)
		m_aSideHints[z] = 0;

	s_DoorMap[str_quickhash(Name)] = this;

	dbg_msg("door", "Door \"%s\" (%s) created from (%f,%f) to (%f,%f)",
			m_Name, m_Vert?"vert":"horiz", From.x, From.y, 
			To.x, To.y);
}

CDoor::~CDoor()
{
	dbg_msg("door", "~CDoor()");
	s_DoorMap.erase(str_quickhash(m_Name));
}

void CDoor::SetOpen(bool Open)
{
	m_Open = Open;
}

void CDoor::Destroy()
{
	dbg_msg("door", "Destroy()");

	CEntity::Destroy();
}

void CDoor::Reset()
{
	dbg_msg("door", "Reset()");
	m_Open = false;
	CEntity::Reset();
}

void CDoor::Tick()
{
	//dbg_msg("door", "Tick()");
	CEntity::Tick();
	
	if (m_Open) 
		return;

	vec2 NewPos;

	CCharacter *pChr;
	while((pChr = GameWorld()->IntersectCharacter(m_Pos, m_DestPos,
		0.0f, NewPos/*dummy*/, GameServer()->GetPlayerChar(15)))) {

		NewPos = pChr->GetPos();
		dbg_msg("door", "intersecting tee at (%f, %f)", 
				NewPos.x, NewPos.y);
		if (m_Vert)
			NewPos.x = m_Pos.x + (pChr->m_ProximityRadius * 
				m_aSideHints[pChr->GetPlayer()->GetCID()]);
		else
			NewPos.y = m_Pos.y + (pChr->m_ProximityRadius * 
				m_aSideHints[pChr->GetPlayer()->GetCID()]);
		
		dbg_msg("door", "adj pos to (%f, %f)", NewPos.x, NewPos.y);

		pChr->SetPos(NewPos);		

		m_FlashTick = Server()->Tick();
	}
	if (m_FlashTick + Server()->TickSpeed() < Server()->Tick())
		m_FlashTick = Server()->Tick();

	for(int z = 0; z < MAX_CLIENTS - 1; ++z) {
		pChr = GameServer()->GetPlayerChar(z);
		
		if (!pChr) {
			m_aSideHints[z] = 0;
			continue;
		}

		float tmp = (m_Vert ? (pChr->m_Pos.x - m_Pos.x)
		                    : (pChr->m_Pos.y - m_Pos.y));

		dbg_msg("door", "%cdist of tee %i is %f", 
				m_Vert?'h':'v', z, tmp);

		int sig = (int)sign(tmp);
				
		if (m_aSideHints[z] != sig) {
			dbg_msg("door", "tee %i is now on the %s side",
					z, (sig>0)?"light":"dark");
			m_aSideHints[z] = sig;
		}
	}
}

void CDoor::TickDefered()
{
	//dbg_msg("door", "TickDefered");

	CEntity::TickDefered();
}

void CDoor::Snap(int SnappingClient)
{
	//dbg_msg("door", "Snap(%i)", SnappingClient);

	CEntity::Snap(SnappingClient);

	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->
		SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_DestPos.x;
	pObj->m_Y = (int)m_DestPos.y;
	pObj->m_FromX = (int)m_Pos.x;
	pObj->m_FromY = (int)m_Pos.y;
	pObj->m_StartTick = m_Open?0:m_FlashTick;
	
	pObj = static_cast<CNetObj_Laser *>(Server()->
		SnapNewItem(NETOBJTYPE_LASER, m_Id2,sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_DestPos.x;
	pObj->m_FromY = (int)m_DestPos.y;
	pObj->m_StartTick = m_Open?0:m_FlashTick;
}

  
