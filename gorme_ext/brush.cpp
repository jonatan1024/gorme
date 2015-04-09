#include "brush.h"
#include <KeyValues.h>

extern KeyValues * g_pGormeConfig;
extern CMdlCompile * g_pMdlCompile;

CFace::CFace() {
	strcpy(m_material, g_pGormeConfig->GetString("defaultMaterial", "debug/debugempty"));
	m_hammerId = -1;
	memset(m_uv, 0, 2 * sizeof(VPlane));
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
		bool merge = false;
		for(int j = 0; j < numNewFaces; j++) {
			if(helper->normal == m_faces[j].m_normal) {
				merge = true;
				break;
			}
		}
		if(merge) 
			continue;
		m_faces.InsertBefore(numNewFaces);
		if(helper->index >= 0) {
			m_faces[numNewFaces] = oldfaces[helper->index];
		}
		tmpBrush->GetFacePoints(i, m_faces[numNewFaces].m_points);
		m_faces[numNewFaces].m_normal = helper->normal;
		numNewFaces++;
	}
	m_brushFlags.SetFlag(BFL_MDLCOMPILE);
	g_pMdlCompile->AddToQueue(this);
}