#pragma once
#include <mathlib/vector.h>
#include <mathlib/polyhedron.h>
#include <utlvector.h>
#include "vscript_fun.h"
#include <mathlib/vplane.h>

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

struct TTmpFaceHelper {
	TTmpFaceHelper() {}
	TTmpFaceHelper(const Vector & normal, int index) : normal(normal), index(index) {}
	Vector normal;
	int index;
};

class CPhysConvex;
class CTmpBrush {
public:
	CTmpBrush(bool visible = true) : m_polyhedron(NULL), m_numFaces(0), m_visible(visible) {  }
	~CTmpBrush();
	bool SetPoints(const Vector * points, int ptsCnt);
	bool SetPlanes(const VPlane * planes, int plnCnt);
	void AddHelper(const TTmpFaceHelper & helper) {
		m_helpers.AddToTail(helper);
	}
	void SetHelpers(const CUtlVector<TTmpFaceHelper> & helpers) {
		m_helpers = helpers;
	}
	int GetNumFaces() const { return m_numFaces; }
	const TTmpFaceHelper & GetFaceHelper(int i) const { return m_helpers[i]; }
	void GetFacePoints(int index, Vector points[3]) const;
	Vector GetCenter() const;
	void SetVisible(bool visible) {
		m_visible = visible;
		Refresh();
	}
private:
	bool SetConvex(CPhysConvex * convex);
	void Refresh();
	int m_numFaces;
	CUtlVector<CTmpTriangle> m_tmpTriangles;
	CUtlVector<TTmpFaceHelper> m_helpers;
	CPolyhedron * m_polyhedron;
	bool m_visible;
};

inline void ComputeTriangleNormal(const Vector& v1, const Vector& v2, const Vector& v3, Vector& normal) {
	CrossProduct(v2 - v1, v3 - v1, normal);
#if 0
	VectorNormalizeFast(normal);
#else
	normal /= normal.Length();
#endif
}
