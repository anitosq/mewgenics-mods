/* Soul Well: collection is native; summon effects remain in the game engine. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "guard.h"

typedef unsigned char byte;
typedef void (*Spawn)(void*);
typedef void (*CastPoint)(void*, byte, byte);
typedef void* (*SourceItem)(void*);
typedef void* (*CatLookup)(void*, int64_t);
typedef void (*Resize)(void*, unsigned);
typedef byte (*Allied)(void*, void*, byte);
typedef int (*Install)(UINT_PTR, int, void*, void**, int, const char*);
typedef void (*Log)(const char*, const char*, ...);
typedef struct { unsigned capacity, count; void* data; } Vector;
#define MAX_SOULS 1024
/* Keep both short branches inside the trampoline or on its jump-back boundary. */
#define SOURCE_STOLEN 31
#define FN(type, rva) ((type)(base + (rva)))
static byte* base;
static Spawn original_spawn;
static CastPoint original_castpoint;
static SourceItem original_source;
static Log logger;
static int hooks_ready;

typedef struct {
    int32_t ids[MAX_SOULS];
    unsigned count, duplicates, invalid, stored, carried, equipped;
} Souls;
typedef struct Scope {
    void* ability;
    Souls souls;
    unsigned next;
    byte source[0x60];
    int prepared;
} Scope;
static _Thread_local Scope* active;

static int read_at(const void* p, size_t offset, void* out, size_t n) {
    SIZE_T got = 0;
    return p && ReadProcessMemory(GetCurrentProcess(), (const byte*)p + offset, out, n, &got) && got == n;
}
static void* pointer(const void* p, size_t offset) {
    void* value = NULL;
    read_at(p, offset, &value, sizeof(value));
    return value;
}
static int named(const void* item, const char* expected) {
    uint64_t length = 0, capacity = 0;
    char text[128];
    if (!read_at(item, 0x18, &length, 8) || !read_at(item, 0x20, &capacity, 8) ||
        length != strlen(expected) || length >= sizeof(text) || capacity < length) return 0;
    const void* source = capacity > 15 ? pointer(item, 8) : (const byte*)item + 8;
    return read_at(source, 0, text, (size_t)length) && !memcmp(text, expected, (size_t)length);
}
static int add_item(Souls* result, const void* item, unsigned origin) {
    int32_t id;
    if (!read_at(item, 0x4c, &id, 4)) return 0;
    if (!named(item, "SoulJar_Full")) return 1;
    if (id < 0) { result->invalid++; return 1; }
    if (origin == 0) result->carried++;
    else if (origin == 1) result->stored++;
    else result->equipped++;
    for (unsigned i = 0; i < result->count; i++) {
        if (result->ids[i] == id) { result->duplicates++; return 1; }
    }
    if (result->count == MAX_SOULS) return 0;
    result->ids[result->count++] = id;
    return 1;
}
static int scan_container(Souls* result, const void* manager, size_t offset, unsigned origin) {
    const byte* map = (const byte*)manager + offset;
    uint64_t count;
    void* head = pointer(map, 8);
    if (!head || !read_at(map, 0x10, &count, 8) || count > 100000) return 0;
    void* node = pointer(head, 0);
    for (uint64_t i = 0; i < count; i++) {
        if (!node || node == head || !add_item(result, (byte*)node + 0x18, origin)) return 0;
        node = pointer(node, 0);
    }
    return node == head;
}
static int compare_id(const void* a, const void* b) {
    int32_t x = *(const int32_t*)a, y = *(const int32_t*)b;
    return (x > y) - (x < y);
}
static int scan_equipment(Souls* result, void* caster) {
    void* board = pointer(caster, 0x80);
    int dimensions[2];
    if (!read_at(board, 0xb8, dimensions, 8) || dimensions[0] < 1 || dimensions[1] < 1 ||
        dimensions[0] > 64 || dimensions[1] > 64) return 0;
    void* cells = pointer(board, 0xb0);
    void* seen[256];
    unsigned seen_count = 0;
    for (int i = 0; i < dimensions[0] * dimensions[1]; i++) {
        Vector cell;
        if (!read_at(cells, (size_t)i * sizeof(cell), &cell, sizeof(cell)) ||
            cell.count > cell.capacity || cell.count > 128) return 0;
        for (unsigned j = 0; j < cell.count; j++) {
            void* object = pointer(cell.data, j * 8);
            void* actor = pointer(object, 0x98);
            void* cat = pointer(actor, 0x88);
            int64_t cat_id;
            if (!cat) continue;
            if (!read_at(cat, 0xc48, &cat_id, 8)) return 0;
            if (cat_id < 0) continue; /* Native ghost copies have no campaign ID. */
            unsigned k = 0;
            while (k < seen_count && seen[k] != cat) k++;
            if (k < seen_count) continue;
            if (seen_count == 256) return 0;
            seen[seen_count++] = cat;
            if (actor != caster && !FN(Allied, 0x119e20)(caster, actor, 0)) continue;
            for (unsigned slot = 0; slot < 5; slot++) {
                if (!add_item(result, (byte*)cat + 0x9b0 + slot * 0x60, 2)) return 0;
            }
        }
    }
    return 1;
}
static int capture(Souls* result, void* ability) {
    memset(result, 0, sizeof(*result));
    void* game = pointer(base, 0x13dac30);
    void* inventory = pointer(game, 0x588);
    void* database = pointer(game, 0x598);
    if (!inventory || !database || !scan_container(result, inventory, 0x38, 0) ||
        !scan_container(result, inventory, 0x78, 1) ||
        !scan_equipment(result, pointer(ability, 0x10))) return 0;
    qsort(result->ids, result->count, sizeof(result->ids[0]), compare_id);
    unsigned valid = 0;
    for (unsigned i = 0; i < result->count; i++) {
        if (FN(CatLookup, 0xd7220)(database, result->ids[i])) result->ids[valid++] = result->ids[i];
        else result->invalid++;
    }
    result->count = valid;
    return 1;
}
static void castpoint(void* ability, byte tiles, byte effects) {
    original_castpoint(ability, tiles, effects);
    Scope* scope = active;
    if (!scope || scope->ability != ability) return;
    Vector* targets = (Vector*)((byte*)ability + 0x8e8);
    uint64_t position;
    Vector before;
    if (!read_at(targets, 0, &before, sizeof(before)) || before.count != 1 ||
        before.capacity < before.count || !read_at(before.data, 0, &position, 8)) {
        targets->count = 0;
        return;
    }
    /* The engine owns this allocation and releases it with the ability. */
    FN(Resize, 0x48fb0)(targets, scope->souls.count);
    if (scope->souls.count && !targets->data) { targets->count = 0; return; }
    for (unsigned i = 0; i < scope->souls.count; i++) ((uint64_t*)targets->data)[i] = position;
    targets->count = scope->souls.count;
    scope->prepared = 1;
}
static void* source_item(void* ability) {
    Scope* scope = active;
    if (scope && scope->prepared && scope->ability == ability &&
        __builtin_return_address(0) == base + 0x7a3c51 && scope->next < scope->souls.count) {
        memcpy(scope->source + 0x4c, &scope->souls.ids[scope->next++], 4);
        return scope->source;
    }
    return original_source(ability);
}
static void spawn(void* callback) {
    void* ability = pointer(callback, 0);
    void* source = original_source(ability);
    if (!hooks_ready || !named(source, "SoulWell")) { original_spawn(callback); return; }
    Scope scope = {0};
    scope.ability = ability;
    int stage = -1;
    byte split = 1;
    if (!read_at(ability, 0x8a8, &stage, 4) || !read_at(callback, 8, &split, 1) || stage != 0 || split ||
        !read_at(source, 0, scope.source, sizeof(scope.source)) || !capture(&scope.souls, ability)) {
        if (logger) logger("SoulWell", "Cast refused: native state validation failed.");
        FN(Spawn, 0x43a50)(ability);
        return;
    }
    Scope* previous = active;
    active = &scope;
    original_spawn(callback);
    active = previous;
    if (logger) logger("SoulWell", "Cast: stored=%u carried=%u equipped=%u unique=%u duplicates=%u invalid=%u requested=%u prepared=%d",
        scope.souls.stored, scope.souls.carried, scope.souls.equipped, scope.souls.count,
        scope.souls.duplicates, scope.souls.invalid, scope.next, scope.prepared);
}

