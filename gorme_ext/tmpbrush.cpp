#include "tmpbrush.h"
#include "extension.h"
#include <vphysics_interface.h>
#include <toolframework/itoolentity.h>
#include <vscript/ivscript.h>

extern IServerTools * g_pServerTools;
extern CVsfun * g_pVsfun;
extern KeyValues * g_pGormeConfig;

inline edict_t *BaseEntityToEdict(CBaseEntity *pEntity) {
	IServerUnknown *pUnk = (IServerUnknown *)pEntity;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if(!pNet)
		return NULL;

	return pNet->GetEdict();
}

CTmpTriangle::CTmpTriangle() {
	const char * triangleModel = g_pGormeConfig->GetString("triangleModel", "models/gorme/triangle.mdl");
	const char * triangleEntType = g_pGormeConfig->GetString("triangleEntType", "prop_dynamic");
	const char * triangleEntClass = g_pGormeConfig->GetString("triangleEntClass", "CDynamicProp");
	engine->PrecacheModel(triangleModel);
	m_entity = (byte*)g_pServerTools->CreateEntityByName(triangleEntType);
	m_edict = BaseEntityToEdict((CBaseEntity*)m_entity);
	g_pServerTools->SetKeyValue(m_entity, "model", triangleModel);
	g_pServerTools->DispatchSpawn(m_entity);

	//todo holy fucking shit move this someplace else, this needs to be static!!!! FOR FUCKS SAKE
	//or not, we can keep it here for a while
	sm_sendprop_info_t propInfo;
	gamehelpers->FindSendPropInfo(triangleEntClass, "m_vecOrigin", &propInfo);
	m_origin = (Vector*)(m_entity + propInfo.actual_offset);
	gamehelpers->FindSendPropInfo(triangleEntClass, "m_flModelScale", &propInfo);
	m_scale = (float*)(m_entity + propInfo.actual_offset);
	gamehelpers->FindSendPropInfo(triangleEntClass, "m_flPoseParameter", &propInfo);
	m_controllers = (float*)(m_entity + propInfo.actual_offset);

	m_fScriptSetOrigin = g_pVsfun->LookupFunction("CBaseEntity", "ScriptSetOrigin");
	__asm{nop}
}

CTmpTriangle::~CTmpTriangle() {
	g_pVsfun->CallFunction("CBaseEntity", "ScriptUtilRemove", m_entity, NULL);
}

void CTmpTriangle::SetPoints(const Vector ** vecPts) {
	Vector localPts[2];
	localPts[0] = *vecPts[1] - *vecPts[0];
	localPts[1] = *vecPts[2] - *vecPts[0];
	float maxDist = 0.0;
	for(int i = 0; i < 6; i++) {
		maxDist = fmax(maxDist, abs(localPts[i / 3][i % 3]));
	}

	if(*m_origin != *vecPts[0]) {
		ScriptVariant_t neworigin = *vecPts[0];
		g_pVsfun->CallFunction(m_fScriptSetOrigin, m_entity, &neworigin);
	}
	if(*m_scale != maxDist) {
		*m_scale = maxDist;
		gamehelpers->SetEdictStateChanged(m_edict, m_entity - (byte*)m_scale);
	}
	for(int i = 0; i < 6; i++) {
		//controller works in range from 0.0 to 1.0, we want to work in range from -Inf to +Inf
		float controllerValue = (1.0 + localPts[i / 3][i % 3] / maxDist) * 0.5;
		if(m_controllers[i] != controllerValue) {
			m_controllers[i] = controllerValue;
			gamehelpers->SetEdictStateChanged(m_edict, m_entity - (byte*)m_controllers + i);
		}
	}
}

CTmpBrush::~CTmpBrush() {
	delete m_polyhedron;
}

inline const Vector& GetPolygonVertex(CPolyhedron * polyhedron, const Polyhedron_IndexedPolygon_t& polygon, int index) {
	const Polyhedron_IndexedLineReference_t& line = polyhedron->pIndices[polygon.iFirstIndex + index];
	int vertexIndex = polyhedron->pLines[line.iLineIndex].iPointIndices[line.iEndPointIndex];
	return polyhedron->pVertices[vertexIndex];
}

void CTmpBrush::GetFacePoints(int i, Vector points[3]) const {
	const int order[3] = {1, 0, 2};
	const Polyhedron_IndexedPolygon_t& polygon = m_polyhedron->pPolygons[i];
	for(int j = 0; j < 3; j++)
		points[order[j]] = GetPolygonVertex(m_polyhedron, polygon, j);
}


Vector CTmpBrush::GetCenter() const {
	Vector center(0, 0, 0);
	for(int i = 0; i < m_polyhedron->iVertexCount; i++) {
		center += m_polyhedron->pVertices[i];
	}
	center /= m_polyhedron->iVertexCount;
	return center;
}

void CTmpBrush::Refresh() {
	CUtlVector<TmpFaceHelper_t> oldhelpers;
	oldhelpers = m_helpers;

	m_numFaces = m_polyhedron->iPolygonCount;
	m_tmpTriangles.SetCountNonDestructively(m_numFaces);
	m_helpers.SetCountNonDestructively(m_numFaces);
	for(int i = 0; i < m_numFaces; i++) {
		const Polyhedron_IndexedPolygon_t& polygon = m_polyhedron->pPolygons[i];
		Assert(polygon.iIndexCount == 3);
		const Vector * triPts[3];
		for(int j = 0; j < 3; j++) {
			triPts[j] = &GetPolygonVertex(m_polyhedron, polygon, j);
		}
		m_tmpTriangles[i].SetPoints(triPts);
		//refresh helpers
		//todo finish
		//finishing - find best dot product; you also should modify helpers when rotating/scaling to avoid errors
		ComputeTriangleNormal(*triPts[0], *triPts[1], *triPts[2], m_helpers[i].normal);
		m_helpers[i].index = -1;
	}
}

bool CTmpBrush::SetPoints(const Vector * points, int ptsCnt) {
	//prepare polyhedron
	{
		const Vector ** ptrPts = new const Vector*[ptsCnt];
		for(int i = 0; i < ptsCnt; i++)
			ptrPts[i] = &points[i];
		CPhysConvex * convex = g_pPhysicsCollision->ConvexFromVerts((Vector**)ptrPts, ptsCnt);
		delete ptrPts;
		if(!convex)
			return false;
		if(!g_pPhysicsCollision->ConvexVolume(convex))
			return false;
		m_polyhedron = g_pPhysicsCollision->PolyhedronFromConvex(convex, false);
		if(!m_polyhedron)
			return false;
	}
	Refresh();
	return true;
}

bool CTmpBrush::SetPolyhedron(CPolyhedron * polyhedron) {
	if(!polyhedron)
		return false;
	m_polyhedron = polyhedron;
	Refresh();
	return true;
}
