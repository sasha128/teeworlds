#ifndef GAME_SERVER_ENTITIES_DOOR_H
#define GAME_SERVER_ENTITIES_DOOR_H

#include <map>

#include <game/generated/protocol.h>
#include <game/server/entity.h>

class CDoor : public CEntity
{
private:
	vec2 m_DestPos;
	bool m_Vert;
	int m_FlashTick;
	bool m_Open;
	int m_aSideHints[MAX_CLIENTS];
	int m_Id2; //for second laser beam
	const char *m_Name;

	static std::map<unsigned int, CDoor*> s_DoorMap;

public:
	CDoor(CGameWorld *pWorld, vec2 From, vec2 To, const char *Name); 
	virtual ~CDoor();

	void SetOpen(bool Open);

	virtual void Destroy();
	virtual void Reset();
	virtual void Tick();
	virtual void TickDefered();
	virtual void Snap(int SnappingClient);

	static CDoor* GetDoorByName(const char *name);
};

#endif /* GAME_SERVER_ENTITIES_DOOR_H */