/* Thread-attach notifications may arrive on graphics threads with small stacks. */
static __attribute__((noinline)) int file_matches(const wchar_t* path, const byte* expected) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    byte buffer[65536], digest[32];
    DWORD n;
    int ok = 0;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) goto done;
    if (BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0) < 0) goto done;
    for (;;) {
        if (!ReadFile(file, buffer, sizeof(buffer), &n, NULL)) goto done;
        if (!n) break;
        if (BCryptHashData(hash, buffer, n, 0) < 0) goto done;
    }
    if (BCryptFinishHash(hash, digest, 32, 0) >= 0) ok = !memcmp(digest, expected, 32);
done:
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return ok;
}
#include "startup.h"

static int activate_hooks(Install install) {
    hooks_ready = 0;
    if (!install(0x43350, 0, (void*)castpoint, (void**)&original_castpoint, 50, "SoulWell.CastPoint") ||
        !install(0x475c0, SOURCE_STOLEN, (void*)source_item, (void**)&original_source, 50, "SoulWell.Source") ||
        !install(0x7a2ba0, 0, (void*)spawn, (void**)&original_spawn, 50, "SoulWell.Spawn")) return 0;
    hooks_ready = 1;
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(module);
    base = (byte*)GetModuleHandleW(NULL);
    HMODULE loader = GetModuleHandleW(L"version.dll");
    if (!loader) return TRUE;
    Install install = (Install)(void*)GetProcAddress(loader, "MJ_InstallHook");
    int (*version)(void) = (int (*)(void))(void*)GetProcAddress(loader, "MJ_GetVersion");
    logger = (Log)(void*)GetProcAddress(loader, "MJ_Log");
    wchar_t executable[MAX_PATH], module_path[MAX_PATH];
    if (!install || !version || version() < 3) return TRUE;
    if (!GetModuleFileNameW(NULL, executable, MAX_PATH) ||
        !file_matches(executable, EXPECTED_SHA256)) {
        if (logger) logger("SoulWell", "Disabled: unsupported executable.");
        return TRUE;
    }
    if (!GetModuleFileNameW(module, module_path, MAX_PATH) ||
        !SoulWellValidateAssetsW(module_path, GetCommandLineW())) {
        if (logger) logger("SoulWell", "Disabled: data missing, disabled or incompatible. Check -modpaths and restore core GON fields.");
        return TRUE;
    }
    for (unsigned i = 0; i < sizeof(GUARDS) / sizeof(*GUARDS); i++) {
        if (memcmp(base + GUARDS[i].rva, GUARDS[i].bytes, 64)) {
            if (logger) logger("SoulWell", "Disabled: native entry differs at RVA %x.", GUARDS[i].rva);
            return TRUE;
        }
    }
    if (!activate_hooks(install)) {
        if (logger) logger("SoulWell", "Disabled: hook installation failed.");
        return TRUE;
    }
    if (logger) logger("SoulWell", "%s loaded. Scanning home storage, backpack and allied adventurers.", SW_VERSION);
    return TRUE;
}
