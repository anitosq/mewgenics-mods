#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned reads,presentations;
static const void* unreadable;
static BOOL counted_read(HANDLE process,LPCVOID from,LPVOID to,SIZE_T n,SIZE_T* got) {
    reads++;
    if(unreadable && (uintptr_t)from<=(uintptr_t)unreadable &&
       (uintptr_t)unreadable-(uintptr_t)from<n){*got=0;return FALSE;}
    return ReadProcessMemory(process,from,to,n,got);
}
#define ReadProcessMemory counted_read
static void report(const char* message) {(void)message;}
#include "../src/native/inventory_layout.h"
static int iq_extended_match(void* drawer,uint64_t id,int sets) {(void)drawer;(void)id;(void)sets;return 1;}
static void iq_filter_view(void) {}
static void iq_ui_update(void) {}
static int iq_ui_event(void* event) {(void)event;return 0;}
static int iq_wheel_side(void) {return -1;}
static void iq_ui_capture(void) {}
static IQItemTraits iq_traits(void* drawer,uint64_t id) {(void)drawer;(void)id;return (IQItemTraits){0};}
static unsigned char original_button_hit(void* p) {(void)p;return 1;}
#include "../src/native/inventory_hit.h"
static struct {uint64_t generation;unsigned char data[0x600];} objects[410];
static unsigned char renderers[400][0x60];
static void pointer_at(void* object,size_t offset,void* value) {memcpy((unsigned char*)object+offset,&value,8);}
static void __cdecl present_stub(void* drawer) {(void)drawer;presentations++;}
static void setup(int count,int equipment,int open) {
    memset(&iq,0,sizeof(iq));memset(objects,0,sizeof(objects));
    for(int i=0;i<410;i++)objects[i].generation=1;
    layout_test=1;filter_popup=0;unreadable=NULL;
    iq.owner=objects[0].data;iq.panels[0]=objects[1].data;iq.panels[1]=objects[2].data;
    iq.owner_ref=iq_reference(iq.owner);iq.panel_refs[0]=iq_reference(iq.panels[0]);
    iq.panel_refs[1]=iq_reference(iq.panels[1]);iq.scene_ref=iq_reference(objects[3].data);
    iq.entity_ref=iq_reference(objects[4].data);iq.manager_ref=iq_reference(objects[5].data);
    pointer_at(iq.owner,0x18,objects[4].data);pointer_at(objects[4].data,8,objects[3].data);
    pointer_at(iq.panels[0],0x20,objects[3].data);pointer_at(iq.panels[0],0x18,objects[4].data);
    pointer_at(objects[5].data,0x58,open?iq.panels[0]:NULL);
    objects[3].data[0x4da]=(unsigned char)!open;
    iq.count=count;iq.equipment=equipment;iq.columns[0]=iq.columns[1]=6;
    iq.span[0]=600;iq.scale[0]=1;
    game_base=(unsigned char*)((uintptr_t)present_stub-(equipment?0x34e230:0x213a20));
    for(int i=0;i<count;i++) {
        void* drawer=objects[i+6].data;uint64_t id=(uint64_t)i+1;
        pointer_at(drawer,0x38,iq.owner);
        pointer_at(drawer,equipment?0x40:0x50,renderers[i]);
        pointer_at(drawer,equipment?0x48:0x58,drawer);
        pointer_at(drawer,equipment?0x50:0x60,drawer);
        memcpy((unsigned char*)drawer+(equipment?0x58:0x68),&id,8);
        iq.drawers[i]=(IQDrawer){.drawer=drawer,.reference=iq_reference(drawer),.item_id=id,.ordinal=i};
    }
    iq_index_view();
}
int main(void) {
    for(int equipment=0;equipment<2;equipment++)for(int open=0;open<2;open++) {
        setup(400,equipment,open);reads=presentations=0;
        for(int i=0;i<iq.count;i++) {
            IQDrawer* d=iq_find(iq.drawers[i].drawer);assert(d==&iq.drawers[i]);iq_present(d);
            int visible=i<(equipment?30:36);
            assert(renderers[i][0x51]==visible);
            double x=0,y=0,scale=0;memcpy(&x,(unsigned char*)d->drawer+(equipment?0xb8:0x78),8);
            memcpy(&y,(unsigned char*)d->drawer+(equipment?0xc0:0x80),8);
            memcpy(&scale,(unsigned char*)d->drawer+(equipment?0xc8:0x88),8);
            assert(x==(visible?90*(i%6)+45:-100000));
            double expected_y=visible?90*(5-i/6)+45-600*0.072:-100000;
            assert(y==(equipment&&visible?600-expected_y:expected_y));assert(scale==0.9);
        }
        assert(presentations==400);assert(reads==(unsigned)(equipment?9:11)*400);
        printf("400 %s items, %s: %u validation/layout reads (previous %u); all native presentations retained.\n",
            equipment?"equipment":"house",open?"open":"covered",reads,(equipment?17:21)*400);
        reads=0;
        for(int i=0;i<iq.count;i++)assert(iq_button_hit(iq.drawers[i].button)==(unsigned char)(!open||i<(equipment?30:36)));
        assert(reads==(unsigned)(400-(equipment?30:36))*(open?(equipment?11:14):(equipment?5:7)));
        IQDrawer* d=&iq.drawers[0];unsigned char* drawer=d->drawer;
        for(int part=0;part<3;part++) {
            size_t offset=(equipment?0x40:0x50)+(size_t)part*8;
            void* saved=iq_ptr(drawer,offset);pointer_at(drawer,offset,NULL);
            presentations=0;iq_present(d);assert(!presentations);pointer_at(drawer,offset,saved);
        }
        unreadable=drawer+(equipment?0x50:0x60)+7;presentations=0;iq_present(d);assert(!presentations);unreadable=NULL;
        uint64_t wrong=999;memcpy(drawer+(equipment?0x58:0x68),&wrong,8);assert(!iq_find(drawer));
        memcpy(drawer+(equipment?0x58:0x68),&d->item_id,8);
        objects[6].generation++;presentations=0;iq_present(d);assert(!presentations);assert(!iq_find(drawer));
        objects[6].generation--;objects[0].generation++;assert(!iq_find(drawer));
    }
    puts("Layout: batching, hidden items, covered views, null/unreadable components and stale identities passed.");
}
