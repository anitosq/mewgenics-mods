#undef NDEBUG
#include <assert.h>
#include "native.c"

static DWORD WINAPI thread_attach_check(void* unused) {
    (void)unused;
    return DllMain(NULL, DLL_THREAD_ATTACH, NULL) ? 0 : 1;
}
static void* root_result(void* root) { return root; }
static void source_trampoline_check(void) {
    const byte* code = GUARDS[1].bytes;
    assert(GUARDS[1].rva == 0x475c0);
    assert(code[10] == 0x74 && code[29] == 0x75);
    assert(12 + (int8_t)code[11] == SOURCE_STOLEN);
    assert(31 + (int8_t)code[30] == 16);
    byte* copy = VirtualAlloc(NULL, SOURCE_STOLEN + 14, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(copy);
    memcpy(copy, code, SOURCE_STOLEN);
    memcpy(copy + SOURCE_STOLEN, "\xff\x25\0\0\0\0", 6);
    void* tail = (void*)root_result;
    memcpy(copy + SOURCE_STOLEN + 6, &tail, 8);
    DWORD old;
    assert(VirtualProtect(copy, SOURCE_STOLEN + 14, PAGE_EXECUTE_READ, &old));
    assert(FlushInstructionCache(GetCurrentProcess(), copy, SOURCE_STOLEN + 14));
    byte root[0x710] = {0}, child[0x710] = {0}, grandchild[0x710] = {0};
    void* r = root; void* c = child;
    memcpy(child + 0x708, &r, 8); memcpy(grandchild + 0x708, &c, 8);
    SourceItem run = (SourceItem)(void*)copy;
    assert(run(root) == root && run(child) == root && run(grandchild) == root);
    assert(VirtualFree(copy, 0, MEM_RELEASE));
}
static void item(byte* p, const char* name, int32_t id) {
    memset(p, 0, 0x60);
    size_t length = strlen(name);
    assert(length <= 15);
    memcpy(p + 8, name, length);
    uint64_t len = length, cap = 15;
    memcpy(p + 0x18, &len, 8);
    memcpy(p + 0x20, &cap, 8);
    memcpy(p + 0x4c, &id, 4);
}
static void ptr(byte* p, unsigned offset, void* value) { memcpy(p + offset, &value, 8); }
static void* lookup_fixture(void* database, int64_t id) {
    assert(database);
    return id == 7 ? database : NULL;
}
static void ordinary_campaign_check(void) {
    byte* saved_base = base;
    base = VirtualAlloc(NULL, 0x13dac40, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(base);
    byte game[0x600] = {0}, inventory[0xf8] = {0}, head[0x18] = {0}, node[0x78] = {0};
    byte backpack[0x18] = {0}, caster[0x90] = {0}, board[0xc0] = {0}, ability[0x18] = {0};
    Vector cell = {0};
    ptr(base, 0x13dac30, game); ptr(game, 0x588, inventory); ptr(game, 0x598, game);
    ptr(inventory, 0x40, backpack); ptr(backpack, 0, backpack);
    ptr(inventory, 0x80, head); uint64_t count = 1; memcpy(inventory + 0x88, &count, 8);
    ptr(head, 0, node); ptr(node, 0, head); item(node + 0x18, "SoulJar_Full", 7);
    ptr(ability, 0x10, caster); ptr(caster, 0x80, board); ptr(board, 0xb0, &cell);
    int dimensions[] = {1, 1}; memcpy(board + 0xb8, dimensions, 8);
    memcpy(base + 0xd7220, "\xff\x25\0\0\0\0", 6);
    ptr(base, 0xd7226, (void*)lookup_fixture);
    DWORD old;
    assert(VirtualProtect(base + 0xd7000, 0x1000, PAGE_EXECUTE_READ, &old));
    assert(FlushInstructionCache(GetCurrentProcess(), base + 0xd7220, 14));
    Souls result;
    /* No SoulWell marker exists anywhere in this inventory. */
    assert(capture(&result, ability) && result.count == 1 && result.ids[0] == 7);
    item(node + 0x18, "SoulJar_Full", 123);
    assert(capture(&result, ability) && !result.count && result.invalid == 1);
    item(node + 0x18, "SoulJar", -1);
    assert(capture(&result, ability) && !result.count);
    assert(VirtualFree(base, 0, MEM_RELEASE)); base = saved_base;
}
static int install_count, fail_install, spawn_calls, point_calls;
static void pass_spawn(void* callback) { (void)callback; spawn_calls++; }
static void pass_point(void* ability, byte tiles, byte effects) {
    (void)ability; (void)tiles; (void)effects; point_calls++;
}
static int mock_install(UINT_PTR rva, int bytes, void* hook, void** original, int priority, const char* name) {
    (void)hook; (void)priority; (void)name;
    if (++install_count == fail_install) return 0;
    if (rva == 0x43350) *original = (void*)pass_point;
    else if (rva == 0x475c0) { assert(bytes == SOURCE_STOLEN); *original = (void*)root_result; }
    else { assert(rva == 0x7a2ba0); *original = (void*)pass_spawn; }
    return 1;
}
static void hook_failure_check(void) {
    for (fail_install = 1; fail_install <= 3; fail_install++) {
        install_count = 0;
        assert(!activate_hooks(mock_install) && !hooks_ready && install_count == fail_install);
        if (fail_install > 1) { point_calls = 0; castpoint(NULL, 0, 0); assert(point_calls == 1); }
        if (fail_install > 2) assert(source_item((void*)123) == (void*)123);
    }
    fail_install = 0; install_count = 0;
    assert(activate_hooks(mock_install) && hooks_ready && install_count == 3);
    spawn_calls = 0; spawn(NULL); assert(spawn_calls == 1); /* Unrelated source passes through. */
    hooks_ready = 0;
}
static void passive_edit_check(void) {
    char body[4096];
    snprintf(body, sizeof(body), "%s\n SafeDoomed 1 InjuryImmunity 1\n%s", ABILITY_PREFIX, ABILITY_SUFFIX);
    assert(ability_text_matches(body, strlen(body)));
    snprintf(body, sizeof(body), "%s\n // } is a comment\n Added { key \"}\\\"\" } /* } */\n%s", ABILITY_PREFIX, ABILITY_SUFFIX);
    assert(ability_text_matches(body, strlen(body)));
    snprintf(body, sizeof(body), "%s}\n spawn { object Other }\n%s", ABILITY_PREFIX, ABILITY_SUFFIX);
    assert(!ability_text_matches(body, strlen(body)));
    snprintf(body, sizeof(body), "%s /* unfinished %s", ABILITY_PREFIX, ABILITY_SUFFIX);
    assert(!ability_text_matches(body, strlen(body)));
}
int main(void) {
    source_trampoline_check();
    ordinary_campaign_check();
    hook_failure_check();
    passive_edit_check();
    byte one[0x60], duplicate[0x60], other[0x60], empty[0x60], well[0x60];
    item(one, "SoulJar_Full", 7); item(duplicate, "SoulJar_Full", 7);
    item(other, "SoulJar_Full", 0); item(empty, "SoulJar", 18);
    item(well, "SoulWell", -1);
    Souls souls = {0};
    assert(add_item(&souls, one, 1) && add_item(&souls, duplicate, 0));
    assert(add_item(&souls, other, 2) && add_item(&souls, empty, 1));
    assert(add_item(&souls, well, 1));
    assert(souls.count == 2 && souls.duplicates == 1);
    assert(souls.stored == 1 && souls.carried == 1 && souls.equipped == 1);
    qsort(souls.ids, souls.count, sizeof(*souls.ids), compare_id);
    assert(souls.ids[0] == 0 && souls.ids[1] == 7);
    byte before[0x60]; memcpy(before, one, sizeof(before));
    assert(add_item(&souls, one, 1)); assert(!memcmp(one, before, sizeof(before)));
    item(other, "SoulJar_Full", -1); assert(add_item(&souls, other, 1));
    assert(souls.count == 2 && souls.invalid == 1);
    /* Exact map node layout: list links, key, then ItemInfo. */
    byte manager[0xf8] = {0}, head[0x18] = {0}, node[0x78] = {0};
    void* h = head; void* n = node; uint64_t count = 1;
    memcpy(manager + 0x80, &h, 8); memcpy(manager + 0x88, &count, 8);
    memcpy(head, &n, 8); memcpy(node, &h, 8); memcpy(node + 0x18, one, 0x60);
    Souls scanned = {0};
    assert(scan_container(&scanned, manager, 0x78, 1) && scanned.count == 1 && scanned.ids[0] == 7);
    count = 2; memcpy(manager + 0x88, &count, 8);
    assert(!scan_container(&scanned, manager, 0x78, 1));
    count = 1; memcpy(manager + 0x88, &count, 8); memcpy(node, &n, 8);
    assert(!scan_container(&scanned, manager, 0x78, 1));
    memset(&scanned, 0, sizeof(scanned)); scanned.count = MAX_SOULS;
    for (unsigned i = 0; i < MAX_SOULS; i++) scanned.ids[i] = (int32_t)i;
    item(other, "SoulJar_Full", MAX_SOULS);
    assert(!add_item(&scanned, other, 1));
    assert(!named((void*)1, "SoulWell"));
    HANDLE thread = CreateThread(NULL, 65536, thread_attach_check, NULL, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    assert(thread && WaitForSingleObject(thread, 10000) == WAIT_OBJECT_0);
    DWORD code = 1; assert(GetExitCodeThread(thread, &code) && !code); CloseHandle(thread);
    puts("PASS: ordinary campaign capture, partial hooks, passive edits, trampoline, deduplication, invalid jars, preservation and bounds.");
    return 0;
}
