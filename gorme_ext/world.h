#pragma once
#include "entity.h"
#include <igameevents.h>

class CWorld : public IGameEventListener {
public:
	CWorld() {}
	~CWorld();
	CWorld(const CWorld & ent) = delete;
	CWorld & operator=(const CWorld &) = delete;

	virtual void FireGameEvent(KeyValues *event) {
		Refresh();
	}

	void AddEntity(CEntity * ent) {
		m_entities.AddToTail(ent);
	}
	void Clear();
	void Refresh();
	bool ImportVMF(const char * vmffile);
private:
	CUtlVector<CEntity*> m_entities;
};