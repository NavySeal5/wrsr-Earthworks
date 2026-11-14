#include <windows.h>
#include <cstddef>
#include "tesmio_api.h"
#include "earthworks_version.h"
#include "../runtime/runtime.h"
#include "../core/user_settings.h"

namespace {
const TsmHost* g_host = nullptr;
#if defined(EARTHWORKS_TRACE_FAULT)
LONG CALLBACK TraceFault(EXCEPTION_POINTERS* exception)
{
    if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && g_host) {
        MEMORY_BASIC_INFORMATION memory{};
        const auto address = exception->ExceptionRecord->ExceptionAddress;
        VirtualQuery(address,&memory,sizeof(memory));
        char path[MAX_PATH]{};
        GetModuleFileNameA(static_cast<HMODULE>(memory.AllocationBase),path,MAX_PATH);
        g_host->log("earthworks  AV trace module=%s offset=0x%llX address=%p accessed=%p rsp=%p",
            path,reinterpret_cast<uintptr_t>(address)-reinterpret_cast<uintptr_t>(memory.AllocationBase),address,
            reinterpret_cast<void*>(exception->ExceptionRecord->ExceptionInformation[1]),reinterpret_cast<void*>(exception->ContextRecord->Rsp));
        for (int i=0;i<6;++i) {
            const auto value=reinterpret_cast<void**>(exception->ContextRecord->Rsp)[i];
            MEMORY_BASIC_INFORMATION frame{}; VirtualQuery(value,&frame,sizeof(frame));
            char framePath[MAX_PATH]{}; GetModuleFileNameA(static_cast<HMODULE>(frame.AllocationBase),framePath,MAX_PATH);
            g_host->log("earthworks  AV stack[%d]=%p module=%s offset=0x%llX",i,value,framePath,
                reinterpret_cast<uintptr_t>(value)-reinterpret_cast<uintptr_t>(frame.AllocationBase));
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

}

extern "C" __declspec(dllexport) unsigned TsmPluginApiVersion(void)
{
    return TSM_API_VERSION;
}

extern "C" __declspec(dllexport) int TsmPluginInit(const TsmHost* host, TsmPluginInfo* info)
{
    g_host = nullptr;
    if (!host || !info || host->apiVersion < TSM_API_VERSION
        || host->structSize < offsetof(TsmHost, installInlineHook) + sizeof(host->installInlineHook)
        || !host->log) {
        return 1;
    }

    info->name = "Earthworks Planning - dedicated facility prototype";
    info->version = EARTHWORKS_VERSION;

    g_host = host;
    host->log("earthworks  planning prototype initialized; API %u", TSM_API_VERSION);
    return 0;
}

extern "C" __declspec(dllexport) int TsmPluginStart(void)
{
    if (!g_host) {
        return 1;
    }
    ew::host = g_host;
    ew::LoadGlobalSettings();
#if defined(EARTHWORKS_TRACE_FAULT)
    AddVectoredExceptionHandler(0,TraceFault);
#endif
    if (ew::InstallRuntime()) ew::StartOverlay();
    // A failed partial installation must remain resident: the host owns the
    // trampolines and cannot safely unload a DLL containing active detours.
    g_host->log("earthworks  startup complete; asset-identified Earthworks facility filters and Stage 7 dry-run active; Stage 3 edits require isolated S03 and explicit test buttons");
    return 0;
}
