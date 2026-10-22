/* Beta-only, read-only discovery for the adventure Backpack screen.
 * Nothing here changes layout, items or input: probes only log bounded,
 * de-duplicated observations of native UI objects the mod does not manage.
 * Stable builds compile none of it. */
#include <stdint.h>
#define IQ_SEEN_CAPACITY 1024
typedef struct { uint64_t keys[IQ_SEEN_CAPACITY]; int count; } IQSeen;
/* Returns 1 the first time a key is seen; 0 for repeats or once full. */
static int iq_seen_insert(IQSeen* seen,uint64_t key) {
    for(int i=0;i<seen->count;i++)if(seen->keys[i]==key)return 0;
    if(seen->count==IQ_SEEN_CAPACITY)return 0;
    seen->keys[seen->count++]=key;
    return 1;
}
static uint64_t iq_probe_hash(const char* text,uint64_t seed) {
    uint64_t hash=UINT64_C(1469598103934665603)^seed;
    for(;text && *text;text++){hash^=(unsigned char)*text;hash*=UINT64_C(1099511628211);}
    return hash;
}
#if IQ_ADVENTURE_PROBE
static IQSeen iq_probe_renderers,iq_probe_owners;
static int iq_probe_grids;
/* Code addresses are logged relative to the executable, matching the RVAs in
   the research notes; anything outside the image logs as 0. */
static uintptr_t iq_probe_rva(const void* p) {
    uintptr_t at=(uintptr_t)p,base=(uintptr_t)game_base;
    return base && at>=base && at-base<EXPECTED_IMAGE_SIZE?at-base:0;
}
static uintptr_t iq_probe_vtable(const void* object) {return iq_probe_rva(iq_ptr(object,0));}
static void iq_probe_renderer(const char* name,const void* caller) {
    char readable[96];
    if(!read_name(name,readable,sizeof(readable)) ||
       !iq_seen_insert(&iq_probe_renderers,iq_probe_hash(readable,iq_probe_rva(caller))))return;
    char message[200];
    snprintf(message,sizeof(message),"Adventure probe: renderer %s from %llx (%llu ms).",readable,
             (unsigned long long)iq_probe_rva(caller),(unsigned long long)GetTickCount64());
    report(message);
}
/* side 0: house drawer callback; 1: equipment drawer callback. */
static void iq_probe_drawer(void* drawer,int side) {
    static uint64_t last;
    void* owner=iq_ptr(drawer,0x38); /* IQDrawerState.owner, both drawer kinds */
    uint64_t key=((uint64_t)(uintptr_t)owner<<1)|(unsigned)side;
    /* Per-frame callbacks repeat one owner; skip the set scan for those. */
    if(key==last)return;
    last=key;
    if(!iq_seen_insert(&iq_probe_owners,key))return;
    char message[200];
    snprintf(message,sizeof(message),"Adventure probe: unmanaged %s drawer vtable %llx, owner %p vtable %llx (%llu ms).",
             side?"equipment":"house",(unsigned long long)iq_probe_vtable(drawer),owner,
             (unsigned long long)iq_probe_vtable(owner),(unsigned long long)GetTickCount64());
    report(message);
}
/* Equipment grid calls outside pre-adventure selection (mode 1). */
static void iq_probe_equipment_grid(void* owner,const void* caller) {
    if(iq_probe_grids>=60)return;
    iq_probe_grids++;
    char message[400];int at=snprintf(message,sizeof(message),
        "Adventure probe: equipment grid owner %p vtable %llx from %llx, ints 0x50-0xc8:",
        owner,(unsigned long long)iq_probe_vtable(owner),(unsigned long long)iq_probe_rva(caller));
    for(size_t off=0x50;off<=0xc8 && at>0 && at<(int)sizeof(message)-12;off+=4)
        at+=snprintf(message+at,sizeof(message)-(size_t)at," %d",iq_int(owner,off));
    report(message);
}
#endif
