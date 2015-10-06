#include "entity.h"

inline bool IsEditorVisible(KeyValues * kv) {
	return kv->GetBool("visgroupshown", false) && kv->GetBool("visgroupautoshown", false);
}

bool IsTreeVisible(KeyValues * kv) {
	FOR_EACH_TRUE_SUBKEY(kv, subkey) {
		if(!V_strcasecmp(subkey->GetName(), "editor")) {
			if(!IsEditorVisible(subkey))
				return false;
		}
		else if(!IsTreeVisible(subkey))
			return false;
	}
	return true;
}

CEntity::CEntity(bool world) : m_world(world) {
	m_kvs = new KeyValues(world ? "world" : "entity");
}

CEntity::CEntity(KeyValues * kv) {
	m_world = !V_strcasecmp(kv->GetName(), "world");
	m_kvs = new KeyValues(m_world ? "world" : "entity");
	FOR_EACH_VALUE(kv, value) {
		m_kvs->SetString(value->GetName(), value->GetString());
	}

	CUtlVector<KeyValues *> solids;
	FOR_EACH_TRUE_SUBKEY(kv, subkey) {
		const char * name = subkey->GetName();
		if(!V_strcasecmp(name, "hidden")) {
			FOR_EACH_TRUE_SUBKEY(subkey, subsubkey) {
				solids.AddToTail(subsubkey);
			}
		}
		else {
			if(IsTreeVisible(subkey)) {
				solids.AddToTail(subkey);
			}
		}
	}
	FOR_EACH_VEC(solids, it) {
		const char * name = solids[it]->GetName();
		if(!V_strcasecmp(name, "solid")) {
			AddBrush(new CBrush(solids[it]));
		}
	}
}

CEntity::~CEntity() {
	m_kvs->deleteThis();
	FOR_EACH_VEC(m_brushes, it)
		delete m_brushes[it];
}

void CEntity::AddBrush(CBrush* brush) {
	m_brushes.AddToTail(brush);
}

void CEntity::Refresh() {
	FOR_EACH_VEC(m_brushes, it)
		m_brushes[it]->SpawnModel();
}