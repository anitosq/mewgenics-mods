#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#ifdef NDEBUG
#error UI checks require assertions
#endif

static unsigned reads,hits;
static BOOL counted_read(HANDLE process,LPCVOID source,LPVOID target,SIZE_T size,SIZE_T* got) {
    reads++;
    return ReadProcessMemory(process,source,target,size,got);
}
#define ReadProcessMemory counted_read
#include "native.c"

static unsigned char native_hit(void* button) {(void)button;hits++;return 42;}

int main(void) {
    byte owner_memory[0x100]={0},scene[0x500]={0},entity[16]={0};
    byte renderer_memory[0x100]={0},transform[0xb0]={0};
    byte* owner=owner_memory+8;byte* renderer=renderer_memory+8;
    uint64_t gen=7;
    memcpy(owner_memory,&gen,8);memcpy(renderer_memory,&gen,8);
    memcpy(owner+0x18,&(void*){entity},8);memcpy(entity+8,&(void*){scene},8);
    memcpy(renderer+0x40,&(void*){transform},8);
    initialized=2;ui_enabled=1;original_hit=native_hit;
    ui_state.owner=owner;ui_state.scene=scene;ui_state.generation=gen;ui_state.allowed=1;
    owner[0x78]=1;

    reads=hits=0;
    for(int i=0;i<400;i++)assert(ui_hit(NULL)==42);
    printf("Closed panel, 400 hits: %u reads, %u native calls\n",reads,hits);
    assert(reads==0&&hits==400);
    ui_state.modal=1;reads=hits=0;
    assert(ui_hit(NULL)==0&&reads==4&&hits==0);
    scene[0x4b0]=1;
    assert(ui_hit(NULL)==42&&hits==1);
    scene[0x4b0]=0;ui_state.generation++;
    assert(ui_hit(NULL)==42&&hits==2);
    ui_state.generation=gen;ui_state.modal=0;

    Control* c=&ui_state.panel;c->renderer=renderer;c->generation=gen;
    control_show(c,"",1,2,0.5,42);
    assert(renderer[0x51]==1);
    reads=0;control_hide(c);
    assert(renderer[0x51]==0&&reads==2);
    reads=0;control_hide(c);assert(reads==0);
    control_show(c,"",3,4,0.5,42);
    assert(renderer[0x51]==1);
    /* A recycled renderer must not be written through a retained reference. */
    gen++;memcpy(renderer_memory,&gen,8);reads=0;control_hide(c);
    assert(renderer[0x51]==1&&reads==1);
    control_hide(c);assert(reads==1);
    c->generation=gen;control_show(c,"",3,4,0.5,42);
    renderer[15]=1;control_hide(c);assert(renderer[0x51]==1);
    renderer[15]=0;control_show(c,"",3,4,0.5,42);

    owner[0x78]=0;ui_state.modal=1;plan_cancel=0;
    reads=0;ui_update(owner);
    assert(!renderer[0x51]&&!ui_state.modal&&plan_cancel&&reads==7);
    reads=0;
    for(int i=0;i<400;i++)ui_update(owner);
    printf("Inactive UI, 400 updates: %u reads\n",reads);
    assert(reads==2000);
    /* Closing again after a re-open still hides the panel and cancels work. */
    control_show(c,"",1,2,0.5,42);ui_state.modal=1;plan_cancel=0;
    ui_close();assert(!renderer[0x51]&&!ui_state.modal&&plan_cancel);
    puts("Auto Furniture UI checks passed.");
    return 0;
}
