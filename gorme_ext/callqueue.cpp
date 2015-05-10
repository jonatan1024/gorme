#include "callqueue.h"

void CCallQueue::operator<<(CFunctor * call) {
	m_mutex.Lock();
	m_calls.AddToTail(call);
	m_mutex.Unlock();
}

void CCallQueue::operator()() {
	m_mutex.Lock();
	while(m_calls.Count()) {
		CFunctor * call = m_calls.Element(m_calls.Head());
		m_calls.Remove(m_calls.Head());
		(*call)();
		delete call;
	}
	m_mutex.Unlock();
}

CCallQueue::~CCallQueue() {
	m_mutex.Lock();
	FOR_EACH_LL(m_calls, it) {
		delete m_calls.Element(m_calls.Head());
	}
}