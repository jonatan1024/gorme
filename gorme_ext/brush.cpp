#include "brush.h"
#include <KeyValues.h>
#include "downloader.h"
#include <toolframework/itoolentity.h>
#include "smsdk_ext.h"
#include "entfilter.h"
#include "callqueue.h"
#include "vscript_fun.h"

extern KeyValues * g_pGormeConfig;
extern CMdlCompile * g_pMdlCompile;
extern CDownloader * g_pDownloader;
extern IServerTools * g_pServerTools;
extern CEntFilter * g_pEntFilter;
extern CCallQueue * g_pCallQueue;
extern CVsfun * g_pVsfun;

CFace::CFace() {
	strcpy(m_material, g_pGormeConfig->GetString("defaultMaterial", "debug/debugempty"));
	memset(m_uv, 0, 2 * sizeof(VPlane));
}

CFace::CFace(KeyValues * kv) {
	strcpy(m_material, kv->GetString("material", g_pGormeConfig->GetString("defaultMaterial", "debug/debugempty")));
	memset(m_uv, 0, 2 * sizeof(VPlane));
	for(char axisname[] = "uaxis"; axisname[0] <= 'v'; axisname[0]++) {
		VPlane & axis = m_uv[axisname[0] - 'u'];
		const char * szAxis = kv->GetString(axisname);
		float scale = 1.f;
		auto a = sscanf(szAxis, "[%f %f %f %f] %f", &axis.m_Normal.x, &axis.m_Normal.y, &axis.m_Normal.z, &axis.m_Dist, &scale);
		axis.m_Normal /= scale;
		axis.m_Dist /= scale;
	}
}

CFace::CFace(const CFace & o) {
	memcpy(this, &o, sizeof(CFace));
	m_triangles = o.m_triangles;
}

void CFace::InitUV(bool faceAlign) {
	float bestDot = 0;
	Vector & u = m_uv[0].m_Normal;
	Vector & v = m_uv[1].m_Normal;
	u.Zero();
	v.Zero();
	for(int i = 0; i < 6; i++) {
		Vector orientation;
		orientation.Zero();
		orientation[i % 3] = i < 3 ? 1 : -1;
		float dot = orientation.Dot(m_normal);
		if(dot > bestDot) {
			bestDot = dot;
			if(!(i % 3))
				u.Init(0, 1, 0);
			else
				u.Init(1, 0, 0);
			if(i % 3 == 2)
				v.Init(0, -1, 0);
			else
				v.Init(0, 0, -1);
		}
	}
	if(faceAlign) {
		CrossProduct(m_normal, v, u);
		u /= u.Length();
		CrossProduct(u, m_normal, v);
		v /= v.Length();
	}
	__asm{nop}
}

CBrush::CBrush() : m_mdlent(NULL) {
	m_mdlfile[0] = '\0';
}



CBrush::CBrush(KeyValues * kv) : m_mdlent(NULL) {
	m_mdlfile[0] = '\0';

	CTmpBrush tmpbrush(false);
	CUtlVector<VPlane> planes;
	FOR_EACH_TRUE_SUBKEY(kv, subkey) {
		const char * name = subkey->GetName();
		if(!V_strcasecmp(name, "side")) {
			const char * szPlane = subkey->GetString("plane");
			Vector pts[3];
			sscanf(szPlane, "(%f %f %f) (%f %f %f) (%f %f %f)",
				&pts[0].x, &pts[0].y, &pts[0].z,
				&pts[1].x, &pts[1].y, &pts[1].z,
				&pts[2].x, &pts[2].y, &pts[2].z);
			Vector normal;
			float dist;
			ComputeTrianglePlane(pts[0], pts[1], pts[2], normal, dist);
			planes.AddToTail(VPlane(-normal, -dist));

			tmpbrush.AddHelper(TTmpFaceHelper(-normal, m_faces.Count()));

			m_faces.AddToTail(CFace(subkey));
		}
	}
	tmpbrush.SetPlanes(planes.Base(), planes.Count());
	ApplyTmpBrush(tmpbrush);
}

CBrush::~CBrush() {
	assert(!m_brushFlags.IsFlagSet(BFL_LOCK));
	if(m_brushFlags.IsFlagSet(BFL_LOCK)) {
		smutils->LogError(myself, "CBrush::~CBrush -> BFL_LOCK is set in m_brushFlags!");
	}
	if(m_mdlent) {
		g_pVsfun->CallFunction("CBaseEntity", "ScriptUtilRemove", m_mdlent, NULL);
		m_mdlent = NULL;
	}
}

