#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <wchar.h>
#include <stdarg.h>
#include "policy.h"
#include "guard.h"

typedef unsigned char byte;

typedef struct { union { char local[16]; const char *pointer; }; size_t size, capacity; } GameString;
typedef byte (*RandomMutation)(void *, byte, GameString *, void *);
typedef byte (*MutatePiece)(void *, int, GameString *);
typedef void (*SetPiece)(void *, uint64_t);
typedef void (*Logger)(const char *, const char *, ...);
typedef struct { int combat, protect; void *cat; } MutationContext;

static byte *base;
static int ready;
static Logger log_message;
static RandomMutation original_random;
static MutatePiece original_mutate;
static SetPiece original_set;
static _Thread_local MutationContext *context;
#define OWNER "GoodGenes"
#define COUNT(a) (sizeof(a) / sizeof(*(a)))
#define FN(type,rva) ((type)(void *)(base + (rva)))

static int read_bytes(const void *p, void *out, size_t n) {
    SIZE_T got = 0;
    return p && ReadProcessMemory(GetCurrentProcess(), p, out, n, &got) && got == n;
}

static int game_string(const void *p, char out[128]) {
    GameString s;
    if (!read_bytes(p, &s, sizeof(s)) || s.size >= 128 || s.capacity < s.size) return 0;
    if (s.capacity <= 15) memcpy(out, s.local, s.size);
    else if (!read_bytes(s.pointer, out, s.size)) return 0;
    out[s.size] = 0;
    return strlen(out) == s.size;
}

static byte *definition_for(int part, int id) {
    void *manager = NULL;
    if (!read_bytes(base + 0x13c79d0, &manager, sizeof(manager)) || !manager) return NULL;
    uint64_t pair = (uint32_t)part | ((uint64_t)(uint32_t)id << 32);
    return FN(byte *(*)(void *, uint64_t), 0x7bdac0)(manager, pair);
}

static MutationQuality quality(int part, int id) {
    MutationQuality q = {UNKNOWN, {0}};
    byte *definition = definition_for(part, id);
    int type;
    if (!definition || !read_bytes(definition + 0xa8, &type, 4)) return q;
    if (!type) { q.kind = id < 0 ? UNKNOWN : UNMUTATED; return q; }
    if (type != 3) return q;
    uintptr_t start, end;
    if (!read_bytes(definition + 0x38, &start, 8) || !read_bytes(definition + 0x40, &end, 8) ||
        end < start || (end - start) % 0xb0 || (end - start) / 0xb0 > 256 || (!start && end)) return q;
    int special = 0, defect = 0, stats = 0, needs_description = 0, has_description = 0;
    unsigned seen = 0;
    static const char *names[] = {"str", "dex", "con", "int", "spd", "cha", "lck"};
    _Static_assert(COUNT(names) == MUTATION_STATS, "Mutation stat order changed");
    for (uintptr_t p = start; p < end; p += 0xb0) {
        char key[128];
        if (!game_string((void *)(p + 0x88), key) || !read_bytes((void *)(p + 0xa8), &type, 4)) return q;
        if (!strcmp(key, "tag")) {
            char tag[128];
            if (type != 1 || !game_string((void *)(p + 0x68), tag)) return q;
            if (!strcmp(tag, "birth_defect")) defect = 1;
            continue;
        }
        if (!strcmp(key, "name") || !strcmp(key, "desc")) {
            char value[128];
            if (type != 1 || !game_string((void *)(p + 0x68), value)) return q;
            if (!strcmp(key, "desc") && *value) has_description = 1;
            continue;
        }
        unsigned i = 0;
        while (i < COUNT(names) && strcmp(key, names[i])) ++i;
        if (i == COUNT(names)) {
            if (!strcmp(key, "shield") || !strcmp(key, "divine_shield")) {
                double value;
                if (type != 2 || !read_bytes((void *)(p + 0x58), &value, 8) ||
                    !isfinite(value) || fabs(value) > 10000) return q;
            } else if (!strcmp(key, "passives")) {
                if (type != 3) return q;
                needs_description = 1;
            } else if (!strcmp(key, "override_move") || !strcmp(key, "attack")) {
                if (type != 1) return q;
                needs_description = 1;
            } else return q; /* Do not offer an effect the comparison cannot explain. */
            special = 1;
            continue;
        }
        double value;
        if (type != 2 || (seen & (1u << i)) || !read_bytes((void *)(p + 0x58), &value, 8) ||
            !isfinite(value) || fabs(value) > 10000) return q;
        seen |= 1u << i;
        q.stats[i] = value;
        ++stats;
    }
    if (needs_description && !has_description) return q;
    q.kind = special || !stats ? SPECIAL_MUTATION : defect ? BIRTH_DEFECT : STAT_MUTATION;
    return q;
}

#include "breeding.h"
#include "selector.h"

static __attribute__((noinline)) byte random_mutation(void *cat, byte symmetric, GameString *tag, void *filter) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - (uintptr_t)base;
    MutationContext local = {ready && caller == 0x60b1d4, 0, cat};
    MutationContext *previous = context;
    context = &local;
    byte result = original_random(cat, symmetric, tag, filter);
    context = previous;
    return result;
}

static __attribute__((noinline)) byte mutate_piece(void *cat, int part, GameString *tag) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - (uintptr_t)base;
    MutationContext *previous = context;
    int from_combat = (caller == 0xcc67e || caller == 0xcc8ce) && previous &&
        previous->combat && previous->cat == cat;
    MutationContext local = {0, ready && (caller == 0x9338ac || from_combat), cat};
    context = &local;
    /* The original owns and destroys tag; always let its cleanup run. */
    byte result = original_mutate(cat, part, tag);
    context = previous;
    /* A declined/deferred roll is consumed, not reported as a failed roll. */
    return result;
}

