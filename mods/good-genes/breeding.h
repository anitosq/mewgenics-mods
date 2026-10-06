/* Birth-only policy, derived from the supported game's inheritance helpers.
   Part IDs are selected before the game propagates coat IDs and mirrors limbs. */
typedef void (*InheritPart)(void *, const void *, const void *, uintptr_t, uintptr_t, double, double);
typedef void (*InheritStat)(int *, const int *, const int *, double, double, double);
static InheritPart original_inherit_part;
static InheritStat original_inherit_stat;

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
}

static __attribute__((noinline)) void inherit_stat(int *child, const int *first, const int *second,
                                                   double a, double b, double bias) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0) - (uintptr_t)base;
    if (ready && caller >= 0xa8e92 && caller <= 0xa8fa0)
        *child = *first > *second ? *first : *second;
    else original_inherit_stat(child, first, second, a, b, bias);
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
