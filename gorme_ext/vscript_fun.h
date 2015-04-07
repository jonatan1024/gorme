#pragma once
#include <UtlStringMap.h>

struct ScriptFunctionBinding_t;
struct ScriptVariant_t;

class CVsfun {
public:
	void RegisterFunction(const char * classname, const ScriptFunctionBinding_t* funcptr);
	const ScriptFunctionBinding_t * LookupFunction(const char * classname, const char * funcname);
	ScriptVariant_t CallFunction(const char * classname, const char * funcname, void * context, const ScriptVariant_t * args);
	ScriptVariant_t CallFunction(const ScriptFunctionBinding_t* func, void * context, const ScriptVariant_t * args);
private:
	CUtlStringMap<CUtlStringMap<const ScriptFunctionBinding_t*>> m_classes;
};