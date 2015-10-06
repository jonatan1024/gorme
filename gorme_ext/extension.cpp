#include "extension.h"
#include "tmpbrush.h"
#include <toolframework/itoolentity.h>
#include <IGameHelpers.h>
#include <vscript/ivscript.h>
#include "vscript_fun.h"
#include "brush.h"
#include <KeyValues.h>
#include <filesystem.h>
#include "mdlcompile.h"
#include "downloader.h"
#include "callqueue.h"
#include "entfilter.h"
#include <networkstringtabledefs.h>
#include "entity.h"
#include "world.h"
#include <igameevents.h>

CGorme g_Gorme;
SMEXT_LINK(&g_Gorme);

IServerTools * g_pServerTools = NULL;
IScriptManager * g_pScriptManager = NULL;
IServerGameEnts * g_pServerGameEnts = NULL;
INetworkStringTableContainer * g_pNSTC = NULL;
IGameEventManager * g_pGEM = NULL;

CVsfun * g_pVsfun = NULL;
KeyValues * g_pGormeConfig = NULL;
CMdlCompile * g_pMdlCompile = NULL;
CDownloader * g_pDownloader = NULL;
CCallQueue * g_pCallQueue = NULL;
CEntFilter *g_pEntFilter = NULL;

CThreadMutex g_tickMutex;
CInterlockedInt g_tickWaiting;

CWorld * g_pWorld = NULL;

bool CGorme::SDK_OnLoad(char *error, size_t maxlength, bool late) {
	g_tickMutex.Lock();
	g_pGormeConfig = new KeyValues("Gorme Config");
	g_pGormeConfig->LoadFromFile(g_pFullFileSystem, "cfg/sourcemod/gorme.cfg");
	g_pMdlCompile = new CMdlCompile();
	g_pCallQueue = new CCallQueue();

	g_pDownloader->AddStaticDownload(g_pGormeConfig->GetString("triangleModel", "models/gorme/triangle.mdl"));

	g_pWorld = new CWorld();
	g_pGEM->AddListener(g_pWorld, "round_start", true);

	return true;
}

CUtlVector<int> hookIds;
SH_DECL_HOOK1_void(IScriptVM, RegisterFunction, SH_NOATTRIB, 0, ScriptFunctionBinding_t*);
SH_DECL_HOOK1(IScriptVM, RegisterClass, SH_NOATTRIB, 0, bool, ScriptClassDesc_t*);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK3_void(IServerGameEnts, CheckTransmit, SH_NOATTRIB, 0, CCheckTransmitInfo *, const unsigned short *, int);
SH_DECL_HOOK2(IBaseFileSystem, FileExists, SH_NOATTRIB, 0, bool, const char *, const char *);
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);

