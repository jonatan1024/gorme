#pragma once
#include <utlqueue.h>
#include <amtl/include/am-thread-utils.h>
#include <threadtools.h>
#include <UtlStringMap.h>
#include <utlstring.h>

typedef struct texDims_t {
	uint16 width;
	uint16 height;
}texDims_t;


class CFace;
class CBrush;
class CMdlCompile : ke::IRunnable {
public:
	CMdlCompile();
	~CMdlCompile();
	void AddToQueue(CBrush * brush);
	void Run();
private:
	const char * GetMaterialTexture(const char * material);
	const texDims_t& GetTexDims(const char * material);
	void GetVertexUV(const CFace& face, const Vector& vertex, float* vertexUV);
	void CreateMaterials();
	void CreateSource();

	CUtlQueue<CBrush*> m_queue;
	CThreadMutex m_mutex;
	ke::Thread m_thread;
	//thread vars:
	bool m_threadTerminate;
	CBrush * m_brush;

	CUtlStringMap<CUtlString> m_matTex;
	CUtlStringMap<texDims_t> m_texDims;
};