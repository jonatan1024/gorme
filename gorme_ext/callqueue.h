#include <tier1/functors.h>
#include <tier1/utllinkedlist.h>
#include <tier0/threadtools.h>

class CCallQueue {
public:
	~CCallQueue();
	void operator<<(CFunctor * call);
	void operator()();
private:
	CThreadMutex m_mutex;
	CUtlLinkedList<CFunctor *> m_calls;
};