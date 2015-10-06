#pragma once

#include "brush.h"
#include <KeyValues.h>
#include <utlvector.h>

class CEntity {
public:
	CEntity(bool world = false);
	CEntity(KeyValues * kv);
	~CEntity();
	CEntity(const CEntity & ent) = delete;
	CEntity & operator=(const CEntity &) = delete;
	void AddBrush(CBrush* brush);
	void Refresh();
private:
	bool m_world;
	KeyValues * m_kvs;
	CUtlVector<CBrush*> m_brushes;
};