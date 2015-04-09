#pragma once
#include <mathlib/vector.h>
#include <mathlib/polyhedron.h>
#include <utlvector.h>
#include "tmpbrush.h"
#include <mathlib/vplane.h>
#include <utlflags.h>
#include "mdlcompile.h"

class CBrush;
class CFace {
	friend class CBrush;
public:
	CFace();
private:
	Vector m_normal;
	Vector m_points[3];
	char m_material[MAX_PATH];
	VPlane m_uv[2];
	int m_hammerId;
};

enum _brushFlags {
	BFL_READY = 1 << 0,
	BFL_LOCK = 1 << 1,
	BFL_MDLCOMPILE = 1 << 2,
	BFL_MDLREADY = 1 << 3,
};

class CBrush {
	friend class CMdlCompile;
public:
	void ApplyTmpBrush(CTmpBrush * tmpBrush);
	bool GetBrushFlag(int flag) {
		return m_brushFlags.IsFlagSet(flag);
	}
private:
	CUtlVector<CFace> m_faces;
	CUtlFlags<int> m_brushFlags;
};