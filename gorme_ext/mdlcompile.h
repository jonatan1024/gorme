#pragma once
#include <utlqueue.h>
#include <UtlStringMap.h>
#include <utlstring.h>
#include <utlmap.h>

typedef struct texDims_t {
	uint16 width;
	uint16 height;
}texDims_t;


class CFace;
class CBrush;
class CCompileThread;
class CMdlCompile {
	friend class CCompileThread;
public:
	CMdlCompile();
	~CMdlCompile();
	void Compile(CBrush * brush);
private:
	const char * GetMaterialTexture(const char * material);
	void CreateMaterial(const char * material);
	void WriteSMD(const char * filename, const CBrush * brush);
	void CMdlCompile::WriteQC(const char * filename, const CBrush * brush, const char * path, const char * mdlname);
	void CompileQC(const char * filename);
	const texDims_t& CMdlCompile::GetTexDims(const char * material);
	void GetVertexUV(const CFace& face, const Vector& vertex, float* vertexUV);

	friend unsigned CompileThread(void * vhptr);

	int m_maxThreads;
	CInterlockedInt m_numThreads;
	CUtlQueue<CBrush*> m_brushQueue;
	CThreadMutex m_brushQueueMutex;

	CThreadMutex m_createMaterialMutex;
	CThreadMutex m_matTexMutex;
	CUtlStringMap<CUtlString> m_matTex;
	CThreadMutex m_texDimsMutex;
	CUtlStringMap<texDims_t> m_texDims;
};
