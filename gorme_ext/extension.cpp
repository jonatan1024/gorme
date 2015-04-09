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

CGorme g_Gorme;
SMEXT_LINK(&g_Gorme);

IServerTools * g_pServerTools = NULL;
IGameHelpers * g_pGameHelpers = NULL;
IScriptManager * g_pScriptManager = NULL;
IBaseFileSystem *g_pBaseFileSystem = NULL;

CVsfun * g_pVsfun = NULL;
KeyValues * g_pGormeConfig = NULL;
CMdlCompile * g_pMdlCompile = NULL;


bool CGorme::SDK_OnLoad(char *error, size_t maxlength, bool late) {
	SM_GET_IFACE(GAMEHELPERS, g_pGameHelpers);
	g_pVsfun = new CVsfun();
	g_pGormeConfig = new KeyValues("Gorme Config");
	g_pGormeConfig->LoadFromFile(g_pBaseFileSystem, "cfg/sourcemod/gorme.cfg");
	g_pMdlCompile = new CMdlCompile();
	return true;
}

SH_DECL_MANUALHOOK1_void(MHook_RegisterFunction, 18, 0, 0, ScriptFunctionBinding_t*);
SH_DECL_MANUALHOOK1(MHook_RegisterClass, 19, 0, 0, bool, ScriptClassDesc_t*);

int hookId_OnRegisterFunction = 0;
int hookId_OnRegisterClass = 0;

void OnRegisterFunction(ScriptFunctionBinding_t* func) {
	g_pVsfun->RegisterFunction("", func);
	RETURN_META(MRES_IGNORED);
}

bool OnRegisterClass(ScriptClassDesc_t* pClassDest) {
	for(int i = 0; i < pClassDest->m_FunctionBindings.Count(); i++) {
		g_pVsfun->RegisterFunction(pClassDest->m_pszClassname, &pClassDest->m_FunctionBindings[i]);
	}
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

bool CGorme::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
	//todo error msgs?
	GET_V_IFACE_ANY(GetPhysicsFactory, g_pPhysicsCollision, IPhysicsCollision, VPHYSICS_COLLISION_INTERFACE_VERSION);
	if(!g_pPhysicsCollision)
		return false;
	GET_V_IFACE_ANY(GetServerFactory, g_pServerTools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	if(!g_pServerTools)
		return false;
	GET_V_IFACE_ANY(GetEngineFactory, g_pScriptManager, IScriptManager, VSCRIPT_INTERFACE_VERSION);
	if(!g_pScriptManager)
		return false;
	GET_V_IFACE_ANY(GetFileSystemFactory, g_pBaseFileSystem, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION);
	if(!g_pBaseFileSystem)
		return false;
	
	IScriptVM * svm = g_pScriptManager->CreateVM();
	hookId_OnRegisterFunction = SH_ADD_MANUALVPHOOK(MHook_RegisterFunction, svm, SH_STATIC(OnRegisterFunction), true);
	hookId_OnRegisterClass = SH_ADD_MANUALVPHOOK(MHook_RegisterClass, svm, SH_STATIC(OnRegisterClass), true);
	g_pScriptManager->DestroyVM(svm);
	return true;
}

void CGorme::SDK_OnUnload() {
	SH_REMOVE_HOOK_ID(hookId_OnRegisterFunction);
	SH_REMOVE_HOOK_ID(hookId_OnRegisterClass);
	delete g_pVsfun;
	g_pGormeConfig->deleteThis();
	delete g_pMdlCompile;
}

CTmpBrush * g_test;

cell_t GormeTest(IPluginContext *pContext, const cell_t *params) {
	
	g_test = new CTmpBrush();
	Vector points[] = {
		Vector(-16, -16, 0),
		Vector(-16, 16, 0),
		Vector(16, -16, 0),
		Vector(16, 16, 0),
		Vector(-16, -16, 72),
		Vector(-16, 16, 72),
		Vector(16, -16, 72),
		Vector(16, 16, 72),
	};
	g_test->SetPoints(points, 8);
	CBrush b;
	b.ApplyTmpBrush(g_test);
	return 0;
}

cell_t GormeTest2(IPluginContext *pContext, const cell_t *params) {
	delete g_test;
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