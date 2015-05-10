#pragma once
#include <UtlStringMap.h>

struct ScriptFunctionBinding_t;
struct ScriptVariant_t;
struct ScriptClassDesc_t;

class CVsfun {
public:
	void RegisterFunction(const char * classname, const ScriptFunctionBinding_t* funcptr);
	const ScriptFunctionBinding_t * LookupFunction(const char * classname, const char * funcname);
	ScriptVariant_t CallFunction(const char * classname, const char * funcname, void * context, const ScriptVariant_t * args);
	ScriptVariant_t CallFunction(const ScriptFunctionBinding_t* func, void * context, const ScriptVariant_t * args);

	void OnRegisterFunction(ScriptFunctionBinding_t* func);
	bool OnRegisterClass(ScriptClassDesc_t* pClassDest);
private:
	CUtlStringMap<CUtlStringMap<const ScriptFunctionBinding_t*>> m_classes;
};