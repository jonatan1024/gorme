#include "downloader.h"
#include "smsdk_ext.h"
#include <inetchannel.h>
#include <inetmsghandler.h>

int hookId;

SH_DECL_HOOK3_void(INetChannelHandler, FileReceived, SH_NOATTRIB, 0, const char *, unsigned int, bool);

bool uintCmp(const unsigned int& u1, const unsigned int& u2) {
	return u1 < u2;
}

CDownloader::CDownloader() : m_lastId(0), m_hooked(false) {

}

CDownloader::~CDownloader() {
	SH_REMOVE_HOOK_ID(hookId);
}

void CDownloader::InitHook(INetChannelHandler * handler) {
	printf("%08X\n", handler);
	hookId = SH_ADD_VPHOOK(INetChannelHandler, FileReceived, handler, SH_MEMBER(this, &CDownloader::OnFileReceived), true);
	m_hooked = true;
}

void CDownloader::SendFiles(const CUtlVector<CUtlString>& files, CFunctor * callback) {
	TJob * job = new TJob(callback);
	int numFiles = files.Count();
	int numPlayers = playerhelpers->GetMaxClients();
	for(int iPlayer = 1; iPlayer < numPlayers; iPlayer++) {
		INetChannel * chan = (INetChannel*)engine->GetPlayerNetInfo(iPlayer);
		if(!chan)
			continue;
		if(!m_hooked)
			InitHook(chan->GetMsgHandler());
		for(int iFile = 0; iFile < numFiles; iFile++) {
			if(chan->SendFile(files[iFile], m_lastId, false)) {
				job->m_remaining++;
				m_downloads.AddToTail(TDownload(files[iFile], iPlayer, job));
				m_lastId++;
			}
			else {
				assert(0);
				//todo error
			}
		}
	}
	m_jobs.AddToTail(job);
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
	if(m_downloads.Count()){
		const TDownload& down = m_downloads.Element(m_downloads.Head());
		INetChannel * chan = (INetChannel*)engine->GetPlayerNetInfo(down.m_client);
		if(chan) {
			chan->RequestFile(down.m_file, false);
			m_downloads.AddToTail(down);
		}
		else
			down.m_job->m_remaining--;
		m_downloads.Remove(m_downloads.Head());
	}
}

void CDownloader::OnFileReceived(const char *file, unsigned int id, bool demo) {
	IClient *client = META_IFACEPTR(IClient);
	int iPlayer = client->GetPlayerSlot() + 1;
	FOR_EACH_LL(m_downloads, iterator) {
		const TDownload& down = m_downloads.Element(iterator);
		if(down.m_client == iPlayer && down.m_file == (CUtlString)file) {
			down.m_job->m_remaining--;
			m_downloads.Remove(iterator);
			break;
		}
	}
}
