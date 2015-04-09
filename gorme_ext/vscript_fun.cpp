#include "vscript_fun.h"
#include <vscript/ivscript.h>

void CVsfun::RegisterFunction(const char * classname, const ScriptFunctionBinding_t* funcptr) {
	m_classes[classname][funcptr->m_desc.m_pszFunction] = funcptr;
}

const ScriptFunctionBinding_t * CVsfun::LookupFunction(const char * classname, const char * funcname) {
	return m_classes[classname][funcname];
}

ScriptVariant_t CVsfun::CallFunction(const char * classname, const char * funcname, void * context, const ScriptVariant_t * args) {
	const ScriptFunctionBinding_t * func = LookupFunction(classname, funcname);
	return CallFunction(func, context, args);
}

ScriptVariant_t CVsfun::CallFunction(const ScriptFunctionBinding_t * func, void * context, const ScriptVariant_t * args) {
	ScriptVariant_t ret;
	ScriptVariant_t * retPtr = ((func->m_desc.m_ReturnType) ? &ret : 0);
	bool ok = func->m_pfnBinding(func->m_pFunction, context, (ScriptVariant_t*)args, func->m_desc.m_Parameters.Count(), retPtr);
	Assert(ok);
	return ret;
}
