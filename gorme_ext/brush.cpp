#include "brush.h"
#include <KeyValues.h>
#include <generichash.h>

extern KeyValues * g_pGormeConfig;
extern CMdlCompile * g_pMdlCompile;

CFace::CFace() {
	strcpy(m_material, g_pGormeConfig->GetString("defaultMaterial", "debug/debugempty"));
	m_hammerId = -1;
	memset(m_uv, 0, 2 * sizeof(VPlane));
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

void CBrush::ApplyTmpBrush(CTmpBrush * tmpBrush) {
	Assert(!m_brushFlags.IsFlagSet(BFL_LOCK));
	m_brushFlags.SetFlag(BFL_LOCK);
	CUtlVector<CFace> oldfaces;
	oldfaces = m_faces;
	int numTmpFaces = tmpBrush->GetNumFaces();
	int numNewFaces = 0;
	m_faces.RemoveAll();
	for(int i = 0; i < numTmpFaces; i++) {
		TmpFaceHelper_t * helper = tmpBrush->GetFaceHelper(i);
		int index;
		for(index = 0; index < numNewFaces; index++)
			if(-helper->normal == m_faces[index].m_normal)
				break;
		if(index == numNewFaces) {
			m_faces.InsertBefore(numNewFaces);
			if(helper->index >= 0) {
				m_faces[index] = oldfaces[helper->index];
			}
			m_faces[index].m_normal = -helper->normal;
			m_faces[index].InitUV();
			numNewFaces++;
		}
		int tail = m_faces[index].m_triangles.AddMultipleToTail(3);
		tmpBrush->GetFacePoints(i, m_faces[index].m_triangles.Base() + tail);
	}
	m_center = tmpBrush->GetCenter();

	m_brushFlags.SetFlag(BFL_MDLCOMPILE);
	g_pMdlCompile->AddToQueue(this);
}

unsigned CFace::Hash() {
	unsigned hash[4];
	hash[0] = Hash12(m_normal.Base());	//normal
	hash[1] = HashBlock(m_triangles.Base(), sizeof(Vector) * m_triangles.Count()); //tris
	hash[2] = HashString(m_material);
	hash[3] = HashBlock(m_uv, 2 * sizeof(VPlane));
	return HashBlock(hash, sizeof(hash));
}

unsigned CBrush::Hash() {
	unsigned hash[2] = {0, 0};
	int faceCount = m_faces.Count();
	for(int i = 0; i < faceCount; i++) {
		hash[1] = m_faces[i].Hash();
		hash[0] = Hash8(hash);
	}
	return hash[0];
}