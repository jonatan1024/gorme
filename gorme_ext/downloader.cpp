#include "downloader.h"
#include "smsdk_ext.h"
#include <inetchannel.h>
#include <inetmsghandler.h>
#include <networkstringtabledefs.h>

extern INetworkStringTableContainer * g_pNSTC;

bool uintCmp(const unsigned int& u1, const unsigned int& u2) {
	return u1 < u2;
}

CDownloader::CDownloader() : m_lastId(0), m_ofeFalseNegative(false) {
	RefreshDT();
}

void CDownloader::RefreshDT() {
	m_downloadTable = g_pNSTC->FindTable("downloadables");
	if(!m_downloadTable)
		smutils->LogError(myself, "CDownloader::CDownloader -> INetworkStringTableContainer::FindTable(downloadables) returned null!");
}

CDownloader::~CDownloader() {

}

void CDownloader::AddStaticDownload(const char * file) {
	if(m_downloadTable->FindStringIndex(file) == INVALID_STRING_INDEX) {
		m_downloadTable->AddString(true, file);
	}
}

bool CDownloader::IsDownloadStatic(const char * file) {
	return m_downloadTable->FindStringIndex(file) != INVALID_STRING_INDEX;
}

void CDownloader::SendFiles(const CUtlVector<CUtlString>& files, CFunctor * callback) {
	TJob * job = new TJob(callback);
	int numFiles = files.Count();
	int numPlayers = playerhelpers->GetMaxClients();
	for(int iPlayer = 1; iPlayer < numPlayers; iPlayer++) {
		INetChannel * chan = (INetChannel*)engine->GetPlayerNetInfo(iPlayer);
		if(!chan)
			continue;
		for(int iFile = 0; iFile < numFiles; iFile++) {
			if(IsFileInQueue(chan, files[iFile]))
				continue;
			if(IsDownloadStatic(files[iFile]))
				continue;
			if(chan->SendFile(files[iFile], m_lastId, false)) {
				job->m_remaining++;
				m_downloads.AddToTail(TDownload(files[iFile], iPlayer, job));
				m_lastId++;
			}
			else {
				smutils->LogError(myself, "CDownloader::SendFiles -> SendFile(%s) failed!", files[iFile]);
			}
		}
	}
	m_jobs.AddToTail(job);
	FOR_EACH_VEC(files, iFile)
		AddStaticDownload(files[iFile]);
}

void CDownloader::Tick() {
	int numJobs = m_jobs.Count();
	for(int i = 0; i < numJobs; i++) {
		if(!m_jobs[i]->m_remaining) {
			(*m_jobs[i]->m_call)();
			delete m_jobs[i];
			m_jobs.Remove(i);
			i--;
			numJobs--;
			continue;
		}
	}
	if(m_downloads.Count()) {
		const TDownload& down = m_downloads.Element(m_downloads.Head());
		INetChannel * chan = (INetChannel*)engine->GetPlayerNetInfo(down.m_client);
		if(chan && IsFileInQueue(chan, down.m_file)) {
			m_downloads.AddToTail(down);
		}
		else
			down.m_job->m_remaining--;
		m_downloads.Remove(m_downloads.Head());
	}
}

bool CDownloader::OnFileExists(const char * file, const char * path) {
	if(m_ofeFalseNegative == file) {
		m_ofeFalseNegative = NULL;
		RETURN_META_VALUE(MRES_SUPERCEDE, false);
	}
	RETURN_META_VALUE(MRES_HANDLED, false);
}

bool CDownloader::IsFileInQueue(INetChannel * channel, const char * file) {
	//I love fucking with blackboxes :D
	m_ofeFalseNegative = file;
	return channel->SendFile(file, 0, false);
}