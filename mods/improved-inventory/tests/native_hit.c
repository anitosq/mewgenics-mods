#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "../src/native/inventory_index.h"
static unsigned long long reads,scene_checks,native_calls;
static int iq_read(const void* p,void* out,size_t n) {
    SIZE_T received=0;reads++;
    return p && ReadProcessMemory(GetCurrentProcess(),p,out,n,&received) && received==n;
}
#include "../src/native/inventory_lifetime.h"
typedef struct {void* drawer;void* button;IQReference reference;uint64_t item_id;int ordinal;} IQDrawer;
static struct {int count,equipment;IQDrawer drawers[1024];} iq;
static struct {uint64_t generation;unsigned char data[128];} objects[1024];
static int buttons[1152],layout_test=1,filter_popup,scene_open=1;
static IQIndex iq_button_index;
static int iq_is_open(void) {scene_checks++;return scene_open;}
static int iq_visible(const IQDrawer* d) {return d->ordinal>=0 && d->ordinal<30;}
static void* iq_drawer_button(void* drawer) {
    void* value=NULL;iq_read((unsigned char*)drawer+(iq.equipment?0x48:0x58),&value,sizeof(value));return value;
}
static unsigned char original_button_hit(void* button) {(void)button;native_calls++;return 1;}
#include "../src/native/inventory_hit.h"
/* Previous implementation, retained only to measure guarded memory reads for
 * identical stable inputs. Timing here is a microbenchmark, not game FPS. */
static unsigned char legacy_hit(void* button) {
    if(layout_test && iq_is_open())
        for(int i=0;i<iq.count;i++)
            if((filter_popup || !iq_visible(&iq.drawers[i])) &&
               iq_reference_valid(iq.drawers[i].reference) && iq_drawer_button(iq.drawers[i].drawer)==button)return 0;
    return original_button_hit(button);
}
static void setup(int count,int equipment) {
    memset(&iq,0,sizeof(iq));iq_index_clear(&iq_button_index);
    iq.count=count;iq.equipment=equipment;scene_open=1;layout_test=1;filter_popup=0;
    for(int i=0;i<count;i++) {
        objects[i].generation++;
        void* identity=(void*)(uintptr_t)0x1234;void* button=&buttons[i];uint64_t id=(uint64_t)i+1;
        memcpy(objects[i].data,&identity,sizeof(identity));
        memcpy(objects[i].data+(equipment?0x48:0x58),&button,sizeof(button));
        memcpy(objects[i].data+(equipment?0x58:0x68),&id,sizeof(id));
        IQDrawer* d=&iq.drawers[i];
        *d=(IQDrawer){objects[i].data,button,iq_reference(objects[i].data),id,i};
        assert(iq_index_put(&iq_button_index,button,i));
    }
}
int main(void) {
    /* Real collisions, absent keys, updates and complete reset. */
    IQIndex index={0};size_t bucket=iq_index_slot((void*)(uintptr_t)16);int collisions=0;
    for(uintptr_t p=16;collisions<20;p+=16)if(iq_index_slot((void*)p)==bucket){
        assert(iq_index_put(&index,(void*)p,collisions));
        assert(iq_index_get(&index,(void*)p)==collisions++);
    }
    assert(iq_index_put(&index,(void*)(uintptr_t)16,77));assert(iq_index_get(&index,(void*)(uintptr_t)16)==77);
    iq_index_clear(&index);assert(iq_index_get(&index,(void*)(uintptr_t)16)==-1);
    for(int equipment=0;equipment<2;equipment++) {
        setup(1024,equipment);
        for(int popup=0;popup<2;popup++) {
            filter_popup=popup;
            reads=scene_checks=0;
            for(int i=0;i<1152;i++)assert(iq_button_hit(&buttons[i])==(unsigned char)(i>=1024 || (!popup && i<30)));
            assert(reads<=4*1024);assert(scene_checks<=1024);
        }
        reads=scene_checks=0;assert(iq_button_hit(&buttons[1100]));assert(!reads && !scene_checks);
        scene_open=0;assert(iq_button_hit(&buttons[100]));scene_open=1;
        objects[100].generation++;assert(iq_button_hit(&buttons[100]));
        uint64_t different=9999;memcpy(objects[101].data+(equipment?0x58:0x68),&different,8);
        assert(iq_button_hit(&buttons[101]));
        void* replacement=&buttons[1101];memcpy(objects[102].data+(equipment?0x48:0x58),&replacement,sizeof(replacement));
        assert(iq_button_hit(&buttons[102]));
        iq.drawers[103].reference.pointer=(void*)(uintptr_t)1;assert(iq_button_hit(&buttons[103]));
        iq_index_clear(&iq_button_index);assert(iq_button_hit(&buttons[104]));
        setup(1,equipment);filter_popup=1;assert(!iq_button_hit(&buttons[0]));assert(iq_button_hit(&buttons[104]));
        iq.count=0;assert(iq_button_hit(&buttons[0]));
    }
    setup(94,1);filter_popup=1;
    LARGE_INTEGER frequency,start,end;QueryPerformanceFrequency(&frequency);
    for(int mode=0;mode<2;mode++) {
        reads=scene_checks=native_calls=0;QueryPerformanceCounter(&start);
        for(int frame=0;frame<60;frame++)for(int i=0;i<150;i++) {
            unsigned char result=mode?iq_button_hit(&buttons[i]):legacy_hit(&buttons[i]);
            assert(result==(unsigned char)(i>=94));
        }
        QueryPerformanceCounter(&end);
        printf("%s: 60 fixture frames, 94 items / 150 buttons: %llu reads, %llu scene checks, %.3f ms total\n",
            mode?"Indexed":"Previous",reads,scene_checks,1000.0*(end.QuadPart-start.QuadPart)/frequency.QuadPart);
    }
    puts("Hit testing: visible, hidden, popup, unrelated, stale, rebound, destroyed and rebuilt views passed.");
    return 0;
}
