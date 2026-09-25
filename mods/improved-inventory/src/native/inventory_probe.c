/* Release mode activates with a matching enabled asset folder. Diagnostic
 * mode retains explicit observation/layout session markers. This code does not
 * write item containers; ordinary native interactions still have game effects.
 * A byte/PE-guarded startup hook defers initialization to the game thread.
 * Observer/layout hooks additionally require the full executable hash.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "build_guard.h"

typedef int (__cdecl *ApiVersion)(void);
typedef void (__cdecl *ApiLog)(const char*, const char*, ...);
typedef int (__cdecl *ApiInstall)(UINT_PTR, int, void*, void**, int, const char*);
typedef void* (__cdecl *CreateRenderer)(void*, void*, const char*);
static ApiLog log_message;
static CreateRenderer original_renderer;
static volatile LONG event_count;
typedef void (__cdecl *LayoutGrid)(void*);
static LayoutGrid original_grid;
static volatile LONG grid_event_count;
static HINSTANCE own_module;
typedef uintptr_t (__cdecl *Bootstrap)(uintptr_t, uintptr_t, uintptr_t);
static Bootstrap original_bootstrap;
static volatile LONG initialized;
static int session_mode;

#if !IQ_RELEASE_BUILD

static int session_marker_enabled(void) {
    /* Steam starts a new process and does not inherit the launcher's environment.
       A short-lived marker beside this research DLL survives that handoff. */
    wchar_t path[MAX_PATH];
    DWORD size = GetModuleFileNameW(own_module, path, MAX_PATH);
    if (!size || size >= MAX_PATH) return 0;
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash || (size_t)(slash-path) + 32 >= MAX_PATH) return 0;
    wcscpy(slash+1, L"ImprovedInventoryProbe.session");
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info)) return 0;
    FILETIME now; GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER current, written;
    current.LowPart=now.dwLowDateTime; current.HighPart=now.dwHighDateTime;
    written.LowPart=info.ftLastWriteTime.dwLowDateTime; written.HighPart=info.ftLastWriteTime.dwHighDateTime;
    if (current.QuadPart < written.QuadPart || current.QuadPart-written.QuadPart > 300ULL*10000000ULL) return 0;
    HANDLE file=CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file==INVALID_HANDLE_VALUE) return 0;
    char data[16]={0}; DWORD received=0;
    BOOL ok=ReadFile(file,data,sizeof(data),&received,NULL); CloseHandle(file);
    if(ok && received==11 && memcmp(data,"layout-test",11)==0) {session_mode=2;return 1;}
    return ok && received==12 && memcmp(data,"observe-only",12)==0;
}


#endif

static void report(const char* message) {
    if (log_message) log_message(IQ_RELEASE_BUILD ? "ImprovedInventory" : "ImprovedInventoryProbe", "%s", message);
}

#include "inventory_layout.h"
#include "inventory_controls.h"
#include "inventory_equipment.h"
#include "inventory_hooks.h"

static int iq_file_matches(const wchar_t* path,const unsigned char expected[32]) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    unsigned char buffer[65536], digest[32];
    DWORD count = 0;
    int ok = 0;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) goto done;
    if (BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0) < 0) goto done;
    for (;;) {
        if (!ReadFile(file, buffer, sizeof(buffer), &count, NULL)) goto done;
        if (!count) break;
        if (BCryptHashData(hash, buffer, count, 0) < 0) goto done;
    }
    if (BCryptFinishHash(hash, digest, sizeof(digest), 0) < 0) goto done;
    ok = memcmp(digest, expected, sizeof(digest)) == 0;
done:
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return ok;
}

__declspec(dllexport) int InventoryProbeValidateImageW(const wchar_t* path) {
    return path && iq_file_matches(path,EXPECTED_SHA256);
}
__declspec(dllexport) int ImprovedInventoryBuildKind(void) {return IQ_RELEASE_BUILD;}
__declspec(dllexport) const char* ImprovedInventoryVersion(void) {return IQ_VERSION;}
#if IQ_RELEASE_BUILD
#include "inventory_startup.h"
#endif

static int read_name(const char* source, char* destination, size_t capacity) {
    if (!source) return 0;
    for (size_t i = 0; i + 1 < capacity; ++i) {
        SIZE_T read = 0;
        if (!ReadProcessMemory(GetCurrentProcess(), source + i, destination + i, 1, &read) || read != 1) return 0;
        if (destination[i] == 0) return 1;
        if ((unsigned char)destination[i] < 32 || (unsigned char)destination[i] > 126) return 0;
    }
    return 0;
}

