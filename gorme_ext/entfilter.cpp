#include "entfilter.h"

void CEntFilter::OnCheckTransmit(CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts) {
	int iPlayer = gamehelpers->IndexOfEdict(pInfo->m_pClientEnt);
	assert(iPlayer != -1);
	assert(iPlayer > 0);
	int numEids = m_filter[iPlayer].Count();
	for(int i = 0; i < numEids; i++) {
		pInfo->m_pTransmitEdict->Clear(m_filter[iPlayer][i]);
	}
}

void CEntFilter::Filter(unsigned short eid) {
	int numClients = playerhelpers->GetMaxClients();
	for(int i = 1; i < numClients; i++) {
		if(engine->GetPlayerNetInfo(i)) {
			m_filter[i].AddToTail(eid);
		}
	}
}

void CEntFilter::Clear(unsigned short eid) {
	for(int i = 0; i < SM_MAXPLAYERS; i++) {
		int numEids = m_filter[i].Count();
		for(int j = 0; j < numEids; j++) {
			if(m_filter[i][j] == eid) {
				m_filter[i].Remove(j);
				j--;
				numEids--;
			}
		}
	}
}

void CEntFilter::ClearAll() {
	for(int i = 0; i < SM_MAXPLAYERS; i++) {
		m_filter[i].RemoveAll();
	}
}