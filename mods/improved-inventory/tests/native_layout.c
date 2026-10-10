#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned reads,presentations,native_updates,ui_updates;
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
static void iq_ui_update(void) {ui_updates++;}
static int iq_ui_event(void* event) {(void)event;return 0;}
static int iq_wheel_side(void) {return -1;}
static void iq_ui_capture(void) {}
static IQItemTraits iq_traits(void* drawer,uint64_t id) {(void)drawer;(void)id;return (IQItemTraits){0};}
static unsigned char original_button_hit(void* p) {(void)p;return 1;}
#include "../src/native/inventory_hit.h"
static struct {uint64_t generation;unsigned char data[0x600];} objects[1034];
static unsigned char renderers[1024][0x60],transforms[1024][0xb0];
static double panel_origin[2],native_alpha=0.2;
static void pointer_at(void* object,size_t offset,void* value) {memcpy((unsigned char*)object+offset,&value,8);}
static void __cdecl present_stub(void* drawer) {
    void* transform=NULL;double target[3];
    memcpy(&transform,(unsigned char*)drawer+(iq.equipment?0x50:0x60),8);
    memcpy(target,(unsigned char*)drawer+(iq.equipment?0xb8:0x78),sizeof(target));
    double state[]={target[0]+(iq.equipment?0:panel_origin[0]),target[1]+(iq.equipment?0:panel_origin[1]),0,target[2],target[2]};
    memcpy((unsigned char*)transform+0x80,state,sizeof(state));presentations++;
}
static void __cdecl update_stub(void* drawer) {
    void* renderer=NULL;memcpy(&renderer,(unsigned char*)drawer+(iq.equipment?0x40:0x50),8);
    void* transform=NULL;double target[3],state[5];
    memcpy(&transform,(unsigned char*)drawer+(iq.equipment?0x50:0x60),8);
    memcpy(target,(unsigned char*)drawer+(iq.equipment?0xb8:0x78),sizeof(target));
    memcpy(state,(unsigned char*)transform+0x80,sizeof(state));
    for(int field=0;field<5;field++) {
        double next=field<2?target[field]+(iq.equipment?0:panel_origin[field]):field==2?0:target[2];
        state[field]=field==2?0:next*native_alpha+state[field]*(1-native_alpha);
    }
    memcpy((unsigned char*)transform+0x80,state,sizeof(state));
    /* Equipment can show its renderer again before our presentation hook. */
    if(iq.equipment)*((unsigned char*)renderer+0x51)=1;
    native_updates++;
}
static void update_all(void) {
    for(int i=0;i<iq.count;i++) {
        if(!iq.equipment)iq_update(iq.drawers[i].drawer);
        else {update_stub(iq.drawers[i].drawer);IQDrawer* d=iq_find(iq.drawers[i].drawer);if(d)iq_present(d);}
    }
}
static void setup(int count,int equipment,int open) {
    memset(&iq,0,sizeof(iq));memset(objects,0,sizeof(objects));
    memset(renderers,0,sizeof(renderers));memset(transforms,0,sizeof(transforms));
    panel_origin[0]=panel_origin[1]=0;native_alpha=0.2;
    for(int i=0;i<1034;i++)objects[i].generation=1;
    original_drawer_update=update_stub;
    layout_test=1;filter_popup=0;unreadable=NULL;
    iq.owner=objects[0].data;iq.panels[0]=objects[1].data;iq.panels[1]=objects[2].data;
    iq.owner_ref=iq_reference(iq.owner);iq.panel_refs[0]=iq_reference(iq.panels[0]);
    iq.panel_refs[1]=iq_reference(iq.panels[1]);iq.scene_ref=iq_reference(objects[3].data);
    iq.entity_ref=iq_reference(objects[4].data);iq.manager_ref=iq_reference(objects[5].data);
    pointer_at(iq.owner,0x18,objects[4].data);pointer_at(objects[4].data,8,objects[3].data);
    pointer_at(iq.panels[0],0x20,objects[3].data);pointer_at(iq.panels[0],0x18,objects[4].data);
    pointer_at(objects[5].data,0x58,open?iq.panels[0]:NULL);
    objects[3].data[0x4da]=(unsigned char)!open;
    iq.count=count;iq.items[0]=count;iq.equipment=equipment;iq.columns[0]=iq.columns[1]=6;
    iq.span[0]=600;iq.scale[0]=1;
    game_base=(unsigned char*)((uintptr_t)present_stub-(equipment?0x34e230:0x213a20));
    for(int i=0;i<count;i++) {
        void* drawer=objects[i+6].data;uint64_t id=(uint64_t)i+1;
        pointer_at(drawer,0x38,iq.owner);
        pointer_at(drawer,equipment?0x40:0x50,renderers[i]);
        pointer_at(drawer,equipment?0x48:0x58,drawer);
        pointer_at(drawer,equipment?0x50:0x60,transforms[i]);
        memcpy((unsigned char*)drawer+(equipment?0x58:0x68),&id,8);
        iq.drawers[i]=(IQDrawer){.drawer=drawer,.reference=iq_reference(drawer),.item_id=id,.ordinal=i};
    }
    iq_index_view();
}
int main(void) {
    /* Closing without a rebuild must stop mod layout, but keep native cleanup. */
    setup(400,0,1);update_all();
    pointer_at(objects[5].data,0x58,NULL);
    reads=presentations=native_updates=ui_updates=0;update_all();
    assert(!presentations && native_updates==400 && ui_updates==1);
    assert(reads==400);
    reads=0;assert(iq_button_hit(iq.drawers[100].button));assert(reads==1);
    pointer_at(objects[5].data,0x58,iq.panels[1]);
    presentations=0;iq_update(iq.drawers[35].drawer);assert(presentations==1);
    /* Fresh manager checks handle reuse, unreadable state and another panel. */
    for(int fault=0;fault<5;fault++) {
        setup(37,0,1);update_all();
        if(fault==0)objects[5].generation++;
        if(fault==1)pointer_at(objects[5].data,0,(void*)(uintptr_t)123);
        if(fault==2)unreadable=objects[5].data+0x58;
        if(fault==3)pointer_at(objects[5].data,0x58,objects[4].data);
        if(fault==4)iq.manager_ref.pointer=NULL;
        reads=presentations=native_updates=0;iq_update(iq.drawers[36].drawer);
        assert(!presentations && native_updates==1 && reads==(unsigned)(fault!=4));
        reads=0;assert(iq_button_hit(iq.drawers[36].button));assert(reads==(unsigned)(fault!=4));
    }
    setup(37,0,1);update_all();
    /* Covered, still-active inventory retains presentation; stale views cannot write. */
    objects[3].data[0x4da]=1;presentations=0;update_all();assert(presentations==36);
    objects[3].generation++;presentations=0;update_all();assert(!presentations);
    objects[3].generation--;
    iq_index_clear(&iq_drawer_index);iq_index_clear(&iq_button_index);
    reads=presentations=native_updates=0;iq_update(iq.drawers[36].drawer);
    assert(!reads && !presentations && native_updates==1);
    assert(iq_button_hit(iq.drawers[36].button));assert(!reads);
    const int sizes[]={0,1,16,35,36,37,513};
    for(unsigned size=0;size<sizeof(sizes)/sizeof(sizes[0]);size++) {
        int count=sizes[size];setup(count,0,1);update_all();
        for(int cycle=0;cycle<3;cycle++) {
            pointer_at(objects[5].data,0x58,NULL);
            reads=presentations=native_updates=ui_updates=0;update_all();
            assert(!presentations && native_updates==(unsigned)count && reads==(unsigned)count);
            assert(ui_updates==(unsigned)(count!=0));
            iq_scroll(0,-10000);
            pointer_at(objects[5].data,0x58,iq.panels[cycle%2]);
            presentations=0;update_all();
            assert(presentations==(unsigned)(count<36?count:36));
            for(int i=0;i<count;i++)assert(renderers[i][0x51]==(i<36));
            int row=iq.row[0];iq_scroll(0,1);if(count<=36)assert(iq.row[0]==row);
        }
    }
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
        assert(presentations==400);assert(reads==(unsigned)((equipment?6:8)*400+400-(equipment?30:36)));
        reads=presentations=native_updates=ui_updates=0;update_all();
        assert(presentations==(unsigned)(equipment?30:open?36:0));assert(native_updates==400);
        assert(ui_updates==(unsigned)!equipment);
        assert(reads==(unsigned)(equipment?6*400+400-30:open?9*400+400-36:400));
        for(int i=0;i<400;i++)assert(renderers[i][0x51]==(i<(equipment?30:36)));
        printf("400 %s items, %s: %u guarded reads (0.3.6: %u), %u instant layouts (0.3.6: 400); 400 native updates retained.\n",
            equipment?"equipment":"house",open?"open":equipment?"covered":"closed",reads,(equipment?9:11)*400,presentations);
        reads=0;
        for(int i=0;i<iq.count;i++)assert(iq_button_hit(iq.drawers[i].button)==(unsigned char)(!open||i<(equipment?30:36)));
        assert(reads==(unsigned)(400-(equipment?30:36))*(open?(equipment?11:15):(equipment?5:1)));
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
    for(int equipment=0;equipment<2;equipment++) {
        setup(1024,equipment,1);update_all();
        reads=presentations=native_updates=0;update_all();
        assert(presentations==(unsigned)(equipment?30:36));assert(native_updates==1024);
        assert(reads==(unsigned)((equipment?6:9)*1024+1024-(equipment?30:36)));
        double rates[]={0.1,0.15,1.0/6,0.2,0.25,1.0/3,0.5,0.9};
        for(int rate=0;rate<8;rate++) {
            native_alpha=rates[rate];
            for(int frame=0;frame<10;frame++) {
                presentations=0;update_all();assert(presentations==(unsigned)(equipment?30:36));
            }
        }

        /* Every native transform field must invalidate a settled hidden item. */
        IQDrawer* hidden=&iq.drawers[100];
        for(int field=0;field<5;field++) {
            iq_write_double(transforms[100],0x80+8*field,123);
            presentations=0;iq_present(hidden);assert(presentations==1);
            double corrected=0;memcpy(&corrected,transforms[100]+0x80+8*field,8);
            assert(corrected==(field<2?-100000:field==2?0:0.9));
            presentations=0;iq_present(hidden);assert(!presentations);
        }
        iq_write_double(transforms[100],0x98,0.9+2e-9);
        presentations=0;iq_present(hidden);assert(presentations==1);
        iq_write_double(transforms[100],0x80,NAN);
        presentations=0;iq_present(hidden);assert(presentations==1);
        iq_write_double(transforms[100],0x80,INFINITY);
        presentations=0;iq_present(hidden);assert(presentations==1);
        if(!equipment) {
            panel_origin[0]=123.45;panel_origin[1]=56.78;
            presentations=0;update_all();assert(presentations==1024);
            presentations=0;update_all();assert(presentations==36);
            panel_origin[0]=panel_origin[1]=0;update_all();
        }
        iq.scale[0]=2;presentations=0;iq_present(hidden);assert(presentations==1);
        iq.scale[0]=1;iq_present(hidden);
        if(equipment) {
            iq.left[0]=10;iq.bottom[0]=20;presentations=0;iq_present(hidden);assert(presentations==1);
            iq.left[0]=iq.bottom[0]=0;iq_present(hidden);
        }
        /* A replacement component cannot inherit another transform's cache. */
        unsigned char replacement[0xb0]={0};memcpy(replacement,transforms[100],sizeof(replacement));
        pointer_at(hidden->drawer,equipment?0x50:0x60,replacement);
        presentations=0;iq_present(hidden);assert(presentations==1);
        pointer_at(hidden->drawer,equipment?0x50:0x60,transforms[100]);
        presentations=0;iq_present(hidden);assert(presentations==1);
        unreadable=transforms[100]+0x80;presentations=0;iq_present(hidden);assert(presentations==1);
        unreadable=NULL;presentations=0;iq_present(hidden);assert(presentations==1);
        presentations=0;iq_present(hidden);assert(!presentations);
        uint64_t wrong=9999;memcpy((unsigned char*)hidden->drawer+(equipment?0x58:0x68),&wrong,8);
        presentations=0;iq_present(hidden);assert(!presentations);assert(!iq_find(hidden->drawer));
        memcpy((unsigned char*)hidden->drawer+(equipment?0x58:0x68),&hidden->item_id,8);
        pointer_at(hidden->drawer,0x38,NULL);assert(!iq_find(hidden->drawer));
        presentations=0;iq_present(hidden);assert(!presentations);pointer_at(hidden->drawer,0x38,iq.owner);
        pointer_at(hidden->drawer,0,(void*)(uintptr_t)123);assert(!iq_find(hidden->drawer));
        presentations=0;iq_present(hidden);assert(!presentations);pointer_at(hidden->drawer,0,hidden->reference.vtable);
        unreadable=(unsigned char*)hidden->drawer+(equipment?0x58:0x68)+7;
        presentations=0;iq_present(hidden);assert(!presentations);assert(!iq_find(hidden->drawer));unreadable=NULL;

        /* Scrolling touches the old/new windows, including leaving cells. */
        setup(1024,equipment,1);update_all();reads=presentations=0;
        iq_scroll(0,1);int visible=equipment?30:36;
        assert(presentations==(unsigned)(visible+6));assert(reads==(unsigned)(visible+12));
        for(int i=0;i<1024;i++)assert(renderers[i][0x51]==(i>=6 && i<visible+6));
        reads=presentations=0;iq_scroll(0,0);assert(!reads && !presentations);
        reads=presentations=0;iq_scroll(0,20);
        assert(presentations==(unsigned)(visible*2));assert(reads==(unsigned)(visible*3));
        for(int i=0;i<1024;i++)assert(renderers[i][0x51]==(i>=126 && i<126+visible));
        iq_scroll(0,10000);assert(iq.row[0]==iq_scroll_limit(0));
        reads=presentations=0;iq_scroll(0,1);assert(!reads && !presentations);
        iq_scroll(0,-10000);assert(!iq.row[0]);
        reads=presentations=0;iq_scroll(0,-1);assert(!reads && !presentations);

        /* Filtering/recapture and empty views discard or replace layout state. */
        hidden=&iq.drawers[100];hidden->ordinal=0;presentations=0;iq_present(hidden);assert(presentations==1);
        hidden->ordinal=-1;presentations=0;iq_present(hidden);assert(presentations==1);
        presentations=0;iq_present(hidden);assert(!presentations);
        setup(1024,equipment,1);presentations=0;update_all();assert(presentations==1024);
        setup(0,equipment,1);reads=presentations=0;iq_scroll(0,1);update_all();assert(!reads && !presentations);
    }
    /* Trash can have fewer columns; scrolling it must leave Storage alone. */
    setup(80,0,1);iq.items[0]=iq.items[1]=40;iq.columns[1]=4;iq.span[1]=400;iq.scale[1]=1;
    for(int i=40;i<80;i++){iq.drawers[i].side=1;iq.drawers[i].ordinal=i-40;}
    update_all();unsigned char storage[40][0xb0];memcpy(storage,transforms,sizeof(storage));
    reads=presentations=0;iq_scroll(1,1);assert(presentations==20 && reads==24);
    assert(!memcmp(storage,transforms,sizeof(storage)));assert(!iq.row[0] && iq.row[1]==1);
    for(int i=40;i<80;i++)assert(renderers[i][0x51]==(i>=44 && i<60));
    puts("Layout: settled hidden items, native updates, scroll windows, animation changes, rebinds and stale identities passed.");
}