static void __cdecl observe_grid(void* instance) {
    iq_before_grid(instance);
    original_grid(instance);
    iq_capture(instance);
    /* Bounded logging follows the optional layout experiment on the game thread. */
    unsigned char fields[0x88];
    SIZE_T received = 0;
    if (IQ_RELEASE_BUILD || grid_event_count >= 40 || !ReadProcessMemory(GetCurrentProcess(), instance,
        fields, sizeof(fields), &received) || received != sizeof(fields)) return;
    LONG event = InterlockedIncrement(&grid_event_count);
    if (event > 40) return;
    int mode, storage_columns, trash_columns, storage_drawers, trash_drawers;
    memcpy(&mode, fields + 0x48, 4);
    memcpy(&storage_columns, fields + 0x50, 4);
    memcpy(&trash_columns, fields + 0x54, 4);
    memcpy(&storage_drawers, fields + 0x64, 4);
    memcpy(&trash_drawers, fields + 0x74, 4);
    char message[256];
    snprintf(message, sizeof(message), "grid instance=%p mode=%d columns=%d/%d candidate_drawer_counts=%d/%d",
             instance, mode, storage_columns, trash_columns, storage_drawers, trash_drawers);
    report(message);
}

static void* __cdecl observe_renderer(void* scene, void* entity, const char* name) {
    /* Always preserve the original call and return value. No item pointers are
       retained or dereferenced; logged renderer addresses are transient hints. */
    void* result = original_renderer(scene, entity, name);
    char readable[96], message[260];
    if (!IQ_RELEASE_BUILD && event_count < 200 && read_name(name, readable, sizeof(readable))) {
        if (strstr(readable, "Inventory") || strstr(readable, "Storage") || strstr(readable, "Trash")) {
            LONG event = InterlockedIncrement(&event_count);
            if (event <= 200 && log_message) {
                snprintf(message, sizeof(message), "[ImprovedInventoryProbe] renderer=%s scene=%p entity=%p result=%p",
                         readable, scene, entity, result);
                report(message);
            }
        }
    }
    return result;
}