bool CGorme::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
	IScriptVM * svm;

	GET_V_IFACE_ANY(GetPhysicsFactory, g_pPhysicsCollision, IPhysicsCollision, VPHYSICS_COLLISION_INTERFACE_VERSION);
	if(!g_pPhysicsCollision)
		goto SDK_OnMetamodLoad_failed;
	GET_V_IFACE_ANY(GetServerFactory, g_pServerTools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	if(!g_pServerTools)
		goto SDK_OnMetamodLoad_failed;
	GET_V_IFACE_ANY(GetEngineFactory, g_pScriptManager, IScriptManager, VSCRIPT_INTERFACE_VERSION);
	if(!g_pScriptManager)
		goto SDK_OnMetamodLoad_failed;
	GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	if(!g_pFullFileSystem)
		goto SDK_OnMetamodLoad_failed;
	GET_V_IFACE_ANY(GetServerFactory, g_pServerGameEnts, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	if(!g_pServerGameEnts)
		goto SDK_OnMetamodLoad_failed;
	GET_V_IFACE_ANY(GetEngineFactory, g_pNSTC, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	if(!g_pNSTC)
		goto SDK_OnMetamodLoad_failed;
	GET_V_IFACE_ANY(GetEngineFactory, g_pGEM, IGameEventManager, INTERFACEVERSION_GAMEEVENTSMANAGER);
	if(!g_pGEM)
		goto SDK_OnMetamodLoad_failed;

	g_pVsfun = new CVsfun();
	g_pEntFilter = new CEntFilter();
	g_pDownloader = new CDownloader();

	svm = g_pScriptManager->CreateVM();
	hookIds.AddToTail(SH_ADD_VPHOOK(IScriptVM, RegisterFunction, svm, SH_MEMBER(g_pVsfun, &CVsfun::OnRegisterFunction), true));
	hookIds.AddToTail(SH_ADD_VPHOOK(IScriptVM, RegisterClass, svm, SH_MEMBER(g_pVsfun, &CVsfun::OnRegisterClass), true));
	g_pScriptManager->DestroyVM(svm);
	hookIds.AddToTail(SH_ADD_VPHOOK(IServerGameDLL, GameFrame, gamedll, SH_MEMBER(this, &CGorme::OnGameFrame), false));
	//hookIds.AddToTail(SH_ADD_VPHOOK(IServerGameDLL, GameFrame, gamedll, SH_MEMBER(this, &CGorme::OnGameFrame_post), true));
	hookIds.AddToTail(SH_ADD_VPHOOK(IServerGameDLL, LevelInit, gamedll, SH_MEMBER(this, &CGorme::OnLevelInit), true));
	hookIds.AddToTail(SH_ADD_VPHOOK(IServerGameEnts, CheckTransmit, g_pServerGameEnts, SH_MEMBER(g_pEntFilter, &CEntFilter::OnCheckTransmit), true));
	hookIds.AddToTail(SH_ADD_VPHOOK(IBaseFileSystem, FileExists, g_pFullFileSystem, SH_MEMBER(g_pDownloader, &CDownloader::OnFileExists), false));
	return true;

SDK_OnMetamodLoad_failed:
	smutils->LogError(myself, "CGorme::SDK_OnMetamodLoad");
	if(!g_pPhysicsCollision)
		smutils->LogError(myself, "g_pPhysicsCollision is null!");
	else if(!g_pServerTools)
		smutils->LogError(myself, "g_pServerTools is null!");
	else if(!g_pScriptManager)
		smutils->LogError(myself, "g_pScriptManager is null!");
	else if(!g_pFullFileSystem)
		smutils->LogError(myself, "g_pFullFileSystem is null!");
	else if(!g_pServerGameEnts)
		smutils->LogError(myself, "g_pServerGameEnts is null!");
	else if(!g_pNSTC)
		smutils->LogError(myself, "g_pNSTC is null!");
	else if(!g_pGEM)
		smutils->LogError(myself, "g_pGEM is null!");
	return false;
}

void CGorme::SDK_OnUnload() {
	delete g_pWorld;

	FOR_EACH_VEC(hookIds, it)
		SH_REMOVE_HOOK_ID(hookIds[it]);
	delete g_pVsfun;
	g_pGormeConfig->deleteThis();
	delete g_pMdlCompile;
	delete g_pDownloader;
	delete g_pCallQueue;
	delete g_pEntFilter;
}

void CGorme::OnGameFrame(bool simulating) {
	g_pDownloader->Tick();
	(*g_pCallQueue)();

	if(g_tickWaiting > 0) {
		g_tickMutex.Unlock();
		while(g_tickWaiting > 0) {
			ThreadSleep();	//yield
		}
		g_tickMutex.Lock();
	}

	RETURN_META(MRES_HANDLED);
}

void CGorme::OnGameFrame_post(bool simulating) {
	//...
	RETURN_META(MRES_HANDLED);
}

bool CGorme::OnLevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background) {
	g_pDownloader->RefreshDT();
	g_pWorld->Refresh();
	RETURN_META_VALUE(MRES_HANDLED, false);
}

cell_t GormeTest(IPluginContext *pContext, const cell_t *params) {
	//g_pWorld->ImportVMF("../sdk_content/maps/instances/de_aztec/de_aztec_pyramid_top_a.vmf");
	g_pWorld->ImportVMF("_gorme.vmf");
	return 0;
	float dist = 10000.0;
	VPlane planes[] = {
		VPlane(Vector(0, 0, 1), dist),
		VPlane(Vector(0, 0, -1), dist),
		VPlane(Vector(0, 1, 0), dist),
		VPlane(Vector(0, -1, 0), dist),
		VPlane(Vector(1, 0, 0), dist),
		VPlane(Vector(-1, 0, 0), dist),
	};
	CTmpBrush t(false);
	t.SetPlanes(planes, 6);
	auto br = new CBrush();
	br->ApplyTmpBrush(t);
	return 0;
	for(int i = 0; i < 100; i++) {
		CTmpBrush t(false);
		Vector pts[] = {
			Vector(0, 0, 0 + 16 * i),
			Vector(8, 0, 0 + 16 * i),
			Vector(0, 8, 0 + 16 * i),
			Vector(8, 8, 0 + 16 * i),
			Vector(0, 0, 8 + 16 * i),
			Vector(8, 0, 8 + 16 * i),
			Vector(0, 8, 8 + 16 * i),
			Vector(8, 8, 8 + 16 * i),
		};
		t.SetPoints(pts, 8);
		auto br = new CBrush();
		br->ApplyTmpBrush(t);
	}
	return 0;
}

cell_t GormeTest2(IPluginContext *pContext, const cell_t *params) {
	g_pWorld->Clear();
	return 0;
}

const sp_nativeinfo_t MyNatives[] =
{
	{"GormeTest", GormeTest},
	{"GormeTest2", GormeTest2},
	{NULL, NULL},
};

void CGorme::SDK_OnAllLoaded() {
	sharesys->AddNatives(myself, MyNatives);
}
