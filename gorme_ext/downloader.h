#pragma once

#include <tier1/utlvector.h>
#include <tier1/utlstring.h>
#include <tier1/functors.h>
#include <tier1/utlmap.h>
#include <tier1/UtlStringMap.h>
#include <tier1/utlflags.h>

class INetChannelHandler;
class CDownloader {
private:
	struct TJob {
		TJob(CFunctor* call) : m_call(call), m_remaining(0) {}
		~TJob() {
			delete m_call;
		}
		int m_remaining;
		CFunctor * m_call;
	};
	struct TDownload {
		TDownload(const CUtlString& file, int client, TJob * job) : m_file(file), m_client(client), m_job(job) {}
		CUtlString m_file;
		int m_client;
		TJob* m_job;
	};
public:
	CDownloader();
	~CDownloader();
	
	void Tick();

	void SendFiles(const CUtlVector<CUtlString>& files, CFunctor * callback);
	void OnFileReceived(const char *file, unsigned int id, bool demo);
	void OnFileDenied(const char *file, unsigned int id, bool demo);
private:
	void InitHook(INetChannelHandler* hander);
	unsigned int m_lastId;
	bool m_hooked;
	
	CUtlVector<TJob*> m_jobs;
	CUtlLinkedList<TDownload> m_downloads;
};