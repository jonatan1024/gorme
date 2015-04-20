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
	friend class CMdlCompile;
public:
	CFace();
	void InitUV(bool faceAlign = false);
	unsigned Hash();
private:
	Vector m_normal;
	CUtlVector<Vector> m_triangles;
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
	bool GetFlag(int flag) {
		return m_brushFlags.IsFlagSet(flag);
	}
	unsigned Hash();
private:
	Vector m_center;
	CUtlVector<CFace> m_faces;
	CUtlFlags<int> m_brushFlags;
};