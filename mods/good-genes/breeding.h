/* Birth-only policy, derived from the supported game's inheritance helpers.
   Part IDs are selected before coat propagation; birth gates prevent mirroring. */
typedef void (*InheritPart)(void *, const void *, const void *, uintptr_t, uintptr_t, double, double);
typedef void (*InheritStat)(int *, const int *, const int *, double, double, double);
static InheritPart original_inherit_part;
static InheritStat original_inherit_stat;

#include <intrin.h>
#include "skills.h"

typedef struct { GameString name; int upgrade; } NativeSkillName;
_Static_assert(sizeof(NativeSkillName) == 0x28, "Native skill normalizer also writes an upgrade at +0x20");

static int collect_inherited_skills(const byte *parent, int passive, InheritedSkills *pool) {
    unsigned slots = passive ? 2 : 4;
    for (unsigned i = 0; i < slots; ++i) {
        const void *source = parent + (passive ? 0x910 + i*0x28 : 0x810 + i*0x20);
        char name[INHERITED_SKILL_NAME];
        if (!game_string(source, name)) return 0;
        if (!passive && *name && strcmp(name, "None")) {
            NativeSkillName normalized;
            FN(NativeSkillName *(*)(NativeSkillName *, const void *, int), 0x94b320)(&normalized, source, 0);
            int valid = game_string(&normalized.name, name);
            FN(void (*)(GameString *), 0x52780)(&normalized.name);
            if (!valid) return 0;
        }
        if (!add_inherited_skill(pool, name, passive)) return 0;
    }
    return 1;
}

static int roll_inherited_skill(unsigned count, int excluded, void *rng) {
    if (!count || (count == 1 && excluded >= 0)) return -1;
    double roll = FN(double (*)(double, void *), 0x1595a0)(1.0, rng);
    return inherited_skill_index(count, excluded, roll);
}

static void inherit_skills(byte *child, const byte *first, const byte *second) {
    InheritedSkills active = {0}, passive = {0};
    byte *tls = NULL;
    uint64_t rng_state[4];
    if (!collect_inherited_skills(first, 0, &active) ||
        !collect_inherited_skills(second, 0, &active) ||
        !collect_inherited_skills(first, 1, &passive) ||
        !collect_inherited_skills(second, 1, &passive) ||
        !read_bytes((void *)__readgsqword(0x58), &tls, sizeof(tls)) || !tls ||
        !read_bytes(tls+0x178, rng_state, sizeof(rng_state))) {
        log_message(OWNER, "Skill inheritance unavailable; keeping initialized kitten skills.");
        return;
    }
    int a = roll_inherited_skill(active.count, -1, tls+0x178);
    int b = active.count > 1 ? roll_inherited_skill(active.count, a, tls+0x178) : -1;
    int p = roll_inherited_skill(passive.count, -1, tls+0x178);
    if ((active.count && a < 0) || (active.count > 1 && b < 0) || (passive.count && p < 0)) {
        log_message(OWNER, "Invalid inheritance roll; keeping initialized kitten skills.");
        return;
    }
    /* Native active selection removes upgrades. */
    int selected[2] = {a, b};
    for (unsigned i = 0; i < 2 && selected[i] >= 0; ++i) {
        const char *name = active.names[selected[i]];
        FN(void *(*)(void *, const char *, size_t), 0x520d0)(child+0x810+i*0x20, name, strlen(name));
    }
    if (p >= 0) {
        const char *name = passive.names[p];
        GameString reference = {.pointer = name, .size = strlen(name), .capacity = INHERITED_SKILL_NAME};
        GameString owned;
        FN(GameString *(*)(GameString *, const GameString *), 0x52460)(&owned, &reference);
        /* Native setter consumes its string and applies passive-specific state. */
        FN(void (*)(void *, GameString *, int, int), 0xc3a00)(child, &owned, 0, 1);
    }
    /* Passive setters can also grant actives. Refresh all birth copies after
       that setter, matching the constructor's original ordering. */
    for (unsigned i = 0; i < 4; ++i) {
        GameString *source = (GameString *)(child+0x810+i*0x20);
        const char *name = source->capacity <= 15 ? source->local : source->pointer;
        FN(void *(*)(void *, const char *, size_t), 0x520d0)(child+0x890+i*0x20, name, source->size);
    }
    log_message(OWNER, "Birth skill picks: active=[%s, %s] passive=[%s]; eligible pools=%u/%u.",
                a >= 0 ? active.names[a] : "unchanged",
                b >= 0 ? active.names[b] : "unchanged",
                p >= 0 ? passive.names[p] : "unchanged", active.count, passive.count);
}

static int has_inherited_mutation(const byte *part) {
    int kind;
    byte *definition = definition_for(*(const int *)part,
                                     part[0x18] ? *(const int *)(part+4) : 0);
    return definition && read_bytes(definition+0xa8, &kind, sizeof(kind)) ? kind != 0 : -1;
}

static void choose_part(void *child, const void *first, const void *second,
                        uintptr_t unused4, uintptr_t unused5, double common, double defect) {
    int a = has_inherited_mutation(first), b = has_inherited_mutation(second);
    int parent = inherited_parent(a, b);
    if (parent >= 0) {
        const byte *source = parent ? second : first;
        memcpy((byte *)child+4, source+4, sizeof(int));
    } else {
        if (a < 0 || b < 0)
            log_message(OWNER, "Birth part=%d: unknown parental definition; using native inheritance.",
                        *(const int *)first);
        /* Equal mutation occupancy uses the game's fair roll, not effect ranking.
           Unknown definitions retain the native arguments and behavior. */
        original_inherit_part(child, first, second, unused4, unused5,
                              a >= 0 && b >= 0 ? 0 : common,
                              a >= 0 && b >= 0 ? 0 : defect);
    }
}

