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

class CTmpBrush {
public:
	CTmpBrush() { m_polyhedron = NULL; }
	~CTmpBrush();
	bool SetPoints(const Vector * points, int ptsCnt);
private:
	CUtlVector<CTmpTriangle> m_tmpTriangles;
	CPolyhedron * m_polyhedron;
};