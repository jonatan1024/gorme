#pragma once
#include <mathlib/vector.h>
#include <mathlib/polyhedron.h>
#include <utlvector.h>
#include "vscript_fun.h"

struct edict_t;

class CTmpTriangle {
public:
	CTmpTriangle();
	~CTmpTriangle();
	void SetPoints(const Vector ** points);
private:
	const ScriptFunctionBinding_t * m_fScriptSetOrigin;
	byte * m_entity;
	edict_t * m_edict;
	Vector * m_origin;
	float * m_scale;
	float * m_controllers;
};

typedef struct TmpFaceHelper_t {
	Vector normal;
	int index;
}TmpFaceHelper_t;

class CTmpBrush {
public:
	CTmpBrush() { m_polyhedron = NULL; m_numFaces = 0; }
	~CTmpBrush();
	bool SetPoints(const Vector * points, int ptsCnt);
	bool SetPolyhedron(CPolyhedron * m_polyhedron);
	int GetNumFaces() const { return m_numFaces; }
	const TmpFaceHelper_t & GetFaceHelper(int i) const { return m_helpers[i]; }
	void GetFacePoints(int index, Vector points[3]) const;
	Vector GetCenter() const;
private:
	void Refresh();
	int m_numFaces;
	CUtlVector<CTmpTriangle> m_tmpTriangles;
	CUtlVector<TmpFaceHelper_t> m_helpers;
	CPolyhedron * m_polyhedron;
};

inline void ComputeTriangleNormal(const Vector& v1, const Vector& v2, const Vector& v3, Vector& normal) {
	CrossProduct(v2 - v1, v3 - v1, normal);
#if 0
	VectorNormalizeFast(normal);
#else
	normal /= normal.Length();
#endif
}
