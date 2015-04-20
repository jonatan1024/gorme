#include "mdlcompile.h"
#include "brush.h"
#include <filesystem.h>
#include <KeyValues.h>
#include <vtf/vtf.h>

extern KeyValues * g_pGormeConfig;

CMdlCompile::CMdlCompile() : m_thread(this, "MdlCompile"), m_threadTerminate(false), m_mutex() {
}

CMdlCompile::~CMdlCompile() {
	m_threadTerminate = true;
	m_thread.Join();
}

void CMdlCompile::AddToQueue(CBrush * brush) {
	m_mutex.Lock();
	m_queue.Insert(brush);
	m_mutex.Unlock();
}

void CMdlCompile::Run() {
	while(true) {
		m_mutex.Lock();
		if(!m_queue.Count()) {
			m_mutex.Unlock();
			if(m_threadTerminate)
				break;
			continue;
		}
		m_brush = m_queue.Head();
		m_queue.RemoveAtHead();
		m_mutex.Unlock();
		CreateMaterials();
		CreateSource();
	}
}

const char * CMdlCompile::GetMaterialTexture(const char * material) {
	if(!m_matTex.Defined(material)) {
		char filename[MAX_PATH];
		V_snprintf(filename, MAX_PATH, "materials/%s.vmt", material);
		KeyValues * vmtKv = new KeyValues(NULL);
		vmtKv->LoadFromFile(g_pFullFileSystem, filename);
		m_matTex[material] = vmtKv->GetString("$baseTexture");
		vmtKv->deleteThis();
	}
	return m_matTex[material];
}

void CMdlCompile::CreateMaterials() {
	int facecount = m_brush->m_faces.Count();
	CUtlBuffer outbuffer(0, 128, CUtlBuffer::TEXT_BUFFER);
	for(int i = 0; i < facecount; i++) {
		const char *material = m_brush->m_faces[i].m_material;
		char filename[MAX_PATH];
		V_snprintf(filename, MAX_PATH, "materials/models/gorme/%s.vmt", material);
		if(!g_pFullFileSystem->FileExists(filename)) {
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
	}
	//TODO add to downloader
}

const texDims_t& CMdlCompile::GetTexDims(const char * material) {
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

void CMdlCompile::CreateSource() {
	Assert(m_brush->m_brushFlags.IsFlagSet(BFL_LOCK) && m_brush->m_brushFlags.IsFlagSet(BFL_MDLCOMPILE));
	const char * path = "models/gorme/temp";
	g_pFullFileSystem->CreateDirHierarchy(path);

	char mdlname[10];
	sprintf(mdlname, "%08x", m_brush->Hash());

	CUtlBuffer outbuffer(0, 256, CUtlBuffer::TEXT_BUFFER);
	char filename[MAX_PATH];

	//SMD

	outbuffer.Clear();
	outbuffer.PutString("version 1\n\nnodes\n0 \"r\" -1\nend\n\nskeleton\ntime 0\n0\t0 0 0\t0 0 0\nend\n\ntriangles\n");
	int facecount = m_brush->m_faces.Count();
	for(int i = 0; i < facecount; i++) {
		CFace* face = &(m_brush->m_faces[i]);
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
	V_snprintf(filename, MAX_PATH, "%s/%s.smd", path, mdlname);
	g_pFullFileSystem->WriteFile(filename, 0, outbuffer);

	//QC

	outbuffer.Clear();
	outbuffer.Printf("$modelname \"../%s/%s.mdl\"\n$origin\t%g %g %g\n$body body %s.smd\n$cdmaterials \"\"\n$sequence idle %s.smd\n$collisionmodel %s.smd\n", path, mdlname, m_brush->m_center[1], -m_brush->m_center[0], m_brush->m_center[2], mdlname, mdlname, mdlname);

	V_snprintf(filename, MAX_PATH, "%s/%s.qc", path, mdlname);
	g_pFullFileSystem->WriteFile(filename, 0, outbuffer);

	//MDL

	char command[MAX_PATH];
	const char * compiler = g_pGormeConfig->GetString("mdlCompiler");
	if(compiler && compiler[0]) {
		V_snprintf(command, MAX_PATH, compiler, filename);
		int errorCode = system(command);
		if(errorCode != 0) {
			//todo error
		}
	}
	else {
		//todo error
	}
	__asm{nop}
}



