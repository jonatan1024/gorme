#include "mdlcompile.h"
#include "brush.h"
#include <filesystem.h>
#include <KeyValues.h>
#include <vtf/vtf.h>
#include <threadtools.h>
#include "callqueue.h"
#include "smsdk_ext.h"

extern KeyValues * g_pGormeConfig;
extern CMdlCompile * g_pMdlCompile;
extern CCallQueue * g_pCallQueue;

extern CThreadMutex g_tickMutex;

bool MTFileExists(const char * filename) {
	g_tickMutex.Lock();
	bool retval = g_pFullFileSystem->FileExists(filename);
	g_tickMutex.Unlock();
	return retval;
}

class CCompileThread {
public:
	CCompileThread(CBrush * brush) : m_brush(brush) {}
	~CCompileThread() {
		g_pMdlCompile->CompileDone();
	}
	void Run();
private:
	CBrush *m_brush;
};

void CCompileThread::Run() {
	FOR_EACH_VEC(m_brush->m_faces, it)
		g_pMdlCompile->CreateMaterial(m_brush->m_faces[it].m_material);

	const char * path = "models/gorme/temp";
	g_pFullFileSystem->CreateDirHierarchy(path);

	char mdlhash[10];
	V_snprintf(mdlhash, sizeof(mdlhash), "%08x", m_brush->Hash());
	char filename[MAX_PATH];
	V_snprintf(filename, MAX_PATH, "%s/%s.mdl", path, mdlhash);
	if(!MTFileExists(filename)) {
		V_snprintf(filename, MAX_PATH, "%s/%s.smd", path, mdlhash);
		bool deleteSmd = !MTFileExists(filename);
		g_pMdlCompile->WriteSMD(filename, m_brush);
		V_snprintf(filename, MAX_PATH, "%s/%s.qc", path, mdlhash);
		bool deleteQc = !MTFileExists(filename);
		g_pMdlCompile->WriteQC(filename, m_brush, path, mdlhash);
		g_pMdlCompile->CompileQC(filename);
		//clean
		if(deleteQc)
			g_pFullFileSystem->RemoveFile(filename);
		V_snprintf(filename, MAX_PATH, "%s/%s.smd", path, mdlhash);
		if(deleteSmd)
			g_pFullFileSystem->RemoveFile(filename);

		V_snprintf(filename, MAX_PATH, "%s/%s.mdl", path, mdlhash);
	}
	*g_pCallQueue << CreateFunctor(m_brush, &CBrush::OnMdlCompiled, CUtlString(filename));
}

unsigned CompileThread(void * params) {
	CCompileThread t((CBrush*)params);
	t.Run();
	return 0;
}

CMdlCompile::CMdlCompile() {
	int maxThreads = g_pGormeConfig->GetInt("maxThreads", 1);
	if(maxThreads < 1)
		maxThreads = 1;
	m_numThreads = new CThreadSemaphore(maxThreads, maxThreads);
}

CMdlCompile::~CMdlCompile() {
	
	m_threadHandles.Lock();
	FOR_EACH_VEC(m_threadHandles, it) {
		ThreadJoin(m_threadHandles[it]);
		ReleaseThreadHandle(m_threadHandles[it]);
	}
	//delete m_numThreads;	//WTF???
}

void CMdlCompile::Compile(CBrush * brush) {
	m_numThreads->Wait();
	m_threadHandles.Lock();
	for(int i = 0; i < m_threadHandles.Count(); i++) {
		if(ThreadJoin(m_threadHandles[i], 0)) {
			bool released = ReleaseThreadHandle(m_threadHandles[i]);
			m_threadHandles.Remove(i);
			i--;
		}
	}
	ThreadHandle_t handle = CreateSimpleThread(CompileThread, brush);
	m_threadHandles.AddToTail(handle);
	m_threadHandles.Unlock();
}

void CMdlCompile::CompileDone() {
	m_numThreads->Release();
}

const char * CMdlCompile::GetMaterialTexture(const char * material) {
	m_matTexMutex.Lock();
	if(!m_matTex.Defined(material)) {
		char filename[MAX_PATH];
		V_snprintf(filename, MAX_PATH, "materials/%s.vmt", material);
		KeyValues * vmtKv = new KeyValues(NULL);
		vmtKv->LoadFromFile(g_pFullFileSystem, filename);
		m_matTex[material] = vmtKv->GetString("$baseTexture");
		vmtKv->deleteThis();
	}
	m_matTexMutex.Unlock();
	return m_matTex[material];
}

