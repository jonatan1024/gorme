#pragma once

#include <tier1/utlvector.h>
#include <tier1/utlstring.h>
#include <tier1/functors.h>
#include <tier1/utlmap.h>
#include <tier1/UtlStringMap.h>
#include <tier1/utlflags.h>


class INetChannel;
class INetChannelHandler;
class INetworkStringTable;
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
	bool OnFileExists(const char *file, const char * path);
	bool IsFileInQueue(INetChannel * channel, const char * file);
	void AddStaticDownload(const char * file);
	bool IsDownloadStatic(const char * file);
	void RefreshDT();
private:
	unsigned int m_lastId;
	INetworkStringTable * m_downloadTable;
	CUtlVector<TJob*> m_jobs;
	CUtlLinkedList<TDownload> m_downloads;
	const char * m_ofeFalseNegative;
};