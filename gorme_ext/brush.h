#pragma once
#include <mathlib/vector.h>
#include <mathlib/polyhedron.h>
#include <utlvector.h>
#include "tmpbrush.h"
#include <mathlib/vplane.h>
#include <utlflags.h>
#include "mdlcompile.h"
#include <checksum_crc.h>
class CCompileThread;

class KeyValues;

class CBrush;
class CFace {
	friend class CBrush;
	friend class CMdlCompile;
	friend class CCompileThread;
public:
	CFace();
	CFace(KeyValues * kv);
	CFace(const CFace &);
	void InitUV(bool faceAlign = false);
	void Hash(CRC32_t* crc);
private:
	Vector m_normal;
	CUtlVector<Vector> m_triangles;
	char m_material[MAX_PATH];
	VPlane m_uv[2];
};

enum _brushFlags {
	BFL_READY = 1 << 0,
	BFL_LOCK = 1 << 1,
};

class CBrush {
	friend class CMdlCompile;
	friend class CCompileThread;
public:
	CBrush();
	CBrush(KeyValues *);
	~CBrush();
	CBrush(const CBrush&) = delete;
	CBrush& operator=(const CBrush&) = delete;
	bool ApplyTmpBrush(const CTmpBrush & tmpBrush);
	bool GetFlag(int flag) {
		return m_brushFlags.IsFlagSet(flag);
	}
	CRC32_t Hash();
	void OnMdlCompiled(CUtlString mdlfile);
	void SpawnModel();
private:
	Vector m_center;
	CUtlVector<CFace> m_faces;
	CUtlFlags<int> m_brushFlags;
	char m_mdlfile[MAX_PATH];
	CBaseEntity * m_mdlent;
};