void CMdlCompile::CreateMaterial(const char * material) {
	CUtlBuffer outbuffer(0, 128, CUtlBuffer::TEXT_BUFFER);
	char filename[MAX_PATH];
	V_snprintf(filename, MAX_PATH, "materials/models/gorme/%s.vmt", material);
	m_createMaterialMutex.Lock();
	if(!MTFileExists(filename)) {
		char * lastDir = filename;
		for(int j = 0; filename[j]; j++)
			if(filename[j] == '/')
				lastDir = filename + j;
		*lastDir = 0;
		g_pFullFileSystem->CreateDirHierarchy(filename);
		*lastDir = '/';
		outbuffer.Clear();
		outbuffer.Printf("\"vertexLitGeneric\"\n{\n\t\"$baseTexture\" \"%s\"\n}\n", GetMaterialTexture(material));
		g_pFullFileSystem->WriteFile(filename, 0, outbuffer);
	}
	m_createMaterialMutex.Unlock();
}

const texDims_t& CMdlCompile::GetTexDims(const char * material) {
	m_texDimsMutex.Lock();
	if(!m_texDims.Defined(material)) {
		VTFFileHeaderV7_1_t * header;
		CUtlBuffer buffer;
		char filename[MAX_PATH];
		V_snprintf(filename, MAX_PATH, "materials/%s.vtf", material);
		g_pFullFileSystem->ReadFile(filename, 0, buffer, sizeof(*header));
		header = (VTFFileHeaderV7_1_t *)buffer.Base();
		texDims_t& dims = m_texDims[material];
		dims.height = header->height;
		dims.width = header->width;
	}
	m_texDimsMutex.Unlock();
	return m_texDims[material];
}

void CMdlCompile::GetVertexUV(const CFace& face, const Vector& vertex, float* vertexUV) {
	const texDims_t& texDims = GetTexDims(GetMaterialTexture(face.m_material));
	Vector uv[2];
	uv[0] = face.m_uv[0].m_Normal / texDims.width;
	uv[1] = face.m_uv[1].m_Normal / texDims.height;
	for(int i = 0; i < 2; i++)
		vertexUV[i] = uv[i].Dot(vertex);
}

void CMdlCompile::WriteSMD(const char * filename, const CBrush * brush) {
	CUtlBuffer outbuffer(0, 256, CUtlBuffer::TEXT_BUFFER);
	outbuffer.PutString("version 1\n\nnodes\n0 \"r\" -1\nend\n\nskeleton\ntime 0\n0\t0 0 0\t0 0 0\nend\n\ntriangles\n");
	FOR_EACH_VEC(brush->m_faces, it) {
		const CFace* face = &(brush->m_faces[it]);
		int tricount = face->m_triangles.Count();
		for(int k = 0; k < tricount;) {
			outbuffer.Printf("models/gorme/%s\n", face->m_material);
			for(int j = 0; j < 3; j++, k++) {
				float vertexUV[2];
				GetVertexUV(*face, face->m_triangles[k], vertexUV);
				outbuffer.Printf("0\t\t%g %g %g\t\t%g %g %g\t\t%g %g\n",
					face->m_triangles[k][1], -face->m_triangles[k][0], face->m_triangles[k][2],
					face->m_normal[1], -face->m_normal[0], face->m_normal[2],
					vertexUV[0], vertexUV[1]);
			}
		}
	}
	outbuffer.PutString("end\n");
	g_pFullFileSystem->WriteFile(filename, 0, outbuffer);
}

void CMdlCompile::WriteQC(const char * filename, const CBrush * brush, const char * path, const char * mdlname) {
	CUtlBuffer outbuffer(0, 256, CUtlBuffer::TEXT_BUFFER);
	//todo $staticprop & $opaque
	outbuffer.Printf("$modelname \"../%s/%s.mdl\"\n$origin\t%g %g %g\n$body body %s.smd\n$cdmaterials \"\"\n$sequence idle %s.smd\n$collisionmodel %s.smd\n", path, mdlname, brush->m_center[1], -brush->m_center[0], brush->m_center[2], mdlname, mdlname, mdlname);

	g_pFullFileSystem->WriteFile(filename, 0, outbuffer);
}

void CMdlCompile::CompileQC(const char * filename) {
	//there was an issue with accessing not-yet existing file
	//retrying compilation in an ugly, but working workaround
	int attempsLeft = 3;
	int wait = 0;
	char command[MAX_PATH];
	const char * compiler = g_pGormeConfig->GetString("mdlCompiler");
	if(compiler && compiler[0]) {
		V_snprintf(command, MAX_PATH, compiler, filename);
		int errorCode = -1;
		do {
			ThreadSleep(wait);
			errorCode = system(command);
			if(!errorCode)
				break;
			wait += 1000; //1s
		} while(--attempsLeft);
		if(errorCode != 0) {
			smutils->LogError(myself, "CMdlCompile::CompileQC -> system(%s) returned nonzero!", command);
		}
	}
	else {
		smutils->LogError(myself, "CMdlCompile::CompileQC -> mdlCompiler is null!");
	}
}