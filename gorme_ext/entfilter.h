#pragma once

#include "smsdk_ext.h"
#include "tier1/utlvector.h"

class CEntFilter {
public:
	void OnCheckTransmit(CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts);
	void Filter(unsigned short eid);
	void Clear(unsigned short eid);
	void ClearAll();
private:
	CUtlVector<unsigned short> m_filter[SM_MAXPLAYERS];
};