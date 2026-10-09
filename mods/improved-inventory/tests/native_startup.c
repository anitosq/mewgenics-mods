/* Exercise the production callback without a game, loader or save access. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#ifdef NDEBUG
#error Assertions must be enabled.
#endif
static int loader_lookups, renderer_calls;
static HMODULE WINAPI missing_loader(LPCWSTR name) {
    assert(!wcscmp(name,L"version.dll"));
    loader_lookups++;
    return NULL;
}
#define GetModuleHandleW missing_loader
#include "../src/native/inventory_probe.c"
#undef GetModuleHandleW

static void* __cdecl renderer(void* scene,void* entity,const char* name) {
    assert(scene==(void*)1 && entity==(void*)2);
    assert(!strcmp(name,"Inventory"));
    assert(loader_lookups==1); /* Initialization precedes the first original call. */
    renderer_calls++;
    return (void*)3;
}

int main(void) {
    original_renderer=renderer;
    assert(initialized==0 && loader_lookups==0);
    /* No one-time startup event is delivered, as in the failing delayed load. */
    for(int i=0;i<3;i++)assert(observe_renderer((void*)1,(void*)2,"Inventory")== (void*)3);
    assert(initialized==1 && loader_lookups==1 && renderer_calls==3);
    assert(layout_test==0); /* Failed validation leaves UI changes disabled. */
    puts("Late renderer startup initializes once and preserves calls/results after rejection.");
}
