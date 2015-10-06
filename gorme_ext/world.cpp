#include "world.h"
#include <filesystem.h>

extern IFileSystem * g_pFullFileSystem;

CWorld::~CWorld() {
	Clear();
}

void CWorld::Clear() {
	FOR_EACH_VEC(m_entities, it)
		delete m_entities[it];
	m_entities.RemoveAll();
}

void CWorld::Refresh() {
	FOR_EACH_VEC(m_entities, it)
		m_entities[it]->Refresh();
}

bool CWorld::ImportVMF(const char * vmffile) {
	KeyValues * kv = new KeyValues("");
	if(!kv->LoadFromFile(g_pFullFileSystem, vmffile))
		return false;

	CUtlVector<KeyValues *> ents;
	for(KeyValues *key = kv; key; key = key->GetNextKey()) {
		const char * name = key->GetName();
		if(!V_strcasecmp(name, "hidden")) {
			FOR_EACH_TRUE_SUBKEY(key, subkey) {
				ents.AddToTail(subkey);
			}
		}
		else {
			ents.AddToTail(key);
		}
	}

	FOR_EACH_VEC(ents, it) {
		const char * name = ents[it]->GetName();
		if(!V_strcasecmp(name, "world") || !V_strcasecmp(name, "entity")) {
			AddEntity(new CEntity(ents[it]));
		}
	}

	kv->deleteThis();
	return true;
}