static __attribute__((noinline)) void inherit_part(void *child, const void *first, const void *second,
                         uintptr_t unused4, uintptr_t unused5, double common, double defect) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - (uintptr_t)base;
    if (!ready || caller < 0xa993a || caller > 0xa9bec) {
        original_inherit_part(child, first, second, unused4, unused5, common, defect);
        return;
    }
    if (caller == 0xa993a) {
        /* First head-part call is unconditional with the guarded loss gate.
           Skill Share and parental disorders have run; final sprite refresh has not. */
        inherit_skills((byte *)child-0x8c, (const byte *)first-0x8c, (const byte *)second-0x8c);
        /* Coat inheritance is inlined in the constructor. Correct it at the
           first part call, before all part coat fields are copied from +0x78. */
        _Alignas(int) byte coat[3][0x1c] = {{0}};
        for (unsigned i = 0; i < 3; ++i) {
            *(int *)coat[i] = 10;
            coat[i][0x18] = 1;
        }
        memcpy(coat[1]+4, (const byte *)first-0x14, sizeof(int));
        memcpy(coat[2]+4, (const byte *)second-0x14, sizeof(int));
        choose_part(coat[0], coat[1], coat[2], 0, 0, common, defect);
        memcpy((byte *)child-0x14, coat[0]+4, sizeof(int));
    }
    choose_part(child, first, second, unused4, unused5, common, defect);
    if (caller == 0xa9bec) {
        const byte *cat = (const byte *)child-0x4d0;
        log_message(OWNER, "Birth part IDs before head visibility: body=%d head=%d tail=%d mouth=%d coat=%d.",
                    *(const int *)(cat+0x90), *(const int *)(cat+0xe4),
                    *(const int *)(cat+0x138), *(const int *)(cat+0x4d4), *(const int *)(cat+0x78));
        log_message(OWNER, "Birth paired IDs (left/right), mirroring disabled: arms=%d/%d legs=%d/%d eyes=%d/%d eyebrows=%d/%d ears=%d/%d.",
                    *(const int *)(cat+0x18c), *(const int *)(cat+0x1e0),
                    *(const int *)(cat+0x234), *(const int *)(cat+0x288),
                    *(const int *)(cat+0x2dc), *(const int *)(cat+0x330),
                    *(const int *)(cat+0x384), *(const int *)(cat+0x3d8),
                    *(const int *)(cat+0x42c), *(const int *)(cat+0x480));
    }
}

static __attribute__((noinline)) void inherit_stat(int *child, const int *first, const int *second,
                                                   double a, double b, double bias) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - (uintptr_t)base;
    if (ready && caller >= 0xa8e92 && caller <= 0xa8fa0) {
        *child = *first > *second ? *first : *second;
        if (caller == 0xa8fa0)
            log_message(OWNER, "Birth base stats (STR DEX CON INT SPD CHA LCK): %d %d %d %d %d %d %d; higher parental values applied.",
                        child[-6], child[-5], child[-4], child[-3], child[-2], child[-1], child[0]);
    } else original_inherit_stat(child, first, second, a, b, bias);
}

static int breeding_compatible(void) {
    for (unsigned i = 0; i < COUNT(BREEDING_GUARDS); ++i) {
        byte digest[32];
        BCRYPT_ALG_HANDLE algorithm = NULL;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) return 0;
        NTSTATUS status = BCryptHash(algorithm, NULL, 0, base+BREEDING_GUARDS[i].rva,
                                    BREEDING_GUARDS[i].size, digest, sizeof(digest));
        BCryptCloseAlgorithmProvider(algorithm, 0);
        if (status < 0 || memcmp(digest, BREEDING_GUARDS[i].sha256, sizeof(digest))) return 0;
    }
    return 1;
}

static int apply_breeding(void) {
    DWORD protections[COUNT(BIRTH_GATES)];
    unsigned writable = 0;
    for (; writable < COUNT(BIRTH_GATES); ++writable)
        if (!VirtualProtect(base+BIRTH_GATES[writable].rva, BIRTH_GATES[writable].size,
                            PAGE_EXECUTE_READWRITE, &protections[writable])) break;
    int ok = writable == COUNT(BIRTH_GATES);
    if (ok) {
        for (unsigned i = 0; i < COUNT(BIRTH_GATES); ++i)
            memcpy(base+BIRTH_GATES[i].rva, BIRTH_GATES[i].after, BIRTH_GATES[i].size);
        ok = FlushInstructionCache(GetCurrentProcess(), NULL, 0) != 0;
        if (!ok) {
            for (unsigned i = 0; i < COUNT(BIRTH_GATES); ++i)
                memcpy(base+BIRTH_GATES[i].rva, BIRTH_GATES[i].before, BIRTH_GATES[i].size);
            FlushInstructionCache(GetCurrentProcess(), NULL, 0);
        }
    }
    while (writable) {
        unsigned i = --writable;
        DWORD ignored;
        if (!VirtualProtect(base+BIRTH_GATES[i].rva, BIRTH_GATES[i].size, protections[i], &ignored))
            log_message(OWNER, "Warning: could not restore a birth code page's protection.");
    }
    return ok;
}
