#pragma once
#include <utlqueue.h>
#include <amtl/include/am-thread-utils.h>
#include <threadtools.h>

class CBrush;
class CMdlCompile : ke::IRunnable {
public:
	CMdlCompile();
	~CMdlCompile();
	void AddToQueue(CBrush * brush);
	void Run();
private:
	void CreateSource();

	CUtlQueue<CBrush*> m_queue;
	CThreadMutex m_mutex;
	ke::Thread m_thread;
	//thread vars:
	bool m_threadTerminate;
	CBrush * m_brush;
};