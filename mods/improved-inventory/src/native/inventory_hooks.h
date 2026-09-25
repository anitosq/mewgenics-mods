/* Verify the complete plan first; commit UI activation only on total success.
 * Mewjector v3 has no hook removal API. Installed callbacks must pass through
 * while active is zero, including after a later installation failure. */
typedef struct {
    uintptr_t rva;
    const unsigned char* expected;
    void* callback;
    void** original;
    const char* name;
} IQHook;

static int iq_install_plan(const unsigned char* base,const IQHook* hooks,
                           size_t count,ApiInstall install,int* active) {
    *active=0;
    for(size_t i=0;i<count;i++)
        if(memcmp(base+hooks[i].rva,hooks[i].expected,64))return -1;
    for(size_t i=0;i<count;i++)
        if(!install(hooks[i].rva,0,hooks[i].callback,hooks[i].original,50,hooks[i].name))return 0;
    *active=1;
    return 1;
}
