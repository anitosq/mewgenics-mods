/* Exact-build guarded native furniture controls. Diagnostic builds require a test marker. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "guard.h"

typedef unsigned char byte;
typedef void (*Update)(void*);
typedef int (*Install)(UINT_PTR,int,void*,void**,int,const char*);
typedef void (*Log)(const char*,const char*,...);
static Update original_update;
static Install install_hook;
static Log log_message;
static HMODULE own_module;
static byte* base;
static LONG initialized;
static wchar_t command_path[MAX_PATH];
#define FN(type,rva) ((type)(base+(rva)))
#define MAX_ITEMS 2048
#define MARKER_ID UINT64_C(0x4155464e)

typedef struct {
    uint64_t id,variant;
    char name[128],room[128];
    int32_t placement[5]; /* x, y, horizontal scale, vertical scale, order */
    byte* entry;
} Item;
static Item before[MAX_ITEMS], after[MAX_ITEMS], current[MAX_ITEMS];
static unsigned count;
static void *saved_scene,*saved_inventory,*saved_ui;
static uint64_t saved_generation;
static int undo_ready,refresh_pending;
static char ui_status[180];
static unsigned ui_return_count;

static void report(const char* message) {
    if(log_message) log_message("AutoFurniture","%s",message);
    if(!strncmp(message,"UNDO SETTLED:",13)||!strncmp(message,"APPLY PASS:",11))return;
    if(!strncmp(message,"UNDO PASS:",10))message="Previous layout restored.";
    ui_return_count=0;
    snprintf(ui_status,sizeof(ui_status),"%s",message);
}
static int read_bytes(const void* p,void* out,size_t n) {
    SIZE_T got=0;
    return p && ReadProcessMemory(GetCurrentProcess(),p,out,n,&got) && got==n;
}
static void* ptr(const void* p,size_t offset) {
    void* result=NULL;
    if(p) read_bytes((const byte*)p+offset,&result,sizeof(result));
    return result;
}
static int integer(const void* p,size_t offset) {
    int n=0;if(p)read_bytes((const byte*)p+offset,&n,4);return n;
}
static int rooms(void* scene,byte** out,unsigned* n) {
    byte* list=ptr(ptr(scene,0x20),0x3ed0);int size=integer(list,12);byte** data=ptr(list,16);
    if(size<1||size>32)return 0;*n=0;
    for(int i=0;i<size;i++) {
        byte* room=ptr(data,(size_t)i*8);byte dead=1;
        if(!room||!read_bytes(room+15,&dead,1))return 0;
        if(!dead)out[(*n)++]=room;
    }
    return *n>0;
}
static int string(const byte* s,char out[128]) {
    uint64_t length,capacity;
    if(!read_bytes(s+16,&length,8)||!read_bytes(s+24,&capacity,8)||length>=128||capacity<length) return 0;
    const void* data=capacity>15?ptr(s,0):s;
    if(length&&!read_bytes(data,out,(size_t)length))return 0;
    out[length]=0;
    return strlen(out)==length;
}
static int capture(Item* items,unsigned* n,void** inventory) {
    byte* inv=ptr(base,0x13da9a0);
    unsigned size=0;
    if(!read_bytes(inv?inv+0x3c:NULL,&size,4)||size>MAX_ITEMS||!size)return 0;
    byte** entries=ptr(inv,0x40);
    int marked=0;
    for(unsigned i=0;i<size;i++) {
        Item* v=&items[i]; memset(v,0,sizeof(*v));
        v->entry=ptr(entries,i*8);
        if(!v->entry||!read_bytes(v->entry,&v->id,8)||
           !read_bytes(v->entry+0x28,&v->variant,8)||
           !string(v->entry+8,v->name)||!string(v->entry+0x30,v->room)||
           !read_bytes(v->entry+0x50,v->placement,20))return 0;
        for(unsigned j=0;j<i;j++)if(items[j].id==v->id)return 0;
        if(v->id==MARKER_ID && !v->room[0])marked=1;
    }
    *n=size;*inventory=inv;
#ifdef AF_RELEASE
    (void)marked;return 1;
#else
    return marked;
#endif
}
static int equal(const Item* a,const Item* b) {
    return a->id==b->id&&a->variant==b->variant&&!strcmp(a->name,b->name)&&
        !strcmp(a->room,b->room)&&!memcmp(a->placement,b->placement,20)&&a->entry==b->entry;
}
static int return_snapshot(const Item* source,Item* destination,unsigned n,const char* room) {
    unsigned placed=0;
    for(unsigned i=0;i<n;i++)if(source[i].room[0])placed++;
    /* The native house compacts drawing ranks after removing entities. */
    for(unsigned i=0;i<n;i++)if(source[i].room[0]) {
        int order=source[i].placement[4];
        if(order<0||(unsigned)order>=placed)return 0;
        for(unsigned j=0;j<i;j++)if(source[j].room[0]&&source[j].placement[4]==order)return 0;
    }
    memcpy(destination,source,n*sizeof(Item));
    for(unsigned i=0;i<n;i++) {
        if(!strcmp(source[i].room,room))destination[i].room[0]=0;
        else if(source[i].room[0]) {
            int order=0;
            for(unsigned j=0;j<n;j++)if(source[j].room[0]&&strcmp(source[j].room,room)&&
                source[j].placement[4]<source[i].placement[4])order++;
            destination[i].placement[4]=order;
        }
    }
    return 1;
}
static int unchanged(const Item* expected) {
    unsigned n=0;void* inv=NULL;
    if(!capture(current,&n,&inv)||n!=count||inv!=saved_inventory)return 0;
    for(unsigned i=0;i<count;i++) {
        unsigned j=0;while(j<n&&current[j].id!=expected[i].id)j++;
        if(j==n||!equal(&expected[i],&current[j]))return 0;
    }
    return 1;
}
static int pieces(void* scene,byte** out,unsigned* n) {
    byte* list=ptr(ptr(scene,0x20),0x3f00);
    unsigned size=0;
    if(!read_bytes(list?list+12:NULL,&size,4)||size>MAX_ITEMS)return 0;
    byte** data=ptr(list,16);
    *n=0;
    for(unsigned i=0;i<size;i++) {
        byte* p=ptr(data,i*8);byte state[2];
        if(!p||!read_bytes(p+0x2b0,state,2))return 0;
        if(state[0]||state[1])return 0; /* Never transfer during a drag. */
        byte dead=0;if(!read_bytes(p+15,&dead,1))return 0;
        if(!dead)out[(*n)++]=p;
    }
    return 1;
}
/* Operation-local snapshot; never reuse across native scene mutations. */
static void piece_entries(byte* const* live,unsigned n,byte** entries) {
    for(unsigned i=0;i<n;i++)entries[i]=ptr(live[i],0x2d8);
}
static void refresh(void* ui) {
    *((byte*)ui+0x58)=1;
    original_update(ui); /* Rebuild placed IDs before rebuilding the drawer. */
    FN(void(*)(void*,int),0x1a6680)(ui,0);
}
#include "transaction.h"
static void dump(void) {
    unsigned n;void* inventory;
    if(!capture(current,&n,&inventory)){report("DISABLED: disposable-save marker absent or state unreadable.");return;}
    for(unsigned i=0;i<n;i++) {
        Item* v=&current[i];char message[400];
        snprintf(message,sizeof(message),"item id=%llu name=%s variant=%llu room=%s x=%d y=%d sx=%d sy=%d order=%d",
            (unsigned long long)v->id,v->name,(unsigned long long)v->variant,v->room,
            v->placement[0],v->placement[1],v->placement[2],v->placement[3],v->placement[4]);report(message);
    }
    report("DUMP COMPLETE: disposable campaign verified.");
}
#include "planner.h"
#include "ui.h"
static int hash_matches(void);
static int assets_match(void);
static void update(void* ui) {
    original_update(ui);
    if(InterlockedCompareExchange(&initialized,1,0)==0) {
        initialized=hash_matches()&&assets_match()?2:3;
        report(initialized==2?"Auto Furniture " AF_VERSION " ready.":
            "Inactive: executable or enabled UI assets do not match this build.");
        /* Shared input hooks must wait until other mods finish their byte guards.
           The full executable hash was checked above; Mewjector chains hooks. */
        if(initialized==2&&install_hook(0xc36110,0,(void*)ui_input,(void**)&original_input,40,"AutoFurniture.Input")&&
           install_hook(0x97f0e0,0,(void*)ui_hit,(void**)&original_hit,40,"AutoFurniture.Hit")&&
           install_hook(0x1a0880,0,(void*)ui_furniture_hover,(void**)&original_furniture_hover,40,"AutoFurniture.Hover"))ui_enabled=1;
    }
    if(initialized!=2)return;
    poll_search();
    if(saved_ui && (ui!=saved_ui||ptr(ptr(ui,0x18),8)!=saved_scene)) {
        undo_ready=0;refresh_pending=0;saved_ui=NULL;
        InterlockedExchange(&plan_cancel,1);plan_ready=0;
        report("Room changed. Calculate again.");
    }
    if(refresh_pending&&!--refresh_pending){refresh(ui);transaction_verify();}
    ui_update(ui);
#ifdef AF_RELEASE
    return;
#endif
    static ULONGLONG last_poll;
    ULONGLONG now=GetTickCount64();if(now-last_poll<200)return;last_poll=now;
    HANDLE f=CreateFileW(command_path,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return;
    char command[160]={0};DWORD got=0;BOOL ok=ReadFile(f,command,sizeof(command)-1,&got,NULL);CloseHandle(f);
    if(!DeleteFileW(command_path)||!ok||!got)return;
    command[strcspn(command,"\r\n")]=0;
    if(!strncmp(command,"return ",7))return_room(ui,command+7);
    else if(!strcmp(command,"undo"))undo(ui);
    else if(!strcmp(command,"apply"))apply_plan(ui);
    else if(!strcmp(command,"dump"))dump();
    else if(!strncmp(command,"preview ",8)) {
        char room[128];unsigned selected=0,targets=0;double minimum[AF_STATS]={0};
        int fields=sscanf(command+8,"%127s %u %u %lf %lf %lf %lf %lf",room,&selected,&targets,
                  &minimum[0],&minimum[1],&minimum[2],&minimum[3],&minimum[4]);
        if(fields==7||fields==8)preview(ui,room,selected,targets,minimum,0,NULL);
        else report("Expected preview ROOM SELECTED_MASK TARGET_MASK C S H M [A]");
    }
    else report("Unknown diagnostic command.");
}
__declspec(dllexport) int AutoFurnitureProbeSelfTest(void) {
    /* Exercise the actual snapshot reader without a game process or hooks. */
    byte* real_base=base;
    byte* fake=VirtualAlloc(NULL,0x13daa00,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!fake)return 1;
    byte inventory[0x48]={0},entry[0x68]={0};byte* entries[]={entry,entry};
    uint64_t id=MARKER_ID,capacity=15;unsigned n=1;void* inv=NULL;int result=0;
    memcpy(fake+0x13da9a0,&(void*){inventory},8);
    memcpy(inventory+0x40,&(void*){entries},8);memcpy(inventory+0x3c,&n,4);
    memcpy(entry,&id,8);memcpy(entry+0x20,&capacity,8);memcpy(entry+0x48,&capacity,8);
    base=fake;
    if(!capture(current,&n,&inv)||n!=1||inv!=inventory)result|=2;
    Item copy=current[0];
    if(!equal(&copy,&current[0]))result|=4;
    copy.variant=2;if(equal(&copy,&current[0]))result|=8;
    copy=current[0];copy.placement[4]=1;if(equal(&copy,&current[0]))result|=16;
    copy=current[0];strcpy(copy.room,"Attic");if(equal(&copy,&current[0]))result|=32;
    id=42;memcpy(entry,&id,8);
#ifdef AF_RELEASE
    if(!capture(current,&n,&inv))result|=64;
#else
    if(capture(current,&n,&inv))result|=64;
#endif
    id=MARKER_ID;memcpy(entry,&id,8);n=2;memcpy(inventory+0x3c,&n,4);
    if(capture(current,&n,&inv))result|=128;
    n=1;memcpy(inventory+0x3c,&n,4);uint64_t bad_length=128;memcpy(entry+0x18,&bad_length,8);
    if(capture(current,&n,&inv))result|=256;
    Item source[4]={0},expected[4];
    strcpy(source[0].room,"A");source[0].placement[4]=0;
    strcpy(source[1].room,"A");source[1].placement[4]=2;
    strcpy(source[2].room,"B");source[2].placement[4]=1;
    source[3].placement[4]=99;source[2].variant=2;
    if(!return_snapshot(source,expected,4,"A")||expected[0].room[0]||expected[1].room[0]||
       expected[2].placement[4]!=0||expected[2].variant!=2||expected[3].placement[4]!=99||
       source[2].placement[4]!=1)result|=512;
    source[2].placement[4]=2;
    if(return_snapshot(source,expected,4,"A"))result|=1024;
    ui_state.selected=0;ui_state.focus=9;ui_state.action=2;ui_state.pin_page=3;
    memset(ui_state.minimum,'1',sizeof(ui_state.minimum));memset(ui_state.maximum,'2',sizeof(ui_state.maximum));
    include_utilities=1;pinned_count=2;plan_ready=1;plan_cancel=0;undo_ready=1;
    ui_reset();
    if(ui_state.selected!=AF_ALL_STATS||ui_state.focus!=-1||ui_state.action||ui_state.pin_page||
       include_utilities||pinned_count||plan_ready||!plan_cancel||!undo_ready)result|=2048;
    for(int i=0;i<AF_STATS;i++)if(ui_state.minimum[i][0]||ui_state.maximum[i][0])result|=2048;
    undo_ready=0;
    for(unsigned i=0;i<sizeof(panel_boxes)/sizeof(*panel_boxes);i++) {
        const UIBox* b=&panel_boxes[i];double point[2]={b->x+b->w/2,b->y+b->h/2};
        if(ui_target(panel_boxes,24,point)!=(int)i)result|=4096;
    }
    if(ui_available(1,19)||ui_available(1,20)||!ui_available(1,18))result|=4096;
    ui_state.selected=0;if(ui_available(1,6)||ui_available(1,18))result|=4096;
    ui_state.selected=AF_ALL_STATS;plan_ready=1;
    if(!ui_available(1,19))result|=4096;
    plan_worker=(HANDLE)1;
    for(int i=18;i<24;i++)if(ui_available(1,i))result|=4096;
    if(!ui_available(1,17))result|=4096;
    plan_worker=NULL;plan_ready=0;ui_state.pin_page=0;ui_state.pin_total=10;
    if(ui_available(2,1)||!ui_available(2,2))result|=4096;
    ui_state.pin_page=1;if(!ui_available(2,1)||ui_available(2,2))result|=4096;
    ui_state.pin_page=ui_state.pin_total=0;
    if(panel_boxes[22].y<522+20||panel_boxes[23].y!=panel_boxes[22].y)result|=8192;
    report("Returned 2 items to inventory.");
    report("APPLY PASS: verified.");
    if(strcmp(ui_status,"Returned 2 items to inventory."))result|=8192;
    report("UNDO PASS: verified.");report("UNDO SETTLED: verified.");
    if(strcmp(ui_status,"Previous layout restored."))result|=8192;
    for(int i=0;i<3;i++) {
        problem.targets=i?1:0;problem.minimum[0]=10;
        problem.caps=i==2?1:0;problem.maximum[0]=5;search.best.stats[0]=9;
        plan_cancel=0;plan_worker=CreateEventW(NULL,TRUE,TRUE,NULL);
        if(!plan_worker){result|=8192;break;}
        poll_search();
        const char* expected=i==2?"No layout found within Max. Raise a limit or unpin furniture.":
            i==1?"Calculation complete. Some Min targets are still unmet.":"Calculation complete.";
        if(plan_worker||!plan_ready||strcmp(ui_status,expected))result|=8192;
    }
    problem.targets=problem.caps=0;problem.minimum[0]=problem.maximum[0]=search.best.stats[0]=0;
    ui_invalidate();
    base=real_base;VirtualFree(fake,0,MEM_RELEASE);return result;
}
static int iq_file_matches(const wchar_t* path,const byte* expected) {
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(f==INVALID_HANDLE_VALUE)return 0;
    BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;byte buf[65536],digest[32];DWORD n;int ok=0;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    if(BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
    for(;;){if(!ReadFile(f,buf,sizeof(buf),&n,NULL))goto done;if(!n)break;if(BCryptHashData(hash,buf,n,0)<0)goto done;}
    if(BCryptFinishHash(hash,digest,32,0)>=0)ok=!memcmp(digest,expected,32);
done:
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(f);return ok;
}
#include "startup.h"
static int hash_matches(void) {
    wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);
    return n&&n<MAX_PATH&&iq_file_matches(path,EXPECTED_SHA256);
}
static int assets_match(void) {
#ifdef AF_RELEASE
    wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(own_module,path,MAX_PATH);
    return n&&n<MAX_PATH&&AutoFurnitureValidateAssetsW(path,GetCommandLineW());
#else
    return 1;
#endif
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if(reason!=DLL_PROCESS_ATTACH)return TRUE;
    own_module=module;DisableThreadLibraryCalls(module);base=(byte*)GetModuleHandleW(NULL);
    HMODULE loader=GetModuleHandleW(L"version.dll");if(!loader)return TRUE;
    Install install=(Install)(void*)GetProcAddress(loader,"MJ_InstallHook");
    install_hook=install;
    int(*version)(void)=(int(*)(void))(void*)GetProcAddress(loader,"MJ_GetVersion");
    log_message=(Log)(void*)GetProcAddress(loader,"MJ_Log");
    if(!install||!version||version()<3)return TRUE;
#ifndef AF_RELEASE
    DWORD length=GetModuleFileNameW(module,command_path,MAX_PATH);
    if(!length||length>=MAX_PATH-32)return TRUE;
    wchar_t* slash=wcsrchr(command_path,L'\\');if(!slash)return TRUE;
    wcscpy(slash+1,L"AutoFurnitureProbe.session");
    WIN32_FILE_ATTRIBUTE_DATA info;FILETIME now;
    if(!GetFileAttributesExW(command_path,GetFileExInfoStandard,&info))return TRUE;
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER t,w;t.LowPart=now.dwLowDateTime;t.HighPart=now.dwHighDateTime;
    w.LowPart=info.ftLastWriteTime.dwLowDateTime;w.HighPart=info.ftLastWriteTime.dwHighDateTime;
    if(t.QuadPart<w.QuadPart||t.QuadPart-w.QuadPart>300ULL*10000000ULL)return TRUE;
    wcscpy(slash+1,L"AutoFurnitureProbe.command");
#endif
    IMAGE_DOS_HEADER* dos=(IMAGE_DOS_HEADER*)base;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return TRUE;
    IMAGE_NT_HEADERS64* pe=(IMAGE_NT_HEADERS64*)(base+dos->e_lfanew);
    if(pe->Signature!=IMAGE_NT_SIGNATURE||pe->FileHeader.TimeDateStamp!=0x6a9c51e5||
       pe->OptionalHeader.SizeOfImage!=0x1574000||memcmp(base+0x1a5ab0,UPDATE_BYTES,64)||
       memcmp(base+0xc36110,INPUT_BYTES,64)||memcmp(base+0x97f0e0,HIT_BYTES,64)||
       memcmp(base+0x1a0880,HOVER_BYTES,64))return TRUE;
    install(0x1a5ab0,0,(void*)update,(void**)&original_update,50,"AutoFurniture");
    return TRUE;
}