static __attribute__((noinline)) void set_piece(void *cat, uint64_t pair) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - (uintptr_t)base;
    int random = caller == 0xccb4f && context && context->protect && context->cat == cat;
    int event = caller == 0x932bfd || caller == 0x933035;
    if (ready && (random || event) && offer_mutation(cat, pair)) return;
    original_set(cat, pair);
}

static int iq_file_matches(const wchar_t *path, const byte expected[32]) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    byte buffer[65536], digest[32];
    DWORD count;
    int ok = 0;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) goto done;
    if (BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0) < 0) goto done;
    for (;;) {
        if (!ReadFile(file, buffer, sizeof(buffer), &count, NULL)) goto done;
        if (!count) break;
        if (BCryptHashData(hash, buffer, count, 0) < 0) goto done;
    }
    if (BCryptFinishHash(hash, digest, 32, 0) >= 0) ok = !memcmp(digest, expected, 32);
done:
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return ok;
}

#include "startup.h"

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    base = (byte *)GetModuleHandleW(NULL);
    HMODULE loader = GetModuleHandleW(L"version.dll");
    if (!loader) return TRUE;
    int (*install)(UINT_PTR,int,void *,void **,int,const char *) =
        (void *)GetProcAddress(loader, "MJ_InstallHook");
    int (*query)(UINT_PTR) = (void *)GetProcAddress(loader, "MJ_QueryHook");
    int (*version)(void) = (void *)GetProcAddress(loader, "MJ_GetVersion");
    log_message = (Logger)(void *)GetProcAddress(loader, "MJ_Log");
    if (!install || !query || !version || !log_message || version() < 3) return TRUE;
    wchar_t executable[MAX_PATH], dll[MAX_PATH];
    DWORD exe_length = GetModuleFileNameW(NULL, executable, MAX_PATH);
    DWORD dll_length = GetModuleFileNameW(module, dll, MAX_PATH);
    if (!exe_length || exe_length >= MAX_PATH || !dll_length || dll_length >= MAX_PATH ||
        !iq_file_matches(executable, EXPECTED_SHA256) || !breeding_compatible()) {
        log_message(OWNER, "DISABLED: unsupported executable or conflicting breeding patches."); return TRUE;
    }
    if (iq_assets_status(dll, GetCommandLineW()) != IQ_ASSETS_READY) {
        log_message(OWNER, "DISABLED: UI assets missing, disabled, ambiguous, or mismatched."); return TRUE;
    }
    /* Mewjector v3 does not relocate short conditional branches. Copy the full
       SetPiece body case so its JNE lands on the trampoline's jump back, not
       inside that jump's address literal (the default 17-byte span crashes). */
    _Static_assert(sizeof(SET_BYTES) == 0x1c, "SetPiece branch boundary changed");
    struct { uintptr_t rva; const byte *expected; void *hook; void **original; int stolen; } hooks[] = {
        {0xa7820, INHERIT_PART_BYTES, (void *)inherit_part, (void **)&original_inherit_part, sizeof(INHERIT_PART_BYTES)},
        {0xa7a00, INHERIT_STAT_BYTES, (void *)inherit_stat, (void **)&original_inherit_stat, sizeof(INHERIT_STAT_BYTES)},
        {0xcc3f0, RANDOM_BYTES, (void *)random_mutation, (void **)&original_random, 0},
        {0xcc970, MUTATE_BYTES, (void *)mutate_piece, (void **)&original_mutate, 0},
        {0xcd080, SET_BYTES, (void *)set_piece, (void **)&original_set, sizeof(SET_BYTES)},
        {0x97bdf0, PANEL_BYTES, (void *)selector_panel, (void **)&original_panel, 0},
        {0x77dc70, CLOSE_BYTES, (void *)selector_close, (void **)&original_close, 0},
        {0x77f5f0, DESTROY_BYTES, (void *)selector_destroy, (void **)&original_destroy, 0},
        {0x5d630, CAT_DESTROY_BYTES, (void *)selector_cat_destroy, (void **)&original_cat_destroy, 0},
        {0x97e330, BUTTON_UPDATE_BYTES, (void *)selector_button_update, (void **)&original_button_update, sizeof(BUTTON_UPDATE_BYTES)}
    };
    for (unsigned i = 0; i < COUNT(hooks); ++i) {
        if (query(hooks[i].rva) || memcmp(base + hooks[i].rva, hooks[i].expected,
                                        hooks[i].stolen ? (size_t)hooks[i].stolen : 16)) {
            log_message(OWNER, "DISABLED: mutation hook conflict at %llx.", (unsigned long long)hooks[i].rva);
            return TRUE;
        }
    }
    for (unsigned i = 0; i < COUNT(CALL_GUARDS); ++i) {
        if (memcmp(base + CALL_GUARDS[i].rva, CALL_GUARDS[i].bytes, 5)) {
            log_message(OWNER, "DISABLED: a mutation call site has changed."); return TRUE;
        }
    }
    for (unsigned i = 0; i < COUNT(hooks); ++i) {
        if (!install(hooks[i].rva, hooks[i].stolen, hooks[i].hook, hooks[i].original, 10, OWNER)) {
            log_message(OWNER, "DISABLED: hook registration failed; registered hooks remain pass-through."); return TRUE;
        }
    }
    if (!apply_breeding()) {
        log_message(OWNER, "DISABLED: breeding patch transaction failed; hooks remain pass-through."); return TRUE;
    }
    ready = 1;
    log_message(OWNER, "0.2.0 enabled: birth inheritance plus event/combat replacement selector. Empty slots and overnight unchanged.");
    return TRUE;
}