static DWORD WINAPI initialize(void* ignored) {
    (void)ignored;
    HMODULE loader = GetModuleHandleW(L"version.dll");
    if (!loader) return 0;
    ApiVersion version = (ApiVersion)(void*)GetProcAddress(loader, "MJ_GetVersion");
    log_message = (ApiLog)(void*)GetProcAddress(loader, "MJ_Log");
    ApiInstall install = (ApiInstall)(void*)GetProcAddress(loader, "MJ_InstallHook");
    if (!version || version() < 3 || !log_message || !install) return 0;
    report("Improved Inventory " IQ_VERSION " initialization started.");
#if IQ_RELEASE_BUILD
    wchar_t module[MAX_PATH];
    DWORD module_size=GetModuleFileNameW(own_module,module,MAX_PATH);
    if(!module_size || module_size>=MAX_PATH || !iq_assets_enabled(module,GetCommandLineW())) {
        report("Inactive: enable ImprovedInventory in the mod load order and deploy matching UI assets.");
        return 0;
    }
    session_mode=2;
#else
    char enabled[8] = {0};
    if ((GetEnvironmentVariableA("IMPROVED_INVENTORY_ENABLE_PROBE", enabled, sizeof(enabled)) != 1 || enabled[0] != '1') && !session_marker_enabled()) {
        report("Observers inactive. Explicit test switch or fresh session marker required.");
        return 0;
    }
    #endif
    wchar_t image[MAX_PATH];
    DWORD size = GetModuleFileNameW(NULL, image, MAX_PATH);
    if (!size || size >= MAX_PATH || !InventoryProbeValidateImageW(image)) {
        report("Unsupported executable hash. No observer hooks installed.");
        return 0;
    }
    report("Executable hash matched.");
    unsigned char* base = (unsigned char*)GetModuleHandleW(NULL);
    game_base=base;
    IQHook hooks[]={
        {RENDERER_RVA,EXPECTED_RENDERER_BYTES,(void*)observe_renderer,(void**)&original_renderer,"ImprovedInventory.Renderer"},
        {GRID_RVA,EXPECTED_GRID_BYTES,(void*)observe_grid,(void**)&original_grid,"ImprovedInventory.Grid"},
        {DRAWER_UPDATE_RVA,EXPECTED_DRAWER_UPDATE_BYTES,(void*)iq_update,(void**)&original_drawer_update,"ImprovedInventory.LayoutUpdate"},
        {MOUSE_EVENT_RVA,EXPECTED_MOUSE_EVENT_BYTES,(void*)iq_mouse,(void**)&original_mouse_event,"ImprovedInventory.Mouse"},
        {ITEM_CLICK_RVA,EXPECTED_ITEM_CLICK_BYTES,(void*)iq_click,(void**)&original_item_click,"ImprovedInventory.Click"},
        {ITEM_BIND_RVA,EXPECTED_ITEM_BIND_BYTES,(void*)iq_bind,(void**)&original_bind_item,"ImprovedInventory.Metadata"},
        {MOUSE_POSITION_RVA,EXPECTED_MOUSE_POSITION_BYTES,(void*)iq_mouse_position,(void**)&original_mouse_position,"ImprovedInventory.MousePosition"},
        {BUTTON_HIT_RVA,EXPECTED_BUTTON_HIT_BYTES,(void*)iq_button_hit,(void**)&original_button_hit,"ImprovedInventory.PopupHitTest"},
        {EQUIPMENT_GRID_RVA,EXPECTED_EQUIPMENT_GRID_BYTES,(void*)iq_equipment_grid,(void**)&original_equipment_grid,"ImprovedInventory.EquipmentGrid"},
        {EQUIPMENT_UPDATE_RVA,EXPECTED_EQUIPMENT_UPDATE_BYTES,(void*)iq_equipment_update,(void**)&original_equipment_update,"ImprovedInventory.EquipmentUpdate"},
        {EQUIPMENT_DRAWER_RVA,EXPECTED_EQUIPMENT_DRAWER_BYTES,(void*)iq_equipment_drawer_update,(void**)&original_equipment_drawer_update,"ImprovedInventory.EquipmentDrawer"},
        {EQUIPMENT_BIND_RVA,EXPECTED_EQUIPMENT_BIND_BYTES,(void*)iq_equipment_bind,(void**)&original_equipment_bind,"ImprovedInventory.EquipmentMetadata"}
    };
    int active=0;
    int status=iq_install_plan(base,hooks,session_mode==2?sizeof(hooks)/sizeof(hooks[0]):2,install,&active);
    if(status!=1) {
        report(status<0 ? "Incompatible hook entry; Improved Inventory inactive." :
               "Hook installation failed; installed callbacks remain pass-through.");
        return 0;
    }
    layout_test=active && session_mode==2;
    report(layout_test ? "Improved Inventory enabled; all UI hooks installed." : "Observation enabled; no layout changes.");
    return 0;
}

static uintptr_t __cdecl observe_bootstrap(uintptr_t a, uintptr_t b, uintptr_t c) {
    if (InterlockedCompareExchange(&initialized, 1, 0)==0) initialize(NULL);
    return original_bootstrap(a,b,c);
}

static void register_bootstrap(void) {
    /* No worker thread: this build's thread-local constructors can abort before
       the game CRT initializes. Defer hashing and observers to the game thread.
       Mewjector initializes its hook API before loading this DLL. */
    HMODULE loader=GetModuleHandleW(L"version.dll");
    if (!loader) return;
    ApiVersion version=(ApiVersion)(void*)GetProcAddress(loader,"MJ_GetVersion");
    ApiInstall install=(ApiInstall)(void*)GetProcAddress(loader,"MJ_InstallHook");
    log_message=(ApiLog)(void*)GetProcAddress(loader,"MJ_Log");
    if (!version || !install || version()<3) {report("Mewjector API v3 or newer is required.");return;}
    unsigned char* base=(unsigned char*)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER* dos=(IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS64* pe=(IMAGE_NT_HEADERS64*)(base+dos->e_lfanew);
    if (pe->FileHeader.TimeDateStamp!=EXPECTED_TIMESTAMP || pe->OptionalHeader.SizeOfImage!=EXPECTED_IMAGE_SIZE) {report("Unsupported executable headers; no bootstrap hook installed.");return;}
    if (memcmp(base+BOOTSTRAP_RVA,EXPECTED_BOOTSTRAP_BYTES,sizeof(EXPECTED_BOOTSTRAP_BYTES))!=0) {report("Bootstrap entry mismatch or another Improved Inventory copy already loaded; inactive.");return;}
    if(!install(BOOTSTRAP_RVA,0,(void*)observe_bootstrap,(void**)&original_bootstrap,50,"ImprovedInventory.Bootstrap"))report("Bootstrap hook installation failed.");
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        own_module=instance;
        DisableThreadLibraryCalls(instance);
        register_bootstrap();
    }
    return TRUE;
}
