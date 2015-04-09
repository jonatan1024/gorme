#include "mdlcompile.h"
#include "brush.h"
#include <filesystem.h>

extern IBaseFileSystem *g_pBaseFileSystem;

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
		CreateSource();
	}
}

void CMdlCompile::CreateSource() {
	Assert(m_brush->GetBrushFlag(BFL_LOCK) && m_brush->GetBrushFlag(BFL_MDLCOMPILE));
	
}