bool CBrush::ApplyTmpBrush(const CTmpBrush & tmpBrush) {
	if(m_brushFlags.IsFlagSet(BFL_LOCK))
		return false;
	m_brushFlags.SetFlag(BFL_LOCK);
	//destroy old mdl
	if(m_mdlent) {
		g_pVsfun->CallFunction("CBaseEntity", "ScriptUtilRemove", m_mdlent, NULL);
		m_mdlent = NULL;
	}

	CUtlVector<CFace> oldfaces;
	oldfaces = m_faces;
	int numTmpFaces = tmpBrush.GetNumFaces();
	int numNewFaces = 0;
	m_faces.RemoveAll();
	for(int i = 0; i < numTmpFaces; i++) {
		const TTmpFaceHelper & helper = tmpBrush.GetFaceHelper(i);
		int index;
		for(index = 0; index < numNewFaces; index++)
			if(helper.normal == m_faces[index].m_normal)
				break;
		if(index == numNewFaces) {
			m_faces.InsertBefore(numNewFaces);
			if(helper.index >= 0) {
				m_faces[index] = oldfaces[helper.index];
			}
			else {
				m_faces[index].m_normal = helper.normal;
				m_faces[index].InitUV();
			}
			numNewFaces++;
		}
		int tail = m_faces[index].m_triangles.AddMultipleToTail(3);
		tmpBrush.GetFacePoints(i, m_faces[index].m_triangles.Base() + tail);
	}
	m_center = tmpBrush.GetCenter();
	g_pMdlCompile->Compile(this);
	return true;
}


void CBrush::SpawnModel() {
	engine->PrecacheModel(m_mdlfile);
	m_mdlent = (CBaseEntity*)g_pServerTools->CreateEntityByName(g_pGormeConfig->GetString("mdlEntType", "prop_dynamic"));
	g_pServerTools->SetKeyValue(m_mdlent, "model", m_mdlfile);
	g_pServerTools->SetKeyValue(m_mdlent, "solid", "6");
	g_pServerTools->DispatchSpawn(m_mdlent);
	ScriptVariant_t origin = m_center;
	g_pVsfun->CallFunction("CBaseEntity", "ScriptSetOrigin", m_mdlent, &origin);
	m_brushFlags.ClearFlag(BFL_LOCK);
	m_brushFlags.SetFlag(BFL_READY);
}

void CBrush::OnMdlCompiled(CUtlString mdlfile) {
	strcpy(m_mdlfile, mdlfile);
	CUtlVector<CUtlString> files;

	{
		int dotpos = strlen(m_mdlfile) - 4;
		const char exts[][16] = {".vvd", ".phy", ".dx90.vtx", ".mdl"};
		for(int i = 0; i < sizeof(exts) / sizeof(*exts); i++) {
			strcpy(m_mdlfile + dotpos, exts[i]);
			files.AddToTail(m_mdlfile);
		}
	}
	CUtlString matfile;
	FOR_EACH_VEC(m_faces, it) {
		matfile.Format("materials/models/gorme/%s.vmt", m_faces[it].m_material);
		bool uniq = true;
		FOR_EACH_VEC(files, it2) {
			if(files[it2] == matfile) {
				uniq = false;
				break;
			}
		}
		if(uniq)
			files.AddToTail(matfile);
	}
	g_pDownloader->SendFiles(files, CreateFunctor(this, &CBrush::SpawnModel));
}

void CFace::Hash(CRC32_t* crc) {
	CRC32_ProcessBuffer(crc, m_normal.Base(), sizeof(Vector));
	CRC32_ProcessBuffer(crc, m_triangles.Base(), sizeof(Vector) * m_triangles.Count());
	CRC32_ProcessBuffer(crc, m_triangles.Base(), sizeof(Vector) * m_triangles.Count());
	CRC32_ProcessBuffer(crc, m_material, strlen(m_material));
	CRC32_ProcessBuffer(crc, m_uv, sizeof(VPlane) * 2);
}

CRC32_t CBrush::Hash() {
	CRC32_t crc;
	CRC32_Init(&crc);
	FOR_EACH_VEC(m_faces, it) {
		m_faces[it].Hash(&crc);
	}
	CRC32_Final(&crc);
	return crc;
}
